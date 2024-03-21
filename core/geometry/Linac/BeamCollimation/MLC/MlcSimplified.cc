#include <vector>
#include "Services.hh"
#include "BeamCollimation.hh"
#include "MlcSimplified.hh"



class MlcSimplified : VMlc("MlcGhost"){

G4ThreeVector CurrentCentre;
auto fieldSize_a = Service<ConfigSvc>()->GetValue<double>("RunSvc", "FieldSizeA");
auto fieldSize_b = Service<ConfigSvc>()->GetValue<double>("RunSvc", "FieldSizeB");

if(fieldSize_b == 0)
    fieldSize_b = fieldSize_a;

auto fieldshape = Service<ConfigSvc>()->GetValue<std::string>("RunSvc", "FieldShape");
double cutFieldSize_a = 0;
double cutFieldSize_b = 0;

if (fieldSize_a>0){ 
    auto fa = 570.0; 
    auto ssd = 1000.;
    cutFieldSize_a = (fa / ssd) * fieldSize_a / 2.;
    cutFieldSize_b = (fa / ssd) * fieldSize_b / 2.;
}

bool MlcSimplified::formField(G4ThreeVector centre){
    if (fieldshape == "Rectangular"){
        if (CurrentCentre.x()<=-cutFieldSize_a || CurrentCentre.x() >= cutFieldSize_a || CurrentCentre.y()<=-cutFieldSize_b || CurrentCentre.y() >= cutFieldSize_b ){
            return false;
        }
    }
    if (fieldshape == "Elipsoidal"){
        if ((pow(CurrentCentre.x(),2)/ pow(cutFieldSize_a,2) + pow(CurrentCentre.y(),2)/pow(cutFieldSize_b,2))>= 1 ){
            return false;
        }
    }
    if (fieldshape == "RTPlan"){
        // just exist - for now
        // TODO: implement me
    }

}

}
