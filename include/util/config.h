#ifndef CONFIG_H
#define CONFIG_H

#include <map>
#include <string>
#include <vector>
#include <sstream>
#include "error.h"

namespace base {
namespace util {

class Config {
public:
    static Config load(const std::string& filePath);
    Config();
    ~Config();
    bool save(const std::string& filePath) const;
    std::string toJson() const;

    template <typename T>
    T get(const std::string& key, const T& defaultValue = T()) const;

    template <typename T>
    void set(const std::string& key, const T& value);

    bool has(const std::string& key) const;
    void remove(const std::string& key);

private:
    std::map<std::string, std::string> m_data;

    template <typename T>
    static std::string toString(const T& value) {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }

    static std::string toString(const std::string& value) { return value; }
    static std::string toString(const char* value) { return value; }
};

template <typename T>
T Config::get(const std::string& key, const T& defaultValue) const {
    auto it = m_data.find(key);
    if (it == m_data.end()) {
        return defaultValue;
    }
    std::istringstream iss(it->second);
    T value;
    iss >> value;
    return value;
}

template <typename T>
void Config::set(const std::string& key, const T& value) {
    m_data[key] = toString(value);
}

} // namespace util
} // namespace base

#endif // CONFIG_H
