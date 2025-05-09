#ifndef M_GEOMRTRY_TOLERANCE_HH
#define M_GEOMRTRY_TOLERANCE_HH
#include "G4GeometryTolerance.hh"

class MyGeometryTolerance : public G4GeometryTolerance {
  public:

    static void ResetSurfaceTolerance(G4double worldExtent) {

      G4GeometryTolerance* tol = G4GeometryTolerance::GetInstance();

      tol->SetSurfaceTolerance(worldExtent);
    }
  };
#endif  // M_GEOMRTRY_TOLERANCE_HH
