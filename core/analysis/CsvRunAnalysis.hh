//
// Created by brachwal on 13.02.2024.
//

#ifndef CSV_RUN_ANALYSIS_HH
#define CSV_RUN_ANALYSIS_HH

#include "globals.hh"

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

    public:
        ///
        static CsvRunAnalysis* GetInstance();
};

#endif //CSV_RUN_ANALYSIS_HH