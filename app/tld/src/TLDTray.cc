#include "TLDTray.hh"
#include "Services.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4SystemOfUnits.hh"
#include "VPatientSD.hh"

#include <fstream>
#include <iterator>

namespace {

bool IsSurfaceVoxelIndex(int ix, int iy, int iz, int nx, int ny, int nz, int layers) {
  if (layers <= 0)
    return true;

  auto onSurfaceAxis = [layers](int id, int n) {
    if (n <= 0)
      return false;
    return id < layers || id >= (n - layers);
  };

  return onSurfaceAxis(ix, nx) || onSurfaceAxis(iy, ny) || onSurfaceAxis(iz, nz);
}

int CountSurfaceVoxels(int nx, int ny, int nz, int layers) {
  int count = 0;
  for (int ix = 0; ix < nx; ++ix)
    for (int iy = 0; iy < ny; ++iy)
      for (int iz = 0; iz < nz; ++iz)
        if (IsSurfaceVoxelIndex(ix, iy, iz, nx, ny, nz, layers))
          ++count;
  return count;
}

std::string GetOptionalTomlString(const toml::table& config, const std::string& section, const std::string& key) {
  if (config.contains(section) && config[section].is_table())
    return config[section][key].value_or("");
  return "";
}

double GetOptionalTomlDouble(const toml::table& config, const std::string& section, const std::string& key, double fallback = 0.0) {
  if (config.contains(section) && config[section].is_table())
    return config[section][key].value_or(fallback);
  return fallback;
}

std::shared_ptr<CADMesh::TessellatedMesh> LoadCleanStlMesh(const std::string& inputPath) {
  std::ifstream fin(inputPath, std::ios::binary);
  if (!fin) {
    G4ExceptionDescription msg;
    msg << "Cannot open STL file: " << inputPath;
    G4Exception("TLDTray::LoadCleanStlMesh", "TLDTray001", FatalException, msg);
  }

  std::string content((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());
  fin.close();

  // Remove UTF-8 BOM if present
  const std::string bom = "\xEF\xBB\xBF";
  if (content.rfind(bom, 0) == 0) {
    content.erase(0, bom.size());
  }

  std::string tmpPath = inputPath + ".clean.stl";
  std::ofstream fout(tmpPath, std::ios::binary);
  fout << content;
  fout.close();

  return CADMesh::TessellatedMesh::FromSTL(tmpPath);
}

} // namespace

////////////////////////////////////////////////////////////////////////////////
///
TLDTray::TLDTray(G4VPhysicalVolume *parentPV, const std::string& name)
:VPatient(name), m_tray_name(name) {
    LoadConfiguration();
    Construct(parentPV);
    Service<GeoSvc>()->RegisterPatient(this);
} 

////////////////////////////////////////////////////////////////////////////////
///
void TLDTray::Construct(G4VPhysicalVolume *parentPV) {
    auto medium = Service<ConfigSvc>()->GetValue<G4MaterialSPtr>("MaterialsSvc", "Usr_G4AIR20C");
    
    auto boxName = m_tray_name + "EnvBox";
    G4Box *patientEnv = new G4Box(m_tray_name, m_tray_world_halfSize.x(), m_tray_world_halfSize.y(), m_tray_world_halfSize.z());
    
    auto LVName = m_tray_name + "EnvLV";
    G4LogicalVolume *patientEnvLV = new G4LogicalVolume(patientEnv, medium.get(), LVName, 0, 0, 0);
    
    auto PVName = m_tray_name + "EnvPV";
    SetPhysicalVolume(new G4PVPlacement(&m_rot, m_global_centre, PVName, patientEnvLV, parentPV, false, 0));
    auto patientPV = GetPhysicalVolume();
    // double tld_dist = 2 * TLD::SIZE + 1 * mm;
    double tld_dist = 7.8 * mm;
    auto initial_x = m_global_centre.x() -(m_config.m_nX_tld - 1) * tld_dist / 2.0;
    auto initial_y = m_global_centre.y() -(m_config.m_nY_tld - 1) * tld_dist / 2.0;

    auto z = m_global_centre.z();

    for (int id_x = 0; id_x < m_config.m_nX_tld; ++id_x){
        for (int id_y = 0; id_y < m_config.m_nY_tld; ++id_y){
            std::string tld_name = "TLD_" + std::to_string(id_x) + "_" + std::to_string(id_y);
            auto current_centre = G4ThreeVector(initial_x + id_x*tld_dist,
                                                initial_y + id_y*tld_dist, z);
            m_tld_detectors.push_back(new TLD(tld_name,current_centre, m_config.m_tld_medium, m_config.m_stl_geometry_file_path));
            m_tld_detectors.back()->SetIDs(id_x, id_y, 0);
            m_tld_detectors.back()->SetNVoxels('x', m_config.m_tld_nX_voxels);
            m_tld_detectors.back()->SetNVoxels('y', m_config.m_tld_nY_voxels);
            m_tld_detectors.back()->SetNVoxels('z', m_config.m_tld_nZ_voxels);
            m_tld_detectors.back()->SetSurfaceScoringLayers(m_config.m_surface_scoring_layers);
        }
    }

    for (auto& tld : m_tld_detectors){
        tld->IPhysicalVolume::Construct(this);
        tld->WriteInfo();
    }
    
    ConstructSupplementaryGeometry(patientPV);
}

////////////////////////////////////////////////////////////////////////////////
///
void TLDTray::ConstructSupplementaryGeometry(G4VPhysicalVolume *parentPV) {
    if (m_config.m_supplementary_geometry_path == "None" || m_config.m_supplementary_geometry_path.empty())
        return;

    auto supplementaryGeometryPath = std::string(m_config.m_supplementary_geometry_path);
    if (supplementaryGeometryPath.at(0) != '/') {
        std::string data_path = PROJECT_DATA_PATH;
        supplementaryGeometryPath = data_path + "/" + supplementaryGeometryPath;
    }

    m_supplementary_mesh = CADMesh::TessellatedMesh::FromSTL(supplementaryGeometryPath);
    G4VSolid* solid = m_supplementary_mesh->GetSolid();
    solid->SetName(m_tray_name + "_SupplementarySolid");
    
    auto Medium = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", m_config.m_supplementary_geometry_material);
    auto supplementaryGeometryLV = new G4LogicalVolume(solid, Medium.get(), "LVStl_Supplementary_TLD");
    m_supplementary_volume = new G4PVPlacement(nullptr,
                                               m_config.m_supplementary_geometry_position,
                                               "PVStl_Supplementary_TLD",
                                               supplementaryGeometryLV,
                                               parentPV,
                                               false,
                                               0);
}

////////////////////////////////////////////////////////////////////////////////
///
void TLDTray::DefineSensitiveDetector() {
    for (auto& tld : m_tld_detectors)
        tld->DefineSensitiveDetector();
}

////////////////////////////////////////////////////////////////////////////////
///
void TLDTray::LoadConfiguration(){
    m_rot = G4RotationMatrix();
    m_tray_world_halfSize = G4ThreeVector();  
    m_global_centre = G4ThreeVector(0.0,0.0,0.0);

    m_config.m_tld_medium = "LiF:Mg,Ti";
    m_config.m_surface_scoring_layers = 0;
    m_config.m_supplementary_geometry_path = "None";
    m_config.m_supplementary_geometry_material = "None";
    m_config.m_supplementary_geometry_position = G4ThreeVector(0.0,0.0,0.0);

    ParseTomlConfig();

    m_config.m_initialized = true;
}

////////////////////////////////////////////////////////////////////////////////
///
void TLDTray::ParseTomlConfig(){
    SetTomlConfigFile();
    auto configFile = GetTomlConfigFile();
    if (!svc::checkIfFileExist(configFile)) {
        FATAL_GEO("TLDTray::TConfigurarable::ParseTomlConfig::File {} not fount.", configFile);
        exit(1);
    }
    auto config = toml::parse_file(configFile);
    auto configPrefix = GetTomlConfigPrefix();
    INFO_GEO("TLDTray::Importing configuration from: {}:{}",configFile,configPrefix);

    m_global_centre.setX(config[configPrefix]["Position"][0].value_or(0.0));
    m_global_centre.setY(config[configPrefix]["Position"][1].value_or(0.0));
    m_global_centre.setZ(config[configPrefix]["Position"][2].value_or(0.0));

    auto vox_nX = config[configPrefix]["TLDVoxelization"][0].value_or(0);
    auto vox_nY = config[configPrefix]["TLDVoxelization"][1].value_or(0);
    auto vox_nZ = config[configPrefix]["TLDVoxelization"][2].value_or(0);
    if((vox_nX > 0) && (vox_nY > 0) && (vox_nZ > 0)) {
        m_config.m_tld_nX_voxels = vox_nX;
        m_config.m_tld_nY_voxels = vox_nY;
        m_config.m_tld_nZ_voxels = vox_nZ;
    }

    auto world_x = config[configPrefix]["World_Size"][0].value_or(0.0);
    auto world_y = config[configPrefix]["World_Size"][1].value_or(0.0);
    auto world_z = config[configPrefix]["World_Size"][2].value_or(0.0);
    if ((world_x > 0.0)&&(world_y > 0.0)&&(world_z > 0.0)){
        m_tray_world_halfSize = G4ThreeVector(world_x,world_y,world_z);
    }

    auto tld_id_x = config[configPrefix]["TLDGrid"][0].value_or(0);
    auto tld_id_y = config[configPrefix]["TLDGrid"][1].value_or(0);
    if((tld_id_x > 0)&&( tld_id_y > 0)){
        m_config.m_nX_tld = tld_id_x;
        m_config.m_nY_tld = tld_id_y;
    }

    auto rotX = config[configPrefix]["Rotation"][0].value_or(0);
    if(rotX > 0 ) 
        m_rot.rotateX(rotX*deg);
    auto rotY = config[configPrefix]["Rotation"][1].value_or(0);
    if(rotY > 0 ) 
        m_rot.rotateY(rotY*deg);
    auto rotZ = config[configPrefix]["Rotation"][2].value_or(0);
    if(rotZ > 0 )           
        m_rot.rotateZ(rotZ*deg);

    std::string medium = config[configPrefix]["TLDMedium"].value_or("");
    if (!medium.empty())
        m_config.m_tld_medium = medium;

    std::string stl_geometry_file_path = config[configPrefix]["TLD_Geometry_File_Path"].value_or("");
    if (!stl_geometry_file_path.empty())
        m_config.m_stl_geometry_file_path = stl_geometry_file_path;

    auto surface_layers = config[configPrefix]["SurfaceScoringLayers"].value_or(0);
    if (surface_layers >= 0)
        m_config.m_surface_scoring_layers = surface_layers;

    std::string supplementaryGeometryPath = config[configPrefix]["SupplementaryGeometry"].value_or("");
    if (supplementaryGeometryPath.empty())
        supplementaryGeometryPath = GetOptionalTomlString(config, "PatientGeometry", "SupplementaryGeometry");
    if (!supplementaryGeometryPath.empty())
        m_config.m_supplementary_geometry_path = supplementaryGeometryPath;

    std::string supplementaryGeometryMaterial = config[configPrefix]["SupplementaryGeometryMaterial"].value_or("");
    if (supplementaryGeometryMaterial.empty())
        supplementaryGeometryMaterial = GetOptionalTomlString(config, "PatientGeometry", "SupplementaryGeometryMaterial");
    if (!supplementaryGeometryMaterial.empty())
        m_config.m_supplementary_geometry_material = supplementaryGeometryMaterial;

    if (config[configPrefix]["SupplementaryGeometryPosition"].is_array()) {
        m_config.m_supplementary_geometry_position.setX(config[configPrefix]["SupplementaryGeometryPosition"][0].value_or(0.0));
        m_config.m_supplementary_geometry_position.setY(config[configPrefix]["SupplementaryGeometryPosition"][1].value_or(0.0));
        m_config.m_supplementary_geometry_position.setZ(config[configPrefix]["SupplementaryGeometryPosition"][2].value_or(0.0));
    } else {
        m_config.m_supplementary_geometry_position.setX(GetOptionalTomlDouble(config, "PatientGeometry", "SupplementaryGeometryPositionX", 0.0));
        m_config.m_supplementary_geometry_position.setY(GetOptionalTomlDouble(config, "PatientGeometry", "SupplementaryGeometryPositionY", 0.0));
        m_config.m_supplementary_geometry_position.setZ(GetOptionalTomlDouble(config, "PatientGeometry", "SupplementaryGeometryPositionZ", 0.0));
    }
}

////////////////////////////////////////////////////////////////////////////////
///
std::map<std::size_t, VoxelHit> TLDTray::GetScoringHashedMap(const G4String& scoring_name,Scoring::Type type) const {
    G4cout << "Getting ScoringHashedMap for " << scoring_name << "/" << Scoring::to_string(type) << G4endl;
    std::map<std::size_t, VoxelHit> hashed_map_scoring;

    auto Medium = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", m_config.m_tld_medium);
    auto store_tracks = Service<ConfigSvc>()->GetValue<bool>("RunSvc", "StoreTracks");

    for (auto& tld : m_tld_detectors){
        auto centre = tld->GetGlobalCentre();
        auto idX = tld->GetIdX();
        auto idY = tld->GetIdY();
        auto idZ = tld->GetIdZ();
        auto surfaceLayers = tld->GetSurfaceScoringLayers();

        if (type == Scoring::Type::Voxel) {
            auto tld_sv = tld->GetSD()->GetRunCollectionReferenceScoringVolume(scoring_name,true);
            if(tld_sv == nullptr)
                continue;

            for(int ix = 0; ix < tld_sv->m_nVoxelsX; ix++ ){
              for(int iy = 0; iy < tld_sv->m_nVoxelsY; iy++ ){
                for(int iz = 0; iz < tld_sv->m_nVoxelsZ; iz++ ){
                  if (surfaceLayers > 0 && !IsSurfaceVoxelIndex(ix, iy, iz,
                                                                tld_sv->m_nVoxelsX,
                                                                tld_sv->m_nVoxelsY,
                                                                tld_sv->m_nVoxelsZ,
                                                                surfaceLayers)) {
                    continue;
                  }
                  auto voxelHash = svc::getHashedStrFromIndexes({idX,idY,idZ,ix,iy,iz});
                  hashed_map_scoring[voxelHash] = VoxelHit();
                  auto voxelCentre = tld_sv->GetVoxelCentre(ix,iy,iz);
                  hashed_map_scoring[voxelHash].SetCentre(voxelCentre);
                  hashed_map_scoring[voxelHash].SetStoreTracks(store_tracks);
                  hashed_map_scoring[voxelHash].SetId(ix,iy,iz);
                  hashed_map_scoring[voxelHash].SetGlobalId(idX,idY,idZ);
                  hashed_map_scoring[voxelHash].SetVolume(tld_sv->GetVoxelVolume());
                  hashed_map_scoring[voxelHash].SetMass(Medium->GetDensity() * tld_sv->GetVoxelVolume());
                }
              }
            }
        } else if (type == Scoring::Type::Cell){
            auto tldHash = svc::getHashedStrFromIndexes({idX,idY,idZ});
            hashed_map_scoring[tldHash] = VoxelHit();
            hashed_map_scoring[tldHash].SetCentre(centre);
            hashed_map_scoring[tldHash].SetId(idX,idY,idZ);
            hashed_map_scoring[tldHash].SetGlobalId(idX,idY,idZ);
            hashed_map_scoring[tldHash].SetStoreTracks(store_tracks);

            G4double activeVolume = tld->GetVolume();
            auto tld_sv = tld->GetSD()->GetRunCollectionReferenceScoringVolume(scoring_name,true);
            if (surfaceLayers > 0 && tld_sv != nullptr) {
                activeVolume = CountSurfaceVoxels(tld_sv->m_nVoxelsX,
                                                  tld_sv->m_nVoxelsY,
                                                  tld_sv->m_nVoxelsZ,
                                                  surfaceLayers) * tld_sv->GetVoxelVolume();
            }
            hashed_map_scoring[tldHash].SetVolume(activeVolume);
            hashed_map_scoring[tldHash].SetMass(Medium->GetDensity() * activeVolume);
        }
    }
    return hashed_map_scoring;
}
