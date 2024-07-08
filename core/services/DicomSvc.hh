/**
*
* \author B.Rachwal (brachwal@agh.edu.pl), J.Hajduga
* \date 10.08.2020
*
*/

#ifndef Dose3D_DicomSvcSVC_H
#define Dose3D_DicomSvcSVC_H

#include "Types.hh"
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include "ControlPoint.hh"

namespace py = pybind11;
using namespace py::literals;

////////////////////////////////////////////////////////////////////////////////
/// 
class ICtSvc {
  
  private:
    py::object m_py_dicom_ct;
  public:
    ///
    ICtSvc():m_py_dicom_ct(py::reinterpret_borrow<py::object>(py::module::import("dicom_ct").attr("CtSvc")().ptr())) {
    }
    ///
    ~ICtSvc(){
      m_py_dicom_ct.release();
    }
    void set_paths(const std::string& output_path) const{
      m_py_dicom_ct.attr("set_output_path")(output_path);
      m_py_dicom_ct.attr("set_project_path")(PROJECT_DATA_PATH);
    }
    ///
    void create_ct_series(const std::string& series_csv_path) const{
      m_py_dicom_ct.attr("create_ct_series")(series_csv_path);
    }
};

////////////////////////////////////////////////////////////////////////////////
/// 
class IPlan {
  public:
    virtual ControlPointConfig GetControlPointConfig(int controlpointIdx, const std::string& planFile) = 0;
    virtual std::pair<double,double> ReadJawsAperture(const std::string& planFile,const std::string& side,int beamIdx, int controlpointIdx) = 0;
    virtual double ReadJawPossition(const std::string& planFile, const std::string& jawName, int beamIdx, int controlpointIdx) const = 0;
    virtual std::vector<G4double> ReadMlcPositioning(const std::string& planFile, const std::string& side, int beamIdx, int controlpointIdx) = 0;
    void AcknowledgeJawsAperture(const std::string& side, const std::pair<double,double>& jawsAperture) const;
    void AcknowledgeMlcPositioning(const std::string& side, const std::vector<G4double>& mlc_positioning) const;

};

////////////////////////////////////////////////////////////////////////////////
/// 
class IDicomPlan: public IPlan {
  private:
    double ReadJawPossition(const std::string& planFile, const std::string& jawName, int beamIdx, int controlpointIdx) const override;
  public:
    ControlPointConfig GetControlPointConfig(int id, const std::string& planFile) override;
    std::pair<double,double> ReadJawsAperture(const std::string& planFile,const std::string& side,int beamIdx, int controlpointIdx) override;
    std::vector<G4double> ReadMlcPositioning(const std::string& planFile, const std::string& side, int beamIdx, int controlpointIdx) override;
};
////////////////////////////////////////////////////////////////////////////////
/// 
class ICustomPlan : public IPlan {
  private:
    // TODO: "Don't repeat yourself" (DRY)...
    int GetNEvents(const std::string& planFile);
    double GetRotation(const std::string& planFile);
    double ReadJawPossition(const std::string& planFile, const std::string& jawName, int beamIdx, int controlpointIdx) const override;

  public:
    ControlPointConfig GetControlPointConfig(int id, const std::string& planFile) override;
    std::pair<double,double> ReadJawsAperture(const std::string& planFile,const std::string& side,int beamIdx=0, int controlpointIdx=0) override;
    std::vector<G4double> ReadMlcPositioning(const std::string& planFile, const std::string& side, int beamIdx=0, int controlpointIdx=0) override;
};

////////////////////////////////////////////////////////////////////////////////
///
class DicomSvc {
  private:
    /// \brief The full path the RT-Plan file
    std::string m_rtplan_file;

    /// \brief Current beam number
    int m_beamId = 0; // temporary fixed

    /// \brief Current control point number
    int m_controlPointId = 0; // temporary fixed

    ///
    int m_rt_plan_n_beams = 0;
    std::vector<int> m_rtplan_beam_n_control_points;

    ///
    DicomSvc() = default;

    ///
    ~DicomSvc() = default;

    /// Delete the copy and move constructors
    DicomSvc(const DicomSvc &) = delete;
    DicomSvc &operator=(const DicomSvc &) = delete;
    DicomSvc(DicomSvc &&) = delete;
    DicomSvc &operator=(DicomSvc &&) = delete;

    ///
    ICtSvc m_ct_svc;

    ///
    std::unique_ptr<IPlan> m_plan;

  public:
    ///\brief Static method to get instance of this singleton object.
    static DicomSvc* GetInstance();

    ///
    void Initialize(const std::string& planFileType);

    ///
    void SetPlanFile(const std::string& plan_file);

     ///
    bool Initialized() const { return m_plan.get() ? true : false; }

    IPlan* GetPlan() const {
      return m_plan.get();
    }

    ///\brief  Describe me.
    double GetRTPlanAngle(int current_beam, int current_controlpoint) const;

    ///\brief  Describe me.
    double GetRTPlanDose(int current_beam, int current_controlpoint) const;

    ///\brief  Describe me.
    unsigned GetRTPlanNumberOfBeams(const std::string& planFile) const;

    ///\brief  Sum of the control points for a given beam number
    unsigned GetRTPlanNumberOfControlPoints(const std::string& planFile,unsigned beamNumber) const;

    ///
    unsigned GetTotalNumberOfControlPoints() const;

    ///
    void ExportPatientToCT(const std::string& series_csv_path, const std::string& output_path) const;

    ///
    static ControlPointConfig GetControlPointConfig(int id, const std::string& planFile);
};

#endif  // Dose3D_DicomSvcSVC_H
