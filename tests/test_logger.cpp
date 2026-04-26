#include "../include/core/logger.h"
#include <iostream>

using namespace base::log;

int main() {
    std::cout << "=== Logger Test Started ===" << std::endl;
    
    Logger::init("logs");
    std::cout << "Logger initialized" << std::endl;
    
    std::cout << "\n=== Testing Log Levels ===" << std::endl;
    LOG_DEBUG("Test", "This is a debug message");
    LOG_INFO("Test", "This is an info message");
    LOG_WARN("Test", "This is a warning message");
    LOG_ERROR("Test", "This is an error message");
    LOG_FATAL("Test", "This is a fatal message");
    
    std::cout << "\n=== Testing Log Level Setting ===" << std::endl;
    Logger::setLevel(Level::WARN);
    std::cout << "Log level set to WARN" << std::endl;
    
    LOG_DEBUG("Test", "This debug message should NOT be printed");
    LOG_INFO("Test", "This info message should NOT be printed");
    LOG_WARN("Test", "This warning message should be printed");
    LOG_ERROR("Test", "This error message should be printed");
    LOG_FATAL("Test", "This fatal message should be printed");
    
    std::cout << "\n=== Testing Custom Format ===" << std::endl;
    Logger::setLevel(Level::DEBUG);
    Logger::setFormat("[%TIME%] [%LEVEL%] [%MODULE%] [%FILE%:%LINE%] [%FUNCTION%] %MSG%");
    LOG_INFO("Test", "Custom format message");
    
    std::cout << "\n=== Testing Log Rotation ===" << std::endl;
    Logger::setRotation(1024);
    for (int i = 0; i < 100; i++) {
        LOG_INFO("RotationTest", "Log message " + std::to_string(i));
    }
    
    std::cout << "\n=== Logger Test Completed ===" << std::endl;
    
    Logger::shutdown();
    
    return 0;
}