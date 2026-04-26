#include "../../include/core/logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#undef ERROR
#undef FATAL
#endif

namespace base {
namespace log {

LoggerConfig Logger::s_config;
std::string Logger::s_logDir = "logs";
std::string Logger::s_logName = "base";
Level Logger::s_logLevel = Level::INFO;
std::string Logger::s_format = "[%Y-%m-%d %H:%M:%S] [%LEVEL%] [%MODULE%] [%FILE%:%LINE%] [%FUNCTION%] %MSG%";
size_t Logger::s_maxFileSize = 200 * 1024 * 1024;
std::ofstream Logger::s_logFile;
base::util::RecursiveMutex Logger::s_mutex;
size_t Logger::s_currentFileSize = 0;
bool Logger::s_initialized = false;
std::string Logger::s_currentLogFilename = "";
std::atomic<bool> Logger::s_internalLogEnabled(true);

void Logger::init(const LoggerConfig& config) {
    base::util::LockGuard<base::util::RecursiveMutex> lock(s_mutex);

    s_config = config;
    s_logDir = config.logDir;
    s_logName = config.logName;
    s_logLevel = config.level;
    s_format = config.format;
    s_maxFileSize = config.maxFileSize;
    s_internalLogEnabled = config.enableInternalLog;

    if (s_initialized && s_logFile.is_open()) {
        s_logFile.close();
    }

#ifdef _WIN32
    system(("mkdir " + s_logDir).c_str());
#else
    system(("mkdir -p " + s_logDir).c_str());
#endif

    if (!openLogFile()) {
        std::cerr << "[Logger] Failed to open log file!" << std::endl;
    } else {
        s_initialized = true;
        if (s_config.enableInternalLog) {
            std::cout << "[Logger] Initialized successfully" << std::endl;
            std::cout << "[Logger] Log dir: " << s_logDir << std::endl;
            std::cout << "[Logger] Log level: " << levelToString(s_logLevel) << std::endl;
        }
    }
}

void Logger::shutdown() {
    base::util::LockGuard<base::util::RecursiveMutex> lock(s_mutex);

    if (s_internalLogEnabled && s_initialized) {
        std::cout << "[Logger] Shutting down..." << std::endl;
    }

    if (s_logFile.is_open()) {
        s_logFile.flush();
        s_logFile.close();
    }

    s_initialized = false;
}

void Logger::setLevel(Level level) {
    base::util::LockGuard<base::util::RecursiveMutex> lock(s_mutex);
    s_logLevel = level;
    s_config.level = level;
    if (s_internalLogEnabled) {
        std::cout << "[Logger] Level set to: " << levelToString(level) << std::endl;
    }
}

void Logger::setFormat(const std::string& format) {
    base::util::LockGuard<base::util::RecursiveMutex> lock(s_mutex);
    s_format = format;
    s_config.format = format;
}

void Logger::setRotation(size_t maxFileSize) {
    base::util::LockGuard<base::util::RecursiveMutex> lock(s_mutex);
    s_maxFileSize = maxFileSize;
    s_config.maxFileSize = maxFileSize;
}

bool Logger::isInitialized() {
    return s_initialized;
}

Level Logger::getLevel() {
    return s_logLevel;
}

const LoggerConfig& Logger::getConfig() {
    return s_config;
}

void Logger::setInternalLogEnabled(bool enabled) {
    s_internalLogEnabled = enabled;
}

bool Logger::isInternalLogEnabled() {
    return s_internalLogEnabled;
}

void Logger::debug(const std::string& module, const std::string& message, const std::string& file, int line, const std::string& function) {
    log(Level::DEBUG, module, message, file, line, function);
}

void Logger::info(const std::string& module, const std::string& message, const std::string& file, int line, const std::string& function) {
    log(Level::INFO, module, message, file, line, function);
}

void Logger::warn(const std::string& module, const std::string& message, const std::string& file, int line, const std::string& function) {
    log(Level::WARN, module, message, file, line, function);
}

void Logger::error(const std::string& module, const std::string& message, const std::string& file, int line, const std::string& function) {
    log(Level::ERROR, module, message, file, line, function);
}

void Logger::fatal(const std::string& module, const std::string& message, const std::string& file, int line, const std::string& function) {
    log(Level::FATAL, module, message, file, line, function);
}

bool Logger::shouldLog(Level level) {
    return static_cast<int>(level) >= static_cast<int>(s_logLevel);
}

std::string Logger::levelToString(Level level) {
    switch (level) {
        case Level::DEBUG: return "DEBUG";
        case Level::INFO:  return "INFO";
        case Level::WARN:  return "WARN";
        case Level::ERROR: return "ERROR";
        case Level::FATAL: return "FATAL";
        default:           return "UNKNOWN";
    }
}

std::string Logger::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();

    return ss.str();
}

std::string Logger::getCurrentTimeForFilename() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y%m%d%H%M%S");

    return ss.str();
}

std::string Logger::getExecutableName() {
#ifdef _WIN32
    char filename[MAX_PATH];
    GetModuleFileNameA(NULL, filename, MAX_PATH);
    std::string path(filename);
    size_t lastSlash = path.find_last_of('\\');
    if (lastSlash != std::string::npos) {
        path = path.substr(lastSlash + 1);
    }
    size_t dotPos = path.find_last_of('.');
    if (dotPos != std::string::npos) {
        path = path.substr(0, dotPos);
    }
    return path;
#else
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        std::string path(buffer);
        size_t lastSlash = path.find_last_of('/');
        if (lastSlash != std::string::npos) {
            path = path.substr(lastSlash + 1);
        }
        size_t dotPos = path.find_last_of('.');
        if (dotPos != std::string::npos) {
            path = path.substr(0, dotPos);
        }
        return path;
    }
    return "unknown";
#endif
}

std::string Logger::getProcessId() {
#ifdef _WIN32
    return std::to_string(GetCurrentProcessId());
#else
    return std::to_string(getpid());
#endif
}

bool Logger::openLogFile() {
    if (s_logFile.is_open()) {
        s_logFile.close();
    }

    std::string execName = getExecutableName();
    std::string pid = getProcessId();
    std::string timeStr = getCurrentTimeForFilename();

    s_currentLogFilename = execName + "_" + pid + "_" + timeStr + ".log";
    std::string logFilePath = s_logDir + "/" + s_currentLogFilename;

    s_logFile.open(logFilePath, std::ios::app);

    if (s_logFile.is_open()) {
        s_logFile.seekp(0, std::ios::end);
        s_currentFileSize = static_cast<size_t>(s_logFile.tellp());
        return true;
    }

    return false;
}

void Logger::rotateLogFile() {
    if (s_logFile.is_open()) {
        s_logFile.flush();
        s_logFile.close();
    }

    if (!openLogFile()) {
        std::cerr << "[Logger] Failed to open new log file during rotation!" << std::endl;
    }
}

void Logger::log(Level level, const std::string& module, const std::string& message,
                const std::string& file, int line, const std::string& function) {
    if (!s_initialized) {
        init(s_config);
    }

    if (!shouldLog(level)) {
        return;
    }

    base::util::LockGuard<base::util::RecursiveMutex> lock(s_mutex);

    if (s_currentFileSize >= s_maxFileSize) {
        rotateLogFile();
    }

    if (s_config.enableFile && !s_logFile.is_open()) {
        if (!openLogFile()) {
            return;
        }
    }

    std::string logLine = s_format;

    size_t pos = logLine.find("%TIME%");
    if (pos != std::string::npos) {
        logLine.replace(pos, 6, getCurrentTime());
    }

    pos = logLine.find("%LEVEL%");
    if (pos != std::string::npos) {
        logLine.replace(pos, 7, levelToString(level));
    }

    pos = logLine.find("%MODULE%");
    if (pos != std::string::npos) {
        logLine.replace(pos, 8, module);
    }

    pos = logLine.find("%FILE%");
    if (pos != std::string::npos) {
        std::string filename = file;
        size_t lastSlash = filename.find_last_of('/');
        if (lastSlash == std::string::npos) {
            lastSlash = filename.find_last_of('\\');
        }
        if (lastSlash != std::string::npos) {
            filename = filename.substr(lastSlash + 1);
        }
        logLine.replace(pos, 6, filename);
    }

    pos = logLine.find("%LINE%");
    if (pos != std::string::npos) {
        logLine.replace(pos, 6, std::to_string(line));
    }

    pos = logLine.find("%FUNCTION%");
    if (pos != std::string::npos) {
        logLine.replace(pos, 10, function);
    }

    pos = logLine.find("%MSG%");
    if (pos != std::string::npos) {
        logLine.replace(pos, 5, message);
    }

    if (s_config.enableFile && s_logFile.is_open()) {
        s_logFile << logLine << std::endl;
        s_logFile.flush();
        s_currentFileSize += logLine.length() + 1;
    }

    if (s_config.enableConsole) {
        if (level == Level::ERROR || level == Level::FATAL) {
            std::cerr << logLine << std::endl;
        } else {
            std::cout << logLine << std::endl;
        }
    }
}

} // namespace log
} // namespace base
