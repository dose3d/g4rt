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
    auto world_pv = WorldConstruction::Construct();

    auto envSize = G4ThreeVector(3000.*mm,3000.*mm,2000.*mm);

    auto bunkerWallBox = new G4Box("bunkerWallBox",750.*mm + envSize.getX(), 750.*mm + envSize.getX(),750.*mm + envSize.getX());
    auto myMedium = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", "BaritesConcrete");
    auto bunkerWallBoxLV = new G4LogicalVolume(bunkerWallBox, myMedium.get(), "bunkerWallBoxLV");
    SetPhysicalVolume(new G4PVPlacement(nullptr, G4ThreeVector(0., 0., 0.), "bunkerWallBoxPV", bunkerWallBoxLV, world_pv, false, 0));
    auto wall_pv = GetPhysicalVolume();


    auto pv = GetPhysicalVolume();
    auto Air = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "Usr_G4AIR20C");
    G4Box *bunkerEnv = new G4Box("bunkerEnvBox", envSize.x(),envSize.y(),envSize.z());
    G4LogicalVolume *bunkerEnvLV = new G4LogicalVolume(bunkerEnv, Air.get(), "bunkerEnvLV", 0, 0, 0);
    SetPhysicalVolume(new G4PVPlacement(nullptr, G4ThreeVector(0., 0., 0.), "bunkerEnvBoxPV", bunkerEnvLV, wall_pv, false, 0));



    // TODO 2: Create tray innstances here with pv as a parent
    // auto position = G4ThreeVector(x,y,z);
    // auto halfSize = G4ThreeVector(x,y,z);
    // m_trays.push_back(new D3DTray(pv, "Name", position, halfSize));
    // m_trays.push_back(new D3DTray(pv, "Name", position, halfSize));
    // m_trays.push_back(new D3DTray(pv, "Name", position, halfSize));

    return true;
}