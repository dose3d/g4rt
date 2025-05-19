#include "G4NistManager.hh"
#include "GeometryParser.hh"
#include "G4SystemOfUnits.hh"
#include "G4Box.hh"
#include "G4ProductionCuts.hh"
#include "G4TessellatedSolid.hh"
#include "G4TriangularFacet.hh"
#include "NTupleEventAnalisys.hh"
#include "G4UserLimits.hh"
#include "GeometryBuilder.hh"
#include "toml.hh"
#include "Services.hh"
#include "D3DCell.hh"
#include "D3DDetector.hh"

////////////////////////////////////////////////////////////////////////////////
///
GeometryBuilder::GeometryBuilder():TomlConfigModule("GeometryBuilder"){}

////////////////////////////////////////////////////////////////////////////////
///
GeometryBuilder* GeometryBuilder::GetInstance() {
  static GeometryBuilder instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
///
GeometryBuilder::~GeometryBuilder() {

}

////////////////////////////////////////////////////////////////////////////////
///
void GeometryBuilder::Build(G4VPhysicalVolume *parentWorld) {
  auto* nist = G4NistManager::Instance();
  auto* mat  = nist->FindOrBuildMaterial(m_phantomMedium);
  auto path = std::string(PROJECT_DATA_PATH) + "/" + Service<ConfigSvc>()->GetValue<std::string>("PatientGeometry", "PatientDBPath");
  GeometryParser parser;
  std::string db_filename = path + "/d3df_scintillator_mapping_db.xlsx";
  std::string csv_filename = path + "/D3DF_bodies.csv";
  std::string sheet = "scintillator_mapping_db";
  
  parser.load(db_filename, csv_filename, sheet);
  const auto& list = GeometryDBReader::GetData();
  const auto& list = parser.data();
  
  
  for (auto const& obj : list) {
    if (obj.sc_id != "nan" && obj.sc_id != "" && !obj.sc_id.empty()) {
      D3DDetector::m_db_cells_positioning.push_back({ obj.sc_id, obj.com }); // This should be in GeometryDBReader -> And also parser should be ranamed as GeometryDBReader
      continue;
    }
    auto* tessSolid = new G4TessellatedSolid(obj.component + "_Solid");

    for (size_t i = 0; i < obj.nodes.size(); ++i) {
      const auto& t = obj.nodes[i];
      G4ThreeVector p1 = obj.vertices[t[0]];
      G4ThreeVector p2 = obj.vertices[t[1]];
      G4ThreeVector p3 = obj.vertices[t[2]];
      
      G4ThreeVector geomN = (p2 - p1).cross(p3 - p1).unit();
      
      
      G4ThreeVector desiredN = obj.normals[i];
      tessSolid->AddFacet(new G4TriangularFacet(p1, p2, p3, ABSOLUTE));

      }
      
    tessSolid->SetSolidClosed(true);
    auto* componentLV = new G4LogicalVolume(
      tessSolid, mat, obj.component + "_Logic");
      
      
      new G4PVPlacement(nullptr, G4ThreeVector(), obj.component + "_PV", componentLV, parentWorld, false, 0);
    }

  }

  void GeometryBuilder::ParseTomlConfig(){
    std::cout << __FUNCTION__ << " called\n";
  }
  
  
  ////////////////////////////////////////////////////////////////////////////////
  /// 
