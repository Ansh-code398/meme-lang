#ifndef MEMELANG_JSONRPC_H
#define MEMELANG_JSONRPC_H

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <cstdint>
#include <algorithm>
#include <cctype>

namespace memelang::lsp {

enum class JsonType {
    Null,
    Bool,
    Number,
    String,
    Array,
    Object
};

struct JsonValue {
    JsonType type = JsonType::Null;
    bool boolVal = false;
    double numVal = 0.0;
    int64_t intVal = 0;
    std::string strVal = "";
    std::vector<JsonValue> arrVal;
    std::unordered_map<std::string, JsonValue> objVal;

    JsonValue() : type(JsonType::Null) {}
    JsonValue(bool b) : type(JsonType::Bool), boolVal(b) {}
    JsonValue(int n) : type(JsonType::Number), numVal(n), intVal(n) {}
    JsonValue(int64_t n) : type(JsonType::Number), numVal(static_cast<double>(n)), intVal(n) {}
    JsonValue(double d) : type(JsonType::Number), numVal(d), intVal(static_cast<int64_t>(d)) {}
    JsonValue(const char *s) : type(JsonType::String), strVal(s ? s : "") {}
    JsonValue(const std::string &s) : type(JsonType::String), strVal(s) {}

    bool isNull() const { return type == JsonType::Null; }
    bool isBool() const { return type == JsonType::Bool; }
    bool isNumber() const { return type == JsonType::Number; }
    bool isString() const { return type == JsonType::String; }
    bool isArray() const { return type == JsonType::Array; }
    bool isObject() const { return type == JsonType::Object; }

    bool asBool(bool def = false) const { return isBool() ? boolVal : def; }
    int64_t asInt(int64_t def = 0) const { return isNumber() ? intVal : def; }
    double asDouble(double def = 0.0) const { return isNumber() ? numVal : def; }
    std::string asString(const std::string &def = "") const { return isString() ? strVal : def; }

    void push(JsonValue v) {
        if (type != JsonType::Array) {
            type = JsonType::Array;
            arrVal.clear();
        }
        arrVal.push_back(std::move(v));
    }

    bool contains(const std::string &key) const {
        return isObject() && (objVal.find(key) != objVal.end());
    }

    const JsonValue &operator[](const std::string &key) const {
        static const JsonValue nullVal;
        if (!isObject()) return nullVal;
        auto it = objVal.find(key);
        if (it != objVal.end()) return it->second;
        return nullVal;
    }

    JsonValue &operator[](const std::string &key) {
        if (!isObject()) {
            type = JsonType::Object;
            objVal.clear();
        }
        return objVal[key];
    }

    const JsonValue &operator[](size_t idx) const {
        static const JsonValue nullVal;
        if (!isArray() || idx >= arrVal.size()) return nullVal;
        return arrVal[idx];
    }

    static std::string escapeString(const std::string &s) {
        std::string out = "\"";
        for (char c : s) {
            switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += c;
                }
                break;
            }
        }
        out += "\"";
        return out;
    }

    std::string stringify() const {
        switch (type) {
        case JsonType::Null: return "null";
        case JsonType::Bool: return boolVal ? "true" : "false";
        case JsonType::Number: {
            if (numVal == static_cast<double>(intVal)) {
                return std::to_string(intVal);
            }
            std::ostringstream ss;
            ss << numVal;
            return ss.str();
        }
        case JsonType::String: return escapeString(strVal);
        case JsonType::Array: {
            std::string out = "[";
            for (size_t i = 0; i < arrVal.size(); ++i) {
                if (i > 0) out += ",";
                out += arrVal[i].stringify();
            }
            out += "]";
            return out;
        }
        case JsonType::Object: {
            std::string out = "{";
            bool first = true;
            for (const auto &kv : objVal) {
                if (!first) out += ",";
                first = false;
                out += escapeString(kv.first) + ":" + kv.second.stringify();
            }
            out += "}";
            return out;
        }
        }
        return "null";
    }

    static JsonValue parse(const std::string &src) {
        size_t pos = 0;
        return parseInternal(src, pos);
    }

private:
    static void skipWhitespace(const std::string &src, size_t &pos) {
        while (pos < src.size() && (src[pos] == ' ' || src[pos] == '\t' || src[pos] == '\r' || src[pos] == '\n')) {
            pos++;
        }
    }

    static JsonValue parseInternal(const std::string &src, size_t &pos) {
        skipWhitespace(src, pos);
        if (pos >= src.size()) return JsonValue();

        char c = src[pos];
        if (c == 'n') {
            if (pos + 4 <= src.size() && src.substr(pos, 4) == "null") { pos += 4; return JsonValue(); }
        }
        if (c == 't') {
            if (pos + 4 <= src.size() && src.substr(pos, 4) == "true") { pos += 4; return JsonValue(true); }
        }
        if (c == 'f') {
            if (pos + 5 <= src.size() && src.substr(pos, 5) == "false") { pos += 5; return JsonValue(false); }
        }
        if (c == '"') {
            return parseString(src, pos);
        }
        if (c == '[') {
            return parseArray(src, pos);
        }
        if (c == '{') {
            return parseObject(src, pos);
        }
        if (c == '-' || (c >= '0' && c <= '9')) {
            return parseNumber(src, pos);
        }
        return JsonValue();
    }

    static JsonValue parseString(const std::string &src, size_t &pos) {
        pos++; // consume '"'
        std::string s;
        while (pos < src.size()) {
            char c = src[pos++];
            if (c == '"') {
                return JsonValue(s);
            }
            if (c == '\\' && pos < src.size()) {
                char esc = src[pos++];
                switch (esc) {
                case '"': s += '"'; break;
                case '\\': s += '\\'; break;
                case '/': s += '/'; break;
                case 'b': s += '\b'; break;
                case 'f': s += '\f'; break;
                case 'n': s += '\n'; break;
                case 'r': s += '\r'; break;
                case 't': s += '\t'; break;
                case 'u':
                    if (pos + 4 <= src.size()) {
                        std::string hex = src.substr(pos, 4);
                        pos += 4;
                        try {
                            int code = std::stoi(hex, nullptr, 16);
                            if (code < 128) s += static_cast<char>(code);
                            else s += '?';
                        } catch (...) { s += '?'; }
                    }
                    break;
                default: s += esc; break;
                }
            } else {
                s += c;
            }
        }
        return JsonValue(s);
    }

    static JsonValue parseNumber(const std::string &src, size_t &pos) {
        size_t start = pos;
        if (src[pos] == '-') pos++;
        while (pos < src.size() && ((src[pos] >= '0' && src[pos] <= '9') || src[pos] == '.' || src[pos] == 'e' || src[pos] == 'E' || src[pos] == '+' || src[pos] == '-')) {
            pos++;
        }
        std::string numStr = src.substr(start, pos - start);
        bool isFloat = (numStr.find('.') != std::string::npos || numStr.find('e') != std::string::npos || numStr.find('E') != std::string::npos);
        try {
            if (isFloat) {
                return JsonValue(std::stod(numStr));
            } else {
                return JsonValue(static_cast<int64_t>(std::stoll(numStr)));
            }
        } catch (...) {
            return JsonValue(0);
        }
    }

    static JsonValue parseArray(const std::string &src, size_t &pos) {
        pos++; // consume '['
        JsonValue arr;
        arr.type = JsonType::Array;
        skipWhitespace(src, pos);
        if (pos < src.size() && src[pos] == ']') {
            pos++;
            return arr;
        }

        while (pos < src.size()) {
            JsonValue elem = parseInternal(src, pos);
            arr.arrVal.push_back(std::move(elem));
            skipWhitespace(src, pos);
            if (pos < src.size() && src[pos] == ',') {
                pos++;
                continue;
            }
            if (pos < src.size() && src[pos] == ']') {
                pos++;
                break;
            }
            pos++;
        }
        return arr;
    }

    static JsonValue parseObject(const std::string &src, size_t &pos) {
        pos++; // consume '{'
        JsonValue obj;
        obj.type = JsonType::Object;
        skipWhitespace(src, pos);
        if (pos < src.size() && src[pos] == '}') {
            pos++;
            return obj;
        }

        while (pos < src.size()) {
            skipWhitespace(src, pos);
            if (pos >= src.size() || src[pos] != '"') break;
            JsonValue keyVal = parseString(src, pos);
            std::string key = keyVal.asString();

            skipWhitespace(src, pos);
            if (pos < src.size() && src[pos] == ':') {
                pos++; // consume ':'
            }

            JsonValue val = parseInternal(src, pos);
            obj.objVal[key] = std::move(val);

            skipWhitespace(src, pos);
            if (pos < src.size() && src[pos] == ',') {
                pos++;
                continue;
            }
            if (pos < src.size() && src[pos] == '}') {
                pos++;
                break;
            }
            pos++;
        }
        return obj;
    }
};

class JsonRpcTransport {
public:
    static bool readMessage(std::istream &in, std::string &outPayload) {
        size_t contentLength = 0;
        std::string line;

        while (std::getline(in, line)) {
            // Trim carriage return
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (line.empty()) {
                // Empty line signals end of headers
                break;
            }
            const std::string prefix = "Content-Length: ";
            if (line.rfind(prefix, 0) == 0) {
                try {
                    contentLength = std::stoul(line.substr(prefix.size()));
                } catch (...) {
                    contentLength = 0;
                }
            }
        }

        if (contentLength == 0 || in.eof()) {
            return false;
        }

        outPayload.resize(contentLength);
        in.read(&outPayload[0], contentLength);
        return in.gcount() == static_cast<std::streamsize>(contentLength);
    }

    static void writeMessage(std::ostream &out, const std::string &payload) {
        out << "Content-Length: " << payload.size() << "\r\n\r\n" << payload << std::flush;
    }
};

} // namespace memelang::lsp

#endif // MEMELANG_JSONRPC_H
