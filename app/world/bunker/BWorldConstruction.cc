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
    G4RotationMatrix rot;
    rot.rotateY(180.*deg);
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray001",  position+G4ThreeVector(- 390., 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray002",  position+G4ThreeVector(- 390., 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray003",  position+G4ThreeVector(- 390., -130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray004",  position+G4ThreeVector(- 390., -390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray005",  position+G4ThreeVector(- 130., 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray006",  position+G4ThreeVector(- 130., 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray007",  position+G4ThreeVector(- 130., -130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray008",  position+G4ThreeVector(- 130., -390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray101",  position+G4ThreeVector(1000. - 390., 1000. + 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray102",  position+G4ThreeVector(1000. - 390., 1000. + 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray103",  position+G4ThreeVector(1000. - 390., 1000. - 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray104",  position+G4ThreeVector(1000. - 390., 1000. - 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray105",  position+G4ThreeVector(1000. - 130., 1000. + 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray106",  position+G4ThreeVector(1000. - 130., 1000. + 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray107",  position+G4ThreeVector(1000. - 130., 1000. - 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray108",  position+G4ThreeVector(1000. - 130., 1000. - 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray201",  position+G4ThreeVector(- 1000. - 390., 1000. + 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray202",  position+G4ThreeVector(- 1000. - 390., 1000. + 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray203",  position+G4ThreeVector(- 1000. - 390., 1000. - 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray204",  position+G4ThreeVector(- 1000. - 390., 1000. - 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray205",  position+G4ThreeVector(- 1000. - 130., 1000. + 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray206",  position+G4ThreeVector(- 1000. - 130., 1000. + 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray207",  position+G4ThreeVector(- 1000. - 130., 1000. - 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray208",  position+G4ThreeVector(- 1000. - 130., 1000. - 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray301",  position+G4ThreeVector(1000. - 390., - 1000. + 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray302",  position+G4ThreeVector(1000. - 390., - 1000. + 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray303",  position+G4ThreeVector(1000. - 390., - 1000. - 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray304",  position+G4ThreeVector(1000. - 390., - 1000. - 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray305",  position+G4ThreeVector(1000. - 130., - 1000. + 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray306",  position+G4ThreeVector(1000. - 130., - 1000. + 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray307",  position+G4ThreeVector(1000. - 130., - 1000. - 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray308",  position+G4ThreeVector(1000. - 130., - 1000. - 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray401",  position+G4ThreeVector(-1000. -390., - 1000. + 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray402",  position+G4ThreeVector(-1000. -390., - 1000. + 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray403",  position+G4ThreeVector(-1000. -390., - 1000. - 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray404",  position+G4ThreeVector(-1000. -390., - 1000. - 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray405",  position+G4ThreeVector(-1000. -130., - 1000. + 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray406",  position+G4ThreeVector(-1000. -130., - 1000. + 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray407",  position+G4ThreeVector(-1000. -130., - 1000. - 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray408",  position+G4ThreeVector(-1000. -130., - 1000. - 390., 0.), halfSize));

    rot.rotateZ(180. * deg);

    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray009", position+G4ThreeVector(390., 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray010", position+G4ThreeVector(390., 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray011", position+G4ThreeVector(390., -130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray012", position+G4ThreeVector(390., -390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray013", position+G4ThreeVector(130., 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray014", position+G4ThreeVector(130., 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray015", position+G4ThreeVector(130., -130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray016", position+G4ThreeVector(130., -390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray109", position+G4ThreeVector(1000. + 390., 1000. + 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray110", position+G4ThreeVector(1000. + 390., 1000. + 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray111", position+G4ThreeVector(1000. + 390., 1000. - 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray112", position+G4ThreeVector(1000. + 390., 1000. - 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray113", position+G4ThreeVector(1000. + 130., 1000. + 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray114", position+G4ThreeVector(1000. + 130., 1000. + 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray115", position+G4ThreeVector(1000. + 130., 1000. - 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray116", position+G4ThreeVector(1000. + 130., 1000. - 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray209", position+G4ThreeVector(-1000. + 390., 1000. + 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray210", position+G4ThreeVector(-1000. + 390., 1000. + 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray211", position+G4ThreeVector(-1000. + 390., 1000. - 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray212", position+G4ThreeVector(-1000. + 390., 1000. - 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray213", position+G4ThreeVector(-1000. + 130., 1000. + 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray214", position+G4ThreeVector(-1000. + 130., 1000. + 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray215", position+G4ThreeVector(-1000. + 130., 1000. - 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray216", position+G4ThreeVector(-1000. + 130., 1000. - 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray309", position+G4ThreeVector(1000. + 390., - 1000. + 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray310", position+G4ThreeVector(1000. + 390., - 1000. + 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray311", position+G4ThreeVector(1000. + 390., - 1000. - 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray312", position+G4ThreeVector(1000. + 390., - 1000. - 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray313", position+G4ThreeVector(1000. + 130., - 1000. + 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray314", position+G4ThreeVector(1000. + 130., - 1000. + 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray315", position+G4ThreeVector(1000. + 130., - 1000. - 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray316", position+G4ThreeVector(1000. + 130., - 1000. - 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray409", position+G4ThreeVector(-1000. + 390., - 1000. + 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray410", position+G4ThreeVector(-1000. + 390., - 1000. + 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray411", position+G4ThreeVector(-1000. + 390., - 1000. - 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray412", position+G4ThreeVector(-1000. + 390., - 1000. - 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray413", position+G4ThreeVector(-1000. + 130., - 1000. + 390., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray414", position+G4ThreeVector(-1000. + 130., - 1000. + 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray415", position+G4ThreeVector(-1000. + 130., - 1000. - 130., 0.), halfSize));
    m_trays.push_back(new D3DTray(rot,m_bunker_inside, "Tray416", position+G4ThreeVector(-1000. + 130., - 1000. - 390., 0.), halfSize));

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
