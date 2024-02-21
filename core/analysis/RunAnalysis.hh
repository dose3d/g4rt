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
#include "VoxelHit.hh"
#include "Types.hh"

class ControlPoint;
class CsvRunAnalysis;
class G4Event;
class G4Run;

typedef std::map<Scoring::Type, std::map<std::size_t, VoxelHit>> ScoringMap;

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

    /// Many HitsCollections can be associated to given collection name 
    // (e.g. when many sensitive detectors constituting a single detection unit)
    static std::map<G4String,std::vector<G4String>> m_run_collection;

    ///
    std::set<Scoring::Type> m_scoring_types;

    ///
    void FillEventCollection(const G4String& collection_name, const G4Event *evt, VoxelHitsCollection* hitsColl);

    ///
    void FillCellEventCollection(std::map<std::size_t, VoxelHit>& scoring_collection, VoxelHit* hit);
    void FillVoxelEventCollection(std::map<std::size_t, VoxelHit>& scoring_collection, VoxelHit* hit);

    ///
    CsvRunAnalysis* m_csv_run_analysis = nullptr;

    ///
    bool m_is_initialized = false;

    ///
    ControlPoint* m_current_cp = nullptr;

  public:
    ///
    static RunAnalysis* GetInstance();

    ///
    void BeginOfRun(const G4Run* runPtr, G4bool isMaster);

    ///
    void EndOfRun(const G4Run* runPtr);

    ///
    void EndOfEventAction(const G4Event *evt);


    /// Many HitsCollections can be associated to given run collection
    static void AddRunHCollection(const G4String& collection_name, const G4String& hc_name);
    static std::map<G4String,std::vector<G4String>> GetRunCollection() { return m_run_collection; }

};
#endif //RUN_ANALYSIS_HH
