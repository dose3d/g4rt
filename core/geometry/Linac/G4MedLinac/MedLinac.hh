/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 14.11.2017
*
*/

#ifndef Dose3D_VARIAN2300CDHEAD_HH
#define Dose3D_VARIAN2300CDHEAD_HH

#include "G4Box.hh"
#include "G4Cons.hh"
#include "G4LogicalVolume.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4Tubs.hh"
#include "G4VPhysicalVolume.hh"
#include "G4VisAttributes.hh"
#include "G4BooleanSolid.hh"
#include "G4IntersectionSolid.hh"
#include "G4ProductionCuts.hh"
#include "G4SubtractionSolid.hh"
#include "G4UnionSolid.hh"
#include "IPhysicalVolume.hh"
#include "Configurable.hh"

#include "ConfigSvc.hh"

///\class MedLinac
class MedLinac : public IPhysicalVolume, public Configurable {
  public:
  static MedLinac *GetInstance(void);

  void Construct(G4VPhysicalVolume *parentPV);

  void Destroy() override;

  G4bool Update() {
    G4cout << "MedLinac::Update::Implement me." << G4endl;
    return true;
  }

  void Reset() override;

  void WriteInfo() override;

  void DefaultConfig(const std::string &unit) override;

  private:
  MedLinac();

  ~MedLinac();

  // Delete the copy and move constructors
  MedLinac(const MedLinac &) = delete;

  MedLinac &operator=(const MedLinac &) = delete;

  MedLinac(MedLinac &&) = delete;

  MedLinac &operator=(MedLinac &&) = delete;

  void Configure() override;

  G4VPhysicalVolume *m_PVvacuumWorld;
  std::vector<G4PVPlacement *> m_headPhspPV;
  std::vector<G4double> m_leavesA, m_leavesB;
  std::map<G4String, G4VPhysicalVolume *> m_physicalVolume;

  void SetJawAperture(G4int idJaw, G4ThreeVector &centre, G4ThreeVector halfSize, G4RotationMatrix *cRotation);

  bool createVacuumWorld();

  bool target();

  bool primaryCollimator();

  bool secondaryCollimator();

  bool BeWindow();

  bool flatteningFilter();

  bool ionizationChamber();

  bool mirror();

  bool cover();

  bool Jaw1X();

  bool Jaw2X();

  bool Jaw1Y();

  bool Jaw2Y();

  bool MLC();

  bool createPhspPlanes();
};

#endif
