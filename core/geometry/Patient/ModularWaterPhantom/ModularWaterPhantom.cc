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

////////////////////////////////////////////////////////////////////////////////
///
void ModularWaterPhantom::Construct(G4VPhysicalVolume *parentPV) {

    std::cout << "ModularWaterPhantom::Construct called" << __FUNCTION__ << " called\n";
    /*
    auto smallInterBox = new G4Box("smallInnerBox", 125.0*mm, 25.0*mm, 200.0*mm);
    auto smallOuterBox = new G4Box("smallOuterBox", 135.0*mm, 35.0*mm, 200.0*mm);
    auto smallAquariumBox =  new G4SubtractionSolid("smallAquaBox", smallOuterBox, smallInterBox, nullptr, G4ThreeVector(0.0*mm,0.0*mm,-10.0*mm));
    auto smallWaterFillingBox = new G4Box("smallWaterFillingBox", 125.0*mm, 25.0*mm, 165.0*mm);

    auto bigInterBox = new G4Box("bigInnerBox", 125.0*mm, 125.0*mm, 200.0*mm);
    auto bigOuterBox = new G4Box("bigOuterBox", 135.0*mm, 135.0*mm, 200.0*mm);
    auto bigAquariumBox =  new G4SubtractionSolid("bigAquaBox", bigOuterBox, bigInterBox, nullptr, G4ThreeVector(0.0*mm,0.0*mm,-10.0*mm));
    auto bigWaterFillingBox = new G4Box("bigWaterFillingBox", 125.0*mm, 125.0*mm, 165.0*mm);
    
    auto smallAquaBoxLV =  new G4LogicalVolume(smallAquariumBox, boxMaterial.get(), "smallAquaBoxLV");
    auto bigAquaBoxLV =    new G4LogicalVolume(bigAquariumBox, boxMaterial.get(), "bigAquaBoxLV");
    auto smallWaterFillingBoxLV = new G4LogicalVolume(smallWaterFillingBox, waterMaterial.get(), "smallWaterFillingBoxLV");
    auto bigWaterFillingBoxLV =   new G4LogicalVolume(bigWaterFillingBox, waterMaterial.get(), "bigWaterFillingBoxLV");
    auto pv1 =         new G4PVPlacement(m_rotation, G4ThreeVector(envPosX + 150.0*mm, envPosY, envPosZ-260.0*mm), 
                                          "smallAquaBoxPV1", smallAquaBoxLV, pv, false, 0);
    auto pv1_filling = new G4PVPlacement(m_rotation, G4ThreeVector(envPosX + 150.0*mm, envPosY, envPosZ-235.0*mm), 
                                          "smallWaterFillingBoxPV1", smallWaterFillingBoxLV, pv, false, 0);
    auto pv2 =         new G4PVPlacement(m_rotation, G4ThreeVector(envPosX - 150.0*mm, envPosY, envPosZ-260.0*mm), 
                                          "smallAquaBoxPV2", smallAquaBoxLV, pv, false, 0);
    auto pv2_filling = new G4PVPlacement(m_rotation, G4ThreeVector(envPosX - 150.0*mm, envPosY, envPosZ-235.0*mm), 
                                          "smallWaterFillingBoxPV2", smallWaterFillingBoxLV, pv, false, 0);
    auto pv3 =         new G4PVPlacement(m_rotation, G4ThreeVector(envPosX + 138.0*mm, envPosY + 173.0*mm, envPosZ-260.0*mm), 
                                          "bigAquaBoxPV3",   bigAquaBoxLV,   pv, false, 0);
    auto pv3_filling = new G4PVPlacement(m_rotation, G4ThreeVector(envPosX + 138.0*mm, envPosY + 173.0*mm, envPosZ-235.0*mm), 
                                          "bigWaterFillingBoxPV3", bigWaterFillingBoxLV, pv, false, 0);
    auto pv4 =         new G4PVPlacement(m_rotation, G4ThreeVector(envPosX - 138.0*mm, envPosY + 173.0*mm, envPosZ-260.0*mm), 
                                          "bigAquaBoxPV4",   bigAquaBoxLV,   pv, false, 0);
    auto pv4_filling = new G4PVPlacement(m_rotation, G4ThreeVector(envPosX - 138.0*mm, envPosY + 173.0*mm, envPosZ-235.0*mm), 
                                          "bigWaterFillingBoxPV4", bigWaterFillingBoxLV, pv, false, 0);
    auto pv5 =         new G4PVPlacement(m_rotation, G4ThreeVector(envPosX + 138.0*mm, envPosY - 173.0*mm, envPosZ-260.0*mm), 
                                          "bigAquaBoxPV5",   bigAquaBoxLV,   pv, false, 0);
    auto pv5_filling = new G4PVPlacement(m_rotation, G4ThreeVector(envPosX + 138.0*mm, envPosY - 173.0*mm, envPosZ-235.0*mm), 
                                          "bigWaterFillingBoxPV5", bigWaterFillingBoxLV, pv, false, 0);
    auto pv6 =         new G4PVPlacement(m_rotation, G4ThreeVector(envPosX - 138.0*mm, envPosY - 173.0*mm, envPosZ-260.0*mm), 
                                          "bigAquaBoxPV6",   bigAquaBoxLV,   pv, false, 0);
    auto pv6_filling = new G4PVPlacement(m_rotation, G4ThreeVector(envPosX - 138.0*mm, envPosY - 173.0*mm, envPosZ-235.0*mm), 
                                          "bigWaterFillingBoxPV6", bigWaterFillingBoxLV, pv, false, 0);
    */
}