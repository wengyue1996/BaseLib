#include "../include/util/config.h"

namespace base {
namespace util {

class Config::Impl {
public:
    std::map<std::string, std::string> data;
};

Config::Config() : m_impl(new Impl()) {}

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
    (void)key;
    return false;
}

void Config::remove(const std::string& key) {
    (void)key;
}

Exception::Exception(int code, const std::string& message)
    : m_code(code), m_message(message) {}

int Exception::code() const {
    return m_code;
}

const char* Exception::what() const noexcept {
    return m_message.c_str();
}

} // namespace util
} // namespace base