#ifndef BASE_LOGGER_H
#define BASE_LOGGER_H

#include <string>
#include <memory>
#include <atomic>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include "util/lock.h"

namespace base {
namespace log {

enum class Level {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4
};

struct LoggerConfig {
    std::string logDir = "logs";
    std::string logName = "base";
    Level level = Level::INFO;
    size_t maxFileSize = 200 * 1024 * 1024;
    std::string format = "[%Y-%m-%d %H:%M:%S] [%LEVEL%] [%MODULE%] [%FILE%:%LINE%] [%FUNCTION%] %MSG%";
    bool enableConsole = true;
    bool enableFile = true;
    bool enableInternalLog = true;
};

class Logger {
public:
    static void init(const LoggerConfig& config);
    static void shutdown();

    static void setLevel(Level level);
    static void setFormat(const std::string& format);
    static void setRotation(size_t maxFileSize);

    static void debug(const std::string& module, const std::string& message, const std::string& file, int line, const std::string& function);
    static void info(const std::string& module, const std::string& message, const std::string& file, int line, const std::string& function);
    static void warn(const std::string& module, const std::string& message, const std::string& file, int line, const std::string& function);
    static void error(const std::string& module, const std::string& message, const std::string& file, int line, const std::string& function);
    static void fatal(const std::string& module, const std::string& message, const std::string& file, int line, const std::string& function);

    static bool isInitialized();
    static Level getLevel();
    static const LoggerConfig& getConfig();

    static void setInternalLogEnabled(bool enabled);
    static bool isInternalLogEnabled();

private:
    static void logInternal(Level level, const std::string& module, const std::string& message,
                           const std::string& file, int line, const std::string& function);
    static std::string levelToString(Level level);
    static std::string getCurrentTime();
    static std::string getCurrentTimeForFilename();
    static std::string getExecutableName();
    static std::string getProcessId();
    static void log(Level level, const std::string& module, const std::string& message,
                   const std::string& file, int line, const std::string& function);
    static bool openLogFile();
    static void rotateLogFile();
    static bool shouldLog(Level level);

    static LoggerConfig s_config;
    static std::string s_logDir;
    static std::string s_logName;
    static Level s_logLevel;
    static std::string s_format;
    static size_t s_maxFileSize;
    static std::ofstream s_logFile;
    static base::util::RecursiveMutex s_mutex;
    static size_t s_currentFileSize;
    static bool s_initialized;
    static std::string s_currentLogFilename;
    static std::atomic<bool> s_internalLogEnabled;
};

#define BASE_LOG_DEBUG(module, msg) base::log::Logger::debug(module, msg, __FILE__, __LINE__, __FUNCTION__)
#define BASE_LOG_INFO(module, msg) base::log::Logger::info(module, msg, __FILE__, __LINE__, __FUNCTION__)
#define BASE_LOG_WARN(module, msg) base::log::Logger::warn(module, msg, __FILE__, __LINE__, __FUNCTION__)
#define BASE_LOG_ERROR(module, msg) base::log::Logger::error(module, msg, __FILE__, __LINE__, __FUNCTION__)
#define BASE_LOG_FATAL(module, msg) base::log::Logger::fatal(module, msg, __FILE__, __LINE__, __FUNCTION__)

} // namespace log
} // namespace base

#endif // BASE_LOGGER_H
