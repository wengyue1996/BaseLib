#ifndef JSON_H
#define JSON_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>

namespace base {
namespace io {

class Json {
public:
    enum class Type {
        NUL,
        NUMBER,
        STRING,
        BOOL,
        ARRAY,
        OBJECT
    };

    static Json parse(const std::string& jsonStr);
    std::string toString() const;

    Json();
    Json(Type type);
    Json(int value);
    Json(double value);
    Json(const std::string& value);
    Json(bool value);
    Json(std::nullptr_t);

    Type type() const;

    bool isObject() const;
    bool isArray() const;
    bool isString() const;
    bool isNumber() const;
    bool isBoolean() const;
    bool isNull() const;

    Json& operator[](const std::string& key);
    bool has(const std::string& key) const;
    void remove(const std::string& key);

    Json& operator[](size_t index);
    size_t size() const;
    void push_back(const Json& value);
    void pop_back();

    double asNumber() const;
    const std::string& asString() const;
    bool asBool() const;

    std::vector<std::string> keys() const;

private:
    Type m_type;
    std::string m_string_value;
    double m_number_value;
    bool m_bool_value;
    std::vector<Json>* m_array_value;
    std::map<std::string, Json>* m_object_value;

    std::string serialize() const;
    void parseValue(const std::string& jsonStr, size_t& pos);
    void parseString(const std::string& jsonStr, size_t& pos);
    void parseNumber(const std::string& jsonStr, size_t& pos);
    void parseObject(const std::string& jsonStr, size_t& pos);
    void parseArray(const std::string& jsonStr, size_t& pos);
    void parseTrue(const std::string& jsonStr, size_t& pos);
    void parseFalse(const std::string& jsonStr, size_t& pos);
    void parseNull(const std::string& jsonStr, size_t& pos);
    void skipWhitespace(const std::string& jsonStr, size_t& pos);
};

} // namespace io
} // namespace base

#endif // JSON_H