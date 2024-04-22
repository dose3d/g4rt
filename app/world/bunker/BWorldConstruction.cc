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
    m_bunker_inside_pv = new G4PVPlacement(nullptr, G4ThreeVector(0., 0., 0.), "bunkerEnvBoxPV", bunkerEnvLV, m_bunker_wall, false, 0);

    return true;
}

void BWorldConstruction::InstallTrayDetectors() {
    
    m_trays.push_back(new D3DTray(m_bunker_inside_pv, "Tray001")); // position+G4ThreeVector(- 390., 390., 0.)
    m_trays.push_back(new D3DTray(m_bunker_inside_pv, "Tray002")); // position+G4ThreeVector(- 390., 130., 0.))
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray003",  position+G4ThreeVector(- 390., -130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray004",  position+G4ThreeVector(- 390., -390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray005",  position+G4ThreeVector(- 130., 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray006",  position+G4ThreeVector(- 130., 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray007",  position+G4ThreeVector(- 130., -130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray008",  position+G4ThreeVector(- 130., -390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray101",  position+G4ThreeVector(1000. - 390., 1000. + 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray102",  position+G4ThreeVector(1000. - 390., 1000. + 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray103",  position+G4ThreeVector(1000. - 390., 1000. - 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray104",  position+G4ThreeVector(1000. - 390., 1000. - 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray105",  position+G4ThreeVector(1000. - 130., 1000. + 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray106",  position+G4ThreeVector(1000. - 130., 1000. + 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray107",  position+G4ThreeVector(1000. - 130., 1000. - 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray108",  position+G4ThreeVector(1000. - 130., 1000. - 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray201",  position+G4ThreeVector(- 1000. - 390., 1000. + 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray202",  position+G4ThreeVector(- 1000. - 390., 1000. + 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray203",  position+G4ThreeVector(- 1000. - 390., 1000. - 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray204",  position+G4ThreeVector(- 1000. - 390., 1000. - 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray205",  position+G4ThreeVector(- 1000. - 130., 1000. + 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray206",  position+G4ThreeVector(- 1000. - 130., 1000. + 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray207",  position+G4ThreeVector(- 1000. - 130., 1000. - 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray208",  position+G4ThreeVector(- 1000. - 130., 1000. - 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray301",  position+G4ThreeVector(1000. - 390., - 1000. + 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray302",  position+G4ThreeVector(1000. - 390., - 1000. + 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray303",  position+G4ThreeVector(1000. - 390., - 1000. - 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray304",  position+G4ThreeVector(1000. - 390., - 1000. - 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray305",  position+G4ThreeVector(1000. - 130., - 1000. + 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray306",  position+G4ThreeVector(1000. - 130., - 1000. + 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray307",  position+G4ThreeVector(1000. - 130., - 1000. - 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray308",  position+G4ThreeVector(1000. - 130., - 1000. - 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray401",  position+G4ThreeVector(-1000. -390., - 1000. + 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray402",  position+G4ThreeVector(-1000. -390., - 1000. + 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray403",  position+G4ThreeVector(-1000. -390., - 1000. - 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray404",  position+G4ThreeVector(-1000. -390., - 1000. - 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray405",  position+G4ThreeVector(-1000. -130., - 1000. + 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray406",  position+G4ThreeVector(-1000. -130., - 1000. + 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray407",  position+G4ThreeVector(-1000. -130., - 1000. - 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray408",  position+G4ThreeVector(-1000. -130., - 1000. - 390., 0.)));

    // rot.rotateZ(180. * deg);

    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray009", position+G4ThreeVector(390., 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray010", position+G4ThreeVector(390., 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray011", position+G4ThreeVector(390., -130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray012", position+G4ThreeVector(390., -390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray013", position+G4ThreeVector(130., 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray014", position+G4ThreeVector(130., 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray015", position+G4ThreeVector(130., -130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray016", position+G4ThreeVector(130., -390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray109", position+G4ThreeVector(1000. + 390., 1000. + 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray110", position+G4ThreeVector(1000. + 390., 1000. + 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray111", position+G4ThreeVector(1000. + 390., 1000. - 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray112", position+G4ThreeVector(1000. + 390., 1000. - 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray113", position+G4ThreeVector(1000. + 130., 1000. + 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray114", position+G4ThreeVector(1000. + 130., 1000. + 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray115", position+G4ThreeVector(1000. + 130., 1000. - 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray116", position+G4ThreeVector(1000. + 130., 1000. - 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray209", position+G4ThreeVector(-1000. + 390., 1000. + 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray210", position+G4ThreeVector(-1000. + 390., 1000. + 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray211", position+G4ThreeVector(-1000. + 390., 1000. - 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray212", position+G4ThreeVector(-1000. + 390., 1000. - 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray213", position+G4ThreeVector(-1000. + 130., 1000. + 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray214", position+G4ThreeVector(-1000. + 130., 1000. + 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray215", position+G4ThreeVector(-1000. + 130., 1000. - 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray216", position+G4ThreeVector(-1000. + 130., 1000. - 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray309", position+G4ThreeVector(1000. + 390., - 1000. + 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray310", position+G4ThreeVector(1000. + 390., - 1000. + 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray311", position+G4ThreeVector(1000. + 390., - 1000. - 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray312", position+G4ThreeVector(1000. + 390., - 1000. - 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray313", position+G4ThreeVector(1000. + 130., - 1000. + 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray314", position+G4ThreeVector(1000. + 130., - 1000. + 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray315", position+G4ThreeVector(1000. + 130., - 1000. - 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray316", position+G4ThreeVector(1000. + 130., - 1000. - 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray409", position+G4ThreeVector(-1000. + 390., - 1000. + 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray410", position+G4ThreeVector(-1000. + 390., - 1000. + 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray411", position+G4ThreeVector(-1000. + 390., - 1000. - 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray412", position+G4ThreeVector(-1000. + 390., - 1000. - 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray413", position+G4ThreeVector(-1000. + 130., - 1000. + 390., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray414", position+G4ThreeVector(-1000. + 130., - 1000. + 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray415", position+G4ThreeVector(-1000. + 130., - 1000. - 130., 0.)));
    // m_trays.push_back(new D3DTray(rot,m_bunker_inside_pv, "Tray416", position+G4ThreeVector(-1000. + 130., - 1000. - 390., 0.)));

    // m_trays.back()->Rotate(rot);
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
