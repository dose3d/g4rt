#include "DicomSvc.hh"
#include "Services.hh"

#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include "LogSvc.hh"

namespace py = pybind11;
using namespace py::literals;

////////////////////////////////////////////////////////////////////////////////
///
DicomSvc::DicomSvc() {
  // Get current RT-Plan file
  m_rtplan_file = Service<ConfigSvc>()->GetValue<std::string>("RunSvc", "RTPlanInputFile");
  Initialize();
}

////////////////////////////////////////////////////////////////////////////////
///
void DicomSvc::Initialize(){
  LOGSVC_INFO("DicomSvc initalization...");
  auto rtplanMlcReader = py::module::import("dicom_rtplan_mlc");
  auto beams_counter = rtplanMlcReader.attr("return_number_of_beams")(m_rtplan_file).cast<int>();
  LOGSVC_INFO("Found #{} beams",beams_counter);
  for(int i=0; i<beams_counter;++i){
    unsigned nCP = rtplanMlcReader.attr("return_number_of_controlpoints")(m_rtplan_file,i).cast<unsigned>();
    m_rtplan_beam_n_control_points.emplace_back(nCP);
    LOGSVC_INFO("Found #{} control poinst for {} beam",nCP,i);
  }
}

////////////////////////////////////////////////////////////////////////////////
///
unsigned DicomSvc::GetTotalNumberOfControlPoints() const {
  return std::accumulate(m_rtplan_beam_n_control_points.begin(), m_rtplan_beam_n_control_points.end(), 0);
}

////////////////////////////////////////////////////////////////////////////////
///
DicomSvc *DicomSvc::GetInstance() {
  static DicomSvc instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
///
G4double DicomSvc::GetRTPlanJawPossition(const std::string& jawName, int current_beam, int current_controlpoint) const {
  G4cout << "[INFO]:: Reading the Jaws configuration for "<< jawName<<" jaw; "
                                                          << current_beam <<" beam; "
                                                          << current_controlpoint<<" ctrl point;" << G4endl;
  if(jawName!="X1" && jawName!="X2" && jawName!="Y1" && jawName!="Y2")
    G4Exception("DicomSvc", "GetRTPlanJawPossition", FatalErrorInArgument, "Wrong jaw name input given!");

  auto rtplanJawsReader = py::module::import("dicom_rtplan_jaws");
  auto beams_counter = rtplanJawsReader.attr("return_number_of_beams")(m_rtplan_file);
  const int number_of_beams = beams_counter.cast<int>();
  auto controlpoints_counter = rtplanJawsReader.attr("return_number_of_controlpoints")(m_rtplan_file, number_of_beams);
  const int number_of_controlpoints = controlpoints_counter.cast<int>();
  auto jaws_counter = rtplanJawsReader.attr("return_number_of_jaws")(m_rtplan_file);
  const int number_of_jaws = jaws_counter.cast<int>();
  std::cout << "We have " << number_of_beams << " beams, " << number_of_controlpoints << " checkpoints and "
            << number_of_jaws << " jaws." << std::endl;
  int jawIdx = 0; // X1
  if (jawName == "X2") jawIdx = 1;
  else if (jawName == "Y1") jawIdx = 2;
  else if (jawName == "Y2") jawIdx = 3;
  return rtplanJawsReader.attr("return_position")(m_rtplan_file, current_beam, current_controlpoint, jawIdx).cast<double>();
}

////////////////////////////////////////////////////////////////////////////////
///
std::vector<G4double> DicomSvc::GetRTPlanMlcPossitioning(const std::string& side, int current_beam, int current_controlpoint) const {
  G4cout << "[INFO]:: Reading the MLC configuration for "<< side<<" side; "
                                                         << current_beam <<" beam; "
                                                         << current_controlpoint<<" ctrl point;" << G4endl;
  if(side!="Y1" && side!="Y2")
    G4Exception("DicomSvc", "GetRTPlanMlcPossitioning", FatalErrorInArgument, "Wrong input side given!");

  std::vector<G4double> mlcPositioning;
  auto rtplanMlcReader = py::module::import("dicom_rtplan_mlc");
  auto beams_counter = rtplanMlcReader.attr("return_number_of_beams")(m_rtplan_file);
  const int number_of_beams = beams_counter.cast<int>();
  auto controlpoints_counter = rtplanMlcReader.attr("return_number_of_controlpoints")(m_rtplan_file, number_of_beams);
  const int number_of_controlpoints = controlpoints_counter.cast<int>();
  auto leaves_counter = rtplanMlcReader.attr("return_number_of_leaves")(m_rtplan_file);
  const int number_of_leaves = leaves_counter.cast<int>();
  std::cout << "We have " << number_of_beams << " beams, " << number_of_controlpoints << " checkpoints and "
            << number_of_leaves << " leaves." << std::endl;
  auto leavesPositions = rtplanMlcReader.attr("return_possition")(m_rtplan_file, side, current_beam, current_controlpoint, number_of_leaves).cast<py::array_t<double>>();
  py::buffer_info acceser = leavesPositions.request();
  double *accesableLeavesPositions = (double *) acceser.ptr;
  for (int i = 0; i < acceser.size; i++) {
    mlcPositioning.emplace_back(accesableLeavesPositions[i]);
  }
  return mlcPositioning;
}

////////////////////////////////////////////////////////////////////////////////
///
double DicomSvc::GetRTPlanAngle(int current_beam, int current_controlpoint) const {
  auto rtplanAngleReader = py::module::import("dicom_rtplan_angle");
  // Zakomentowane części kodu - na przyszłość przy większej ilości runów
  // py::function beams_counter = rtplanAngleReader.attr("return_number_of_beams")(m_rtplan_file);
  // int number_of_beams = beams_counter.cast<int>();
  // py::function controlpoints_counter = rtplanAngleReader.attr("return_number_of_controlpoints")(m_rtplan_file, number_of_beams);
  // int number_of_controlpoints = controlpoints_counter.cast<int>();
  auto angleInControlpoint = rtplanAngleReader.attr("return_angle_in_controlpoint")(m_rtplan_file, current_beam,
                                                                              current_controlpoint).cast<double>();
  std::cout <<angleInControlpoint<< '\n';
  return angleInControlpoint;
}
////////////////////////////////////////////////////////////////////////////////
///
double DicomSvc::GetRTPlanDose(int current_beam, int current_controlpoint) const {

  auto rtplanDoseReader = py::module::import("dicom_rtplan_dose");
  // Zakomentowane części kodu - na przyszłość przy większej ilości runów
  // py::function beams_counter = rtplanDoseReader.attr("return_number_of_beams")(m_rtplan_file);
  // int number_of_beams = beams_counter.cast<int>();
  // py::function controlpoints_counter = rtplanDoseReader.attr("return_number_of_controlpoints")(m_rtplan_file, number_of_beams);
  // int number_of_controlpoints = controlpoints_counter.cast<int>();
  auto doseInControlpoint = rtplanDoseReader.attr("return_dose_during_specified_controlpoint")(m_rtplan_file, current_beam,
                                                                                  current_controlpoint).cast<double>();
  std::cout <<doseInControlpoint<< '\n';
  return doseInControlpoint;
}

////////////////////////////////////////////////////////////////////////////////
///
unsigned DicomSvc::GetRTPlanNumberOfBeams() const {
  // Implement me.
  // To be done when you will not use the 0 beam and control point 0.
  return 2; // dummy number
}

////////////////////////////////////////////////////////////////////////////////
///
unsigned DicomSvc::GetRTPlanNumberOfControlPoints(unsigned beamNumber) const{
  // Implement me. Note: verify if given beamNumber exists!
  // To be done when you will not use the 0 beam and control point 0.
  return 100; // dummy number
}

////////////////////////////////////////////////////////////////////////////////
///
void DicomSvc::ExportPatientToCT(const std::string& series_csv_path, const std::string& output_path) const {

    // PyGILState_Release(gstate);
    m_ct_svc.set_paths(output_path);
    m_ct_svc.create_ct_series(series_csv_path);
    m_ct_svc.~ICtSvc();
  }

////////////////////////////////////////////////////////////////////////////////
///
ControlPointConfig DicomSvc::GetControlPointConfig(int id, const std::string& planFile){
  auto file = planFile;
  if(file.front() != '/'){ // the path is not absolute
    std::string data_path = PROJECT_DATA_PATH;
    file = data_path + "/" + file;
  }
  auto fileExt = svc::getFileExtenstion(file);
  if(fileExt == "dat"){
    return ICustomPlan::GetControlPointConfig(id, planFile);
  }
  // else if(fileExt == "dcm"){
  //   return IDicomPlan::GetControlPointConfig(id, planFile);
  // }
  G4String msg = "Unknown file extension: " + fileExt;
  LOGSVC_CRITICAL(msg.data());
  G4Exception("DicomSvc", "GetControlPointConfig", FatalErrorInArgument, msg);
  return ControlPointConfig();
}

