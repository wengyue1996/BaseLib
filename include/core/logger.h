#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <memory>
#include <atomic>
#include <mutex>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>

namespace base {
namespace log {

enum class Level {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

class Logger {
public:
    static void init(const std::string& logDir);
    static void setLevel(Level level);
    static void setFormat(const std::string& format);
    static void setRotation(size_t maxFileSize);
    
    static void debug(const std::string& module, const std::string& message, const std::string& file, int line, const std::string& function);
    static void info(const std::string& module, const std::string& message, const std::string& file, int line, const std::string& function);
    static void warn(const std::string& module, const std::string& message, const std::string& file, int line, const std::string& function);
    static void error(const std::string& module, const std::string& message, const std::string& file, int line, const std::string& function);
    static void fatal(const std::string& module, const std::string& message, const std::string& file, int line, const std::string& function);
    
    static void shutdown();

private:
    static std::string levelToString(Level level);
    static std::string getCurrentTime();
    static std::string getCurrentTimeForFilename();
    static std::string getExecutableName();
    static std::string getProcessId();
    static void log(Level level, const std::string& module, const std::string& message, const std::string& file, int line, const std::string& function);
    static bool openLogFile();
    static void rotateLogFile();
    
    static std::string m_logDir;
    static std::string m_logName;
    static Level m_logLevel;
    static std::string m_format;
    static size_t m_maxFileSize;
    static std::ofstream m_logFile;
    static std::mutex m_mutex;
    static size_t m_currentFileSize;
    static bool m_initialized;
    static std::string m_currentLogFilename;
};

// 宏定义，简化日志调用
#define LOG_DEBUG(module, message) Logger::debug(module, message, __FILE__, __LINE__, __FUNCTION__)
#define LOG_INFO(module, message) Logger::info(module, message, __FILE__, __LINE__, __FUNCTION__)
#define LOG_WARN(module, message) Logger::warn(module, message, __FILE__, __LINE__, __FUNCTION__)
#define LOG_ERROR(module, message) Logger::error(module, message, __FILE__, __LINE__, __FUNCTION__)
#define LOG_FATAL(module, message) Logger::fatal(module, message, __FILE__, __LINE__, __FUNCTION__)

} // namespace log
} // namespace base

#endif // LOGGER_H