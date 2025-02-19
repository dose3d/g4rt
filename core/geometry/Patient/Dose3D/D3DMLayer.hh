/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 27.07.2022
*
*/

#ifndef D3D_LAYER_HH
#define D3D_LAYER_HH

#include "G4PVPlacement.hh"
#include "D3DCell.hh"

///\class D3DMLayer
///\brief The Phantom constructed on top of Dose3D cells
class D3DMLayer : public VPatient {
  public:
    ///
    D3DMLayer(const G4String& label = "Dose3DLayer");

    ///
    D3DMLayer(const G4String& label,G4String cellMediumName, G4bool shiftZ);

    ///
    D3DMLayer(const G4String& label,G4String cellMediumName, const std::vector<G4ThreeVector>& cellsInLayer);

    ///
    ~D3DMLayer();

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
    static std::set<G4String> HitsCollections;

    ///
    const std::vector<D3DCell*>& GetCells() { return m_d3d_cells; }

    ///
    static G4double COVER_WIDTH;

    ///
    void SetId(G4int id) { m_id = id; }

    ///
    int GetId() const { return m_id; }

    ///
    void ParseTomlConfig() override {}

    /// 
    void SetNCells(char axis, int nc);

    /// 
    void SetCellNVoxels(char axis, int nc);

    ///
    void SetPosition(char axis, double pos);

    ///
    void SetPosition(const G4ThreeVector& vec) { m_init_possition = vec; }


  private:
    ///
    G4bool LoadParameterization();

    ///
    void SetNCells();

    ///
    G4String m_label;

    ///
    std::vector<std::vector<G4int>> m_layer_mapping;

    ///
    G4int m_n_cells_in_layer_x = 0;
    G4int m_n_cells_in_layer_z = 0;

    G4int m_cell_voxelization_x = 0;
    G4int m_cell_voxelization_y = 0;
    G4int m_cell_voxelization_z = 0;

    ///
    std::vector<G4ThreeVector> m_cells_in_layer_positioning;

    ///
    G4int m_id = 0;

    ///
    std::vector<D3DCell*> m_d3d_cells;
    

    /// 
    G4ThreeVector m_cell_voxelization;

    /// 
    G4String m_cell_medium_name;

    ///
    G4double m_init_x;
    G4double m_init_y;
    G4double m_init_z;

    ///
    G4ThreeVector m_init_possition;

    G4bool m_mlayer_shift;
};

#endif  // D3D_CELL_HH
