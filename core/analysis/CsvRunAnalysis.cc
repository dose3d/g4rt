#include "CsvRunAnalysis.hh"
#include "Services.hh"
#include "ControlPoint.hh"

////////////////////////////////////////////////////////////////////////////////
///
CsvRunAnalysis *CsvRunAnalysis::GetInstance() {
    static CsvRunAnalysis instance = CsvRunAnalysis();
    return &instance;
}

////////////////////////////////////////////////////////////////////////////////
///
void CsvRunAnalysis::WriteDoseToCsv(const G4Run* runPtr){
    LOGSVC_INFO("CsvRunAnalysis::WriteDoseToCsv...");
}


