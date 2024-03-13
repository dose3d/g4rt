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


namespace py = pybind11;
using namespace py::literals;

////////////////////////////////////////////////////////////////////////////////
/// 
class CtSvc {
  private:
    py::object m_py_dicom_ct;
  public:
    ///
    CtSvc():m_py_dicom_ct(py::module::import("dicom_ct").attr("PyCtSvc")()) {}
    ///
    void set_output_path(const std::string& path) const{
      m_py_dicom_ct.attr("set_output_path")(path);
    }
    ///
    void create_ct_series(const std::string& series_csv_path) const{
      m_py_dicom_ct.attr("create_ct_series")(series_csv_path);
    }
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
    DicomSvc();

    ///
    ~DicomSvc() = default;

    /// Delete the copy and move constructors
    DicomSvc(const DicomSvc &) = delete;
    DicomSvc &operator=(const DicomSvc &) = delete;
    DicomSvc(DicomSvc &&) = delete;
    DicomSvc &operator=(DicomSvc &&) = delete;

    ///
    void Initialize();

    ///
    CtSvc m_ct_svc;

  public:
    ///\brief Static method to get instance of this singleton object.
    static DicomSvc* GetInstance();

    ///\brief  Mądry opis
    G4double GetRTPlanJawPossition(const std::string& jawName, int current_beam, int current_controlpoint) const;

    ///
    std::vector<G4double> GetRTPlanMlcPossitioning(const std::string& side, int current_beam, int current_controlpoint) const;

    ///\brief  Describe me.
    double GetRTPlanAngle(int current_beam, int current_controlpoint) const;

    ///\brief  Describe me.
    double GetRTPlanDose(int current_beam, int current_controlpoint) const;

    ///\brief  Describe me.
    unsigned GetRTPlanNumberOfBeams() const;

    ///\brief  Sum of the control points for a given beam number
    unsigned GetRTPlanNumberOfControlPoints(unsigned beamNumber) const;

    ///
    unsigned GetTotalNumberOfControlPoints() const;

    void ExportPatientToCT(const std::string& series_csv_path, const std::string& output_path) const;
};

#endif  // Dose3D_DicomSvcSVC_H
