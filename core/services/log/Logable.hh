/**
  *
  * \author P.Matejski
  * \date 2023-04-12
  *   
**/
#ifndef Dose3D_LOGGABLE_H
#define Dose3D_LOGGABLE_H

#include "LogSvc.hh"
class Logable {
    public:
    ///\brief local logger
    std::shared_ptr<spdlog::logger> m_logger;

    /// @brief Constructor, initialize local class logger
    /// @param loggerName logger name, use class name
    Logable(std::string loggerName) {
        // Before any logable object is created, we have to care
        // for the service initialization first!
        if(!LogSvc::Initialized())
          LogSvc::Initialize();
        m_logger = LogSvc::GetLogger(loggerName);
    };

};

#endif //Dose3D_LOGGABLE_H