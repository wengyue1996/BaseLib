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
    void formatWhat();

    int m_code;
    std::string m_message;
    std::string m_details;
    std::string m_what;
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


} // namespace util
} // namespace base

#endif // ERROR_H