# JSON模块 - 详细设计文档

## 1. 模块概述

### 1.1 模块名称
`base::io::Json`

### 1.2 功能描述
实现JSON数据的解析和序列化功能，支持JSON对象、数组、字符串、数字、布尔值和null值的处理。

### 1.3 依赖项
- C++11 标准库
- `<string>` - 字符串处理
- `<vector>` - 数组存储
- `<map>` - 对象存储

## 2. 接口定义

### 2.1 类型枚举
```cpp
enum class Type {
    NUL,    // null值
    NUMBER, // 数字
    STRING, // 字符串
    BOOL,   // 布尔值
    ARRAY,  // 数组
    OBJECT  // 对象
};
```

### 2.2 构造函数
```cpp
Json();                      // 默认构造函数，创建null值
Json(Type type);             // 根据类型创建
Json(int value);             // 从int创建数字
Json(double value);          // 从double创建数字
Json(const std::string& value); // 从字符串创建
Json(bool value);            // 从bool创建布尔值
Json(std::nullptr_t);        // 创建null值
```

### 2.3 静态方法
```cpp
static Json parse(const std::string& jsonStr); // 解析JSON字符串
```

### 2.4 类型查询
```cpp
Type type() const;           // 获取类型
bool isObject() const;       // 是否为对象
bool isArray() const;        // 是否为数组
bool isString() const;       // 是否为字符串
bool isNumber() const;       // 是否为数字
bool isBoolean() const;      // 是否为布尔值
bool isNull() const;         // 是否为null
```

### 2.5 对象操作
```cpp
Json& operator[](const std::string& key); // 获取/设置对象属性
bool has(const std::string& key) const;   // 检查属性是否存在
void remove(const std::string& key);      // 删除属性
std::vector<std::string> keys() const;    // 获取所有键
```

### 2.6 数组操作
```cpp
Json& operator[](size_t index); // 获取/设置数组元素
size_t size() const;            // 获取数组大小
void push_back(const Json& value); // 添加元素
void pop_back();                // 删除最后一个元素
```

### 2.7 值获取
```cpp
double asNumber() const;              // 获取数字值
const std::string& asString() const;  // 获取字符串值
bool asBool() const;                  // 获取布尔值
```

### 2.8 序列化
```cpp
std::string toString() const; // 序列化为JSON字符串
```

## 3. 实现细节

### 3.1 内部存储
```cpp
Type m_type;                              // 类型
std::string m_string_value;                // 字符串值
double m_number_value;                     // 数字值
bool m_bool_value;                         // 布尔值
std::vector<Json>* m_array_value;          // 数组值
std::map<std::string, Json>* m_object_value; // 对象值
```

### 3.2 解析器实现
- 使用递归下降解析器
- 支持嵌套对象和数组
- 支持转义字符处理
- 支持科学计数法数字

### 3.3 序列化实现
- 对象序列化为 `{"key":"value"}` 格式
- 数组序列化为 `["a","b","c"]` 格式
- 字符串自动添加引号
- 数字直接转换

## 4. 使用示例

### 4.1 解析JSON
```cpp
#include "io/json.h"
using namespace base::io;

void parseExample() {
    std::string jsonStr = "{\"name\":\"test\",\"value\":123}";
    Json json = Json::parse(jsonStr);

    std::string name = json["name"].asString(); // "test"
    double value = json["value"].asNumber();    // 123
}
```

### 4.2 创建JSON
```cpp
void createExample() {
    Json obj;
    obj["name"] = "example";
    obj["count"] = 42;
    obj["active"] = true;

    std::string jsonStr = obj.toString();
}
```

### 4.3 数组操作
```cpp
void arrayExample() {
    Json arr = Json::parse("[1,2,3,4,5]");

    for (size_t i = 0; i < arr.size(); ++i) {
        std::cout << arr[i].asNumber() << std::endl;
    }

    arr.push_back(Json(6));
}
```

### 4.4 嵌套结构
```cpp
void nestedExample() {
    std::string jsonStr = "{\"outer\":{\"inner\":{\"value\":100}}}";
    Json nested = Json::parse(jsonStr);

    double value = nested["outer"]["inner"]["value"].asNumber(); // 100
}
```

## 5. 测试结果

### 5.1 功能测试
- ✅ JSON解析测试
- ✅ JSON数组测试
- ✅ JSON序列化测试
- ✅ JSON嵌套测试
- ✅ JSON类型测试

### 5.2 测试输出
```
Testing JSON parsing...
JSON parsing tests passed!
Testing JSON array...
JSON array tests passed!
Testing JSON serialization...
JSON serialization tests passed!
Testing JSON nested...
JSON nested tests passed!
Testing JSON types...
JSON types tests passed!

All JSON module tests passed!
```

## 6. 交付物清单

- [x] `include/io/json.h` - 头文件
- [x] `src/io/json.cpp` - 实现文件
- [x] `tests/test_io.cpp` - 测试代码
- [x] 测试通过验证

## 7. 后续工作

- [ ] 添加XML模块
- [ ] 添加文件系统模块
- [ ] 完善错误处理
- [ ] 添加更多边界条件测试