# G4RT Generic Phantom CT Generation Workflow

G4RT is a Geant4-based radiotherapy simulation toolkit with support for database-driven geometry construction, TOML job configuration, geometry export, and synthetic CT generation. This README describes the workflow for generating a CT-like DICOM series from an arbitrary phantom imported from a CAD/Fusion 360 3MF-based geometry database.

The main use case is a generic phantom: a 3D geometry that is not tied to a specific patient, detector experiment, or predefined `VPatient` implementation. In this mode, G4RT imports the geometry through the existing DB/3MF pipeline, places it inside the patient environment volume, samples the geometry on a CT grid, writes per-slice CSV files, and converts them into DICOM CT slices.

## What this workflow does

This workflow allows you to:

- prepare a CAD-derived phantom geometry exported from Fusion 360,
- convert the raw exported geometry CSV into the G4RT-compatible Excel database,
- map CAD material names to G4RT material names,
- import the geometry as `GenericPhantom`,
- generate CT CSV slices by sampling the Geant4 geometry,
- convert the CT CSV slices into a DICOM CT series,
- optionally export geometry to GDML, ROOT/TFile, or CSV for inspection.

This workflow does not yet assign sensitive detectors automatically based on 3MF/DB element names. That is planned as a future extension.

## Repository layout

A typical G4RT checkout should contain:

```text
g4rt/
├── app/                         # Main executable entry point
├── core/                        # Geometry, services, physics, actions, analysis
├── data/                        # Dictionaries, templates, input assets
├── jobs/                        # TOML job files
├── output/                      # Simulation and geometry export output
├── submodules/
│   └── d3df-patients/           # Phantom and patient database repository
└── build/                       # CMake/Ninja build directory
```

The phantom submodule is expected to provide a structure similar to:

```text
submodules/d3df-patients/
└── patient/
    └── phantoms/
        ├── IBA_ImRT_phantom/
        │   ├── D3DF_bodies.csv
        │   ├── D3DF_bodies.xlsx
        │   ├── D3DF_components.csv
        │   └── Phantom_v18.3mf
        └── modular_water_phantom/
            ├── D3DF_bodies.csv
            ├── D3DF_bodies.xlsx
            ├── D3DF_components.csv
            └── WaterPhantom_v8.3mf
```

For a new phantom, create a new folder under:

```text
submodules/d3df-patients/patient/phantoms/<your_phantom_name>/
```

and place the relevant `D3DF_bodies.csv`, `D3DF_bodies.xlsx`, optional `D3DF_components.csv`, and the source `.3mf` file there.

## Prerequisites

Recommended environment:

- Linux,
- CMake,
- Ninja,
- C++ compiler compatible with the current G4RT build,
- Geant4,
- ROOT if enabled in the build,
- Python with `pandas`, `numpy`, `pydicom`, `openpyxl`, `loguru`, and `pybind11` support,
- initialized `d3df-patients` submodule.

Example clone with submodules:

```bash
git clone --recursive git@github.com:dose3d/g4rt.git
cd g4rt
```

If the repository was cloned without submodules:

```bash
git submodule update --init --recursive
```
 

---
## Enviroment 
We recommend using Conda/Mamba for environment management: `g4rt_env.yml` provides the standard user environment, while `g4rt_dev_env.yml` contains the extended developer setup with additional tools and dependencies.

---
## Building G4RT

From the repository root:

```bash
mkdir -p build
cd build
cmake .. -G Ninja
ninja -j 6
```

The executable is expected at:

```text
build/app/g4rt
```

For debugging a new geometry workflow, start with one thread:

```bash
./app/g4rt -j 1 -f -t ../jobs/examples/generic_phantom_ct.toml
```

The relevant CLI options are:

```text
-g, --BuildGeometry       build geometry only
-f, --FullSimulation      run full simulation
-j, --nCPU                number of threads
-n, --nEvt                number of events
-t, --TOML                path to TOML job file
-o, --OutputDir           output directory override
-d, --LogLevel            terminal log level
--GeoExportCSV            export geometry to CSV
--GeoExportTFile          export geometry to ROOT/TFile
--GeoExportGate           export geometry to GATE-like format
--GeoExportGdml           export geometry to GDML
```

## Preparing geometry from Fusion 360 export

The DB geometry pipeline expects a cleaned and normalized geometry database. The helper script:

```text
submodules/d3df-patients/utils/build_db_from_fusion_export.py
```

converts a raw Fusion/CAD CSV export into a formatted Excel file with the required sheet structure. It normalizes column names, adds additional mapping columns, sorts by component name, adds a reproducibility hash, optionally translates material names, and writes an Excel workbook with two sheets: `scintillator_mapping_db` and `Metadata`.

Example for the IBA phantom:

```bash
python submodules/d3df-patients/utils/build_db_from_fusion_export.py \
  --csv_path submodules/d3df-patients/patient/phantoms/IBA_ImRT_phantom/D3DF_bodies.csv \
  --output submodules/d3df-patients/patient/phantoms/IBA_ImRT_phantom/D3DF_bodies.xlsx \
  --material_map submodules/d3df-patients/utils/config/material_dictionary_iba.json
```

Example for the modular water phantom:

```bash
python submodules/d3df-patients/utils/build_db_from_fusion_export.py \
  --csv_path submodules/d3df-patients/patient/phantoms/modular_water_phantom/D3DF_bodies.csv \
  --output submodules/d3df-patients/patient/phantoms/modular_water_phantom/D3DF_bodies.xlsx \
  --material_map submodules/d3df-patients/utils/config/material_dictionary_modular_water_phantom.json
```

For a new phantom:

```bash
python submodules/d3df-patients/utils/build_db_from_fusion_export.py \
  --csv_path submodules/d3df-patients/patient/phantoms/my_generic_phantom/D3DF_bodies.csv \
  --output submodules/d3df-patients/patient/phantoms/my_generic_phantom/D3DF_bodies.xlsx \
  --material_map submodules/d3df-patients/utils/config/material_dictionary_my_generic_phantom.json
```

## Material translation dictionary

Fusion 360 material names are often not identical to material names used by G4RT/Geant4. The material translation dictionary maps CAD material labels to simulation material labels.

Example:

```json
{
  "Acrylic, Clear": "PMMA",
  "GFRP": "EPS",
  "Glass, Cast, Gray": "PMMA",
  "Nylon 12 (with Formlabs Fuse 1 3D Printer)": "PLA",
  "Phenolic Resin": "RMPS470",
  "Rubber": "Rubber",
  "Water": "G4_WATER"
}
```

Create one material dictionary per phantom if needed. The important rule is that the translated values must exist in `MaterialsSvc` or in the material configuration used by the simulation.

## Hounsfield dictionary for DICOM CT export

DICOM CT generation uses a separate HU dictionary. This dictionary maps Geant4/G4RT material names to synthetic Hounsfield units.

Expected path:

```text
data/config/hounsfield_scale_120keV.json
```

Minimal example:

```json
[
  {
    "Usr_G4AIR20C": -995.0,
    "G4_AIR": -1000.0,
    "G4_Galactic": -1000.0,
    "Vacuum": -1000.0,
    "G4_WATER": 0.0,
    "Water": 0.0,
    "RW3": 6.3,
    "PMMA": 120.0,
    "PLA": 142.4,
    "RMPS470": 126.0,
    "EPS": -950.0,
    "Rubber": 100.0,
    "Lung": -650.0,
    "Muscle": 45.0,
    "Brain": 35.0,
    "Prostate": 35.0,
    "CorticalBone": 1000.0,
    "Bone": 700.0,
    "Unknown": -1000.0,
    "UNKNOWN": -1000.0
  },
  {
    "CT": "1.2.840.10008.5.1.4.1.1.2."
  }
]
```

If a material appears in CT CSV export but is missing from this dictionary, the Python DICOM converter should warn about it and replace it with the default unknown-material HU, usually air-like `-1000`. This fallback keeps the export alive, but the correct fix is to add the missing material explicitly.

To inspect all material names written to CT CSV:

```bash
awk -F, 'NR>1 {print $4}' output/<run_name>/geo/dicom/ct_csv/img*.csv \
  | sort | uniq -c | sort -nr
```

## Generic phantom TOML configuration

Create a job file, for example:

```text
jobs/examples/generic_phantom_ct.toml
```

A minimal `PatientGeometry` block:

```toml
[PatientGeometry]
Type = "GenericPhantom"

PatientIsocentreX = 0.0
PatientIsocentreY = 0.0
PatientIsocentreZ = 0.0

EnviromentSizeX = 1400.0
EnviromentSizeY = 1400.0
EnviromentSizeZ = 1400.0

VoxelSizeXCT = 7.0
VoxelSizeYCT = 7.0
VoxelSizeZCT = 7.0

EnviromentMedium = "Usr_G4AIR20C"
EnviromentPatientEnvelop = "GenericPhantom_3mf"

PatientDBPath = "d3df-patients/patient/phantoms/modular_water_phantom"
ConfigPrefix = "GenericPhantom"

[GenericPhantom]
Position = [-271.0, 275.0, 225.0]
Rotation = [0.0, 0.0, 0.0]
ExcludeObjList = []
```

For a new phantom, change `PatientDBPath` and adjust `Position`, `Rotation`, and the environment size. If Geant4 reports that daughter volumes are outside `patientEnvLV`, either the envelope is too small or the imported geometry placement is wrong.

For the first test, use a generous envelope:

```toml
EnviromentSizeX = 1400.0
EnviromentSizeY = 1400.0
EnviromentSizeZ = 1400.0
```

After confirming the geometry placement, reduce the envelope to the required size.

## CT grid settings

The CT grid is controlled by:

```toml
VoxelSizeXCT = 7.0
VoxelSizeYCT = 7.0
VoxelSizeZCT = 7.0
```

and by the environment size:

```toml
EnviromentSizeX = 1400.0
EnviromentSizeY = 1400.0
EnviromentSizeZ = 1400.0
```

The number of CT samples is approximately:

```text
x_resolution = EnviromentSizeX / VoxelSizeXCT
y_resolution = EnviromentSizeY / VoxelSizeYCT
z_resolution = EnviromentSizeZ / VoxelSizeZCT
```

For example, `1400 mm / 7 mm` gives 200 samples along that axis. A very fine CT voxel size can generate a large number of CSV rows and DICOM slices, so start with a coarse grid when debugging.

## Running CT generation

Recommended first run:

```bash
cd build
./app/g4rt \
  -j 1 \
  -f \
  -t ../jobs/examples/generic_phantom_ct.toml \
  -o ../output/generic_phantom_ct_test \
  -d INFO
```

Geometry-only run with GDML export:

```bash
cd build
./app/g4rt \
  -j 1 \
  -g \
  --GeoExportGdml \
  -t ../jobs/examples/generic_phantom_ct.toml \
  -o ../output/generic_phantom_geometry_debug \
  -d DEBUG
```

Full run with geometry exports:

```bash
cd build
./app/g4rt \
  -j 1 \
  -f \
  --GeoExportCSV \
  --GeoExportTFile \
  --GeoExportGdml \
  -t ../jobs/examples/generic_phantom_ct.toml \
  -o ../output/generic_phantom_full_debug \
  -d INFO
```

## Expected output

The output directory should contain simulation logs and geometry output. For CT export, the important paths are:

```text
output/<run_name>/geo/dicom/ct_csv/
├── ct_series_metadata.csv
├── img0001.csv
├── img0002.csv
├── ...
└── imgNNNN.csv
```

and:

```text
output/<run_name>/geo/dicom/ct_dcm/
├── CT_1.dcm
├── CT_2.dcm
├── ...
└── CT_N.dcm
```

The CT CSV files contain sampled material names. The DICOM files contain the corresponding HU values after material-to-HU conversion.

## Typical troubleshooting

### `Given unit (G4_AIR) is not defined`

Use a material defined by `MaterialsSvc`, for example:

```toml
EnviromentMedium = "Usr_G4AIR20C"
```

Alternatively, define `G4_AIR` explicitly in `MaterialsSvc` if that alias is desired.

### `Overlapping daughter with mother volume` or `entirely outside mother logical volume`

The imported geometry is outside `patientEnvLV`. Increase the envelope temporarily:

```toml
EnviromentSizeX = 2000.0
EnviromentSizeY = 2000.0
EnviromentSizeZ = 2000.0
```

Then adjust:

```toml
[GenericPhantom]
Position = [...]
Rotation = [...]
```

### Crash during run merge with `GeoSvc::Patient() == nullptr`

Generic phantom mode intentionally does not create a `VPatient`. Any scoring or merge code must treat `GeoSvc::Patient()` as optional. Patient-specific volume correction must only be applied when a patient object exists.

### `RuntimeWarning: invalid value encountered in cast`

The DICOM converter encountered material names that are missing from the HU dictionary. Inspect material names in the CT CSV files:

```bash
awk -F, 'NR>1 {print $4}' output/<run_name>/geo/dicom/ct_csv/img*.csv \
  | sort | uniq -c | sort -nr
```

Then add the missing names to:

```text
data/config/hounsfield_scale_120keV.json
```

### Too many DICOM slices or very slow CT export

Increase CT voxel size while debugging:

```toml
VoxelSizeXCT = 20.0
VoxelSizeYCT = 20.0
VoxelSizeZCT = 20.0
```

or reduce the environment size.

## Recommended validation checklist

Before committing a new phantom setup:

- the submodule contains `.3mf`, `D3DF_bodies.csv`, and `D3DF_bodies.xlsx`,
- material names in `D3DF_bodies.xlsx` are translated to known G4RT materials,
- `PatientDBPath` points to the correct phantom folder,
- `EnviromentMedium` is a known material,
- `GenericPhantom.Position` places the geometry inside `patientEnvLV`,
- CT CSV export produces the expected number of slices,
- DICOM CT export completes without NaN/cast warnings,
- all unknown material warnings are either fixed in the HU dictionary or intentionally accepted.

## Future work

Planned extension:

```toml
[GenericPhantom]
SensitiveObjList = ["scintillator", "detector", "roi"]
SensitiveObjMode = "substring"
RunCollectionName = "generic_phantom"
```

The goal is to allow sensitive-volume assignment directly from selected 3MF/DB element names or keys. This should probably be implemented in the DB geometry builder, because that layer already has access to imported component names, body names, material names, and object IDs.
