#include "ActionInitialization.hh"
#include "PrimaryGenerationAction.hh"
#include "G4ScoringManager.hh"
#include "UserScoreWriter.hh"
#include "EventAction.hh"
#include "SteppingAction.hh"
#include "RunAction.hh"

/////////////////////////////////////////////////////////////////////////////
///
void ActionInitialization::BuildForMaster() const {
  // In MT mode, to be clearer, the RunAction class for the master thread might be
  // different than the one used for the workers.
  // This RunAction will be called before and after starting the workers.
  // For more details, please refer to :
  // https://twiki.cern.ch/twiki/bin/view/Geant4/Geant4MTForApplicationDevelopers
  //

  G4ScoringManager::GetScoringManager()->SetScoreWriter(new UserScoreWriter());

  SetUserAction(new RunAction());
}

/////////////////////////////////////////////////////////////////////////////
///
void ActionInitialization::Build() const {

  G4ScoringManager::GetScoringManager()->SetScoreWriter(new UserScoreWriter());

  // Initialize the primary particles
  SetUserAction(new PrimaryGenerationAction());

  // Optional UserActions: run, event, stepping
  SetUserAction(new RunAction());
  auto evt = new EventAction();
  SetUserAction(evt);
  SetUserAction(new SteppingAction(evt));

}