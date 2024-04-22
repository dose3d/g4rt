
#include "D3DTray.hh"
#include "Services.hh"
#include "G4Box.hh"

////////////////////////////////////////////////////////////////////////////////
///
D3DTray::D3DTray(G4VPhysicalVolume *parentPV, const std::string& name)
:IPhysicalVolume(name), TomlConfigModule(name), m_tray_name(name) {
    m_detector = new D3DDetector(m_tray_name);
    LoadConfiguration();
    Construct(parentPV);
} 

////////////////////////////////////////////////////////////////////////////////
///
void D3DTray::Construct(G4VPhysicalVolume *parentPV) {
    auto medium = Service<ConfigSvc>()->GetValue<G4MaterialSPtr>("MaterialsSvc", "Usr_G4AIR20C");
    
    auto boxName = m_tray_name + "EnvBox";
    G4Box *patientEnv = new G4Box(m_tray_name, m_tray_world_halfSize.x(), m_tray_world_halfSize.y(), m_tray_world_halfSize.z());
    
    auto LVName = m_tray_name + "EnvLV";
    G4LogicalVolume *patientEnvLV = new G4LogicalVolume(patientEnv, medium.get(), LVName, 0, 0, 0);
    
    auto PVName = m_tray_name + "EnvPV";
    SetPhysicalVolume(new G4PVPlacement(&m_rot, m_global_centre, PVName, patientEnvLV, parentPV, false, 0));
    
    auto pv = GetPhysicalVolume();

    m_detector->Construct(pv);
    m_detector->WriteInfo();
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DTray::DefineSensitiveDetector() {
    m_detector->DefineSensitiveDetector();
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DTray::LoadConfiguration(){

    // Deafult configuration
    m_rot.rotateY(180.*deg);
    m_tray_world_halfSize = G4ThreeVector(130.,130.,50.);
    
    m_global_centre += G4ThreeVector(- 390., 390., 0.); // TEMP FIXED

    m_det_config.m_top_position_in_env = G4ThreeVector(0.0,0.0,0.0);

    m_det_config.m_cell_nX_voxels = 4;
    m_det_config.m_cell_nY_voxels = 4;
    m_det_config.m_cell_nZ_voxels = 4;

    m_det_config.m_mrow_shift = false;
    m_det_config.m_mlayer_shift = false;

    m_det_config.m_cell_medium = "RMPS470";
    m_det_config.m_in_layer_positioning_module = "dose3d/geo/Tray/4x5x1_tray.csv";
    m_det_config.m_stl_geometry_file_path = "dose3d/geo/Tray/tray.stl";

    m_det_config.m_top_position_in_env = G4ThreeVector(-85.,0.0,0.0); // "centre" of the trey is not in the geometrical centre

    // Any config value is being replaced by the one existing in TOML config 
    ParseTomlConfig();

    // Finally, inject the configuration to the detector instance
    dynamic_cast<D3DDetector*>(m_detector)->SetConfig(m_det_config);
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DTray::ParseTomlConfig(){
    SetTomlConfigFile(); // it set the job main file for searching this configuration
    auto configFile = GetTomlConfigFile();
    if (!svc::checkIfFileExist(configFile)) {
        LOGSVC_CRITICAL("D3DTray::TConfigurarable::ParseTomlConfig::File {} not fount.", configFile);
        exit(1);
    }
    auto configPrefix = GetTomlConfigPrefix();
    LOGSVC_INFO("D3DTray::Importing configuration from: {}:{}",configFile,configPrefix);
}

