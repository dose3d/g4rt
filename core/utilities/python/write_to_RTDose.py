import os
import pydicom
import numpy as np
from pydicom.dataset import Dataset, FileMetaDataset
from pydicom.uid import UID, ExplicitVRLittleEndian
from pydicom.sequence import Sequence

def write_rtdose(metadata, dose_grid, output_file):
    """
    Create RTdose file.
    """
    dose_arr = np.array(dose_grid, copy=True)

    x_unique = metadata['x_unique']
    y_unique = metadata['y_unique']
    z_unique = metadata['z_unique']
    
    number_of_frames, rows, columns = dose_arr.shape

    ds = Dataset()
    file_meta = FileMetaDataset()
    file_meta.MediaStorageSOPClassUID = UID("1.2.840.10008.5.1.4.1.1.481.2")
    file_meta.MediaStorageSOPInstanceUID = UID("1.2.5")
    file_meta.ImplementationClassUID = UID('1.2.3.5')
    file_meta.TransferSyntaxUID = ExplicitVRLittleEndian

    ds.file_meta = file_meta
    ds.SOPClassUID = file_meta.MediaStorageSOPClassUID
    ds.SOPInstanceUID = file_meta.MediaStorageSOPInstanceUID

    ds.Modality = 'RTDOSE'
    ds.DoseUnits = 'GY'
    ds.Rows = rows
    ds.Columns = columns
    ds.NumberOfFrames = number_of_frames
    
    max_dose = dose_arr.max()
    max_dose = max_dose if max_dose() > 0 else 0
    ds.DoseGridScaling = f"{max_dose / 65535:.8e}"
    
    ds.BitsAllocated = 16
    ds.BitsStored = 16
    ds.HighBit = 15
    ds.PixelRepresentation = 0
    
    z_start = float(z_unique[0])
    ds.GridFrameOffsetVector = [float(z) - z_start for z in z_unique]
    ds.ImagePositionPatient = [float(x_unique[0]), float(y_unique[0]), z_start]
    ds.PixelSpacing = [float(x_unique[1] - x_unique[0]), float(y_unique[1] - y_unique[0])]
    ds.SliceThickness = abs(float(z_unique[1] - z_unique[0])) if len(z_unique) > 1 else 1.0
    ds.ImageOrientationPatient = [1.0, 0.0, 0.0, 0.0, 1.0, 0.0]
    
    normalized_dose = dose_arr / max_dose
    scaled_dose = (normalized_dose * 65535).clip(0, 65535)
    ds.PixelData = scaled_dose.astype(np.uint16).tobytes()

    ds.PatientName = metadata.get('PatientName', 'CELL DOSE')
    ds.PatientID = metadata.get('PatientID', '121235')
    ds.PatientBirthDate = metadata.get('PatientBirthDate', '20021208')
    ds.PatientSex = metadata.get('PatientSex', 'F')
    
    ds.FrameOfReferenceUID = UID('1.2.840.10008.15.1.1')
    ds.PositionReferenceIndicator = "XZ"
    ds.Manufacturer = "SIEMENS"
    ds.StudyDate = '20241208'
    ds.StudyTime = '161334'
    ds.AccessionNumber = '2504966403753009'
    ds.ReferringPhysicianName = "Physician Name"
    ds.StudyInstanceUID = pydicom.uid.generate_uid()
    ds.StudyID = '12123434'
    ds.SamplesPerPixel = 1
    ds.PhotometricInterpretation = "MONOCHROME2"
    
    ds.OperatorsName = 'REMOVED'
    ds.SeriesInstanceUID = pydicom.uid.generate_uid()
    ds.SeriesNumber = '2'
    ds.FrameIncrementPointer = (0x3004, 0x000C)

    referenced_rt_plan = Dataset()
    referenced_rt_plan.ReferencedSOPClassUID = "1.2.840.10008.5.1.4.1.1.481.5"
    referenced_rt_plan.ReferencedSOPInstanceUID = "1.2.3.4.5.6.7.8.9.0"
    ds.ReferencedRTPlanSequence = Sequence([referenced_rt_plan])

    ds.DoseSummationType = 'PLAN'
    ds.DoseType = 'PHYSICAL'

    print("===== Saved to file =====")
    ds.save_as(output_file, enforce_file_format=True)