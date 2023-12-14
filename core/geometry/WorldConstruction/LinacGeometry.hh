/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 14.11.2017
*
*/

#ifndef Dose3D_HEADCONSTUCTION_HH
#define Dose3D_HEADCONSTUCTION_HH

#include "IPhysicalVolume.hh"
#include "Configurable.hh"

class G4Run;
class G4VPhysicalVolume;;

///\class LinacGeometry
///\brief The linac head level volume construction factory.
class LinacGeometry : public Configurable, public IPhysicalVolume {
  public:
  static LinacGeometry *GetInstance();

  void Construct(G4VPhysicalVolume *parentPV) override;

  void Destroy() override;

  G4bool Update() override;

  void Reset() override {};

  void ResetHead();

  void WriteInfo() override;

  G4RotationMatrix *rotateHead();

  G4RotationMatrix *rotateHead(G4double angleX);

  ///
  void DefineSensitiveDetector();

  private:
  LinacGeometry();

  ~LinacGeometry() = default;

  // Delete the copy and move constructors
  LinacGeometry(const LinacGeometry &) = delete;

  LinacGeometry &operator=(const LinacGeometry &) = delete;

  LinacGeometry(LinacGeometry &&) = delete;

  LinacGeometry &operator=(LinacGeometry &&) = delete;

  bool design();

  void DefaultConfig(const std::string &unit) override;

  void Configure() override;

  /// Linac Head:

  ///
  IPhysicalVolume* m_headInstance = nullptr;

  /// Periferials:

  ///
  IPhysicalVolume* m_tableInstance = nullptr;

  ///
  IPhysicalVolume* m_obiInstance = nullptr;

  ///
  IPhysicalVolume* m_epidInstance = nullptr;
};

#endif  // Dose3D_HEADCONSTUCTION_HH
