#ifndef ERROR_H
#define ERROR_H

#include <string>
#include <exception>
#include <map>

namespace base {
namespace util {

class Exception : public std::exception {
public:
    Exception(int code, const std::string& message);
    Exception(int code, const std::string& message, const std::string& details);

    int code() const;
    const std::string& message() const;
    const std::string& details() const;
    const char* what() const noexcept override;

    void setDetails(const std::string& details);

private:
    int m_code;
    std::string m_message;
    std::string m_details;
    mutable std::string m_what;
};

class ErrorCategory {
public:
    enum Category {
        SYSTEM = 1000,
        NETWORK = 2000,
        FILE = 3000,
        MEMORY = 4000,
        LOGIC = 5000,
        RUNTIME = 6000,
        UNKNOWN = 9999
    };

    static const std::string& getCategoryName(Category category);
    static Category getCategory(int errorCode);

private:
    static std::map<Category, std::string> s_category_names;
};

class ErrorCode {
public:
    static const int SUCCESS = 0;

    static const int INVALID_ARGUMENT = 1001;
    static const int NULL_POINTER = 1002;
    static const int OUT_OF_RANGE = 1003;
    static const int INVALID_OPERATION = 1004;

    static const int NETWORK_TIMEOUT = 2001;
    static const int NETWORK_CONNECTION_REFUSED = 2002;
    static const int NETWORK_HOST_UNREACHABLE = 2003;
    static const int NETWORK_SOCKET_ERROR = 2004;

    static const int FILE_NOT_FOUND = 3001;
    static const int FILE_PERMISSION_DENIED = 3002;
    static const int FILE_ALREADY_EXISTS = 3003;
    static const int FILE_IO_ERROR = 3004;

    static const int MEMORY_ALLOC_FAILED = 4001;
    static const int MEMORY_FREE_FAILED = 4002;

    static const int INVALID_CONFIG = 5001;
    static const int INVALID_FORMAT = 5002;
    static const int PARSE_ERROR = 5003;

    static const int UNKNOWN_ERROR = 9999;

    static const std::string& getErrorMessage(int errorCode);
};

} // namespace util
} // namespace base

#endif // ERROR_H