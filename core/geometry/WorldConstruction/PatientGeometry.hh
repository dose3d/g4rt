/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 10.12.2017
*
*/

#ifndef Dose3D_PHANTOMCONSTRUCTION_HH
#define Dose3D_PHANTOMCONSTRUCTION_HH

#include "G4GeometryManager.hh"
#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "G4UImessenger.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "globals.hh"
#include "IPhysicalVolume.hh"

class VPatient;

///\class PatientGeometry
///\brief The liniac Phantom volume construction.
class PatientGeometry : public IPhysicalVolume,
                            public Configurable {
  public:
  ///
  static PatientGeometry *GetInstance();

  ///
  void Construct(G4VPhysicalVolume *parentPV) override;

  ///
  void Destroy() override;

  ///
  G4bool Update() override;

  ///
  void Reset() override {}

  ///
  void WriteInfo() override;

  ///
  void DefineSensitiveDetector();

  ///
  void DefaultConfig(const std::string &unit) override;

  ///
  VPatient* GetPatient() const { return m_patient; }


  private:
  ///
  PatientGeometry();

  ///
  ~PatientGeometry();

  /// Delete the copy and move constructors
  PatientGeometry(const PatientGeometry &) = delete;

  PatientGeometry &operator=(const PatientGeometry &) = delete;

  PatientGeometry(PatientGeometry &&) = delete;

  PatientGeometry &operator=(PatientGeometry &&) = delete;

  ///
  bool design();

  ///
  void Configure() override;

  ///
  VPatient* m_patient;
};

#endif // Dose3D_PHANTOMCONSTRUCTION_HH
