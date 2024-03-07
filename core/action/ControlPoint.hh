/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 29.03.2023
*
*/

#ifndef Dose3D_ControlPoint_H
#define Dose3D_ControlPoint_H

#include "Types.hh"
#include "VoxelHit.hh"
#include "TFile.h"
#include "G4Cache.hh"
#include "VPatient.hh"
#include "G4Run.hh"
#include "VoxelHit.hh"

typedef std::map<Scoring::Type, std::map<std::size_t, VoxelHit>> ScoringMap;

class ControlPoint;

class ControlPointConfig {
  public:
    ControlPointConfig() = default;
    ControlPointConfig(int id, int nevts, double rot);
    double RotationInDeg = 0.;
    int NEvts = 0.;
    int Id = 0;
    std::string RTPlanFile = std::string();
};

class ControlPointRun : public G4Run {
  private:
    ///
    mutable std::map<G4String,ScoringMap> m_hashed_scoring_map;
    
    ///
    void InitializeScoringCollection();

    ///
    void FillDataTagging();

  public:
    ControlPointRun(bool scoring=false) {
      if(scoring)
        InitializeScoringCollection();
    };

    ~ControlPointRun(){
      G4cout << "DESTRUCOTOR OF ControlPointRun..." << G4endl;
    };

    ///
    void Merge(const G4Run* aRun) override;

    ///
    ScoringMap& GetScoringCollection(const G4String& name);

    ///
    const std::map<G4String,ScoringMap>& GetScoringCollections() const {return m_hashed_scoring_map;}

    ///
    void EndOfRun();
};

class ControlPoint {
  public:
    ControlPoint(const ControlPointConfig& config);
    ControlPoint(const ControlPoint& cp);
    ControlPoint(ControlPoint&& cp);
    ~ControlPoint();
    int GetId() const { return m_config.Id; }
    int GetNEvts() const { return m_config.NEvts; }
    G4RotationMatrix* GetRotation() const { return m_rotation; }
    G4double GetDegreeRotation() const {return m_config.RotationInDeg;} 
    // void SetRotation(G4RotationMatrix* rot) { m_rotation=rot; }
    void SetRotation(double rotationInDegree);
    void SetNEvts(int nevts) { m_config.NEvts = nevts; }
    // void SetRotationInDegree(G4double rot_deg) {m_degree = rot_deg;}
    G4bool IsInField(const G4ThreeVector& position) const; 
    G4double GetInFieldMaskTag(const G4ThreeVector& position) const;
    G4ThreeVector TransformToMaskPosition(const G4ThreeVector& position) const;

    const std::vector<G4ThreeVector>& GetFieldMask(const std::string& type="Plan") const;
    
    void DumpVolumeMaskToFile(std::string scoring_vol_name, const std::map<std::size_t, VoxelHit>& volume_scoring) const;
    std::string GetSimOutputTFileName(bool workerMT = false) const;

    static std::string GetOutputDir();

    G4Run* GenerateRun(bool scoring=false);

    ControlPointRun* GetRun() {return m_cp_run.Get();}

    std::string GetOutputFileName() const;

    const std::vector<std::string>& DataTypes() const { return m_data_types; }

    void FillEventCollections(G4HCofThisEvent* evtHC);

  private:
    friend class ControlPointRun;
    friend class VPatientSD;

    ControlPointConfig m_config;
    static std::string m_sim_dir;
    std::vector<std::string> m_data_types={"Plan", "Sim"};

    G4RotationMatrix* m_rotation = nullptr;

    std::vector<double> m_mlc_a_positioning;
    std::vector<double> m_mlc_b_positioning;

    std::set<Scoring::Type> m_scoring_types;

    std::vector<G4ThreeVector> m_plan_mask_points;
    G4VectorCache<G4ThreeVector> m_sim_mask_points;

    /// Store to kepp raw pointers from ControlPoint::GenerateRun
    std::vector<ControlPointRun*> m_mt_run;

    /// MTRunManager generates new run on each thread
    G4Cache<ControlPointRun*> m_cp_run;

    /// Many HitsCollections can be associated to given collection name 
    // (e.g. when many sensitive detectors constituting a single detection unit)
    static std::map<G4String,std::vector<G4String>> m_run_collections;
    static void RegisterRunHCollection(const G4String& collection_name, const G4String& hc_name);

    static double FIELD_MASK_POINTS_DISTANCE;

    G4bool IsInField(const G4ThreeVector& position, G4bool transformedToMaskPosition) const;
    void FillPlanFieldMask();
    void FillPlanFieldMaskForRegularShapes(const std::string& shape);
    void FillPlanFieldMaskFromRTPlan();
    void FillScoringDataTagging(ScoringMap* scoring_data = nullptr);
    void FillEventCollection(const G4String& run_collection, VoxelHitsCollection* hitsColl);

};

#endif //Dose3D_ControlPoint_H