/**
*
* \author B. Rachwal
* \date 05.05.2020
*
*/

#ifndef Dose3D_RUNACTION_HH
#define Dose3D_RUNACTION_HH

#include "G4Timer.hh"
#include "G4UserRunAction.hh"
#include "globals.hh"
#include "G4AnalysisManager.hh"
#include "G4Run.hh"

class CPRun : public G4Run {
  public:
    CPRun() = default;
    ~CPRun(){
      G4cout << "DESTRUCOTOR OF CPRUN..." << G4endl;
    };
    void Merge(const G4Run* aRun) override;
};

///
class RunAction : public G4UserRunAction {
  public:
  RunAction();

  ~RunAction();

  void BeginOfRunAction(const G4Run *aRun) override ;

  void EndOfRunAction(const G4Run *aRun) override;

  G4Run* GenerateRun() override;

  private:
  G4Timer m_timer;
};

#endif // Dose3D_RUNACTION_HH
