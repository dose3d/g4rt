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
        ("nb,nBeams", "Number of beams (default value ALL)", cxxopts::value<int>(), "N")
        ("ncp,nCtrlPts", "Number of control points (default value ALL)", cxxopts::value<int>(), "N")
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

      // OPERATION
      // --------------------------------------------------------------------
      auto rtplan_file = std::string();
      if (cmdopts.count("f")) 
        rtplan_file = cmdopts["f"].as<std::string>();

      if(rtplan_file.empty())
        svc::invalidArgumentError("main","Please specify RT-Plan file!");
      
      dicomSvc->Initialize(svc::getFileExtenstion(rtplan_file));
      auto output_dir = std::string();
      auto nBeams = dicomSvc->GetRTPlanNumberOfBeams();
      //auto nCtrlPts = dicomSvc->GetNumberOfControlPoints(rtplan_file);

      if (cmdopts.count("o")) 
        output_dir = cmdopts["o"].as<std::string>();
      if (output_dir.empty())
        svc::invalidArgumentError("main","Please specify output directory!");

    } catch (const cxxopts::OptionException &e) {
      std::cout << "Error parsing options: " << e.what() << std::endl;
      std::exit(EXIT_FAILURE);
    } 
  } else {
    G4cout << "[ERROR]:: Command line options missing (use '" << argv[0] << " --help' if needed)" << G4endl;
  }
  
  return EXIT_SUCCESS;
}
