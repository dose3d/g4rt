/**
*
* \author B. Rachwal (brachwal [at] agh.edu.pl)
* \date 21.06.2024
*
*/

#ifndef TLD_SD_HH
#define TLD_SD_HH

#include "VPatientSD.hh"

class TLDSD : public VPatientSD {
  public:
    ///
    TLDSD(const G4String& sdName, const G4ThreeVector& centre, G4int idX, G4int idY, G4int idZ, G4int surfaceScoringLayers = 0);

    ///
    ~TLDSD() = default;

    ///
    G4bool ProcessHits(G4Step*, G4TouchableHistory*) override;

  private:
    G4bool IsSurfaceVoxel(const ScoringVolume* scoringVolumePtr, G4int voxelIdX, G4int voxelIdY, G4int voxelIdZ) const;
    void ProcessHitsCollectionSurfaceAware(const G4String& hitsCollectionName, G4Step* aStep);

    G4int m_surface_scoring_layers = 0;
};

#endif //TLD_SD_HH
