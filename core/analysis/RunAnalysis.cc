//
// Created by brachwal on 28.04.2020.
//

#include "RunAnalysis.hh"
#include "G4Event.hh"
#include "G4Run.hh"
#include "G4SDManager.hh"
#include "VoxelHit.hh"
#include "G4AnalysisManager.hh"

////////////////////////////////////////////////////////////////////////////////
///
RunAnalysis *RunAnalysis::GetInstance() {
  static RunAnalysis instance = RunAnalysis();
  return &instance;
}


////////////////////////////////////////////////////////////////////////////////
///
void RunAnalysis::BeginOfRun(const G4Run* runPtr, G4bool isMaster){
  // Extract from VPatient geometry information, and define NTuples structure
  //
  //auto analysisManager =  G4AnalysisManager::Instance();

  // Book Voxel data Ntuple
  //------------------------------------------
  // TODO: define any TTree ?

  // Book histograms
  //------------------------------------------
  //for(int i =0; i<200; ++i) m_voxelTotalDoseZProfile.Push_back(0.);
  //auto histId = analysisManager->CreateH1("PDD","PDD",200,0,40);
  //m_pddHistId.Put(histId);

}

////////////////////////////////////////////////////////////////////////////////
///
void RunAnalysis::FillEvent(G4double totalEvEnergy) {}

////////////////////////////////////////////////////////////////////////////////
/// This member is called at the end of every event from EventAction::EndOfEventAction
void RunAnalysis::EndOfEventAction(const G4Event *evt){}

////////////////////////////////////////////////////////////////////////////////
///
void RunAnalysis::ClearEventData(){}

////////////////////////////////////////////////////////////////////////////////
///
void RunAnalysis::EndOfRun(){
//  auto analysisManager = G4AnalysisManager::Instance();
//  auto hist = analysisManager->GetH1(m_pddHistId.Get());
//  for(int i =0; i<200; ++i)
//    hist->set_bin_content(i,m_voxelTotalDoseZProfile[i]);
}