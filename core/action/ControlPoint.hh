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

class ControlPointConfig {
  public:
    ControlPointConfig() = default;
    ControlPointConfig(int id, int nevts, int rot);
    double RotationInDeg = 0.;
    int NEvts = 0.;
    int Id = 0;
    std::string RTPlanFile = std::string();
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
    
    // G4ThreeVector GetWeightedActivityGeoCentre(const std::map<std::size_t, VoxelHit>& data)
    void DumpVolumeMaskToFile(std::string scoring_vol_name, const std::map<std::size_t, VoxelHit>& volume_scoring) const;
    std::string GetSimOutputTFileName() const;
    void WriteIntegratedDoseToFile(bool tfile=true, bool csv=true) const;

    // Type of the FieldMask export: 
    // Plan = export points distribution based on the plan/beam shape data
    // Sim  = export points distribution based on simulated particles
    //void WriteFieldMaskToFile(std::string type="Plan", bool tfile=true, bool csv=true) const;
    void WriteFieldMaskToTFile() const;
    void WriteFieldMaskToCsv() const;

    void WriteVolumeFieldMaskToTFile();
    void WriteVolumeFieldMaskToCsv();

    void WriteIntegratedDoseToTFile();
    void WriteIntegratedDoseToCsv();

    void ClearCachedData();

    static void IntegrateAndWriteTotalDoseToTFile();
    static void IntegrateAndWriteTotalDoseToCsv();

    static std::string GetOutputDir();

  private:
    ControlPointConfig m_config;
    static std::string m_sim_dir;
    std::vector<std::string> m_data_types={"Plan", "Sim"};
    std::set<Scoring::Type> m_scoring_types={Scoring::Type::Cell, Scoring::Type::Voxel};

    G4RotationMatrix* m_rotation = nullptr;

    std::vector<double> m_mlc_a_positioning;
    std::vector<double> m_mlc_b_positioning;

    std::vector<G4ThreeVector> m_plan_mask_points;
    G4VectorCache<G4ThreeVector> m_sim_mask_points;

    std::map<Scoring::Type, std::map<std::size_t, VoxelHit>> m_hashed_scoring_map;
    bool m_is_scoring_data_filled = false;

    static double FIELD_MASK_POINTS_DISTANCE;

    G4bool IsInField(const G4ThreeVector& position, G4bool transformedToMaskPosition) const;
    void FillPlanFieldMask();
    void FillPlanFieldMaskForRegularShapes(const std::string& shape);
    void FillPlanFieldMaskFromRTPlan();
    void FillScoringData();
    void FillScoringDataTagging();
    std::string GetOutputFileName() const;
};

#endif //Dose3D_ControlPoint_H