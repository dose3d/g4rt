#include "IRunConfigurable.hh"

std::vector<IRunConfigurable*> IRunConfigurable::m_run_configurable_modules;

////////////////////////////////////////////////////////////////////////////////
///
void IRunConfigurable::AddRunConfigurableModule(IRunConfigurable* module){
    IRunConfigurable::m_run_configurable_modules.push_back(module);
}

////////////////////////////////////////////////////////////////////////////////
///
void IRunConfigurable::SetRunConfigurationForAllModules(const G4Run* aRun){
    for(const auto& module : IRunConfigurable::m_run_configurable_modules)
        module->SetRunConfiguration(aRun);
}