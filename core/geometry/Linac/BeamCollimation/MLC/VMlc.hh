#include "TomlConfigModule.hh"
#include "Logable.hh"

class vMlc :public Logable, public Configurable {

    public:
        vMlc() = delete;
        explicit vMlc(const std::string& name):Configurable(name),Logable(){}
        ~vMlc() = default;

    
};