
#ifndef DOSE3DMLCSIMPLIFIED_HH
#define DOSE3DMLCSIMPLIFIED_HH

#include "VMlc.hh"

class MlcSimplified : public VMlc {
    private:
        std::string m_fieldShape;
        G4double m_fieldParamA = 0.0;
        G4double m_fieldParamB = 0.0;
        void Initialize(const G4ThreeVector& vertexPosition);
        std::vector<std::pair<double,double>> m_mlc_a_corners;
        std::vector<std::pair<double,double>> m_mlc_b_corners;
        std::vector<std::pair<double,double>> m_mlc_corners;
public:
    MlcSimplified();
    ~MlcSimplified() override {};	
    bool IsInField(const G4ThreeVector& position) override;
    bool IsInField(G4PrimaryVertex* vrtx) override;
    void Configure() override {};
    void DefaultConfig(const std::string &unit) override {};
    void ParseTomlConfig() override {};

};

#endif //DOSE3DMLCSIMPLIFIED_HH
