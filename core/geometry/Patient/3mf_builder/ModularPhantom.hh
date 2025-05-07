#ifndef MODULAR_PHANTOM_HH
#define MODULAR_PHANTOM_HH

#include "G4PVPlacement.hh"
#include "VPatient.hh"

///\class ModularPhantom
class ModularPhantom : public VPatient {
  public:

  /// 
  static ModularPhantom *GetInstance();

  /// 
  ModularPhantom();

  /// 
  ~ModularPhantom();

  /// 
  void Construct(G4VPhysicalVolume *parentPV) override;

  /// 
  void Destroy() override;

  /// 
  G4bool Update() override;

  /// 
  void Reset() override { G4cout << "Implement me." << G4endl; }

  ///  
  void WriteInfo() override;

  ///
  void DefineSensitiveDetector() override;

  ///
  std::map<std::size_t, VoxelHit> GetScoringHashedMap(const G4String& scoring_name,Scoring::Type type) const override;

  ///
  friend class PatientTest_ModularPhantomScoring_Test;

  private:

  ///
  G4bool LoadParameterization();

  ///
  G4bool LoadDefaultParameterization();

  ///
  void ParseTomlConfig() override;

  ///
  void ConstructSensitiveDetector() override;


  ///
  std::string m_phantomMedium = "None";

  /// 
  G4double m_centrePositionX = 0.0;
  G4double m_centrePositionY = 0.0;
  G4double m_centrePositionZ = 0.0;

  /// 
  G4double m_sizeX = 0.0;
  G4double m_sizeY = 0.0;
  G4double m_sizeZ = 0.0;

  ///
  G4int m_detectorVoxelizationX = 0;
  G4int m_detectorVoxelizationY = 0;
  G4int m_detectorVoxelizationZ = 0;

  ///
  G4bool m_watertankScoring = true;

  ///
  G4bool m_farmerScoring = false;

};

#endif  // MODULAR_PHANTOM_HH
