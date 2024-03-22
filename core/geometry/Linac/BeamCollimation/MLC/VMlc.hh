#ifndef VMLC_HH
#define VMLC_HH

#include "TomlConfigurable.hh"
#include "G4VPhysicalVolume.hh"


class VMlc: public TomlConfigurable {

    protected:
    std::vector<G4VPhysicalVolumeUPtr> m_y1_leaves;
    std::vector<G4VPhysicalVolumeUPtr> m_y2_leaves;

    public:
        VMlc() = delete;
        explicit VMlc(const std::string& name) : TomlConfigurable(name) {};
        virtual ~VMlc() = default;
        void ParseTomlConfig() override {};
    
};
#endif // VMLC_HH