//
// Created by brachwal on 13.02.2024.
//

#ifndef CSV_RUN_ANALYSIS_HH
#define CSV_RUN_ANALYSIS_HH

#include "globals.hh"
#include "Types.hh"
#include "VoxelHit.hh"

class CsvRunAnalysis {
    private:
        ///
        CsvRunAnalysis();

        ///
        ~CsvRunAnalysis() = default;

        /// Delete the copy and move constructors
        CsvRunAnalysis(const CsvRunAnalysis &) = delete;
        CsvRunAnalysis &operator=(const CsvRunAnalysis &) = delete;
        CsvRunAnalysis(CsvRunAnalysis &&) = delete;
        CsvRunAnalysis &operator=(CsvRunAnalysis &&) = delete;

        ///
        std::map<Scoring::Type, std::map<std::size_t, VoxelHit>> m_hashed_scoring_map;

        ///
        std::set<Scoring::Type> m_scoring_types = {Scoring::Type::Cell, Scoring::Type::Voxel};

    public:
        ///
        static CsvRunAnalysis* GetInstance();
};

#endif //CSV_RUN_ANALYSIS_HH