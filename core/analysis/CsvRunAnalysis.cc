#include "CsvRunAnalysis.hh"
#include "Services.hh"

////////////////////////////////////////////////////////////////////////////////
///
CsvRunAnalysis::CsvRunAnalysis(){
    for(const auto& scoring_type: m_scoring_types){
        LOGSVC_INFO("Adding new map for scoring type: {}",Scoring::to_string(scoring_type));
        m_hashed_scoring_map[scoring_type] = Service<GeoSvc>()->Patient()->GetScoringHashedMap(scoring_type);
    }
}

////////////////////////////////////////////////////////////////////////////////
///
CsvRunAnalysis *CsvRunAnalysis::GetInstance() {
    static CsvRunAnalysis instance = CsvRunAnalysis();
    return &instance;
}
