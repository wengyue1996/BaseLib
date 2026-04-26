#include "../include/core/logger.h"
#include <iostream>

using namespace base::log;

void demoBasicUsage() {
    std::cout << "=== Basic Usage Demo ===" << std::endl;
    
    Logger::init("logs");
    
    LOG_DEBUG("Demo", "This is a debug message");
    LOG_INFO("Demo", "This is an info message");
    LOG_WARN("Demo", "This is a warning message");
    LOG_ERROR("Demo", "This is an error message");
    LOG_FATAL("Demo", "This is a fatal message");
    
    std::cout << "Basic usage demo completed!" << std::endl << std::endl;
}

void demoLogLevel() {
    std::cout << "=== Log Level Demo ===" << std::endl;
    
    Logger::setLevel(Level::WARN);
    std::cout << "Log level set to WARN" << std::endl;
    
    LOG_DEBUG("Demo", "This debug message should NOT be printed");
    LOG_INFO("Demo", "This info message should NOT be printed");
    LOG_WARN("Demo", "This warning message SHOULD be printed");
    LOG_ERROR("Demo", "This error message SHOULD be printed");
    LOG_FATAL("Demo", "This fatal message SHOULD be printed");
    
    std::cout << "Log level demo completed!" << std::endl << std::endl;
}

void demoCustomFormat() {
    std::cout << "=== Custom Format Demo ===" << std::endl;
    
    Logger::setLevel(Level::DEBUG);
    Logger::setFormat("[%TIME%] [%LEVEL%] [%MODULE%] [%FILE%:%LINE%] [%FUNCTION%] %MSG%");
    std::cout << "Format: [%TIME%] [%LEVEL%] [%MODULE%] [%FILE%:%LINE%] [%FUNCTION%] %MSG%" << std::endl;
    
    LOG_INFO("Demo", "This is a message with custom format");
    
    std::cout << "Custom format demo completed!" << std::endl << std::endl;
}

void demoLogRotation() {
    std::cout << "=== Log Rotation Demo ===" << std::endl;
    
    Logger::setRotation(1024);
    std::cout << "Rotation: max file size = 1024 bytes" << std::endl;
    
    for (int i = 0; i < 50; i++) {
        LOG_INFO("Demo", "Log message " + std::to_string(i));
    }
    
    std::cout << "Log rotation demo completed!" << std::endl << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   C++ Base Library - Logger Demo" << std::endl;
    std::cout << "========================================" << std::endl << std::endl;
    
    demoBasicUsage();
    demoLogLevel();
    demoCustomFormat();
    demoLogRotation();
    
    std::cout << "========================================" << std::endl;
    std::cout << "All demos completed successfully!" << std::endl;
    std::cout << "Check the logs directory for log files." << std::endl;
    std::cout << "========================================" << std::endl;
    
    Logger::shutdown();
    
    return 0;
}