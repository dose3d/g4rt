#include "LogSvcConfig.hh"
#include "LogSvc.hh"
#include "Services.hh"

LogSvcConfig::LogSvcConfig(Callback func):TomlConfigModule("LogSvc"),callbackUpdateConfig(func) {};

void LogSvcConfig::Configure() {

  SPDLOG_DEBUG("Log configuration");
  
  ParseTomlConfig();
  PrintLoggerConfigs();
}


void LogSvcConfig::ParseTomlConfig(){

  SPDLOG_TRACE("LogSvcConf Parse 1");
  auto configSvc = Service<ConfigSvc>();
  SPDLOG_TRACE("LogSvcConf Parse 2 ");
  auto configFileName = configSvc->GetValue<std::string>("RunSvc","LogConfigFile");

  std::string projectPath = PROJECT_LOCATION_PATH;
  std::string configFilePath = projectPath+configFileName;
  SPDLOG_INFO("Log config file: {}.", configFilePath);

  toml::v3::ex::parse_result config;

  SPDLOG_TRACE("LogSvcConf Parse 3");

  if (svc::checkIfFileExist(configFilePath)) {
    SPDLOG_TRACE("LogSvcConf Parse 4");
    SetTomlConfigFile(configFilePath);
    SPDLOG_TRACE("LogSvcConf Parse 5");
    config = toml::parse_file(configFilePath);
  }
  else {
    SPDLOG_ERROR("Log config file {} not found!", configFilePath);
  };
  auto configPrefix = GetTomlConfigPrefix();

  for(auto& [key, value] : config) {
    std::string keyStr = std::string(key);
    auto loggerName = keyStr.substr(keyStr.find("_") + 1);
    for(auto& [paramName, value2] : *(config[key]).as_table()) {
       SetLoggerConfig(loggerName, std::string(paramName), config[key][paramName].value_or(""));
    }
  }
  callbackUpdateConfig();
}

LoggerConfig LogSvcConfig::GetLoggerConfig(std::string paramLoggerName)
{
  LoggerConfig resultConfig;
  std::string loggerName = paramLoggerName;

  // Initiaite with defoult config
  auto it = m_loggerConfigs.find("Global");
  if (it != m_loggerConfigs.end()) {
     resultConfig = it->second;
  }

  SPDLOG_DEBUG("Logger global config");

  // Update result with existing logger parameters
  it = m_loggerConfigs.find(loggerName);
  if (it != m_loggerConfigs.end()) {
    auto storedConfig = it->second;
    SPDLOG_DEBUG("Logger config");
    PrintLoggerConfig(it->second);
    if (storedConfig.logLevel != "" ) {
      resultConfig.logLevel = storedConfig.logLevel;
    }
    if (storedConfig.consolePattern != "" ) {
      resultConfig.consolePattern = storedConfig.consolePattern;
    }
    if (storedConfig.filePattern != "" ) {
      resultConfig.filePattern = storedConfig.filePattern;
    }
    if (storedConfig.logFileName != "" ) {
      resultConfig.logFileName = storedConfig.logFileName;
    }
    if (storedConfig.logDir != "" ) {
      resultConfig.logDir = storedConfig.logDir;
    }
    if (storedConfig.consoleLog != "" ) {
      resultConfig.consoleLog = storedConfig.consoleLog;
    }

  }
  
  if (resultConfig.logFileName != "") {
    resultConfig.logFilePath = svc::getOutputDir() + std::string("/") + resultConfig.logDir + std::string("/") + resultConfig.logFileName;
    SPDLOG_TRACE("logFilePath: {}", resultConfig.logFilePath);
  }

  return resultConfig;
}

void LogSvcConfig::SetLoggerConfig(std::string loggerName, std::string param, std::string value)
{
    if (m_loggerConfigs.count(loggerName) == 0) {
        // Create a new LoggerConfig object if loggerName is not found
        LoggerConfig newLoggerConfig;
        newLoggerConfig.name = loggerName;
        m_loggerConfigs[loggerName] = newLoggerConfig;
    }

    std::string paramLowercase;
    std::transform(param.begin(), param.end(), std::back_inserter(paramLowercase),
                   [](char c) { return std::tolower(c); });
    
    SPDLOG_DEBUG("For logger: {}, set param: {} ({}) value: {}", loggerName, param, paramLowercase, value );
    // Set the field value for the specified loggerName and param
    if (paramLowercase == "loglevel") {
        m_loggerConfigs[loggerName].logLevel = value;
     } else if (paramLowercase == "pattern") {
         m_loggerConfigs[loggerName].pattern = value;
    } else if (paramLowercase == "consolepattern") {
        m_loggerConfigs[loggerName].consolePattern = value;
    } else if (paramLowercase == "filepattern") {
        m_loggerConfigs[loggerName].filePattern = value;
    } else if (paramLowercase == "logfilename") {
        m_loggerConfigs[loggerName].logFileName = value;
    } else if (paramLowercase == "logdir") {
        m_loggerConfigs[loggerName].logDir = value;
    } else if (paramLowercase == "console") {
        m_loggerConfigs[loggerName].consoleLog = value;
    } else {
        SPDLOG_ERROR("Error: invalid param name {} in log config file.",param);
        return;
    }
    
}

void LogSvcConfig::PrintLoggerConfigs() {
  SPDLOG_DEBUG("Logger configs:");
  for (auto& logger : m_loggerConfigs) {
    SPDLOG_DEBUG("Logger: {}", logger.first);
    PrintLoggerConfig(logger.second);
  }
}

void LogSvcConfig::PrintLoggerConfig(const LoggerConfig config) {
  SPDLOG_DEBUG("Name: {}", config.name);
  if (config.logLevel != "") 
    SPDLOG_DEBUG("LogLevel: {}", config.logLevel);
  if (config.consolePattern != "") 
    SPDLOG_DEBUG("ConsolePattern: {}", config.consolePattern);
  if (config.filePattern != "") 
    SPDLOG_DEBUG("FilePattern: {}", config.filePattern);
  if (config.logDir != "") 
    SPDLOG_DEBUG("logDir: {}", config.logDir);
  if (config.logFileName != "") 
    SPDLOG_DEBUG("LogFileName: {}", config.logFileName);
  if (config.consoleLog != "") 
    SPDLOG_DEBUG("Console: {}", config.consoleLog);

}
