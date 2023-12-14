/**
*
* \author B.Rachwal (brachwal [at] agh.edu.pl)
* \date 14.11.2017
*
*/

#ifndef Dose3D_WORLDCONSTRUCTION_HH
#define Dose3D_WORLDCONSTRUCTION_HH

#include "G4VUserDetectorConstruction.hh"
#include "IPhysicalVolume.hh"
#include "Configurable.hh"
#include "Logable.hh"

class G4Run;
class PatientGeometry;
class SavePhSpConstruction;
class LinacGeometry;
class BeamMonitoring;

///\class WorldConstruction
///\brief The top level world volume construction factory. Geant4 app manner.
class WorldConstruction : public G4VUserDetectorConstruction,
                          public IPhysicalVolume,
                          public Configurable,
                          public Logable {
  public:
  ///
  void Destroy() override;

  ///
  G4bool Update() override;
  G4bool Update(int runId);

  ///
  void Reset() override {}

  ///
  void DefaultConfig(const std::string &unit) override;

  ///
  static WorldConstruction *GetInstance();

  ///
  void ConstructSDandField() override;

  ///
  bool newGeometry();

  ///
  std::string ExportToGDML(const std::string& path, const std::string& fileName, const std::string &worldName = std::string());

  ///
  void checkVolumeOverlap();

  ///
  void WriteInfo() override;

  ///
  void SetRunConfiguration(const G4Run* runPtr);

  /// 
  PatientGeometry* PatientEnvironment() { return m_phantomEnv; }


  /// so that the unique_ptr may delete the singleton
  friend std::unique_ptr<WorldConstruction>::deleter_type;

  /// Classes to which the ConstructSD is delegated
  friend class SavePhSpConstruction;
  friend class VPatient;
  friend class BeamMonitoring;

  private:
  ///
  WorldConstruction();

  ///
  ~WorldConstruction();

  /// Delete the copy and move constructors
  WorldConstruction(const WorldConstruction &) = delete;

  WorldConstruction &operator=(const WorldConstruction &) = delete;

  WorldConstruction(WorldConstruction &&) = delete;

  WorldConstruction &operator=(WorldConstruction &&) = delete;

  /// have to implement pure virtual function
  G4VPhysicalVolume* Construct() override;      // <- G4VUserDetectorConstruction

  /// have to implement pure virtual function
  void Construct(G4VPhysicalVolume*) override {}  // <- IPhysicalVolume

  ///
  bool Create();

  ///
  void Configure() override;

  ///
  PatientGeometry* m_phantomEnv = nullptr;

  ///
  SavePhSpConstruction* m_savePhSpEnv = nullptr;

  ///
  LinacGeometry* m_gantryEnv = nullptr;

  ///
  BeamMonitoring* m_beamMonitoring = nullptr;
};

#endif  // Dose3D_WORLDCONSTRUCTION_HH
