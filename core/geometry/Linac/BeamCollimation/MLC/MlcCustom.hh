//
// Created by brachwal on 25.08.2020.
//

#ifndef DOSE3D_VARIANMLCCUSTOM_HH
#define DOSE3D_VARIANMLCCUSTOM_HH

#include <vector>
#include "Types.hh"
#include "IPhysicalVolume.hh"

class G4Region;

class MlcCustom : public IPhysicalVolume
                , public VMlc {
  private:
      ///
      std::unique_ptr<G4Region> m_mlc_region;

      ///
      std::vector<G4VPhysicalVolumeUPtr> m_y1_leaves;
      std::vector<G4VPhysicalVolumeUPtr> m_y2_leaves;

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

  public:
      ///
      MlcCustom() = delete;

      ///
      explicit MlcCustom(G4VPhysicalVolume* parentPV);

      ////
      ~MlcCustom();

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
#endif //DOSE3D_VARIANMLCCUSTOM_HH
