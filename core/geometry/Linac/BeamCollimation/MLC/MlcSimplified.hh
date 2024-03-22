
#ifndef DOSE3DMLCSIMPLIFIED_HH
#define DOSE3DMLCSIMPLIFIED_HH

#include "VMlc.hh"

class MlcSimplified : public VMlc {

public:
    MlcSimplified() : VMlc("MlcGhost"){};
    ~MlcSimplified() override {};	
    bool IsInField(const G4ThreeVector& vertexPosition);
    void Configure() override {};
    void DefaultConfig(const std::string &unit) override {};
    void ParseTomlConfig() override {};

};

#endif //DOSE3DMLCSIMPLIFIED_HH
