#ifndef D3DF_GENERICPHANTOM_HH
#define D3DF_GENERICPHANTOM_HH

#include "IPhysicalVolume.hh"
#include "G4ThreeVector.hh"
#include "G4RotationMatrix.hh"

/// Generic DB/3MF phantom module.
///
/// This class builds an arbitrary geometry imported through the existing
/// GeometryDBReader/GeometryBuilder pipeline. It is intentionally not a VPatient:
/// no patient volume, patient-specific scoring model, or detector-specific
/// assumptions are created here.
class GenericPhantom : public IPhysicalVolume {
  private:
    GenericPhantom();
    ~GenericPhantom() = default;

    GenericPhantom(const GenericPhantom&) = delete;
    GenericPhantom& operator=(const GenericPhantom&) = delete;
    GenericPhantom(GenericPhantom&&) = delete;
    GenericPhantom& operator=(GenericPhantom&&) = delete;

    G4RotationMatrix* m_rotation = nullptr;

  public:
    static GenericPhantom* GetInstance();

    void Construct(G4VPhysicalVolume* parentPV) override;
    void Destroy() override {}
    G4bool Update() override { return true; }
    void Reset() override {}
    void WriteInfo() override;

    void SetRotation(G4RotationMatrix* rotation) { m_rotation = rotation; }
    G4RotationMatrix* GetRotation() const { return m_rotation; }
};

#endif // D3DF_GENERICPHANTOM_HH
