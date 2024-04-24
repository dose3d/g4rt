#include "LinacGeometry.hh"
#include "BeamCollimation.hh"
#include "MlcMillennium.hh"
#include "MlcSimplified.hh"
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

std::unique_ptr<VMlc> BeamCollimation::m_mlc = nullptr;
G4double BeamCollimation::AfterMLC = -430.0;
G4double BeamCollimation::BeforeMLC  = -870.0;

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
  Jaw1X();
  Jaw2X();
  Jaw1Y();
  Jaw2Y();
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

void BeamCollimation::FilterPrimaries(std::vector<G4PrimaryVertex*>& p_vrtx) {
  Service<RunSvc>()->CurrentControlPoint()->MLC();
  for(int i=0; i < p_vrtx.size();++i){
    BeamCollimation::SetParticlePositionBeforeMLC(p_vrtx.at(i), BeforeMLC);
  }

  auto model = Service<GeoSvc>()->GetMlcModel();
  if(model != EMlcModel::Simplified)
    return;

  for(int i=0; i < p_vrtx.size();++i){
    auto vrtx = p_vrtx.at(i);
    if(!m_mlc->IsInField(vrtx)) {
      delete vrtx;
      p_vrtx.at(i) = nullptr;
    }
  }
  p_vrtx.erase(std::remove_if(p_vrtx.begin(), p_vrtx.end(), [](G4PrimaryVertex* ptr) { return ptr == nullptr; }), p_vrtx.end());
  Service<RunSvc>()->CurrentControlPoint()->FillSimFieldMask(p_vrtx);
}


////////////////////////////////////////////////////////////////////////////////
///
G4ThreeVector BeamCollimation::SetParticlePositionBeforeMLC(G4PrimaryVertex* vrtx, G4double finalZ) {
  G4double x, y, zRatio = 0.;
  G4double deltaX, deltaY, deltaZ;
  G4ThreeVector position = vrtx->GetPosition();
  deltaZ = finalZ - position.getZ();
  G4ThreeVector directionalVersor = vrtx->GetPrimary()->GetMomentum().unit();
  zRatio = deltaZ / directionalVersor.getZ(); 
  x = position.getX() + zRatio * directionalVersor.getX(); // x + deltaX;
  y = position.getY() + zRatio * directionalVersor.getY(); // y + deltaY;
  vrtx->SetPosition(x, y, finalZ);
  return G4ThreeVector(x, y, finalZ);
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
  G4ThreeVector centre(0., 0., (105.) * mm);
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
  G4ThreeVector centre(0., 0., (105.) * mm);
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
  G4ThreeVector centre(0., 0., (205.) * mm);
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
  G4ThreeVector centre(0., 0., (205.) * mm);
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

  if(!m_mlc.get()){
    G4cout << "[INFO]:: BeamCollimation::MLC: Constructing the MLC model instantiation! " << G4endl;
    switch (model) {
      case EMlcModel::Millennium:
        //m_mlc = std::make_unique<MlcMillennium>(m_parentPV);
        break;
      case EMlcModel::HD120:
        m_mlc = std::make_unique<MlcHd120>(m_parentPV);
        break;
      case EMlcModel::Simplified:
        LOGSVC_INFO("Using Simplified type of MLC");
        m_mlc = std::make_unique<MlcSimplified>();
        break;
    }
  } else {
    G4cout << "[INFO]:: BeamCollimation::MLC: RESET the MLC model instantiation! " << G4endl;
    switch (model) {
      case EMlcModel::Millennium:
        //m_mlc.reset(new MlcMillennium(m_parentPV));
        break;
      case EMlcModel::HD120:
        m_mlc.reset(new MlcHd120(m_parentPV));
        break;
      case EMlcModel::Simplified:
        LOGSVC_INFO("Using Simplified type of MLC");
        m_mlc.reset(new MlcSimplified());
        break;
    }

  }
  return true;
}
