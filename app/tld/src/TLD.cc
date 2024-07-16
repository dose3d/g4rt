#include "TLD.hh"
#include "G4SystemOfUnits.hh"
#include "GeoSvc.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4SubtractionSolid.hh"
#include "G4ProductionCuts.hh"
#include "TLDSD.hh"
#include "ConfigSvc.hh"
#include "G4UserLimits.hh"
#include "NTupleEventAnalisys.hh"
#include "RunAnalysis.hh"
#include <vector>
#include "Services.hh"

namespace {
    G4Mutex TldMutex = G4MUTEX_INITIALIZER;
}

G4double TLD::SIZE = 4.5 * mm;

G4bool TLD::m_set_tld_scorer = true;
G4bool TLD::m_set_tld_voxelised_scorer = true;
////////////////////////////////////////////////////////////////////////////////
/// static
void TLD::CellScorer(G4bool val) { 
  m_set_tld_scorer = val; 
  // if(!val){ // by default it's set to true
  //   Service<RunSvc>()->GetScoringTypes().erase(Scoring::Type::Cell);
  // }
}
////////////////////////////////////////////////////////////////////////////////
/// static
void TLD::CellVoxelisedScorer(G4bool val) { 
  m_set_tld_voxelised_scorer = val; 
  if(!val){ // by default it's set to true
    Service<RunSvc>()->GetScoringTypes().erase(Scoring::Type::Voxel);
  }
}

////////////////////////////////////////////////////////////////////////////////
///
TLD::TLD(const G4String& label, const G4ThreeVector& centre, G4String tldMediumName)
: VPatient(label),m_tld_medium(tldMediumName){
    m_centre = centre;
}

////////////////////////////////////////////////////////////////////////////////
///
TLD::~TLD() {
  Destroy();
  if(m_step_limit)
    delete m_step_limit;
}

void TLD::SetIDs(G4int x, G4int y, G4int z){
  m_id_x = x;
  m_id_y = y;
  m_id_z = z;
}


////////////////////////////////////////////////////////////////////////////////
///
void TLD::WriteInfo() {
  LOGSVC_INFO("The Dose3D cell {} info: Implement me.", GetName());
}

////////////////////////////////////////////////////////////////////////////////
///
void TLD::Destroy() {
  LOGSVC_INFO("Destroing the TLD volume.");
  auto phantomVolume = GetPhysicalVolume();
  if (phantomVolume) {
    delete phantomVolume;
    SetPhysicalVolume(nullptr);
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void TLD::SetNVoxels(char axis, int nv){
  switch(std::tolower(axis)) {
    case 'x':
      m_tld_voxelization_x = nv;
      break;
    case 'y':
      m_tld_voxelization_y = nv;
      break;
    case 'z':
      m_tld_voxelization_z = nv;
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void TLD::Construct(G4VPhysicalVolume *parentWorld) {
  // std::cout << "[INFO]:: TLD construction... " << std::endl;
  auto label = GetName();
  auto size = G4ThreeVector(TLD::SIZE,TLD::SIZE,TLD::SIZE);
  // std::cout << "Parent world was set. " << std::endl;
  m_parentPV = parentWorld;

  auto Medium = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", m_tld_medium);
  // create a cell box filled with PMMA, with given side dimensions
  auto tldBox = new G4Box(label+"Box", size.getX() / 2., size.getY() / 2., size.getZ() / 2.);
  auto tldLV = new G4LogicalVolume(tldBox, Medium.get(), label+"LV");
  // the placement of phantom center in the gantry (global) coordinate system that is managed by PatientGeometry class
  // here we locate the phantom box in the center of envelope box created in PatientGeometry:
  LOGSVC_DEBUG("centre {} {} {}",m_centre.getX(),m_centre.getY(),m_centre.getZ()," for cell construction... "); 
  SetPhysicalVolume(new G4PVPlacement(nullptr, m_centre, label+"PV", tldLV, m_parentPV, false, 0));

  SetGlobalCentre( m_centre + m_parentPV->GetTranslation()); 
  LOGSVC_DEBUG("Construct() >> current TLD translation {}", m_global_centre);

  // std::cout << "[DEBUG]:: TLD:: creating cuts " << label <<"_G4RegionCuts" << G4endl;
  auto regVol = new G4Region(label+"_G4RegionCuts");
  auto cuts = new G4ProductionCuts;
  cuts->SetProductionCut(0.1 * mm);
  regVol->SetProductionCuts(cuts);
  tldLV->SetRegion(regVol);
  regVol->AddRootLogicalVolume(tldLV);

}

////////////////////////////////////////////////////////////////////////////////
///
G4bool TLD::Update() {
  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
bool TLD::IsRunCollectionScoringVolumeVoxelised(const G4String& run_collection) const {
  if (GetSD()->GetRunCollectionReferenceScoringVolume(run_collection,true))
    return true;
  return false;
}

////////////////////////////////////////////////////////////////////////////////
///
void TLD::DefineSensitiveDetector(){
  G4AutoLock lock(&TldMutex);
  if(m_patientSD.Get()==0){
    auto pv = GetPhysicalVolume();
    auto centre = m_global_centre; // wrap this to VPatient::GetGlobalTranslation
    
    LOGSVC_DEBUG("Construct SD >> current centre {} {} {}", centre.x(),centre.y(),centre.z());

    auto envBox = dynamic_cast<G4Box*>(pv->GetLogicalVolume()->GetSolid());
    auto label = GetName();
    m_patientSD.Put(new TLDSD(label+"_SD",centre,m_id_x,m_id_y,m_id_z));
    auto patientSD = m_patientSD.Get();
    patientSD->SetTracksAnalysis(m_tracks_analysis);

    G4String hcName;
    // Scoring in the centre of the cell
    // ________________________________________________________________________
    hcName = label+"_HC";
    // G4cout << "Current cell hcName: " << hcName << G4endl;
    G4int nvx(1), nvy(1), nvz(1); // Scoring resolution: nVoxelsX, nVoxelsY, nVoxelsZ
    if(TLD::m_set_tld_voxelised_scorer){
      nvx = m_tld_voxelization_x;
      nvy = m_tld_voxelization_y;
      nvz = m_tld_voxelization_z;
    }
    std::string name = GetName();
    // TEMPORARY METHOD TO GET RUN COLLECTION NAME:
    // TODO: extract this from Detector::name scope
    std::string runCollName = name.substr(0, name.find('_', 0));
    //G4cout << "[DEBUG]:: TLD::DefineSensitiveDetector name " << name << " runCollName " << runCollName << G4endl;
    patientSD->AddScoringVolume(runCollName,hcName,*envBox,nvx,nvy,nvz);


    // ________________________________________________________________________
    VPatient::SetSensitiveDetector(label+"LV", patientSD); // this call G4SDManager::GetSDMpointer()->AddNewDetector(aSD);

  }
}


