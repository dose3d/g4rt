#include "ModularPhantom.hh"
#include "G4NistManager.hh"
#include "GeometryParser.hh"
#include "G4SystemOfUnits.hh"
#include "G4Box.hh"
#include "G4ProductionCuts.hh"
#include "ModularPhantomSD.hh"
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
  std::string filename = PROJECT_DATA_PATH + "dose3d/geo/IBA_ImRT/d3df_scintillator_mapping_db_updated.xlsx";
  parser.load(filename,"scintillator_mapping_db");

  const auto& list = parser.data();

  auto* sdman = G4SDManager::GetSDMpointer();
  for (auto const& obj : list) {
    if (!obj.sc_id.empty()) {
      auto* sd = new ScintillatorSD(obj.sc_id);
      sdman->AddNewDetector(sd);
    }
  }

  for (auto const& obj : list) {
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

    if (!obj.sc_id.empty()) {
      auto* sd = sdman->FindSensitiveDetector(obj.sc_id);
      componentLV->SetSensitiveDetector(sd);
    }

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
void ModularPhantom::ConstructFullVolumeScoring(const G4String& name){
  std::cout << __FUNCTION__ << " called\n";
}

////////////////////////////////////////////////////////////////////////////////
///
void ModularPhantom::DefineSensitiveDetector(){
  std::cout << __FUNCTION__ << " called\n";
}






////////////////////////////////////////////////////////////////////////////////
///
std::map<std::size_t, VoxelHit> ModularPhantom::GetScoringHashedMap(const G4String& scoring_name,Scoring::Type type) const{

  std::map<std::size_t, VoxelHit> hashed_map_scoring;

  auto Medium = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", m_phantomMedium);

  std::string hashedPhantomString = "000";

  if( type==Scoring::Type::Voxel ){
    auto sv = GetSD()->GetRunCollectionReferenceScoringVolume(scoring_name,true);
    if(sv==nullptr) return hashed_map_scoring; // no voxelisation for this volume, return empty map
    auto centre = G4ThreeVector(m_centrePositionX*mm , m_centrePositionY*mm  , m_centrePositionZ*mm);
      
    for(int ix=0; ix < sv->m_nVoxelsX; ix++ ){
      for(int iy=0; iy < sv->m_nVoxelsY; iy++ ){
        for(int iz=0; iz < sv->m_nVoxelsZ; iz++ ){
          auto voxelHash = svc::getHashedStrFromIndexes({0,0,0,ix,iy,iz});
          hashed_map_scoring[voxelHash] = VoxelHit();
          auto voxelCentre = sv->GetVoxelCentre(ix,iy,iz);
          // std::cout << voxelCentre.getZ() << std::endl;
          hashed_map_scoring[voxelHash].SetCentre(voxelCentre);
          hashed_map_scoring[voxelHash].SetGlobalCentre(centre);
          hashed_map_scoring[voxelHash].SetId(ix,iy,iz);
          hashed_map_scoring[voxelHash].SetGlobalId(0,0,0);
          hashed_map_scoring[voxelHash].SetVolume( sv->GetVoxelVolume() );
          hashed_map_scoring[voxelHash].SetMass(Medium->GetDensity() * sv->GetVoxelVolume());
        } // z
      }   // y
    }     // x
  } 
  else if (type==Scoring::Type::Cell){
    // auto phantomHash = std::hash<std::string>{}(hashedPhantomString);
    auto phantomHash = svc::getHashedStrFromIndexes({0,0,0});
    hashed_map_scoring[phantomHash] = VoxelHit();
    auto centre = G4ThreeVector(m_centrePositionX*mm , m_centrePositionY*mm  , m_centrePositionZ*mm);
    hashed_map_scoring[phantomHash].SetCentre(centre);
    hashed_map_scoring[phantomHash].SetGlobalCentre(centre);
    hashed_map_scoring[phantomHash].SetId(0,0,0);
    hashed_map_scoring[phantomHash].SetGlobalId(0,0,0); // Id == GlobalId
    auto volume = m_sizeX*m_sizeY*m_sizeZ;
    hashed_map_scoring[phantomHash].SetVolume( volume );
    hashed_map_scoring[phantomHash].SetMass(Medium->GetDensity()*volume);
  }

  return hashed_map_scoring;
}


