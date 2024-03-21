#include "TomlConfigModule.hh"
#include "Logable.hh"

class VMlc: public TomlConfigurable {

    protected:
    std::vector<G4VPhysicalVolumeUPtr> m_y1_leaves;
    std::vector<G4VPhysicalVolumeUPtr> m_y2_leaves;


    private:
        void Configure() override;
        VMlc() = delete;
        ~VMlc() = default;
    public:
        explicit VMlc(const std::string& name):TomlConfigurable(name){}
        void ParseTomlConfig() override {}
    
}