#ifndef Dose3D_TRAYCONSTRUCTION_HH
#define Dose3D_TRAYCONSTRUCTION_HH

#include "IPhysicalVolume.hh"
#include "Services.hh"

class VPatient;

///\class PatientGeometry
///\brief The liniac Phantom volume construction.
class D3DTray : public IPhysicalVolume{
    public:
    ///
    D3DTray(G4RotationMatrix& rotMatrix, G4VPhysicalVolume *parentPV, const std::string& name, const G4ThreeVector& position, const G4ThreeVector& halfSize);

    ///
    ~D3DTray() {};

    ///
    void Construct(G4VPhysicalVolume *parentPV) override;

    ///
    void Destroy() override {}

    ///
    G4bool Update() override { return true;}

    ///
    void Reset() override {}

    ///
    void WriteInfo() override {}
    
    
    void Rotate(G4RotationMatrix& rotMatrix); 

    ///
    void DefineSensitiveDetector();


    VPatient* GetDetector() const { return m_detector; }

    private:
    ///
    void Configure();

    G4ThreeVector m_global_centre;
    G4ThreeVector m_tray_world_halfSize;
    std::string m_tray_name;
    std::string m_tray_config_file;
    G4RotationMatrix m_rot;

    ///
    VPatient* m_detector;

};


#endif //Dose3D_TRAYCONSTRUCTION_HH