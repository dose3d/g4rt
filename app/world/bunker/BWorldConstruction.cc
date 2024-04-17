#include "BWorldConstruction.hh"
#include "Services.hh"
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

    auto myMedium = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", "BaritesConcrete");
    G4Box *bunkerWallBox = new G4Box("bunkerWallBox",750.*mm + envSize.getX(), 750.*mm + envSize.getY(),750.*mm + envSize.getZ());
    G4LogicalVolume *bunkerWallBoxLV = new G4LogicalVolume(bunkerWallBox, myMedium.get(), "bunkerWallBoxLV", 0, 0, 0);
    m_bunker_wall = new G4PVPlacement(nullptr, G4ThreeVector(0., 0., 0.), "bunkerWallBoxPV", bunkerWallBoxLV, world_pv, false, 0);
    
    auto Air = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "Usr_G4AIR20C");
    G4Box *bunkerEnv = new G4Box("bunkerEnvBox", envSize.x(),envSize.y(),envSize.z());
    G4LogicalVolume *bunkerEnvLV = new G4LogicalVolume(bunkerEnv, Air.get(), "bunkerEnvLV", 0, 0, 0);
    m_bunker_inside = new G4PVPlacement(nullptr, G4ThreeVector(0., 0., 0.), "bunkerEnvBoxPV", bunkerEnvLV, m_bunker_wall, false, 0);
    // new G4PVPlacement(nullptr, G4ThreeVector(0., 0., 0.), "bunkerEnvBoxPV", bunkerEnvLV, wall_pv, false, 0);





    return true;
}

void BWorldConstruction::InstallTrayDetectors() {
    
    auto halfSize = G4ThreeVector(130.,130.,50.);
    auto position = G4ThreeVector(0.,0.,0.);
    m_trays.push_back(new D3DTray(m_bunker_inside, "Tray1", position+G4ThreeVector(130.,130.,0.), halfSize));
    m_trays.push_back(new D3DTray(m_bunker_inside, "Tray2", position+G4ThreeVector(130,-130.,0.), halfSize));
    m_trays.push_back(new D3DTray(m_bunker_inside, "Tray3", position+G4ThreeVector(-130.,130.,0.), halfSize));
    m_trays.push_back(new D3DTray(m_bunker_inside, "Tray4", position+G4ThreeVector(-130.,-130.,0.), halfSize));
}

void BWorldConstruction::ConstructSDandField() {
    WorldConstruction::ConstructSDandField();
    for(auto& tray : m_trays){
    tray->DefineSensitiveDetector();
    }
}

std::vector<VPatient*> BWorldConstruction::GetCustomDetectors() const {
    std::vector<VPatient*> customDetectors;
    for(const auto& tray : m_trays){
        customDetectors.push_back(tray->GetDetector());
    }
    return std::move(customDetectors);
}
