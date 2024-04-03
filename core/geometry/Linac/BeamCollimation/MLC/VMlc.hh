#ifndef VMLC_HH
#define VMLC_HH

#include "TomlConfigurable.hh"
#include "G4VPhysicalVolume.hh"

class ControlPoint;

class VMlc: public TomlConfigurable {

    protected:
        std::vector<G4VPhysicalVolumeUPtr> m_y1_leaves;
        std::vector<G4VPhysicalVolumeUPtr> m_y2_leaves;
        ControlPoint* m_control_point = nullptr;
        bool m_isInitialized = false;
    public:
        VMlc() = delete;
        explicit VMlc(const std::string& name) : TomlConfigurable(name) {};
        virtual ~VMlc() = default;
        void ParseTomlConfig() override {};
        virtual bool IsInField(const G4ThreeVector& vertexPosition) { return false;}

    
};
#endif // VMLC_HH