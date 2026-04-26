#include "../include/util/error.h"
#include <sstream>

namespace base {
namespace util {

Exception::Exception(int code, const std::string& message)
    : m_code(code), m_message(message), m_details("") {}

Exception::Exception(int code, const std::string& message, const std::string& details)
    : m_code(code), m_message(message), m_details(details) {}

int Exception::code() const {
    return m_code;
}

const std::string& Exception::message() const {
    return m_message;
}

const std::string& Exception::details() const {
    return m_details;
}

const char* Exception::what() const noexcept {
    std::ostringstream oss;
    oss << "[" << m_code << "] " << m_message;
    if (!m_details.empty()) {
        oss << " (" << m_details << ")";
    }
    m_what = oss.str();
    return m_what.c_str();
}

void Exception::setDetails(const std::string& details) {
    m_details = details;
}

std::map<ErrorCategory::Category, std::string> ErrorCategory::s_category_names = {
    {ErrorCategory::SYSTEM, "System Error"},
    {ErrorCategory::NETWORK, "Network Error"},
    {ErrorCategory::FILE, "File Error"},
    {ErrorCategory::MEMORY, "Memory Error"},
    {ErrorCategory::LOGIC, "Logic Error"},
    {ErrorCategory::RUNTIME, "Runtime Error"},
    {ErrorCategory::UNKNOWN, "Unknown Error"}
};

const std::string& ErrorCategory::getCategoryName(Category category) {
    return s_category_names[category];
}

ErrorCategory::Category ErrorCategory::getCategory(int errorCode) {
    int categoryCode = (errorCode / 1000) * 1000;
    switch (categoryCode) {
        case 1000: return SYSTEM;
        case 2000: return NETWORK;
        case 3000: return FILE;
        case 4000: return MEMORY;
        case 5000: return LOGIC;
        case 6000: return RUNTIME;
        default: return UNKNOWN;
    }
}

const std::string& ErrorCode::getErrorMessage(int errorCode) {
    static std::map<int, std::string> error_messages = {
        {SUCCESS, "Success"},
        {INVALID_ARGUMENT, "Invalid argument"},
        {NULL_POINTER, "Null pointer"},
        {OUT_OF_RANGE, "Out of range"},
        {INVALID_OPERATION, "Invalid operation"},
        {NETWORK_TIMEOUT, "Network timeout"},
        {NETWORK_CONNECTION_REFUSED, "Connection refused"},
        {NETWORK_HOST_UNREACHABLE, "Host unreachable"},
        {NETWORK_SOCKET_ERROR, "Socket error"},
        {FILE_NOT_FOUND, "File not found"},
        {FILE_PERMISSION_DENIED, "Permission denied"},
        {FILE_ALREADY_EXISTS, "File already exists"},
        {FILE_IO_ERROR, "File I/O error"},
        {MEMORY_ALLOC_FAILED, "Memory allocation failed"},
        {MEMORY_FREE_FAILED, "Memory free failed"},
        {INVALID_CONFIG, "Invalid configuration"},
        {INVALID_FORMAT, "Invalid format"},
        {PARSE_ERROR, "Parse error"},
        {UNKNOWN_ERROR, "Unknown error"}
    };

    auto it = error_messages.find(errorCode);
    if (it != error_messages.end()) {
        return it->second;
    }
    return error_messages[UNKNOWN_ERROR];
}

} // namespace util
} // namespace base