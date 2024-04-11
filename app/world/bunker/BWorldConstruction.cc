#include "BWorldConstruction.hh"
#include "Services.hh"
#include "D3DTray.hh"
#include "G4Box.hh"
#include "Types.hh"

BWorldConstruction::BWorldConstruction(){}

    ///
BWorldConstruction::~BWorldConstruction(){}

WorldConstruction* BWorldConstruction::GetInstance() {
    static auto instance = new BWorldConstruction(); // It's being released by G4GeometryManager
    return instance;
}

bool BWorldConstruction::Create() {
    WorldConstruction::Create();
    auto pv = GetPhysicalVolume();
    // TODO 1: Create walls here with pv as a parent
    auto Air = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "Usr_G4AIR20C");


    // create an envelope box filled with seleceted medium
    auto envSize = G4ThreeVector(3000.,3000.,2000.);
    G4Box *patientEnv = new G4Box("patientEnvBox", envSize.x(),envSize.y(),envSize.z());
    G4LogicalVolume *patientEnvLV = new G4LogicalVolume(patientEnv, Air.get(), "patientEnvLV", 0, 0, 0);
    // TODO 2: Create tray innstances here with pv as a parent
    // auto position = G4ThreeVector(x,y,z);
    // auto halfSize = G4ThreeVector(x,y,z);
    // m_trays.push_back(new D3DTray(pv, "Name", position, halfSize));
    // m_trays.push_back(new D3DTray(pv, "Name", position, halfSize));
    // m_trays.push_back(new D3DTray(pv, "Name", position, halfSize));

    return true;
}