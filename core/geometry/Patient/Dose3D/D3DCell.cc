#include "D3DCell.hh"
#include "G4SystemOfUnits.hh"
#include "GeoSvc.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4SubtractionSolid.hh"
#include "G4ProductionCuts.hh"
#include "D3DCellSD.hh"
#include "ConfigSvc.hh"
#include "G4UserLimits.hh"
#include "NTupleEventAnalisys.hh"
#include "RunAnalysis.hh"
#include "colors.hh"
#include <vector>
#include "Services.hh"

namespace {
    G4Mutex CellMutex = G4MUTEX_INITIALIZER;
}

G4double D3DCell::SIZE = 10.0 * mm;
// G4double D3DCell::SIZE = 5.4 * mm;
// G4double D3DCell::SIZE = 2 * cm;


G4bool D3DCell::m_set_cell_scorer = true;
G4bool D3DCell::m_set_cell_voxelised_scorer = true;
////////////////////////////////////////////////////////////////////////////////
/// static
void D3DCell::CellScorer(G4bool val) { 
  m_set_cell_scorer = val; 
  // if(!val){ // by default it's set to true
  //   Service<RunSvc>()->GetScoringTypes().erase(Scoring::Type::Cell);
  // }
}
////////////////////////////////////////////////////////////////////////////////
/// static
void D3DCell::CellVoxelisedScorer(G4bool val) { 
  m_set_cell_voxelised_scorer = val; 
  if(!val){ // by default it's set to true
    Service<RunSvc>()->GetScoringTypes().erase(Scoring::Type::Voxel);
  }
}

////////////////////////////////////////////////////////////////////////////////
///
D3DCell::D3DCell(const G4String& label, const G4ThreeVector& centre, G4String cellMediumName)
: VPatient(label),m_cell_medium(cellMediumName){
    m_centre = centre;
}

////////////////////////////////////////////////////////////////////////////////
///
D3DCell::~D3DCell() {
  Destroy();
  if(m_step_limit)
    delete m_step_limit;
}

void D3DCell::SetIDs(G4int x, G4int y, G4int z){
  m_id_x = x;
  m_id_y = y;
  m_id_z = z;
}


////////////////////////////////////////////////////////////////////////////////
///
void D3DCell::WriteInfo() {
  LOGSVC_INFO("The Dose3D cell {} info: Implement me.", GetName());
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DCell::Destroy() {
  LOGSVC_INFO("Destroing the D3DCell volume.");
  auto phantomVolume = GetPhysicalVolume();
  if (phantomVolume) {
    delete phantomVolume;
    SetPhysicalVolume(nullptr);
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DCell::SetNVoxels(char axis, int nv){
  switch(std::tolower(axis)) {
    case 'x':
      m_cell_voxelization_x = nv;
      break;
    case 'y':
      m_cell_voxelization_y = nv;
      break;
    case 'z':
      m_cell_voxelization_z = nv;
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DCell::Construct(G4VPhysicalVolume *parentWorld) {
  // std::cout << "[INFO]:: D3DCell construction... " << std::endl;
  auto label = GetName();
  auto size = G4ThreeVector(D3DCell::SIZE,D3DCell::SIZE,D3DCell::SIZE);
  // std::cout << "Parent world was set. " << std::endl;
  m_parentPV = parentWorld;

  // auto dose3dPaintedCellBox = new G4Box(label+"PreBox",0.15*mm + (size.getX()/ 2.), 0.15*mm + (size.getX()/ 2.), 0.15*mm + (size.getX()/ 2.));
  // auto myMedium = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", "TiO2");
  // auto dose3dPaintLV = new G4LogicalVolume(dose3dPaintedCellBox, myMedium.get(), label+"PaintedLV");
  // SetPhysicalVolume(new G4PVPlacement(nullptr, m_centre, label+"PaintedPV", dose3dPaintLV, parentWorld, false, 0));
  // auto pv = GetPhysicalVolume();
  // std::cout << "Setting medium..." << std::endl;
  auto Medium = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", m_cell_medium);
  // create a cell box filled with PMMA, with given side dimensions
  auto dose3dCellBox = new G4Box(label+"Box", size.getX() / 2., size.getY() / 2., size.getZ() / 2.);
  auto dose3dCellLV = new G4LogicalVolume(dose3dCellBox, Medium.get(), label+"LV");
  // the placement of phantom center in the gantry (global) coordinate system that is managed by PatientGeometry class
  // here we locate the phantom box in the center of envelope box created in PatientGeometry:
  LOGSVC_DEBUG("centre {} {} {}",m_centre.getX(),m_centre.getY(),m_centre.getZ()," for cell construction... "); 
  // For Painted
  // SetPhysicalVolume(new G4PVPlacement(nullptr, G4ThreeVector(), label+"PV", dose3dCellLV, pv, false, 0));
  SetPhysicalVolume(new G4PVPlacement(nullptr, m_centre, label+"PV", dose3dCellLV, m_parentPV, false, 0));

  SetGlobalCentre( m_centre + m_parentPV->GetTranslation()); 
  LOGSVC_DEBUG("Construct() >> current cell translation {}", m_global_centre);
  // std::cout << "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" << std::endl;
    // Region for cuts
  //G4cout << "[DEBUG]:: D3DCell:: creating cuts " << label <<"_G4RegionCuts" << G4endl;

  // std::cout << "[DEBUG]:: D3DCell:: creating cuts " << label <<"_G4RegionCuts" << G4endl;
  auto regVol = new G4Region(label+"_G4RegionCuts");
  auto cuts = new G4ProductionCuts;
  cuts->SetProductionCut(0.1 * mm);
  regVol->SetProductionCuts(cuts);
  dose3dCellLV->SetRegion(regVol);
  regVol->AddRootLogicalVolume(dose3dCellLV);

  // G4UserLimits* userLimits = new G4UserLimits();
  // userLimits->SetMaxAllowedStep(1.0 * um);
  // dose3dCellLV->SetUserLimits(userLimits);

  }

////////////////////////////////////////////////////////////////////////////////
///
G4bool D3DCell::Update() {
  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
bool D3DCell::IsRunCollectionScoringVolumeVoxelised(const G4String& run_collection) const {
  if (GetSD()->GetRunCollectionReferenceScoringVolume(run_collection,true))
    return true;
  return false;
}



////////////////////////////////////////////////////////////////////////////////
///
void D3DCell::DefineSensitiveDetector(){
  G4AutoLock lock(&CellMutex);
  if(m_patientSD.Get()==0){
    auto pv = GetPhysicalVolume();
    auto centre = m_global_centre; // wrap this to VPatient::GetGlobalTranslation
    
    LOGSVC_DEBUG("Construct SD >> current centre {} {} {}", centre.x(),centre.y(),centre.z());

    auto envBox = dynamic_cast<G4Box*>(pv->GetLogicalVolume()->GetSolid());
    auto label = GetName();
    m_patientSD.Put(new D3DCellSD(label+"_SD",centre,m_id_x,m_id_y,m_id_z));
    auto patientSD = m_patientSD.Get();
    patientSD->SetTracksAnalysis(m_tracks_analysis);

    G4String hcName;
    // Scoring in the centre of the cell
    // ________________________________________________________________________
    hcName = label+"_HC";
    // G4cout << "Current cell hcName: " << hcName << G4endl;
    G4int nvx(1), nvy(1), nvz(1); // Scoring resolution: nVoxelsX, nVoxelsY, nVoxelsZ
    if(D3DCell::m_set_cell_voxelised_scorer){
      nvx = m_cell_voxelization_x;
      nvy = m_cell_voxelization_y;
      nvz = m_cell_voxelization_z;
    }
    std::string name = GetName();
    // TEMPORARY METHOD TO GET RUN COLLECTION NAME:
    // TODO: extract this from Detector::name scope
    std::string runCollName = name.substr(0, name.find('_', 0));
    //G4cout << "[DEBUG]:: D3DCell::DefineSensitiveDetector name " << name << " runCollName " << runCollName << G4endl;
    patientSD->AddScoringVolume(runCollName,hcName,*envBox,nvx,nvy,nvz);


    // ________________________________________________________________________
    VPatient::SetSensitiveDetector(label+"LV", patientSD); // this call G4SDManager::GetSDMpointer()->AddNewDetector(aSD);

  }
}


