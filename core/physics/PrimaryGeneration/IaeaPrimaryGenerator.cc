#include "IaeaPrimaryGenerator.hh"
#include "G4IAEAphspReader.hh"
#include "Services.hh"
#include "PrimariesAnalysis.hh"
#include "G4Event.hh"


G4IAEAphspReader* IaeaPrimaryGenerator::m_iaeaFileReader = nullptr;

////////////////////////////////////////////////////////////////////////////////
///
IaeaPrimaryGenerator::IaeaPrimaryGenerator(const G4String& fileName) {

  if(!m_iaeaFileReader){ 

    auto id_thread = G4Threading::G4GetThreadId();
    auto max_thread = G4Threading::GetNumberOfRunningWorkerThreads();

    m_iaeaFileReader = new G4IAEAphspReader(fileName.data());
    m_iaeaFileReader->SetParallelRun(1 + id_thread);
    m_iaeaFileReader->SetTotalParallelRuns(max_thread);

    // define the position of the isocenter
    // it’s mandatory before rotations around this point
    m_iaeaFileReader->SetIsocenterPosition(G4ThreeVector(0., 0., 0. * cm));
  
    // phase-space plane shift
    m_phspShiftZ = Service<ConfigSvc>()->GetValue<double>("RunSvc", "phspShiftZ");
    m_iaeaFileReader->SetGlobalPhspTranslation(G4ThreeVector(0.,0.,-m_phspShiftZ));
    G4cout << "[INFO]:: IaeaPrimaryGenerator:: Setting phase space Z position " << m_phspShiftZ / cm  << " [cm]"<< G4endl;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void IaeaPrimaryGenerator::GeneratePrimaryVertex(G4Event *evt) {
  // the actual generation is defined in G4IAEAphspReader, so it's being delegated:
  m_iaeaFileReader->GeneratePrimaryVertex(evt);
}

////////////////////////////////////////////////////////////////////////////////
///
std::vector<G4PrimaryVertex*> IaeaPrimaryGenerator::GeneratePrimaryVertex(G4int evtId){
  return m_iaeaFileReader->GeneratePrimaryVertex(evtId);
}