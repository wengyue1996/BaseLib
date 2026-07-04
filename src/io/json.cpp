#include "../include/io/json.h"
#include <sstream>
#include <cstring>
#include <cctype>
#include <stdexcept>

namespace base {
namespace io {

static std::string escapeString(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    for (size_t i = 0; i < str.size(); ++i) {
        char c = str[i];
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    result += buf;
                } else {
                    result += c;
                }
                break;
        }
    }
    return result;
}

Json::Json() : m_type(Type::NUL), m_number_value(0), m_bool_value(false), m_array_value(nullptr), m_object_value(nullptr) {}

Json::Json(Type type) : m_type(type), m_number_value(0), m_bool_value(false), m_array_value(nullptr), m_object_value(nullptr) {
    if (type == Type::ARRAY) {
        m_array_value = new std::vector<Json>();
    } else if (type == Type::OBJECT) {
        m_object_value = new std::map<std::string, Json>();
    }
}

Json::Json(int value) : m_type(Type::NUMBER), m_number_value(static_cast<double>(value)), m_bool_value(false), m_array_value(nullptr), m_object_value(nullptr) {}

Json::Json(double value) : m_type(Type::NUMBER), m_number_value(value), m_bool_value(false), m_array_value(nullptr), m_object_value(nullptr) {}

Json::Json(const std::string& value) : m_type(Type::STRING), m_string_value(value), m_number_value(0), m_bool_value(false), m_array_value(nullptr), m_object_value(nullptr) {}

Json::Json(const char* value) : m_type(Type::STRING), m_string_value(value), m_number_value(0), m_bool_value(false), m_array_value(nullptr), m_object_value(nullptr) {}

Json::Json(bool value) : m_type(Type::BOOL), m_string_value(""), m_number_value(0), m_bool_value(value), m_array_value(nullptr), m_object_value(nullptr) {}

Json::Json(std::nullptr_t) : m_type(Type::NUL), m_string_value(""), m_number_value(0), m_bool_value(false), m_array_value(nullptr), m_object_value(nullptr) {}

Json::Json(const Json& other) : m_type(other.m_type), m_string_value(other.m_string_value), m_number_value(other.m_number_value), m_bool_value(other.m_bool_value), m_array_value(nullptr), m_object_value(nullptr) {
    if (other.m_array_value) {
        m_array_value = new std::vector<Json>(*other.m_array_value);
    }
    if (other.m_object_value) {
        m_object_value = new std::map<std::string, Json>(*other.m_object_value);
    }
}

Json::Json(Json&& other) noexcept : m_type(other.m_type), m_string_value(std::move(other.m_string_value)), m_number_value(other.m_number_value), m_bool_value(other.m_bool_value), m_array_value(other.m_array_value), m_object_value(other.m_object_value) {
    other.m_type = Type::NUL;
    other.m_array_value = nullptr;
    other.m_object_value = nullptr;
    other.m_number_value = 0;
    other.m_bool_value = false;
}

Json& Json::operator=(const Json& other) {
    if (this != &other) {
        delete m_array_value;
        delete m_object_value;
        m_type = other.m_type;
        m_string_value = other.m_string_value;
        m_number_value = other.m_number_value;
        m_bool_value = other.m_bool_value;
        m_array_value = other.m_array_value ? new std::vector<Json>(*other.m_array_value) : nullptr;
        m_object_value = other.m_object_value ? new std::map<std::string, Json>(*other.m_object_value) : nullptr;
    }
    return *this;
}

Json& Json::operator=(Json&& other) noexcept {
    if (this != &other) {
        delete m_array_value;
        delete m_object_value;
        m_type = other.m_type;
        m_string_value = std::move(other.m_string_value);
        m_number_value = other.m_number_value;
        m_bool_value = other.m_bool_value;
        m_array_value = other.m_array_value;
        m_object_value = other.m_object_value;
        other.m_type = Type::NUL;
        other.m_array_value = nullptr;
        other.m_object_value = nullptr;
        other.m_number_value = 0;
        other.m_bool_value = false;
    }
    return *this;
}

Json::~Json() {
    delete m_array_value;
    delete m_object_value;
}

Json::Type Json::type() const {
    return m_type;
}

bool Json::isObject() const {
    return m_type == Type::OBJECT;
}

bool Json::isArray() const {
    return m_type == Type::ARRAY;
}

bool Json::isString() const {
    return m_type == Type::STRING;
}

bool Json::isNumber() const {
    return m_type == Type::NUMBER;
}

bool Json::isBoolean() const {
    return m_type == Type::BOOL;
}

bool Json::isNull() const {
    return m_type == Type::NUL;
}

Json& Json::operator[](const std::string& key) {
    if (m_type != Type::OBJECT) {
        if (m_type != Type::NUL) {
            throw std::runtime_error("Json is not an object");
        }
        m_type = Type::OBJECT;
        m_object_value = new std::map<std::string, Json>();
    }
    return (*m_object_value)[key];
}

const Json& Json::operator[](const std::string& key) const {
    if (m_type != Type::OBJECT || !m_object_value) {
        throw std::runtime_error("Json is not an object");
    }
    auto it = m_object_value->find(key);
    if (it == m_object_value->end()) {
        throw std::runtime_error("Key not found: " + key);
    }
    return it->second;
}

bool Json::has(const std::string& key) const {
    if (m_type != Type::OBJECT || !m_object_value) {
        return false;
    }
    return m_object_value->find(key) != m_object_value->end();
}

void Json::remove(const std::string& key) {
    if (m_type == Type::OBJECT && m_object_value) {
        m_object_value->erase(key);
    }
}

Json& Json::operator[](size_t index) {
    if (m_type != Type::ARRAY || !m_array_value) {
        throw std::runtime_error("Json is not an array");
    }
    return (*m_array_value)[index];
}

const Json& Json::operator[](size_t index) const {
    if (m_type != Type::ARRAY || !m_array_value) {
        throw std::runtime_error("Json is not an array");
    }
    return (*m_array_value)[index];
}

size_t Json::size() const {
    if (m_type == Type::ARRAY && m_array_value) {
        return m_array_value->size();
    }
    if (m_type == Type::OBJECT && m_object_value) {
        return m_object_value->size();
    }
    return 0;
}

void Json::push_back(const Json& value) {
    if (m_type != Type::ARRAY) {
        if (m_type != Type::NUL) {
            throw std::runtime_error("Json is not an array");
        }
        m_type = Type::ARRAY;
        m_array_value = new std::vector<Json>();
    }
    m_array_value->push_back(value);
}

void Json::pop_back() {
    if (m_type == Type::ARRAY && m_array_value && !m_array_value->empty()) {
        m_array_value->pop_back();
    }
}

void Json::clear() {
    m_type = Type::NUL;
    delete m_array_value;
    m_array_value = nullptr;
    delete m_object_value;
    m_object_value = nullptr;
    m_string_value.clear();
    m_number_value = 0;
    m_bool_value = false;
}

int Json::asInt() const {
    return static_cast<int>(m_number_value);
}

double Json::asDouble() const {
    return m_number_value;
}

double Json::asNumber() const {
    return m_number_value;
}

const std::string& Json::asString() const {
    return m_string_value;
}

bool Json::asBool() const {
    return m_bool_value;
}

std::vector<std::string> Json::keys() const {
    std::vector<std::string> result;
    if (m_type == Type::OBJECT && m_object_value) {
        for (const auto& pair : *m_object_value) {
            result.push_back(pair.first);
        }
    }
    return result;
}

std::string Json::toString() const {
    return serialize();
}

std::string Json::serialize() const {
    switch (m_type) {
        case Type::NUL:
            return "null";
        case Type::NUMBER: {
            if (m_number_value == static_cast<int>(m_number_value)) {
                return std::to_string(static_cast<int>(m_number_value));
            }
            return std::to_string(m_number_value);
        }
        case Type::STRING:
            return "\"" + escapeString(m_string_value) + "\"";
        case Type::BOOL:
            return m_bool_value ? "true" : "false";
        case Type::ARRAY: {
            std::string result = "[";
            if (m_array_value) {
                for (size_t i = 0; i < m_array_value->size(); ++i) {
                    if (i > 0) result += ",";
                    result += (*m_array_value)[i].serialize();
                }
            }
            result += "]";
            return result;
        }
        case Type::OBJECT: {
            std::string result = "{";
            if (m_object_value) {
                bool first = true;
                for (const auto& pair : *m_object_value) {
                    if (!first) result += ",";
                    first = false;
                    result += "\"" + escapeString(pair.first) + "\":" + pair.second.serialize();
                }
            }
            result += "}";
            return result;
        }
        default:
            return "null";
    }
}

Json Json::parse(const std::string& jsonStr) {
    Json json;
    size_t pos = 0;
    json.skipWhitespace(jsonStr, pos);
    json.parseValue(jsonStr, pos);
    return json;
}

void Json::parseValue(const std::string& jsonStr, size_t& pos) {
    skipWhitespace(jsonStr, pos);
    if (pos >= jsonStr.size()) return;

    char c = jsonStr[pos];
    if (c == '{') {
        clear();
        m_type = Type::OBJECT;
        m_object_value = new std::map<std::string, Json>();
        parseObject(jsonStr, pos);
    } else if (c == '[') {
        clear();
        m_type = Type::ARRAY;
        m_array_value = new std::vector<Json>();
        parseArray(jsonStr, pos);
    } else if (c == '\"') {
        parseString(jsonStr, pos);
    } else if (c == 't') {
        parseTrue(jsonStr, pos);
    } else if (c == 'f') {
        parseFalse(jsonStr, pos);
    } else if (c == 'n') {
        parseNull(jsonStr, pos);
    } else {
        parseNumber(jsonStr, pos);
    }
}

static std::string decodeJsonString(const std::string& raw) {
    std::string result;
    for (size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] == '\\' && i + 1 < raw.size()) {
            ++i;
            switch (raw[i]) {
                case '"':  result += '"'; break;
                case '\\': result += '\\'; break;
                case '/':  result += '/'; break;
                case 'b':  result += '\b'; break;
                case 'f':  result += '\f'; break;
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                case 'u': {
                    if (i + 4 < raw.size()) {
                        std::string hex = raw.substr(i + 1, 4);
                        unsigned int cp = 0;
                        std::istringstream iss(hex);
                        iss >> std::hex >> cp;
                        if (cp < 0x80) {
                            result += static_cast<char>(cp);
                        } else if (cp < 0x800) {
                            result += static_cast<char>(0xC0 | (cp >> 6));
                            result += static_cast<char>(0x80 | (cp & 0x3F));
                        } else {
                            result += static_cast<char>(0xE0 | (cp >> 12));
                            result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                            result += static_cast<char>(0x80 | (cp & 0x3F));
                        }
                        i += 4;
                    }
                    break;
                }
                default: result += raw[i]; break;
            }
        } else {
            result += raw[i];
        }
    }
    return result;
}

void Json::parseString(const std::string& jsonStr, size_t& pos) {
    m_type = Type::STRING;
    m_string_value.clear();
    ++pos;
    size_t start = pos;
    while (pos < jsonStr.size() && jsonStr[pos] != '\"') {
        if (jsonStr[pos] == '\\' && pos + 1 < jsonStr.size()) {
            ++pos;
        }
        ++pos;
    }
    std::string raw = jsonStr.substr(start, pos - start);
    m_string_value = decodeJsonString(raw);
    if (pos < jsonStr.size()) ++pos;
}

void Json::parseNumber(const std::string& jsonStr, size_t& pos) {
    m_type = Type::NUMBER;
    size_t start = pos;
    if (pos < jsonStr.size() && (jsonStr[pos] == '-' || jsonStr[pos] == '+')) ++pos;
    while (pos < jsonStr.size() && std::isdigit(jsonStr[pos])) ++pos;
    if (pos < jsonStr.size() && jsonStr[pos] == '.') {
        ++pos;
        while (pos < jsonStr.size() && std::isdigit(jsonStr[pos])) ++pos;
    }
    if (pos < jsonStr.size() && (jsonStr[pos] == 'e' || jsonStr[pos] == 'E')) {
        ++pos;
        if (pos < jsonStr.size() && (jsonStr[pos] == '+' || jsonStr[pos] == '-')) ++pos;
        while (pos < jsonStr.size() && std::isdigit(jsonStr[pos])) ++pos;
    }
    m_number_value = std::stod(jsonStr.substr(start, pos - start));
}

void Json::parseObject(const std::string& jsonStr, size_t& pos) {
    ++pos;
    skipWhitespace(jsonStr, pos);
    if (pos < jsonStr.size() && jsonStr[pos] == '}') {
        ++pos;
        return;
    }

    while (pos < jsonStr.size()) {
        skipWhitespace(jsonStr, pos);
        if (jsonStr[pos] != '\"') break;
        ++pos;
        size_t keyStart = pos;
        while (pos < jsonStr.size() && jsonStr[pos] != '\"') {
            if (jsonStr[pos] == '\\' && pos + 1 < jsonStr.size()) {
                ++pos;
            }
            ++pos;
        }
        std::string key = decodeJsonString(jsonStr.substr(keyStart, pos - keyStart));
        if (pos < jsonStr.size()) ++pos;
        skipWhitespace(jsonStr, pos);
        if (pos < jsonStr.size() && jsonStr[pos] == ':') ++pos;
        skipWhitespace(jsonStr, pos);
        Json valueJson;
        valueJson.parseValue(jsonStr, pos);
        (*m_object_value)[key] = std::move(valueJson);
        skipWhitespace(jsonStr, pos);
        if (pos < jsonStr.size() && jsonStr[pos] == ',') {
            ++pos;
        } else {
            break;
        }
    }
    skipWhitespace(jsonStr, pos);
    if (pos < jsonStr.size() && jsonStr[pos] == '}') ++pos;
}

void Json::parseArray(const std::string& jsonStr, size_t& pos) {
    ++pos;
    skipWhitespace(jsonStr, pos);
    if (pos < jsonStr.size() && jsonStr[pos] == ']') {
        ++pos;
        return;
    }

    while (pos < jsonStr.size()) {
        Json itemJson;
        itemJson.parseValue(jsonStr, pos);
        m_array_value->push_back(std::move(itemJson));
        skipWhitespace(jsonStr, pos);
        if (pos < jsonStr.size() && jsonStr[pos] == ',') {
            ++pos;
        } else {
            break;
        }
    }
    skipWhitespace(jsonStr, pos);
    if (pos < jsonStr.size() && jsonStr[pos] == ']') ++pos;
}

void Json::parseTrue(const std::string& jsonStr, size_t& pos) {
    m_type = Type::BOOL;
    m_bool_value = true;
    if (jsonStr.substr(pos, 4) == "true") pos += 4;
}

void Json::parseFalse(const std::string& jsonStr, size_t& pos) {
    m_type = Type::BOOL;
    m_bool_value = false;
    if (jsonStr.substr(pos, 5) == "false") pos += 5;
}

void Json::parseNull(const std::string& jsonStr, size_t& pos) {
    m_type = Type::NUL;
    if (jsonStr.substr(pos, 4) == "null") pos += 4;
}

void Json::skipWhitespace(const std::string& jsonStr, size_t& pos) {
    while (pos < jsonStr.size() && std::isspace(jsonStr[pos])) ++pos;
}

} // namespace io
} // namespace base
