/**
* Simple application for .dcm to .dat files conversion
*/

#include "Services.hh"
#include "cxxopts.h"
#include <pybind11/embed.h>
#include "LogSvc.hh"
#include <locale.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
// Helper functions to parse a pair from a string
std::pair<float, float> convertPair(const std::pair<std::string, std::string>& input) {
    try {
        float first = std::stof(input.first);
        float second = std::stof(input.second);
        return {first, second};
    } catch (const std::invalid_argument& e) {
        throw std::invalid_argument("Invalid argument: One or both strings cannot be converted to float.");
    } catch (const std::out_of_range& e) {
        throw std::out_of_range("Out of range: One or both strings represent a value out of float range.");
    }
}
std::pair<float, float> parsePair(const std::string& input) {
    std::istringstream stream(input);
    std::string first, second;
    if (std::getline(stream, first, ',') && std::getline(stream, second)) {
        return convertPair({first, second});
    }
    throw std::invalid_argument("Invalid pair format. Expected 'value1,value2'.");
}

int main(int argc, const char *argv[]) {
  // Force POSIX "C" locale to ensure consistent scientific notation (e.g., 1.23e-12).
  // In some locales (e.g., pl_PL.UTF-8), numerical formatting functions may emit
  // invalid or locale-specific formats that ROOT or GDML parsers can't read.
  // This fixes cases where exponent notation is lost or misformatted.
  setenv("LC_ALL", "C", 1);


  pybind11::scoped_interpreter guard{};
  pybind11::module sys = pybind11::module::import("sys");
  sys.attr("path").attr("append")(std::string(PROJECT_PY_PATH));

  // In order to capture G4cout and G4err before the kernel UI manager launches -> I initialize loggSession at the beginning of Main.`  

  LogSvc::Init(argc, argv, "build/tmp_logs/app_main.log", loguru::Verbosity_MAX, 100);

  auto dicomSvc = Service<DicomSvc>();    //

  if (argc > 1) {
  cxxopts::Options options(argv[0], "Text UI mode - command line options");
  try {
    options.positional_help("[optional args]");
    options.show_positional_help();

    options.add_options()("help", "Print help")("version", "Application version");

    options.add_options("Application run mode")
        ("b,nBeams", "Number of beams (default value ALL)", cxxopts::value<int>(), "N")
        ("c,nCtrlPts", "Number of control points (default value ALL)", cxxopts::value<int>(), "N")
        ("nParticles", "Number of particles to be set in the plan (default value 1e3)", cxxopts::value<int>()->default_value("1000"), "N")
        ("f,File", "Specify RT-Plan file", cxxopts::value<std::string>(), "FILE")
        ("fieldCentre", "Perform Field centralization in AB sides", cxxopts::value<bool>()->default_value("false"))
        ("o,OutputDir", "Specify output directory", cxxopts::value<std::string>(), "PATH")
        ("fieldConstrain", "Input pair ([mm] in format: value1,value2)", cxxopts::value<std::string>());
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

      std::exit(EXIT_SUCCESS);
      }


      // USER OBLIGATORY PARAMETERS
      // --------------------------------------------------------------------
      auto rtplan_file = std::string();
      if (cmdopts.count("f")) 
        rtplan_file = cmdopts["f"].as<std::string>();
      if(rtplan_file.empty())
        svc::invalidArgumentError("main","Please specify RT-Plan file!");
      if(!svc::checkIfFileExist(rtplan_file))
        svc::invalidArgumentError("main","RT-Plan file not found!");
      auto output_dir = std::string();
      if (cmdopts.count("o")) 
        output_dir = cmdopts["o"].as<std::string>();
      if (output_dir.empty())
        svc::invalidArgumentError("main","Please specify output directory!");

      // USER OPTIONAL PARAMETERS
      // --------------------------------------------------------------------
      int usr_nBeams = -1000;
      if (cmdopts.count("b")) 
        usr_nBeams = cmdopts["b"].as<int>();
      int usr_nCtrlPts = -1000;
      if (cmdopts.count("c")) 
        usr_nCtrlPts = cmdopts["c"].as<int>();
      
      bool fieldConstrain = false;
      std::pair<float, float> field{-1,-1};
      if (cmdopts.count("fieldConstrain")){ 
        auto input = cmdopts["fieldConstrain"].as<std::string>();
        try {
            field = parsePair(input);
            std::cout << "Parsed pair: (" << field.first << ", " << field.second << ")\n";
            fieldConstrain = true;
        } catch (const std::invalid_argument& e) {
            std::cerr << e.what() << '\n';
            return 1;
        }
      }

      // OPERATION
      // --------------------------------------------------------------------
      auto fieldCentre = cmdopts["fieldCentre"].as<bool>();
      int nParticles = cmdopts["nParticles"].as<int>();
      constexpr G4double max_centre_offset = 30.0; // mm
      std::mt19937 centre_rng(std::random_device{}());
      std::uniform_real_distribution<G4double> centre_distribution(
          -max_centre_offset, max_centre_offset);

      auto leaf_boundaries = [](size_t leaf_count) {
        std::vector<G4double> boundaries(leaf_count + 1, 0.0);

        // Geometry used by MlcSimplified: 14 x 5 mm, 32 x 2.5 mm,
        // 14 x 5 mm. For other MLC sizes use the central 2.5 mm pitch.
        G4double total_width = 2.5 * leaf_count;
        if (leaf_count == 60)
          total_width = 220.0;

        boundaries.front() = -total_width / 2.0;
        for (size_t leaf = 0; leaf < leaf_count; ++leaf) {
          const G4double width =
              leaf_count == 60 && (leaf < 14 || leaf >= 46) ? 5.0 : 2.5;
          boundaries.at(leaf + 1) = boundaries.at(leaf) + width;
        }
        return boundaries;
      };

      auto centralize_field = [&](std::vector<G4double>& mlc_a,
                                  std::vector<G4double>& mlc_b) {
        if (mlc_a.size() != mlc_b.size() || mlc_a.empty()) {
          LOG_ERROR("Cannot centralize field: inconsistent or empty MLC data");
          return;
        }

        auto is_open = [&](size_t leaf) {
          return std::abs(mlc_a.at(leaf) - mlc_b.at(leaf)) > 1e-6;
        };

        const auto boundaries = leaf_boundaries(mlc_a.size());
        size_t first_open = mlc_a.size();
        size_t last_open = 0;
        G4double total_area = 0.0;
        G4double weighted_ab = 0.0;
        for (size_t leaf = 0; leaf < mlc_a.size(); ++leaf) {
          if (!is_open(leaf))
            continue;
          first_open = std::min(first_open, leaf);
          last_open = leaf;
          const G4double aperture =
              std::abs(mlc_a.at(leaf) - mlc_b.at(leaf));
          const G4double leaf_width =
              boundaries.at(leaf + 1) - boundaries.at(leaf);
          const G4double area = aperture * leaf_width;
          total_area += area;
          weighted_ab +=
              area * (mlc_a.at(leaf) + mlc_b.at(leaf)) / 2.0;
        }

        if (first_open == mlc_a.size() || total_area <= 0.0) {
          LOG_WARN("Cannot centralize a closed MLC field");
          return;
        }

        const G4double target_ab = centre_distribution(centre_rng);
        const G4double centre_ab = weighted_ab / total_area;
        const G4double shift_ab = target_ab - centre_ab;
        for (size_t leaf = 0; leaf < mlc_a.size(); ++leaf) {
          mlc_a.at(leaf) += shift_ab;
          mlc_b.at(leaf) += shift_ab;
        }

        // In the direction perpendicular to leaf travel the aperture can only
        // be translated by moving complete leaf pairs.
        const G4double target_rows = centre_distribution(centre_rng);
        int best_shift = 0;
        G4double best_error = std::numeric_limits<G4double>::max();
        const int min_shift = -static_cast<int>(first_open);
        const int max_shift =
            static_cast<int>(mlc_a.size() - 1 - last_open);
        for (int shift = min_shift; shift <= max_shift; ++shift) {
          G4double shifted_area = 0.0;
          G4double shifted_weighted_rows = 0.0;
          for (size_t leaf = first_open; leaf <= last_open; ++leaf) {
            if (!is_open(leaf))
              continue;
            const auto destination =
                static_cast<size_t>(static_cast<int>(leaf) + shift);
            const G4double aperture =
                std::abs(mlc_a.at(leaf) - mlc_b.at(leaf));
            const G4double destination_width =
                boundaries.at(destination + 1) - boundaries.at(destination);
            const G4double area = aperture * destination_width;
            const G4double destination_centre =
                (boundaries.at(destination) +
                 boundaries.at(destination + 1)) /
                2.0;
            shifted_area += area;
            shifted_weighted_rows += area * destination_centre;
          }
          const G4double shifted_centre =
              shifted_weighted_rows / shifted_area;
          if (std::abs(shifted_centre) > max_centre_offset)
            continue;
          const G4double error = std::abs(shifted_centre - target_rows);
          if (error < best_error) {
            best_error = error;
            best_shift = shift;
          }
        }

        if (best_error == std::numeric_limits<G4double>::max()) {
          LOG_WARN("Field cannot be moved within +/- {} mm in leaf-row axis",
                   max_centre_offset);
        } else if (best_shift != 0) {
          auto shifted_a = std::vector<G4double>(mlc_a.size(), 0.0);
          auto shifted_b = std::vector<G4double>(mlc_b.size(), 0.0);
          for (size_t leaf = 0; leaf < mlc_a.size(); ++leaf) {
            const int destination = static_cast<int>(leaf) + best_shift;
            if (destination >= 0 &&
                destination < static_cast<int>(mlc_a.size())) {
              shifted_a.at(destination) = mlc_a.at(leaf);
              shifted_b.at(destination) = mlc_b.at(leaf);
            }
          }
          mlc_a = std::move(shifted_a);
          mlc_b = std::move(shifted_b);
        }

        G4double centered_area = 0.0;
        G4double centered_weighted_rows = 0.0;
        for (size_t leaf = 0; leaf < mlc_a.size(); ++leaf) {
          if (!is_open(leaf))
            continue;
          const G4double aperture =
              std::abs(mlc_a.at(leaf) - mlc_b.at(leaf));
          const G4double width =
              boundaries.at(leaf + 1) - boundaries.at(leaf);
          const G4double area = aperture * width;
          centered_area += area;
          centered_weighted_rows +=
              area * (boundaries.at(leaf) + boundaries.at(leaf + 1)) / 2.0;
        }
        const G4double centre_rows =
            centered_weighted_rows / centered_area;
        std::cout << "Field geometric centre [mm]: " << centre_rows << ","
                  << target_ab << std::endl;
      };
      
      auto isPassingFieldConstrain = [&](std::vector<G4double>& mlc_a,
                                        std::vector<G4double>& mlc_b) -> bool {
        if(field.first<0){
          std::cout << "[WARN]:: No field size specified!" << std::endl;
          return true;
        }
        G4double min_a = 10000, min_b = 0;
        G4double max_a = 0, max_b = -10000;
        double min_leaf_x = -1, max_leaf_x = -1;
        double min_leaf_y = -1, max_leaf_y = -1;
        double x_width = 3; // assume 3 mm width
        double current_x = 0;
        for(size_t i_leaf=0; i_leaf < mlc_a.size(); i_leaf++){
          current_x = i_leaf * x_width;
          if(mlc_a.at(i_leaf) - mlc_b.at(i_leaf) != 0){ // check if mlc is not closed
            if(min_leaf_x<0) min_leaf_x = current_x;
            if(max_leaf_x < current_x) max_leaf_x = current_x;
            if(mlc_a.at(i_leaf) < min_a) min_a = mlc_a.at(i_leaf);
            if(mlc_b.at(i_leaf) < min_b) min_b = mlc_b.at(i_leaf);
            if(mlc_a.at(i_leaf) > max_a) max_a = mlc_a.at(i_leaf);
            if(mlc_b.at(i_leaf) > max_b) max_b = mlc_b.at(i_leaf);
          }
        }
        min_leaf_y = min_a < min_b ? min_a : min_b;
        max_leaf_y = max_a > max_b ? max_a : max_b;
        std::cout << "X range: " << min_leaf_x << " : " << max_leaf_x << std::endl;
        std::cout << "Y range: " << min_leaf_y << " : " << max_leaf_y << std::endl;
        if (field.first > max_leaf_x - min_leaf_x &&
            field.second > max_leaf_y - min_leaf_y){
              std::cout << "Field Constrain PASSED" << std::endl;
            return true;
            }
            std::cout << "Field Constrain NOT PASSED: X:" << max_leaf_x - min_leaf_x << "  Y: " << max_leaf_y - min_leaf_y  << std::endl;
        return false;
      };

      auto write_dat_plan_file = [&]( std::string dat_plan_file,
                                      std::pair<double,double>& jaw_x,
                                      std::pair<double,double>& jaw_y, 
                                      std::vector<G4double>& mlc_a, 
                                      std::vector<G4double>& mlc_b) {
        if(mlc_a.empty() || mlc_b.empty()){
        LOG_ERROR("MLC data is empty!");
        }
        std::ofstream outFile(dat_plan_file);
        if (outFile.is_open()) {
          // Write data to file
          // NOTE: There are FIXED values!!!!
          outFile << "# Rotation: 0.0\n";
          outFile << "# Particles: "<< nParticles << "\n";
          outFile << "# Jaws: X1[mm],X2[mm],Y1[mm],Y2[mm]\n";
          outFile << jaw_x.first << "," << jaw_x.second << "," << jaw_y.first << "," << jaw_y.second << "\n";
          outFile << "# MLC: Y1[mm],Y2[mm]\n";
          for(size_t i_leaf=0; i_leaf < mlc_a.size(); i_leaf++)
            outFile << mlc_a.at(i_leaf) << "," << mlc_b.at(i_leaf) << "\n";

          // Close file
          outFile.close();
          LOG_INFO("Plan data written into file: {}", dat_plan_file);

      } else LOG_ERROR("Unable to open file: {}", dat_plan_file);
      };


      dicomSvc->SetPlanFile(rtplan_file);
      auto nBeams = dicomSvc->GetRTPlanNumberOfBeams(rtplan_file);
      nBeams = (usr_nBeams > 0 && usr_nBeams < nBeams) ? usr_nBeams : nBeams;
      std::cout << "Number of beams: " << nBeams << std::endl;
      int passing_rate_counter{0};
      int cp_counter{0};
      for(int i_beam=0; i_beam<nBeams; i_beam++){
        auto nCtrlPts = dicomSvc->GetRTPlanNumberOfControlPoints(rtplan_file,i_beam);
        nCtrlPts = (usr_nCtrlPts > 0 && usr_nCtrlPts < nCtrlPts) ? usr_nCtrlPts : nCtrlPts;
        std::cout << "Beam " << i_beam << "  => Number of control points: " << nCtrlPts << std::endl;
        // NOTE: For all control points in the beam the jaws aperture
        // is defined in the first control point:
        auto jaw_x = dicomSvc->GetPlan()->ReadJawsAperture(rtplan_file,"X",i_beam,0);
        auto jaw_y = dicomSvc->GetPlan()->ReadJawsAperture(rtplan_file,"Y",i_beam,0);
        std::cout << "Jaws: " << jaw_x.first << "," << jaw_x.second << "," << jaw_y.first << "," << jaw_y.second << std::endl;
        for(int i_cp=0; i_cp < nCtrlPts; i_cp++){
          ++cp_counter;
          auto mlc_a = dicomSvc->GetPlan()->ReadMlcPositioning(rtplan_file,"Y1",i_beam,i_cp);
          std::cout << "MLC A: " << mlc_a.size() << std::endl;

          auto mlc_b = dicomSvc->GetPlan()->ReadMlcPositioning(rtplan_file,"Y2",i_beam,i_cp);
          std::cout << "MLC B: " << mlc_b.size() << std::endl;

          std::string dat_plan_file = svc::getFileName(rtplan_file);
          dat_plan_file = output_dir + "/"+dat_plan_file+"_beam"+std::to_string(i_beam)+"_cp"+std::to_string(i_cp)+".dat";
          if (fieldCentre){
            centralize_field(mlc_a, mlc_b);
          }
          auto passed = true;
          if (fieldConstrain)
            passed = isPassingFieldConstrain(mlc_a, mlc_b);
          if (passed){
            write_dat_plan_file(dat_plan_file,jaw_x,jaw_y,mlc_a,mlc_b);
            ++passing_rate_counter;
          }
          mlc_a.clear();
          mlc_b.clear();
          
        }
      }
      std::cout << "#Processed CP: " << cp_counter << " filtered and saved: " << passing_rate_counter << "("<< double(passing_rate_counter)*100/cp_counter <<"%)" << std::endl;
    } catch (const cxxopts::OptionException &e) {
      std::cout << "Error parsing options: " << e.what() << std::endl;
      std::exit(EXIT_FAILURE);
    } 
  } else {
    G4cout << "[ERROR]:: Command line options missing (use '" << argv[0] << " --help' if needed)" << G4endl;
  }
  
  return EXIT_SUCCESS;
}
