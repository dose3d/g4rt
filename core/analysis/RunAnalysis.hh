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

class G4Event;
class G4Run;

class RunAnalysis {

  private:
  ///
  RunAnalysis() = default;

  ///
  ~RunAnalysis() = default;

  /// Delete the copy and move constructors
  RunAnalysis(const RunAnalysis &) = delete;

  RunAnalysis &operator=(const RunAnalysis &) = delete;

  RunAnalysis(RunAnalysis &&) = delete;

  RunAnalysis &operator=(RunAnalysis &&) = delete;

  // TODO
  // zdefiniować histogram do ktorego bede ustawial wartosci poprzez SetBinContent()
  //G4VectorCache<G4double> m_voxelTotalDoseZProfile; // suma po wszystkich eventa (nie robic .Clear())
  //G4Cache<G4int> m_pddHistId;

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

};
#endif //RUN_ANALYSIS_HH
