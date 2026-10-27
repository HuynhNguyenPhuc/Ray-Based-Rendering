#include "render/io/json_parser.h"
#include <cctype>
#include <fstream>
#include <sstream>

namespace render {

bool ConfigValue::asBool(bool defaultVal) const {
    return isBool() ? boolVal_ : defaultVal;
}

int ConfigValue::asInt(int defaultVal) const {
    return isInt() ? intVal_ : defaultVal;
}

float ConfigValue::asFloat(float defaultVal) const {
    if (type_ == Type::Float) return floatVal_;
    if (type_ == Type::Int) return static_cast<float>(intVal_);
    return defaultVal;
}

const std::string& ConfigValue::asString(const std::string& defaultVal) const {
    return isString() ? stringVal_ : defaultVal;
}

const std::vector<ConfigValue>& ConfigValue::asArray() const {
    static std::vector<ConfigValue> empty;
    return isArray() ? arrayVal_ : empty;
}

Vec3 ConfigValue::asVec3(const Vec3& defaultVal) const {
    if (isArray() && arrayVal_.size() >= 3) {
        return Vec3(arrayVal_[0].asFloat(), arrayVal_[1].asFloat(), arrayVal_[2].asFloat());
    }
    return defaultVal;
}

bool ConfigValue::contains(const std::string& key) const {
    return isObject() && objectVal_.count(key) > 0;
}

const ConfigValue& ConfigValue::operator[](const std::string& key) const {
    static ConfigValue nullValue;
    if (isObject()) {
        auto it = objectVal_.find(key);
        if (it != objectVal_.end()) return it->second;
    }
    return nullValue;
}

const ConfigValue& ConfigValue::operator[](size_t index) const {
    static ConfigValue nullValue;
    if (isArray() && index < arrayVal_.size()) {
        return arrayVal_[index];
    }
    return nullValue;
}

ConfigValue ConfigParser::parse(const std::string& content) {
    size_t pos = 0;
    return parseValue(content, pos);
}

ConfigValue ConfigParser::parseFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return ConfigValue();
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return parse(buffer.str());
}

void ConfigParser::skipWhitespace(const std::string& s, size_t& pos) {
    while (pos < s.size() && std::isspace(s[pos])) ++pos;
    // Skip comments
    if (pos < s.size() - 1 && s[pos] == '/' && s[pos + 1] == '/') {
        while (pos < s.size() && s[pos] != '\n') ++pos;
        skipWhitespace(s, pos);
    }
}

ConfigValue ConfigParser::parseValue(const std::string& s, size_t& pos) {
    skipWhitespace(s, pos);
    
    if (pos >= s.size()) return ConfigValue();
    
    char c = s[pos];
    
    if (c == '{') return parseObject(s, pos);
    if (c == '[') return parseArray(s, pos);
    if (c == '"') return parseString(s, pos);
    if (c == 't' || c == 'f') return parseBool(s, pos);
    if (c == 'n') { pos += 4; return ConfigValue(); } // null
    if (c == '-' || std::isdigit(c)) return parseNumber(s, pos);
    
    return ConfigValue();
}

ConfigValue ConfigParser::parseObject(const std::string& s, size_t& pos) {
    ConfigValue obj;
    obj.setObject();
    ++pos; // skip '{'
    
    skipWhitespace(s, pos);
    if (s[pos] == '}') { ++pos; return obj; }
    
    while (true) {
        skipWhitespace(s, pos);
        auto key = parseString(s, pos);
        
        skipWhitespace(s, pos);
        ++pos; // skip ':'
        
        auto value = parseValue(s, pos);
        obj.set(key.asString(), value);
        
        skipWhitespace(s, pos);
        if (s[pos] == '}') { ++pos; break; }
        ++pos; // skip ','
    }
    
    return obj;
}

ConfigValue ConfigParser::parseArray(const std::string& s, size_t& pos) {
    ConfigValue arr;
    arr.setArray();
    ++pos; // skip '['
    
    skipWhitespace(s, pos);
    if (s[pos] == ']') { ++pos; return arr; }
    
    while (true) {
        arr.push(parseValue(s, pos));
        
        skipWhitespace(s, pos);
        if (s[pos] == ']') { ++pos; break; }
        ++pos; // skip ','
    }
    
    return arr;
}

ConfigValue ConfigParser::parseString(const std::string& s, size_t& pos) {
    ++pos; // skip opening '"'
    std::string result;
    
    while (pos < s.size() && s[pos] != '"') {
        if (s[pos] == '\\' && pos + 1 < s.size()) {
            ++pos;
            switch (s[pos]) {
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case 'r': result += '\r'; break;
                case '\\': result += '\\'; break;
                case '"': result += '"'; break;
                default: result += s[pos]; break;
            }
        } else {
            result += s[pos];
        }
        ++pos;
    }
    ++pos; // skip closing '"'
    
    return ConfigValue(result);
}

ConfigValue ConfigParser::parseNumber(const std::string& s, size_t& pos) {
    size_t start = pos;
    bool isFloat = false;
    
    if (s[pos] == '-') ++pos;
    while (pos < s.size() && std::isdigit(s[pos])) ++pos;
    
    if (pos < s.size() && s[pos] == '.') {
        isFloat = true;
        ++pos;
        while (pos < s.size() && std::isdigit(s[pos])) ++pos;
    }
    
    if (pos < s.size() && (s[pos] == 'e' || s[pos] == 'E')) {
        isFloat = true;
        ++pos;
        if (s[pos] == '+' || s[pos] == '-') ++pos;
        while (pos < s.size() && std::isdigit(s[pos])) ++pos;
    }
    
    std::string numStr = s.substr(start, pos - start);
    
    if (isFloat) {
        return ConfigValue(std::stof(numStr));
    } else {
        return ConfigValue(std::stoi(numStr));
    }
}

ConfigValue ConfigParser::parseBool(const std::string& s, size_t& pos) {
    if (s.substr(pos, 4) == "true") {
        pos += 4;
        return ConfigValue(true);
    } else if (s.substr(pos, 5) == "false") {
        pos += 5;
        return ConfigValue(false);
    }
    return ConfigValue();
}

} // namespace render
