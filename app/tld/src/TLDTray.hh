#ifndef Dose3D_TRAYCONSTRUCTION_HH
#define Dose3D_TRAYCONSTRUCTION_HH

#include "IPhysicalVolume.hh"
#include "TomlConfigurable.hh"
#include "Services.hh"
#include "TLD.hh"
#include "CADMesh.hh"
#include <map>
#include <memory>
#include <vector>

class TLDTray : public VPatient {
    private:
        ///
        void ParseTomlConfig() override;

        ///
        void LoadConfiguration();

        ///
        void ConstructSupplementaryGeometry(G4VPhysicalVolume *parentPV);

        class Config {
            public:
                std::string m_tld_medium = "None";

                G4int m_nX_tld = 3;
                G4int m_nY_tld = 4;
                G4int m_nZ_tld = 1;
                
                G4ThreeVector m_top_position_in_env;
                
                G4int m_tld_nX_voxels = 10;
                G4int m_tld_nY_voxels = 10;
                G4int m_tld_nZ_voxels = 10;

                G4int m_surface_scoring_layers = 0;

                G4String m_stl_geometry_file_path = "None";
                G4String m_supplementary_geometry_path = "None";
                G4String m_supplementary_geometry_material = "None";
                G4ThreeVector m_supplementary_geometry_position = G4ThreeVector();
                
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

    G4ThreeVector m_global_centre;
    G4ThreeVector m_tray_world_halfSize;
    std::string m_tray_name;
    std::string m_tray_config_file;
    G4RotationMatrix m_rot;

    ///
    std::vector<TLD*> m_tld_detectors;

    ///
    TLDTray::Config m_config;

    ///
    std::map<std::size_t, VoxelHit> GetScoringHashedMap(const G4String& scoring_name,Scoring::Type type) const override;

  private:
    std::shared_ptr<CADMesh::TessellatedMesh> m_supplementary_mesh;
    G4PVPlacement* m_supplementary_volume = nullptr;
};

#endif //Dose3D_TRAYCONSTRUCTION_HH
