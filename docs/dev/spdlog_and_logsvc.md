# LogSvc - Logging service
# Initialization
## Initialization

For use default logger, with compile time log level and pattern.

```
LogSvc::Initialize();
```

## Configure

For configure from config file (TODO - now some sample code)

```
LogSvc::Configure();
```

Seting global log level for use with cmd option

```
LogSvc::DefaulLogLevel(logLevelStr);
```

## Use to write log message

For default logger use macro:
```
SPDLOG_INFO("Message");
SPDLOG_INFO("Message {} {}",var1,var2);
```
Available macros for each level:
```
SPDLOG_TRACE();
SPDLOG_DEBUG();
SPDLOG_INFO();
SPDLOG_WARN();
SPDLOG_ERROR();
SPDLOG_CRITICAL()
```

# Use as class local logger
## Initialize class local  logger
Add class member m_logger:

```
  ///\brief local logger
  std::shared_ptr<spdlog::logger> m_logger;
```

In class constructor use macro to initialize logger with class name as logger name (typeid(*this).name())

```
LOGSVC_LOGGER_INITIALIZE;
```

Or with custom logger name

```
m_logger = LogSvc::GetLogger("RunSvc");
```
## Use to write log message to class local logger

For local class logger use macro: 
```
LOGSVC_INFO("Message");
LOGSVC_INFO("Message {} {}",var1,var2);
```
Available macros for each level:

```
LOGSVC_TRACE();
LOGSVC_DEBUG();
LOGSVC_INFO();
LOGSVC_WARN();
LOGSVC_ERROR();
LOGSVC_CRITICAL()
```

