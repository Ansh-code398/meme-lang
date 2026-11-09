#ifndef MEME_RUNTIME_H
#define MEME_RUNTIME_H

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <memory>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <functional>
#include <sstream>
#include <iomanip>
#include <variant>
#include <algorithm>
#include <stdexcept>

// ==========================================
// 1. Dynamic Polymorphic MemeValue
// ==========================================

class MemeValue {
public:
    enum Type : uint8_t {
        TYPE_NALLA = 0,
        TYPE_INT = 1,
        TYPE_FLOAT = 2,
        TYPE_BOOL = 3,
        TYPE_STRING = 4,
        TYPE_LIST = 5,
        TYPE_POINTER = 6,
        TYPE_OBJECT = 7
    };

    Type type = TYPE_NALLA;
    int64_t i_val = 0;
    double f_val = 0.0;
    bool b_val = false;
    std::string s_val = "";
    std::vector<MemeValue> list_val = {};
    void *ptr_val = nullptr;
    std::shared_ptr<void> obj_val = nullptr;

    MemeValue() : type(TYPE_NALLA) {}
    MemeValue(std::nullptr_t) : type(TYPE_NALLA) {}
    MemeValue(int64_t v) : type(TYPE_INT), i_val(v) {}
    MemeValue(int v) : type(TYPE_INT), i_val(v) {}
    MemeValue(double v) : type(TYPE_FLOAT), f_val(v) {}
    MemeValue(float v) : type(TYPE_FLOAT), f_val(static_cast<double>(v)) {}
    MemeValue(bool v) : type(TYPE_BOOL), b_val(v) {}
    MemeValue(const char *s) : type(TYPE_STRING), s_val(s) {}
    MemeValue(const std::string &s) : type(TYPE_STRING), s_val(s) {}
    MemeValue(std::vector<MemeValue> l) : type(TYPE_LIST), list_val(std::move(l)) {}
    MemeValue(void *p) : type(TYPE_POINTER), ptr_val(p) {}
    MemeValue(std::shared_ptr<void> obj) : type(TYPE_OBJECT), obj_val(std::move(obj)) {}

    // Truthiness
    bool is_truthy() const {
        switch (type) {
        case TYPE_NALLA: return false;
        case TYPE_INT: return i_val != 0;
        case TYPE_FLOAT: return f_val != 0.0;
        case TYPE_BOOL: return b_val;
        case TYPE_STRING: return !s_val.empty();
        case TYPE_LIST: return !list_val.empty();
        case TYPE_POINTER: return ptr_val != nullptr;
        case TYPE_OBJECT: return obj_val != nullptr;
        }
        return false;
    }

    operator bool() const {
        return is_truthy();
    }

    operator int64_t() const {
        return as_int();
    }

    operator double() const {
        return as_double();
    }

    // Number conversions
    int64_t as_int() const {
        switch (type) {
        case TYPE_INT: return i_val;
        case TYPE_FLOAT: return static_cast<int64_t>(f_val);
        case TYPE_BOOL: return b_val ? 1 : 0;
        case TYPE_STRING: {
            try { return std::stoll(s_val); } catch (...) { return 0; }
        }
        default: return 0;
        }
    }

    double as_double() const {
        switch (type) {
        case TYPE_FLOAT: return f_val;
        case TYPE_INT: return static_cast<double>(i_val);
        case TYPE_BOOL: return b_val ? 1.0 : 0.0;
        case TYPE_STRING: {
            try { return std::stod(s_val); } catch (...) { return 0.0; }
        }
        default: return 0.0;
        }
    }

    std::string to_string() const {
        switch (type) {
        case TYPE_NALLA: return "nalla";
        case TYPE_INT: return std::to_string(i_val);
        case TYPE_FLOAT: {
            std::ostringstream ss;
            ss << f_val;
            return ss.str();
        }
        case TYPE_BOOL: return b_val ? "han" : "nhi";
        case TYPE_STRING: return s_val;
        case TYPE_LIST: {
            std::string res = "[";
            for (size_t idx = 0; idx < list_val.size(); ++idx) {
                if (idx > 0) res += ", ";
                res += list_val[idx].to_string();
            }
            res += "]";
            return res;
        }
        case TYPE_POINTER: return "<patta " + std::to_string(reinterpret_cast<uintptr_t>(ptr_val)) + ">";
        case TYPE_OBJECT: return "<object>";
        }
        return "nalla";
    }

    // Arithmetic operators
    MemeValue operator+(const MemeValue &other) const {
        if (type == TYPE_INT && other.type == TYPE_INT) {
            return MemeValue(i_val + other.i_val);
        }
        if (type == TYPE_STRING || other.type == TYPE_STRING) {
            return MemeValue(to_string() + other.to_string());
        }
        if ((type == TYPE_INT || type == TYPE_FLOAT) && (other.type == TYPE_INT || other.type == TYPE_FLOAT)) {
            return MemeValue(as_double() + other.as_double());
        }
        if (type == TYPE_LIST && other.type == TYPE_LIST) {
            auto combined = list_val;
            combined.insert(combined.end(), other.list_val.begin(), other.list_val.end());
            return MemeValue(combined);
        }
        return MemeValue();
    }

    MemeValue operator+(int64_t v) const {
        if (type == TYPE_INT) return MemeValue(i_val + v);
        return MemeValue(as_double() + static_cast<double>(v));
    }

    MemeValue operator+(double v) const {
        return MemeValue(as_double() + v);
    }

    MemeValue operator-(const MemeValue &other) const {
        if (type == TYPE_INT && other.type == TYPE_INT) {
            return MemeValue(i_val - other.i_val);
        }
        return MemeValue(as_double() - other.as_double());
    }

    MemeValue operator-(int64_t v) const {
        if (type == TYPE_INT) return MemeValue(i_val - v);
        return MemeValue(as_double() - static_cast<double>(v));
    }

    MemeValue operator-(double v) const {
        return MemeValue(as_double() - v);
    }

    MemeValue operator*(const MemeValue &other) const {
        if (type == TYPE_INT && other.type == TYPE_INT) {
            return MemeValue(i_val * other.i_val);
        }
        return MemeValue(as_double() * other.as_double());
    }

    MemeValue operator*(int64_t v) const {
        if (type == TYPE_INT) return MemeValue(i_val * v);
        return MemeValue(as_double() * static_cast<double>(v));
    }

    MemeValue operator*(double v) const {
        return MemeValue(as_double() * v);
    }

    MemeValue operator/(const MemeValue &other) const {
        if (type == TYPE_INT && other.type == TYPE_INT) {
            if (other.i_val == 0) throw std::runtime_error("Zero division error!");
            return MemeValue(i_val / other.i_val);
        }
        double div = other.as_double();
        if (div == 0.0) throw std::runtime_error("Zero division error!");
        return MemeValue(as_double() / div);
    }

    MemeValue operator/(int64_t v) const {
        if (v == 0) throw std::runtime_error("Zero division error!");
        if (type == TYPE_INT) return MemeValue(i_val / v);
        return MemeValue(as_double() / static_cast<double>(v));
    }

    MemeValue operator/(double v) const {
        if (v == 0.0) throw std::runtime_error("Zero division error!");
        return MemeValue(as_double() / v);
    }

    MemeValue operator%(const MemeValue &other) const {
        int64_t m = other.as_int();
        if (m == 0) throw std::runtime_error("Zero modulo error!");
        return MemeValue(as_int() % m);
    }

    MemeValue operator%(int64_t m) const {
        if (m == 0) throw std::runtime_error("Zero modulo error!");
        return MemeValue(as_int() % m);
    }

    // Comparisons
    bool operator==(std::nullptr_t) const { return type == TYPE_NALLA; }
    bool operator!=(std::nullptr_t) const { return type != TYPE_NALLA; }
    bool operator==(int64_t v) const { return type == TYPE_INT ? i_val == v : as_double() == static_cast<double>(v); }
    bool operator!=(int64_t v) const { return !(*this == v); }
    bool operator==(double v) const { return as_double() == v; }
    bool operator!=(double v) const { return !(*this == v); }

    bool operator==(const MemeValue &other) const {
        if (type == TYPE_INT && other.type == TYPE_INT) return i_val == other.i_val;
        if (type == TYPE_FLOAT && other.type == TYPE_FLOAT) return f_val == other.f_val;
        if (type == TYPE_BOOL && other.type == TYPE_BOOL) return b_val == other.b_val;
        if (type == TYPE_STRING && other.type == TYPE_STRING) return s_val == other.s_val;
        if (type == TYPE_NALLA && other.type == TYPE_NALLA) return true;
        if ((type == TYPE_INT || type == TYPE_FLOAT) && (other.type == TYPE_INT || other.type == TYPE_FLOAT)) {
            return as_double() == other.as_double();
        }
        return false;
    }

    bool operator!=(const MemeValue &other) const { return !(*this == other); }

    bool operator<(const MemeValue &other) const {
        if (type == TYPE_INT && other.type == TYPE_INT) return i_val < other.i_val;
        return as_double() < other.as_double();
    }

    bool operator<=(const MemeValue &other) const {
        if (type == TYPE_INT && other.type == TYPE_INT) return i_val <= other.i_val;
        return as_double() <= other.as_double();
    }

    bool operator>(const MemeValue &other) const {
        if (type == TYPE_INT && other.type == TYPE_INT) return i_val > other.i_val;
        return as_double() > other.as_double();
    }

    bool operator>=(const MemeValue &other) const {
        if (type == TYPE_INT && other.type == TYPE_INT) return i_val >= other.i_val;
        return as_double() >= other.as_double();
    }

    // Indexing
    MemeValue &operator[](size_t index) {
        if (type != TYPE_LIST) {
            type = TYPE_LIST;
        }
        if (index >= list_val.size()) {
            list_val.resize(index + 1);
        }
        return list_val[index];
    }

    const MemeValue &operator[](size_t index) const {
        if (type != TYPE_LIST || index >= list_val.size()) {
            static const MemeValue nilVal;
            return nilVal;
        }
        return list_val[index];
    }

    MemeValue &operator[](const MemeValue &index) {
        return (*this)[static_cast<size_t>(index.as_int())];
    }

    const MemeValue &operator[](const MemeValue &index) const {
        return (*this)[static_cast<size_t>(index.as_int())];
    }

    // Methods
    void push(const MemeValue &val) {
        if (type != TYPE_LIST) {
            type = TYPE_LIST;
        }
        list_val.push_back(val);
    }

    void push_back(const MemeValue &val) {
        push(val);
    }

    size_t size() const {
        if (type == TYPE_LIST) return list_val.size();
        if (type == TYPE_STRING) return s_val.size();
        return 0;
    }

    size_t length() const {
        return size();
    }
};

inline std::ostream &operator<<(std::ostream &os, const MemeValue &v) {
    os << v.to_string();
    return os;
}

inline MemeValue operator+(int64_t a, const MemeValue &b) { return MemeValue(a) + b; }
inline MemeValue operator+(double a, const MemeValue &b) { return MemeValue(a) + b; }
inline MemeValue operator-(int64_t a, const MemeValue &b) { return MemeValue(a) - b; }
inline MemeValue operator-(double a, const MemeValue &b) { return MemeValue(a) - b; }
inline MemeValue operator*(int64_t a, const MemeValue &b) { return MemeValue(a) * b; }
inline MemeValue operator*(double a, const MemeValue &b) { return MemeValue(a) * b; }
inline MemeValue operator/(int64_t a, const MemeValue &b) { return MemeValue(a) / b; }
inline MemeValue operator/(double a, const MemeValue &b) { return MemeValue(a) / b; }

// Global truthiness helper
template <typename T>
inline bool is_truthy(const T &val) {
    if constexpr (std::is_same_v<T, MemeValue>) {
        return val.is_truthy();
    } else if constexpr (std::is_same_v<T, bool>) {
        return val;
    } else if constexpr (std::is_pointer_v<T>) {
        return val != nullptr;
    } else if constexpr (std::is_arithmetic_v<T>) {
        return val != 0;
    } else {
        return static_cast<bool>(val);
    }
}

// ==========================================
// 2. Standard Modules & Builtins
// ==========================================

struct SamayModule {
    double ab() {
        return std::chrono::duration<double>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }
    double now() { return ab(); }
};

struct ItertoolsModule {
    template <typename N>
    auto repeat(bool val, N count) {
        std::vector<uint8_t> vec(static_cast<size_t>(count), val ? 1 : 0);
        return vec;
    }

    template <typename T, typename N>
    auto repeat(const T &val, N count) {
        std::vector<T> vec;
        vec.reserve(static_cast<size_t>(count));
        for (N i = 0; i < count; ++i) {
            vec.push_back(val);
        }
        return vec;
    }
};

struct MathModule {
    double pi = 3.14159265358979323846;
    double e = 2.71828182845904523536;

    template <typename T> double sqrt(T x) { return std::sqrt(static_cast<double>(x)); }
    template <typename T, typename U> double pow(T b, U exp) { return std::pow(static_cast<double>(b), static_cast<double>(exp)); }
    template <typename T> double sin(T x) { return std::sin(static_cast<double>(x)); }
    template <typename T> double cos(T x) { return std::cos(static_cast<double>(x)); }
    template <typename T> double tan(T x) { return std::tan(static_cast<double>(x)); }
    template <typename T> auto abs(T x) { return std::abs(x); }
    template <typename T> double floor(T x) { return std::floor(static_cast<double>(x)); }
    template <typename T> double ceil(T x) { return std::ceil(static_cast<double>(x)); }

    template <typename T> double sqrt_fn(T x) { return sqrt(x); }
    template <typename T, typename U> double pow_fn(T b, U exp) { return pow(b, exp); }
    template <typename T> double sin_fn(T x) { return sin(x); }
    template <typename T> double cos_fn(T x) { return cos(x); }
    template <typename T> double tan_fn(T x) { return tan(x); }
    template <typename T> auto abs_fn(T x) { return abs(x); }
    template <typename T> double floor_fn(T x) { return floor(x); }
    template <typename T> double ceil_fn(T x) { return ceil(x); }
};

inline SamayModule samay;
inline ItertoolsModule itertools;
inline MathModule math;

inline std::string point_wala(double v, int precision = 4) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(precision) << v;
    return ss.str();
}

inline std::string point_wala(int64_t v) {
    return std::to_string(v);
}

inline std::string point_wala(const MemeValue &v, int precision = 4) {
    return point_wala(v.as_double(), precision);
}

inline int64_t bina_point(double v) { return static_cast<int64_t>(v); }
inline int64_t bina_point(int64_t v) { return v; }
inline int64_t bina_point(const std::string &s) {
    try { return std::stoll(s); } catch (...) { return 0; }
}
inline int64_t bina_point(const MemeValue &v) { return v.as_int(); }

template <typename T>
inline auto nakal(const T &x) {
    return x; // C++ copy semantics
}

template <typename T>
inline void khatam(T &&) {
    // Explicit drop
}

// ==========================================
// Rich text formatting and printing support
// ==========================================

inline std::string strip_ansi_runtime(const std::string &str) {
    std::string result;
    size_t i = 0;
    while (i < str.size()) {
        if (str[i] == '\033' && i + 1 < str.size() && str[i + 1] == '[') {
            size_t j = i + 2;
            while (j < str.size() && (std::isdigit(static_cast<unsigned char>(str[j])) || str[j] == ';')) {
                j++;
            }
            if (j < str.size() && str[j] == 'm') {
                i = j + 1;
                continue;
            }
        }
        result += str[i++];
    }
    return result;
}

inline bool rt_parseHexColor(const std::string &str, int &r, int &g, int &b) {
    if (str.empty() || str[0] != '#') return false;
    if (str.size() == 7) {
        for (size_t i = 1; i < 7; ++i) {
            if (!std::isxdigit(static_cast<unsigned char>(str[i]))) return false;
        }
        r = std::stoi(str.substr(1, 2), nullptr, 16);
        g = std::stoi(str.substr(3, 2), nullptr, 16);
        b = std::stoi(str.substr(5, 2), nullptr, 16);
        return true;
    } else if (str.size() == 4) {
        for (size_t i = 1; i < 4; ++i) {
            if (!std::isxdigit(static_cast<unsigned char>(str[i]))) return false;
        }
        r = std::stoi(std::string(2, str[1]), nullptr, 16);
        g = std::stoi(std::string(2, str[2]), nullptr, 16);
        b = std::stoi(std::string(2, str[3]), nullptr, 16);
        return true;
    }
    return false;
}

inline bool rt_parseRgbColor(const std::string &str, int &r, int &g, int &b) {
    if (str.size() < 7) return false;
    if (str.rfind("rgb(", 0) != 0 || str.back() != ')') return false;
    std::string inner = str.substr(4, str.size() - 5);
    std::stringstream ss(inner);
    std::string p1, p2, p3;
    if (!std::getline(ss, p1, ',') || !std::getline(ss, p2, ',') || !std::getline(ss, p3, ',')) return false;
    try {
        r = std::stoi(p1);
        g = std::stoi(p2);
        b = std::stoi(p3);
    } catch (...) {
        return false;
    }
    return (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255);
}

inline bool rt_parse256Color(const std::string &str, int &code) {
    if (str.empty()) return false;
    for (char c : str) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    }
    try {
        code = std::stoi(str);
    } catch (...) {
        return false;
    }
    return (code >= 0 && code <= 255);
}

inline bool rt_isBadgeTag(const std::string &w, std::string &outBadge) {
    if (w == "success" || w == "theek") {
        outBadge = "\033[1;32m[SUCCESS]\033[0m";
        return true;
    }
    if (w == "error" || w == "galat" || w == "gadbad") {
        outBadge = "\033[1;31m[ERROR]\033[0m";
        return true;
    }
    if (w == "warn" || w == "warning" || w == "chetavni") {
        outBadge = "\033[1;33m[WARN]\033[0m";
        return true;
    }
    if (w == "info" || w == "suchna" || w == "jankari") {
        outBadge = "\033[1;36m[INFO]\033[0m";
        return true;
    }
    return false;
}

inline bool rt_resolveRichWord(const std::string &w, std::string &outAnsi, bool allowStatusAliases = true) {
    static const std::unordered_map<std::string, std::string> RICH_TAGS = {
        {"laal", "\033[31m"}, {"red", "\033[31m"},
        {"hara", "\033[32m"}, {"green", "\033[32m"},
        {"peela", "\033[33m"}, {"yellow", "\033[33m"},
        {"neela", "\033[34m"}, {"blue", "\033[34m"},
        {"gulabi", "\033[35m"}, {"pink", "\033[35m"}, {"magenta", "\033[35m"},
        {"asmani", "\033[36m"}, {"cyan", "\033[36m"},
        {"safed", "\033[37m"}, {"white", "\033[37m"},
        {"kala", "\033[30m"}, {"black", "\033[30m"},
        {"gray", "\033[90m"}, {"grey", "\033[90m"}, {"dhundhla", "\033[90m"},
        {"orange", "\033[38;5;208m"}, {"santre", "\033[38;5;208m"},
        {"purple", "\033[38;5;129m"}, {"jamuni", "\033[38;5;129m"},
        {"bright_red", "\033[91m"}, {"bright_laal", "\033[91m"}, {"chamkila_laal", "\033[91m"},
        {"bright_green", "\033[92m"}, {"bright_hara", "\033[92m"}, {"chamkila_hara", "\033[92m"},
        {"bright_yellow", "\033[93m"}, {"bright_peela", "\033[93m"}, {"chamkila_peela", "\033[93m"},
        {"bright_blue", "\033[94m"}, {"bright_neela", "\033[94m"}, {"chamkila_neela", "\033[94m"},
        {"bright_magenta", "\033[95m"}, {"bright_pink", "\033[95m"}, {"chamkila_gulabi", "\033[95m"},
        {"bright_cyan", "\033[96m"}, {"bright_asmani", "\033[96m"}, {"chamkila_asmani", "\033[96m"},
        {"bright_white", "\033[97m"}, {"bright_safed", "\033[97m"}, {"chamkila_safed", "\033[97m"},
        {"mota", "\033[1m"}, {"bold", "\033[1m"},
        {"halka", "\033[2m"}, {"dim", "\033[2m"},
        {"tedha", "\033[3m"}, {"italic", "\033[3m"},
        {"line", "\033[4m"}, {"underline", "\033[4m"},
        {"blink", "\033[5m"}, {"chamak", "\033[5m"},
        {"ulta", "\033[7m"}, {"reverse", "\033[7m"}, {"invert", "\033[7m"},
        {"kata", "\033[9m"}, {"strikethrough", "\033[9m"}, {"strike", "\033[9m"},
        {"/", "\033[0m"}, {"reset", "\033[0m"}, {"saaf", "\033[0m"}
    };

    auto it = RICH_TAGS.find(w);
    if (it != RICH_TAGS.end()) {
        outAnsi = it->second;
        return true;
    }

    if (allowStatusAliases) {
        if (w == "success" || w == "theek") { outAnsi = "\033[1;32m"; return true; }
        if (w == "error" || w == "galat" || w == "gadbad") { outAnsi = "\033[1;31m"; return true; }
        if (w == "warn" || w == "warning" || w == "chetavni") { outAnsi = "\033[1;33m"; return true; }
        if (w == "info" || w == "suchna" || w == "jankari") { outAnsi = "\033[1;36m"; return true; }
    }

    if (w.rfind("#", 0) == 0) {
        int r, g, b;
        if (rt_parseHexColor(w, r, g, b)) {
            outAnsi = "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
            return true;
        }
    } else if (w.rfind("fg:#", 0) == 0) {
        int r, g, b;
        if (rt_parseHexColor(w.substr(3), r, g, b)) {
            outAnsi = "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
            return true;
        }
    } else if (w.rfind("bg:#", 0) == 0) {
        int r, g, b;
        if (rt_parseHexColor(w.substr(3), r, g, b)) {
            outAnsi = "\033[48;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
            return true;
        }
    }

    if (w.rfind("rgb(", 0) == 0) {
        int r, g, b;
        if (rt_parseRgbColor(w, r, g, b)) {
            outAnsi = "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
            return true;
        }
    } else if (w.rfind("fg:rgb(", 0) == 0) {
        int r, g, b;
        if (rt_parseRgbColor(w.substr(3), r, g, b)) {
            outAnsi = "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
            return true;
        }
    } else if (w.rfind("bg:rgb(", 0) == 0) {
        int r, g, b;
        if (rt_parseRgbColor(w.substr(3), r, g, b)) {
            outAnsi = "\033[48;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
            return true;
        }
    }

    if (w.rfind("color:", 0) == 0) {
        int code;
        if (rt_parse256Color(w.substr(6), code)) {
            outAnsi = "\033[38;5;" + std::to_string(code) + "m";
            return true;
        }
    } else if (w.rfind("fg:", 0) == 0) {
        int code;
        if (rt_parse256Color(w.substr(3), code)) {
            outAnsi = "\033[38;5;" + std::to_string(code) + "m";
            return true;
        }
    } else if (w.rfind("c:", 0) == 0) {
        int code;
        if (rt_parse256Color(w.substr(2), code)) {
            outAnsi = "\033[38;5;" + std::to_string(code) + "m";
            return true;
        }
    } else if (w.rfind("bg:", 0) == 0) {
        std::string rest = w.substr(3);
        int code;
        if (rt_parse256Color(rest, code)) {
            outAnsi = "\033[48;5;" + std::to_string(code) + "m";
            return true;
        }
        static const std::unordered_map<std::string, std::string> BG_MAP = {
            {"laal", "\033[41m"}, {"red", "\033[41m"},
            {"hara", "\033[42m"}, {"green", "\033[42m"},
            {"peela", "\033[43m"}, {"yellow", "\033[43m"},
            {"neela", "\033[44m"}, {"blue", "\033[44m"},
            {"gulabi", "\033[45m"}, {"pink", "\033[45m"}, {"magenta", "\033[45m"},
            {"asmani", "\033[46m"}, {"cyan", "\033[46m"},
            {"safed", "\033[47m"}, {"white", "\033[47m"},
            {"kala", "\033[40m"}, {"black", "\033[40m"},
            {"gray", "\033[100m"}, {"grey", "\033[100m"}, {"dhundhla", "\033[100m"},
            {"orange", "\033[48;5;208m"}, {"santre", "\033[48;5;208m"},
            {"purple", "\033[48;5;129m"}, {"jamuni", "\033[48;5;129m"},
            {"bright_red", "\033[101m"}, {"bright_laal", "\033[101m"},
            {"bright_green", "\033[102m"}, {"bright_hara", "\033[102m"},
            {"bright_yellow", "\033[103m"}, {"bright_peela", "\033[103m"},
            {"bright_blue", "\033[104m"}, {"bright_neela", "\033[104m"},
            {"bright_magenta", "\033[105m"}, {"bright_pink", "\033[105m"},
            {"bright_cyan", "\033[106m"}, {"bright_asmani", "\033[106m"},
            {"bright_white", "\033[107m"}, {"bright_safed", "\033[107m"}
        };
        auto bit = BG_MAP.find(rest);
        if (bit != BG_MAP.end()) {
            outAnsi = bit->second;
            return true;
        }
    }
    return false;
}

inline std::string runtime_format_rich_text(const std::string &input) {
    std::string result;
    bool styled = false;
    size_t i = 0;
    while (i < input.size()) {
        if (input[i] == '[') {
            size_t closePos = input.find(']', i + 1);
            if (closePos != std::string::npos) {
                std::string tagContent = input.substr(i + 1, closePos - i - 1);
                std::string normalizedTag;
                bool inParens = false;
                for (char c : tagContent) {
                    if (c == '(') inParens = true;
                    else if (c == ')') inParens = false;
                    if (inParens && std::isspace(static_cast<unsigned char>(c))) continue;
                    normalizedTag += c;
                }
                std::vector<std::string> words;
                std::stringstream ss(normalizedTag);
                std::string word;
                while (ss >> word) {
                    for (char &c : word) c = std::tolower(static_cast<unsigned char>(c));
                    words.push_back(word);
                }

                if (words.size() == 1) {
                    std::string badge;
                    if (rt_isBadgeTag(words[0], badge)) {
                        result += badge;
                        i = closePos + 1;
                        continue;
                    }
                }

                bool allValid = !words.empty();
                std::string combinedAnsi;
                for (const auto &w : words) {
                    std::string wordAnsi;
                    if (!rt_resolveRichWord(w, wordAnsi, false)) {
                        allValid = false;
                        break;
                    }
                    combinedAnsi += wordAnsi;
                }

                if (allValid) {
                    result += combinedAnsi;
                    for (const auto &w : words) {
                        if (w != "/" && w != "reset" && w != "saaf") {
                            styled = true;
                        }
                    }
                    i = closePos + 1;
                    continue;
                }
            }
        }
        result += input[i++];
    }
    if (styled && (result.size() < 4 || result.substr(result.size() - 4) != "\033[0m")) {
        result += "\033[0m";
    }
    return result;
}

inline std::string runtime_resolve_style_ansi(const std::string &rawStyle) {
    std::string s = rawStyle;
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
    if (s.empty()) return "";
    if (s.front() == '[' && s.back() == ']') {
        s = s.substr(1, s.size() - 2);
    }
    std::string normalized;
    bool inParens = false;
    for (char c : s) {
        if (c == '(') inParens = true;
        else if (c == ')') inParens = false;
        if (inParens && std::isspace(static_cast<unsigned char>(c))) continue;
        if (c == ',') { normalized += ' '; continue; }
        normalized += c;
    }
    std::stringstream ss(normalized);
    std::string word;
    std::string combined;
    while (ss >> word) {
        for (char &c : word) c = std::tolower(static_cast<unsigned char>(c));
        std::string ansi;
        if (rt_resolveRichWord(word, ansi, true)) {
            combined += ansi;
        }
    }
    return combined;
}

template <typename T>
inline std::string meme_to_string_fmt(const T &val) {
    if constexpr (std::is_same_v<std::decay_t<T>, bool>) {
        return val ? "sahi" : "galat";
    } else if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
        return val;
    } else if constexpr (std::is_same_v<std::decay_t<T>, const char *>) {
        return std::string(val);
    } else {
        std::ostringstream ss;
        ss << val;
        return ss.str();
    }
}

template <typename... Args>
void print_dikha(const Args&... args) {
    bool first = true;
    auto print_one = [&](const auto &val) {
        if (!first) std::cout << " ";
        first = false;
        std::cout << runtime_format_rich_text(meme_to_string_fmt(val));
    };
    (print_one(args), ...);
    std::cout << "\n";
}

template <typename E, typename S, typename St, typename... Args>
void print_dikha_opts(const E &endVal, const S &sepVal, const St &styleVal, const Args&... args) {
    std::string endStr = meme_to_string_fmt(endVal);
    std::string sepStr = meme_to_string_fmt(sepVal);
    std::string styleStr = meme_to_string_fmt(styleVal);
    std::string stylePrefix = runtime_resolve_style_ansi(styleStr);
    std::string styleReset = stylePrefix.empty() ? "" : "\033[0m";

    if (!stylePrefix.empty()) {
        std::cout << stylePrefix;
    }
    bool first = true;
    auto print_one = [&](const auto &val) {
        if (!first) std::cout << sepStr;
        first = false;
        std::cout << runtime_format_rich_text(meme_to_string_fmt(val));
    };
    (print_one(args), ...);
    if (!styleReset.empty()) {
        std::cout << styleReset;
    }
    std::cout << endStr;
}

#endif // MEME_RUNTIME_H
