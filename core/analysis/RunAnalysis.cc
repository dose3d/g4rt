//
// Created by brachwal on 28.04.2020.
//

#include "RunAnalysis.hh"
#include "G4Event.hh"
#include "G4Run.hh"
#include "G4SDManager.hh"
#include "VoxelHit.hh"
#include "G4AnalysisManager.hh"
#include "CsvRunAnalysis.hh"
#include "Services.hh"

CsvRunAnalysis* RunAnalysis::m_csv_run_analysis = nullptr;

RunAnalysis::RunAnalysis(){
  if(!m_is_initialized){
    if(!m_csv_run_analysis) // TODO: && RUN_CSV_ANALYSIS
        m_csv_run_analysis = CsvRunAnalysis::GetInstance();
    // TODO: RUN_NTUPLE_ANALYSIS
    // TODO: RUN_HDF5_ANALYSIS
  }
  m_is_initialized = true;
}


////////////////////////////////////////////////////////////////////////////////
///
RunAnalysis *RunAnalysis::GetInstance() {
  static RunAnalysis instance = RunAnalysis();
  return &instance;
}


////////////////////////////////////////////////////////////////////////////////
///
void RunAnalysis::BeginOfRun(const G4Run* runPtr, G4bool isMaster){
  LOGSVC_INFO("RUN ANALYSIS :: BEGIN OF RUN: {}",runPtr->GetRunID());
  if(m_csv_run_analysis) m_csv_run_analysis->BeginOfRun();
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

////////////////////////////////////////////////////////////////////////////////
///
void RunAnalysis::AddRunCollection(const G4String& collection_name, const G4String& hc_name){
  RunAnalysis::GetInstance(); // initialize (if needed) the whole factory
  if(m_csv_run_analysis) m_csv_run_analysis->AddRunCollection(collection_name,hc_name);
}
