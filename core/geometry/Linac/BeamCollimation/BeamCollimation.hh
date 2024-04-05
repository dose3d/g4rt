/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 18.05.2020
*
*/

#ifndef Dose3D_VARIAN_TRUEBEAM_HEAD_MOCKUP_HH
#define Dose3D_VARIAN_TRUEBEAM_HEAD_MOCKUP_HH

#include "IPhysicalVolume.hh"
#include "G4PrimaryVertex.hh"
#include "Configurable.hh"
#include "Types.hh"
#include "VMlc.hh"

class G4VPhysicalVolume;

///\class BeamCollimation
class BeamCollimation : public IPhysicalVolume,
                        public Configurable {
  public:
  ///
  static BeamCollimation *GetInstance();

  ///
  void Construct(G4VPhysicalVolume *parentPV) override;

  ///
  void Destroy() override;

  ///
  G4bool Update() override {
    G4cout << "BeamCollimation::Update::Implement me." << G4endl;
    return true;
  }

  ///
  void Reset() override;

  ///
  void WriteInfo() override;

  ///
  void DefaultConfig(const std::string &unit) override;

  static void FilterPrimaries(std::vector<G4PrimaryVertex*>& p_vrtx);

  static G4ThreeVector SetParticlePositionBeforeMLC(G4PrimaryVertex* vrtx, G4double finalZ);

  VMlc* GetMlc() const { return m_mlc.get(); }

  static G4double AfterMLC;
  static G4double BeforeMLC;
  VMlc* GetMlc() { return m_mlc.get(); }

  private:
  ///
  BeamCollimation();

  ///
  ~BeamCollimation();

  // Delete the copy and move constructors
  BeamCollimation(const BeamCollimation &) = delete;

  BeamCollimation &operator=(const BeamCollimation &) = delete;

  BeamCollimation(BeamCollimation &&) = delete;

  BeamCollimation &operator=(BeamCollimation &&) = delete;

  ///
  void Configure() override;

  ///
  std::vector<G4double> m_leavesA, m_leavesB; 

  ///
  std::map<G4String, G4VPhysicalVolume *> m_physicalVolume;

  ///
  void SetJawAperture(G4int idJaw, G4ThreeVector &centre, G4ThreeVector halfSize, G4RotationMatrix *cRotation);

  bool Jaw1X();

  bool Jaw2X();

  bool Jaw1Y();

  bool Jaw2Y();

  bool MLC();

  ///
  static std::unique_ptr<VMlc> m_mlc;

  ///
  void DefineSensitiveDetector() {}



};

#endif // Dose3D_VARIAN_TRUEBEAM_HEAD_MOCKUP_HH
