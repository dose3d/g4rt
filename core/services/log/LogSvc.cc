#include "LogSvc.hh"
#include <iostream>
#include <filesystem>
#include "Services.hh"

std::shared_ptr<spdlog::logger> m_logger;

std::map<std::string,std::shared_ptr<spdlog::logger>> LogSvc::m_loggersMap;
std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> LogSvc::m_consoleSink;
std::shared_ptr<spdlog::logger> LogSvc::m_defaultLogger;
std::shared_ptr<LogSvcConfig> LogSvc::m_config;
std::map<std::string,std::shared_ptr<spdlog::sinks::basic_file_sink_mt>> LogSvc::m_fileSinks;
std::map<std::string,std::shared_ptr<spdlog::sinks::basic_file_sink_mt>> LogSvc::m_loggerFileSinks;


////////////////////////////////////////////////////////////////////////////////
///
LogSvc::LogSvc(){
  if(!LogSvc::Initialized())
    LogSvc::Initialize();
}

////////////////////////////////////////////////////////////////////////////////
///
LogSvc *LogSvc::GetInstance() {
  static LogSvc instance;
  return &instance;
}

std::shared_ptr<spdlog::logger> LogSvc::RecreateLogger(std::string loggerName)
{
  auto it = m_loggersMap.find(loggerName);

  if (it != m_loggersMap.end()){
     spdlog::drop(loggerName);
     m_loggersMap.erase(it);
  } 
  return GetLogger(loggerName);
}

////////////////////////////////////////////////////////////////////////////////
///
void LogSvc::UpdateLoggers() {
  SPDLOG_INFO("Update loggers from config.");
  auto globalConfig = m_config->GetLoggerConfig("Global");
  if (globalConfig.consolePattern != "") {
    m_consoleSink->set_pattern(globalConfig.consolePattern);
    SPDLOG_TRACE("Set console sink pattern (config): {}",globalConfig.consolePattern);
  }

  // m_config->PrintLoggerConfigs();
  for(auto& [loggerName, loggerPtr] : m_loggersMap) {
    ConfigureLogger(loggerPtr);
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void LogSvc::ShutDown(){
  spdlog::shutdown();
}

////////////////////////////////////////////////////////////////////////////////
///
void LogSvc::Initialize() {

  SPDLOG_INFO("LogSvc Initialize.");
  m_loggersMap = std::map<std::string,std::shared_ptr<spdlog::logger>>();
  m_consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

  //Create own default logger
  m_defaultLogger = std::make_shared<spdlog::logger>("Global", m_consoleSink);
  spdlog::register_logger(m_defaultLogger);
  spdlog::set_default_logger(m_defaultLogger);
  m_loggersMap["Global"] = m_defaultLogger;

  //Default pattern
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] %n [%^%l%$] %v");
  SPDLOG_DEBUG("LogSvc Initialize default pattern.");

  m_config = std::make_shared<LogSvcConfig>(&LogSvc::UpdateLoggers);

}

////////////////////////////////////////////////////////////////////////////////
///
bool LogSvc::Initialized() {
  return m_config ? true : false;
}

////////////////////////////////////////////////////////////////////////////////
///
spdlog::level::level_enum LogSvc::Str2LogLevel(std::string logLevelStr) {
  spdlog::level::level_enum logLevel = spdlog::level::info;
  if (logLevelStr == "trace") {
    logLevel = spdlog::level::trace;
  } else if (logLevelStr == "debug") {
    logLevel = spdlog::level::debug;
  } else if (logLevelStr == "info") {
    logLevel = spdlog::level::info;
  } else if (logLevelStr == "warning") {
    logLevel = spdlog::level::warn;
  } else if (logLevelStr == "error") {
    logLevel = spdlog::level::err;
  } else if (logLevelStr == "critical") {
    logLevel = spdlog::level::critical;
  }
  return logLevel;
}

////////////////////////////////////////////////////////////////////////////////
///
void LogSvc::DefaulLogLevel(std::string logLevelStr){
  auto logLevel = LogSvc::Str2LogLevel(logLevelStr);
  spdlog::set_level(logLevel);
  // SPDLOG_CRITICAL("Test critical");
  // SPDLOG_ERROR("Test error");
  // SPDLOG_WARN("Test warn");
  // SPDLOG_INFO("Test info");
  // SPDLOG_DEBUG("Test debug");
  // SPDLOG_TRACE("Test trace");
}  

////////////////////////////////////////////////////////////////////////////////
///
void LogSvc::Configure() {
  SPDLOG_DEBUG("LogSvc Configure.");
  m_config->Configure();
}

////////////////////////////////////////////////////////////////////////////////
///
void LogSvc::ConfigureLogger(std::shared_ptr<spdlog::logger> logger) {
  SPDLOG_DEBUG("Configure logger: {}", logger->name());
  auto loggerConfig = m_config->GetLoggerConfig(logger->name());
  SPDLOG_TRACE("logger file name: {}", loggerConfig.logFileName);
  SPDLOG_TRACE("logger level: {}", loggerConfig.logLevel);
//  SPDLOG_TRACE("logger pattern: {}", loggerConfig.pattern);
  SPDLOG_TRACE("logger console pattern: {}", loggerConfig.consolePattern);
  SPDLOG_TRACE("logger file pattern: {}", loggerConfig.filePattern);

  logger->set_pattern(loggerConfig.consolePattern);

  if (loggerConfig.filePattern != "") {
    auto it = m_loggerFileSinks.find(logger->name());
    if (it != m_loggerFileSinks.end()){
      auto fileSink = it->second;
      fileSink->set_pattern(loggerConfig.filePattern);
    }
  }

  SetLoggerLogLevel(logger, loggerConfig.logLevel);
  
}

////////////////////////////////////////////////////////////////////////////////
///
std::shared_ptr<spdlog::logger> LogSvc::GetLogger(std::string loggerName){

  auto it = m_loggersMap.find(loggerName);

  if (it != m_loggersMap.end()){
    return it->second;
  } else {

  SPDLOG_DEBUG("GetLogger {}", loggerName);
    auto loggerConfig = m_config->GetLoggerConfig(loggerName);
    
    std::vector<spdlog::sink_ptr> sinks;
    if(loggerConfig.consoleLog == "true") {
      SPDLOG_DEBUG("GetLogger: set console");
      sinks.push_back(m_consoleSink);
    }
    if(loggerConfig.logFilePath != "") {
      SPDLOG_DEBUG("GetLogger: set file");
      auto fileSink = GetFileSink(loggerConfig.logFilePath);
      sinks.push_back(fileSink);
      m_loggerFileSinks[loggerName] = fileSink;
    }
    
    auto logger = std::make_shared<spdlog::logger>(loggerName, begin(sinks),end(sinks));
    spdlog::register_logger(logger);
    SPDLOG_LOGGER_DEBUG(logger, "Logger {} created.", loggerName);
    LogSvc::ConfigureLogger(logger);
    m_loggersMap[loggerName] = logger;
    SPDLOG_LOGGER_DEBUG(logger, "Logger {} configured.", loggerName);
    SPDLOG_DEBUG("GetLogger {} end.", loggerName);
    return logger;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
std::shared_ptr<spdlog::sinks::basic_file_sink_mt> LogSvc::GetFileSink(std::string fileName){
  auto it = m_fileSinks.find(fileName);
  if (it != m_fileSinks.end()){
    return it->second;
  } else {
    auto config = m_config->GetLoggerConfig("Global");
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(fileName);
    if (config.filePattern != "") {
      sink->set_pattern(config.filePattern);
    }
    m_fileSinks[fileName] = sink;
    return sink;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void LogSvc::SetLoggerLogLevel(std::shared_ptr<spdlog::logger> loggerPtr, std::string newLogLevel){
  loggerPtr->set_level(Str2LogLevel(newLogLevel));
}

////////////////////////////////////////////////////////////////////////////////
///
void LogSvc::SetConsolePattern(std::string newPattern){
  m_consoleSink->set_pattern(newPattern);
  SPDLOG_TRACE("Set console sink pattern: {}",newPattern);
}

////////////////////////////////////////////////////////////////////////////////
///
void LogSvc::SetFilePattern(std::string newPattern){
  for(auto& [fileName, sinkPtr] : m_fileSinks) {
    sinkPtr->set_pattern(newPattern);
    SPDLOG_TRACE("Set console sink pattern: {}",newPattern);
  }
}
