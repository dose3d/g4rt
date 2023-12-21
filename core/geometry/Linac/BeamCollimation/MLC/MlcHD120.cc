//
// Created by jakc on 24.09.2020.
//

#include "MlcHD120.hh"
#include "Services.hh"
#include "G4Region.hh"
#include "G4ProductionCuts.hh"
#include "G4PVReplica.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4RotationMatrix.hh"
#include "G4PVPlacement.hh"

////////////////////////////////////////////////////////////////////////////////
///
MlcHd120::MlcHd120(G4VPhysicalVolume* parentPV):IPhysicalVolume("MlcHd120"), Configurable("MlcHd120"){
    Configure();
    Construct(parentPV);
    IRunConfigurable::AddRunConfigurableModule(this);
}

////////////////////////////////////////////////////////////////////////////////
///
MlcHd120::~MlcHd120() {
    configSvc()->Unregister(thisConfig()->GetName());
}

////////////////////////////////////////////////////////////////////////////////
///
void MlcHd120::Configure() {
    G4cout << "\n[INFO]::  Configuring the " << thisConfig()->GetName() << G4endl;

    DefineUnit<std::string>("PositionningFileType");

    Configurable::DefaultConfig();   // setup the default configuration for all defined units/parameters
    Configurable::PrintConfig();

    // Region and default production cuts
    m_mlc_region = std::make_unique<G4Region>("MlcHd120Region");
    m_mlc_region->SetProductionCuts(new G4ProductionCuts());
    m_mlc_region->GetProductionCuts()->SetProductionCut(1.0 * cm);
}

////////////////////////////////////////////////////////////////////////////////
///
void MlcHd120::DefaultConfig(const std::string &unit) {

    // Volume name
    if (unit.compare("Label") == 0)
        thisConfig()->SetValue(unit, std::string("Varian HD120 MLC"));
    if (unit.compare("PositionningFileType") == 0)
        thisConfig()->SetValue(unit, std::string("Custom")); /// RTPlan, Custom

}

////////////////////////////////////////////////////////////////////////////////
///
void MlcHd120::Construct(G4VPhysicalVolume *parentPV){
    G4cout << "\n[INFO]::  Construction of the "
           << thisConfig()->GetValue<std::string>("Label")
           << G4endl;

    m_parentPV = parentPV;
    auto W_All = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "tungstenAlloy1");

    CreateMlcModules(parentPV,W_All.get());
}

////////////////////////////////////////////////////////////////////////////////
///
G4VPhysicalVolume* MlcHd120::CreateMlcModules(G4VPhysicalVolume* parentPV, G4Material* material){

    // TODO JIRA TNSIM-55
    // If yRotationTest0->rotateZ(90.*deg) - It is our mlc that moves in the Y plane.
    // If yRotationTest0->rotateZ(0.*deg), it moves in the X plane.
    // The rest of the code doesn't need to be changed, it retrieves the appropriate data by itself (I rotate the box where the MLC is, so I am always interested in its X position, because it becomes either X or Y position)
    G4RotationMatrix* mlcWorldRotation = new G4RotationMatrix();
    mlcWorldRotation->rotateZ(90.*deg);

    /////////////////////////////////////////////////////////////////////////////
    //  Creating the MLC world.
    /////////////////////////////////////////////////////////////////////////////

    auto air = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_Galactic");
    std::string moduleName = "MlcWorld";
    G4ThreeVector head_halfSize(68./2*cm, 68./2*cm, 7.1/2*cm);
    auto mlcWorldPosition =  G4ThreeVector(0. * cm, 0. * cm, -269. * mm);
    auto mlcWorldBox = new G4Box(moduleName+"Box", head_halfSize.getX() * mm, head_halfSize.getY() * mm, head_halfSize.getZ() * mm);
    auto mlcWorldLV = new G4LogicalVolume(mlcWorldBox, air.get(), moduleName+"LV", 0, 0, 0);
    auto mlcWorldPV = new G4PVPlacement(mlcWorldRotation, mlcWorldPosition, moduleName+"PV", mlcWorldLV, parentPV, false, 0);

    /////////////////////////////////////////////////////////////////////////////
    //  Giving shape to the leaves located in the center of the MLC.
    /////////////////////////////////////////////////////////////////////////////

    auto centralLeafShape = CreateCentralLeafShape();
    auto centralLeafLV = new G4LogicalVolume(centralLeafShape, material, "centralLeafLV", 0, 0, 0);
    centralLeafLV->SetRegion(m_mlc_region.get());

    /////////////////////////////////////////////////////////////////////////////
    //  Giving shape to the leaves on the outer parts of the MLC.
    /////////////////////////////////////////////////////////////////////////////

    auto sideLeafShape = CreateSideLeafShape();
    auto sideLeafLV = new G4LogicalVolume(sideLeafShape, material, "sideLeafLV", 0, 0, 0);
    sideLeafLV->SetRegion(m_mlc_region.get());

    /////////////////////////////////////////////////////////////////////////////
    //  The shape of the transition leaves between wide and narrow leaves - wide version.
    /////////////////////////////////////////////////////////////////////////////

    auto transitionLeafShape1 = CreateTransitionLeafShape("Side");
    auto transitionLeafLV1 = new G4LogicalVolume(transitionLeafShape1, material, "transitionLeafLV", 0, 0, 0);
    transitionLeafLV1->SetRegion(m_mlc_region.get());

    /////////////////////////////////////////////////////////////////////////////
    //  The shape of the transition leaves between wide and narrow leaves - narrow version.
    /////////////////////////////////////////////////////////////////////////////

    auto transitionLeafShape2 = CreateTransitionLeafShape("Central");
    auto transitionLeafLV2 = new G4LogicalVolume(transitionLeafShape2, material, "transitionLeafLV", 0, 0, 0);
    transitionLeafLV2->SetRegion(m_mlc_region.get());

    /////////////////////////////////////////////////////////////////////////////
    //  The orientation of leaves in space - Version 1.
    /////////////////////////////////////////////////////////////////////////////

    G4RotationMatrix* leavesOrientation1 = new G4RotationMatrix();
    leavesOrientation1->rotateX(90.*deg);
    leavesOrientation1->rotateY(180.*deg);
    leavesOrientation1->rotateZ(0.*deg);

    /////////////////////////////////////////////////////////////////////////////
    //  The orientation of leaves in space - Version 2.
    /////////////////////////////////////////////////////////////////////////////

    G4RotationMatrix* leavesOrientation2 = new G4RotationMatrix();
    leavesOrientation2->rotateX(90.*deg);
    leavesOrientation2->rotateY(0.*deg);
    leavesOrientation2->rotateZ(180.*deg);

    /////////////////////////////////////////////////////////////////////////////
    //  The orientation of leaves in space - Version 3.
    /////////////////////////////////////////////////////////////////////////////

    G4RotationMatrix* leavesOrientation3 = new G4RotationMatrix();
    leavesOrientation3->rotateX(90.*deg);
    leavesOrientation3->rotateY(0.*deg);
    leavesOrientation3->rotateZ(0.*deg);

    /////////////////////////////////////////////////////////////////////////////
    //  The orientation of leaves in space - Version 4.
    /////////////////////////////////////////////////////////////////////////////

    G4RotationMatrix* leavesOrientation4 = new G4RotationMatrix();
    leavesOrientation4->rotateX(90.*deg);
    leavesOrientation4->rotateY(180.*deg);
    leavesOrientation4->rotateZ(180.*deg);

    /////////////////////////////////////////////////////////////////////////////
    //  The placement of leaves in space.
    /////////////////////////////////////////////////////////////////////////////

    G4LogicalVolume *leafLV;
    G4double mlcCentrePosZa = 0.5 * mm;
    G4double mlcCentrePosZb = -0.5 * mm;
    G4ThreeVector leafOneCentre3Vec(16.*cm, -11.*cm, 0.);
    G4ThreeVector leafTwoCentre3Vec(-16.*cm, -11.*cm, 0.);

    /////////////////////////////////////////////////////////////////////////////
    //  Entry of initial logical volume to loop.
    /////////////////////////////////////////////////////////////////////////////

    leafLV = sideLeafLV;

    for (unsigned i = 0; i < 60; ++i) {

        /////////////////////////////////////////////////////////////////////////////
        //  The if/else function divides the leaves into even and odd -
        //  - mirrored leaf reflections from the top and bottom of the MLV.
        //  Even side - top.
        /////////////////////////////////////////////////////////////////////////////

        if (i%2==0) {
            // std::cout << i << std::endl;
            auto name = "LeafY1PV" + G4String(std::to_string(i));

            /////////////////////////////////////////////////////////////////////////////
            //  Creating a pair leaves on opposite sides of the MLC - corresponding leaves.
            /////////////////////////////////////////////////////////////////////////////

            leafOneCentre3Vec.setZ(mlcCentrePosZa);
            leafTwoCentre3Vec.setZ(mlcCentrePosZa);
            m_y1_leaves.push_back(
                    std::make_unique<G4PVPlacement>(leavesOrientation1, leafOneCentre3Vec, name, leafLV, mlcWorldPV, false, i));
            m_y2_leaves.push_back(
                    std::make_unique<G4PVPlacement>(leavesOrientation3, leafTwoCentre3Vec, name, leafLV, mlcWorldPV, false, i));

            G4double shiftStep;

            /////////////////////////////////////////////////////////////////////////////
            //  The leaves numbered from 0 to 12 $ from 47 to 59 belong to the side leaf class.
            //  Below we exclude rest of the leaves (13 - 46) from the side leaf class.
            /////////////////////////////////////////////////////////////////////////////

            if (i > 12 && i < 47) {
                if (i == 14){

                    /////////////////////////////////////////////////////////////////////////////
                    // Leaf 14 -  Creating a pair of transition leaves - narrow version.
                    /////////////////////////////////////////////////////////////////////////////

                    shiftStep = ((5.05 * mm) + (2.52 * mm))/2.;
                    leafLV = transitionLeafLV2;
                }
                else if (i == 46){

                    /////////////////////////////////////////////////////////////////////////////
                    // Leaf 46 -  Creating a pair of transition leaves - wide version.
                    /////////////////////////////////////////////////////////////////////////////

                    shiftStep = ((5.05 * mm) + (2.52 * mm))/2.;
                    leafLV = transitionLeafLV1;
                }
                else {

                    /////////////////////////////////////////////////////////////////////////////
                    //  Creating a pair of central leaves.
                    /////////////////////////////////////////////////////////////////////////////

                    shiftStep = 2.52 * mm;
                    leafLV = centralLeafLV;
                }
                leafOneCentre3Vec.setY(leafOneCentre3Vec.getY() + shiftStep);
                leafTwoCentre3Vec.setY(leafTwoCentre3Vec.getY() + shiftStep);
            }
            else {

                /////////////////////////////////////////////////////////////////////////////
                //  Creating a pair of side leaves.
                /////////////////////////////////////////////////////////////////////////////

                shiftStep = 5.05 * mm;
                leafLV = sideLeafLV;

                leafOneCentre3Vec.setY(leafOneCentre3Vec.getY() + shiftStep);
                leafTwoCentre3Vec.setY(leafTwoCentre3Vec.getY() + shiftStep);
            }
        }

        /////////////////////////////////////////////////////////////////////////////
        //  The if/else function divides the leaves into even and odd -
        //  - mirrored leaf reflections from the top and bottom of the MLV.
        //  Odd side - bottom.
        /////////////////////////////////////////////////////////////////////////////

        else {
            // std::cout << i << std::endl;
            auto name = "LeafY1PV" + G4String(std::to_string(i));

            /////////////////////////////////////////////////////////////////////////////
            //  Creating a pair leaves on opposite sides of the MLC - corresponding leaves.
            /////////////////////////////////////////////////////////////////////////////

            leafOneCentre3Vec.setZ(mlcCentrePosZb);
            leafTwoCentre3Vec.setZ(mlcCentrePosZb);
            m_y1_leaves.push_back(
                    std::make_unique<G4PVPlacement>(leavesOrientation2, leafOneCentre3Vec, name, leafLV, mlcWorldPV, false, i));
            m_y2_leaves.push_back(
                    std::make_unique<G4PVPlacement>(leavesOrientation4, leafTwoCentre3Vec, name, leafLV, mlcWorldPV, false, i));

            G4double shiftStep;

            /////////////////////////////////////////////////////////////////////////////
            //  The leaves numbered from 0 to 12 $ from 47 to 59 belong to the side leaf class.
            //  Below we exclude rest of the leaves (13 - 46) from the side leaf class.
            /////////////////////////////////////////////////////////////////////////////

            if (i > 12 && i < 47) { // central leafes
                if (i == 13) {

                    /////////////////////////////////////////////////////////////////////////////
                    // Leaf 13 -  Creating a pair of transition leaves - wide version.
                    /////////////////////////////////////////////////////////////////////////////

                    shiftStep = 5.05 * mm;
                    leafLV = transitionLeafLV1;
                }
                else  if (i == 45){

                    /////////////////////////////////////////////////////////////////////////////
                    // Leaf 45 -  Creating a pair of transition leaves - narrow version.
                    /////////////////////////////////////////////////////////////////////////////

                    shiftStep = 2.52 * mm;
                    leafLV = transitionLeafLV2;
                }
                else {

                    /////////////////////////////////////////////////////////////////////////////
                    //  Creating a pair of central leaves.
                    /////////////////////////////////////////////////////////////////////////////

                    shiftStep = 2.52 * mm;
                    leafLV = centralLeafLV;
                }
                leafOneCentre3Vec.setY(leafOneCentre3Vec.getY() + shiftStep);
                leafTwoCentre3Vec.setY(leafTwoCentre3Vec.getY() + shiftStep);
            }
            else {

                /////////////////////////////////////////////////////////////////////////////
                //  Creating a pair of side leaves.
                /////////////////////////////////////////////////////////////////////////////

                shiftStep = 5.05 * mm;
                leafLV = sideLeafLV;

                leafOneCentre3Vec.setY(leafOneCentre3Vec.getY() + shiftStep);
                leafTwoCentre3Vec.setY(leafTwoCentre3Vec.getY() + shiftStep);
            }
        }
   }
   mlcWorldPV->CheckOverlaps();


   return mlcWorldPV;
}

/////////////////////////////////////////////////////////////////////////////
//  Giving the shape to the central leaf.
/////////////////////////////////////////////////////////////////////////////

G4VSolid* MlcHd120::CreateCentralLeafShape() const {

    G4double leafWidth = 3.22 * mm;

    auto cylinderCentralLeaf = new G4Tubs("CentralLeafTube",
                                               m_innerRadius / 1.,
                                               m_cylinderRadius / 1.,
                                               leafWidth / 2.,
                                               0 / 1.,
                                               2 * M_PI / 1.);


    auto boxCentralLeaf = new G4Box("CentralLeafBox",
                                         m_leafLength / 2.,
                                         m_leafHeight / 2.,
                                         (2 * cm + leafWidth / 2.));


    auto boxCentralLeaf2 = new G4Box("CentralLeafBox2",
                                          ((4. * cm) + m_leafLength / 2.),
                                          m_leafHeight / 2.,
                                          leafWidth / 2.);


    auto translation1 = G4ThreeVector( 1.5 * cm,  0., 0.);

    auto intersectionCentralLeaf  = new G4IntersectionSolid("PrimalCentralLeaf",
                                                              cylinderCentralLeaf,
                                                              boxCentralLeaf,
                                                              0,
                                                              translation1);


    auto translation2 = G4ThreeVector(0. * cm, 3.5 * cm,  2.52 * mm);

    auto halfDoneCentralLeaf = new G4SubtractionSolid("HalfDoneCentralLeaf",
                                                        intersectionCentralLeaf,
                                                        boxCentralLeaf2,
                                                        0,
                                                        translation2);


    auto translation3 = G4ThreeVector(0. * cm, 3.5 * cm, - 2.52 * mm);
    auto almostDoneCentralLeaf = new G4SubtractionSolid("CentralLeaf0",
                                                          halfDoneCentralLeaf,
                                                          boxCentralLeaf2,
                                                          0,
                                                          translation3);


    auto endCapBox = CreateEndCapCutBox();


    auto translation4 = G4ThreeVector(-10. * cm, 0. * cm, 0. * mm);

    auto centralLeaf = new G4SubtractionSolid("CentralLeaf",
                                                 almostDoneCentralLeaf,
                                                 endCapBox,
                                                 0,
                                                 translation4);


    return centralLeaf;
}

/////////////////////////////////////////////////////////////////////////////
//  Giving the shape to the side leaf.
/////////////////////////////////////////////////////////////////////////////

G4VSolid* MlcHd120::CreateSideLeafShape() const {

    G4double leafWidth = 6.45 * mm;

    auto cylinderSideLeaf = new G4Tubs("SideLeafTube",
                                            m_innerRadius / 1.,
                                            m_cylinderRadius / 1.,
                                            leafWidth / 2.,
                                            0 / 1.,
                                            2 * M_PI / 1.);


    auto boxSideLeaf = new G4Box("SideLeafBox",
                                      m_leafLength / 2.,
                                      m_leafHeight / 2.,
                                      (2 * cm + leafWidth / 2.));


    auto boxSideLeaf2 = new G4Box("SideLeafBox",
                                       ((4. * cm) + m_leafLength / 2.),
                                       m_leafHeight / 2.,
                                       leafWidth / 2.);


    auto interTranslation = G4ThreeVector( 1.5 * cm,0., 0.);

    auto intersectionSideLeaf  = new G4IntersectionSolid("PrimalSideLeaf",
                                                               cylinderSideLeaf,
                                                               boxSideLeaf,
                                                               0,
                                                               interTranslation);


    auto outerTranslation1 = G4ThreeVector(0.* cm, 3.5 * cm, 5.05 * mm);

    auto halfDoneSideLeaf = new G4SubtractionSolid("HalfDoneSideLeaf",
                                                        intersectionSideLeaf,
                                                        boxSideLeaf2,
                                                       0,
                                                        outerTranslation1);


    auto outerTranslation2 = G4ThreeVector(0.* cm, 3.5 * cm, -5.05 * mm);

    auto almostDoneSideLeaf = new G4SubtractionSolid("SideLeaf0",
                                                 halfDoneSideLeaf ,
                                                 boxSideLeaf2,
                                                0,
                                                 outerTranslation2);


    auto endCapBox = CreateEndCapCutBox();


    auto translation = G4ThreeVector(-10. * cm, 0. * cm, 0. * mm);

    auto sideLeaf = new G4SubtractionSolid("SideLeaf",
                                                almostDoneSideLeaf,
                                                endCapBox,
                                                0,
                                                translation);


    return sideLeaf;
}

/////////////////////////////////////////////////////////////////////////////
//  Giving the shape to the transition leaf.
/////////////////////////////////////////////////////////////////////////////

G4VSolid* MlcHd120::CreateTransitionLeafShape(const std::string& type) const{

    G4VSolid* transitionLeaf;

    if (type.compare("Side") == 0){

        G4double leafWidth = 6.1 * mm;

        auto cylinderTransitionLeaf = new G4Tubs("TransitionLeafTube",
                                           m_innerRadius / 1.,
                                           m_cylinderRadius / 1.,
                                           leafWidth / 2.,
                                           0 / 1.,
                                           2 * M_PI / 1.);


        auto boxTransitionLeaf = new G4Box("TransitionLeafBox",
                                     m_leafLength / 2.,
                                     m_leafHeight / 2.,
                                     (2 * cm + leafWidth / 2.));


        auto boxTransitionLeaf2 = new G4Box("TransitionLeafBox",
                                      ((4. * cm) + m_leafLength / 2.),
                                      m_leafHeight / 2.,
                                      leafWidth / 2.);


        auto interTranslation = G4ThreeVector( 1.5 * cm,0., 0.);

        auto intersectionTransitionLeaf  = new G4IntersectionSolid("PrimalTransitionLeaf",
                                                             cylinderTransitionLeaf,
                                                             boxTransitionLeaf,
                                                             0,
                                                             interTranslation);


        auto outerTranslation1 = G4ThreeVector(0.* cm, 3.5 * cm, 4.70 * mm);

        auto halfDoneTransitionLeaf = new G4SubtractionSolid("HalfDoneTransitionLeaf",
                                                       intersectionTransitionLeaf,
                                                       boxTransitionLeaf2,
                                                       0,
                                                       outerTranslation1);


        auto outerTranslation2 = G4ThreeVector(0.* cm, 3.5 * cm, -5.05 * mm);

        auto almostDoneTransitionLeaf = new G4SubtractionSolid("TransitionLeaf0",
                                                         halfDoneTransitionLeaf ,
                                                         boxTransitionLeaf2,
                                                         0,
                                                         outerTranslation2);


        auto endCapBox = CreateEndCapCutBox();


        auto translation = G4ThreeVector(-10. * cm, 0. * cm, 0. * mm);

        auto transitionLeaf = new G4SubtractionSolid("TransitionLeaf",
                                               almostDoneTransitionLeaf,
                                               endCapBox,
                                               0,
                                               translation);


        return transitionLeaf;
    }

    else if (type.compare("Central") == 0){

        G4double leafWidth = 3.57 * mm;

        auto cylinderTransitionLeaf = new G4Tubs("TransitionLeafTube",
                                          m_innerRadius,
                                          m_cylinderRadius,
                                          leafWidth / 2.,
                                          0,
                                          2 * M_PI);


        auto boxTransitionLeaf = new G4Box("TransitionLeafBox",
                                    m_leafLength / 2.,
                                    m_leafHeight / 2.,
                                    (2 * cm + leafWidth / 2.));


        auto boxTransitionLeaf2 = new G4Box("TransitionLeafBox2",
                                     ((4. * cm) + m_leafLength / 2.),
                                     m_leafHeight / 2.,
                                     leafWidth / 2.);


        auto translation1 = G4ThreeVector(1.5 * cm, 0. * cm, 0. * mm);

        auto intersectionTransitionLeaf  = new G4IntersectionSolid("PrimalTransitionLeaf",
                                                            cylinderTransitionLeaf,
                                                            boxTransitionLeaf,
                                                            0,
                                                            translation1);


        auto translation2 = G4ThreeVector(0. * cm, 3.5 * cm, 2.52 * mm);

        auto halfDoneTransitionLeaf = new G4SubtractionSolid("HalfDoneTransitionLeaf",
                                                      intersectionTransitionLeaf,
                                                      boxTransitionLeaf2,
                                                      0,
                                                      translation2);


        auto translation3 = G4ThreeVector(0. * cm, 3.5 * cm, - 2.87 * mm);

        auto almostDoneTransitionLeaf = new G4SubtractionSolid("TransitionLeaf0",
                                                        halfDoneTransitionLeaf,
                                                        boxTransitionLeaf2,
                                                        0,
                                                        translation3);

        auto endCapBox = CreateEndCapCutBox();


        auto translation4 = G4ThreeVector(-10. * cm, 0. * cm, 0. * mm);

        auto transitionLeaf = new G4SubtractionSolid("TransitionLeaf",
                                              almostDoneTransitionLeaf,
                                              endCapBox,
                                              0,
                                              translation4);


        return transitionLeaf;
    }
}

////////////////////////////////////////////////////////////////////////////////
///
G4VSolid* MlcHd120::CreateEndCapCutBox() const {
    return new G4Box("EndCapCutBox", 10. * cm, 10. * cm, 10. * cm);
}

////////////////////////////////////////////////////////////////////////////////
///
G4bool MlcHd120::Update(){
    // implement me.
}

////////////////////////////////////////////////////////////////////////////////
///
void MlcHd120::Reset(){
    // implement me.
}

////////////////////////////////////////////////////////////////////////////////
///
void MlcHd120::WriteInfo(){
    // implement me.
}

void MlcHd120::SetRunConfiguration(const G4Run* runPtr){

    auto inputType = thisConfig()->GetValue<std::string>("PositionningFileType");
    G4cout << "[INFO]:: MlcHd120:: the run configuration type: "<< inputType << G4endl;

    if(inputType=="Custom"){
        auto flsz = std::string("3x3"); // temporary fixed; it should come from GeoSvc or RunSvc
        SetCustomPositioning(flsz);
    }
    else if(inputType=="RTPlan"){
        auto beamId = int(0);         // temporary fixed; it will come from LinacRun instance
        auto controlPointId = int(0); // temporary fixed; it will come from LinacRun instance
        SetRTPlanPositioning(beamId,controlPointId);
    }
}

////////////////////////////////////////////////////////////////////////////////
///
void MlcHd120::SetCustomPositioning(const std::string& fieldSize){
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

                    //G4cout << "[DEBUG]:: MlcHd120:: config value ("<<count<<")"<< svalue << G4endl;
                }
                //
                // Input data file should contain 2 columns with comma separation, verify it:
                if(y1y2_position.size()>2)
                    G4Exception("MlcHd120", "SetCustomPositioning", FatalErrorInArgument, "Wrong input data format!");

                if(leafsCounter>59)
                    G4Exception("MlcHd120", "SetCustomPositioning", FatalErrorInArgument, "To many leafs configuration!");

                auto y1_translation = m_y1_leaves[leafsCounter]->GetTranslation();
                y1_translation.setX(y1_translation.getX()+y1y2_position.at(1));
                m_y1_leaves[leafsCounter]->SetTranslation(y1_translation);

                auto y2_translation = m_y2_leaves[leafsCounter]->GetTranslation();
                y2_translation.setX(y2_translation.getX()+y1y2_position.at(0));
                m_y2_leaves[leafsCounter]->SetTranslation(y2_translation);

                ++leafsCounter;

            }
        }
        if(leafsCounter<59)
            G4Exception("MlcHd120", "SetCustomPositioning", FatalErrorInArgument, "To less leafs configuration!");
    }
}

////////////////////////////////////////////////////////////////////////////////
///
void MlcHd120::SetRTPlanPositioning(int current_beam, int current_controlpoint){
    auto dicomSvc = Service<DicomSvc>();
    auto pos1 = dicomSvc->GetRTPlanMlcPossitioning("Y1",current_beam,current_controlpoint);
    auto pos2 = dicomSvc->GetRTPlanMlcPossitioning("Y2",current_beam,current_controlpoint);

    if(pos1.size()!=pos2.size() || pos1.size()!=60){
        G4cout << "[DEBUG]:: MlcHd120:: posY1.size() " << pos1.size() << G4endl;
        G4cout << "[DEBUG]:: MlcHd120:: posY2.size() " << pos2.size() << G4endl;
        G4Exception("MlcHd120", "SetRTPlanPositioning", FatalErrorInArgument, "Wrong MLC positioning data retrieved!");
    }

    for(int i=0; i<pos1.size(); ++i){
        G4cout << "[DEBUG]:: MlcHd120:: Y1 "<<pos1.at(i)<<", Y2 "<< pos2.at(i) << G4endl;
        // overlap check:
        if(pos2.at(i)-pos1.at(i)<0){
            G4cout << "[WARNING]:: MlcHd120:: OVERLAP Y1 "<<pos1.at(i)<<", Y2 "<< pos2.at(i) << G4endl;
            pos1[i] = 0;
            pos2[i] = 0;
        }

        auto y1_translation = m_y1_leaves[i]->GetTranslation();
        y1_translation.setX(y1_translation.getX()+pos2.at(i) );
        m_y1_leaves[i]->SetTranslation(y1_translation);

        auto y2_translation = m_y2_leaves[i]->GetTranslation();
        y2_translation.setX(y2_translation.getX()+pos1.at(i) );
        m_y2_leaves[i]->SetTranslation(y2_translation);
    }
}

