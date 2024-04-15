
#include "D3DTray.hh"
#include "D3DDetector.hh"
#include "Services.hh"
#include "G4Box.hh"


D3DTray::D3DTray(G4VPhysicalVolume *parentPV, const std::string& name, const G4ThreeVector& position, const G4ThreeVector& halfSize)
:IPhysicalVolume(name), m_global_centre(position), m_tray_world_halfSize(halfSize), m_tray_name(name), m_parentPV(parentPV) {
    m_patient = new D3DDetector();
    m_patient->TomlConfig(true);
    m_tray_config_file = "/path/to/config.toml";
    m_patient->SetTomlConfigFile(m_tray_config_file);
} 

void D3DTray::Construct() {

    auto medium = Service<ConfigSvc>()->GetValue<G4MaterialSPtr>("MaterialsSvc", "Usr_G4AIR20C");
    
    auto boxName = m_tray_name + "EnvBox";
    G4Box *patientEnv = new G4Box(m_tray_name, m_tray_world_halfSize.x(), m_tray_world_halfSize.y(), m_tray_world_halfSize.z());
    
    auto LVName = m_tray_name + "EnvLV";
    G4LogicalVolume *patientEnvLV = new G4LogicalVolume(patientEnv, medium.get(), LVName, 0, 0, 0);
    
    auto PVName = m_tray_name + "EnvPV";
    SetPhysicalVolume(new G4PVPlacement(0, m_global_centre, PVName, patientEnvLV, m_parentPV, false, 0));
    
    auto pv = GetPhysicalVolume();

    m_patient->Construct(pv);
    m_patient->WriteInfo();
}