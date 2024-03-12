/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 16.05.2022
*
*/

#ifndef D3D_MODULE_HH
#define D3D_MODULE_HH

#include "G4PVPlacement.hh"
#include "D3DCell.hh"
#include "D3DMLayer.hh"
#include "Types.hh"
#include <string>

///\class D3DDetector
///\brief The Phantom constructed on top of Dose3D cells
class D3DDetector : public VPatient, public GeoComponet{
  public:
    ///
    D3DDetector();

    ///
    ~D3DDetector();

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
    std::string SetGeometrySource();
    
    ///
    bool IsAnyCellVoxelised(int idx, const G4String& run_collection) const;
    bool IsAnyCellVoxelised(D3DMLayer* layer, const G4String& run_collection) const;

    ///
    void ExportCellPositioningToCsv(const std::string& path_to_output_dir) const override;
    void ExportVoxelPositioningToCsv(const std::string& path_to_output_dir) const override;
    void ExportPositioningToTFile(const std::string& path_to_output_dir) const override;
    void ExportToGateCsv(const std::string& path_to_output_dir) const override;
    void ExportLayerPads(const std::string& path_to_output_dir) const;

    //
    std::map<std::size_t, VoxelHit> GetScoringHashedMap(const G4String& scoring_name,Scoring::Type type) const override;

  private:

    ///
    G4bool LoadParameterization();

    ///
    G4bool LoadDefaultParameterization();

    ///
    void ParseTomlConfig() override;

    ///
    void AcceptGeoVisitor(GeoSvc *visitor) const override;

    ///
    G4String m_label;

    std::string m_cell_medium = "None";

    //
    G4String m_stl_geometry_file_path = "None";

    //
    G4String m_in_layer_positioning_module = "None";

    G4int m_nX_cells = 0;
    G4int m_nY_cells = 0;
    G4int m_nZ_cells = 0;
    
    /// 
    G4ThreeVector m_top_position_in_env;
    
    ///
    G4int m_cell_nX_voxels = 0;
    G4int m_cell_nY_voxels = 0;
    G4int m_cell_nZ_voxels = 0;
    
    ///
    bool m_mrow_shift = false;
    bool m_mlayer_shift = false;

    ///
    std::vector<D3DMLayer*> m_d3d_layers;
    
    /// Container for cells positioning read-in from file
    std::vector<std::vector<G4ThreeVector>> m_d3d_cells_in_layers_positioning;

    ///
    void ReadCellsInLayersPositioning();

    ///
    static std::map<std::string, std::map<std::size_t, VoxelHit>> m_hashed_scoring_map_template; 
};

#endif  // D3D_MODULE_HH
