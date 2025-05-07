#include "ModularPhantom.hh"
#include "G4NistManager.hh"
#include "GeometryParser.hh"
#include "G4SystemOfUnits.hh"
#include "G4Box.hh"
#include "G4ProductionCuts.hh"
#include "G4TessellatedSolid.hh"
#include "G4TriangularFacet.hh"
#include "NTupleEventAnalisys.hh"
#include "G4UserLimits.hh"
#include "toml.hh"
#include "Services.hh"
#include "D3DCell.hh"

////////////////////////////////////////////////////////////////////////////////
///
ModularPhantom::ModularPhantom():VPatient("ModularPhantom"){}

////////////////////////////////////////////////////////////////////////////////
///
ModularPhantom::~ModularPhantom() {
  Destroy();
}

////////////////////////////////////////////////////////////////////////////////
///

ModularPhantom* ModularPhantom::GetInstance() {
  static ModularPhantom instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
///
void ModularPhantom::ParseTomlConfig(){
  std::cout << __FUNCTION__ << " called\n";
}

////////////////////////////////////////////////////////////////////////////////
///
void ModularPhantom::WriteInfo() {
  std::cout << __FUNCTION__ << " called\n";
}

////////////////////////////////////////////////////////////////////////////////
///
void ModularPhantom::Destroy() {
  std::cout << __FUNCTION__ << " called\n";
}


////////////////////////////////////////////////////////////////////////////////
///
void ModularPhantom::Construct(G4VPhysicalVolume *parentWorld) {
  std::cout << __FUNCTION__ << " called\n";  
  auto* nist = G4NistManager::Instance();
  auto* mat  = nist->FindOrBuildMaterial("G4_Al");
  GeometryParser parser;
  std::string filename = std::string(PROJECT_DATA_PATH) + "dose3d/geo/IBA_ImRT/d3df_scintillator_mapping_db_updated.xlsx";
  parser.load(filename,"scintillator_mapping_db");
  
  const auto& list = parser.data();
  

  for (auto const& obj : list) {
    if (!obj.sc_id.empty()) {
      std::cout << "Pass cause its Cell" << std::endl;
      continue;
      // TODO :: Create cell here? 
    }
    auto* solid = new G4TessellatedSolid(obj.component + "_Solid");
    for (auto const& t : obj.nodes) {
      if (t[0] < 0 || t[1] < 0 || t[2] < 0 ||
          t[0] >= (int)obj.vertices.size() ||
          t[1] >= (int)obj.vertices.size() ||
          t[2] >= (int)obj.vertices.size()) {
        G4cerr<<"Bad triangle idx for "<<obj.component<<G4endl;
        continue;
      }
      solid->AddFacet(new G4TriangularFacet(
        obj.vertices[t[0]],
        obj.vertices[t[1]],
        obj.vertices[t[2]],
        ABSOLUTE));
    }
    solid->SetSolidClosed(true);

    auto* componentLV = new G4LogicalVolume(
      solid, mat, obj.component + "_Logic");


    new G4PVPlacement(nullptr, obj.com, obj.component + "_PV", componentLV, parentWorld, false, 0);
  }

}


////////////////////////////////////////////////////////////////////////////////
///
G4bool ModularPhantom::Update() {
  std::cout << __FUNCTION__ << " called\n";
  return true;
}


////////////////////////////////////////////////////////////////////////////////
///
void ModularPhantom::ConstructSensitiveDetector(){
  std::cout << __FUNCTION__ << " called\n";
}



////////////////////////////////////////////////////////////////////////////////
///
void ModularPhantom::DefineSensitiveDetector(){
  std::cout << __FUNCTION__ << " called\n";
  // I Dont think we need that - SD wil be created by Dose3D detector class.

}






////////////////////////////////////////////////////////////////////////////////
/// 
