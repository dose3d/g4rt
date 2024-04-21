#ifndef Dose3D_TRAYCONSTRUCTION_HH
#define Dose3D_TRAYCONSTRUCTION_HH

#include "IPhysicalVolume.hh"
#include "TomlConfigurable.hh"
#include "Services.hh"

class VPatient;

///\class PatientGeometry
///\brief The liniac Phantom volume construction.
class D3DTray : public IPhysicalVolume {
    private:
        // Extra wrapper to make single TOML-like configuration for all instances of Trays,
        // Typpically, each D3DTray would inherit from TomlConfigModule.
        class TConfigurarable: public TomlConfigModule {
            public:
                TConfigurarable(const std::string& name):TomlConfigModule(name){}
                void ParseTomlConfig() override;
        };

        static TConfigurarable m_tconfigurable;

    public:
    ///
    D3DTray(G4RotationMatrix& rotMatrix, G4VPhysicalVolume *parentPV, const std::string& name, const G4ThreeVector& position);

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
    
    ///
    void DefineSensitiveDetector();


    VPatient* GetDetector() const { return m_detector; }

    G4ThreeVector m_global_centre;
    G4ThreeVector m_tray_world_halfSize;
    std::string m_tray_name;
    std::string m_tray_config_file;
    G4RotationMatrix m_rot;

    ///
    VPatient* m_detector;

};


#endif //Dose3D_TRAYCONSTRUCTION_HH