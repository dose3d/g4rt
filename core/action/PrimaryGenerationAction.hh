/**
*
* \author B.Rachwal (brachwal [at] agh.edu.pl)
* \date 08.04.2020
*
*/

#ifndef PRIMARYGENERATIONACTION_H
#define PRIMARYGENERATIONACTION_H


//#include "G4RunManager.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4String.hh"
#include "G4RotationMatrix.hh"

class G4Event;
class G4PrimaryVertex;
class G4VPrimaryGenerator;

enum class PrimaryGeneratorType : G4int {
  PhspIAEA = 1,   ///< IAEA binary file format
  GPS = 2,        ///< Geant4 GPS particle source (based on macro files interface)
  IonGPS = 3      ///< Ion GPS-based particle source
};

///\class PrimaryGenerationAction
class PrimaryGenerationAction : public G4VUserPrimaryGeneratorAction {
  public:
    ///
    PrimaryGenerationAction();

    ///
    ~PrimaryGenerationAction();

    ///
    void GeneratePrimaries(G4Event *anEvent) override;

    ///
    static void SetRotation(G4RotationMatrix* rotMatrix) { m_rotation_matrix = rotMatrix; }

  private:

    ///
    PrimaryGeneratorType m_generatorType = PrimaryGeneratorType::PhspIAEA;

    ///
    G4VPrimaryGenerator* m_primaryGenerator = nullptr;

    ///
    void SetPrimaryGenerator();

    ///
    void ConfigurePrimaryGenerator();

    ///
    static G4RotationMatrix* m_rotation_matrix;

    ///
    int m_min_p_vrtx_vec_size = 1;
};

#endif // PRIMARYGENERATIONACTION_H
