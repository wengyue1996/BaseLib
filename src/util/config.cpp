#include "../include/util/config.h"

namespace base {
namespace util {

Config::Config() = default;
Config::~Config() = default;

Config Config::load(const std::string& filePath) {
    (void)filePath;
    return Config();
}

bool Config::save(const std::string& filePath) const {
    (void)filePath;
    return true;
}

std::string Config::toJson() const {
    return "{}";
}

bool Config::has(const std::string& key) const {
    return m_data.find(key) != m_data.end();
}

void Config::remove(const std::string& key) {
    m_data.erase(key);
}

} // namespace util
} // namespace base
