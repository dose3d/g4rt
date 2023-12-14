#include "RunSvc.hh"
#include "Services.hh"
#include "UIManager.hh"
#include "ActionInitialization.hh"
#include "WorldConstruction.hh"
#include "RunAction.hh"
#include "PhysicsList.hh"
#include "G4ScoringManager.hh"
#include "G4UImanager.hh"
#include "G4PhysicalConstants.hh"
#include "CLHEP/Random/RanecuEngine.h"
#include "CLHEP/Random/RandomEngine.h"
#include "colors.hh"
#include "G4RotationMatrix.hh"
#include "TFileMerger.h"
#ifdef G4MULTITHREADED
  #include "G4Threading.hh"
  #include "G4MTRunManager.hh"
#endif
#include "PatientGeometry.hh"
#include "D3DDetector.hh"
#include "TGeoManager.h"
#include "TGeoVolume.h"
#include "TFile.h"
#include "TTree.h"

////////////////////////////////////////////////////////////////////////////////
///
RunSvc::RunSvc() : TomlConfigurable("RunSvc"), Logable("RunSvc"){
  std::cout << "Config start" << std::endl;
  Configure();
  std::cout << "Config end" << std::endl;
  // Instantiate and initialize all core services:
  Service<GeoSvc>();        // initialize GeoSvc
}

////////////////////////////////////////////////////////////////////////////////
///
RunSvc::~RunSvc() {
  configSvc()->Unregister(thisConfig()->GetName());
}

////////////////////////////////////////////////////////////////////////////////
///
RunSvc *RunSvc::GetInstance() {
  static RunSvc instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::Configure() {

  // G4cout << "[INFO]:: RunSvc :: Service default configuration " << G4endl;
  LOGSVC_INFO("Service default configuration ");
  DefineUnit<std::string>("JobName");

  // MULTI RUN
  DefineUnit<std::string>("SimConfigFile");
  DefineUnit<std::string>("LogConfigFile");
  DefineUnit<std::string>("LogConfigPrefix");


  // RUN PARAMETERS
  DefineUnit<int>("NumberOfEvents");      // Number of primary events
  DefineUnit<double>("PrintProgressFrequency");    // Fraction of total events after which progress info is printed
  DefineUnit<int>("NumberOfThreads");
#ifdef G4MULTITHREADED
  DefineUnit<int>("MaxNumberOfThreads");
#endif
  DefineUnit<long>("RNGSeed");            // Random Number Generator seed

  // PRIMARY GENERATOR
  DefineUnit<std::string>("BeamType");
  DefineUnit<double>("FieldSizeA");
  DefineUnit<double>("FieldSizeB");
  DefineUnit<std::string>("FieldShape");
  DefineUnit<double>("phspShiftZ");
  DefineUnit<std::string>("Physics");
  DefineUnit<int>("idEnergy");

  // General Particle Source
  DefineUnit<std::string>("GpsMacFileName");

  // PHASE SPACE
  DefineUnit<bool>("SavePhSp");
  DefineUnit<std::string>("PhspInputFileName");
  DefineUnit<std::string>("PhspInputPosition");
  DefineUnit<std::string>("PhspOutputFileName");

  // DICOM initial parameters
  DefineUnit<bool>("DICOM"); // the general flag to activate DICOM processing
  DefineUnit<std::string>("RTPlanInputFile");

  // ANALYSIS MANAGEMENT
  DefineUnit<bool>("RunAnalysis");
  DefineUnit<bool>("StepAnalysis");
  DefineUnit<bool>("NTupleAnalysis");
  DefineUnit<bool>("PrimariesAnalysis");
  DefineUnit<bool>("BeamAnalysis");
  DefineUnit<bool>("ExportIntegratedDose");

  DefineUnit<std::string>("OutputDir");

  Configurable::DefaultConfig();   // setup the default configuration for all defined units/parameters
  // Configurable::PrintConfig();

  MaterialsSvc::GetInstance();  // initialize/configure the materials list
}

////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::DefaultConfig(const std::string &unit) {
  // Multirun option
  if (unit.compare("SimConfigFile") == 0)
    thisConfig()->SetTValue<std::string>(unit, std::string("None"));

  if (unit.compare("LogConfigFile") == 0)
    thisConfig()->SetTValue<std::string>(unit, std::string("/data/config/deafult_logger.toml"));

  if (unit.compare("LogConfigPrefix") == 0)
    thisConfig()->SetTValue<std::string>(unit, std::string("Log"));

  // Config volume name
  if (unit.compare("Label") == 0) 
    thisConfig()->SetValue(unit, std::string("Geant4-RT Run Service"));

  // default simulation job name
  if (unit.compare("JobName") == 0) 
    thisConfig()->SetTValue<std::string>(unit, std::string("ExampleJobName"));

  // default number of primary events
  if (unit.compare("NumberOfEvents") == 0) 
    thisConfig()->SetTValue<int>(unit, int(1000));

  // default fraction of total events after which progress info is printed
  if (unit.compare("PrintProgressFrequency") == 0) 
    thisConfig()->SetTValue<double>(unit, 0.01);

  // default RNG Seed
  if (unit.compare("RNGSeed") == 0) 
    thisConfig()->SetValue(unit, long(137));

  // default number of CPU to be used
  if (unit.compare("NumberOfThreads") == 0) {
#ifdef G4MULTITHREADED
    thisConfig()->SetTValue<int>(unit, int(G4Threading::G4GetNumberOfCores()));
    thisConfig()->SetValue("MaxNumberOfThreads", int(G4Threading::G4GetNumberOfCores()));
#else
    thisConfig()->SetValue(unit, int(1));
#endif
  }

  // default source type
  // Available source types: phaseSpace, gps, phaseSpaceCustom
  if (unit.compare("BeamType") == 0) 
    thisConfig()->SetTValue<std::string>(unit, std::string("None")); //IAEA or gps 

  if (unit.compare("FieldSizeA") == 0) 
    thisConfig()->SetTValue<double>(unit, double(-1)); // by default no cut at the level of PrimaryGenerationAction::GeneratePrimaries

  if (unit.compare("FieldSizeB") == 0) 
    thisConfig()->SetTValue<double>(unit, double(-1)); // by default no cut at the level of PrimaryGenerationAction::GeneratePrimaries

  if (unit.compare("FieldShape") == 0){
    m_config->SetTValue<std::string>(unit, std::string("Rectangular"));
  }

  // Fixed value for BeamCollimation: 1.25 cm above secondary collimator
  // if (unit.compare("phspShiftZ") == 0) thisConfig()->SetValue(unit, G4double(78.0 * cm)); // Custom phsp reader
  if (unit.compare("phspShiftZ") == 0) 
    thisConfig()->SetValue(unit, double(100 * cm)); // IAEA phsp reader

  // default physics
  if (unit.compare("Physics") == 0) 
    thisConfig()->SetTValue<std::string>(unit, std::string("emstandard_opt3")); //      LowE_Livermore   LowE_Penelope   emstandard_opt3

  // default ID energy
  if (unit.compare("idEnergy") == 0) 
    thisConfig()->SetValue(unit, int(6));

  if (unit.compare("SavePhSp") == 0) 
    thisConfig()->SetValue(unit, false);

  if (unit.compare("GpsMacFileName") == 0){
    std::string project_path = PROJECT_LOCATION_PATH;    
    thisConfig()->SetTValue<std::string>(unit, std::string(project_path+"/scripts/gps.mac")); // ./gps.mac, gps_cd109_gammas_pre.mac
}

  if (unit.compare("PhspInputFileName") == 0){
    m_config->SetTValue<std::string>(unit, std::string("None"));
  }
  // if (unit.compare("PhspInputFileName") == 0) thisConfig()->SetValue(unit, G4String("/primo/iaea_clinac2300/s2/field3x3-s2"));

  if (unit.compare("PhspInputPosition") == 0){
    m_config->SetTValue<std::string>(unit, std::string("s2"));
  }

  if (unit.compare("PhspOutputFileName") == 0) 
    thisConfig()->SetValue(unit, std::string("phasespaces"));

  if (unit.compare("DICOM") == 0) 
    thisConfig()->SetTValue<bool>(unit, false);

  if (unit.compare("RTPlanInputFile") == 0){
    std::string data_path = PROJECT_DATA_PATH;
    thisConfig()->SetValue(unit, std::string(data_path+"/DICOM/example-imrt.dcm"));
  }

  if (unit.compare("RunAnalysis") == 0) 
    thisConfig()->SetTValue<bool>(unit, false);
  if (unit.compare("StepAnalysis") == 0) 
    thisConfig()->SetTValue<bool>(unit, false);
  if (unit.compare("NTupleAnalysis") == 0) 
    thisConfig()->SetTValue<bool>(unit, false);
  if (unit.compare("BeamAnalysis") == 0) 
    thisConfig()->SetTValue<bool>(unit, false);
  if (unit.compare("PrimariesAnalysis") == 0) 
    thisConfig()->SetTValue<bool>(unit, false);
  if (unit.compare("ExportIntegratedDose") == 0) 
    thisConfig()->SetTValue<bool>(unit, true);

  if (unit.compare("OutputDir") == 0) 
    thisConfig()->SetTValue<std::string>(unit, std::string());

}

////////////////////////////////////////////////////////////////////////////////
///
bool RunSvc::ValidateConfig() const {
  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::Initialize() {

  InitializeOutputDir();
  LogSvc::Configure();
  m_logger = LogSvc::RecreateLogger("RunSvc");
  LOGSVC_INFO("Logger recreated.");

  if (m_application_mode == OperationalMode::BuildGeometry)
    return;

  if (!m_isInitialized) {
    LOGSVC_INFO("Service initialization...");

    Configurable::ValidateConfig();
    PrintConfig();

    // Handle the context specific configuration
    auto simConfigFile =thisConfig()->GetValue<std::string>("SimConfigFile");
    if(simConfigFile.empty() || simConfigFile=="None")
      SetTomlConfigFile();
    else{
      std::string projectPath = PROJECT_LOCATION_PATH;
      SetTomlConfigFile(projectPath+simConfigFile);
    }

    // Define Control Points etc.
    SetSimulationConfiguration();

    // Add simple scoring
    // if(thisConfig()->GetValue<G4String>("patientName")=="WaterPhantom"){
    //   m_macFiles.push_back("./scoring_pre.mac");   // List of commands being applied before beamOn execution
    //   m_macFiles.push_back("./scoring_post.mac");  // List of commands being applied after beamOn execution
    // }
    // TODO : when the modes of operation will be defined:
    // m_macFiles.push_back("./vis.mac");           // List of commands being applied before beamOn execution
    auto numberOfThreads = m_configSvc->GetValue<int>("RunSvc", "NumberOfThreads");
    auto physics = m_configSvc->GetValue<std::string>("RunSvc", "Physics");
    auto numberOfControlPoints = m_control_points.size();
    LOGSVC_INFO("Launching {} thread(s)",numberOfThreads);
    LOGSVC_INFO("Launching {} physics model",physics);
    LOGSVC_INFO("Launching {} control points",numberOfControlPoints);

    
 
#ifdef G4MULTITHREADED
    m_g4RunManager = new G4MTRunManager();
#else
    m_g4RunManager = new G4RunManager();
#endif
    m_isInitialized = true;
  } else {
    LOGSVC_WARN("RunSvc Service is already initialized.");
  }
  m_g4RunManager->SetRunIDCounter(1);
  
}

////////////////////////////////////////////////////////////////////////////////
///
std::string RunSvc::InitializeOutputDir() {
  auto path = m_configSvc->GetValue<std::string>("RunSvc","OutputDir");
  if(!path.empty()){ 
    svc::createDirIfNotExits(path);
    auto jobDir = RunSvc::GetJobNameLabel();
    auto nDirs = svc::countDirsInLocation(path,jobDir);
    auto jobNewDir = path+"/"+jobDir;
    if(nDirs>0) 
      jobNewDir+="_"+std::to_string(nDirs);
    svc::createDirIfNotExits(jobNewDir);
    m_configSvc->SetValue("RunSvc","OutputDir", jobNewDir);
  }
  else { // create default location for the output
    std::string jobNewDir = PROJECT_LOCATION_PATH;
    jobNewDir+="/output";
    m_configSvc->SetValue("RunSvc","OutputDir", jobNewDir);
    InitializeOutputDir();
  }
   return path;
}

////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::Finalize() {

  // Perform geometry exports
  WriteGeometryData();

  // Perform physics outcome exports
  WriteControlPointData();

  //
  MergeOutput(false);

  //
  auto runWorld = Service<GeoSvc>()->World();
  if(runWorld){
    runWorld->Destroy();
  }

  LOGSVC_INFO("Goodbye from G4RT!");
  LogSvc::ShutDown();
}

////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::UserG4Initialization() {
  if (!m_isUsrG4Initialized) {
    G4Timer timer;
    timer.Start();
    LOGSVC_INFO("UserG4Initialization...");
    m_g4RunManager->SetUserInitialization(Service<GeoSvc>()->World());
    m_g4RunManager->SetUserInitialization(new PhysicsList());
    m_g4RunManager->SetUserInitialization(new ActionInitialization());

    // measure initialization time
    timer.Stop();
    LOGSVC_INFO( "Initialisation elapsed time [s]: {}", timer.GetRealElapsed());
    m_isUsrG4Initialized = true;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::Run() {
  switch (m_application_mode) {
    case OperationalMode::BuildGeometry:
      BuildGeometryMode();
      break;
    case OperationalMode::FullSimulation:
      FullSimulationMode();
      break;
    default:
      LOGSVC_ERROR("Operational mode missing!");
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::ParseTomlConfig(){
  auto configFile = GetTomlConfigFile();
  auto configPrefix = GetTomlConfigPrefix();
  LOGSVC_INFO("Importing configuration from:\n{}",configFile);
  std::string configObj("Plan");
  if(!configPrefix.empty() || configPrefix=="None" ){ // It shouldn't be empty!
    configObj.insert(0,configPrefix+"_");
  } 
  else {
    G4ExceptionDescription msg;
    msg << "The configuration PREFIX is not defined";
    G4Exception("RunSvc", "ParseTomlConfig", FatalErrorInArgument, msg);
  }
  auto config = toml::parse_file(configFile);
  G4double rotationInDegree = 0.*deg;
  auto numberOfCP = config[configObj]["Control_Points_In_Treatment_Plan"].value_or(0);
  if(numberOfCP>0){
    for( int i = 0; i < numberOfCP; i++ ){
      rotationInDegree = (config[configObj]["Gantry_Angle_Per_Control_Point"][i].value_or(0.0))*deg;
      int nEvents = config[configObj]["Particle_Counter_Per_Control_Point"][i].value_or(-1);
      if(nEvents<0)
        nEvents = thisConfig()->GetValue<int>("NumberOfEvents");
      m_control_points_config.emplace_back(i,nEvents,rotationInDegree);
    }
  }
  else{
    G4String msg = "The configuration PREFIX is not defined";
    LOGSVC_CRITICAL(msg);
    G4Exception("RunSvc", "ParseTomlConfig", FatalErrorInArgument, msg);
  }
}

////////////////////////////////////////////////////////////////////////////////
/// 
void RunSvc::SetSimulationConfiguration(){
  if(IsTomlConfigExists())
    ParseTomlConfig();
  else
    SetSimulationDefaultConfig();
  
  if(m_configSvc->GetValue<bool>("RunSvc", "DICOM"))
    ParseDicomInputData();
    
  DefineControlPoints();
}

////////////////////////////////////////////////////////////////////////////////
/// 
void RunSvc::DefineControlPoints() {
  for(const auto& icpc : m_control_points_config){
    m_control_points.emplace_back(icpc);
  }
  if(m_control_points.size()>0){
    m_current_control_point = &m_control_points.at(0);
  } else {
    G4String msg = "Any control point is created. Verify job definition";
    LOGSVC_CRITICAL(msg);
    G4Exception("RunSvc", "DefineControlPoints", FatalErrorInArgument, msg);
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::ParseDicomInputData(){
  auto dicomSvc = Service<DicomSvc>(); // initialize the DICOM service
  auto nCP = 1; // temp fixed, final version: dicomSvc->GetTotalNumberOfControlPoints();
  LOGSVC_INFO("RT-Plan #ControlPoints: {}",nCP);
  int nevts = 10000; // temp fixed
  double rot_in_deg = 0;// temp fixed
  for(unsigned i=0;i<nCP;++i){
    m_control_points_config.emplace_back(i,nevts,rot_in_deg);
    //TODO m_control_points_config.back().RTPlanFile = ...
    
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Define simply single Control Point
void RunSvc::SetSimulationDefaultConfig(){
  G4double rotationInDegree = 0.*deg;
  auto nEvents = thisConfig()->GetValue<int>("NumberOfEvents");
  m_control_points_config.emplace_back(0,nEvents,rotationInDegree);

}

////////////////////////////////////////////////////////////////////////////////
///
/// TODO: implement methods for exporting particular world volumes
void RunSvc::BuildGeometryMode() {
  LOGSVC_INFO("Building World Geometry...");
  m_logger->flush();
  Service<GeoSvc>()->Build();
}

////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::FullSimulationMode() {
  LOGSVC_INFO("FullSimulationMode");
  auto sourceName = m_configSvc->GetValue<std::string>("RunSvc", "BeamType");
  if (sourceName.compare("gps") == 0)
    m_macFiles.push_back(m_configSvc->GetValue<std::string>("RunSvc", "GpsMacFileName"));

  G4Random::setTheEngine(new CLHEP::RanecuEngine);
  auto userSeed = thisConfig()->GetValue<long>("RNGSeed");
  if (userSeed < 214) {
    G4Random::setTheSeed(userSeed);
  } else {
    long seeds[2] = {userSeed, userSeed};
    G4Random::setTheSeeds(seeds);
  }

  LOGSVC_INFO("RNG Seed: {} ", G4Random::getTheSeed()); 

  #ifdef G4MULTITHREADED
    auto NofThreads = m_configSvc->GetValue<int>("RunSvc", "NumberOfThreads");
    dynamic_cast<G4MTRunManager *>(m_g4RunManager)->SetNumberOfThreads(NofThreads);
  #endif

  UserG4Initialization();

  // Activate command-based scorer
  G4ScoringManager::GetScoringManager()->SetVerboseLevel(1);

  // Run and G4 kernel setup
  auto uiManager = UIManager::GetInstance();


  uiManager->UserRunInitialization(); // ControlPoint loop is here

  // Final stuff & cleaning
  uiManager->UserRunFinalization();

  // release world volume pointer from the G4RunManager store (otherwise it will be destroyed)
  // m_g4RunManager->SetUserInitialization(static_cast<G4VUserDetectorConstruction*>(nullptr));

  // TODO deleting runManager causes and error with closing phasespace file - figure out why ?
  // delete m_g4RunManager;
  // m_g4RunManager = nullptr;
}

////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::SetNofThreads(int val) {
  auto MaxNThresds = thisConfig()->GetValue<int>("MaxNumberOfThreads");
  m_configSvc->SetValue("RunSvc", "NumberOfThreads", val);
  if (val > MaxNThresds) {
    LOGSVC_WARN("Specified number of threads is higher than available CPUs.");
  }
}

////////////////////////////////////////////////////////////////////////////////
///
std::string RunSvc::GetJobNameLabel() {
  auto job_name = Service<ConfigSvc>()->GetValue<std::string>("RunSvc", "JobName");
  std::replace(job_name.begin(), job_name.end(), ' ', '_');
  // convert to lower case letters only
  std::transform(job_name.begin(), job_name.end(), job_name.begin(),
    [](unsigned char c){ return std::tolower(c); });
  return job_name;
}

////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::WriteGeometryData() const {
  LOGSVC_DEBUG("Writing Geometry Data");
  auto geoSvc = Service<GeoSvc>();
  geoSvc->WriteWorldToGdml();
  geoSvc->WriteWorldToTFile();
  geoSvc->WriteScoringComponentsPositioningToTFile(); // TODO
  geoSvc->WriteScoringComponentsPositioningToCsv();
}

////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::WriteControlPointData(){
  for (auto& cp : m_control_points){
    LOGSVC_DEBUG("Writing Control Point Data (CP-{})",cp.GetId());
    // Field mask export:
    cp.WriteFieldMaskToCsv();
    cp.WriteFieldMaskToTFile();
    cp.WriteVolumeFieldMaskToCsv();
    cp.WriteVolumeFieldMaskToTFile();

    // Integrated dose:
    cp.WriteIntegratedDoseToTFile();
    cp.WriteIntegratedDoseToCsv();

    // Release memory alocated for scoring:
    cp.ClearCachedData(); 
  }
  ControlPoint::IntegrateAndWriteTotalDoseToTFile();
  ControlPoint::IntegrateAndWriteTotalDoseToCsv();
}

////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::ExportControlPointsFieldMask() const {
  for (const auto& cp : m_control_points){
    LOGSVC_DEBUG("Exporting Field Mask for Control Point - {}",cp.GetId());
    // cp.WriteFieldMaskToFile("Plan",true,true);  // tfile, csv
    // cp.WriteFieldMaskToFile("Sim",true,true);   // tfile, csv
    // cp.WriteVolumeFieldMaskToFile(true,true);   // tfile, csv
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::ExportControlPointsIntegratedDose() const {
  for (const auto& cp : m_control_points){
    LOGSVC_DEBUG("Exporting Integrated Doese for Control Point - {}",cp.GetId());
    cp.WriteIntegratedDoseToFile(true,true); // tfile, csv
  }
}

////////////////////////////////////////////////////////////////////////////////
/// TOBE REMOVED
void RunSvc::ExportIntegratedDose() const {
  LOGSVC_INFO("Processing {} run(s) outputs - Dose Integration and data processing.",m_control_points.size());
  for (const auto& cp : m_control_points){
    //TODO: cp.ExportIntegratedDose();
    auto runId = cp.GetId();
    auto runIdString = std::to_string(runId);
    auto output_dir = thisConfig()->GetValue<std::string>("OutputDir");
    auto output_file = cp.GetSimOutputTFileName();
    LOGSVC_INFO("Processing: {}",output_file);

    auto patient = PatientGeometry::GetInstance();
    auto det = dynamic_cast<D3DDetector*>(patient->GetPatient());
    auto hashedMapCellDose = det->GetScoringHashedMap("CellDose", false);
    auto hashedMapVoxDose = det->GetScoringHashedMap("VoxelDose", true);

    ///
    auto f = std::make_unique<TFile>(TString(output_file));
    auto tree = static_cast<TTree*>(f->Get("Dose3DVoxelisedTTree"));

    G4int cIdx; tree->SetBranchAddress("CellIdX",&cIdx);
    G4int cIdy; tree->SetBranchAddress("CellIdY",&cIdy);
    G4int cIdz; tree->SetBranchAddress("CellIdZ",&cIdz);

    G4int vIdx; tree->SetBranchAddress("VoxelIdX",&vIdx);
    G4int vIdy; tree->SetBranchAddress("VoxelIdY",&vIdy);
    G4int vIdz; tree->SetBranchAddress("VoxelIdZ",&vIdz);
    G4int evtRunId; tree->SetBranchAddress("G4RunId",&evtRunId);

    G4double c_dose; tree->SetBranchAddress("CellDose",&c_dose);
    G4double v_dose; tree->SetBranchAddress("VoxelDose",&v_dose);

    auto nentries = tree->GetEntries();
    for (Long64_t i=0;i<nentries;i++) {
        tree->GetEntry(i);
          auto hash_str = std::to_string(cIdx);
          hash_str+= std::to_string(cIdy);
          hash_str+= std::to_string(cIdz);
          auto hash_key_c = std::hash<std::string>{}(hash_str);
          auto& cell_hit = hashedMapCellDose.at(hash_key_c);
          cell_hit.SetDose(cell_hit.GetDose()+c_dose);

          hash_str+= std::to_string(vIdx);
          hash_str+= std::to_string(vIdy);
          hash_str+= std::to_string(vIdz);
          auto hash_key = std::hash<std::string>{}(hash_str);
          auto& voxel_hit = hashedMapVoxDose.at(hash_key);
          voxel_hit.SetDose(voxel_hit.GetDose()+v_dose);
    }

    LOGSVC_INFO("GetActivityGeoCentre for Cell");
    auto geo_centre = GetActivityGeoCentre(cp,hashedMapCellDose,false);
    auto wgeo_centre = GetActivityGeoCentre(cp,hashedMapCellDose,true);

    auto fillScoringVolumeTagging = [&](VoxelHit& hit){
      auto mask_tag = cp.GetInFieldMaskTag(hit.GetCentre());
      auto geo_tag = 1./sqrt(hit.GetCentre().diff2(geo_centre));
      auto wgeo_tag = 1./sqrt(hit.GetCentre().diff2(wgeo_centre));
      hit.FillTagging(mask_tag, geo_tag, wgeo_tag);
    };
    LOGSVC_INFO("FillScoringVolumeTagging for Cell");
    for(auto& scoring_volume : hashedMapCellDose){
      fillScoringVolumeTagging(scoring_volume.second);
    }
    
    LOGSVC_INFO("GetActivityGeoCentre for Voxel");
    geo_centre = GetActivityGeoCentre(cp,hashedMapVoxDose,false);
    wgeo_centre = GetActivityGeoCentre(cp,hashedMapVoxDose,true);
    LOGSVC_INFO("FillScoringVolumeTagging for Voxel");
    for(auto& scoring_volume : hashedMapVoxDose){
      fillScoringVolumeTagging(scoring_volume.second);
    }

    std::string c_file = output_dir+"/"+GetJobNameLabel()+"_cell_dose_cp-"+runIdString+".csv";
    WriteIntegratedDoseToCsv(hashedMapCellDose,cp,c_file, false);
    WriteIntegratedDoseToNTuple(hashedMapCellDose,cp,false);

    std::string v_file = output_dir+"/"+GetJobNameLabel()+"_voxelised_cell_dose_cp-"+runIdString+".csv";
    WriteIntegratedDoseToCsv(hashedMapVoxDose,cp,v_file, true);
    WriteIntegratedDoseToNTuple(hashedMapVoxDose,cp,true);

    // cp.DumpFieldMaskToFile();
    cp.DumpVolumeMaskToFile("_",hashedMapCellDose);
    cp.DumpVolumeMaskToFile("_voxelised_",hashedMapVoxDose);

  }
}
//////////////////////////////////////////////////////////////////////////////
///
G4ThreeVector RunSvc::GetActivityGeoCentre(const ControlPoint& cp, const std::map<std::size_t, VoxelHit>& data, bool weighted) const {
  std::vector<const VoxelHit*> in_field_scoring_volume;
  for(auto& scoring_volume : data){
    auto inField = cp.IsInField(scoring_volume.second.GetCentre());
    if(inField)
      in_field_scoring_volume.push_back(&scoring_volume.second);
  }
  G4ThreeVector sum{0,0,0};
  G4double total_dose{0};
  if(weighted){
    std::for_each(in_field_scoring_volume.begin(), in_field_scoring_volume.end(), [&](const VoxelHit* iv) {
          sum += iv->GetCentre() * iv->GetDose();
          total_dose += iv->GetDose();
      });
    return total_dose == 0 ? sum : sum / total_dose;
  } else {
    std::for_each(in_field_scoring_volume.begin(), in_field_scoring_volume.end(), [&](const VoxelHit* iv) {
          sum += iv->GetCentre();
      });
    return in_field_scoring_volume.size() > 0 ? sum / in_field_scoring_volume.size() : sum;
  }
}


////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::WriteIntegratedDoseToCsv(const std::map<std::size_t, VoxelHit>& data, const ControlPoint& cp, const std::string& file, bool voxelised) const {
  auto runId = std::to_string(cp.GetId());
  LOGSVC_INFO("Exporting dose of Ctrl Point {} to CSV file {}",runId, file);
  std::string header = "Cell IdX,Cell IdY,Cell IdZ,X [mm],Y [mm],Z [mm],Dose,MaskTag,GeoTag,wGeoTag,GeoMaskTagDose,wGeoMaskTagDose";
  if (voxelised) // In this case we have more ID columns
    header = "Cell IdX,Cell IdY,Cell IdZ,Voxel IdX,Voxel IdY,Voxel IdZ,X [mm],Y [mm],Z [mm],Dose,MaskTag,GeoTag,wGeoTag,GeoMaskTagDose,wGeoMaskTagDose";
  auto writeVolumeHitDataRaw = [&](std::ofstream& file, const VoxelHit& hit, bool voxelised){
      auto cxId = hit.GetGlobalID(0);
      auto cyId = hit.GetGlobalID(1);
      auto czId = hit.GetGlobalID(2);
      auto vxId = hit.GetID(0);
      auto vyId = hit.GetID(1);
      auto vzId = hit.GetID(2);
      auto volume_centre = hit.GetCentre();
      auto dose = hit.GetDose();
      auto geoTag = hit.GetGeoTag();
      auto wgeoTag = hit.GetWeigthedGeoTag();
      auto inField = hit.GetMaskTag();
      std::string data_raw;
      file <<cxId<<","<<cyId<<","<<czId;
      if(voxelised)
        file <<","<<vxId<<","<<vyId<<","<<vzId;
      file <<","<<volume_centre.getX()<<","<<volume_centre.getY()<<","<<volume_centre.getZ();
      file <<","<<dose<<","<< inField <<","<< geoTag<<","<< wgeoTag;
      file <<","<< dose / ( geoTag * inField );
      file <<","<< dose / ( wgeoTag * inField ) << std::endl;
    };
    std::ofstream c_outFile;
    c_outFile.open(file.c_str(), std::ios::out);
    c_outFile << header << std::endl;
    for(auto& scoring_volume : data){
      writeVolumeHitDataRaw(c_outFile, scoring_volume.second, voxelised);
    }
    c_outFile.close();
}

////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::WriteIntegratedDoseToNTuple(const std::map<std::size_t, VoxelHit>& data, const ControlPoint& cp, bool voxelised) const {

  auto runId = std::to_string(cp.GetId());
  LOGSVC_INFO("Exporting dose of Ctrl Point {} to NTuple...",runId);
  auto output_dir = thisConfig()->GetValue<std::string>("OutputDir");
  std::string voxels = voxelised ? "_voxelized" : std::string();

  auto file_name = output_dir+"/dose3d_integrated_dose_" + runId + voxels + ".root";
  auto tf = std::make_unique<TFile>(file_name.c_str(),"RECREATE");
  auto geo_dir = tf->mkdir("Geometry");
  
  std::vector<double> linearized_dose;
  std::vector<double> linearized_positioning;
  std::vector<int> linearized_id;
  std::vector<int> voxel_linearized_id;
  std::vector<double> fmask_linearized;

  auto fillVolumePositioning = [&](const VoxelHit& hit, bool voxelised){
      linearized_id.emplace_back(hit.GetGlobalID(0));
      linearized_id.emplace_back(hit.GetGlobalID(1));
      linearized_id.emplace_back(hit.GetGlobalID(2));
      auto volume_centre = hit.GetCentre();
      if(voxelised){
        voxel_linearized_id.emplace_back(hit.GetID(0));
        voxel_linearized_id.emplace_back(hit.GetID(1));
        voxel_linearized_id.emplace_back(hit.GetID(2));
      }
      linearized_positioning.emplace_back(volume_centre.getX());
      linearized_positioning.emplace_back(volume_centre.getY());
      linearized_positioning.emplace_back(volume_centre.getZ());
    };

  auto infile_name_list = tf->GetListOfKeys();
  TDirectory* cp_dirs = nullptr;
  if(!(infile_name_list->Contains("RT_Plan"))){
    cp_dirs = tf->mkdir("RT_Plan");
  } else {
    cp_dirs = tf->GetDirectory("RT_Plan");
  }
  infile_name_list = cp_dirs->GetListOfKeys();
  TString cp_name(("CP_"+runId).c_str());
  TDirectory* current_cp_dir = nullptr;
  if(!(infile_name_list->Contains(cp_name))){
    current_cp_dir = cp_dirs->mkdir(cp_name);
  } else {
    current_cp_dir = static_cast<TDirectory*>(cp_dirs->Get(cp_name));
  }

  current_cp_dir->cd();
  for(auto& scoring_volume : data){
    fillVolumePositioning(scoring_volume.second, voxelised);
    linearized_dose.emplace_back(scoring_volume.second.GetDose());
  }

  if((cp.GetId()==0)&&voxelised){ // We do need only one instance of geometry objects 
                                  // -> for all control points
    geo_dir->WriteObject(&linearized_id, "VoxelGlobalID");
    geo_dir->WriteObject(&voxel_linearized_id, "VoxelID");
    geo_dir->WriteObject(&linearized_positioning, "VoxelPosition");
  }
  if((cp.GetId()==0)&&(!voxelised)){
    geo_dir->WriteObject(&linearized_positioning, "CellPosition");
    geo_dir->WriteObject(&linearized_id, "CellID");
  }
  if((voxelised)){
    current_cp_dir->WriteObject(&linearized_dose, "VoxelDose");
  }
  if((!voxelised)){
    current_cp_dir->WriteObject(&linearized_dose, "CellDose");
    //fmask_linearized = cp.GetFieldMaskLinearized();
    //current_cp_dir->WriteObject(&fmask_linearized,"FieldMask");
  }

  tf->Write();
  tf->Close();

}

//////////////////////////////////////////////////////////////////////////////
///
void RunSvc::ExportIntegratedTotalDose() const {

  auto output_dir = thisConfig()->GetValue<std::string>("OutputDir");
  auto file_name = output_dir+"/dose3d_integrated_total_dose.root";
  auto tf = std::make_unique<TFile>(file_name.c_str(),"RECREATE");
  auto output_file = output_dir+"/"+GetJobNameLabel()+".root";
  LOGSVC_INFO("Intagrating total dose from all Control Points: {}",output_file);

  std::vector<double> cell_linearized_dose;
  std::vector<double> voxel_linearized_dose;

  auto patient = PatientGeometry::GetInstance();
  auto det = dynamic_cast<D3DDetector*>(patient->GetPatient());
  auto hashedMapCellDose = det->GetScoringHashedMap("CellDose", false);
  auto hashedMapVoxDose = det->GetScoringHashedMap("VoxelDose", true);

  TFile f(output_file.c_str(),"READ");
  auto tree = static_cast<TTree*>(f.Get("Dose3DVoxelisedTTree"));

  G4int cIdx; tree->SetBranchAddress("CellIdX",&cIdx);
  G4int cIdy; tree->SetBranchAddress("CellIdY",&cIdy);
  G4int cIdz; tree->SetBranchAddress("CellIdZ",&cIdz);

  G4int vIdx; tree->SetBranchAddress("VoxelIdX",&vIdx);
  G4int vIdy; tree->SetBranchAddress("VoxelIdY",&vIdy);
  G4int vIdz; tree->SetBranchAddress("VoxelIdZ",&vIdz);

  G4double c_dose; tree->SetBranchAddress("CellDose",&c_dose);
  G4double v_dose; tree->SetBranchAddress("VoxelDose",&v_dose);

  auto nentries = tree->GetEntries();
  for (Long64_t i=0;i<nentries;i++) {
    tree->GetEntry(i);
    auto hash_str = std::to_string(cIdx);
    hash_str+= std::to_string(cIdy);
    hash_str+= std::to_string(cIdz);
    auto hash_key_c = std::hash<std::string>{}(hash_str);
    auto& cell_hit = hashedMapCellDose.at(hash_key_c);
    cell_hit.SetDose(cell_hit.GetDose()+c_dose);

    hash_str+= std::to_string(vIdx);
    hash_str+= std::to_string(vIdy);
    hash_str+= std::to_string(vIdz);
    auto hash_key = std::hash<std::string>{}(hash_str);
    auto& voxel_hit = hashedMapVoxDose.at(hash_key);
    voxel_hit.SetDose(voxel_hit.GetDose()+v_dose);
    }

  for(auto& cell : hashedMapCellDose){
    cell_linearized_dose.emplace_back(cell.second.GetDose());
  }
  
  for(auto& voxel : hashedMapVoxDose){
    voxel_linearized_dose.emplace_back(voxel.second.GetDose());
  }


  auto cp_dirs = tf->mkdir("RT_Plan");
  auto total_dir = cp_dirs->mkdir("Total");
  total_dir->WriteObject(&cell_linearized_dose,"CellDose");
  total_dir->WriteObject(&voxel_linearized_dose,"VoxelDose");

  tf->Write();
  tf->Close();

}

////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::MergeOutput(bool cleanUp) const {
  auto output_dir = thisConfig()->GetValue<std::string>("OutputDir");
  LOGSVC_INFO("Job output dir: {}",output_dir);
  auto output_file = output_dir+"/"+GetJobNameLabel()+".root";
  TFileMerger fm(kFALSE);
  fm.OutputFile(output_file.c_str());
  // __________________________________________________________________________
  // Handle .root output files - merge them all
  auto sim_dir = ControlPoint::GetOutputDir();
  auto files_to_merge = svc::getFilesInDir(sim_dir,".root");
  auto geo_dir = GeoSvc::GetOutputDir();
  auto files_to_merge_geo = svc::getFilesInDir(geo_dir,".root");
  files_to_merge.insert(std::end(files_to_merge), std::begin(files_to_merge_geo), std::end(files_to_merge_geo));
  for(const auto& file : files_to_merge){
    LOGSVC_DEBUG("AddFile: {}",file);
    fm.AddFile((file).c_str());
  }
  fm.Merge();
  LOGSVC_INFO("Merging to file: {} - done!",output_file);

  if(cleanUp){
    LOGSVC_INFO("Clean-up....");
    for(const auto& file : files_to_merge){
      svc::deleteFileIfExists(file);
    }
  }
}
