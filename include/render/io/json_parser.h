// Minimal JSON-like parser used for configuration files

#pragma once

#include "../core/vector.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace render {

/**
 * @class ConfigValue
 * @brief Simple variant-like class for configuration values.
 */
class ConfigValue {
public:
    enum class Type { Null, Bool, Int, Float, String, Array, Object };

private:
    Type type_ = Type::Null;
    bool boolVal_ = false;
    int intVal_ = 0;
    float floatVal_ = 0.0f;
    std::string stringVal_;
    std::vector<ConfigValue> arrayVal_;
    std::unordered_map<std::string, ConfigValue> objectVal_;

public:
    ConfigValue() = default;
    ConfigValue(bool v) : type_(Type::Bool), boolVal_(v) {}
    ConfigValue(int v) : type_(Type::Int), intVal_(v) {}
    ConfigValue(float v) : type_(Type::Float), floatVal_(v) {}
    ConfigValue(double v) : type_(Type::Float), floatVal_(static_cast<float>(v)) {}
    ConfigValue(const char* v) : type_(Type::String), stringVal_(v) {}
    ConfigValue(const std::string& v) : type_(Type::String), stringVal_(v) {}
    
    Type type() const { return type_; }
    bool isNull() const { return type_ == Type::Null; }
    bool isBool() const { return type_ == Type::Bool; }
    bool isInt() const { return type_ == Type::Int; }
    bool isFloat() const { return type_ == Type::Float || type_ == Type::Int; }
    bool isString() const { return type_ == Type::String; }
    bool isArray() const { return type_ == Type::Array; }
    bool isObject() const { return type_ == Type::Object; }
    
    bool asBool(bool defaultVal = false) const;
    
    int asInt(int defaultVal = 0) const;
    
    float asFloat(float defaultVal = 0.0f) const;
    
    const std::string& asString(const std::string& defaultVal = "") const;
    
    const std::vector<ConfigValue>& asArray() const;
    
    Vec3 asVec3(const Vec3& defaultVal = Vec3(0.0f)) const;
    
    bool contains(const std::string& key) const;
    
    const ConfigValue& operator[](const std::string& key) const;
    
    const ConfigValue& operator[](size_t index) const;
    
    // For building arrays and objects
    void setArray() { type_ = Type::Array; }
    void setObject() { type_ = Type::Object; }
    void push(const ConfigValue& v) { arrayVal_.push_back(v); }
    void set(const std::string& key, const ConfigValue& v) { objectVal_[key] = v; }
};

/**
 * @class ConfigParser
 * @brief Simple JSON-like configuration parser.
 */
class ConfigParser {
public:
    static ConfigValue parse(const std::string& content);
    
    static ConfigValue parseFile(const std::string& path);

private:
    static void skipWhitespace(const std::string& s, size_t& pos);
    
    static ConfigValue parseValue(const std::string& s, size_t& pos);
    
    static ConfigValue parseObject(const std::string& s, size_t& pos);
    
    static ConfigValue parseArray(const std::string& s, size_t& pos);
    
    static ConfigValue parseString(const std::string& s, size_t& pos);
    
    static ConfigValue parseNumber(const std::string& s, size_t& pos);
    
    static ConfigValue parseBool(const std::string& s, size_t& pos);
};

} // namespace render
