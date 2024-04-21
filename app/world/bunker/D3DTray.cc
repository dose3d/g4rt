
#include "D3DTray.hh"
#include "D3DDetector.hh"
#include "Services.hh"
#include "G4Box.hh"

D3DTray::TConfigurarable D3DTray::m_tconfigurable = D3DTray::TConfigurarable("D3DTray");


D3DTray::D3DTray(G4RotationMatrix& rotMatrix, G4VPhysicalVolume *parentPV, const std::string& name, const G4ThreeVector& position)
:IPhysicalVolume(name), m_rot(rotMatrix), m_global_centre(position), m_tray_world_halfSize(G4ThreeVector(130.,130.,50.)), m_tray_name(name) {
    m_detector = new D3DDetector(m_tray_name);
    auto config = D3DDetector::Config();

    config.m_top_position_in_env = G4ThreeVector(0.0,0.0,0.0);

    config.m_cell_nX_voxels = 4;
    config.m_cell_nY_voxels = 4;
    config.m_cell_nZ_voxels = 4;

    config.m_mrow_shift = false;
    config.m_mlayer_shift = false;

    config.m_cell_medium = "RMPS470";
    config.m_in_layer_positioning_module = "dose3d/geo/Tray/4x5x1_tray.csv";
    config.m_stl_geometry_file_path = "dose3d/geo/Tray/tray.stl";

    config.m_top_position_in_env = G4ThreeVector(-85.,0.0,0.0); // "centre" of the trey is not in the geometrical centre


    dynamic_cast<D3DDetector*>(m_detector)->SetConfig(config);
    Construct(parentPV);
} 

void D3DTray::Construct(G4VPhysicalVolume *parentPV) {
    m_tconfigurable.ParseTomlConfig();
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

void D3DTray::DefineSensitiveDetector() {
    m_detector->DefineSensitiveDetector();
}

void D3DTray::TConfigurarable::ParseTomlConfig(){
    SetTomlConfigFile(); // it set the job main file for searching this configuration
    auto configFile = GetTomlConfigFile();
    if (!svc::checkIfFileExist(configFile)) {
        LOGSVC_CRITICAL("D3DTray::TConfigurarable::ParseTomlConfig::File {} not fount.", configFile);
        exit(1);
    }


    auto configPrefix = GetTomlConfigPrefix();
    LOGSVC_INFO("D3DTray::Importing configuration from: {}:{}",configFile,configPrefix);
}

