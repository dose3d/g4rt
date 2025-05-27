#include "G4NistManager.hh"
#include "GeometryDBReader.hh"
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
GeometryBuilder::GeometryBuilder():TomlConfigModule("GeometryBuilder"){

  ParseTomlConfig();
}


////////////////////////////////////////////////////////////////////////////////
///   
void GeometryBuilder::ParseTomlConfig(){
  SetTomlConfigFile();
  auto configFile = GetTomlConfigFile();
  auto configPrefix = GetTomlConfigPrefix();

  // LOGSVC_INFO("Importing configuration from:\n{}",configFile); // Not Logable RN
  // if (!svc::checkIfFileExist(configFile)) {
  //   LOGSVC_CRITICAL("File {} not fount.", configFile);
  //   G4Exception("GeometryBuilder", "ParseTomlConfig", FatalErrorInArgument, "");
  // }

  std::cout << "Importing configuration from:\n" << configFile << "\n";
  auto config = toml::parse_file(configFile);
  std::cout << config << "\n";


  m_centrePositionX = config["Phantom"]["Position"][0].value_or(0.0);
  m_centrePositionY = config["Phantom"]["Position"][1].value_or(0.0);
  m_centrePositionZ = config["Phantom"]["Position"][2].value_or(0.0);

  m_phantomRotationX = config["Phantom"]["Rotation"][0].value_or(0.0);
  m_phantomRotationY = config["Phantom"]["Rotation"][1].value_or(0.0);
  m_phantomRotationZ = config["Phantom"]["Rotation"][2].value_or(0.0);

  m_exclusde_object_list = config["ExcludeObjList"].as_array();

  if (m_exclusde_object_list) {
    for (const auto& elem : *m_exclusde_object_list) {
      if (elem.is_string()) {
        std::string value = elem.value_or("");
        std::cout << "Wykluczony obiekt: " << value << "\n";
      }
    }
  }
}

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
  // auto* nist = G4NistManager::Instance();
  // auto* mat  = nist->FindOrBuildMaterial(m_phantomMedium); // Default temp material
  
  
  const auto& list =  GeometryDBReader::Instance().GetData();
  

  if (m_exclusde_object_list) {
    for (const auto& elem : *m_exclusde_object_list) {
      if (elem.is_string()) {
        std::string value = elem.value_or("");
        std::cout << "Wykluczony obiekt: " << value << "\n";
      }
    }
  }

  
  for (auto const& obj : list) {
    if (obj.sc_id != "nan" && obj.sc_id != "" && !obj.sc_id.empty()) {
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
      
    auto mat = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", std::string(obj.mat));

    tessSolid->SetSolidClosed(true);
    auto* componentLV = new G4LogicalVolume(tessSolid, mat.get(), obj.component + "_Logic");
    G4ThreeVector tranlation;
      if (ConfigSvc::GetInstance()->GetValue<std::string>("PatientGeometry", "EnviromentPatientEnvelop") == "IbaImRT_3mf"){
        tranlation = G4ThreeVector(-95.0,-90.0,-90.0);
      }
      else if (ConfigSvc::GetInstance()->GetValue<std::string>("PatientGeometry", "EnviromentPatientEnvelop") == "ModularWaterPhantom_3mf"){
        tranlation = G4ThreeVector(-271.0,-275.0,-225.0);
      }
      new G4PVPlacement(nullptr, tranlation, obj.component + "_PV", componentLV, parentWorld, false, 0);
    }

  }
    
  