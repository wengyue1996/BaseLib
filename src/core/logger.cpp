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
// 处理Windows宏冲突
#undef ERROR
#undef FATAL
#endif

namespace base {
namespace log {

std::string Logger::m_logDir = "logs";
std::string Logger::m_logName = "base";
Level Logger::m_logLevel = Level::INFO;
std::string Logger::m_format = "[%Y-%m-%d %H:%M:%S] [%LEVEL%] [%MODULE%] [%FILE%:%LINE%] [%FUNCTION%] %MSG%";
size_t Logger::m_maxFileSize = 200 * 1024 * 1024; // 200MB
std::ofstream Logger::m_logFile;
std::mutex Logger::m_mutex;
size_t Logger::m_currentFileSize = 0;
bool Logger::m_initialized = false;
std::string Logger::m_currentLogFilename = "";

void Logger::init(const std::string& logDir) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_initialized) {
        if (m_logFile.is_open()) {
            m_logFile.close();
        }
    }
    
    m_logDir = logDir;
    
#ifdef _WIN32
    system(("mkdir " + m_logDir).c_str());
#else
    system(("mkdir -p " + m_logDir).c_str());
#endif
    
    if (!openLogFile()) {
        std::cerr << "Failed to open log file!" << std::endl;
    } else {
        m_initialized = true;
    }
}

void Logger::setLevel(Level level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_logLevel = level;
}

void Logger::setFormat(const std::string& format) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_format = format;
}

void Logger::setRotation(size_t maxFileSize) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_maxFileSize = maxFileSize;
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_logFile.is_open()) {
        m_logFile.flush();
        m_logFile.close();
    }
    
    m_initialized = false;
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
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
    
    std::string execName = getExecutableName();
    std::string pid = getProcessId();
    std::string timeStr = getCurrentTimeForFilename();
    
    m_currentLogFilename = execName + "_" + pid + "_" + timeStr + ".log";
    std::string logFilePath = m_logDir + "/" + m_currentLogFilename;
    
    m_logFile.open(logFilePath, std::ios::app);
    
    if (m_logFile.is_open()) {
        m_logFile.seekp(0, std::ios::end);
        m_currentFileSize = static_cast<size_t>(m_logFile.tellp());
        return true;
    }
    
    return false;
}

void Logger::rotateLogFile() {
    if (m_logFile.is_open()) {
        m_logFile.flush();
        m_logFile.close();
    }
    
    if (!openLogFile()) {
        std::cerr << "Failed to open new log file during rotation!" << std::endl;
    }
}

void Logger::log(Level level, const std::string& module, const std::string& message, const std::string& file, int line, const std::string& function) {
    if (!m_initialized) {
        init(m_logDir);
    }
    
    if (level < m_logLevel) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_currentFileSize >= m_maxFileSize) {
        rotateLogFile();
    }
    
    if (!m_logFile.is_open()) {
        if (!openLogFile()) {
            std::cerr << "Log file is not open!" << std::endl;
            return;
        }
    }
    
    std::string logLine = m_format;
    
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
        // 提取文件名（包含扩展名）
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
    
    m_logFile << logLine << std::endl;
    if (!m_logFile.good()) {
        std::cerr << "Failed to write to log file!" << std::endl;
        return;
    }
    
    m_logFile.flush();
    if (!m_logFile.good()) {
        std::cerr << "Failed to flush log file!" << std::endl;
        return;
    }
    
    m_currentFileSize += logLine.length() + 1;
    
    std::cout << logLine << std::endl;
}

} // namespace log
} // namespace base