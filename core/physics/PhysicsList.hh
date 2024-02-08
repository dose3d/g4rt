/**
*
* \author B.Rachwal (brachwal [at] agh.edu.pl)
* \date 08.04.2020
*
*/

#ifndef Dose3D_PHYSICSLIST_H
#define Dose3D_PHYSICSLIST_H

#include "G4VModularPhysicsList.hh"
#include "globals.hh"
#include <memory>

class G4VPhysicsConstructor;

///\class PhysicsList
class PhysicsList : public G4VModularPhysicsList {
  public:
    ///
    PhysicsList();

    ///
    ~PhysicsList() = default;

    ///
    void ConstructParticle() override;

    ///
    void AddStepMax();   

    ///
    void AddPhysicsList(const G4String &name);

    ///
    void ConstructProcess() override;

  private:
    ///
    G4String m_emPhysicsModelName = "emstandard_opt3";

    ///
    std::unique_ptr<G4VPhysicsConstructor> m_emPhysicsModelCtr;

    ///
    std::unique_ptr<G4VPhysicsConstructor> m_decayPhysicsModelCtr;
    
};

#endif
