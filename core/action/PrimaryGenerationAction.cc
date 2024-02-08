#include "PrimaryGenerationAction.hh"
#include "IaeaPrimaryGenerator.hh"
#include "G4GeneralParticleSource.hh"
#include "IonPrimaryGenerator.hh"
#include "PrimariesAnalysis.hh"
#include "G4Event.hh"
#include "Services.hh"
#include "PrimaryParticleInfo.hh"
#include "G4EventManager.hh"
#include "BeamCollimation.hh"


namespace {
  G4Mutex PrimGenMutex = G4MUTEX_INITIALIZER;
}

G4RotationMatrix* PrimaryGenerationAction::m_rotation_matrix=nullptr;

////////////////////////////////////////////////////////////////////////////////
///
PrimaryGenerationAction::PrimaryGenerationAction(){
  SetPrimaryGenerator();
  ConfigurePrimaryGenerator();
}

////////////////////////////////////////////////////////////////////////////////
///
void PrimaryGenerationAction::SetPrimaryGenerator() {
  auto configSvc = Service<ConfigSvc>();
  auto sourceType = configSvc->GetValue<std::string>("RunSvc", "BeamType");
  if (sourceType == "IAEA") {
    m_generatorType = PrimaryGeneratorType::PhspIAEA;
  } else if (sourceType == "gps") {
    m_generatorType = PrimaryGeneratorType::GPS;
  } else if (sourceType == "ion") {
    m_generatorType = PrimaryGeneratorType::IonGPS;
  } else {
    G4ExceptionDescription msg;
    msg << "Primary generator of type '" << sourceType << "' is not defined";
    G4Exception("PrimaryGenerationAction", "SetPrimaryGenerator", FatalErrorInArgument, msg);
  }
}
  

////////////////////////////////////////////////////////////////////////////////
///
void PrimaryGenerationAction::ConfigurePrimaryGenerator() {
  auto configSvc = Service<ConfigSvc>();
  G4AutoLock lock(&PrimGenMutex);
  std::string file;
  switch (m_generatorType) {
    case PrimaryGeneratorType::PhspIAEA:
      file = configSvc->GetValue<std::string>("RunSvc", "PhspInputFileName");
      if(!svc::checkIfFileExist(file+".IAEAheader")){
        G4ExceptionDescription msg;
        msg << "IAEA header file doesn't exists: ";
        msg << file+".IAEAheader";
        G4Exception("PrimaryGenerationAction", "ConfigurePrimaryGenerator", FatalErrorInArgument, msg);
      }
      if(!svc::checkIfFileExist(file+".IAEAphsp")){
        G4ExceptionDescription msg;
        msg << "IAEA phsp file doesn't exists: ";
        msg << file+".IAEAphsp";
        G4Exception("PrimaryGenerationAction", "ConfigurePrimaryGenerator", FatalErrorInArgument, msg);
      }
      m_primaryGenerator = new IaeaPrimaryGenerator(G4String(file));
      break;
    case PrimaryGeneratorType::GPS:
      m_primaryGenerator = new G4GeneralParticleSource();
      break;
    case PrimaryGeneratorType::IonGPS:
      // Cd 109
      G4int A = 48;
      G4int Z = 109;
      G4int Q = 0;
      G4double E = 59.6; // keV, 109m1
      // G4double E = 463.0; // keV, 109m2
      m_primaryGenerator = new IonPrimaryGenerator();
      static_cast<IonPrimaryGenerator*>(m_primaryGenerator)->AddSource(A,Z,Q,E,1);
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
PrimaryGenerationAction::~PrimaryGenerationAction(void) {
  G4AutoLock lock(&PrimGenMutex);
  if (m_primaryGenerator) {
    delete m_primaryGenerator;
    m_primaryGenerator = nullptr;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void PrimaryGenerationAction::GeneratePrimaries(G4Event *anEvent) {
  auto runSvc = Service<RunSvc>();
  G4AutoLock lock(&PrimGenMutex);
  auto evtID = anEvent->GetEventID();
  m_primaryGenerator->GeneratePrimaryVertex(anEvent);
  auto nVrtx = anEvent->GetNumberOfPrimaryVertex();
  if (nVrtx>0 && Service<ConfigSvc>()->GetValue<bool>("RunSvc", "PrimariesAnalysis") )
    PrimariesAnalysis::GetInstance()->FillPrimaries(anEvent);
  G4ThreeVector NewCentre;
  G4ThreeVector CurrentCentre;
  G4ThreeVector AxisOfRotation = G4ThreeVector(0.0,0.0,1.0);
  G4RotationMatrix *RotMat = new G4RotationMatrix();
  RotMat -> rotate(90.0 *deg, AxisOfRotation);
  

  auto fieldSize_a = Service<ConfigSvc>()->GetValue<double>("RunSvc", "FieldSizeA");
  auto fieldSize_b = Service<ConfigSvc>()->GetValue<double>("RunSvc", "FieldSizeB");

  if(fieldSize_b == 0){
    fieldSize_b = fieldSize_a;
  }
  auto fieldshape = Service<ConfigSvc>()->GetValue<std::string>("RunSvc", "FieldShape");
  auto fieldPossition =  Service<ConfigSvc>()->GetValue<std::string>("RunSvc", "PhspInputPosition");
  double cutFieldSize_a = 0;
  double cutFieldSize_b = 0;
  double fa = 1000.;
  if (fieldSize_a>0){ // calc for SSD = 1000 mm
    double ssd = 1000;
    // double fa = ssd - 745.4; // ssd - s1 z position
    // double fa = 699.75;      // ssd - s2 z position
    if (fieldPossition == "s1")
      fa = ssd - 745.4;
    if (fieldPossition == "s2")
      fa = 699.75; 

    cutFieldSize_a = (fa / ssd) * fieldSize_a / 2.;
    cutFieldSize_b = (fa / ssd) * fieldSize_b / 2.;
  }

  for(int i=0; i<nVrtx;++i){
    auto vrtx = anEvent->GetPrimaryVertex(i);
    auto model = Service<GeoSvc>()->GetMlcModel();
    CurrentCentre = vrtx->GetPosition();
    if(model == EMlcModel::Ghost){
      CurrentCentre = BeamCollimation::TransformToHeadOuputPlane(vrtx->GetPrimary()->GetMomentum()); 
    }
      // Set arbitrary cut for field size:
      if(cutFieldSize_a>0){
        // G4cout << "X: " << CurrentCentre.x() << "Y: " << CurrentCentre.y() << "Z: " << CurrentCentre.z() << G4endl;
        if (fieldshape == "Rectangular"){
          if (CurrentCentre.x()<-cutFieldSize_a || CurrentCentre.x() > cutFieldSize_a || CurrentCentre.y()<-cutFieldSize_b || CurrentCentre.y() > cutFieldSize_b ){
              vrtx->GetPrimary()-> SetKineticEnergy(0.); // actually kill this particle
          }
        }
        if (fieldshape == "Elipsoidal"){
          if ((pow(CurrentCentre.x(),2)/ pow(cutFieldSize_a,2) + pow(CurrentCentre.y(),2)/pow(cutFieldSize_b,2))> 1 ){
              vrtx->GetPrimary()-> SetKineticEnergy(0.); // actually kill this particle
          }
        }
      }
      if(m_rotation_matrix){
        NewCentre=(*m_rotation_matrix)*(vrtx->GetPosition());
        NewCentre=(*RotMat)*(NewCentre);
      }
      vrtx->SetPosition(NewCentre.x(),NewCentre.y(),NewCentre.z());

      auto momentum = (vrtx->GetPrimary()->GetMomentum());
      if(m_rotation_matrix){
        momentum = (*m_rotation_matrix)*(momentum);
        momentum = (*RotMat)*(momentum);
      }
      vrtx->GetPrimary()->SetMomentum(momentum.x(),momentum.y(),momentum.z());

    }
  // if (nVrtx>50){
  //   evtID = anEvent->GetEventID();
  //   G4EventManager::GetEventManager()->AbortCurrentEvent();
  //   G4cout << "[DEBUG]:: Evt("<<evtID<< "): nVrtx ("<<nVrtx<<"): Aborting event.." << G4endl;
  //   nVrtx = 0;
  // }

  // if(m_generatorType==PrimaryGeneratorType::PhspIAEA){
    
  //   if (vvrtx.size()>0){
  //     //G4cout << "[DEBUG] Evt("<<evtID<<") generatePrimaries got " << vvrtx.size() << " vertices."<< G4endl;
  //     for (const auto& vrtx : vvrtx){
  //       anEvent->AddPrimaryVertex(vrtx);
  //       auto pparticle = vrtx->GetPrimary();
  //       auto energy = pparticle->GetTotalEnergy() / keV ;
  //       if(energy<80 || energy > 90){
  //           G4EventManager::GetEventManager()->AbortCurrentEvent();
  //           break;
  //       }
  //       // if(energy>80 && energy < 90){
  //         //G4cout << "[DEBUG] Evt("<<evtID<<") energy = " << energy << "[keV]"<< G4endl;
  //         //G4cout << "[DEBUG] Evt("<<evtID<<") theta = " << pparticle->GetMomentumDirection().theta()*180/3.14<< G4endl;
  //         // anEvent->AddPrimaryVertex(vrtx);
  //         // filledEvt = true; // any vrtx in evt means that it has been filled
  //       // }
  //       // else {
  //       //   anEvent->AbortCurrentEvent();
  //       //   // delete vrtx;
  //       //   // ++evtID;
  //       // }
  //     }
  //   }
  //   // }
  // }
  // anEvent->SetEventID(evtID);
  // G4cout << "[DEBUG] Evt("<<evtID<<") nVrtx = " << nVrtx << G4endl;

  G4bool filter_passed = true;
  for(int i =0; i<nVrtx; ++i ){
    auto pparticle = anEvent->GetPrimaryVertex(i)->GetPrimary();
    
    // Energy cut
    auto energy = pparticle->GetTotalEnergy() / keV ;

    //
    if(!filter_passed)
      G4EventManager::GetEventManager()->AbortCurrentEvent();

    // Fill PrimaryParticleInfo
    auto pparticleInfo = pparticle->GetUserInformation();
    if(pparticleInfo){
      dynamic_cast<PrimaryParticleInfo*>(pparticleInfo)->FillInfo(pparticle);
    }
    else{
      pparticleInfo = new PrimaryParticleInfo(); // it's being cleaned up by kernel when event ends it's life
      dynamic_cast<PrimaryParticleInfo*>(pparticleInfo)->FillInfo(pparticle);
      pparticle->SetUserInformation(pparticleInfo);
    }
  }
  delete RotMat;
}
