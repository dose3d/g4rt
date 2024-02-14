//
// Created by brachwal on 13.02.2024.
//

#ifndef CSV_RUN_ANALYSIS_HH
#define CSV_RUN_ANALYSIS_HH

#include "globals.hh"
#include "Types.hh"
// #include "VoxelHit.hh"
// #include "G4Cache.hh"

// typedef std::map<Scoring::Type, std::map<std::size_t, VoxelHit>> ScoringMap;

class CsvRunAnalysis {
    private:
        ///
        CsvRunAnalysis() = default;

        ///
        ~CsvRunAnalysis() = default;

        /// Delete the copy and move constructors
        CsvRunAnalysis(const CsvRunAnalysis &) = delete;
        CsvRunAnalysis &operator=(const CsvRunAnalysis &) = delete;
        CsvRunAnalysis(CsvRunAnalysis &&) = delete;
        CsvRunAnalysis &operator=(CsvRunAnalysis &&) = delete;
        
        // /// Many HitsCollections can be associated to given collection name 
        // // (e.g. many sensitive detectors constituting a single detection unit)
        // static std::map<G4String,std::vector<G4String>> m_run_collection;

        // ///
        // G4MapCache<G4String,ScoringMap> m_run_scoring_collection;

        // ///
        // std::set<Scoring::Type> m_scoring_types = {Scoring::Type::Cell, Scoring::Type::Voxel};

        // ///
        // void FillEventCollection(const G4String& collection_name, const G4Event *evt, VoxelHitsCollection* hitsColl);

        // ///
        // void FillCellEventCollection(std::map<std::size_t, VoxelHit>& scoring_collection, VoxelHit* hit);
        // void FillVoxelEventCollection(std::map<std::size_t, VoxelHit>& scoring_collection, VoxelHit* hit);


    public:
        ///
        static CsvRunAnalysis* GetInstance();

        /// Many HitsCollections can be associated to given run collection
        // static void AddRunCollection(const G4String& collection_name, const G4String& hc_name);

        ///
        // void BeginOfRun();

        ///
        // void EndOfEvent(const G4Event *evt);
};

#endif //CSV_RUN_ANALYSIS_HH