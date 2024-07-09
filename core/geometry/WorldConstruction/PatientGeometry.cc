#include "PatientGeometry.hh"
#include "WaterPhantom.hh"
#include "SciSlicePhantom.hh"
#include "DishCubePhantom.hh"
#include "G4ProductionCuts.hh"
#include "D3DDetector.hh"
#include "G4SystemOfUnits.hh"
#include "Services.hh"
#include "G4Box.hh"
#include "TomlConfigModule.hh"
#include "WorldConstruction.hh"
#include "IO.hh"

namespace {
  G4Mutex phantomConstructionMutex = G4MUTEX_INITIALIZER;
}

////////////////////////////////////////////////////////////////////////////////
///
PatientGeometry::PatientGeometry()
      :IPhysicalVolume("PatientGeometry"), Configurable("PatientGeometry"){
    Configure();
  }

////////////////////////////////////////////////////////////////////////////////
///
PatientGeometry::~PatientGeometry() {
  configSvc()->Unregister(thisConfig()->GetName());
}

////////////////////////////////////////////////////////////////////////////////
///
PatientGeometry* PatientGeometry::GetInstance() {
  static PatientGeometry instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
///
void PatientGeometry::Configure() {
  G4cout << "\n\n[INFO]::  Configuring the " << thisConfig()->GetName() << G4endl;
  DefineUnit<std::string>("Type");
  DefineUnit<double>("EnviromentPositionX");
  DefineUnit<double>("EnviromentPositionY");
  DefineUnit<double>("EnviromentPositionZ");
  DefineUnit<double>("EnviromentSizeX");
  DefineUnit<double>("EnviromentSizeY");
  DefineUnit<double>("EnviromentSizeZ");
  DefineUnit<std::string>("EnviromentMedium");
  DefineUnit<std::string>("ConfigFile");
  DefineUnit<std::string>("ConfigPrefix");
  DefineUnit<double>("VoxelSizeXCT");
  DefineUnit<double>("VoxelSizeYCT");
  DefineUnit<double>("VoxelSizeZCT");

  Configurable::DefaultConfig();   // setup the default configuration for all defined units/parameters
  // G4cout << "[DEBUG]:: PatientGeometry:: Configure: DefaultConfig"<< G4endl;
  // Configurable::PrintConfig();
}

////////////////////////////////////////////////////////////////////////////////
///



void PatientGeometry::DefaultConfig(const std::string &unit) {
  // Volume name
  if (unit.compare("Label") == 0){
    thisConfig()->SetValue(unit, std::string("Patient environmet"));
    }
  if (unit.compare("Type") == 0){
    thisConfig()->SetTValue<std::string>(unit, std::string("None")); // "DishCubePhantom"  ,WaterPhantom , SciSlicePhantom, Dose3D
    // thisConfig()->SetValue(unit, std::string("Dose3D")); // "DishCubePhantom"  ,WaterPhantom , SciSlicePhantom, Dose3D
    }
  // default box size
  if (unit.compare("EnviromentPositionX") == 0){
    thisConfig()->SetTValue<double>(unit, 0.0);
    }
  if (unit.compare("EnviromentPositionY") == 0){
    thisConfig()->SetTValue<double>(unit, 0.0);
    }
  if (unit.compare("EnviromentPositionZ") == 0){
    thisConfig()->SetTValue<double>(unit, 0.0);
    }

  if (unit.compare("EnviromentSizeX") == 0){
    thisConfig()->SetTValue<double>(unit, 0.0);
    }

  if (unit.compare("EnviromentSizeY") == 0){
    thisConfig()->SetTValue<double>(unit, 0.0);
    }

  if (unit.compare("EnviromentSizeZ") == 0){
    thisConfig()->SetTValue<double>(unit, 0.0);
    }
 
 if (unit.compare("EnviromentMedium") == 0){
    thisConfig()->SetTValue<std::string>(unit, std::string("None"));
    }

 if (unit.compare("ConfigFile") == 0){
    thisConfig()->SetTValue<std::string>(unit, std::string("None"));
    }
  if (unit.compare("ConfigPrefix") == 0){
    thisConfig()->SetTValue<std::string>(unit, std::string("None"));
    }
  if (unit.compare("VoxelSizeXCT") == 0){
    thisConfig()->SetTValue<double>(unit, double(1.00));
    }
  if (unit.compare("VoxelSizeYCT") == 0){
    thisConfig()->SetTValue<double>(unit, double(1.00));
    }
  if (unit.compare("VoxelSizeZCT") == 0){
    thisConfig()->SetTValue<double>(unit, double(1.00));
    }

}

////////////////////////////////////////////////////////////////////////////////
///
bool PatientGeometry::design(void) {
  auto patientType = thisConfig()->GetValue<std::string>("Type");
  G4cout << "I'm building " << patientType << "  patient geometry" << G4endl;

  if (patientType == "WaterPhantom") {
    m_patient = new WaterPhantom();
    m_patient->TomlConfig(true);
  }
  else if (patientType == "SciSlicePhantom"){
    m_patient = new SciSlicePhantom();
  }
  else if (patientType == "DishCubePhantom"){
    m_patient = new DishCubePhantom();
  }
  else if (patientType == "D3DDetector") {
    m_patient = new D3DDetector();
    m_patient->TomlConfig(true);
  }
  else 
    return false;

  // TOML-like contextual configuration
  if(m_patient->TomlConfig()){ 
    auto configFile =thisConfig()->GetValue<std::string>("ConfigFile");
    if(configFile.empty() || configFile=="None")
      m_patient->SetTomlConfigFile(); // get the job main file
    else{
      std::string projectPath = PROJECT_LOCATION_PATH;
      m_patient->SetTomlConfigFile(projectPath+configFile);
    }
    configFile = m_patient->GetTomlConfigFile();
    G4cout << "PatientGeometry::ConfigFile:: Importing configuration from:\n"<< configFile << "\n" << G4endl;
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
void PatientGeometry::Destroy() {
  auto pv = GetPhysicalVolume();
  if (pv) {
    if (m_patient) m_patient->Destroy();
    delete pv;
    SetPhysicalVolume(nullptr);
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void PatientGeometry::Construct(G4VPhysicalVolume *parentPV) {
  PrintConfig();
  design(); // a call to select the right phantom
  auto isoToSim = Service<ConfigSvc>()->GetValue<G4ThreeVector>("WorldConstruction", "IsoToSimTransformation");

  auto mediumName = thisConfig()->GetValue<std::string>("EnviromentMedium");
  auto medium = Service<ConfigSvc>()->GetValue<G4MaterialSPtr>("MaterialsSvc", mediumName);

  // create an envelope box filled with seleceted medium
  auto envSize = G4ThreeVector(thisConfig()->GetValue<double>("EnviromentSizeX")/2.,thisConfig()->GetValue<double>("EnviromentSizeY")/2.,thisConfig()->GetValue<double>("EnviromentSizeZ")/2.);
  G4Box *patientEnv = new G4Box("patientEnvBox", envSize.x(),envSize.y(),envSize.z());
  G4LogicalVolume *patientEnvLV = new G4LogicalVolume(patientEnv, medium.get(), "patientEnvLV", 0, 0, 0);
  // The envelope box will bo located at given point with respect to the parentPV.
  // However it shifted to Sim locatin (ie. to the positive querter of the World coordinate system)
  auto envPosX = thisConfig()->GetValue<double>("EnviromentPositionX");
  auto envPosY = thisConfig()->GetValue<double>("EnviromentPositionY");
  auto envPosZ = thisConfig()->GetValue<double>("EnviromentPositionZ");

  // Region for cuts
  auto regVol = new G4Region("phantomEnviromentRegion");
  auto cuts = new G4ProductionCuts;
  cuts->SetProductionCut(0.5 * mm);
  regVol->SetProductionCuts(cuts);
  patientEnvLV->SetRegion(regVol);
  regVol->AddRootLogicalVolume(patientEnvLV);
  SetPhysicalVolume(new G4PVPlacement(m_rotation, G4ThreeVector(envPosX,envPosY,envPosZ), "phmWorldPV", patientEnvLV, parentPV, false, 0));
  auto pv = GetPhysicalVolume();
  // create the actual phantom
  m_patient->Construct(pv);
  m_patient->WriteInfo();

  // Creation of bed?
  auto tableMaterial = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_POLYACRYLONITRILE");
  auto tableHeight =  7.0*mm;
  auto tableBox = new G4Box("TableBox", 1100.0*mm, 225.0*mm, tableHeight);
  auto dcoverLV = new G4LogicalVolume(tableBox, tableMaterial.get(), "TableBoxLV");
  SetPhysicalVolume(new G4PVPlacement(nullptr, G4ThreeVector(900.0,0.0,((1.0*mm)+tableHeight+envPosZ+envSize.z())), "CoverBoxPV", dcoverLV, parentPV, false, 0));





}

////////////////////////////////////////////////////////////////////////////////
///
G4bool PatientGeometry::Update() {
  // TODO:: Update this GetPhysicalVolume(); then the daughter
  if (m_patient) {
    if (!m_patient->Update()) return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
void PatientGeometry::WriteInfo() {
  auto envPosX = thisConfig()->GetValue<double>("EnviromentPositionX");
  auto envPosY = thisConfig()->GetValue<double>("EnviromentPositionY");
  auto envPosZ = thisConfig()->GetValue<double>("EnviromentPositionZ");
  auto centre = G4ThreeVector(envPosX,envPosY,envPosZ);
  G4cout << "Phantom centre: " << centre / cm << " [cm] " << G4endl; 
}

////////////////////////////////////////////////////////////////////////////////
/// NOTE: This method is called from WorldConstruction::ConstructSDandField
///       which is being called in workers in MT mode
void PatientGeometry::DefineSensitiveDetector() {
  if (m_patient){
    // check if there is any analysis switched on in patient:
    auto configSvc = Service<ConfigSvc>();
    if(configSvc->GetValue<bool>("RunSvc", "StepAnalysis") ||
       configSvc->GetValue<bool>("RunSvc", "RunAnalysis") ||
       configSvc->GetValue<bool>("RunSvc", "NTupleAnalysis") ) {
      G4AutoLock lock(&phantomConstructionMutex);
      m_patient->DefineSensitiveDetector();
    } else {
      std::string worker = G4Threading::IsWorkerThread() ? "worker" : "master";
      LOGSVC_WARN("No sensitive detector defined for patient. Any analysis is switched on ({})!",worker);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void PatientGeometry::ExportToCsvCT(const std::string& path_to_output_dir) const {
  auto patientEnv = Service<GeoSvc>()->World()->PatientEnvironment();
  if (!patientEnv) {
    return;
  }
  auto patientInstance = patientEnv->GetPatient();

  auto g4Navigator = std::make_unique<G4Navigator>();
  auto worldInstance = Service<GeoSvc>()->World();
  g4Navigator->SetWorldVolume(worldInstance->GetPhysicalVolume());
  
  IO::CreateDirIfNotExits(path_to_output_dir);

  G4String materialName;
  G4ThreeVector currentPos;

  auto patientPositionInWorldEnv = patientInstance->GetPatientTopPositionInWolrdEnv();

  auto env_size_x = thisConfig()->GetValue<double>("EnviromentSizeX");
  auto ct_cube_init_x = -svc::round_with_prec(env_size_x/2 + thisConfig()->GetValue<double>("EnviromentPositionX"),4);

  auto env_size_y = thisConfig()->GetValue<double>("EnviromentSizeY");
  auto ct_cube_init_y = -svc::round_with_prec(env_size_y/2 + thisConfig()->GetValue<double>("EnviromentPositionY"),4);

  auto env_size_z = thisConfig()->GetValue<double>("EnviromentSizeZ");
  auto ct_cube_init_z = -svc::round_with_prec(env_size_z/2 + thisConfig()->GetValue<double>("EnviromentPositionZ"),4);

  auto sizeX = thisConfig()->GetValue<double>("VoxelSizeXCT"); 
  auto sizeY = thisConfig()->GetValue<double>("VoxelSizeYCT"); 
  auto sizeZ = thisConfig()->GetValue<double>("VoxelSizeZCT"); 

  G4int xResolution = env_size_x / sizeX;
  G4int yResolution = env_size_y / sizeY;
  G4int zResolution = env_size_z / sizeZ;

  LOGSVC_INFO("ExportToCsvCT: Resolution: x {}, y {}, z {}", xResolution, yResolution, zResolution);

  // DUMP METADATA TO FILE 
  auto meta =  path_to_output_dir+"/../ct_series_metadata.csv";
  std::ofstream metadata_file;
  metadata_file.open(meta.c_str(), std::ios::out);

  metadata_file << "x_min," << ct_cube_init_x  << std::endl;
  metadata_file << "y_min," << ct_cube_init_y  << std::endl;
  metadata_file << "z_min," << ct_cube_init_z  << std::endl;

  metadata_file << "x_max," << svc::round_with_prec((ct_cube_init_x+env_size_x),4) << std::endl;
  metadata_file << "y_max," << svc::round_with_prec((ct_cube_init_y+env_size_y),4) << std::endl;
  metadata_file << "z_max," << svc::round_with_prec((ct_cube_init_z+env_size_z),4) << std::endl;

  metadata_file << "x_resolution," << xResolution << std::endl;
  metadata_file << "y_resolution," << yResolution << std::endl;
  metadata_file << "z_resolution," << zResolution << std::endl;

  metadata_file << "x_step," << sizeX << std::endl;
  metadata_file << "y_step," << sizeY << std::endl;
  metadata_file << "z_step," << sizeZ << std::endl;

  double source_to_isocentre = 1000;
  metadata_file << "SSD," << svc::round_with_prec((source_to_isocentre + patientPositionInWorldEnv.getZ()),4) << std::endl;

  for( int y = 0; y < yResolution; y++ ){
  std::ostringstream ss;
  ss << std::setw(4) << std::setfill('0') << y+1 ;
  std::string s2(ss.str());
  auto file =  path_to_output_dir+"/img"+s2+".csv";
  // G4cout << "output filepath:  " << file << G4endl;
  std::string header = "X [mm],Y [mm],Z [mm],Material";
  std::ofstream c_outFile;
  c_outFile.open(file.c_str(), std::ios::out);
  c_outFile << header << std::endl;
  for( int x = 0; x < xResolution; x++ ){
    for( int z = 0; z < zResolution; z++ ){
      currentPos.setX((ct_cube_init_x+sizeX*x));
      currentPos.setY((ct_cube_init_y+sizeY*y));
      currentPos.setZ((ct_cube_init_z+sizeZ*z));
      materialName = g4Navigator->LocateGlobalPointAndSetup(currentPos)->GetLogicalVolume()->GetMaterial()->GetName();
      c_outFile << currentPos.getX() << "," << currentPos.getY() << "," << currentPos.getZ() << "," << materialName << std::endl;
    }
  }
  c_outFile.close();
}

}  

////////////////////////////////////////////////////////////////////////////////
///
void PatientGeometry::ExportDoseToCsvCT(const G4Run* runPtr) const {
  auto patientEnv = Service<GeoSvc>()->World()->PatientEnvironment();
  if (!patientEnv) {
    return;
  }
  auto patientInstance = patientEnv->GetPatient();

  auto g4Navigator = std::make_unique<G4Navigator>();
  auto worldInstance = Service<GeoSvc>()->World();
  g4Navigator->SetWorldVolume(worldInstance->GetPhysicalVolume());

  auto cp = Service<RunSvc>()->CurrentControlPoint();
  auto run_id = std::to_string(runPtr->GetRunID());
  auto path_to_output_dir = cp->GetOutputDir()+"_ct_dose_"+run_id;
  
  IO::CreateDirIfNotExits(path_to_output_dir);

  G4String materialName;
  G4ThreeVector currentPos;

  auto patientPositionInWorldEnv = patientInstance->GetPatientTopPositionInWolrdEnv();

  auto env_size_x = thisConfig()->GetValue<double>("EnviromentSizeX");
  auto ct_cube_init_x = -svc::round_with_prec(env_size_x/2 + thisConfig()->GetValue<double>("EnviromentPositionX"),4);

  auto env_size_y = thisConfig()->GetValue<double>("EnviromentSizeY");
  auto ct_cube_init_y = -svc::round_with_prec(env_size_y/2 + thisConfig()->GetValue<double>("EnviromentPositionY"),4);

  auto env_size_z = thisConfig()->GetValue<double>("EnviromentSizeZ");
  auto ct_cube_init_z = -svc::round_with_prec(env_size_z/2 + thisConfig()->GetValue<double>("EnviromentPositionZ"),4);

  auto sizeX = thisConfig()->GetValue<double>("VoxelSizeXCT"); 
  auto sizeY = thisConfig()->GetValue<double>("VoxelSizeYCT"); 
  auto sizeZ = thisConfig()->GetValue<double>("VoxelSizeZCT"); 

  G4int xResolution = env_size_x / sizeX;
  G4int yResolution = env_size_y / sizeY;
  G4int zResolution = env_size_z / sizeZ;

  LOGSVC_INFO("ExportDoseToCsvCT: Resolution: x {}, y {}, z {}", xResolution, yResolution, zResolution);

  // DUMP METADATA TO FILE 
  auto meta =  path_to_output_dir+"/../ct_series_metadata.csv";
  std::ofstream metadata_file;
  metadata_file.open(meta.c_str(), std::ios::out);

  metadata_file << "x_min," << ct_cube_init_x  << std::endl;
  metadata_file << "y_min," << ct_cube_init_y  << std::endl;
  metadata_file << "z_min," << ct_cube_init_z  << std::endl;

  metadata_file << "x_max," << svc::round_with_prec((ct_cube_init_x+env_size_x),4) << std::endl;
  metadata_file << "y_max," << svc::round_with_prec((ct_cube_init_y+env_size_y),4) << std::endl;
  metadata_file << "z_max," << svc::round_with_prec((ct_cube_init_z+env_size_z),4) << std::endl;

  metadata_file << "x_resolution," << xResolution << std::endl;
  metadata_file << "y_resolution," << yResolution << std::endl;
  metadata_file << "z_resolution," << zResolution << std::endl;

  metadata_file << "x_step," << sizeX << std::endl;
  metadata_file << "y_step," << sizeY << std::endl;
  metadata_file << "z_step," << sizeZ << std::endl;

  double source_to_isocentre = 1000;
  metadata_file << "SSD," << svc::round_with_prec((source_to_isocentre + patientPositionInWorldEnv.getZ()),4) << std::endl;

  // std::vector<std::pair<double,size_t>> xMappedVoxels;
  std::vector<std::pair<double,std::pair<size_t,size_t>>> xMappedVoxels;
  std::vector<std::pair<double,std::pair<size_t,size_t>>> yMappedVoxels;
  std::vector<std::pair<double,std::pair<size_t,size_t>>> zMappedVoxels;
  std::unordered_set<std::pair<double, size_t>, pair_hash> addedPairs;

  const auto& scoring_maps = cp->GetRun()->GetScoringCollections();
  for(auto& scoring_map: scoring_maps){
    for(auto& scoring: scoring_map.second){
      auto scoring_type = scoring.first;
      auto& data = scoring.second;
      if(scoring_type==Scoring::Type::Voxel){      
        for(auto& voxel: data){
          auto& voxel_data = voxel.second;
          std::pair<double, size_t> pairToCheckX = std::make_pair(
            voxel_data.GetCentre().getX(), 
            std::hash<std::string>{}(
            "XCell" + std::to_string(voxel_data.GetGlobalID(0)) + 
            "Voxel" + std::to_string(voxel_data.GetID(0))
            ));
          std::pair<double, size_t> pairToCheckY = std::make_pair(
            voxel_data.GetCentre().getY(), std::hash<std::string>{}(
            "YCell" + std::to_string(voxel_data.GetGlobalID(1)) + 
            "Voxel" + std::to_string(voxel_data.GetID(1))));

          std::pair<double, size_t> pairToCheckZ = std::make_pair(
            voxel_data.GetCentre().getZ(), std::hash<std::string>{}(
            "ZCell" + std::to_string(voxel_data.GetGlobalID(2)) + 
            "Voxel" + std::to_string(voxel_data.GetID(2))));

          if(addedPairs.find(pairToCheckX) == addedPairs.end()) {
              auto idsx = std::make_pair(voxel_data.GetGlobalID(0),voxel_data.GetID(0));
              xMappedVoxels.push_back(std::make_pair(voxel_data.GetCentre().getX(),idsx));
              addedPairs.insert(pairToCheckX);            
          }
          if(addedPairs.find(pairToCheckY) == addedPairs.end()) {
            auto idsy = std::make_pair(voxel_data.GetGlobalID(1),voxel_data.GetID(1));
            yMappedVoxels.push_back(std::make_pair(voxel_data.GetCentre().getY(),idsy));
            addedPairs.insert(pairToCheckY);
          }
          if(addedPairs.find(pairToCheckZ) == addedPairs.end()) {
            auto idsz = std::make_pair(voxel_data.GetGlobalID(2),voxel_data.GetID(2));
            zMappedVoxels.push_back(std::make_pair(voxel_data.GetCentre().getZ(),idsz));
            addedPairs.insert(pairToCheckZ);
          }
        }
      }
    }
  }

    // Custom comparator to sort by the first element of the pair
    auto compareByFirst = [](const std::pair<double,std::pair<size_t,size_t>>& a, const std::pair<double,std::pair<size_t,size_t>>& b) {
        return a.first < b.first;
    };

  std::sort(xMappedVoxels.begin(), xMappedVoxels.end(), compareByFirst);
  std::sort(yMappedVoxels.begin(), yMappedVoxels.end(), compareByFirst);
  std::sort(zMappedVoxels.begin(), zMappedVoxels.end(), compareByFirst);

  auto magicLambdaToLocateDoseInCurrentPosition = [](G4ThreeVector position, 
  std::vector<std::pair<double,std::pair<size_t,size_t>>> xVec, 
  std::vector<std::pair<double,std::pair<size_t,size_t>>> yVec, 
  std::vector<std::pair<double,std::pair<size_t,size_t>>> zVec, 
  double halfsize) {
    std::pair<size_t,size_t> closestXPoint = xVec[0].second;
    std::pair<size_t,size_t> closestYPoint = xVec[0].second;
    std::pair<size_t,size_t> closestZPoint = xVec[0].second;
    
    double minX = xVec.front().first - halfsize;
    double minY = yVec.front().first - halfsize;
    double minZ = zVec.front().first - halfsize; 
    double maxX = xVec.back().first + halfsize;
    double maxY = yVec.back().first + halfsize;
    double maxZ = zVec.back().first + halfsize;
    
    double minXDistance = std::abs(position.x() - xVec[0].first);
    double minYDistance = std::abs(position.y() - yVec[0].first);
    double minZDistance = std::abs(position.z() - zVec[0].first);

    if ((position.x() > maxX) || (position.y() > maxY) || (position.z() > maxZ) || (position.x() < minX) || (position.y() < minY) || (position.z() < minZ)) {
        return 0.;
    }
    for (const std::pair<double,std::pair<size_t,size_t>>& point : xVec) {
        double distance = std::abs(position.x() - point.first);
        if (distance < minXDistance) {
            minXDistance = distance;
            closestXPoint = point.second;
        }
    }

    for (const std::pair<double,std::pair<size_t,size_t>>& point : yVec) {
        double distance = std::abs(position.y() - point.first);
        if (distance < minYDistance) {
            minYDistance = distance;
            closestYPoint = point.second;
        }
    }

    for (const std::pair<double,std::pair<size_t,size_t>>& point : zVec) {
        double distance = std::abs(position.z() - point.first);
        if (distance < minZDistance) {
            minZDistance = distance;
            closestZPoint = point.second;
        }
    }
    // std::cout << "CellIdX: " << closestXPoint.first << " VoxelIdX: " << closestXPoint.second << " CellIdY: " <<  closestYPoint.first 
    // << " VoxelIdY: " << closestYPoint.second << " CellIdZ: " << closestZPoint.first << " VoxelIdZ: " << closestZPoint.second << std::endl;
    return 1.0*closestZPoint.first+ zVec.size()*closestYPoint.first + zVec.size()*yVec.size()*closestXPoint.first;
  };



  double dose = 0.;


  for( int y = 0; y < yResolution; y++ ){
    std::ostringstream ss;
    ss << std::setw(4) << std::setfill('0') << y+1 ;
    std::string s2(ss.str());
    auto file =  path_to_output_dir+"/img"+s2+".csv";
    // G4cout << "output filepath:  " << file << G4endl;
    std::string header = "X [mm],Y [mm],Z [mm],Material,Dose [Gy]";
    std::ofstream c_outFile;
    c_outFile.open(file.c_str(), std::ios::out);
    c_outFile << header << std::endl;
    for( int x = 0; x < xResolution; x++ ){
      for( int z = 0; z < zResolution; z++ ){
        currentPos.setX((ct_cube_init_x+sizeX*x));
        currentPos.setY((ct_cube_init_y+sizeY*y));
        currentPos.setZ((ct_cube_init_z+sizeZ*z));
        materialName = g4Navigator->LocateGlobalPointAndSetup(currentPos)->GetLogicalVolume()->GetMaterial()->GetName();
        dose = magicLambdaToLocateDoseInCurrentPosition(currentPos,xMappedVoxels,yMappedVoxels,zMappedVoxels, 0.5);
        c_outFile << currentPos.getX() << "," << currentPos.getY() << "," << currentPos.getZ() << "," << materialName  << "," << dose << std::endl;
      }
    }
    c_outFile.close();
  }
}

