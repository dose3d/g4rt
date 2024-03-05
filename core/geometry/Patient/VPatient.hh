//
// Created by brachwal on 28.04.2020.
//

#ifndef DOSE3DVPATIENT_HH
#define DOSE3DVPATIENT_HH

#include "G4Cache.hh"
#include "IPhysicalVolume.hh"
#include "TomlConfigModule.hh"
#include "Logable.hh"
#include "VoxelHit.hh"
#include <map>

class VPatientSD;

class VPatient : public IPhysicalVolume, public TomlConfigModule, public Logable {
  protected:
    ///
    bool m_tracks_analysis = false;

    ///
    void SetSensitiveDetector(const G4String& logicalVName, VPatientSD* sensitiveDetectorPtr);

    ///
    virtual void ConstructSensitiveDetector(){};

    /// Pointer to the sensitive detectors, wrapped into the MT service
    G4Cache<VPatientSD*> m_patientSD;

  public:
    ///
    VPatient() = delete;
    
    ///
    explicit VPatient(const std::string& name):IPhysicalVolume(name),TomlConfigModule(name),Logable("GeoAndScoring"){
      m_patientSD.Put(nullptr);
    }

    ///
    ~VPatient() = default;

    ///
    static std::set<G4String> HitsCollections;

    ///
    void SetTracksAnalysis(bool flag) {m_tracks_analysis = flag; }

    ///
    VPatientSD* GetSD() const { return m_patientSD.Get(); }

    virtual std::map<std::size_t, VoxelHit> GetScoringHashedMap(const G4String&,Scoring::Type) const {
      LOGSVC_WARN("Returning empty scoring hashed map!");
      return std::map<std::size_t, VoxelHit>();
    }


};
#endif //Dose3DVPatient_HH
