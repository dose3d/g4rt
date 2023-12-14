//
// Created by g4dev on 20.11.19.
//

#include "IO.hh"
#include <fstream>
#include <iostream>
#include <utility>
#include <filesystem>

namespace fs = std::filesystem;

////////////////////////////////////////////////////////////////////////////////
///
IO::IO() {
  m_path_to_binary_dir = PROJECT_BINARY_PATH;
  m_path_to_project_data_dir = PROJECT_DATA_PATH;
}

////////////////////////////////////////////////////////////////////////////////
///
IO::IO(const std::string &file) {
  m_current_file = file;
}

////////////////////////////////////////////////////////////////////////////////
///
std::string IO::GetPathToTempRunDirectory() {
  auto temp_dir = m_path_to_binary_dir + "/temporary_run_dir";
  CreateDirIfNotExits(temp_dir);
  return temp_dir;
}

////////////////////////////////////////////////////////////////////////////////
///
std::string IO::GetPathProjectDataDirectory() {
  return m_path_to_project_data_dir;
}

////////////////////////////////////////////////////////////////////////////////
///
void IO::CleanUP() {
  // todo in future...
}

////////////////////////////////////////////////////////////////////////////////
///
std::vector<std::string> IO::ReadAllLinesFromFile() {
  std::vector<std::string> lines;
  if (!m_current_file.empty()) {
    std::ifstream infile(m_current_file);
    std::string line;
    while (std::getline(infile, line)) lines.push_back(line);
  }
  return lines;
}

////////////////////////////////////////////////////////////////////////////////
///
void IO::WriteToFile(const std::string &fileWithFullPath, const std::vector<std::string> &lines) {
  DeleteFile(fileWithFullPath);

  // open a file in write mode.
  std::ofstream outfile;
  outfile.open(fileWithFullPath);

  // again write inputted data into the file.
  for (const auto &line : lines)
    outfile << line << std::endl;

  // close the opened file.
  outfile.close();
}


/////////////////////////////////////////////////////////////////////////////////////////
void IO::CreateDirIfNotExits(const std::string &path) {
  fs::path dp(path);
  if (!fs::exists(dp)) {
    std::cout << "[INFO]:: Created directory: " << path << std::endl;
    fs::create_directories(dp);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
void IO::DeleteFile(const std::string &file_path) {
  fs::path fp = file_path;
  if (fs::exists(fp)) {
    std::cout << "[INFO]:: Remove existing file:\n"
                 "[INFO]:: " << file_path << std::endl;
    fs::remove(fp);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
std::unique_ptr<TFile> IO::CreateOutputTFile(const std::string& name, const std::string& dir){
  auto output_tfile = std::make_unique<TFile>(name.c_str(),"RECREATE");
  if(!dir.empty()){
    std::string tmp; 
    std::stringstream ss(dir);
    std::vector<std::string> dirs;
    while(getline(ss, tmp, '/')){
      dirs.push_back(tmp);
    }
    auto current_dir = output_tfile->mkdir(dirs.at(0).c_str());
    std::cout << "Created dir: " << dirs.at(0) << std::endl;
    for(const auto& d : dirs){
      if(d!=dirs.at(0)){
        current_dir = current_dir->mkdir(d.c_str());
        std::cout << "Created dir: " << d << std::endl;
      }
    }
  }
  return std::move(output_tfile);
}

