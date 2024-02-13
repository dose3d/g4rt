//
// Created by brachwal on 28.04.2020.
//

#ifndef RUN_ANALYSIS_HH
#define RUN_ANALYSIS_HH

#include "globals.hh"
#include <vector>
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"
#include "G4Cache.hh"

class CsvRunAnalysis;
class G4Event;
class G4Run;

class RunAnalysis {

  private:
    ///
    RunAnalysis();

    ///
    ~RunAnalysis() = default;

    /// Delete the copy and move constructors
    RunAnalysis(const RunAnalysis &) = delete;
    RunAnalysis &operator=(const RunAnalysis &) = delete;
    RunAnalysis(RunAnalysis &&) = delete;
    RunAnalysis &operator=(RunAnalysis &&) = delete;

    ///
    static CsvRunAnalysis* m_csv_run_analysis;

    ///
    bool m_is_initialized = false;

  public:
    ///
    static RunAnalysis* GetInstance();

    ///
    void FillEvent(G4double totalEvEnergy);

    ///
    void BeginOfRun(const G4Run* runPtr, G4bool isMaster);

    ///
    void EndOfRun();

    ///
    void EndOfEventAction(const G4Event *evt);

    ///
    void ClearEventData();

    /// Many HitsCollections can be associated to given run collection
    static void AddRunCollection(const G4String& collection_name, const G4String& hc_name);

};
#endif //RUN_ANALYSIS_HH
