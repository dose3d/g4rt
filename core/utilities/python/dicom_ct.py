import datetime
import json
import os
import re
from pathlib import Path

import numpy as np
import pandas as pd
import pydicom
import pydicom.uid
from pydicom.uid import ExplicitVRLittleEndian


# =============================================================================
# CT/DICOM export constants
# =============================================================================

DICOM_UID_PREFIX = "1.2.826.0.1.3680043.8.498.997."
DICOM_SLICE_UID_PREFIX = "1.2.826.0.1.3680043.8.498.997.1997."

HU_MIN = -1024
HU_MAX = 3071

DEFAULT_UNKNOWN_MATERIAL_HU = -1000
DEFAULT_UNKNOWN_MATERIAL_NAME = "UNKNOWN"

HU_DICTIONARY_RELATIVE_PATH = "config/hounsfield_scale_120keV.json"
CT_TEMPLATE_RELATIVE_PATH = "config/ct_slice_template.dcm"

METADATA_FILENAME = "ct_series_metadata.csv"
MATERIAL_COLUMN = "Material"

DICOM_LABEL_DEFAULT = "CT_"

DICOM_MANUFACTURER = "Dose3D"
DICOM_INSTITUTION_NAME = "AGH WFiIS"
DICOM_INSTITUTION_ADDRESS = "Kraków"
DICOM_SOFTWARE_VERSION = "DICOMaker v. alpha v0.19.33"
DICOM_SOURCE_AE_TITLE = "Dose-3D"

DICOM_PATIENT_ID = "048517244102"
DICOM_PATIENT_BIRTH_DATE = "20000101"
DICOM_PATIENT_POSITION = "HFS"

DICOM_SERIES_DESCRIPTION = "Synthetic CT generated from G4RT geometry"
DICOM_STUDY_DESCRIPTION = "Synthetic CT export"


class CtSvc:
    __output_path = None
    __project_path = None

    # -------------------------------------------------------------------------
    # Init
    # -------------------------------------------------------------------------

    def __init__(self, label=DICOM_LABEL_DEFAULT):
        print("Initializing CT scanner.")
        print()

        self.__label = label

        self.__data_path = ""

        self.__x_min = 0.0
        self.__x_max = 0.0
        self.__y_min = 0.0
        self.__y_max = 0.0
        self.__z_min = 0.0
        self.__z_max = 0.0

        self.__pixel_in_x = 0
        self.__pixel_in_y = 0
        self.__pixel_in_z = 0

        self.__step_x = 0.0
        self.__step_y = 0.0
        self.__step_z = 0.0

        self.__SSD = 0.0

        self.__hounsfield_units_dictionary = {}
        self.__image_type_dictionary = {}

        self.series = None
        self.instance_UID = None

    # -------------------------------------------------------------------------
    # Public path setters
    # -------------------------------------------------------------------------

    @classmethod
    def set_output_path(cls, output_path):
        cls.__output_path = str(output_path)

        if cls.__output_path[-1] != "/":
            cls.__output_path += "/"

        os.makedirs(cls.__output_path, exist_ok=True)

        print(f"Output was set to: {cls.__output_path}")

    @classmethod
    def set_project_path(cls, project_path):
        cls.__project_path = str(project_path)

        if cls.__project_path[-1] != "/":
            cls.__project_path += "/"

        if not os.path.isdir(cls.__project_path):
            raise NotADirectoryError(
                f"Project root path does not exist or is not a directory: "
                f"{cls.__project_path}"
            )

        print(f"Project root was set to: {cls.__project_path}")

    # -------------------------------------------------------------------------
    # Configuration loading
    # -------------------------------------------------------------------------

    def __require_paths(self):
        if not self.__output_path:
            raise RuntimeError(
                "CT output path was not set. "
                "Call CtSvc.set_output_path(...) before create_ct_series(...)."
            )

        if not self.__project_path:
            raise RuntimeError(
                "Project path was not set. "
                "Call CtSvc.set_project_path(...) before create_ct_series(...)."
            )

    def __set_hounsfield_dictionary(self):
        dictionary_path = Path(self.__project_path) / HU_DICTIONARY_RELATIVE_PATH

        if not dictionary_path.is_file():
            raise FileNotFoundError(
                f"Hounsfield dictionary file not found: {dictionary_path}"
            )

        with open(dictionary_path, "r", encoding="utf-8") as jsn_file:
            loaded = json.load(jsn_file)

        if not isinstance(loaded, list) or len(loaded) < 1:
            raise ValueError(
                f"Invalid Hounsfield dictionary format in {dictionary_path}. "
                f"Expected JSON list: [hu_dictionary, image_type_dictionary]."
            )

        self.__hounsfield_units_dictionary = loaded[0]

        if len(loaded) > 1:
            self.__image_type_dictionary = loaded[1]
        else:
            self.__image_type_dictionary = {}

        print(f"Loaded Hounsfield dictionary from: {dictionary_path}")
        print(
            f"Known HU materials: "
            f"{len(self.__hounsfield_units_dictionary)}"
        )

    # -------------------------------------------------------------------------
    # Metadata
    # -------------------------------------------------------------------------

    def __get_metadata_from_file(self):
        metadata_path = Path(self.__data_path) / METADATA_FILENAME

        if not metadata_path.is_file():
            raise FileNotFoundError(f"Missing CT metadata file: {metadata_path}")

        data = pd.read_csv(
            metadata_path,
            header=None,
            index_col=0,
            names=["key", "value"]
        )

        data_dict = data["value"].to_dict()

        def get_float(key):
            try:
                return round(float(data_dict[key]), 4)
            except KeyError:
                raise KeyError(f"Missing metadata key: {key} in {metadata_path}")
            except ValueError:
                raise ValueError(
                    f"Metadata key '{key}' should be numeric, "
                    f"but got value: {data_dict[key]!r}"
                )

        def get_int(key):
            try:
                return int(round(float(data_dict[key])))
            except KeyError:
                raise KeyError(f"Missing metadata key: {key} in {metadata_path}")
            except ValueError:
                raise ValueError(
                    f"Metadata key '{key}' should be integer-like, "
                    f"but got value: {data_dict[key]!r}"
                )

        self.__x_min = get_float("x_min")
        self.__x_max = get_float("x_max")
        self.__y_min = get_float("y_min")
        self.__y_max = get_float("y_max")
        self.__z_min = get_float("z_min")
        self.__z_max = get_float("z_max")

        self.__pixel_in_x = get_int("x_resolution")
        self.__pixel_in_y = get_int("y_resolution")
        self.__pixel_in_z = get_int("z_resolution")

        self.__step_x = get_float("x_step")
        self.__step_y = get_float("y_step")
        self.__step_z = get_float("z_step")

        self.__SSD = get_float("SSD")

        print("Loaded CT metadata:")
        print(
            f"  X: min={self.__x_min}, max={self.__x_max}, "
            f"pixels={self.__pixel_in_x}, step={self.__step_x}"
        )
        print(
            f"  Y: min={self.__y_min}, max={self.__y_max}, "
            f"pixels={self.__pixel_in_y}, step={self.__step_y}"
        )
        print(
            f"  Z: min={self.__z_min}, max={self.__z_max}, "
            f"pixels={self.__pixel_in_z}, step={self.__step_z}"
        )
        print(f"  SSD: {self.__SSD}")

    def __set_image_properties(self, data_path):
        self.__data_path = str(data_path)
        self.__get_metadata_from_file()
        self.series = self.__start_Dicom_series()

    # -------------------------------------------------------------------------
    # DICOM series setup
    # -------------------------------------------------------------------------

    def __start_Dicom_series(self):
        self.__set_hounsfield_dictionary()

        template_path = Path(self.__project_path) / CT_TEMPLATE_RELATIVE_PATH

        if not template_path.is_file():
            raise FileNotFoundError(f"CT DICOM template not found: {template_path}")

        ds = pydicom.dcmread(template_path)

        now = datetime.datetime.now()
        date_string = now.strftime("%Y%m%d")

        # ---------------------------------------------------------------------
        # File meta
        # ---------------------------------------------------------------------

        ds.file_meta.MediaStorageSOPInstanceUID = pydicom.uid.generate_uid(
            prefix=DICOM_UID_PREFIX
        )
        ds.file_meta.ImplementationClassUID = DICOM_UID_PREFIX.rstrip(".")
        ds.file_meta.ImplementationVersionName = "pydicom_3_0_1"
        ds.file_meta.SourceApplicationEntityTitle = DICOM_SOURCE_AE_TITLE
        ds.file_meta.TransferSyntaxUID = ExplicitVRLittleEndian

        # ---------------------------------------------------------------------
        # Main DICOM metadata
        # ---------------------------------------------------------------------

        ds.SOPClassUID = ds.file_meta.MediaStorageSOPClassUID
        ds.SOPInstanceUID = ds.file_meta.MediaStorageSOPInstanceUID

        ds.StudyTime = "123030"
        ds.SeriesTime = "123100"

        ds.StudyDate = date_string
        ds.SeriesDate = date_string
        ds.ContentDate = date_string

        ds.Manufacturer = DICOM_MANUFACTURER
        ds.InstitutionName = DICOM_INSTITUTION_NAME
        ds.InstitutionAddress = DICOM_INSTITUTION_ADDRESS

        ds.SeriesDescription = DICOM_SERIES_DESCRIPTION
        ds.StudyDescription = DICOM_STUDY_DESCRIPTION

        ds.ManufacturerModelName = DICOM_SOFTWARE_VERSION
        ds.SoftwareVersions = DICOM_SOFTWARE_VERSION

        ds.PatientName = self.__label
        ds.PatientID = DICOM_PATIENT_ID
        ds.PatientBirthDate = DICOM_PATIENT_BIRTH_DATE
        ds.PatientPosition = DICOM_PATIENT_POSITION

        ds.DistanceSourceToDetector = self.__SSD
        ds.DistanceSourceToPatient = self.__SSD

        ds.StudyInstanceUID = pydicom.uid.generate_uid(prefix=DICOM_UID_PREFIX)
        ds.SeriesInstanceUID = pydicom.uid.generate_uid(prefix=DICOM_UID_PREFIX)
        ds.FrameOfReferenceUID = pydicom.uid.generate_uid(prefix=DICOM_UID_PREFIX)

        ds.StudyID = "1"
        ds.SeriesNumber = "1"

        ds.PositionReferenceIndicator = "XZ"
        ds.ImageOrientationPatient = r"1\0\0\0\1\0"

        # ---------------------------------------------------------------------
        # Pixel data settings
        # ---------------------------------------------------------------------

        ds.SamplesPerPixel = 1
        ds.PhotometricInterpretation = "MONOCHROME2"

        ds.BitsAllocated = 16
        ds.BitsStored = 16
        ds.HighBit = 15
        ds.PixelRepresentation = 1

        ds.WindowCenter = "110.0"
        ds.WindowWidth = "224.0"

        ds.RescaleIntercept = 0
        ds.RescaleSlope = 1
        ds.RescaleType = "HU"

        ds.is_little_endian = True
        ds.is_implicit_VR = False

        return ds

    # -------------------------------------------------------------------------
    # Material -> HU conversion
    # -------------------------------------------------------------------------

    def __materials_to_hu(self, materials, csv_path=None, slice_number=None):
        materials = materials.astype(str)

        hu = materials.map(self.__hounsfield_units_dictionary)
        missing_mask = hu.isna()

        if missing_mask.any():
            missing_materials = sorted(materials[missing_mask].unique().tolist())

            prefix = f"CT slice {slice_number}: " if slice_number is not None else ""

            print(
                f"[WARN] {prefix}{len(missing_materials)} material name(s) "
                f"not found in Hounsfield dictionary."
            )

            if csv_path is not None:
                print(f"[WARN] File: {csv_path}")

            print(f"[WARN] Missing materials: {missing_materials[:30]}")

            if len(missing_materials) > 30:
                print(f"[WARN] ... and {len(missing_materials) - 30} more.")

            print(
                f"[WARN] Unknown materials will be replaced with "
                f"HU={DEFAULT_UNKNOWN_MATERIAL_HU}."
            )

            hu = hu.fillna(DEFAULT_UNKNOWN_MATERIAL_HU)

        return hu.astype(float)

    # -------------------------------------------------------------------------
    # DICOM slice writing
    # -------------------------------------------------------------------------

    def __write_Dicom_ct_slice(self, rawdata, slice_number):
        if not np.isfinite(rawdata).all():
            n_bad = np.size(rawdata) - np.isfinite(rawdata).sum()
            print(
                f"[WARN] CT slice {slice_number}: raw HU array contains "
                f"{n_bad} non-finite value(s). "
                f"Replacing with HU={DEFAULT_UNKNOWN_MATERIAL_HU}."
            )

            rawdata = np.nan_to_num(
                rawdata,
                nan=DEFAULT_UNKNOWN_MATERIAL_HU,
                posinf=HU_MAX,
                neginf=HU_MIN
            )

        rawdata = np.clip(rawdata, HU_MIN, HU_MAX)
        image2d = rawdata.astype(np.int16)

        ds = self.series

        ds.file_meta.MediaStorageSOPInstanceUID = pydicom.uid.generate_uid(
            prefix=DICOM_SLICE_UID_PREFIX
        )
        ds.SOPInstanceUID = ds.file_meta.MediaStorageSOPInstanceUID

        ds.Rows = image2d.shape[0]
        ds.Columns = image2d.shape[1]

        ds.SliceThickness = str(self.__step_y)

        # Current convention:
        # CSV slice is Y-fixed.
        # Image rows correspond to Z.
        # Image columns correspond to X.
        ds.PixelSpacing = [str(self.__step_z), str(self.__step_x)]

        slice_y = round(
            self.__y_min + (slice_number - 1) * self.__step_y,
            4
        )

        ds.ImagePositionPatient = (
            f"{self.__x_min}\\{self.__z_min}\\{slice_y}"
        )

        ds.SliceLocation = str(slice_y)

        now = datetime.datetime.now()
        ds.InstanceCreationTime = now.strftime("%H%M%S.%f")[:-3]
        ds.ContentTime = now.strftime("%H%M%S")

        ds.InstanceNumber = slice_number
        ds.PixelData = image2d.tobytes()

        pydicom.dataset.validate_file_meta(ds.file_meta, enforce_standard=True)

        output_file = Path(self.__output_path) / f"{self.__label}{slice_number}.dcm"
        ds.save_as(output_file)

        print(f"I just saved {output_file}")

    # -------------------------------------------------------------------------
    # CSV series -> DICOM series
    # -------------------------------------------------------------------------

    @staticmethod
    def __slice_sort_key(path):
        match = re.search(r"(\d+)", path.stem)
        if match:
            return int(match.group(1))
        return path.name

    def __write_whole_Dicom_ct_from_csv(self):
        self.series = self.__start_Dicom_series()
        self.instance_UID = self.series.SOPInstanceUID

        images_path = Path(self.__data_path)

        if not images_path.is_dir():
            raise NotADirectoryError(f"CT CSV input directory does not exist: {images_path}")

        slice_paths = sorted(
            [
                path for path in images_path.iterdir()
                if path.is_file()
                and path.suffix.lower() == ".csv"
                and path.name != METADATA_FILENAME
            ],
            key=self.__slice_sort_key
        )

        expected_slices = int(self.__pixel_in_y)

        if len(slice_paths) != expected_slices:
            print(
                f"[WARN] Expected {expected_slices} CT slice CSV files, "
                f"but found {len(slice_paths)} files."
            )

        print("Start iteration over CT CSV images")

        for slice_number, csv_path in enumerate(slice_paths, start=1):
            df = pd.read_csv(csv_path)

            if MATERIAL_COLUMN not in df.columns:
                raise KeyError(
                    f"File {csv_path} does not contain required column "
                    f"'{MATERIAL_COLUMN}'. "
                    f"Available columns: {list(df.columns)}"
                )

            expected_pixels = int(self.__pixel_in_x) * int(self.__pixel_in_z)

            if len(df) != expected_pixels:
                raise ValueError(
                    f"File {csv_path} contains {len(df)} rows, "
                    f"but expected {expected_pixels} rows "
                    f"({self.__pixel_in_x} x {self.__pixel_in_z})."
                )

            hu = self.__materials_to_hu(
                df[MATERIAL_COLUMN],
                csv_path=csv_path,
                slice_number=slice_number
            )

            rawdata = hu.values.reshape(
                int(self.__pixel_in_z),
                int(self.__pixel_in_x),
                order="F"
            )

            self.__write_Dicom_ct_slice(rawdata, slice_number)

        print(
            f"Finished DICOM CT export. "
            f"Written slices: {len(slice_paths)}. "
            f"Output path: {self.__output_path}"
        )

    # -------------------------------------------------------------------------
    # Public API
    # -------------------------------------------------------------------------

    def create_ct_series(self, directory_path):
        self.__require_paths()
        self.__set_image_properties(directory_path)
        self.__write_whole_Dicom_ct_from_csv()