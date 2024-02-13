#include "CsvRunAnalysis.hh"
#include "G4SDManager.hh"
#include "Services.hh"

std::map<G4String,std::vector<G4String>> CsvRunAnalysis::m_run_collection = std::map<G4String,std::vector<G4String>>();

////////////////////////////////////////////////////////////////////////////////
///
CsvRunAnalysis *CsvRunAnalysis::GetInstance() {
    static CsvRunAnalysis instance = CsvRunAnalysis();
    return &instance;
}

////////////////////////////////////////////////////////////////////////////////
///
void CsvRunAnalysis::AddRunCollection(const G4String& collection_name, const G4String& hc_name){
    // LOGSVC_DEBUG("CsvRunAnalysis AddRunCollection: {} / {}",collection_name, hc_name);
    if(m_run_collection.find( collection_name ) == m_run_collection.end())
        m_run_collection[collection_name] = std::vector<G4String>();
    m_run_collection.at(collection_name).emplace_back(hc_name);
}

////////////////////////////////////////////////////////////////////////////////
///
void CsvRunAnalysis::BeginOfRun(){
    LOGSVC_INFO("CsvRunAnalysis::BeginOfRun");
    LOGSVC_DEBUG("CsvRunAnalysis #RunCollections: {}",m_run_collection.size());
    for(const auto& run_collection: m_run_collection){
        LOGSVC_DEBUG("CsvRunAnalysis RunCollection: {} / #HitsCollections {}",run_collection.first, run_collection.second.size());
        for(const auto& scoring_type: m_scoring_types){
            auto scoring_str = Scoring::to_string(scoring_type);
            LOGSVC_INFO("Adding new map for {} / {} scoring",run_collection.first, scoring_str);
            if(!m_run_scoring_collection.Has(run_collection.first))
                m_run_scoring_collection.Insert(run_collection.first,ScoringMap());
            auto& scoring_collection = m_run_scoring_collection.Get(run_collection.first);
            scoring_collection[scoring_type] = Service<GeoSvc>()->Patient()->GetScoringHashedMap(scoring_type);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
///
void CsvRunAnalysis::EndOfEvent(const G4Event *evt){
    auto hCofThisEvent = evt->GetHCofThisEvent();
    for(const auto& run_collection: m_run_collection){
        LOGSVC_DEBUG("RunCol {}",run_collection.first);
        for(const auto& hc: run_collection.second){
            // Related SensitiveDetector collection ID (Geant4 architecture)
            // collID==-1 the collection is not found
            // collID==-2 the collection name is ambiguous
            auto collection_id = G4SDManager::GetSDMpointer()->GetCollectionID(hc);
            if(collection_id<0){
                LOGSVC_DEBUG("CsvRunAnalysis::EndOfEvent: HC: {} / G4SDManager Err: {}", hc, collection_id);
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
void CsvRunAnalysis::FillEventCollection(const G4String& collection_name, const G4Event *evt, VoxelHitsCollection* hitsColl){
    int nHits = hitsColl->entries();
    auto hc_name = hitsColl->GetName();
    //LOGSVC_DEBUG("HC: {} / nHits: {}",hc_name, nHits);
    if(nHits==0){
        return; // no hits in this event
    }
    auto& scoring_collection = m_run_scoring_collection.Get(collection_name);
    for (int i=0;i<nHits;i++){ // a.k.a. voxel loop
        auto hit = dynamic_cast<VoxelHit*>(hitsColl->GetHit(i));
        for(const auto& scoring_type: m_scoring_types){
            auto current_scoring_collection = scoring_collection[scoring_type];
        }
    }
}



