#include <vector>
#include "Services.hh"
#include "BeamCollimation.hh"
#include "MlcSimplified.hh"


MlcSimplified::MlcSimplified() : VMlc("Simplified"){
    // Odwoływanie się do skonfigurowanego parametru zamiast odczytywanie nieistniejącej wartości. 
    m_fieldShape = Service<RunSvc>()->CurrentControlPoint()->GetFieldShape();
};

void MlcSimplified::Initialize(const G4ThreeVector& vertexPosition){
    auto getScaledFieldAB = [=](G4double zPosition) {
        auto fieldSize_a = Service<RunSvc>()->CurrentControlPoint()->GetFieldSizeA();
        auto fieldSize_b = Service<RunSvc>()->CurrentControlPoint()->GetFieldSizeB();
        auto ssd = 1000.0;
        if(fieldSize_b == 0)
            fieldSize_b = fieldSize_a;
        
        // Kierunki poprawione... 
        return std::make_pair( (((ssd - zPosition )/ ssd ) * fieldSize_a / 2.), (((ssd - zPosition )/ ssd) * fieldSize_b / 2.) );
    };

    auto cutFieldParam = getScaledFieldAB(vertexPosition.getZ());
    m_fieldParamA = cutFieldParam.first;
    m_fieldParamB = cutFieldParam.second;
    m_isInitialized = true;
}


bool MlcSimplified::IsInField(const G4ThreeVector& vertexPosition) {
    if(!m_isInitialized)
        Initialize(vertexPosition);
    // Uproszczenie składni, poprawa czytelności.
    if (m_fieldShape == "Rectangular"){
        if (abs(vertexPosition.x())<=m_fieldParamA && abs(vertexPosition.y())<= m_fieldParamB)
        return true;
    }
    if (m_fieldShape == "Elipsoidal"){
        if ((pow(vertexPosition.x(),2)/ pow(m_fieldParamA,2) + pow(vertexPosition.y(),2)/pow(m_fieldParamB,2))<= 1)
        return true;        
    }
    if (m_fieldShape == "RTPlan"){ // TEMP SOLUTION!
        if (abs(vertexPosition.x())<=m_fieldParamA && abs(vertexPosition.y())<= m_fieldParamB)
        return true; 

        /*
        #include <vector>
        #include <utility>
        bool isPointInPolygon(const std::pair<double, double>& point, const std::vector<std::pair<double, double>>& polygon) {
            bool inside = false;
            for (int i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
                if ((polygon[i].second > point.second) != (polygon[j].second > point.second) &&
                    (point.first < (polygon[j].first - polygon[i].first) * (point.second - polygon[i].second) / (polygon[j].second - polygon[i].second) + polygon[i].first)) {
                    inside = !inside;
                }
            }
            return inside;
        }

        TODO:
        Calc in 2D plane position of corners in MLC.
        Each corner as a point in polygon.
        

        */


    }
    return false;
}

