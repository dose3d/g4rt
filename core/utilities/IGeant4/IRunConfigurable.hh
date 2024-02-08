/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 24.09.2020
*
*/

#ifndef Dose3D_IRUNCONFIGURABLE_HH
#define Dose3D_IRUNCONFIGURABLE_HH

#include <vector>

class G4Run;

class IRunConfigurable {
  public:
    ///
    IRunConfigurable() = default;
    
    ///
    virtual ~IRunConfigurable() = default;

    /// TODO: G4Run to be replaced with LinacRun
    virtual void SetRunConfiguration(const G4Run*)=0; 

    ///
    static void AddRunConfigurableModule(IRunConfigurable* module);
  
    /// TODO: G4Run to be replaced with LinacRun
    static void SetRunConfigurationForAllModules(const G4Run* aRun);

  private:
    ///
    static std::vector<IRunConfigurable*> m_run_configurable_modules;

};

#endif  // Dose3D_IRUNCONFIGURABLE_HH
