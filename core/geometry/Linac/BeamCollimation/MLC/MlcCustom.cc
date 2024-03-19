//
// Created by brachwal on 26.08.2020.
//

#include "MlcCustom.hh"
#include "Services.hh"
#include "G4Region.hh"
#include "G4ProductionCuts.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"

////////////////////////////////////////////////////////////////////////////////
///
MlcCustom::MlcCustom(G4VPhysicalVolume* parentPV):IPhysicalVolume("MlcCustom"), Configurable("MlcCustom"){
  Configure();
  Construct(parentPV);
}

////////////////////////////////////////////////////////////////////////////////
///
MlcCustom::~MlcCustom() {
  configSvc()->Unregister(thisConfig()->GetName());
}

////////////////////////////////////////////////////////////////////////////////
///
void MlcCustom::Configure() {
  G4cout << "\n[INFO]::  Configuring the " << thisConfig()->GetName() << G4endl;

  DefineUnit<std::string>("PositionningFileType");

  Configurable::DefaultConfig();   // setup the default configuration for all defined units/parameters
  Configurable::PrintConfig();

  // Region and default production cuts
  m_mlc_region = std::make_unique<G4Region>("MlcCustomRegion");
  m_mlc_region->SetProductionCuts(new G4ProductionCuts());
  m_mlc_region->GetProductionCuts()->SetProductionCut(1.0 * cm);
}

////////////////////////////////////////////////////////////////////////////////
///
void MlcCustom::DefaultConfig(const std::string &unit) {

  // Volume name
  if (unit.compare("Label") == 0)
    thisConfig()->SetValue(unit, std::string("Varian Custom MLC"));

  // PositionningFileType - define the way how the MLC positionning will be configured:
  // Custom - import configuration from the custom ASCII file
  // RTPlan - import comfiguration from the RT-Plan file
  if (unit.compare("PositionningFileType") == 0)
    thisConfig()->SetValue(unit, std::string("RTPlan")); //  

}

////////////////////////////////////////////////////////////////////////////////
/// There is two types of custom leaves ("thin" and "fat") positioned in XY plane,
/// along the Y axis, Y1 and Y2 sides, 60 leafs in total at each side:
/// Leaf length  25 cm, height 6.9 cm
/// - Central "thin" leaf width - 2.5 mm x 32 leafs
/// - Outboard "fat" leaf width - 5.0 mm x 14 x 2 leafs
/// NOTE: by default the MLC instance is constructed with fully-closed positioning
void MlcCustom::Construct(G4VPhysicalVolume *parentPV){
  G4cout << "\n[INFO]::  Construction of the " 
         << thisConfig()->GetValue<std::string>("Label")
         << G4endl;

  m_parentPV = parentPV;
  auto Fe = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_Fe");
  
  G4double centralLeafWidth = 2.5 * mm; // 2.5
  G4double sideLeafWidth = 5.0 * mm; //
  G4double leafHeight = 6.9 * cm;  // along the beam direction
  G4double leafLength = 25. * cm;  // perpendicular to beam direction

  // _________________________________________________________________________
  // Single leaf box and logical volume instances
  auto boxCentraLeaf= new G4Box("CentralLeafBox", centralLeafWidth / 2., leafLength / 2., leafHeight / 2.);
  auto centralLeafLV = new G4LogicalVolume(boxCentraLeaf, Fe.get(), "CentralLeafLV", 0, 0, 0);
  centralLeafLV->SetRegion(m_mlc_region.get());

  auto boxSideLeaf = new G4Box("SideLeafBox", sideLeafWidth / 2., leafLength / 2., leafHeight / 2.);
  auto sideLeafLV = new G4LogicalVolume(boxSideLeaf, Fe.get(), "SideLeafLV", 0, 0, 0);
  sideLeafLV->SetRegion(m_mlc_region.get());

  // _________________________________________________________________________
  // The complete set of MLC leafes - physical volumes
  G4LogicalVolume *leafLV;
  G4double mlcPosZ = 46.5 * cm; // The fixed Z position
  G4double mlcInitialCentrePosX = - 32/2 * centralLeafWidth - 28/2 * sideLeafWidth + sideLeafWidth/2;
  G4double mlcCentrePosY1 = - leafLength / 2.;
  G4double mlcCentrePosY2 =   leafLength / 2.;
  G4ThreeVector leafCentre3Vec(mlcInitialCentrePosX, 0., mlcPosZ);

  leafLV = sideLeafLV;
  for (int i = 0; i <= 59; i++) {
    auto nameY1 = "LeafY1PV" + G4String(std::to_string(i));
    auto nameY2 = "LeafY2PV" + G4String(std::to_string(i));
    leafCentre3Vec.setY(mlcCentrePosY1);
    m_y1_leaves.push_back(std::make_unique<G4PVPlacement>(nullptr, leafCentre3Vec, nameY1, leafLV, m_parentPV, false, i));
    leafCentre3Vec.setY(mlcCentrePosY2);
    m_y2_leaves.push_back(std::make_unique<G4PVPlacement>(nullptr, leafCentre3Vec, nameY2, leafLV, m_parentPV, false, i));

    G4double shiftStep;
    if (i > 13 && i < 46) { // central leafes
      if(i==14)
        shiftStep = sideLeafWidth / 2. +  centralLeafWidth/2.;
      else
        shiftStep = centralLeafWidth;
      leafLV = centralLeafLV;
      leafCentre3Vec.setX(leafCentre3Vec.getX() + shiftStep);
    } else {                 // side leaves
      if(i==46)
        shiftStep = centralLeafWidth/2. + sideLeafWidth / 2.;
      else
        shiftStep = sideLeafWidth;
      leafLV = sideLeafLV;
      leafCentre3Vec.setX(leafCentre3Vec.getX() + shiftStep );
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
///
G4bool MlcCustom::Update(){
  // implement me.
}

////////////////////////////////////////////////////////////////////////////////
///
void MlcCustom::Reset(){
  // implement me.
}

////////////////////////////////////////////////////////////////////////////////
///
void MlcCustom::WriteInfo(){
  // implement me.
}

////////////////////////////////////////////////////////////////////////////////
/// DEV NOTE: It might be also worth to implement the Close(), Open() methods.
/// DEV NOTE: There will be class in the future: LinacRun
///           that will inlcude all info regarding the:
///           1. Ctrl point value
///           2. Gantry rotation value
///           3. etc...
// void MlcCustom::SetRunConfiguration(const G4Run* aRun){

//   G4cout << "[WARNING]:: VMlcCustom:: SetRunConfiguration IS SWITCHED OFF!!!" << G4endl;
//   return; // tempory!
  
//   auto inputType = thisConfig()->GetValue<std::string>("PositionningFileType");
//   G4cout << "[INFO]:: MlcCustom:: the run configuration type: "<< inputType << G4endl;

//   if(inputType=="Custom"){
//     auto flsz = std::string("3x3"); // temporary fixed; it should come from GeoSvc or RunSvc
//     SetCustomPositioning(flsz);
//   }
//   else if(inputType=="RTPlan"){
//     auto beamId = int(0);         // temporary fixed; it will come from LinacRun instance
//     auto controlPointId = int(0); // temporary fixed; it will come from LinacRun instance
//     SetRTPlanPositioning(beamId,controlPointId);
//   }
// }

////////////////////////////////////////////////////////////////////////////////
/// 
void MlcCustom::SetCustomPositioning(const std::string& fieldSize){
  std::string data_path = PROJECT_DATA_PATH;
  auto customPositioningFile = data_path+"/TrueBeam/MLC/Mlc_"+fieldSize+".dat";
  G4cout << "[INFO]:: Reading the MLC configuration file :: " << customPositioningFile << G4endl;
  std::string line;
  std::ifstream file(customPositioningFile.c_str());
  int leafsCounter = 0;
  if (file.is_open()) {  // check if file exists
    while (getline(file, line)){
      // get rid of commented out or empty lines:
      if (line.length() > 0 && line.at(0) != '#') {
        std::istringstream ss(line);
        std::string svalue;
        std::vector<double> y1y2_position;
        while (getline(ss, svalue,',')){
          y1y2_position.emplace_back(std::strtod(svalue.c_str(),nullptr)*mm);
          
          //G4cout << "[DEBUG]:: MlcCustom:: config value ("<<count<<")"<< svalue << G4endl;
        }
        //
        // Input data file should contain 2 columns with comma separation, verify it:
        if(y1y2_position.size()>2)
          G4Exception("MlcCustom", "SetCustomPositioning", FatalErrorInArgument, "Wrong input data format!");

        if(leafsCounter>59)
          G4Exception("MlcCustom", "SetCustomPositioning", FatalErrorInArgument, "To many leafs configuration!");

        auto y1_translation = m_y1_leaves[leafsCounter]->GetTranslation();
        y1_translation.setY(y1_translation.getY()+y1y2_position.at(0) );
        m_y1_leaves[leafsCounter]->SetTranslation(y1_translation);

        auto y2_translation = m_y2_leaves[leafsCounter]->GetTranslation();
        y2_translation.setY(y2_translation.getY()+y1y2_position.at(1) );
        m_y2_leaves[leafsCounter]->SetTranslation(y2_translation);

        ++leafsCounter;

      }
    }
    if(leafsCounter<59)
      G4Exception("MlcCustom", "SetCustomPositioning", FatalErrorInArgument, "To less leafs configuration!");
  }
}

////////////////////////////////////////////////////////////////////////////////
/// 
void MlcCustom::SetRTPlanPositioning(int current_beam, int current_controlpoint){
  auto dicomSvc = Service<DicomSvc>();
  auto posY1 = dicomSvc->GetRTPlanMlcPossitioning("Y1",current_beam,current_controlpoint);
  auto posY2 = dicomSvc->GetRTPlanMlcPossitioning("Y2",current_beam,current_controlpoint);

  if(posY1.size()!=posY2.size() || posY1.size()!=60){
    G4cout << "[DEBUG]:: MlcCustom:: posY1.size() " << posY1.size() << G4endl;
    G4cout << "[DEBUG]:: MlcCustom:: posY2.size() " << posY2.size() << G4endl;
    G4Exception("MlcCustom", "SetRTPlanPositioning", FatalErrorInArgument, "Wrong MLC positioning data retrieved!");
  }
  
  for(int i=0; i<posY1.size(); ++i){
    G4cout << "[DEBUG]:: MlcCustom:: Y1 "<<posY1.at(i)<<", Y2 "<< posY2.at(i) << G4endl;
    // overlap check:
    if(posY2.at(i)-posY1.at(i)<0){
      G4cout << "[WARNING]:: MlcCustom:: OVERLAP Y1 "<<posY1.at(i)<<", Y2 "<< posY2.at(i) << G4endl;
      posY1[i] = 0;
      posY2[i] = 0;
    }

    auto y1_translation = m_y1_leaves[i]->GetTranslation();
    y1_translation.setY(y1_translation.getY()+posY1.at(i) );
    m_y1_leaves[i]->SetTranslation(y1_translation);

    auto y2_translation = m_y2_leaves[i]->GetTranslation();
    y2_translation.setY(y2_translation.getY()+posY2.at(i) );
    m_y2_leaves[i]->SetTranslation(y2_translation);

    if(y1_translation.getX()-y2_translation.getX()!=0){
      G4cout << "[WARNING]:: MlcCustom:: OVERLAP X1 "<<y1_translation.getX()<<", X2 "<< y2_translation.getX() << G4endl;
    }

  }

  G4cout << "[DEBUG]:: OVERLAP Y1 13 "<< m_y1_leaves[13]->GetTranslation() << G4endl;
  G4cout << "[DEBUG]:: OVERLAP Y2 13 "<< m_y2_leaves[13]->GetTranslation() << G4endl;

  G4cout << "[DEBUG]:: OVERLAP Y1 14 "<< m_y1_leaves[14]->GetTranslation() << G4endl;
  G4cout << "[DEBUG]:: OVERLAP Y2 14 "<< m_y2_leaves[14]->GetTranslation() << G4endl;

  G4cout << "[DEBUG]:: OVERLAP Y1 45 "<< m_y1_leaves[45]->GetTranslation() << G4endl;
  G4cout << "[DEBUG]:: OVERLAP Y2 45 "<< m_y2_leaves[45]->GetTranslation() << G4endl;

  G4cout << "[DEBUG]:: OVERLAP Y1 46 "<< m_y1_leaves[46]->GetTranslation() << G4endl;
  G4cout << "[DEBUG]:: OVERLAP Y2 46 "<< m_y2_leaves[46]->GetTranslation() << G4endl;


}

