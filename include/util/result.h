#ifndef BASE_RESULT_H
#define BASE_RESULT_H

#include <string>

namespace base {

class ErrorCode {
public:
    enum Code {
        SUCCESS = 0,

        INVALID_ARGUMENT = 1001,
        NULL_POINTER = 1002,
        OUT_OF_RANGE = 1003,
        INVALID_STATE = 1004,

        NETWORK_ERROR = 2001,
        CONNECTION_FAILED = 2002,
        CONNECTION_TIMEOUT = 2003,
        SEND_FAILED = 2004,
        RECV_FAILED = 2005,
        SOCKET_ERROR = 2006,
        BIND_FAILED = 2007,
        LISTEN_FAILED = 2008,
        ACCEPT_FAILED = 2009,

        FILE_NOT_FOUND = 3001,
        FILE_OPEN_FAILED = 3002,
        FILE_READ_FAILED = 3003,
        FILE_WRITE_FAILED = 3004,
        DIRECTORY_NOT_FOUND = 3005,
        PERMISSION_DENIED = 3006,

        JSON_PARSE_ERROR = 4001,
        JSON_INVALID_TYPE = 4002,
        JSON_KEY_NOT_FOUND = 4003,

        XML_PARSE_ERROR = 5001,
        XML_INVALID_FORMAT = 5002,

        UNKNOWN_ERROR = 9999,

        THREAD_ERROR = 6001,
        THREAD_CREATE_FAILED = 6002,
        THREAD_JOIN_FAILED = 6003,
        DEADLOCK = 6004
    };

    ErrorCode() : m_code(SUCCESS), m_message("") {}
    ErrorCode(Code code) : m_code(code), m_message("") {}
    ErrorCode(Code code, const std::string& message) : m_code(code), m_message(message) {}

    Code code() const { return m_code; }
    const std::string& message() const { return m_message; }

    bool isSuccess() const { return m_code == SUCCESS; }
    bool isError() const { return m_code != SUCCESS; }

    std::string toString() const {
        if (!m_message.empty()) {
            return std::string("[") + std::to_string(m_code) + "] " + m_message;
        }
        return "[" + std::to_string(m_code) + "] " + getDefaultMessage(m_code);
    }

    static std::string getDefaultMessage(Code code) {
        switch (code) {
            case SUCCESS: return "Success";
            case INVALID_ARGUMENT: return "Invalid argument";
            case NULL_POINTER: return "Null pointer";
            case OUT_OF_RANGE: return "Out of range";
            case INVALID_STATE: return "Invalid state";
            case NETWORK_ERROR: return "Network error";
            case CONNECTION_FAILED: return "Connection failed";
            case CONNECTION_TIMEOUT: return "Connection timeout";
            case SEND_FAILED: return "Send failed";
            case RECV_FAILED: return "Receive failed";
            case SOCKET_ERROR: return "Socket error";
            case BIND_FAILED: return "Bind failed";
            case LISTEN_FAILED: return "Listen failed";
            case ACCEPT_FAILED: return "Accept failed";
            case FILE_NOT_FOUND: return "File not found";
            case FILE_OPEN_FAILED: return "File open failed";
            case FILE_READ_FAILED: return "File read failed";
            case FILE_WRITE_FAILED: return "File write failed";
            case DIRECTORY_NOT_FOUND: return "Directory not found";
            case PERMISSION_DENIED: return "Permission denied";
            case JSON_PARSE_ERROR: return "JSON parse error";
            case JSON_INVALID_TYPE: return "JSON invalid type";
            case JSON_KEY_NOT_FOUND: return "JSON key not found";
            case XML_PARSE_ERROR: return "XML parse error";
            case XML_INVALID_FORMAT: return "XML invalid format";
            default: return "Unknown error";
        }
    }

private:
    Code m_code;
    std::string m_message;
};

template<typename T>
class Result {
public:
    static Result success(const T& value) {
        Result r;
        r.m_value = value;
        r.m_error = ErrorCode(ErrorCode::SUCCESS);
        r.m_has_value = true;
        return r;
    }

    static Result failure(ErrorCode::Code code) {
        Result r;
        r.m_error = ErrorCode(code);
        r.m_has_value = false;
        return r;
    }

    static Result failure(ErrorCode::Code code, const std::string& message) {
        Result r;
        r.m_error = ErrorCode(code, message);
        r.m_has_value = false;
        return r;
    }

    static Result failure(const ErrorCode& error) {
        Result r;
        r.m_error = error;
        r.m_has_value = false;
        return r;
    }

    bool isSuccess() const { return m_error.isSuccess() && m_has_value; }
    bool isError() const { return m_error.isError() || !m_has_value; }
    bool hasValue() const { return m_has_value; }

    const T& value() const { return m_value; }
    const ErrorCode& error() const { return m_error; }
    ErrorCode::Code errorCode() const { return m_error.code(); }
    const std::string& errorMessage() const { return m_error.message(); }

    T& value() { return m_value; }

    const T* ptr() const { return m_has_value ? &m_value : NULL; }
    T* ptr() { return m_has_value ? &m_value : NULL; }

    void reset() {
        m_has_value = false;
        m_error = ErrorCode(ErrorCode::SUCCESS);
    }

private:
    Result() : m_has_value(false), m_error(ErrorCode(ErrorCode::SUCCESS)) {}

    T m_value;
    ErrorCode m_error;
    bool m_has_value;
};

template<>
class Result<void> {
public:
    static Result success() {
        Result r;
        r.m_error = ErrorCode(ErrorCode::SUCCESS);
        return r;
    }

    static Result failure(ErrorCode::Code code) {
        Result r;
        r.m_error = ErrorCode(code);
        return r;
    }

    static Result failure(ErrorCode::Code code, const std::string& message) {
        Result r;
        r.m_error = ErrorCode(code, message);
        return r;
    }

    static Result failure(const ErrorCode& error) {
        Result r;
        r.m_error = error;
        return r;
    }

    bool isSuccess() const { return m_error.isSuccess(); }
    bool isError() const { return m_error.isError(); }

    const ErrorCode& error() const { return m_error; }
    ErrorCode::Code errorCode() const { return m_error.code(); }
    const std::string& errorMessage() const { return m_error.message(); }

private:
    Result() : m_error(ErrorCode(ErrorCode::SUCCESS)) {}

    ErrorCode m_error;
};

typedef Result<void> Status;

}

#endif
