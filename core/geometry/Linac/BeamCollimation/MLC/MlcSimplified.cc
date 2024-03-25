#include <vector>
#include "Services.hh"
#include "BeamCollimation.hh"
#include "MlcSimplified.hh"



bool MlcSimplified::IsInField(const G4ThreeVector& vertexPosition){
    auto getFieldAB = [=](G4double zPosition) {
        auto fieldSize_a = Service<ConfigSvc>()->GetValue<double>("RunSvc", "FieldSizeA");
        auto fieldSize_b = Service<ConfigSvc>()->GetValue<double>("RunSvc", "FieldSizeB");
        auto ssd = 1000.0;
        if(fieldSize_b == 0)
            fieldSize_b = fieldSize_a;
        return std::make_pair( ((zPosition / ssd ) * fieldSize_a / 2.), ((zPosition / ssd) * fieldSize_b / 2.) );
    };

    auto cutFieldParam = getFieldAB(vertexPosition.getZ());
    G4double cutFieldSize_a = cutFieldParam.first;
    G4double cutFieldSize_b = cutFieldParam.second;

    auto fieldshape = Service<ConfigSvc>()->GetValue<std::string>("RunSvc", "FieldShape");
    if (fieldshape == "Rectangular"){
        if (vertexPosition.x()<=-cutFieldSize_a || vertexPosition.x() >= cutFieldSize_a || vertexPosition.y()<=-cutFieldSize_b || vertexPosition.y() >= cutFieldSize_b ){
            return true;
        }
    }
    if (fieldshape == "Elipsoidal"){
        if ((pow(vertexPosition.x(),2)/ pow(cutFieldSize_a,2) + pow(vertexPosition.y(),2)/pow(cutFieldSize_b,2))>= 1 ){
            std::cout << "\n[INFO] MlcSimplified::IsInField and elipsoidal\n" << vertexPosition <<  std::endl;
            return true;
        }
    }
    if (fieldshape == "RTPlan"){
        // just exist - for now
        // TODO: implement me
    }

}

