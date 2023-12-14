//
// Created by brachwal on 19.05.2020.
//

#include "BeamAnalysis.hh"
#include "G4Event.hh"
#include "G4Run.hh"
#include "G4AnalysisManager.hh"
#include "G4Step.hh"
#include "G4Gamma.hh"
#include "G4Proton.hh"
#include "G4Positron.hh"
#include "G4Electron.hh"
#include "G4Neutron.hh"
#include "Services.hh"



////////////////////////////////////////////////////////////////////////////////
///
BeamAnalysis::BeamAnalysis(){
  m_particleCodesMapping[G4Gamma::Definition()->GetPDGEncoding()] = 1;
  m_particleCodesMapping[G4Electron::Definition()->GetPDGEncoding()] = 2;
  m_particleCodesMapping[G4Positron::Definition()->GetPDGEncoding()] = 3;
  m_particleCodesMapping[G4Neutron::Definition()->GetPDGEncoding()] = 4;
  m_particleCodesMapping[G4Proton::Definition()->GetPDGEncoding()] = 5;

  const VecG4doubleWPtr& usrPlanesPositionWPtr = Service<ConfigSvc>()->GetValue<VecG4doubleSPtr>("BeamMonitoring", "ScoringZPositions");
  if(!usrPlanesPositionWPtr.expired()) {
    auto usrPlanesPositionSPtr = usrPlanesPositionWPtr.lock();
    m_usrPlaneZP1 = usrPlanesPositionSPtr->at(0);
    m_usrPlaneZP2 = usrPlanesPositionSPtr->at(1);
    m_usrPlaneZP3 = usrPlanesPositionSPtr->at(2);
  } else {
    G4cout << "[ERROR]:: BeamAnalysis:: \"ScoringZPositions\" config expired! " << G4endl;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
BeamAnalysis *BeamAnalysis::GetInstance() {
  static BeamAnalysis instance = BeamAnalysis();
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
///
void BeamAnalysis::BeginOfRun(const G4Run* runPtr, G4bool isMaster){

  auto analysisManager =  G4AnalysisManager::Instance();

  // Book particles data Ntuple
  //------------------------------------------
  auto ntupleId = analysisManager->CreateNtuple("BeamMonitoringEventTree","Beam monitoring data");
  m_ntupleId.Put(ntupleId);
  m_particleN.Put(0);
  analysisManager->CreateNtupleIColumn(ntupleId, "particleN");                             // column Id = 0
  analysisManager->CreateNtupleIColumn(ntupleId, "scoringPlaneId",m_monitoringPlaneId.Get());   // column Id = 1
  analysisManager->CreateNtupleDColumn(ntupleId, "particleE", m_particleE.Get());         // column Id = 2
  analysisManager->CreateNtupleDColumn(ntupleId, "particleEx",m_particleEx.Get());       // column Id = 3
  analysisManager->CreateNtupleDColumn(ntupleId, "particleEy",m_particleEy.Get());       // column Id = 4
  analysisManager->CreateNtupleDColumn(ntupleId, "particleX", m_particleX.Get());         // column Id = 5
  analysisManager->CreateNtupleDColumn(ntupleId, "particleY", m_particleY.Get());         // column Id = 6
  analysisManager->CreateNtupleDColumn(ntupleId, "particleZ", m_particleZ.Get());         // column Id = 7
  analysisManager->CreateNtupleIColumn(ntupleId, "particleId", m_particleId.Get());         // column Id = 8
  analysisManager->CreateNtupleIColumn(ntupleId, "trackId", m_trkId.Get());         // column Id = 9
  analysisManager->CreateNtupleDColumn(ntupleId, "particleP",m_primaryP.Get());       // column Id = 10
  analysisManager->CreateNtupleDColumn(ntupleId, "particlePx", m_primaryPx.Get());         // column Id = 11
  analysisManager->CreateNtupleDColumn(ntupleId, "particlePy", m_primaryPy.Get());         // column Id = 12
  analysisManager->CreateNtupleDColumn(ntupleId, "particlePz", m_primaryPz.Get());         // column Id = 13
  analysisManager->CreateNtupleDColumn(ntupleId, "particleTheta", m_primaryTheta.Get());         // column Id = 14
  analysisManager->FinishNtuple(ntupleId);

}

////////////////////////////////////////////////////////////////////////////////
/// This member is called at the end of every event from EventAction::EndOfEventAction
void BeamAnalysis::EndOfEventAction(const G4Event *evt){
  FillParticlesNTuple();
  ClearParticlesEventData();
}

////////////////////////////////////////////////////////////////////////////////
///
void BeamAnalysis::ClearParticlesEventData(){
  m_particleN.Put(0);
  m_monitoringPlaneId.Clear();
  m_particleE.Clear();
  m_particleEx.Clear();
  m_particleEy.Clear();
  m_particleX.Clear();
  m_particleY.Clear();
  m_particleZ.Clear();
  m_particleId.Clear();
  m_trkId.Clear();
  m_primaryP.Clear();
  m_primaryPx.Clear();
  m_primaryPy.Clear();
  m_primaryPz.Clear();
  m_primaryTheta.Clear();
}

////////////////////////////////////////////////////////////////////////////////
///
void BeamAnalysis::FillParticles(G4Step *step, G4int scoringPlaneId){
  if(step){
    auto pdgcode = step->GetTrack()->GetDefinition()->GetPDGEncoding();

    m_particleN.Put(m_particleN.Get() + 1);
    auto preStepPoint = step->GetPreStepPoint(); // particle
    auto position = preStepPoint->GetPosition();
    m_monitoringPlaneId.Push_back(scoringPlaneId);

    auto aTrack = step->GetTrack();
    G4double partMass = aTrack->GetDefinition()->GetPDGMass();
    G4double partMom = aTrack->GetMomentum().mag();

    auto dynamic = aTrack->GetDynamicParticle();
    auto px = dynamic->GetMomentum().x(); //preStepPoint->GetMomentum().x();
    auto py = dynamic->GetMomentum().y(); //preStepPoint->GetMomentum().y();
      
    m_particleX.Push_back(position.x());
    m_particleY.Push_back(position.y());
    m_particleZ.Push_back(position.z());

    m_particleE.Push_back(dynamic->Get4Momentum().e() / MeV  ); // TODO: is this the same as preStepPoint->GetTotalEnergy(); ????
    m_particleEx.Push_back(std::sqrt(partMass * partMass + px * px));
    m_particleEy.Push_back(std::sqrt(partMass * partMass + py * py));

    m_primaryP.Push_back(partMom);
    m_primaryPx.Push_back(px);
    m_primaryPy.Push_back(py);
    m_primaryPz.Push_back(preStepPoint->GetMomentum().z());

    m_primaryTheta.Push_back(preStepPoint->GetMomentum().theta());

    m_particleId.Push_back(m_particleCodesMapping[pdgcode]);

    m_trkId.Push_back(step->GetTrack()->GetTrackID());
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void BeamAnalysis::FillParticlesNTuple() {
  auto analysisManager = G4AnalysisManager::Instance();
  auto ntupleId = m_ntupleId.Get();
  analysisManager->FillNtupleIColumn( ntupleId,0, m_particleN.Get());
  analysisManager->AddNtupleRow(ntupleId);
}
