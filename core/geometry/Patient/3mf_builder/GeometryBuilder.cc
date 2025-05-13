#include "GeometryBuilder.hh"
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
GeometryBuilder::GeometryBuilder():IPhysicalVolume("GeometryBuilder"){}

////////////////////////////////////////////////////////////////////////////////
///
GeometryBuilder::~GeometryBuilder() {
  Destroy();
}

////////////////////////////////////////////////////////////////////////////////
///

GeometryBuilder* GeometryBuilder::GetInstance() {
  static GeometryBuilder instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
///
void GeometryBuilder::WriteInfo() {
  std::cout << __FUNCTION__ << " called\n";
}

////////////////////////////////////////////////////////////////////////////////
///
void GeometryBuilder::Destroy() {
  std::cout << __FUNCTION__ << " called\n";
}


////////////////////////////////////////////////////////////////////////////////
///
void GeometryBuilder::Construct(G4VPhysicalVolume *parentWorld) {
  std::cout << __FUNCTION__ << " called\n";  
  auto* nist = G4NistManager::Instance();
  auto* mat  = nist->FindOrBuildMaterial(m_phantomMedium);
  GeometryParser parser;
  std::string db_filename = std::string(PROJECT_DATA_PATH) + "/dose3d/geo/IBA_ImRT/d3df_scintillator_mapping_db_updated.xlsx";
  // std::string db_filename = "/home/jackie/work/d3df_data-analysis/3d_mesh_DB/d3df_scintillator_mapping_db.xlsx";
  std::string csv_filename = std::string(PROJECT_DATA_PATH) + "/dose3d/geo/IBA_ImRT/D3DF_bodiesHigh.csv";
  // std::string csv_filename ="/home/jackie/work/d3df_data-analysis/3d_mesh_DB/D3DF_bodies.csv";
  std::string sheet = "scintillator_mapping_db";

  parser.load(db_filename, csv_filename, sheet);
  
  const auto& list = parser.data();
  

  for (auto const& obj : list) {
    if (obj.sc_id != "nan" && obj.sc_id != "" && !obj.sc_id.empty()) {
      std::cout << "Pass cause its Cell" << std::endl;
      std::cout << "sc_id: " << obj.sc_id << std::endl;
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
      // if (geomN.dot(desiredN) < 0) {
      //   auto facet = new G4TriangularFacet(p1, p3, p2, ABSOLUTE);
      //   // tessSolid->AddFacet(facet->GetFlippedFacet());
      //   tessSolid->AddFacet(facet);
      //   delete facet;
      // }
      // else {
        tessSolid->AddFacet(new G4TriangularFacet(p1, p2, p3, ABSOLUTE));
      // }
    }
    // tessSolid->DumpInfo(); 
    
    tessSolid->SetSolidClosed(true);
    auto* componentLV = new G4LogicalVolume(
      tessSolid, mat, obj.component + "_Logic");


    new G4PVPlacement(nullptr, obj.com, obj.component + "_PV", componentLV, parentWorld, false, 0);
  }

}


////////////////////////////////////////////////////////////////////////////////
///
G4bool GeometryBuilder::Update() {
  std::cout << __FUNCTION__ << " called\n";
  return true;
}



////////////////////////////////////////////////////////////////////////////////
/// 
