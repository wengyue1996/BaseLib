#include "../include/core/logger.h"
#include <iostream>

using namespace base::log;

int main() {
    std::cout << "=== Logger Test Started ===" << std::endl;

    LoggerConfig config;
    config.logDir = "logs";
    config.level = Level::DEBUG;
    config.enableConsole = true;
    config.enableFile = true;
    config.enableInternalLog = true;

    Logger::init(config);
    std::cout << "Logger initialized" << std::endl;

    std::cout << "\n=== Testing Log Levels ===" << std::endl;
    BASE_LOG_DEBUG("Test", "This is a debug message");
    BASE_LOG_INFO("Test", "This is an info message");
    BASE_LOG_WARN("Test", "This is a warning message");
    BASE_LOG_ERROR("Test", "This is an error message");
    BASE_LOG_FATAL("Test", "This is a fatal message");

    std::cout << "\n=== Testing Log Level Setting ===" << std::endl;
    Logger::setLevel(Level::WARN);
    BASE_LOG_DEBUG("Test", "This debug should NOT appear");
    BASE_LOG_WARN("Test", "This warning SHOULD appear");
    BASE_LOG_ERROR("Test", "This error SHOULD appear");

    std::cout << "\n=== Testing Custom Format ===" << std::endl;
    Logger::setFormat("[%LEVEL%] [%MODULE%] %MSG%");
    BASE_LOG_INFO("FormatTest", "Message with new format");

    Logger::setFormat("[%Y-%m-%d %H:%M:%S] [%LEVEL%] %MSG%");
    BASE_LOG_INFO("FormatTest", "Message with time format");

    std::cout << "\n=== Testing Internal Log Control ===" << std::endl;
    Logger::setInternalLogEnabled(false);
    Logger::setLevel(Level::DEBUG);
    BASE_LOG_INFO("Test", "This should only go to file if internal log is disabled");

    Logger::setInternalLogEnabled(true);
    BASE_LOG_INFO("Test", "Internal log re-enabled");

    std::cout << "\n=== Testing Get Config ===" << std::endl;
    LoggerConfig currentConfig = Logger::getConfig();
    std::cout << "Current log level: " << static_cast<int>(currentConfig.level) << std::endl;
    std::cout << "Log dir: " << currentConfig.logDir << std::endl;

    std::cout << "\n=== Logger Test Completed ===" << std::endl;

    Logger::shutdown();
    return 0;
}
