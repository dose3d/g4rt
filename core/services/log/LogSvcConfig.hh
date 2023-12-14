/**
  *
  * \author P.Matejski
  * \date 2023-04-15
  *   
**/
#ifndef Dose3D_LOGSVCCONFIG_H
#define Dose3D_LOGSVCCONFIG_H

#include <functional>
#include "TomlConfigurable.hh"

class LoggerConfig {
  public:
  std::string name;
  std::string logLevel;
  std::string pattern;
  std::string consolePattern;
  std::string filePattern;
  std::string logFileName;
  std::string logDir;
  std::string consoleLog;
  std::string logFilePath;

};

class LogSvcConfig : public TomlConfigModule {
  public:
  typedef std::function<void()> Callback;
  Callback callbackUpdateConfig;

  std::map<std::string,LoggerConfig> m_loggerConfigs;
  std::string logPath;
//  LoggerConfig m_defaulLoggerConfig;
  
  LogSvcConfig(Callback func);

//  static std::shared_ptr<LogSvcConfig> GetInstance();

   void Configure();
  // void DefaultConfig(const std::string &unit) override;  
  void ParseTomlConfig() override;

  LoggerConfig GetLoggerConfig(std::string loggerName);

  void SetLoggerConfig(std::string loggerName, std::string param, std::string vale);

  void PrintLoggerConfigs();
  void PrintLoggerConfig(const LoggerConfig config);
  
  private:
  

};

#endif //Dose3D_LOGSVCCONFIG_H