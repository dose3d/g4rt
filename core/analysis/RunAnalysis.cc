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
#include "NTupleRunAnalysis.hh"
#include "Services.hh"
#ifdef G4MULTITHREADED
  #include "G4MTRunManager.hh"
#endif
std::map<G4String,std::vector<G4String>> RunAnalysis::m_run_collection = std::map<G4String,std::vector<G4String>>();

RunAnalysis::RunAnalysis(){
  if(!m_is_initialized){
    m_scoring_types = Service<RunSvc>()->GetScoringTypes();
    if(!m_csv_run_analysis) // TODO: && RUN_CSV_ANALYSIS
        m_csv_run_analysis = CsvRunAnalysis::GetInstance();
    if(!m_ntuple_run_analysis) // TODO: && RUN_NTUPLE_ANALYSIS
        m_ntuple_run_analysis = NTupleRunAnalysis::GetInstance();
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
    m_current_cp = Service<RunSvc>()->CurrentControlPoint();
    std::string worker = G4Threading::IsWorkerThread() ? "*WORKER*" : " *MASTER* ";
    LOGSVC_DEBUG("RunAnalysis:: begin of run at {} thread.",worker);
    // Note: Everything is being care by ControlPointRun::InitializeScoringCollection
}

////////////////////////////////////////////////////////////////////////////////
/// This member is called at the end of every event from EventAction::EndOfEventAction
void RunAnalysis::EndOfEventAction(const G4Event *evt){
    auto hCofThisEvent = evt->GetHCofThisEvent();
    for(const auto& run_collection: m_run_collection){
        // LOGSVC_DEBUG("RunAnalysis::EndOfEvent: RunColllection {}",run_collection.first);
        for(const auto& hc: run_collection.second){
            // Related SensitiveDetector collection ID (Geant4 architecture)
            // collID==-1 the collection is not found
            // collID==-2 the collection name is ambiguous
            auto collection_id = G4SDManager::GetSDMpointer()->GetCollectionID(hc);
            if(collection_id<0){
                LOGSVC_INFO("RunAnalysis::EndOfEvent: HC: {} / G4SDManager Err: {}", hc, collection_id);
            }
            else {
                auto thisHitsCollPtr = hCofThisEvent->GetHC(collection_id);
                if(thisHitsCollPtr) // The particular collection is stored at the current event.
                    FillEventCollection(run_collection.first,evt,dynamic_cast<VoxelHitsCollection*>(thisHitsCollPtr));
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
///
void RunAnalysis::AddRunHCollection(const G4String& collection_name, const G4String& hc_name){
    if(m_run_collection.find( collection_name ) == m_run_collection.end())
        m_run_collection[collection_name] = std::vector<G4String>();
    m_run_collection.at(collection_name).emplace_back(hc_name);
}

////////////////////////////////////////////////////////////////////////////////
///
void RunAnalysis::FillEventCollection(const G4String& collection_name, const G4Event *evt, VoxelHitsCollection* hitsColl){
    int nHits = hitsColl->entries();
    auto hc_name = hitsColl->GetName();
    if(nHits==0){
        return; // no hits in this event
    }
    auto& scoring_collection = m_current_cp->GetRun()->GetScoringCollection(collection_name);
    for (int i=0;i<nHits;i++){ // a.k.a. voxel loop
        auto hit = dynamic_cast<VoxelHit*>(hitsColl->GetHit(i));
        for(const auto& scoring_type: m_scoring_types){
            auto& current_scoring_collection = scoring_collection[scoring_type];
            switch (scoring_type){
                case Scoring::Type::Cell:
                    FillCellEventCollection(current_scoring_collection,hit);
                    break;
                case Scoring::Type::Voxel:
                    FillVoxelEventCollection(current_scoring_collection,hit);
                    break;
                default:
                    break;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
///
void RunAnalysis::FillCellEventCollection(std::map<std::size_t, VoxelHit>& scoring_collection, VoxelHit* hit){
    auto& cell_hit = scoring_collection.at(hit->GetGlobalHashedStrId());
    // LOGSVC_DEBUG("Cell Dose BEFORE {}", cell_hit.GetDose());
    cell_hit.Cumulate(*hit,false); // check global alignement only
    // LOGSVC_DEBUG("Cell Dose AFTER {}", cell_hit.GetDose());
}

////////////////////////////////////////////////////////////////////////////////
///
void RunAnalysis::FillVoxelEventCollection(std::map<std::size_t, VoxelHit>& scoring_collection, VoxelHit* hit){
    auto& voxel_hit = scoring_collection.at(hit->GetHashedStrId());
    // LOGSVC_DEBUG("Voxel Dose BEFORE {}", voxel_hit.GetDose());
    voxel_hit.Cumulate(*hit,true); // check global and local alignement
    // LOGSVC_DEBUG("Voxel Dose AFTER {}", voxel_hit.GetDose());
}

////////////////////////////////////////////////////////////////////////////////
///
void RunAnalysis::EndOfRun(const G4Run* runPtr){
    LOGSVC_INFO("RunAnalysis::EndOfRun:: CtrlPoint-{} / G4Run-{}", m_current_cp->GetId(), runPtr->GetRunID());
    // Note: Multithreading merging is being performed before...
    
    m_csv_run_analysis->WriteDoseToCsv(runPtr);
    m_csv_run_analysis->WriteFieldMaskToCsv(runPtr);

    m_ntuple_run_analysis->WriteDoseToTFile(runPtr);
    m_ntuple_run_analysis->WriteFieldMaskToTFile(runPtr);
}