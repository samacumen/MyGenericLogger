// C++ Header File(s)
#include <iostream>
#include <cstdlib>
#include <ctime>

// Code Specific Header Files(s)
#include "Logger.h"
#include "ConfigFile.h"

#include <fcntl.h>
#include <errno.h>

using namespace std;
using namespace CPlusPlusLogging;

Logger* Logger::m_Instance = 0;

// Log file name. File name should be change from here only
const string logFileName = LOG_FILE_NAME;

// Tags that are persistently logged
#define ALWAYS_TAG "[ALWAYS]: "

// Tags that are logged as per user's will
#define FATAL_TAG "[FATAL]: "
#define ERROR_TAG "[ERROR]: "
#define WARNING_TAG "[WARNING]: "
#define INFO_TAG "[INFO]: "
#define DEBUG_TAG "[DEBUG]: "
#define TRACE_TAG "[TRACE]: "

///
/// Creates the instance + initializes default log type/level +mutex variables
///
Logger::Logger()
{
   m_File.open(logFileName.c_str(), ios::out|ios::app);
   m_LogLevel	= LOG_LEVEL_TRACE;
   m_LogType	= FILE_LOG;
//   mylog(1,"sdfasdf");
//   mylog(1,"sdfasdf","3","4",5);
   //multiparam_logging(LOG_LEVEL_INFO,"sdfasdf","3",this, 66, "4",5);

   // Initialize mutex
   int ret=0;
   ret = pthread_mutexattr_settype(&m_Attr, PTHREAD_MUTEX_ERRORCHECK_NP);
   if(ret != 0)
   {
      printf("Logger::Logger() -- Mutex attribute not initialized!!\n");
      exit(0);
   }
   ret = pthread_mutex_init(&m_Mutex, &m_Attr);
   if(ret != 0)
   {
      printf("Logger::Logger() -- Mutex not initialized!!\n");
      exit(0);
   }
}

Logger::~Logger()
{
   m_File.close();

   pthread_mutexattr_destroy(&m_Attr);
   pthread_mutex_destroy(&m_Mutex);
}

const LOG_LEVEL Logger::getLogLevel() throw()
{
    Utils::ConfigFile _conf;

    try {
        // load main configuration
        const string settings_path = utils::Utils::getSettingsFilePath();
        _conf = Utils::ConfigFile(settings_path);

        LOG_LEVEL log_level = (LOG_LEVEL)_conf.read<int>("logging_level");
        return log_level;

        // int f = open(logfile_path.c_str(), O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    }
    catch (const Utils::ConfigFile::file_not_found&)
    {
        cout <<"\nUnable to read..";
        // Log to error log
        // Raise an error exception with message
    }
    catch (...)
    {
        cout << "External exception..";
    }

    return LOG_LEVEL_INFO;
}

Logger* Logger::getInstance() throw ()
{
   if (m_Instance == 0)
   {
      m_Instance = new Logger();
   }
   return m_Instance;
}

void Logger::persistent_logging(const std::string& tag, const char* text) throw()
{
    string data;
    data.append(tag);
    data.append(__PRETTY_FUNCTION__);
    data.append(text);

    logInto(data, m_LogType);
}

void Logger::persistent_logging(const std::string& tag, std::string& text) throw()
{
    persistent_logging(tag, text.data());
}

///
/// Interface for Buffer Log. Buffer is the special case. So don't add log level
/// and timestamp in the buffer message. Just log the raw bytes.
///
void Logger::buffer_log(LOG_LEVEL level, const char* text) throw()
{
    if (m_LogLevel < level) //LOG_LEVEL_BUFFER
        return;

   if(m_LogType == FILE_LOG)
   {
      lock();
      m_File << text << endl;
      unlock();
   }
   else if(m_LogType == CONSOLE)
   {
      cout << text << endl;
   }
}

///
/// Interface for buffer log. Overload for std::string.
///
void Logger::buffer_log(LOG_LEVEL level, std::string& text) throw()
{
   buffer_log(level, text.data());
}

///
/// Interface for buffer log. Overload for stream.
///
void Logger::buffer_log(LOG_LEVEL level, std::ostringstream& stream) throw()
{
   string text = stream.str();
   buffer_log(level, text.data());
}

///
/// User based logging for plain text logging.
///
void Logger::user_log(LOG_LEVEL level, std::string pretty_func, std::string func_name, const char* text) throw()
{
   if (m_LogLevel < level)
       return;

   string data;
   data.append(getLogTypeTag(level));
   data.append(utils::Utils::className(pretty_func));
   data.append("::");
   data.append(func_name);
   data.append("() - ");
   data.append(text);

   logInto(data, m_LogType);
}

///
/// Returns the log type tag for logging purpose
///
std::string Logger::getLogTypeTag(LOG_LEVEL level)
{
    switch (level)
    {
        case LOG_LEVEL_FATAL:   return FATAL_TAG;
        case LOG_LEVEL_ERROR:   return ERROR_TAG;
        case LOG_LEVEL_WARNING: return WARNING_TAG;
        case LOG_LEVEL_INFO:    return INFO_TAG;
        case LOG_LEVEL_DEBUG:   return DEBUG_TAG;
        case LOG_LEVEL_TRACE:   return TRACE_TAG;

        case LOG_LEVEL_BUFFER:
        case LOG_LEVEL_ALL:
        default: return EMPTY_STR;
    }
}

///
/// Locks the critical section
///
void Logger::lock()
{
   pthread_mutex_lock(&m_Mutex);
}

///
/// Unlocks the critical section
///
void Logger::unlock()
{
   pthread_mutex_unlock(&m_Mutex);
}

///
/// A generic function where string data to be logged along with the desired log type
/// can be provided. This logs into a text file or console.
///
void Logger::logInto(string &data, LOG_TYPE type)
{
    if(m_LogType == FILE_LOG)
    {
       logIntoFile(data);
    }
    else if(m_LogType == CONSOLE)
    {
       logOnConsole(data);
    }
}

///
/// This logs into a text file.
///
void Logger::logIntoFile(std::string& data)
{
   lock();
   m_File << utils::Utils::getCurrentTime() << "  " << data << endl;
   unlock();
}

///
/// This logs into the console/ terminal where the application executes.
///
void Logger::logOnConsole(std::string& data)
{
   cout << utils::Utils::getCurrentTime() << "  " << data << endl;
}

///
/// Interface to set/update the log level. Thus the log level can be updated
/// at runtime.
///
void Logger::setLogLevel(LogLevel logLevel)
{
   m_LogLevel = logLevel;
}

///
/// Interface to set/update the log type. Namely file or console type.
///
void Logger::setLogType(LogType logType)
{
   m_LogType = logType;
}

///
/// Enable all log levels
///
void Logger::enableAllLog()
{
   m_LogLevel = LOG_LEVEL_ALL;
}

///
/// Disable all log levels, except always, error, alarm, warning
///
void Logger:: disableLog()
{
   m_LogLevel = DISABLE_LOG;
}

