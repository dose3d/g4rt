#include <vector>
#include "Services.hh"
#include "BeamCollimation.hh"
#include "MlcSimplified.hh"


MlcSimplified::MlcSimplified() : VMlc("Simplified"){
    m_fieldShape = Service<ConfigSvc>()->GetValue<std::string>("RunSvc", "FieldShape");
};

void MlcSimplified::Initialize(const G4ThreeVector& vertexPosition){
    auto getScaledFieldAB = [=](G4double zPosition) {
        auto fieldSize_a = Service<RunSvc>()->CurrentControlPoint()->GetFieldSizeA();
        auto fieldSize_b = Service<RunSvc>()->CurrentControlPoint()->GetFieldSizeB();
        auto ssd = 1000.0;
        if(fieldSize_b == 0)
            fieldSize_b = fieldSize_a;
        
        return std::make_pair( ((zPosition / ssd ) * fieldSize_a / 2.), ((zPosition / ssd) * fieldSize_b / 2.) );
    };

    auto cutFieldParam = getScaledFieldAB(vertexPosition.getZ());
    m_fieldParamA = cutFieldParam.first;
    m_fieldParamB = cutFieldParam.second;
    m_isInitialized = true;
}


bool MlcSimplified::IsInField(const G4ThreeVector& vertexPosition) {
    if(!m_isInitialized)
        Initialize(vertexPosition);

    // std::cout << "Field: " << m_fieldParamA << " " << m_fieldParamB << std::endl;
    if (m_fieldShape == "Rectangular"){
        // std::cout << "In field Vertex: " << vertexPosition << " " << m_fieldParamA << " " << m_fieldParamB <<   std::endl;
        if (vertexPosition.x()<=-m_fieldParamA || vertexPosition.x() >= m_fieldParamA || vertexPosition.y()<=-m_fieldParamB || vertexPosition.y() >= m_fieldParamB ){
            return false;
        } else {
        return true;
        }
    
    }
    if (m_fieldShape == "Elipsoidal"){
        if ((pow(vertexPosition.x(),2)/ pow(m_fieldParamA,2) + pow(vertexPosition.y(),2)/pow(m_fieldParamB,2))>= 1 ){
            return false;
        } else return true;
    }
    if (m_fieldShape == "RTPlan"){
        // just exist - for now
        // TODO: implement me
    }
    return false;
}

