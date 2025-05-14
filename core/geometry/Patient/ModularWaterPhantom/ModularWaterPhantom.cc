#include "ModularWaterPhantom.hh"
#include "G4UnionSolid.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "Services.hh"

////////////////////////////////////////////////////////////////////////////////
///
ModularWaterPhantom::ModularWaterPhantom():IPhysicalVolume("ModularWaterPhantom"){}

////////////////////////////////////////////////////////////////////////////////
///
ModularWaterPhantom* ModularWaterPhantom::GetInstance() {
  static ModularWaterPhantom instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
///
G4ThreeVector ModularWaterPhantom::IbaToLocalTranslation(0.0, 0.0, 0.0);
////////////////////////////////////////////////////////////////////////////////
///
void ModularWaterPhantom::Construct(G4VPhysicalVolume *parentPV) {

    std::cout << "ModularWaterPhantom::Construct called" << __FUNCTION__ << " called\n";
    // auto FullPhantomLV = new G4LogicalVolume(:v, :vvv.get(), "ModularWaterPhantomLV");
    // SetPhysicalVolume(new G4PVPlacement(:v, m_position, "ModularWaterPhantomPV", FullPhantomLV, parentPV, false, 0));
}