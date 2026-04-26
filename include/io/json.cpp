#include "io/json.h"
#include <sstream>
#include <cstring>
#include <cctype>

namespace base {
namespace io {

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

Json::Json(bool value) : m_type(Type::BOOL), m_string_value(""), m_number_value(0), m_bool_value(value), m_array_value(nullptr), m_object_value(nullptr) {}

Json::Json(std::nullptr_t) : m_type(Type::NUL), m_string_value(""), m_number_value(0), m_bool_value(false), m_array_value(nullptr), m_object_value(nullptr) {}

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
    if (m_type != Type::ARRAY) {
        throw std::runtime_error("Json is not an array");
    }
    return (*m_array_value)[index];
}

size_t Json::size() const {
    if (m_type == Type::ARRAY && m_array_value) {
        return m_array_value->size();
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
        case Type::NUMBER:
            return std::to_string(m_number_value);
        case Type::STRING:
            return "\"" + m_string_value + "\"";
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
                    result += "\"" + pair.first + "\":" + pair.second.serialize();
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
        m_type = Type::OBJECT;
        m_object_value = new std::map<std::string, Json>();
        parseObject(jsonStr, pos);
    } else if (c == '[') {
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

void Json::parseString(const std::string& jsonStr, size_t& pos) {
    m_type = Type::STRING;
    ++pos;
    size_t start = pos;
    while (pos < jsonStr.size() && jsonStr[pos] != '\"') {
        if (jsonStr[pos] == '\\' && pos + 1 < jsonStr.size()) {
            ++pos;
        }
        ++pos;
    }
    m_string_value = jsonStr.substr(start, pos - start);
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

    while (true) {
        skipWhitespace(jsonStr, pos);
        if (jsonStr[pos] != '\"') break;
        Json keyJson;
        keyJson.parseString(jsonStr, pos);
        std::string key = keyJson.m_string_value;
        skipWhitespace(jsonStr, pos);
        if (pos < jsonStr.size() && jsonStr[pos] == ':') ++pos;
        skipWhitespace(jsonStr, pos);
        Json valueJson;
        valueJson.parseValue(jsonStr, pos);
        (*m_object_value)[key] = valueJson;
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

    while (true) {
        Json itemJson;
        itemJson.parseValue(jsonStr, pos);
        m_array_value->push_back(itemJson);
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