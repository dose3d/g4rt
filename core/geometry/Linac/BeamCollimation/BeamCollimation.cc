#include "BeamCollimation.hh"
#include "MlcCustom.hh"
#include "MlcMillennium.hh"
#include "MlcHD120.hh"
#include "Services.hh"

#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4SystemOfUnits.hh"
#include "G4PVPlacement.hh"
#include "G4ProductionCuts.hh"
#include "G4Tubs.hh"
#include "G4Box.hh"
#include "G4Cons.hh"

////////////////////////////////////////////////////////////////////////////////
///
BeamCollimation::BeamCollimation() : IPhysicalVolume("BeamCollimation"), Configurable("BeamCollimation"){
  // m_leavesA = *m_geoSvc->getLeavesPositioning("A");
  // m_leavesB = *m_geoSvc->getLeavesPositioning("B");
  Configure();
}

////////////////////////////////////////////////////////////////////////////////
///
BeamCollimation::~BeamCollimation() {
  Destroy();
  configSvc()->Unregister(thisConfig()->GetName());
}

////////////////////////////////////////////////////////////////////////////////
///
BeamCollimation *BeamCollimation::GetInstance() {
  static BeamCollimation instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
///
void BeamCollimation::Configure() {
  G4cout << "\n\n[INFO]::  Configuring the " << thisConfig()->GetName() << G4endl;
  // DefineUnit<G4double>("ionizationChamberThicknessP");
  // DefineUnit<G4double>("ionizationChamberThicknessW");

  Configurable::DefaultConfig();   // setup the default configuration for all defined units/parameters
  Configurable::PrintConfig();
}

////////////////////////////////////////////////////////////////////////////////
///
void BeamCollimation::DefaultConfig(const std::string &unit) {

  // Volume name
  if (unit.compare("Label") == 0)
    thisConfig()->SetValue(unit, std::string("Varian True Beam Head"));

  // describe me.
  if (unit.compare("ionizationChamberThicknessP") == 0)
    thisConfig()->SetValue(unit, G4double(50.)); // [um]
  if (unit.compare("ionizationChamberThicknessW") == 0)
    thisConfig()->SetValue(unit, G4double(50.)); // [um]

}

////////////////////////////////////////////////////////////////////////////////
///
void BeamCollimation::WriteInfo() {
  G4cout << "\n\n\tnominal beam energy: " << configSvc()->GetValue<int>("RunSvc", "idEnergy") << G4endl;
  G4cout << "\tJaw X aperture: 1) "
         << configSvc()->GetValue<G4double>("GeoSvc", "jaw1XAperture") / cm << "[cm]\t2) "
         << configSvc()->GetValue<G4double>("GeoSvc", "jaw2XAperture") / cm << " [cm]" << G4endl;
  G4cout << "\tJaw Y aperture: 1) "
         << configSvc()->GetValue<G4double>("GeoSvc", "jaw1YAperture") / cm << "[cm]\t2) "
         << configSvc()->GetValue<G4double>("GeoSvc", "jaw2YAperture") / cm << " [cm]\n" << G4endl;
}

////////////////////////////////////////////////////////////////////////////////
///
void BeamCollimation::Destroy() {
  for (auto &ivolume : m_physicalVolume) {
    if (ivolume.second) {
      delete ivolume.second;
      ivolume.second = nullptr;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void BeamCollimation::Construct(G4VPhysicalVolume *parentWorld) {
  m_parentPV = parentWorld;
  // Jaw1X();
  // Jaw2X();
  // Jaw1Y();
  // Jaw2Y();
  MLC();
}

////////////////////////////////////////////////////////////////////////////////
///
void BeamCollimation::Reset() {
  m_leavesA.clear();
  m_leavesB.clear();
}
////////////////////////////////////////////////////////////////////////////////
///

G4ThreeVector BeamCollimation::TransformToHeadOuputPlane(const G4ThreeVector& momentum){
  G4double x, y, z, zRatio = 0.;
  z = 1000 - 320.;
  zRatio = z/momentum.getZ();
  x = zRatio * momentum.getX();
  y = zRatio * momentum.getY();
  z = -320;
  return G4ThreeVector(x,y,z);
}


////////////////////////////////////////////////////////////////////////////////
///
void BeamCollimation::SetJawAperture(G4int idJaw, G4ThreeVector &centre, G4ThreeVector halfSize,
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
bool BeamCollimation::Jaw1X() {
  auto tungsten = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_W");
  G4String name = "Jaws1X";

  auto cRotation = new G4RotationMatrix();
  G4ThreeVector centre(0., 0., (320. + 80. / 2.) * mm);
  //G4ThreeVector halfSize(45. * mm, 93. * mm, 78. / 2. * mm);
  G4ThreeVector halfSize(55. * mm, 100. * mm, 90. / 2. * mm);
  auto box = new G4Box(name + "Box", halfSize.getX(), halfSize.getY(), halfSize.getZ());
  auto logVol = new G4LogicalVolume(box, tungsten.get(), name + "LV", 0, 0, 0);
  SetJawAperture(1, centre, halfSize, cRotation);
  m_physicalVolume[name] = new G4PVPlacement(cRotation, centre, name + "PV", logVol, m_parentPV, false, 0);

  // Region for cuts
  auto regVol = new G4Region(name + "R");
  auto *cuts = new G4ProductionCuts;
  cuts->SetProductionCut(0.001 * mm);
  regVol->SetProductionCuts(cuts);
  logVol->SetRegion(regVol);
  regVol->AddRootLogicalVolume(logVol);

  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
bool BeamCollimation::Jaw2X() {
  auto tungsten = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_W");
  G4String name = "Jaws2X";

  auto cRotation = new G4RotationMatrix();
  G4ThreeVector centre(0., 0., (320. + 80. / 2.) * mm);
  //G4ThreeVector halfSize(45. * mm, 93. * mm, 78. / 2. * mm);
  G4ThreeVector halfSize(55. * mm, 100. * mm, 90. / 2. * mm);
  auto box = new G4Box(name + "Box", halfSize.getX(), halfSize.getY(), halfSize.getZ());
  auto logVol = new G4LogicalVolume(box, tungsten.get(), name + "LV", 0, 0, 0);
  SetJawAperture(2, centre, halfSize, cRotation);
  m_physicalVolume[name] = new G4PVPlacement(cRotation, centre, name + "PV", logVol, m_parentPV, false, 0);

  // Region for cuts
  auto regVol = new G4Region(name + "R");
  auto cuts = new G4ProductionCuts;
  cuts->SetProductionCut(0.001 * mm);
  regVol->SetProductionCuts(cuts);
  logVol->SetRegion(regVol);
  regVol->AddRootLogicalVolume(logVol);

  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
bool BeamCollimation::Jaw1Y() {
  auto tungsten = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_W");
  G4String name = "Jaws1Y";

  auto cRotation = new G4RotationMatrix();
  //G4ThreeVector centre(0., 0., (230. + 80. / 2.) * mm);
  G4ThreeVector centre(0., 0., (230. + 80. / 2. -6) * mm);
  //G4ThreeVector halfSize(93. * mm, 35. * mm, 78. / 2. * mm);
  G4ThreeVector halfSize(100. * mm, 45. * mm, 90. / 2. * mm);

  auto box = new G4Box(name + "Box", halfSize.getX(), halfSize.getY(), halfSize.getZ());
  auto logVol = new G4LogicalVolume(box, tungsten.get(), name + "LV", 0, 0, 0);
  SetJawAperture(3, centre, halfSize, cRotation);
  m_physicalVolume[name] = new G4PVPlacement(cRotation, centre, name + "PV", logVol, m_parentPV, false, 0);

  // Region for cuts
  auto regVol = new G4Region(name + "R");
  auto cuts = new G4ProductionCuts;
  cuts->SetProductionCut(0.001 * mm);
  regVol->SetProductionCuts(cuts);
  logVol->SetRegion(regVol);
  regVol->AddRootLogicalVolume(logVol);

  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
bool BeamCollimation::Jaw2Y() {
  auto tungsten = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_W");
  G4String name = "Jaws2Y";

  auto cRotation = new G4RotationMatrix();
  //G4ThreeVector centre(0., 0., (230. + 80. / 2.) * mm);
  G4ThreeVector centre(0., 0., (230. + 80. / 2. -6) * mm);
  //G4ThreeVector halfSize(93. * mm, 35. * mm, 78. / 2. * mm);
  G4ThreeVector halfSize(100. * mm, 45. * mm, 90. / 2. * mm);
  auto box = new G4Box(name + "Box", halfSize.getX(), halfSize.getY(), halfSize.getZ());
  auto logVol = new G4LogicalVolume(box, tungsten.get(), name + "LV", 0, 0, 0);
  SetJawAperture(4, centre, halfSize, cRotation);
  m_physicalVolume[name] = new G4PVPlacement(cRotation, centre, name + "PV", logVol, m_parentPV, false, 0);

  // Region for cuts
  auto regVol = new G4Region(name + "R");
  auto cuts = new G4ProductionCuts;
  cuts->SetProductionCut(0.001 * mm);
  regVol->SetProductionCuts(cuts);
  logVol->SetRegion(regVol);
  regVol->AddRootLogicalVolume(logVol);

  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
bool BeamCollimation::MLC() {

  auto model = Service<GeoSvc>()->GetMlcModel();

  if(model == EMlcModel::None)
    return true;

  if(!m_mlc){
    G4cout << "[INFO]:: BeamCollimation::MLC: Constructing the MLC model instantiation! " << G4endl;
    switch (model) {
      case EMlcModel::Custom:
        m_mlc = std::make_unique<MlcCustom>(m_parentPV);
        break;
      case EMlcModel::Millennium:
        //m_mlc = std::make_unique<MlcMillennium>(m_parentPV);
        break;
      case EMlcModel::HD120:
        m_mlc = std::make_unique<MlcHd120>(m_parentPV);
        break;
    }
  } else {
    G4cout << "[INFO]:: BeamCollimation::MLC: RESET the MLC model instantiation! " << G4endl;
    switch (model) {
      case EMlcModel::Custom:
        m_mlc.reset(new MlcCustom(m_parentPV));
        break;
      case EMlcModel::Millennium:
        //m_mlc.reset(new MlcMillennium(m_parentPV));
        break;
      case EMlcModel::HD120:
        m_mlc.reset(new MlcHd120(m_parentPV));
        break;
      case EMlcModel::Ghost:
        LOGSVC_INFO("Using Ghost type of MLC")
        break;
    }

  }
  return true;
}
