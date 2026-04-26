#include <map>
#include <string>
#include <vector>
#include <memory>

namespace base {
namespace util {

class Config {
public:
    static Config load(const std::string& filePath);
    Config();
    bool save(const std::string& filePath) const;
    std::string toJson() const;

    template <typename T>
    T get(const std::string& key, const T& defaultValue = T()) const;

    template <typename T>
    void set(const std::string& key, const T& value);

    bool has(const std::string& key) const;
    void remove(const std::string& key);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

template <typename T>
T Config::get(const std::string& key, const T& defaultValue) const {
    return defaultValue;
}

template <typename T>
void Config::set(const std::string& key, const T& value) {
    (void)key;
    (void)value;
}

class Exception : public std::exception {
public:
    Exception(int code, const std::string& message);
    int code() const;
    const char* what() const noexcept override;
private:
    int m_code;
    std::string m_message;
};

class ErrorCode {
public:
    static const int SUCCESS = 0;
    static const int INVALID_ARGUMENT = 1001;
    static const int FILE_NOT_FOUND = 3001;
    static const int UNKNOWN_ERROR = 9999;
};

} // namespace util
} // namespace base