/**
* Simple application for .dcm to .dat files conversion
*/

#include "Services.hh"
#include "cxxopts.h"
#include <pybind11/embed.h>
#include "LogSvc.hh"

int main(int argc, const char *argv[]) {

  pybind11::scoped_interpreter guard{};
  pybind11::module sys = pybind11::module::import("sys");
  sys.attr("path").attr("append")(std::string(PROJECT_PY_PATH));

  SPDLOG_DEBUG("Initialize services");
  auto dicomSvc = Service<DicomSvc>();    //
  SPDLOG_DEBUG("End of initialize services");

  SPDLOG_INFO("Wellcome G4RT!");

  if (argc > 1) {
  cxxopts::Options options(argv[0], "Text UI mode - command line options");
  try {
    options.positional_help("[optional args]");
    options.show_positional_help();

    options.add_options()("help", "Print help")("version", "Application version");

    options.add_options("Application run mode")
        ("b,nBeams", "Number of beams (default value ALL)", cxxopts::value<int>(), "N")
        ("c,nCtrlPts", "Number of control points (default value ALL)", cxxopts::value<int>(), "N")
        ("f,File", "Specify RT-Plan file", cxxopts::value<std::string>(), "FILE")
        ("o,OutputDir", "Specify output directory", cxxopts::value<std::string>(), "PATH")
        ;

    auto results = options.parse(argc, argv);
    if (results.count("help")) {
      std::cout << options.help({"", "Application run mode"}) << std::endl;
      std::exit(EXIT_SUCCESS);
    }

    auto cmdopts = std::move(results);

    // GENERAL
    // --------------------------------------------------------------------
    if (cmdopts.count("version")) {
      SPDLOG_INFO("Geant-RT verson v 1.0.0");
      std::exit(EXIT_SUCCESS);
      }

      // USER OBLIGATORY PARAMETERS
      // --------------------------------------------------------------------
      auto rtplan_file = std::string();
      if (cmdopts.count("f")) 
        rtplan_file = cmdopts["f"].as<std::string>();
      if(rtplan_file.empty())
        svc::invalidArgumentError("main","Please specify RT-Plan file!");
      
      auto output_dir = std::string();
      if (cmdopts.count("o")) 
        output_dir = cmdopts["o"].as<std::string>();
      if (output_dir.empty())
        svc::invalidArgumentError("main","Please specify output directory!");
      
      // OPERATION
      // --------------------------------------------------------------------
      dicomSvc->SetPlanFile(rtplan_file);
      auto nBeams = 2; //dicomSvc->GetRTPlanNumberOfBeams();
      for(int i_beam=0; i_beam<nBeams; i_beam++){
        auto nCtrlPts = 10; //dicomSvc->GetRTPlanNumberOfControlPoints(i_beam);
        // NOTE: For all control points in the beam the jaws aperture
        // is defined in the first control point:
        auto jaw_x = dicomSvc->GetPlan()->ReadJawsAperture(rtplan_file,"X",i_beam,0);
        auto jaw_y = dicomSvc->GetPlan()->ReadJawsAperture(rtplan_file,"Y",i_beam,0);
        for(int i_cp=0; i_cp < nCtrlPts; i_cp++){
          auto mlc_a = dicomSvc->GetPlan()->ReadMlcPositioning(rtplan_file,"Y1",i_beam,i_cp);
          auto mlc_b = dicomSvc->GetPlan()->ReadMlcPositioning(rtplan_file,"Y2",i_beam,i_cp);
          
          SPDLOG_INFO("Beam {} | CP {} | Jaws X: {}, {}, Y: {}, {}", i_beam, i_cp,jaw_x.first,jaw_x.second,jaw_y.first,jaw_y.second);
          for(const auto& mlc_position : mlc_a)
            std::cout << mlc_position << ",";
          std::cout << std::endl;
          for(const auto& mlc_position : mlc_b)
            std::cout << mlc_position << ",";
          std::cout << std::endl;
          
        }
      }
    } catch (const cxxopts::OptionException &e) {
      std::cout << "Error parsing options: " << e.what() << std::endl;
      std::exit(EXIT_FAILURE);
    } 
  } else {
    G4cout << "[ERROR]:: Command line options missing (use '" << argv[0] << " --help' if needed)" << G4endl;
  }
  
  return EXIT_SUCCESS;
}
