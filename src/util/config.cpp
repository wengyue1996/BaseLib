#include "../include/util/config.h"
#include <fstream>
#include <algorithm>

namespace base {
namespace util {

Config::Config() = default;
Config::~Config() = default;

Config Config::load(const std::string& filePath) {
    Config config;
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return config;
    }

    std::string line;
    while (std::getline(file, line)) {
        size_t start = 0;
        while (start < line.size() && (line[start] == ' ' || line[start] == '\t')) {
            ++start;
        }
        if (start >= line.size() || line[start] == '#' || line[start] == ';' || line[start] == '[') {
            continue;
        }

        size_t eqPos = line.find('=', start);
        if (eqPos == std::string::npos) {
            continue;
        }

        std::string key = line.substr(start, eqPos - start);
        std::string value = line.substr(eqPos + 1);

        while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) {
            key.pop_back();
        }
        size_t vstart = 0;
        while (vstart < value.size() && (value[vstart] == ' ' || value[vstart] == '\t')) {
            ++vstart;
        }
        value = value.substr(vstart);

        if (!value.empty() && (value.front() == '"' || value.front() == '\'')) {
            char quote = value.front();
            value = value.substr(1);
            size_t endQuote = value.find(quote);
            if (endQuote != std::string::npos) {
                value = value.substr(0, endQuote);
            }
        }

        if (!key.empty()) {
            config.m_data[key] = value;
        }
    }

    return config;
}

bool Config::save(const std::string& filePath) const {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    for (const auto& pair : m_data) {
        file << pair.first << "=" << pair.second << "\n";
    }

    return file.good();
}

std::string Config::toJson() const {
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& pair : m_data) {
        if (!first) oss << ",";
        first = false;
        oss << "\"" << pair.first << "\":\"" << pair.second << "\"";
    }
    oss << "}";
    return oss.str();
}

bool Config::has(const std::string& key) const {
    return m_data.find(key) != m_data.end();
}

void Config::remove(const std::string& key) {
    m_data.erase(key);
}

void Config::clear() {
    m_data.clear();
}

} // namespace util
} // namespace base
