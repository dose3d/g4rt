# G4RT TODO Task List

## 🔧 Configuration and Setup

- **TLD.hh:17** - Resolve conflict between ROOT and CADMesh (macro naming and include order issues)
- **ConfigSvc.hh:18** - Add std::ostream flexibility

## 🎯 Actions and Control

- **ControlPoint.hh:33** - Introduce FieldType as enum type (see definition in Types.hh)
- **RunAction.cc:71** - Review and verify code implementation

## 📊 Data Analysis

- **BeamAnalysis.cc:126** - Check if this is the same as preStepPoint->GetTotalEnergy()
- **RunAnalysis.cc:21** - Implement RUN_CSV_ANALYSIS
- **RunAnalysis.cc:23** - Implement RUN_NTUPLE_ANALYSIS  
- **RunAnalysis.cc:25** - Implement RUN_HDF5_ANALYSIS
- **StepAnalysis.cc:63** - Define basic histograms

## 🏗️ Geometry

### Linac

- **MlcHD120.cc:40** - Complete implementation
- **MlcSimplified.cc:20** - Get values from configuration
- **README.md:2** - Add table summarizing each model parameterization
- **BeamCollimation.cc:83** - Complete implementation

### Patient/Detector

- **D3DCell.cc:157** - Pass pv and extract global coordinates from it
- **D3DCell.cc:182** - Extract this from Detector::name scope
- **D3DDetector.cc:26** - Complete implementation
- **D3DDetector.cc:209** - Implement method

### Phantom

- **DishCubePhantom.cc** - Implement methods (lines 24, 80, 99, 103, 107)
- **IbaImRT.cc:59** - Filter elements to be removed from IbaImRT
- **IbaImRT.cc:60** - Create module to fetch DB and CSV paths from PhantomWorld
- **SciSlicePhantom.cc:21,64** - Implement methods
- **WaterPhantom.cc:144** - Implement method
- **WaterPhantom.cc:152** - Verify implementation
- **PatientTest.hh:34** - Make std::unique
- **VPatientSD.hh:53** - Check if this is really needed here

### VoxelHit

- **VoxelHit.cc:184** - Store all particles and interactions (not just electrons)
- **VoxelHit.cc:209** - Store Parent ID or Primary ID for visualization purposes
- **VoxelHit.cc:241,257** - Handle these errors
- **VoxelHit.cc:268** - Replace running 1/2-average with true arithmetic mean
- **VoxelHit.cc:506** - Check dose formula implementation
- **VoxelHit_README.md:151** - Continue documentation

### PhaseSpace

- **SavePhSpAnalysis.cc:54** - Move code from SavePhSpSD::ProcessHits
- **SavePhSpSD.cc:30** - Proper handling of data dumping into phsp directory within NTuple

### WorldConstruction

- **LinacGeometry.cc:154** - Refactor code to smart pointers
- **LinacGeometry.hh:36** - No geometry rotation - particles should be rotated after going through Jaws and MLC
- **PatientGeometry.cc:323** - Update GetPhysicalVolume() method
- **PatientGeometry.cc:439,539** - Make generic for any patient
- **SavePhSpConstruction.cc:70** - Move definition to final/specific model definition
- **SavePhSpConstruction.cc:72** - Revise logic for creating SavePhSpSD instances
- **WorldConstruction.cc:340,348** - Make this work for entire geometry tree

## ⚛️ Physics

- **IaeaPrimaryGenerator.cc:27** - Check these functions
- **IaeaPrimaryGenerator.cc:32** - Check how to setup multiple PHSP files
- **IonPrimaryGenerator.cc:27** - Finalize source specification
- **IonPrimaryGenerator.cc:33** - Configure settings
- **PhysicsList.cc:14** - Consider migrating to fully modular physics list

## 🛠️ Services

- **DicomSvc.cc:65** - Refactor context-specific code
- **DicomSvc.cc:138,147,380** - Complete implementation
- **DicomSvc.cc:384** - Implement FieldType::RTPlan
- **DicomSvc.cc:397** - Implement FieldType::CustomPlan
- **DicomSvc.hh:63** - Apply DRY principle
- **GeoSvc.cc:244** - Refactor temporary code
- **GeoSvc.cc:369** - Use enum types
- **RunSvc.cc:271** - Define operation modes
- **RunSvc.cc:465** - Check condition (always true for now)
- **RunSvc.cc:511** - Implement methods for exporting specific world volumes
- **RunSvc.cc:565** - Fix error when closing phasespace file
- **RunSvc.cc:600** - Complete implementation
- **RunSvc.hh:25,26** - Implement TpFractionCounter and DaqTimeCounter
- **Services.cc:243** - Note about RDF::MakeCsvDataFrame

## 🔧 Utilities

- **UIManager.cc:106** - Implement runSvc->GetCurrentRun()

## 📁 Data and Configuration

- **gpsCLinac_pre.mac:80** - Check /gps/ene/emspec 0
- **basic_iba_job.toml:27** - If exists -> read and load
- **basic_gps.toml:25** - Define type

## 🔗 External Libraries ()

- **G4IAEAphspReader.hh:227** - Create setter for multiple files and parallel readout
- **G4IAEAphspReader.cc:142** - Add possibility to introduce theSourceReadId
- **G4IAEAphspReader.cc:543** - Place filtering logic here
- **loguru.cpp:77** - Use defined(_POSIX_VERSION)
- **loguru.cpp:1061** - Store thread name on weird platforms
- **loguru.cpp:1904** - Implement signal handlers on Windows
- **loguru.hpp:1198** - Fix HACK
