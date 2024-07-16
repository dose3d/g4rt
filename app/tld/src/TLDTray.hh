#ifndef Dose3D_TRAYCONSTRUCTION_HH
#define Dose3D_TRAYCONSTRUCTION_HH

#include "IPhysicalVolume.hh"
#include "TomlConfigurable.hh"
#include "Services.hh"
#include "TLD.hh"

///
class TLDTray : public IPhysicalVolume, public TomlConfigModule {
    private:
        ///
        void ParseTomlConfig() override;

        ///
        void LoadConfiguration();

        class Config {
            public:
                std::string m_tld_medium = "None";

                G4int m_nX_tld = 0;
                G4int m_nY_tld = 0;
                G4int m_nZ_tld = 0;
                
                G4ThreeVector m_top_position_in_env;
                
                G4int m_tld_nX_voxels = 0;
                G4int m_tld_nY_voxels = 0;
                G4int m_tld_nZ_voxels = 0;
                
                bool m_initialized = false;
            };

    public:
    ///
    TLDTray(G4VPhysicalVolume *parentPV, const std::string& name);

    ///
    ~TLDTray() {};

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


    // VPatient* GetDetector() const { return m_detector; }

    G4ThreeVector m_global_centre;
    G4ThreeVector m_tray_world_halfSize;
    std::string m_tray_name;
    std::string m_tray_config_file;
    G4RotationMatrix m_rot;

    ///
    std::vector<TLD*> m_tld_detectors;

    ///
    TLDTray::Config m_config;
};


#endif //Dose3D_TRAYCONSTRUCTION_HH