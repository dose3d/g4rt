#include <vector>
#include "Services.hh"
#include "BeamCollimation.hh"
#include "MlcSimplified.hh"


MlcSimplified::MlcSimplified() : VMlc("Simplified"){
    m_fieldShape = Service<ConfigSvc>()->GetValue<std::string>("RunSvc", "FieldShape");
};

void MlcSimplified::Initialize(const G4ThreeVector& vertexPosition){
    auto getScaledFieldAB = [=](G4double zPosition) {
        auto fieldSize_a = Service<ConfigSvc>()->GetValue<double>("RunSvc", "FieldSizeA");
        auto fieldSize_b = Service<ConfigSvc>()->GetValue<double>("RunSvc", "FieldSizeB");
        auto ssd = 1000.0;
        if(fieldSize_b == 0)
            fieldSize_b = fieldSize_a;
        return std::make_pair( ((zPosition / ssd ) * fieldSize_a / 2.), ((zPosition / ssd) * fieldSize_b / 2.) );
    };

    auto cutFieldParam = getScaledFieldAB(vertexPosition.getZ());
    G4double m_fieldParamA = cutFieldParam.first;
    G4double m_fieldParamB = cutFieldParam.second;
    m_isInitialized = true;
}


bool MlcSimplified::IsInField(const G4ThreeVector& vertexPosition) {
    if(!m_isInitialized)
        Initialize(vertexPosition);

    if (m_fieldShape == "Rectangular"){
        if (vertexPosition.x()<=-m_fieldParamA || vertexPosition.x() >= m_fieldParamA || vertexPosition.y()<=-m_fieldParamB || vertexPosition.y() >= m_fieldParamB ){
            return true;
        } else return false;
    }
    if (m_fieldShape == "Elipsoidal"){
        if ((pow(vertexPosition.x(),2)/ pow(m_fieldParamA,2) + pow(vertexPosition.y(),2)/pow(m_fieldParamB,2))>= 1 ){
            return true;
        } else return false;
    }
    if (m_fieldShape == "RTPlan"){
        // just exist - for now
        // TODO: implement me
    }
    return false;
}

