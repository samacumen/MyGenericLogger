#ifndef _LOGGER_H_
#define _LOGGER_H_

// C++ Header File(s)
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <typeinfo>

// POSIX Socket Header File(s)
#include <errno.h>
#include <pthread.h>
#include "Utils.h"

using namespace utils;

namespace CPlusPlusLogging
{
    ///
    /// Enum LOG_LEVEL defines the various log levels. The standard used for log levels are same as
    /// the standard used for apache log4j, in addition to the introduction of raw byte BUFFER logging.
    ///
    typedef enum LOG_LEVEL
    {
      DISABLE_LOG       = 0,        // The highest possible rank and is intended to turn off logging.
      LOG_LEVEL_FATAL   = 1,        // Designates very severe error events that will presumably lead the application to abort.
      LOG_LEVEL_ERROR   = 2,        // Designates error events that might still allow the application to continue running.
      LOG_LEVEL_WARNING = 3,        // Designates potentially harmful situations.
      LOG_LEVEL_INFO    = 4,        // Designates informational messages that highlight the progress of the application at coarse-grained level.
      LOG_LEVEL_DEBUG   = 5,        // Designates fine-grained informational events that are most useful to debug an application.
      LOG_LEVEL_TRACE   = 6,        // Designates finer-grained informational events than the DEBUG.
      LOG_LEVEL_BUFFER  = 7,        // Buffer is the special case for logging raw bytes.
      LOG_LEVEL_ALL     = 8         // All levels including custom levels.
    } LogLevel;

    #define eprintf(...) fprintf (stderr, __VA_ARGS__)
    // eprintf ("%s:%d: ", input_file, lineno)

    /// Direct Interface for logging into log file or console using variadic MACRO(s)
    ///
    #define LOG_ALWAYS(x)    Logger::getInstance()->persistent_logging("[ALWAYS_TAG]: ", x)

    #define LOG_FATAL(...)     Logger::getInstance()->user_log(LOG_LEVEL_FATAL,   __PRETTY_FUNCTION__, __FUNCTION__, __VA_ARGS__)
    #define LOG_ERROR(...)     Logger::getInstance()->user_log(LOG_LEVEL_ERROR,   __PRETTY_FUNCTION__, __FUNCTION__, __VA_ARGS__)
    #define LOG_WARNING(...)   Logger::getInstance()->user_log(LOG_LEVEL_WARNING, __PRETTY_FUNCTION__, __FUNCTION__ , __VA_ARGS__)
    #define LOG_INFO(...)      Logger::getInstance()->user_log(LOG_LEVEL_INFO,    __PRETTY_FUNCTION__, __FUNCTION__, __VA_ARGS__)
    #define LOG_DEBUG(...)     Logger::getInstance()->user_log(LOG_LEVEL_DEBUG,   __PRETTY_FUNCTION__, __FUNCTION__, __VA_ARGS__)
    #define LOG_TRACE(...)     Logger::getInstance()->user_log(LOG_LEVEL_TRACE,   __PRETTY_FUNCTION__, __FUNCTION__, __VA_ARGS__)

    #define LOG_BUFFER(x)    Logger::getInstance()->buffer(LOG_LEVEL_BUFFER, x)

    #define UPDATE_LOG_LEVEL(y) Logger::getInstance()->updateLogLevel(y);
    #define UPDATE_LOG_TYPE(y)  Logger::getInstance()->updateLogType(y);

    // enum for LOG_TYPE
    typedef enum LOG_TYPE
    {
      NO_LOG            = 1,
      CONSOLE           = 2,
      FILE_LOG          = 3,
    } LogType;

    ///
    /// A generic printf type formatting to enable logging of multiple parameters
    ///
    struct format {
        std::ostringstream oss_;
        format (std::string tag) { oss_ << tag; }
        format () { }
        template <typename T>
        format & operator % (T &&a) { oss_ << " " << a << ","; return *this; }
    };

    class Logger
    {
      public:
         static Logger* getInstance() throw ();

         ///
         static const LOG_LEVEL getLogLevel() throw();

         /// Interface for Persistent logging
         /// Error and Alarm and warning log must be always be enabled
         /// Hence, there is no interfce to control warning, error and alarm logs
         ///
         void persistent_logging(const std::string& tag, const char* text) throw();
         void persistent_logging(const std::string& tag, std::string& text) throw();

         ///
         /// A generic interface for logging for user defined levels. This method provides overload
         /// for simple string to multi-parameter logging with user defined level as parameters.
         /// This must be used with the MACRO for invoking info, debug, trace, etc. type of logging.
         ///
         void user_log(LOG_LEVEL level, std::string pretty_func, string func_name, const char* text) throw();

         template <typename T, typename... Params>
         void user_log (LOG_LEVEL level,  std::string pretty_func, string func_name, T arg, Params... parameters)
         {
             if (m_LogLevel < level)
                 return;

             fmt_logging(level, pretty_func, func_name, format() % arg, parameters...);
         }

         template <typename T, typename... Params>
         void fmt_logging (LOG_LEVEL level,  std::string pretty_func, string func_name, format &fmt, T arg, Params... parameters) {
             fmt_logging(level, pretty_func, func_name, fmt % arg, parameters...);
         }

         void fmt_logging (LOG_LEVEL level, std::string pretty_func, string func_name, format &fmt)
         {
             if (level == LOG_LEVEL_BUFFER)
             {
                buffer_log(level, fmt.oss_.str().data());
             }
             else
             {
                user_log(level, pretty_func, func_name, fmt.oss_.str().data());
             }
         }

         /// Interface for Buffer Log (special case)
         ///
         void buffer_log(LOG_LEVEL level, const char* text) throw();
         void buffer_log(LOG_LEVEL level, std::string& text) throw();
         void buffer_log(LOG_LEVEL level, std::ostringstream& stream) throw();

         /// Interface to control log levels
         ///
         void setLogLevel(LogLevel logLevel);
         void setLogType(LogType logType);

         /// Enable all log levels (for most detailed logging)
         ///
         void enableAllLog();

         /// Disable all log levels, except warning, error and alarm
         ///
         void disableLog();

         void mylog (int level, std::string s) {
             std::cout << "msg: " << s << std::endl;
         }

         void mylog_r (int level, format &fmt) {
             std::cout << "fmt: " << fmt.oss_.str() << std::endl;
         }

         template <typename T, typename... Params>
         void mylog_r (int level, format &fmt, T arg, Params... parameters) {
             mylog_r(level, fmt % arg, parameters...);
         }

         template <typename T, typename... Params>
         void mylog (int level, T arg, Params... parameters) {
             mylog_r(level, format() % arg, parameters...);
         }

      protected:
         Logger();
         ~Logger();

         /// Wrapper function for lock/unlock
         /// For Extensible feature, lock and unlock should be in protected mode
         ///
         void lock();
         void unlock();

      private:
         void logInto(std::string& data, LOG_TYPE type);
         void logIntoFile(std::string& data);
         void logOnConsole(std::string& data);
         std::string getLogTypeTag(LOG_LEVEL level);

         Logger(const Logger& obj) {}
         void operator=(const Logger& obj) {}

      private:
         static Logger*          m_Instance;
         std::ofstream           m_File;

         pthread_mutexattr_t     m_Attr;
         pthread_mutex_t         m_Mutex;

         LogLevel                m_LogLevel;
         LogType                 m_LogType;
    };

} // End of namespace

#endif // End of _LOGGER_H_
