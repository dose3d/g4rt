#ifndef VMLC_HH
#define VMLC_HH

#include "TomlConfigurable.hh"
#include "G4VPhysicalVolume.hh"
#include "Types.hh"
class ControlPoint;
class G4PrimaryVertex;

class VMlc: public TomlConfigurable {

    protected:
        std::vector<G4VPhysicalVolumeUPtr> m_y1_leaves;
        std::vector<G4VPhysicalVolumeUPtr> m_y2_leaves;
        ControlPoint* m_control_point = nullptr;
        bool m_isInitialized = false;
        static G4ThreeVector m_isocentre;
    public:
        VMlc() = delete;
        explicit VMlc(const std::string& name);
        virtual ~VMlc() = default;
        void ParseTomlConfig() override {};
        virtual bool IsInField(const G4ThreeVector& position, bool transformToIsocentre=false) = 0;
        virtual bool IsInField(G4PrimaryVertex* vrtx) = 0;
        static G4ThreeVector GetPositionInMaskPlane(const G4ThreeVector& position);
        static G4ThreeVector GetPositionInMaskPlane(const G4PrimaryVertex* vrtx);

    
};
#endif // VMLC_HH