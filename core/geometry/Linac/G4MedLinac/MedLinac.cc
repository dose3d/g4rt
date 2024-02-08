#include "MedLinac.hh"
#include "G4SystemOfUnits.hh"
#include "Services.hh"

////////////////////////////////////////////////////////////////////////////////
///
MedLinac::MedLinac() : IPhysicalVolume("MedLinac"), Configurable("MedLinac"), m_PVvacuumWorld(nullptr) {
  m_leavesA = *m_geoSvc->getLeavesPositioning("A");
  m_leavesB = *m_geoSvc->getLeavesPositioning("B");
  Configure();
}

////////////////////////////////////////////////////////////////////////////////
///
MedLinac::~MedLinac() {
  Destroy();
  configSvc()->Unregister(thisConfig()->GetName());
}

////////////////////////////////////////////////////////////////////////////////
///
MedLinac *MedLinac::GetInstance() {
  static MedLinac instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
///
void MedLinac::Configure() {
  G4cout << "\n\n[INFO]::  Configuring the " << thisConfig()->GetName() << G4endl;
  DefineUnit<std::string>("Name");
  DefineUnit<G4double>("ionizationChamberThicknessP");
  DefineUnit<G4double>("ionizationChamberThicknessW");

  Configurable::DefaultConfig();   // setup the default configuration for all defined units/parameters
  Configurable::PrintConfig();
}

////////////////////////////////////////////////////////////////////////////////
///
void MedLinac::DefaultConfig(const std::string &unit) {

  // Volume name
  if (unit.compare("Name") == 0)
    thisConfig()->SetValue(unit, std::string("MedLinac"));

  // describe me.
  if (unit.compare("ionizationChamberThicknessP") == 0)
    thisConfig()->SetValue(unit, G4double(50.)); // [um]
  if (unit.compare("ionizationChamberThicknessW") == 0)
    thisConfig()->SetValue(unit, G4double(50.)); // [um]

}

////////////////////////////////////////////////////////////////////////////////
///
void MedLinac::WriteInfo() {
  G4cout << "\n\n\tnominal beam energy: " << configSvc()->GetValue<G4int>("RunSvc", "idEnergy") << G4endl;
  G4cout << "\tJaw X aperture: 1) "
         << configSvc()->GetValue<G4double>("GeoSvc", "jaw1XAperture") / cm << "[cm]\t2) "
         << configSvc()->GetValue<G4double>("GeoSvc", "jaw2XAperture") / cm << " [cm]" << G4endl;
  G4cout << "\tJaw Y aperture: 1) "
         << configSvc()->GetValue<G4double>("GeoSvc", "jaw1YAperture") / cm << "[cm]\t2) "
         << configSvc()->GetValue<G4double>("GeoSvc", "jaw2YAperture") / cm << " [cm]\n" << G4endl;
}

////////////////////////////////////////////////////////////////////////////////
///
void MedLinac::Destroy() {
  for (auto &ivolume : m_physicalVolume) {
    if (ivolume.second) {
      delete ivolume.second;
      ivolume.second = nullptr;
    }
  }
  delete m_PVvacuumWorld;
  m_PVvacuumWorld = nullptr;
}

////////////////////////////////////////////////////////////////////////////////
///
void MedLinac::Construct(G4VPhysicalVolume *parentWorld) {
  m_parentPV = parentWorld;
  // NOTE: The additional head internal world to keep target and primary collimator within
  // the vacuum environment, all other elements are within the air as inherited
  // from the mother volume.
  BeWindow();
  createVacuumWorld();
  target();
  primaryCollimator();
  secondaryCollimator();
  ionizationChamber();
  flatteningFilter();
  //mirror();
  MLC();
  Jaw1X();
  Jaw2X();
  Jaw1Y();
  Jaw2Y();

  // create pre-defined Phsp planes, if requested
  if (configSvc()->GetValue<G4bool>("RunSvc", "SavePhSp"))
    createPhspPlanes();

  cover();
}

////////////////////////////////////////////////////////////////////////////////
///
void MedLinac::Reset() {
  m_leavesA.clear();
  m_leavesB.clear();
}

////////////////////////////////////////////////////////////////////////////////
///
bool MedLinac::createVacuumWorld() {

  auto Vacuum = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_Galactic");

  // NOTE: The vacuum world is a tube contains p.collimators and the target,
  //       which is spread up to the BeWindow:
  //       - radius == radii of the collimators
  //       - top Z (on the target side) == top Z of the Upper Collimator
  //       - bottom Z (on the isocentre side) == top Z of the BeWindow

  // import objects/variables for the world size calculation:
  if (m_physicalVolume.find("BeWindow") == m_physicalVolume.end()) {
    G4Exception("MedLinac", "createVacuumWorld::Null pointer reached", FatalException,
                "[INFO]:: The VacuumWorld depends on the BeWindow size and position which hasn't been created."
                "\nPlease call the MedLinac::BeWindow() before.");
  }
  auto BeWindowLV = m_physicalVolume["BeWindow"]->GetLogicalVolume();
  auto BeWindow = dynamic_cast< G4Tubs *>(BeWindowLV->GetSolid());
  auto BeWindowTranslation = m_physicalVolume["BeWindow"]->GetTranslation();

  G4double innerRadius = 0. * cm;
  G4double outerRadius = 8. * cm;

  // NOTE: we are in head origin system
  G4double upperCollimatorPosZinHeadWorld = -1. * cm;
  G4double upperCollimatorHalfSize = 3. * cm;
  G4double worldTopToHeadOrigin = abs(upperCollimatorPosZinHeadWorld) + upperCollimatorHalfSize;
  G4double worldBottomToHeadOrigin = abs(BeWindowTranslation.z()) - BeWindow->GetZHalfLength();

  G4double halfLengthOfTheWorld = (worldTopToHeadOrigin + worldBottomToHeadOrigin) / 2.;
  G4double worldPosZinHeadWorld = halfLengthOfTheWorld - worldTopToHeadOrigin;

  auto vacuumWorldTube = new G4Tubs("vacuumWorldTube", innerRadius, outerRadius,
                                    halfLengthOfTheWorld * mm, 0. * deg, 360. * deg);

  auto vacuumWorldLV = new G4LogicalVolume(vacuumWorldTube, Vacuum.get(), "vacuumWorldLV", 0, 0, 0);

  m_PVvacuumWorld = new G4PVPlacement(0, G4ThreeVector(0. * cm, 0. * cm, worldPosZinHeadWorld), "vacuumWorld",
                                      vacuumWorldLV, m_parentPV, false, 0);

  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
bool MedLinac::target() {
  if (!m_PVvacuumWorld) {
    G4Exception("MedLinac", "target::Null pointer reached", FatalException,
                "[INFO]:: The target is installed within the VacuumWorld which hasn't been created."
                "\nDid you forget to call createVacuumWorld()?");
  }
  // original values of target positioning are given in head world,
  // hence the translation from the HeadWorld to the VacuumWorld is needed
  G4double shiftZ = m_PVvacuumWorld->GetTranslation().z();

  switch (configSvc()->GetValue<G4int>("RunSvc", "idEnergy")) {
    case 6:
      //    materials
      auto Cu = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_Cu");
      auto W = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_W");

      //    colors
      G4Colour cyan(0.0, 1.0, 1.0);
      G4Colour magenta(1.0, 0.0, 1.0);

      //    volumes
      //    beam line along z axis
      //------------------------target 6MV------------------------
      G4double targetADim_x = 0.6 * cm;
      G4double targetADim_y = 0.6 * cm;
      G4double targetADim_z = 0.04445 * cm;

      auto targetA_box = new G4Box("targetA_box", targetADim_x, targetADim_y, targetADim_z);
      auto targetA_log = new G4LogicalVolume(targetA_box, W.get(), "targetA_log", 0, 0, 0);

      G4double targetAPos_z = 0.20055 * cm - shiftZ;  // original value is given in head world
      m_physicalVolume["targetA"] = new G4PVPlacement(0, G4ThreeVector(0. * cm, 0. * cm, targetAPos_z), "targetA",
                                                      targetA_log, m_PVvacuumWorld, false, 0);

      G4double targetBDim_x = 0.6 * cm;
      G4double targetBDim_y = 0.6 * cm;
      G4double targetBDim_z = 0.07874 * cm;
      auto targetB_box = new G4Box("targetB_box", targetBDim_x, targetBDim_y, targetBDim_z);
      auto targetB_log = new G4LogicalVolume(targetB_box, Cu.get(), "targetB_log", 0, 0, 0);

      G4double targetBPos_z = 0.07736 * cm - shiftZ; // original value is given in head world
      m_physicalVolume["targetB"] = new G4PVPlacement(0, G4ThreeVector(0. * cm, 0. * cm, targetBPos_z), "targetB",
                                                      targetB_log, m_PVvacuumWorld, false, 0);

      // ***********  REGIONS for CUTS

      auto regVol = new G4Region("targetR");
      auto cuts = new G4ProductionCuts;
      cuts->SetProductionCut(0.1 * cm);
      regVol->SetProductionCuts(cuts);

      targetA_log->SetRegion(regVol);
      regVol->AddRootLogicalVolume(targetA_log);
      targetB_log->SetRegion(regVol);
      regVol->AddRootLogicalVolume(targetB_log);

      return true;
      break;
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
///
bool MedLinac::primaryCollimator() {
  bool bCreated = false;

  if (!m_PVvacuumWorld) {
    G4Exception("MedLinac", "primaryCollimator::Null pointer reached", FatalException,
                "[INFO]:: The primary collimator is installed within the VacuumWorld which hasn't been created."
                "\nDid you forget to call createVacuumWorld()?");
  }

  // translation from the HeadWorld to the VacuumWorld
  //G4double shiftZ = m_PVvacuumWorld->GetTranslation().z();

  // import objects/variables for the positioning and size calculation:
  if (m_physicalVolume.find("targetA") == m_physicalVolume.end()) {
    G4Exception("MedLinac", "primaryCollimator::Null pointer reached", FatalException,
                "[INFO]:: The primary collimator depends on the target position, which hasn't been created."
                "\nPlease call the MedLinac::target() before.");
  }

  auto motherWorldLV = m_PVvacuumWorld->GetLogicalVolume();
  auto motherSolid = dynamic_cast< G4Tubs *>(motherWorldLV->GetSolid());
  auto targetLV = m_physicalVolume["targetA"]->GetLogicalVolume();
  auto target = dynamic_cast< G4Box *>(targetLV->GetSolid());
  auto targetTranslation = m_physicalVolume["targetA"]->GetTranslation();

  //    materials
  auto W = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_W");

  //-------------------- the first collimator upper----------------
  G4double innerRadius = 1.0 * cm;
  G4double outerRadius = motherSolid->GetOuterRadius();
  G4double upperCollimatorLength = 6.0 * cm;  // == 2*upperCollimatorHalfSize from createVacuumWorld() !
  auto UpperCollimator = new G4Tubs("UpperCollimator", innerRadius, outerRadius,
                                       upperCollimatorLength / 2., 0. * deg, 360. * deg);
  auto UpperCollimator_log = new G4LogicalVolume(UpperCollimator, W.get(), "UpperCollimator_log", 0, 0, 0);

  // already in local coordinate system
  G4double upperCollimatorPosZ = -motherSolid->GetZHalfLength() + upperCollimatorLength / 2.;
  m_physicalVolume["UpperCollimator"] = new G4PVPlacement(0, G4ThreeVector(0., 0., upperCollimatorPosZ),
                                                          "UpperCollimator", UpperCollimator_log, m_PVvacuumWorld,
                                                          false, 0);

  //-------------------- the first collimator lower----------------
  G4double maxFieldSize = 40 * cm; // TODO(?): this can be defined as configurable

  G4double lowerCollimatorLength = 6.2 * cm;
  G4double upperLowerDistance = 1.1 * cm;     // Distance between upper and lower collimators
  G4double lowerCollimatorPosZ = upperCollimatorPosZ
                                 + upperCollimatorLength / 2.
                                 + upperLowerDistance
                                 + lowerCollimatorLength / 2.;

  // radius of circle enclosing max field at isocenter
  G4double apertureProjectedOnIsocenter = maxFieldSize * sqrt(2.) / 2.0;

  G4double defaultSSD = 100. * cm;

  G4double targetToCollimatorFrontSide = abs(lowerCollimatorPosZ)
                                         - lowerCollimatorLength / 2.
                                         + abs(targetTranslation.z())
                                         + target->GetZHalfLength();

  G4double targetToCollimatorBackSide = targetToCollimatorFrontSide + lowerCollimatorLength;

  // inner radius at lowest Z  - on the target side
  G4double innerRadius1 = (targetToCollimatorFrontSide / defaultSSD) * apertureProjectedOnIsocenter;

  // inner radius at highest Z - on the isocenter side
  G4double innerRadius2 = (targetToCollimatorBackSide / defaultSSD) * apertureProjectedOnIsocenter;

  auto LowerCollimator = new G4Cons("LowerCollimator", innerRadius1, outerRadius, innerRadius2, outerRadius,
                                    lowerCollimatorLength / 2., 0. * deg, 360. * deg);

  auto LowerCollimator_log = new G4LogicalVolume(LowerCollimator, W.get(), "LowerCollimator_log", 0, 0, 0);

  m_physicalVolume["LowerCollimator"] = new G4PVPlacement(0, G4ThreeVector(0. * cm, 0. * cm, lowerCollimatorPosZ),
                                                          "LowerCollimator", LowerCollimator_log, m_PVvacuumWorld,
                                                          false, 0);

  // ***********  REGIONS for CUTS

  G4Region *regVol;
  regVol = new G4Region("PrymCollR");
  auto cuts = new G4ProductionCuts;
  cuts->SetProductionCut(0.1 * cm);
  regVol->SetProductionCuts(cuts);

  LowerCollimator_log->SetRegion(regVol);
  regVol->AddRootLogicalVolume(LowerCollimator_log);

  UpperCollimator_log->SetRegion(regVol);
  regVol->AddRootLogicalVolume(UpperCollimator_log);

  bCreated = true;
  return bCreated;
}

////////////////////////////////////////////////////////////////////////////////
///
bool MedLinac::secondaryCollimator() {
  bool bCreated = false;

  //    materials
  auto W = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_W");

  G4double maxFieldSize = 40 * cm;

  G4double collimatorLength = 5 * cm;
  G4double collimatorPosZ = 18.75 * cm;

  // radius of circle enclosing max field at isocenter
  G4double apertureProjectedOnIsocenter = maxFieldSize * sqrt(2.) / 2.0;

  G4double defaultSSD = 100. * cm;

  G4double targetToCollimatorFrontSide = collimatorPosZ - collimatorLength / 2.;
  G4double targetToCollimatorBackSide = collimatorPosZ + collimatorLength / 2.;

  G4double outerRadius = 25. * cm;

  // inner radius at lowest Z  - on the target side
  G4double innerRadius1 = (targetToCollimatorFrontSide / defaultSSD) * apertureProjectedOnIsocenter;

  // inner radius at highest Z - on the isocenter side
  G4double innerRadius2 = (targetToCollimatorBackSide / defaultSSD) * apertureProjectedOnIsocenter;

  auto Collimator = new G4Cons("SecondaryCollimator", innerRadius1, outerRadius, innerRadius2, outerRadius,
                               collimatorLength / 2., 0. * deg, 360. * deg);

  auto Collimator_log = new G4LogicalVolume(Collimator, W.get(), "SecondaryCollimator_log", 0, 0, 0);

  m_physicalVolume["SecondaryCollimator"] = new G4PVPlacement(0, G4ThreeVector(0. * cm, 0. * cm, collimatorPosZ),
                                                              "SecondaryCollimator", Collimator_log, m_parentPV, false, 0);

  bCreated = true;
  return bCreated;
}

////////////////////////////////////////////////////////////////////////////////
///
bool MedLinac::cover() {
  bool bCreated = false;

  //    materials
  auto W = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_W");

  // import objects/variables for the positioning and size calculation:
  if (m_physicalVolume.find("SecondaryCollimator") == m_physicalVolume.end()) {
    G4Exception("MedLinac", "SecondaryCollimator::Null pointer reached", FatalException,
                "[INFO]:: The head cover depends on the secondary collimator position, which hasn't been created."
                "\nPlease call the MedLinac::secondaryCollimator() before.");
  }

  auto secondaryColLV = m_physicalVolume["SecondaryCollimator"]->GetLogicalVolume();
  auto secondaryColSolid = dynamic_cast<G4Cons *>(secondaryColLV->GetSolid());
  auto secondaryColTranslation = m_physicalVolume["SecondaryCollimator"]->GetTranslation();

  // cover upper the secondary collimator
  G4double coverLength = 23.5 * cm;
  G4double coverPosZ = secondaryColTranslation.z()
                       - secondaryColSolid->GetZHalfLength()
                       - coverLength / 2.;

  G4double innerRadius = 9. * cm;
  G4double outerRadius = 14. * cm;

  auto CoverUpper = new G4Cons("HeadCoverUpper", innerRadius, outerRadius, innerRadius, outerRadius,
                               coverLength / 2., 0. * deg, 360. * deg);

  auto CoverUpper_log = new G4LogicalVolume(CoverUpper, W.get(), "HeadCoverUpper_log", 0, 0, 0);

  m_physicalVolume["CoverUpper"] = new G4PVPlacement(0, G4ThreeVector(0. * cm, 0. * cm, coverPosZ),
                                                     "HeadCoverUpper", CoverUpper_log, m_parentPV, false, 0);

  // cover lower the secondary collimator
  coverLength = 20 * cm;
  coverPosZ = secondaryColTranslation.z()
              + secondaryColSolid->GetZHalfLength()
              + coverLength / 2.;
  innerRadius = 20. * cm;
  outerRadius = 25. * cm;

  auto CoverLower = new G4Cons("HeadCoverLower", innerRadius, outerRadius, innerRadius, outerRadius,
                               coverLength / 2., 0. * deg, 360. * deg);

  auto CoverLower_log = new G4LogicalVolume(CoverLower, W.get(), "HeadCoverLower_log", 0, 0, 0);

  m_physicalVolume["CoverLower"] = new G4PVPlacement(0, G4ThreeVector(0. * cm, 0. * cm, coverPosZ),
                                                     "HeadCoverLower", CoverLower_log, m_parentPV, false, 0);

  bCreated = true;
  return bCreated;
}

////////////////////////////////////////////////////////////////////////////////
///
bool MedLinac::BeWindow() {
  bool bCreated = false;

  //    materials
  auto Be = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_Be");

  auto regVol = new G4Region("BeWindow");
  auto cuts = new G4ProductionCuts;
  cuts->SetProductionCut(0.1 * cm);
  regVol->SetProductionCuts(cuts);

  auto BeWTube = new G4Tubs("BeWindowTube", 0., 36. * mm, 0.2 * mm, 0. * deg, 360. * deg);
  auto BeWTubeLV = new G4LogicalVolume(BeWTube, Be.get(), "BeWTubeLV", 0, 0, 0);
  m_physicalVolume["BeWindow"] = new G4PVPlacement(0, G4ThreeVector(0., 0., 100. * mm), "BeWTubePV", BeWTubeLV, m_parentPV,
                                                   false, 0);
  BeWTubeLV->SetRegion(regVol);
  regVol->AddRootLogicalVolume(BeWTubeLV);

  bCreated = true;
  return bCreated;
}

////////////////////////////////////////////////////////////////////////////////
///
bool MedLinac::flatteningFilter() {
  switch (configSvc()->GetValue<G4int>("RunSvc", "idEnergy")) {
    case 6:
      G4double z0, h0;
      G4ThreeVector centre, halSize;
      auto Cu = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_Cu");
      // Region for cuts
      G4Region *regVol;
      regVol = new G4Region("flatfilterR");
      auto cuts = new G4ProductionCuts;
      cuts->SetProductionCut(0.5 * cm);
      regVol->SetProductionCuts(cuts);

      // one
      z0 = 130.0 * mm;
      h0 = 5.0 / 2. * cm;
      centre.set(0., 0., z0);
      auto FFL1A_1Cone = new G4Cons("FFL1A_1", 0. * cm, 0.3 * cm, 0. * cm, 5. * cm, h0, 0. * deg, 360. * deg);
      auto FFL1A_1LV = new G4LogicalVolume(FFL1A_1Cone, Cu.get(), "FFL1A_1LV", 0, 0, 0);
      m_physicalVolume["FFL1A_1PV"] = new G4PVPlacement(0, centre, "FFL1A_1PV", FFL1A_1LV, m_parentPV, false, 0);

      // two
      z0 += h0;
      h0 = 0.081 / 2. * cm;
      z0 += h0;
      centre.setZ(z0);
      z0 += h0;
      auto FFL2_1Tube = new G4Tubs("FFL6_1", 0. * cm, 2.5 * cm, h0, 0. * deg, 360. * deg);
      auto FFL2_1LV = new G4LogicalVolume(FFL2_1Tube, Cu.get(), "FFL2_1LV", 0, 0, 0);
      m_physicalVolume["FFL2_1PV"] = new G4PVPlacement(0, centre, "FFL2_1PV", FFL2_1LV, m_parentPV, false, 0);


      FFL1A_1LV->SetRegion(regVol);
      FFL2_1LV->SetRegion(regVol);

      regVol->AddRootLogicalVolume(FFL1A_1LV);
      regVol->AddRootLogicalVolume(FFL2_1LV);
      return true;
      break;
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
///
bool MedLinac::ionizationChamber() {
  bool bCreated = false;

  auto material = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_KAPTON");

  // Region for cuts
  auto regVol = new G4Region("ionizationChamber");
  auto cuts = new G4ProductionCuts;
  cuts->SetProductionCut(0.1 * cm);
  regVol->SetProductionCuts(cuts);
  G4double innerRadius = 0.;
  G4double outerRadius = 5.08 * cm;

  auto thicknessP = thisConfig()->GetValue<G4double>("ionizationChamberThicknessP") * um;
  auto thicknessW = thisConfig()->GetValue<G4double>("ionizationChamberThicknessW") * um;

  auto ICTubeW =
      new G4Tubs("ionizationChamberTube", innerRadius, outerRadius, thicknessW / 2., 0. * deg, 360. * deg);
  auto ICTubeP =
      new G4Tubs("ionizationChamberTube", innerRadius, outerRadius, thicknessP / 2., 0. * deg, 360. * deg);

  G4ThreeVector centre;
  // W1
  centre.set(0., 0., 157. * mm);
  auto PCUTubeW1LV = new G4LogicalVolume(ICTubeW, material.get(), "ionizationChamberTubeW1LV", 0, 0, 0);
  m_physicalVolume["ionChamberW1"] = new G4PVPlacement(0, centre, "ionizationChamberTubeW1PV", PCUTubeW1LV, m_parentPV,
                                                       false, 0);
  PCUTubeW1LV->SetRegion(regVol);
  regVol->AddRootLogicalVolume(PCUTubeW1LV);

  // P1
  centre.set(0., 0., 158. * mm);
  auto *PCUTubeP1LV = new G4LogicalVolume(ICTubeP, material.get(), "ionizationChamberTubeP1LV", 0, 0, 0);
  m_physicalVolume["ionChamberP1"] = new G4PVPlacement(0, centre, "ionizationChamberTubeP1PV", PCUTubeP1LV, m_parentPV,
                                                       false, 0);
  PCUTubeP1LV->SetRegion(regVol);
  regVol->AddRootLogicalVolume(PCUTubeP1LV);

  // W2
  centre.set(0., 0., 159. * mm);
  G4LogicalVolume *PCUTubeW2LV = new G4LogicalVolume(ICTubeW, material.get(), "ionizationChamberTubeW2LV", 0, 0, 0);
  m_physicalVolume["ionChamberW2"] = new G4PVPlacement(0, centre, "ionizationChamberTubeW2PV", PCUTubeW2LV, m_parentPV,
                                                       false, 0);
  PCUTubeW2LV->SetRegion(regVol);
  regVol->AddRootLogicalVolume(PCUTubeW2LV);

  // P2
  centre.set(0., 0., 160. * mm);
  auto PCUTubeP2LV = new G4LogicalVolume(ICTubeP, material.get(), "ionizationChamberTubeP2LV", 0, 0, 0);
  m_physicalVolume["ionChamberP2"] = new G4PVPlacement(0, centre, "ionizationChamberTubeP2PV", PCUTubeP2LV, m_parentPV,
                                                       false, 0);
  PCUTubeP2LV->SetRegion(regVol);
  regVol->AddRootLogicalVolume(PCUTubeP2LV);

  // W3
  centre.set(0., 0., 161. * mm);
  auto PCUTubeW3LV = new G4LogicalVolume(ICTubeW, material.get(), "ionizationChamberTubeW3LV", 0, 0, 0);
  m_physicalVolume["ionChamberW3"] = new G4PVPlacement(0, centre, "ionizationChamberTubeW3PV", PCUTubeW3LV, m_parentPV,
                                                       false, 0);
  PCUTubeW3LV->SetRegion(regVol);
  regVol->AddRootLogicalVolume(PCUTubeW3LV);

  // P3
  centre.set(0., 0., 162. * mm);
  auto PCUTubeP3LV = new G4LogicalVolume(ICTubeP, material.get(), "ionizationChamberTubeP3LV", 0, 0, 0);
  m_physicalVolume["ionChamberP3"] = new G4PVPlacement(0, centre, "ionizationChamberTubeP3PV", PCUTubeP3LV, m_parentPV,
                                                       false, 0);

  bCreated = true;
  return bCreated;
}

////////////////////////////////////////////////////////////////////////////////
///
bool MedLinac::mirror() {
  bool bCreated = false;
  auto MYLAR = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_MYLAR");
  // Region for cuts
  G4Region *regVol;
  regVol = new G4Region("Mirror");
  auto cuts = new G4ProductionCuts;
  cuts->SetProductionCut(0.1 * cm);
  regVol->SetProductionCuts(cuts);

  auto MirrorTube = new G4Tubs("MirrorTube", 0., 63. * mm, .5 * mm, 0. * deg, 360. * deg);
  G4LogicalVolume *MirrorTubeLV = new G4LogicalVolume(MirrorTube, MYLAR.get(), "MirrorTubeLV", 0, 0, 0);
  auto cRotation = new G4RotationMatrix();
  cRotation->rotateY(12.0 * deg);
  m_physicalVolume["Mirror"] =
      new G4PVPlacement(cRotation, G4ThreeVector(0., 0., 175. * mm), "MirrorTubePV", MirrorTubeLV, m_parentPV, false, 0);

  bCreated = true;
  return bCreated;
}

////////////////////////////////////////////////////////////////////////////////
///
void MedLinac::SetJawAperture(G4int idJaw, G4ThreeVector &centre, G4ThreeVector halfSize,
                                      G4RotationMatrix *cRotation) {
  using namespace std;
  G4double theta, x, y, z, dx, dy, dz, aperture = 0.;
  x = centre.getX();
  y = centre.getY();
  z = centre.getZ();
  if (idJaw == 1) aperture = configSvc()->GetValue<G4double>("GeoSvc", "jaw1XAperture");
  if (idJaw == 2) aperture = configSvc()->GetValue<G4double>("GeoSvc", "jaw2XAperture");
  if (idJaw == 3) aperture = configSvc()->GetValue<G4double>("GeoSvc", "jaw1YAperture");
  if (idJaw == 4) aperture = configSvc()->GetValue<G4double>("GeoSvc", "jaw2YAperture");

  theta = fabs(atan(aperture / configSvc()->GetValue<G4double>("GeoSvc", "isoCentre")));
  dx = halfSize.getX();
  dy = halfSize.getY();
  dz = halfSize.getZ();

  switch (idJaw) {
    case 1:  // idJaw1XV2100:
      centre.set(z * sin(theta) + dx * cos(theta), y, z * cos(theta) - dx * sin(theta));
      cRotation->rotateY(-theta);
      halfSize.set(fabs(dx * cos(theta) + dz * sin(theta)), fabs(dy), fabs(dz * cos(theta) + dx * sin(theta)));
      break;
    case 2:  // idJaw2XV2100:
      centre.set(-(z * sin(theta) + dx * cos(theta)), y, z * cos(theta) - dx * sin(theta));
      cRotation->rotateY(theta);
      halfSize.set(fabs(dx * cos(theta) + dz * sin(theta)), fabs(dy), fabs(dz * cos(theta) + dx * sin(theta)));
      break;
    case 3:  // idJaw1YV2100:
      centre.set(x, z * sin(theta) + dy * cos(theta), z * cos(theta) - dy * sin(theta));
      cRotation->rotateX(theta);
      halfSize.set(fabs(dx), fabs(dy * cos(theta) + dz * sin(theta)), fabs(dz * cos(theta) + dy * sin(theta)));
      break;
    case 4:  // idJaw2YV2100:
      centre.set(x, -(z * sin(theta) + dy * cos(theta)), z * cos(theta) - dy * sin(theta));
      cRotation->rotateX(-theta);
      halfSize.set(fabs(dx), fabs(dy * cos(theta) + dz * sin(theta)), fabs(dz * cos(theta) + dy * sin(theta)));
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
bool MedLinac::Jaw1X() {
  auto tungsten = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_W");
  G4String name = "Jaws1X";

  auto cRotation = new G4RotationMatrix();
  G4ThreeVector centre(0., 0., (320. + 80. / 2.) * mm);
  G4ThreeVector halfSize(45. * mm, 93. * mm, 78. / 2. * mm);
  auto box = new G4Box(name + "Box", halfSize.getX(), halfSize.getY(), halfSize.getZ());
  auto logVol = new G4LogicalVolume(box, tungsten.get(), name + "LV", 0, 0, 0);
  SetJawAperture(1, centre, halfSize, cRotation);
  m_physicalVolume[name] = new G4PVPlacement(cRotation, centre, name + "PV", logVol, m_parentPV, false, 0);

  // Region for cuts
  auto regVol = new G4Region(name + "R");
  auto *cuts = new G4ProductionCuts;
  cuts->SetProductionCut(2. * cm);
  regVol->SetProductionCuts(cuts);
  logVol->SetRegion(regVol);
  regVol->AddRootLogicalVolume(logVol);

  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
bool MedLinac::Jaw2X() {
  auto tungsten = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_W");
  G4String name = "Jaws2X";

  auto cRotation = new G4RotationMatrix();
  G4ThreeVector centre(0., 0., (320. + 80. / 2.) * mm);
  G4ThreeVector halfSize(45. * mm, 93. * mm, 78. / 2. * mm);
  auto box = new G4Box(name + "Box", halfSize.getX(), halfSize.getY(), halfSize.getZ());
  auto logVol = new G4LogicalVolume(box, tungsten.get(), name + "LV", 0, 0, 0);
  SetJawAperture(2, centre, halfSize, cRotation);
  m_physicalVolume[name] = new G4PVPlacement(cRotation, centre, name + "PV", logVol, m_parentPV, false, 0);

  // Region for cuts
  auto regVol = new G4Region(name + "R");
  auto cuts = new G4ProductionCuts;
  cuts->SetProductionCut(2. * cm);
  regVol->SetProductionCuts(cuts);
  logVol->SetRegion(regVol);
  regVol->AddRootLogicalVolume(logVol);

  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
bool MedLinac::Jaw1Y() {
  auto tungsten = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_W");
  G4String name = "Jaws1Y";

  auto cRotation = new G4RotationMatrix();
  G4ThreeVector centre(0., 0., (230. + 80. / 2.) * mm);
  G4ThreeVector halfSize(93. * mm, 35. * mm, 78. / 2. * mm);
  auto box = new G4Box(name + "Box", halfSize.getX(), halfSize.getY(), halfSize.getZ());
  auto logVol = new G4LogicalVolume(box, tungsten.get(), name + "LV", 0, 0, 0);
  SetJawAperture(3, centre, halfSize, cRotation);
  m_physicalVolume[name] = new G4PVPlacement(cRotation, centre, name + "PV", logVol, m_parentPV, false, 0);

  // Region for cuts
  auto regVol = new G4Region(name + "R");
  auto cuts = new G4ProductionCuts;
  cuts->SetProductionCut(2. * cm);
  regVol->SetProductionCuts(cuts);
  logVol->SetRegion(regVol);
  regVol->AddRootLogicalVolume(logVol);

  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
bool MedLinac::Jaw2Y() {
  auto tungsten = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_W");
  G4String name = "Jaws2Y";

  auto cRotation = new G4RotationMatrix();
  G4ThreeVector centre(0., 0., (230. + 80. / 2.) * mm);
  G4ThreeVector halfSize(93. * mm, 35. * mm, 78. / 2. * mm);
  auto box = new G4Box(name + "Box", halfSize.getX(), halfSize.getY(), halfSize.getZ());
  auto logVol = new G4LogicalVolume(box, tungsten.get(), name + "LV", 0, 0, 0);
  SetJawAperture(4, centre, halfSize, cRotation);
  m_physicalVolume[name] = new G4PVPlacement(cRotation, centre, name + "PV", logVol, m_parentPV, false, 0);

  // Region for cuts
  auto regVol = new G4Region(name + "R");
  auto cuts = new G4ProductionCuts;
  cuts->SetProductionCut(2. * cm);
  regVol->SetProductionCuts(cuts);
  logVol->SetRegion(regVol);
  regVol->AddRootLogicalVolume(logVol);

  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
bool MedLinac::MLC() {
  // NOTE: There is two types of leaves
  // - "thin" - in the central region: 0.25 cm x 40 leafs
  // - "fat"  - in the side region: 2 x 0.5 cm x 10 leafs
  // Total number of leafs: 60 per side

  auto Fe = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_Fe");

  // Region for cuts
  auto regVol = new G4Region("MLCR");
  auto cuts = new G4ProductionCuts;
  cuts->SetProductionCut(1.0 * cm);
  regVol->SetProductionCuts(cuts);

  G4double leafWidthCentral = 0.25 * cm;
  G4double leafWidthSide = 0.5 * cm;
  G4double leafHeight = 9. * cm;  // along the beam direction
  G4double leafLength = 25. * cm;  // perpendicular to beam direction

  // single leaf
  auto boxLeafCentral = new G4Box("LeafCentralBox", leafWidthCentral / 2., leafLength / 2., leafHeight / 2.);
  auto boxLeafSide = new G4Box("LeafSideBox", leafWidthSide / 2., leafLength / 2., leafHeight / 2.);

  auto leafCentralLV = new G4LogicalVolume(boxLeafCentral, Fe.get(), "leafCentral", 0, 0, 0);
  auto leafSideLV = new G4LogicalVolume(boxLeafSide, Fe.get(), "leafSide", 0, 0, 0);

  leafCentralLV->SetRegion(regVol);
  regVol->AddRootLogicalVolume(leafCentralLV);

  leafSideLV->SetRegion(regVol);
  regVol->AddRootLogicalVolume(leafSideLV);

  G4LogicalVolume *leafLV;
  G4String PVname;
  G4double mlcPosZ = 46.5 * cm;
  G4double mlcStartPosX = -20 * leafWidthCentral - 10 * leafWidthSide;
  G4double mlcClosedPosY = leafLength / 2.;
  G4ThreeVector centreCurrent(mlcStartPosX, 0., mlcPosZ);

  for (int i = 1; i <= 60; i++) {
    if (i <= 10 || i > 50) {
      leafLV = leafSideLV;
      centreCurrent.setX(centreCurrent.getX() + leafWidthSide);
    } else {
      leafLV = leafCentralLV;
      centreCurrent.setX(centreCurrent.getX() + leafWidthCentral);
    }

    // A side
    PVname = leafLV->GetName() + "A" + G4String(std::to_string(i));
    centreCurrent.setY(-mlcClosedPosY - m_leavesA[i - 1]);
    m_physicalVolume[PVname] = new G4PVPlacement(0, centreCurrent, PVname, leafSideLV, m_parentPV, false, i);

    // B side
    PVname = leafLV->GetName() + "B" + G4String(std::to_string(i));
    centreCurrent.setY(mlcClosedPosY + m_leavesB[i - 1]);
    m_physicalVolume[PVname] = new G4PVPlacement(0, centreCurrent, PVname, leafSideLV, m_parentPV, false, i);
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
bool MedLinac::createPhspPlanes() {

  auto headPhspPositions = configSvc()->GetValue<std::vector<G4double> *>("GeoSvc", "SavePhSpHead");
  auto headToIsocentre = configSvc()->GetValue<G4double>("GeoSvc", "isoCentre"); // refer to global system

  auto headWorldTranslation = m_parentPV->GetTranslation();
  auto headWorldLV = m_parentPV->GetLogicalVolume();
  auto motherSolidBox = dynamic_cast< G4Box *>(headWorldLV->GetSolid());

  auto vacuumWorldTranslation = m_PVvacuumWorld->GetTranslation();
  auto vacuumWorldLV = m_PVvacuumWorld->GetLogicalVolume();
  auto motherSolidTub = dynamic_cast< G4Tubs *>(vacuumWorldLV->GetSolid());

  // Visibility
  auto simplePhspVisAtt = new G4VisAttributes(G4Colour::Blue());
  simplePhspVisAtt->SetVisibility(true);

  // Materials
  auto Air = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "Usr_G4AIR20C");
  auto Vacuum = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_Galactic");

  G4double secondaryCollimatorTopPosZ = (18.75 + 2.5) * cm;
  G4double coverUpperInnerRadius = 9. * cm;
  G4double coverLowerInnerRadius = 20. * cm;

  // create actual phsp planes
  G4VPhysicalVolume *phspMother{nullptr};
  G4LogicalVolume *phspLV{nullptr};
  G4bool isTube_1_LVcreated{false}; // inside head and vacuum world | vacuum world limit
  G4bool isTube_2_LVcreated{false}; // below vacuum world & above secondary collimator | upper cover limit
  G4double iPhspZPosLocal;
  for (auto &iPhspZPos : *headPhspPositions) {
    iPhspZPosLocal = iPhspZPos +
                     headToIsocentre; // phase space volume should be centered around the user requested plane (to set correctly preStepPoint)
    if (iPhspZPos > headWorldTranslation.z() - motherSolidBox->GetZHalfLength() &&
        iPhspZPos < headWorldTranslation.z() + motherSolidBox->GetZHalfLength()) { // inside the head world

      if (iPhspZPosLocal > vacuumWorldTranslation.z() - motherSolidTub->GetZHalfLength() &&
          iPhspZPosLocal < vacuumWorldTranslation.z() + motherSolidTub->GetZHalfLength()) { // inside vacuum world
        if (!isTube_1_LVcreated) {
          G4double outerRadius = motherSolidTub->GetOuterRadius();
          auto phspTub = new G4Tubs("phspTub", 0. * cm, outerRadius, 1. * um, 0. * deg, 360. * deg);
          phspLV = new G4LogicalVolume(phspTub, Vacuum.get(), "phspBoxInHeadLV" + svc::to_string(iPhspZPos), 0, 0, 0);
          iPhspZPosLocal -= vacuumWorldTranslation.z(); // Head -> Vacuum world
          phspMother = m_PVvacuumWorld;
          isTube_1_LVcreated = true;
        } // !isTube_1_LVcreated
        else {
          G4String description = "You have requested to save the second phsp within the head vacuum world,";
          description += " Verify the implementation (->GeoSvc::ParseSavePhspPlaneRequest)";
          G4Exception("MedLinac", "createPhspPlanes", FatalErrorInArgument, description.data());
        } // isTube_1_LVcreated
      }   // inside vacuum world
      else if (iPhspZPosLocal > vacuumWorldTranslation.z() + motherSolidTub->GetZHalfLength() &&
               iPhspZPosLocal < secondaryCollimatorTopPosZ) { // above secondary collimator
        if (!isTube_2_LVcreated) { // create single LV instance of given size
          // the phsp plane is limited with the cover solid
          auto phspTub = new G4Tubs("phspTub", 0. * cm, coverUpperInnerRadius, 1. * um, 0. * deg, 360. * deg);
          phspLV = new G4LogicalVolume(phspTub, Air.get(), "phspBoxInHeadLV" + svc::to_string(iPhspZPos), 0, 0, 0);
          isTube_2_LVcreated = true;
        } // isTube_2_LVcreated
        phspMother = m_parentPV;
      } else if (iPhspZPosLocal > secondaryCollimatorTopPosZ) { // below secondary collimator
        auto phspTub = new G4Tubs("phspTub", 0. * cm, coverLowerInnerRadius, 1. * um, 0. * deg, 360. * deg);
        phspLV = new G4LogicalVolume(phspTub, Air.get(), "phspBoxInHeadLV" + svc::to_string(iPhspZPos), 0, 0, 0);
        phspMother = m_parentPV;
      }   // inside head world but not inside vacuum
    }     // inside the head world
    else {
      G4String description = "You have requested to save the phsp within the head world, however";
      description += " the wrong positioning is caught.";
      description += " Verify the implementation (->GeoSvc::ParseSavePhspPlaneRequest)";
      G4Exception("MedLinac", "createPhspPlanes", FatalErrorInArgument, description.data());
    }  // !inside the head world

    phspLV->SetVisAttributes(simplePhspVisAtt);

    // define the placement of the phsp
    G4cout << "[INFO]:: LinacGeometry:: adding phsp at :" << abs(iPhspZPos) / cm
           << "[cm] above isocentre" << G4endl;
    G4String solidName = "phspSolid" + svc::to_string(iPhspZPos);
    G4ThreeVector phspTranslation(0. * cm, 0. * cm, iPhspZPosLocal);
    m_headPhspPV.push_back(new G4PVPlacement(0, phspTranslation, solidName, phspLV, phspMother, false, 0));

  } // iPhspZPosition loop
  return true;
}

