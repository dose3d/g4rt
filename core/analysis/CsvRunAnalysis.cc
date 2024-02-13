#include "CsvRunAnalysis.hh"
#include "Services.hh"

std::map<G4String,std::vector<G4String>> CsvRunAnalysis::m_run_collection = std::map<G4String,std::vector<G4String>>();

////////////////////////////////////////////////////////////////////////////////
///
CsvRunAnalysis::CsvRunAnalysis(){

}

////////////////////////////////////////////////////////////////////////////////
///
CsvRunAnalysis *CsvRunAnalysis::GetInstance() {
    static CsvRunAnalysis instance = CsvRunAnalysis();
    return &instance;
}

////////////////////////////////////////////////////////////////////////////////
///
void CsvRunAnalysis::AddRunCollection(const G4String& collection_name, const G4String& hc_name){
    LOGSVC_DEBUG("CsvRunAnalysis AddRunCollection: {} / {}",collection_name, hc_name);
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
            LOGSVC_INFO("Adding new map for {} / {} scoring",run_collection.first, Scoring::to_string(scoring_type));
            if(!m_run_scoring_collection.Has(run_collection.first))
                m_run_scoring_collection.Insert(run_collection.first,ScoringMap());
            auto& scoring_collection = m_run_scoring_collection.Get(run_collection.first);
            scoring_collection[scoring_type] = Service<GeoSvc>()->Patient()->GetScoringHashedMap(scoring_type);
        }
    }
}

