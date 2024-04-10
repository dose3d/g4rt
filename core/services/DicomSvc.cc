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
void DicomSvc::Initialize(const std::string& planFileType){
  LOGSVC_INFO("DicomSvc initalization...");
  if(planFileType == "dat")
    m_plan = std::make_unique<ICustomPlan>();
  else if(planFileType == "dcm")
    m_plan = std::make_unique<IDicomPlan>();
  else {
    G4String msg = "Unknown plan file extension: " + planFileType + "Supported: .dat, .dcm";
    LOGSVC_CRITICAL(msg.data());
    G4Exception("DicomSvc", "Initialize", FatalErrorInArgument, msg);
  }

  // TODO refacto the following as the context specific code...
  //
  // auto rtplanMlcReader = py::module::import("dicom_rtplan_mlc");
  // auto beams_counter = rtplanMlcReader.attr("return_number_of_beams")(m_rtplan_file).cast<int>();
  // LOGSVC_INFO("Found #{} beams",beams_counter);
  // for(int i=0; i<beams_counter;++i){
  //   unsigned nCP = rtplanMlcReader.attr("return_number_of_controlpoints")(m_rtplan_file,i).cast<unsigned>();
  //   m_rtplan_beam_n_control_points.emplace_back(nCP);
  //   LOGSVC_INFO("Found #{} control poinst for {} beam",nCP,i);
  // }
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
G4double IDicomPlan::ReadJawPossition(const std::string& planFile, const std::string& jawName, int beamIdx, int controlpointIdx) const{
  LOGSVC_INFO("Reading the Jaws configuration from {}",planFile);
  LOGSVC_INFO("JawName: {}, beamIdx: {}, controlpointIdx: {}",jawName,beamIdx,controlpointIdx);
  if(jawName!="X1" && jawName!="X2" && jawName!="Y1" && jawName!="Y2")
    G4Exception("IDicomPlan", "GetJawPossition", FatalErrorInArgument, "Wrong jaw name input given!");

  auto rtplanJawsReader = py::module::import("dicom_rtplan_jaws");
  auto beams_counter = rtplanJawsReader.attr("return_number_of_beams")(planFile);
  const int number_of_beams = beams_counter.cast<int>();
  auto controlpoints_counter = rtplanJawsReader.attr("return_number_of_controlpoints")(planFile, number_of_beams);
  const int number_of_controlpoints = controlpoints_counter.cast<int>();
  auto jaws_counter = rtplanJawsReader.attr("return_number_of_jaws")(planFile);
  const int number_of_jaws = jaws_counter.cast<int>();
  std::cout << "We have " << number_of_beams << " beams, " << number_of_controlpoints << " checkpoints and "
            << number_of_jaws << " jaws." << std::endl;
  int jawIdx = 0; // X1
  if (jawName == "X2") jawIdx = 1;
  else if (jawName == "Y1") jawIdx = 2;
  else if (jawName == "Y2") jawIdx = 3;
  return rtplanJawsReader.attr("return_position")(planFile, beamIdx, controlpointIdx, jawIdx).cast<double>();
}

////////////////////////////////////////////////////////////////////////////////
///
std::vector<G4double> IDicomPlan::ReadMlcPositioning(const std::string& planFile, const std::string& side, int beamIdx, int controlpointIdx){
  LOGSVC_INFO("Reading the MLC configuration from {}",planFile);
  LOGSVC_INFO("Side: {}, beamIdx: {}, controlpointIdx: {}",side,beamIdx,controlpointIdx);
  if(side!="Y1" && side!="Y2")
    G4Exception("IDicomPlan", "GetMlcPositioning", FatalErrorInArgument, "Wrong input side given!");
  std::vector<G4double> mlcPositioning;
  auto rtplanMlcReader = py::module::import("dicom_rtplan_mlc");
  auto beams_counter = rtplanMlcReader.attr("return_number_of_beams")(planFile);
  const int number_of_beams = beams_counter.cast<int>();
  auto controlpoints_counter = rtplanMlcReader.attr("return_number_of_controlpoints")(planFile, number_of_beams);
  const int number_of_controlpoints = controlpoints_counter.cast<int>();
  auto leaves_counter = rtplanMlcReader.attr("return_number_of_leaves")(planFile);
  const int number_of_leaves = leaves_counter.cast<int>();
  // std::cout << "We have " << number_of_beams << " beams, " << number_of_controlpoints << " checkpoints and "
  //           << number_of_leaves << " leaves." << std::endl;
  auto leavesPositions = rtplanMlcReader.attr("return_possition")(planFile, side, beamIdx, controlpointIdx, number_of_leaves).cast<py::array_t<double>>();
  py::buffer_info acceser = leavesPositions.request();
  double *accesableLeavesPositions = (double *) acceser.ptr;
  for (int i = 0; i < acceser.size; i++) {
    mlcPositioning.emplace_back(accesableLeavesPositions[i]);
  }
  return std::move(mlcPositioning);
}

////////////////////////////////////////////////////////////////////////////////
///
std::vector<G4double> ICustomPlan::ReadMlcPositioning(const std::string& planFile, const std::string& side, int beamIdx, int controlpointIdx){
  LOGSVC_INFO("Reading the MLC {} side configuration from {}",side,planFile);
  if(side!="Y1" && side!="Y2"){
    G4String msg = "Wrong input side given (" + side + ")! Allowed values are 'Y1' and 'Y2'";
    LOGSVC_CRITICAL(msg.data());
    G4Exception("ICustomPlan", "GetMlcPositioning", FatalErrorInArgument, msg);
  }
  std::ifstream file(planFile);
  if (!file.is_open()) {
    G4String msg = "Could not open file: " + planFile;
    LOGSVC_CRITICAL(msg.data());
    G4Exception("ICustomPlan", "GetMlcPositioning", FatalErrorInArgument, msg);
  }
  std::string line;
  std::vector<double> mlc_y1, mlc_y2;

  while (std::getline(file, line)) {
    // Skip header lines
    if (line.empty() || line[0] == '#')
        continue;
    std::istringstream iss(line);
    std::string value_y1, value_y2;
    // Get the values as strings separated by a comma
    if (std::getline(iss, value_y1, ',') && std::getline(iss, value_y2)) {
      // Convert string to double and add to the respective vectors
      mlc_y1.push_back(std::stod(value_y1));
      mlc_y2.push_back(std::stod(value_y2));
    }
  }
  file.close();
  return side=="Y1" ? std::move(mlc_y1) : std::move(mlc_y2);
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
  auto dicomSvc = DicomSvc::GetInstance();
  if(!dicomSvc->Initialized()){
    dicomSvc->Initialize(svc::getFileExtenstion(planFile));
  }
  auto file = planFile;
  if(file.front() != '/'){ // the path is not absolute
    std::string data_path = PROJECT_DATA_PATH;
    file = data_path + "/" + file;
  }
  return dicomSvc->m_plan->GetControlPointConfig(id, file);
}

////////////////////////////////////////////////////////////////////////////////
///
ControlPointConfig IDicomPlan::GetControlPointConfig(int id, const std::string& planFile){
  // TODO
  // extract from planFile, below is dummy mockup
  auto config = ControlPointConfig(id, 1000, 0.);
  config.MlcInputFile = planFile;
  config.FieldShape = "RTPlan"; // TODO FieldShape::RTPlan;
  config.FieldSizeA = 23.0; // Temp
  config.FieldSizeB = 35.0; // Temp 
  return std::move(config);
}

////////////////////////////////////////////////////////////////////////////////
///
ControlPointConfig ICustomPlan::GetControlPointConfig(int id, const std::string& planFile){
  auto nEvents = GetNEvents(planFile);
  auto rotation = GetRotation(planFile);
  auto config = ControlPointConfig(id, nEvents, rotation);
  config.MlcInputFile = planFile;
  config.FieldShape = "RTPlan"; // TODO FieldShape::CustomPlan;
  config.FieldSizeA = 23.0; // Temp
  config.FieldSizeB = 35.0; // Temp 
  return std::move(config);
}
////////////////////////////////////////////////////////////////////////////////
///
int ICustomPlan::GetNEvents(const std::string& planFile) {
  std::string svalue;
  std::string line;
  std::ifstream file(planFile.c_str());
  if (file.is_open()) {
    while (getline(file, line)){
      if (line.length() > 0 && (line.rfind("# Particles:",0) == 0)) {
        std::istringstream ss(line);
        while (getline(ss, svalue,':')){
          if(svalue.rfind("#",0)!=0){
            return static_cast<int>(std::stod(svalue.c_str())); 
            }
          }
        } 
      }
    }
  return 0; 
}

////////////////////////////////////////////////////////////////////////////////
///
double ICustomPlan::GetRotation(const std::string& planFile) {
  std::string svalue;
  std::string line;
  std::ifstream file(planFile.c_str());
  if (file.is_open()) {
    while (getline(file, line)){
      if (line.length() > 0 && (line.rfind("# Rotation:",0) == 0)) {
        std::istringstream ss(line);
        while (getline(ss, svalue,':')){
          if(svalue.rfind("#",0)!=0){
            return std::stod(svalue.c_str()); 
            }
          }
        } 
      }
    }
  return 0.; 
}

