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

template <typename... Args>
void print_dikha(const Args&... args) {
    bool first = true;
    auto print_one = [&](const auto &val) {
        if (!first) std::cout << " ";
        first = false;
        std::cout << val;
    };
    (print_one(args), ...);
    std::cout << "\n";
}

#endif // MEME_RUNTIME_H
