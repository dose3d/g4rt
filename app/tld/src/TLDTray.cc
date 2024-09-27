
#include "TLDTray.hh"
#include "Services.hh"
#include "G4Box.hh"

////////////////////////////////////////////////////////////////////////////////
///
TLDTray::TLDTray(G4VPhysicalVolume *parentPV, const std::string& name)
:IPhysicalVolume(name), TomlConfigModule(name), m_tray_name(name) {
    LoadConfiguration();
    Construct(parentPV);
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
    auto pv = new G4PVPlacement(&m_rot, m_global_centre, PVName, patientEnvLV, parentPV, false, 0);
    double tld_dist = 2 * TLD::SIZE + 1 * mm;
    auto initial_x = m_config.m_top_position_in_env.x() - (m_config.m_nX_tld/2) * tld_dist;
    auto initial_y = m_config.m_top_position_in_env.y() - (m_config.m_nX_tld/2) * tld_dist;
    auto z = m_config.m_top_position_in_env.z();

    for (int id_x = 0; id_x < m_config.m_nX_tld; ++id_x){
        for (int id_y = 0; id_y < m_config.m_nY_tld; ++id_y){
            std::string tld_name = "TLD_" + std::to_string(id_x) + "_" + std::to_string(id_y);
            auto current_centre = G4ThreeVector(initial_x + id_x*tld_dist,
                                                initial_y + id_y*tld_dist, z);
            m_tld_detectors.push_back(new TLD(tld_name,current_centre, m_config.m_tld_medium));
            m_tld_detectors.back()->SetIDs(id_x, id_y, 0);
            m_tld_detectors.back()->SetNVoxels('x', m_config.m_tld_nX_voxels);
            m_tld_detectors.back()->SetNVoxels('y', m_config.m_tld_nY_voxels);
            m_tld_detectors.back()->SetNVoxels('z', m_config.m_tld_nZ_voxels);
        }
    }

    for (auto& tld : m_tld_detectors){
        tld->Construct(pv);
        tld->WriteInfo();
    }
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

    // Deafult configuration
    m_rot = G4RotationMatrix(); //.rotateY(180.*deg);
    m_tray_world_halfSize = G4ThreeVector(102.,111.,9.2);
    
    m_global_centre = G4ThreeVector(0.0,0.0,0.0);

    m_config.m_top_position_in_env = G4ThreeVector(0.0,0.0,0.0);

    m_config.m_tld_medium = "RMPS470";

    m_config.m_top_position_in_env = G4ThreeVector(0.0,0.0,0.0);

    // Any config value is being replaced by the one existing in TOML config 
    ParseTomlConfig();

    m_config.m_initialized = true;
}

////////////////////////////////////////////////////////////////////////////////
///
void TLDTray::ParseTomlConfig(){
    SetTomlConfigFile(); // it set the job main file for searching this configuration
    auto configFile = GetTomlConfigFile();
    if (!svc::checkIfFileExist(configFile)) {
        LOGSVC_CRITICAL("TLDTray::TConfigurarable::ParseTomlConfig::File {} not fount.", configFile);
        exit(1);
    }
    auto config = toml::parse_file(configFile);
    auto configPrefix = GetTomlConfigPrefix();
    LOGSVC_INFO("TLDTray::Importing configuration from: {}:{}",configFile,configPrefix);

    m_global_centre.setX(config[configPrefix]["Position"][0].value_or(0.0));
    m_global_centre.setY(config[configPrefix]["Position"][1].value_or(0.0));
    m_global_centre.setZ(config[configPrefix]["Position"][2].value_or(0.0));
    // G4cout << "global_centre: " << m_global_centre << G4endl;

    auto vox_nX = config[configPrefix]["CellVoxelization"][0].value_or(0);
    if(vox_nX > 0 ) 
        m_config.m_tld_nX_voxels = vox_nX;
    auto vox_nY = config[configPrefix]["CellVoxelization"][1].value_or(0);
    if(vox_nY > 0 ) 
        m_config.m_tld_nY_voxels = vox_nY;
    auto vox_nZ = config[configPrefix]["CellVoxelization"][2].value_or(0);
    if(vox_nZ > 0 ) 
        m_config.m_tld_nZ_voxels = vox_nZ;

    auto rotX = config[configPrefix]["Rotation"][0].value_or(0);
    if(rotX > 0 ) 
        m_rot.rotateX(rotX*deg);
    auto rotY = config[configPrefix]["Rotation"][1].value_or(0);
    if(rotY > 0 ) 
        m_rot.rotateY(rotY*deg);
    auto rotZ = config[configPrefix]["Rotation"][2].value_or(0);
    if(rotZ > 0 )           
        m_rot.rotateZ(rotZ*deg);
}

