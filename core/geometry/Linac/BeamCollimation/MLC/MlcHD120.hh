//
// Created by brachwal on 25.08.2020.
//

#ifndef DOSE3D_VARIANMLCHD120_HH
#define DOSE3D_VARIANMLCHD120_HH

#include <vector>
#include "Types.hh"
#include "IPhysicalVolume.hh"
#include "VMlc.hh"
#include <G4IntersectionSolid.hh>
#include <G4SubtractionSolid.hh>
#include <G4UnionSolid.hh>

class G4Region;

class MlcHd120 :  public IPhysicalVolume, public VMlc {
  private:
    ///
    G4double m_mlcPosZ = 46.5 * cm; // The fixed Z position

    ///
    G4double m_innerRadius = 0. * cm;
    
    ///
    G4double m_cylinderRadius = 16. * cm;
    
    ///
    G4double m_leafLength = 32. * cm;
    
    ///
    G4double m_leafHeight = 6.9 * cm;

    ///
    std::unique_ptr<G4Region> m_mlc_region;

    ///


    ///
    void Construct(G4VPhysicalVolume *parentPV) override;

    ///
    void Destroy() override {}; // issue TNSIM-48

    ///
    void Configure() override;

    ///
    void SetCustomPositioning(const std::string& fieldSize);

    ///
    void SetRTPlanPositioning(int current_beam, int current_controlpoint);

    ///
    G4VPhysicalVolume* CreateMlcModules(G4VPhysicalVolume* parentPV,G4Material* material);

    /// note: replica of y1
    G4VPhysicalVolume* ReplicateSideMlcModule(const std::string& newName, G4VPhysicalVolume* pv);

    ///
    G4VSolid* CreateCentralLeafShape() const;
    G4VSolid* CreateSideLeafShape() const;
    G4VSolid* CreateTransitionLeafShape(const std::string& type) const; // side="Central","Side"
    G4VSolid* CreateEndCapCutBox() const;

  public:

    MlcHd120() = delete;

    ///
    explicit MlcHd120(G4VPhysicalVolume* parentPV);

    ////
    ~MlcHd120() override;

    ///
    void DefaultConfig(const std::string &unit) override;
    
    ///
    void SetRunConfig() override;

    ///
    G4bool Update() override;

    ///
    void Reset() override;

    ///
    void WriteInfo() override;

};
#endif //DOSE3D_VARIANMLCHD120_HH
