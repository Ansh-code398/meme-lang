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
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <cstdio>

// ==========================================
// 0. RAII ScopeGuard & Signals
// ==========================================

template <typename F>
class ScopeGuard {
    F func;
    bool active = true;
public:
    explicit ScopeGuard(F f) : func(std::move(f)) {}
    ~ScopeGuard() {
        if (active) {
            try { func(); } catch (...) {}
        }
    }
    ScopeGuard(const ScopeGuard &) = delete;
    ScopeGuard &operator=(const ScopeGuard &) = delete;
    ScopeGuard(ScopeGuard &&other) noexcept : func(std::move(other.func)), active(other.active) {
        other.active = false;
    }
    void dismiss() { active = false; }
};

template <typename F>
inline ScopeGuard<F> make_scope_guard(F f) {
    return ScopeGuard<F>(std::move(f));
}

class MemeValue;

struct MemeReturnSignal {
    std::shared_ptr<MemeValue> value;
    explicit MemeReturnSignal(const MemeValue &v);
};

// ==========================================
// 0.1 String List Helper (sys.args, file.lines)
// ==========================================

struct MemeStringList : public std::vector<std::string> {
    using std::vector<std::string>::vector;
    MemeStringList() = default;
    MemeStringList(const std::vector<std::string> &v) : std::vector<std::string>(v) {}
    MemeStringList(std::vector<std::string> &&v) : std::vector<std::string>(std::move(v)) {}

    size_t length() const { return size(); }
    size_t lambai() const { return size(); }

    std::string &operator[](size_t idx);
    const std::string &operator[](size_t idx) const;
    std::string &operator[](int idx);
    const std::string &operator[](int idx) const;
    std::string &operator[](long long idx);
    const std::string &operator[](long long idx) const;
    std::string &operator[](unsigned long long idx);
    const std::string &operator[](unsigned long long idx) const;
    std::string &operator[](const MemeValue &idx);
    const std::string &operator[](const MemeValue &idx) const;
    std::string &operator[](MemeValue &idx);
    const std::string &operator[](MemeValue &idx) const;

    std::string to_string() const {
        std::string res = "[";
        for (size_t idx = 0; idx < size(); ++idx) {
            if (idx > 0) res += ", ";
            res += "\"" + (*this)[idx] + "\"";
        }
        res += "]";
        return res;
    }
};

inline std::ostream &operator<<(std::ostream &os, const MemeStringList &l) {
    os << l.to_string();
    return os;
}

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
        TYPE_OBJECT = 7,
        TYPE_MAP = 8,
        TYPE_OPTION = 9,
        TYPE_RESULT = 10
    };

    Type type = TYPE_NALLA;
    int64_t i_val = 0;
    double f_val = 0.0;
    bool b_val = false;
    std::string s_val = "";
    std::vector<MemeValue> list_val = {};
    std::unordered_map<std::string, MemeValue> map_val = {};
    std::vector<std::string> map_keys = {};
    void *ptr_val = nullptr;
    std::shared_ptr<void> obj_val = nullptr;
    bool opt_res_flag = false;
    mutable std::shared_ptr<MemeValue> inner_val = nullptr;

    MemeValue() : type(TYPE_NALLA) {}
    MemeValue(std::nullptr_t) : type(TYPE_NALLA) {}
    MemeValue(int64_t v) : type(TYPE_INT), i_val(v) {}
    MemeValue(int v) : type(TYPE_INT), i_val(v) {}
    MemeValue(double v) : type(TYPE_FLOAT), f_val(v) {}
    MemeValue(float v) : type(TYPE_FLOAT), f_val(static_cast<double>(v)) {}
    MemeValue(bool v) : type(TYPE_BOOL), b_val(v) {}
    MemeValue(const char *s) : type(TYPE_STRING), s_val(s ? s : "") {}
    MemeValue(const std::string &s) : type(TYPE_STRING), s_val(s) {}
    MemeValue(std::vector<MemeValue> l) : type(TYPE_LIST), list_val(std::move(l)) {}
    MemeValue(void *p) : type(TYPE_POINTER), ptr_val(p) {}
    MemeValue(std::shared_ptr<void> obj) : type(TYPE_OBJECT), obj_val(std::move(obj)) {}
    MemeValue(std::unordered_map<std::string, MemeValue> m)
        : type(TYPE_MAP), map_val(std::move(m)) {
        for (const auto &pair : map_val) {
            map_keys.push_back(pair.first);
        }
    }
    MemeValue(std::initializer_list<std::pair<std::string, MemeValue>> init)
        : type(TYPE_MAP) {
        for (const auto &pair : init) {
            if (map_val.find(pair.first) == map_val.end()) {
                map_keys.push_back(pair.first);
            }
            map_val[pair.first] = pair.second;
        }
    }
    MemeValue(const std::vector<std::string> &vec) : type(TYPE_LIST) {
        list_val.reserve(vec.size());
        for (const auto &s : vec) {
            list_val.push_back(MemeValue(s));
        }
    }
    MemeValue(const MemeStringList &vec) : type(TYPE_LIST) {
        list_val.reserve(vec.size());
        for (const auto &s : vec) {
            list_val.push_back(MemeValue(s));
        }
    }

    // Resilient pointer operator
    MemeValue *operator->() { return this; }
    const MemeValue *operator->() const { return this; }

    // Range-for iterability
    auto begin() { return list_val.begin(); }
    auto end() { return list_val.end(); }
    auto begin() const { return list_val.begin(); }
    auto end() const { return list_val.end(); }

    operator std::string() const {
        return to_string();
    }

    // Truthiness
    bool is_truthy() const {
        switch (type) {
        case TYPE_NALLA: return false;
        case TYPE_INT: return i_val != 0;
        case TYPE_FLOAT: return f_val != 0.0;
        case TYPE_BOOL: return b_val;
        case TYPE_STRING: return !s_val.empty();
        case TYPE_LIST: return !list_val.empty();
        case TYPE_MAP: return !map_val.empty();
        case TYPE_OPTION: return opt_res_flag;
        case TYPE_RESULT: return opt_res_flag;
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
        case TYPE_MAP: {
            std::string res = "{";
            for (size_t idx = 0; idx < map_keys.size(); ++idx) {
                if (idx > 0) res += ", ";
                const auto &k = map_keys[idx];
                res += "\"" + k + "\": ";
                auto it = map_val.find(k);
                if (it != map_val.end()) {
                    if (it->second.type == TYPE_STRING) {
                        res += "\"" + it->second.s_val + "\"";
                    } else {
                        res += it->second.to_string();
                    }
                } else {
                    res += "nalla";
                }
            }
            res += "}";
            return res;
        }
        case TYPE_OPTION:
            return opt_res_flag ? ("Some(" + (inner_val ? inner_val->to_string() : "nalla") + ")") : "None";
        case TYPE_RESULT:
            return opt_res_flag ? ("Ok(" + (inner_val ? inner_val->to_string() : "nalla") + ")")
                                : ("Err(" + (inner_val ? inner_val->to_string() : "nalla") + ")");
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
    bool operator<(int64_t v) const { return type == TYPE_INT ? i_val < v : as_double() < static_cast<double>(v); }
    bool operator<=(int64_t v) const { return type == TYPE_INT ? i_val <= v : as_double() <= static_cast<double>(v); }
    bool operator>(int64_t v) const { return type == TYPE_INT ? i_val > v : as_double() > static_cast<double>(v); }
    bool operator>=(int64_t v) const { return type == TYPE_INT ? i_val >= v : as_double() >= static_cast<double>(v); }
    bool operator==(double v) const { return as_double() == v; }
    bool operator!=(double v) const { return !(*this == v); }
    bool operator<(double v) const { return as_double() < v; }
    bool operator<=(double v) const { return as_double() <= v; }
    bool operator>(double v) const { return as_double() > v; }
    bool operator>=(double v) const { return as_double() >= v; }

    bool operator==(const MemeValue &other) const {
        if (type == TYPE_INT && other.type == TYPE_INT) return i_val == other.i_val;
        if (type == TYPE_FLOAT && other.type == TYPE_FLOAT) return f_val == other.f_val;
        if (type == TYPE_BOOL && other.type == TYPE_BOOL) return b_val == other.b_val;
        if (type == TYPE_STRING && other.type == TYPE_STRING) return s_val == other.s_val;
        if (type == TYPE_NALLA && other.type == TYPE_NALLA) return true;
        if (type == TYPE_MAP && other.type == TYPE_MAP) {
            if (map_val.size() != other.map_val.size()) return false;
            for (const auto &pair : map_val) {
                auto it = other.map_val.find(pair.first);
                if (it == other.map_val.end() || it->second != pair.second) {
                    return false;
                }
            }
            return true;
        }
        if (type == TYPE_OPTION && other.type == TYPE_OPTION) {
            if (opt_res_flag != other.opt_res_flag) return false;
            if (!opt_res_flag) return true;
            if (inner_val && other.inner_val) return *inner_val == *other.inner_val;
            return inner_val == other.inner_val;
        }
        if (type == TYPE_RESULT && other.type == TYPE_RESULT) {
            if (opt_res_flag != other.opt_res_flag) return false;
            if (inner_val && other.inner_val) return *inner_val == *other.inner_val;
            return inner_val == other.inner_val;
        }
        if (type == TYPE_LIST && other.type == TYPE_LIST) {
            if (list_val.size() != other.list_val.size()) return false;
            for (size_t i = 0; i < list_val.size(); ++i) {
                if (list_val[i] != other.list_val[i]) return false;
            }
            return true;
        }
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

    MemeValue &operator[](int index) {
        return (*this)[static_cast<long long>(index)];
    }

    const MemeValue &operator[](int index) const {
        return (*this)[static_cast<long long>(index)];
    }

    MemeValue &operator[](unsigned int index) {
        return (*this)[static_cast<long long>(index)];
    }

    const MemeValue &operator[](unsigned int index) const {
        return (*this)[static_cast<long long>(index)];
    }

    MemeValue &operator[](long index) {
        return (*this)[static_cast<long long>(index)];
    }

    const MemeValue &operator[](long index) const {
        return (*this)[static_cast<long long>(index)];
    }

    MemeValue &operator[](long long index) {
        if (type == TYPE_STRING) {
            int64_t origIdx = index;
            int64_t sz = static_cast<int64_t>(s_val.size());
            if (index < 0) index += sz;
            if (index < 0 || static_cast<size_t>(index) >= s_val.size()) {
                throw std::runtime_error("Aukaat ke bahar! Index " + std::to_string(origIdx) + 
                                         " maang rahe ho par string length sirf " + std::to_string(sz) + " hai!");
            }
            inner_val = std::make_shared<MemeValue>(std::string(1, s_val[static_cast<size_t>(index)]));
            return *inner_val;
        }
        if (type != TYPE_LIST) {
            type = TYPE_LIST;
        }
        int64_t origIdx = index;
        int64_t sz = static_cast<int64_t>(list_val.size());
        if (index < 0) index += sz;
        if (index < 0 || static_cast<size_t>(index) >= list_val.size()) {
            throw std::runtime_error("Aukaat ke bahar! Index " + std::to_string(origIdx) + 
                                     " maang rahe ho par size sirf " + std::to_string(sz) + " hai!");
        }
        return list_val[static_cast<size_t>(index)];
    }

    const MemeValue &operator[](long long index) const {
        if (type == TYPE_STRING) {
            int64_t origIdx = index;
            int64_t sz = static_cast<int64_t>(s_val.size());
            if (index < 0) index += sz;
            if (index < 0 || static_cast<size_t>(index) >= s_val.size()) {
                throw std::runtime_error("Aukaat ke bahar! Index " + std::to_string(origIdx) + 
                                         " maang rahe ho par string length sirf " + std::to_string(sz) + " hai!");
            }
            inner_val = std::make_shared<MemeValue>(std::string(1, s_val[static_cast<size_t>(index)]));
            return *inner_val;
        }
        if (type != TYPE_LIST) {
            static const MemeValue nilVal;
            return nilVal;
        }
        int64_t origIdx = index;
        int64_t sz = static_cast<int64_t>(list_val.size());
        if (index < 0) index += sz;
        if (index < 0 || static_cast<size_t>(index) >= list_val.size()) {
            throw std::runtime_error("Aukaat ke bahar! Index " + std::to_string(origIdx) + 
                                     " maang rahe ho par size sirf " + std::to_string(sz) + " hai!");
        }
        return list_val[static_cast<size_t>(index)];
    }

    MemeValue &operator[](unsigned long long index) {
        return (*this)[static_cast<long long>(index)];
    }

    const MemeValue &operator[](unsigned long long index) const {
        return (*this)[static_cast<long long>(index)];
    }

    MemeValue &operator[](const std::string &key) {
        if (type != TYPE_MAP) {
            type = TYPE_MAP;
        }
        if (map_val.find(key) == map_val.end()) {
            map_keys.push_back(key);
        }
        return map_val[key];
    }

    const MemeValue &operator[](const std::string &key) const {
        if (type != TYPE_MAP) {
            static const MemeValue nilVal;
            return nilVal;
        }
        auto it = map_val.find(key);
        if (it != map_val.end()) return it->second;
        static const MemeValue nilVal;
        return nilVal;
    }

    MemeValue &operator[](const char *key) {
        return (*this)[std::string(key ? key : "")];
    }

    const MemeValue &operator[](const char *key) const {
        return (*this)[std::string(key ? key : "")];
    }

    MemeValue &operator[](const MemeValue &index) {
        if (type == TYPE_MAP || index.type == TYPE_STRING) {
            return (*this)[index.to_string()];
        }
        return (*this)[index.as_int()];
    }

    const MemeValue &operator[](const MemeValue &index) const {
        if (type == TYPE_MAP || index.type == TYPE_STRING) {
            return (*this)[index.to_string()];
        }
        return (*this)[index.as_int()];
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
        if (type == TYPE_MAP) return map_val.size();
        return 0;
    }

    size_t length() const {
        return size();
    }

    size_t lambai() const {
        return size();
    }

    // Map methods
    bool has(const std::string &key) const {
        if (type == TYPE_MAP) {
            return map_val.find(key) != map_val.end();
        }
        if (type == TYPE_STRING) {
            return s_val.find(key) != std::string::npos;
        }
        return false;
    }
    bool has(const MemeValue &key) const { return has(key.to_string()); }
    bool has(const char *key) const { return has(std::string(key ? key : "")); }
    bool hai_kya(const std::string &key) const { return has(key); }
    bool hai_kya(const MemeValue &key) const { return has(key); }
    bool contains(const std::string &key) const { return has(key); }
    bool contains(const MemeValue &key) const { return has(key); }

    MemeValue keys() const {
        std::vector<MemeValue> res;
        if (type == TYPE_MAP) {
            res.reserve(map_keys.size());
            for (const auto &k : map_keys) {
                res.push_back(MemeValue(k));
            }
        }
        return MemeValue(res);
    }
    MemeValue chaabi() const { return keys(); }

    MemeValue values() const {
        std::vector<MemeValue> res;
        if (type == TYPE_MAP) {
            res.reserve(map_keys.size());
            for (const auto &k : map_keys) {
                auto it = map_val.find(k);
                if (it != map_val.end()) {
                    res.push_back(it->second);
                }
            }
        }
        return MemeValue(res);
    }
    MemeValue maal() const { return values(); }

    bool remove(const std::string &key) {
        if (type != TYPE_MAP) return false;
        auto it = map_val.find(key);
        if (it == map_val.end()) return false;
        map_val.erase(it);
        for (auto kit = map_keys.begin(); kit != map_keys.end(); ++kit) {
            if (*kit == key) {
                map_keys.erase(kit);
                break;
            }
        }
        return true;
    }
    bool remove(const MemeValue &key) { return remove(key.to_string()); }
    bool hata(const std::string &key) { return remove(key); }
    bool hata(const MemeValue &key) { return remove(key); }

    MemeValue get(const std::string &key, const MemeValue &def = MemeValue()) const {
        if (type != TYPE_MAP) return def;
        auto it = map_val.find(key);
        if (it != map_val.end()) return it->second;
        return def;
    }
    MemeValue get(const MemeValue &key, const MemeValue &def = MemeValue()) const {
        return get(key.to_string(), def);
    }
    MemeValue la(const std::string &key, const MemeValue &def = MemeValue()) const {
        return get(key, def);
    }
    MemeValue la(const MemeValue &key, const MemeValue &def = MemeValue()) const {
        return get(key, def);
    }

    // String utility methods
    MemeValue charAt(int64_t i) const {
        if (type != TYPE_STRING) return MemeValue("");
        int64_t sz = static_cast<int64_t>(s_val.size());
        if (i < 0) i += sz;
        if (i < 0 || i >= sz) {
            throw std::runtime_error("Aukaat ke bahar! Index " + std::to_string(i) + " string ki lambai se bahar hai!");
        }
        return MemeValue(std::string(1, s_val[static_cast<size_t>(i)]));
    }
    MemeValue charAt(const MemeValue &idx) const { return charAt(idx.as_int()); }
    MemeValue akshar(int64_t i) const { return charAt(i); }
    MemeValue akshar(const MemeValue &idx) const { return charAt(idx.as_int()); }

    MemeValue chars() const {
        std::vector<MemeValue> res;
        if (type == TYPE_STRING) {
            res.reserve(s_val.size());
            for (char ch : s_val) {
                res.push_back(MemeValue(std::string(1, ch)));
            }
        }
        return MemeValue(res);
    }
    MemeValue akshar_list() const { return chars(); }

    MemeValue slice(int64_t start, int64_t end = INT64_MIN) const {
        if (type != TYPE_STRING) return MemeValue("");
        int64_t sz = static_cast<int64_t>(s_val.size());
        if (start < 0) start += sz;
        if (start < 0) start = 0;
        if (start > sz) start = sz;

        if (end == INT64_MIN) {
            end = sz;
        } else {
            if (end < 0) end += sz;
            if (end < 0) end = 0;
            if (end > sz) end = sz;
        }

        if (start >= end) return MemeValue("");
        return MemeValue(s_val.substr(static_cast<size_t>(start), static_cast<size_t>(end - start)));
    }
    MemeValue slice(const MemeValue &start) const { return slice(start.as_int()); }
    MemeValue slice(const MemeValue &start, const MemeValue &end) const { return slice(start.as_int(), end.as_int()); }
    MemeValue kaat(int64_t start, int64_t end = INT64_MIN) const { return slice(start, end); }
    MemeValue kaat(const MemeValue &start) const { return slice(start); }
    MemeValue kaat(const MemeValue &start, const MemeValue &end) const { return slice(start, end); }
    MemeValue substring(int64_t start, int64_t end = INT64_MIN) const { return slice(start, end); }
    MemeValue substring(const MemeValue &start, const MemeValue &end) const { return slice(start, end); }

    bool starts_with(const std::string &prefix) const {
        if (type != TYPE_STRING) return false;
        return s_val.rfind(prefix, 0) == 0;
    }
    bool starts_with(const MemeValue &prefix) const { return starts_with(prefix.to_string()); }
    bool startsWith(const std::string &prefix) const { return starts_with(prefix); }
    bool startsWith(const MemeValue &prefix) const { return starts_with(prefix.to_string()); }
    bool shuru_hota(const std::string &prefix) const { return starts_with(prefix); }
    bool shuru_hota(const MemeValue &prefix) const { return starts_with(prefix.to_string()); }

    bool ends_with(const std::string &suffix) const {
        if (type != TYPE_STRING) return false;
        if (suffix.size() > s_val.size()) return false;
        return s_val.compare(s_val.size() - suffix.size(), suffix.size(), suffix) == 0;
    }
    bool ends_with(const MemeValue &suffix) const { return ends_with(suffix.to_string()); }
    bool endsWith(const std::string &suffix) const { return ends_with(suffix); }
    bool endsWith(const MemeValue &suffix) const { return ends_with(suffix.to_string()); }
    bool khatam_hota(const std::string &suffix) const { return ends_with(suffix); }
    bool khatam_hota(const MemeValue &suffix) const { return ends_with(suffix.to_string()); }

    int64_t find(const std::string &sub) const {
        if (type != TYPE_STRING) return -1LL;
        size_t pos = s_val.find(sub);
        return pos == std::string::npos ? -1LL : static_cast<int64_t>(pos);
    }
    int64_t find(const MemeValue &sub) const { return find(sub.to_string()); }
    int64_t dhund(const std::string &sub) const { return find(sub); }
    int64_t dhund(const MemeValue &sub) const { return find(sub.to_string()); }
    int64_t indexOf(const std::string &sub) const { return find(sub); }
    int64_t indexOf(const MemeValue &sub) const { return find(sub.to_string()); }
    int64_t kahan(const std::string &sub) const { return find(sub); }
    int64_t kahan(const MemeValue &sub) const { return find(sub.to_string()); }

    MemeValue split(const std::string &sep) const {
        std::vector<MemeValue> res;
        if (type != TYPE_STRING) return MemeValue(res);
        if (sep.empty()) {
            for (char ch : s_val) {
                res.push_back(MemeValue(std::string(1, ch)));
            }
        } else {
            size_t start = 0;
            size_t pos = 0;
            while ((pos = s_val.find(sep, start)) != std::string::npos) {
                res.push_back(MemeValue(s_val.substr(start, pos - start)));
                start = pos + sep.length();
            }
            res.push_back(MemeValue(s_val.substr(start)));
        }
        return MemeValue(res);
    }
    MemeValue split(const MemeValue &sep) const { return split(sep.to_string()); }
    MemeValue tod(const std::string &sep) const { return split(sep); }
    MemeValue tod(const MemeValue &sep) const { return split(sep.to_string()); }

    MemeValue replace(const std::string &old_str, const std::string &new_str) const {
        if (type != TYPE_STRING) return *this;
        if (old_str.empty()) return *this;
        std::string res = s_val;
        size_t pos = 0;
        while ((pos = res.find(old_str, pos)) != std::string::npos) {
            res.replace(pos, old_str.length(), new_str);
            pos += new_str.length();
        }
        return MemeValue(res);
    }
    MemeValue replace(const MemeValue &o, const MemeValue &n) const {
        return replace(o.to_string(), n.to_string());
    }
    MemeValue badal(const std::string &o, const std::string &n) const { return replace(o, n); }
    MemeValue badal(const MemeValue &o, const MemeValue &n) const { return replace(o, n); }

    MemeValue trim() const {
        if (type != TYPE_STRING) return *this;
        size_t first = s_val.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return MemeValue("");
        size_t last = s_val.find_last_not_of(" \t\r\n");
        return MemeValue(s_val.substr(first, last - first + 1));
    }
    MemeValue saaf() const { return trim(); }
    MemeValue strip() const { return trim(); }

    MemeValue to_upper() const {
        if (type != TYPE_STRING) return *this;
        std::string res = s_val;
        for (char &c : res) c = std::toupper(static_cast<unsigned char>(c));
        return MemeValue(res);
    }
    MemeValue upper() const { return to_upper(); }
    MemeValue chilla() const { return to_upper(); }

    MemeValue to_lower() const {
        if (type != TYPE_STRING) return *this;
        std::string res = s_val;
        for (char &c : res) c = std::tolower(static_cast<unsigned char>(c));
        return MemeValue(res);
    }
    MemeValue lower() const { return to_lower(); }
    MemeValue dheere_bol() const { return to_lower(); }

    // Option / Result methods
    bool is_some() const { return type == TYPE_OPTION && opt_res_flag; }
    bool is_none() const { return type == TYPE_OPTION && !opt_res_flag; }
    bool isSome() const { return is_some(); }
    bool isNone() const { return is_none(); }

    bool is_ok() const { return type == TYPE_RESULT && opt_res_flag; }
    bool is_err() const { return type == TYPE_RESULT && !opt_res_flag; }
    bool isOk() const { return is_ok(); }
    bool isErr() const { return is_err(); }
    bool sahi_hai() const { return is_ok() || is_some(); }
    bool galat_hai() const { return is_err() || is_none(); }

    MemeValue unwrap() const {
        if (type == TYPE_OPTION) {
            if (!opt_res_flag || !inner_val) throw std::runtime_error("Option is None!");
            return *inner_val;
        }
        if (type == TYPE_RESULT) {
            if (!opt_res_flag || !inner_val) {
                throw std::runtime_error("Result is Err: " + (inner_val ? inner_val->to_string() : ""));
            }
            return *inner_val;
        }
        return *this;
    }
    MemeValue khol() const { return unwrap(); }

    MemeValue unwrap_err() const {
        if (type == TYPE_RESULT) {
            if (opt_res_flag) throw std::runtime_error("Result is Ok!");
            return inner_val ? *inner_val : MemeValue();
        }
        throw std::runtime_error("Not a Result type!");
    }
    MemeValue khol_galat() const { return unwrap_err(); }

    MemeValue unwrap_or(const MemeValue &def) const {
        if (type == TYPE_OPTION) {
            return (opt_res_flag && inner_val) ? *inner_val : def;
        }
        if (type == TYPE_RESULT) {
            return (opt_res_flag && inner_val) ? *inner_val : def;
        }
        return *this;
    }
    MemeValue khol_ya_fir(const MemeValue &def) const { return unwrap_or(def); }
};

inline std::string &MemeStringList::operator[](size_t idx) {
    if (idx >= this->size()) {
        thread_local std::string empty;
        empty.clear();
        return empty;
    }
    return std::vector<std::string>::operator[](idx);
}

inline const std::string &MemeStringList::operator[](size_t idx) const {
    if (idx >= this->size()) {
        static const std::string empty = "";
        return empty;
    }
    return std::vector<std::string>::operator[](idx);
}

inline std::string &MemeStringList::operator[](long long idx) {
    int64_t sz = static_cast<int64_t>(this->size());
    if (idx < 0) idx += sz;
    if (idx < 0 || static_cast<size_t>(idx) >= this->size()) {
        thread_local std::string empty;
        empty.clear();
        return empty;
    }
    return std::vector<std::string>::operator[](static_cast<size_t>(idx));
}

inline const std::string &MemeStringList::operator[](long long idx) const {
    int64_t sz = static_cast<int64_t>(this->size());
    if (idx < 0) idx += sz;
    if (idx < 0 || static_cast<size_t>(idx) >= this->size()) {
        static const std::string empty = "";
        return empty;
    }
    return std::vector<std::string>::operator[](static_cast<size_t>(idx));
}

inline std::string &MemeStringList::operator[](int idx) {
    return (*this)[static_cast<long long>(idx)];
}

inline const std::string &MemeStringList::operator[](int idx) const {
    return (*this)[static_cast<long long>(idx)];
}

inline std::string &MemeStringList::operator[](unsigned long long idx) {
    if (idx >= this->size()) {
        thread_local std::string empty;
        empty.clear();
        return empty;
    }
    return std::vector<std::string>::operator[](static_cast<size_t>(idx));
}

inline const std::string &MemeStringList::operator[](unsigned long long idx) const {
    if (idx >= this->size()) {
        static const std::string empty = "";
        return empty;
    }
    return std::vector<std::string>::operator[](static_cast<size_t>(idx));
}

inline std::string &MemeStringList::operator[](const MemeValue &idx) {
    return (*this)[idx.as_int()];
}

inline const std::string &MemeStringList::operator[](const MemeValue &idx) const {
    return (*this)[idx.as_int()];
}

inline std::string &MemeStringList::operator[](MemeValue &idx) {
    return (*this)[idx.as_int()];
}

inline const std::string &MemeStringList::operator[](MemeValue &idx) const {
    return (*this)[idx.as_int()];
}
inline MemeReturnSignal::MemeReturnSignal(const MemeValue &v)
    : value(std::make_shared<MemeValue>(v)) {}

inline MemeValue Some(const MemeValue &v) {
    MemeValue val;
    val.type = MemeValue::TYPE_OPTION;
    val.opt_res_flag = true;
    val.inner_val = std::make_shared<MemeValue>(v);
    return val;
}
inline MemeValue Kuch(const MemeValue &v) { return Some(v); }

inline MemeValue Ok(const MemeValue &v) {
    MemeValue val;
    val.type = MemeValue::TYPE_RESULT;
    val.opt_res_flag = true;
    val.inner_val = std::make_shared<MemeValue>(v);
    return val;
}
inline MemeValue Sahi(const MemeValue &v) { return Ok(v); }

inline MemeValue Err(const MemeValue &v) {
    MemeValue val;
    val.type = MemeValue::TYPE_RESULT;
    val.opt_res_flag = false;
    val.inner_val = std::make_shared<MemeValue>(v);
    return val;
}
inline MemeValue Galat(const MemeValue &v) { return Err(v); }

struct NoneVal : public MemeValue {
    NoneVal() {
        type = TYPE_OPTION;
        opt_res_flag = false;
        inner_val = nullptr;
    }
    MemeValue operator()() const {
        return *this;
    }
};
inline const NoneVal None;
inline const NoneVal Khali;

inline MemeValue make_map(std::initializer_list<std::pair<std::string, MemeValue>> entries = {}) {
    return MemeValue(entries);
}
inline MemeValue make_map(const std::vector<std::pair<std::string, MemeValue>> &entries) {
    std::unordered_map<std::string, MemeValue> m;
    std::vector<std::string> keys;
    for (const auto &p : entries) {
        if (m.find(p.first) == m.end()) {
            keys.push_back(p.first);
        }
        m[p.first] = p.second;
    }
    MemeValue res(m);
    res.map_keys = keys;
    return res;
}

// Standalone String Utilities
inline std::string meme_charAt(const std::string &str, int64_t i) {
    int64_t sz = static_cast<int64_t>(str.size());
    if (i < 0) i += sz;
    if (i < 0 || i >= sz) {
        throw std::runtime_error("Aukaat ke bahar! Index " + std::to_string(i) + " string ki lambai se bahar hai!");
    }
    return std::string(1, str[static_cast<size_t>(i)]);
}
inline MemeValue meme_charAt(const MemeValue &val, int64_t i) { return val.charAt(i); }

inline std::vector<std::string> meme_chars(const std::string &str) {
    std::vector<std::string> res;
    res.reserve(str.size());
    for (char ch : str) {
        res.push_back(std::string(1, ch));
    }
    return res;
}
inline MemeValue meme_chars(const MemeValue &val) { return val.chars(); }

inline std::string meme_slice(const std::string &str, int64_t start, int64_t end = INT64_MIN) {
    int64_t sz = static_cast<int64_t>(str.size());
    if (start < 0) start += sz;
    if (start < 0) start = 0;
    if (start > sz) start = sz;

    if (end == INT64_MIN) {
        end = sz;
    } else {
        if (end < 0) end += sz;
        if (end < 0) end = 0;
        if (end > sz) end = sz;
    }

    if (start >= end) return "";
    return str.substr(static_cast<size_t>(start), static_cast<size_t>(end - start));
}
inline MemeValue meme_slice(const MemeValue &val, int64_t start, int64_t end = INT64_MIN) { return val.slice(start, end); }

inline bool meme_starts_with(const std::string &str, const std::string &prefix) {
    return str.rfind(prefix, 0) == 0;
}
inline bool meme_starts_with(const MemeValue &val, const std::string &prefix) { return val.starts_with(prefix); }

inline bool meme_ends_with(const std::string &str, const std::string &suffix) {
    if (suffix.size() > str.size()) return false;
    return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}
inline bool meme_ends_with(const MemeValue &val, const std::string &suffix) { return val.ends_with(suffix); }

inline int64_t meme_find(const std::string &str, const std::string &sub) {
    size_t pos = str.find(sub);
    return pos == std::string::npos ? -1LL : static_cast<int64_t>(pos);
}
inline int64_t meme_find(const MemeValue &val, const std::string &sub) { return val.find(sub); }

inline size_t meme_length(const std::string &str) { return str.size(); }
inline size_t meme_length(const MemeValue &val) { return val.length(); }

inline std::string meme_trim(const std::string &str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}
inline MemeValue meme_trim(const MemeValue &val) { return val.trim(); }

inline std::ostream &operator<<(std::ostream &os, const MemeValue &v) {
    os << v.to_string();
    return os;
}

inline MemeValue operator+(int64_t a, const MemeValue &b) { return MemeValue(a) + b; }
inline MemeValue operator+(double a, const MemeValue &b) { return MemeValue(a) + b; }
inline std::string operator+(const std::string &a, const MemeValue &b) { return a + b.to_string(); }
inline std::string operator+(const MemeValue &a, const std::string &b) { return a.to_string() + b; }
inline std::string operator+(const char *a, const MemeValue &b) { return std::string(a ? a : "") + b.to_string(); }
inline std::string operator+(const MemeValue &a, const char *b) { return a.to_string() + std::string(b ? b : ""); }
inline MemeValue operator-(int64_t a, const MemeValue &b) { return MemeValue(a) - b; }
inline MemeValue operator-(double a, const MemeValue &b) { return MemeValue(a) - b; }
inline MemeValue operator*(int64_t a, const MemeValue &b) { return MemeValue(a) * b; }
inline MemeValue operator*(double a, const MemeValue &b) { return MemeValue(a) * b; }
inline MemeValue operator/(int64_t a, const MemeValue &b) { return MemeValue(a) / b; }
inline MemeValue operator/(double a, const MemeValue &b) { return MemeValue(a) / b; }

inline bool operator==(int64_t a, const MemeValue &b) { return b == a; }
inline bool operator!=(int64_t a, const MemeValue &b) { return b != a; }
inline bool operator<(int64_t a, const MemeValue &b) { return b > a; }
inline bool operator<=(int64_t a, const MemeValue &b) { return b >= a; }
inline bool operator>(int64_t a, const MemeValue &b) { return b < a; }
inline bool operator>=(int64_t a, const MemeValue &b) { return b <= a; }

inline bool operator==(double a, const MemeValue &b) { return b == a; }
inline bool operator!=(double a, const MemeValue &b) { return b != a; }
inline bool operator<(double a, const MemeValue &b) { return b > a; }
inline bool operator<=(double a, const MemeValue &b) { return b >= a; }
inline bool operator>(double a, const MemeValue &b) { return b < a; }
inline bool operator>=(double a, const MemeValue &b) { return b <= a; }

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
// 1.5 Safe Subscript Helper (meme_index)
// ==========================================

// Strings
inline std::string meme_index(const std::string &s, int64_t idx) {
    int64_t origIdx = idx;
    int64_t sz = static_cast<int64_t>(s.size());
    if (idx < 0) idx += sz;
    if (idx < 0 || static_cast<size_t>(idx) >= s.size()) {
        throw std::runtime_error("Aukaat ke bahar! Index " + std::to_string(origIdx) + 
                                 " maang rahe ho par string length sirf " + std::to_string(sz) + " hai!");
    }
    return std::string(1, s[static_cast<size_t>(idx)]);
}
inline std::string meme_index(std::string &s, int64_t idx) {
    return meme_index(static_cast<const std::string &>(s), idx);
}
inline std::string meme_index(std::string &&s, int64_t idx) {
    return meme_index(static_cast<const std::string &>(s), idx);
}
inline std::string meme_index(const std::string &s, int idx) {
    return meme_index(s, static_cast<int64_t>(idx));
}
inline std::string meme_index(std::string &s, int idx) {
    return meme_index(s, static_cast<int64_t>(idx));
}
inline std::string meme_index(std::string &&s, int idx) {
    return meme_index(static_cast<const std::string &>(s), static_cast<int64_t>(idx));
}
inline std::string meme_index(const std::string &s, size_t idx) {
    return meme_index(s, static_cast<int64_t>(idx));
}
inline std::string meme_index(std::string &s, size_t idx) {
    return meme_index(s, static_cast<int64_t>(idx));
}
inline std::string meme_index(std::string &&s, size_t idx) {
    return meme_index(static_cast<const std::string &>(s), static_cast<int64_t>(idx));
}
inline std::string meme_index(const std::string &s, const MemeValue &idx) {
    return meme_index(s, idx.as_int());
}
inline std::string meme_index(std::string &s, const MemeValue &idx) {
    return meme_index(s, idx.as_int());
}
inline std::string meme_index(std::string &&s, const MemeValue &idx) {
    return meme_index(static_cast<const std::string &>(s), idx.as_int());
}
inline std::string meme_index(const std::string &s, MemeValue &idx) {
    return meme_index(s, idx.as_int());
}
inline std::string meme_index(std::string &s, MemeValue &idx) {
    return meme_index(s, idx.as_int());
}
inline std::string meme_index(std::string &&s, MemeValue &idx) {
    return meme_index(static_cast<const std::string &>(s), idx.as_int());
}

inline std::string meme_index(const char *s, int64_t idx) {
    return meme_index(std::string(s ? s : ""), idx);
}
inline std::string meme_index(const char *s, int idx) {
    return meme_index(std::string(s ? s : ""), static_cast<int64_t>(idx));
}
inline std::string meme_index(const char *s, size_t idx) {
    return meme_index(std::string(s ? s : ""), static_cast<int64_t>(idx));
}
inline std::string meme_index(const char *s, const MemeValue &idx) {
    return meme_index(std::string(s ? s : ""), idx.as_int());
}
inline std::string meme_index(const char *s, MemeValue &idx) {
    return meme_index(std::string(s ? s : ""), idx.as_int());
}

// Vector
template <typename T>
inline T &meme_index(std::vector<T> &v, int64_t idx) {
    int64_t origIdx = idx;
    int64_t sz = static_cast<int64_t>(v.size());
    if (idx < 0) idx += sz;
    if (idx < 0 || static_cast<size_t>(idx) >= v.size()) {
        throw std::runtime_error("Aukaat ke bahar! Index " + std::to_string(origIdx) + 
                                 " maang rahe ho par size sirf " + std::to_string(sz) + " hai!");
    }
    return v[static_cast<size_t>(idx)];
}

template <typename T>
inline const T &meme_index(const std::vector<T> &v, int64_t idx) {
    int64_t origIdx = idx;
    int64_t sz = static_cast<int64_t>(v.size());
    if (idx < 0) idx += sz;
    if (idx < 0 || static_cast<size_t>(idx) >= v.size()) {
        throw std::runtime_error("Aukaat ke bahar! Index " + std::to_string(origIdx) + 
                                 " maang rahe ho par size sirf " + std::to_string(sz) + " hai!");
    }
    return v[static_cast<size_t>(idx)];
}

template <typename T>
inline T &meme_index(std::vector<T> &v, int idx) {
    return meme_index(v, static_cast<int64_t>(idx));
}
template <typename T>
inline const T &meme_index(const std::vector<T> &v, int idx) {
    return meme_index(v, static_cast<int64_t>(idx));
}
template <typename T>
inline T &meme_index(std::vector<T> &v, size_t idx) {
    return meme_index(v, static_cast<int64_t>(idx));
}
template <typename T>
inline const T &meme_index(const std::vector<T> &v, size_t idx) {
    return meme_index(v, static_cast<int64_t>(idx));
}
template <typename T>
inline T &meme_index(std::vector<T> &v, const MemeValue &idx) {
    return meme_index(v, idx.as_int());
}
template <typename T>
inline const T &meme_index(const std::vector<T> &v, const MemeValue &idx) {
    return meme_index(v, idx.as_int());
}
template <typename T>
inline T &meme_index(std::vector<T> &v, MemeValue &idx) {
    return meme_index(v, idx.as_int());
}
template <typename T>
inline const T &meme_index(const std::vector<T> &v, MemeValue &idx) {
    return meme_index(v, idx.as_int());
}

// MemeValue
inline MemeValue &meme_index(MemeValue &val, int64_t idx) {
    return val[idx];
}
inline const MemeValue &meme_index(const MemeValue &val, int64_t idx) {
    return val[idx];
}
inline MemeValue &meme_index(MemeValue &val, int idx) {
    return val[static_cast<long long>(idx)];
}
inline const MemeValue &meme_index(const MemeValue &val, int idx) {
    return val[static_cast<long long>(idx)];
}
inline MemeValue &meme_index(MemeValue &val, size_t idx) {
    return val[idx];
}
inline const MemeValue &meme_index(const MemeValue &val, size_t idx) {
    return val[idx];
}
inline MemeValue &meme_index(MemeValue &val, const std::string &key) {
    return val[key];
}
inline const MemeValue &meme_index(const MemeValue &val, const std::string &key) {
    return val[key];
}
inline MemeValue &meme_index(MemeValue &val, const char *key) {
    return val[key];
}
inline const MemeValue &meme_index(const MemeValue &val, const char *key) {
    return val[key];
}
inline MemeValue &meme_index(MemeValue &val, const MemeValue &key) {
    return val[key];
}
inline const MemeValue &meme_index(const MemeValue &val, const MemeValue &key) {
    return val[key];
}
inline MemeValue &meme_index(MemeValue &val, MemeValue &key) {
    return val[key];
}
inline const MemeValue &meme_index(const MemeValue &val, MemeValue &key) {
    return val[key];
}

// MemeStringList
inline std::string &meme_index(MemeStringList &list, int64_t idx) {
    return list[idx];
}
inline const std::string &meme_index(const MemeStringList &list, int64_t idx) {
    return list[idx];
}
inline std::string &meme_index(MemeStringList &list, int idx) {
    return list[idx];
}
inline const std::string &meme_index(const MemeStringList &list, int idx) {
    return list[idx];
}
inline std::string &meme_index(MemeStringList &list, size_t idx) {
    return list[idx];
}
inline const std::string &meme_index(const MemeStringList &list, size_t idx) {
    return list[idx];
}
inline std::string &meme_index(MemeStringList &list, const MemeValue &idx) {
    return list[idx];
}
inline const std::string &meme_index(const MemeStringList &list, const MemeValue &idx) {
    return list[idx];
}
inline std::string &meme_index(MemeStringList &list, MemeValue &idx) {
    return list[idx];
}
inline const std::string &meme_index(const MemeStringList &list, MemeValue &idx) {
    return list[idx];
}

// Fallback template for map and custom types
template <typename T>
struct is_std_vector : std::false_type {};

template <typename T, typename Alloc>
struct is_std_vector<std::vector<T, Alloc>> : std::true_type {};

template <typename T>
inline constexpr bool is_std_vector_v = is_std_vector<std::remove_cvref_t<T>>::value;

template <typename T>
concept MemeIndexExcluded = 
    is_std_vector_v<T> ||
    std::is_same_v<std::remove_cvref_t<T>, std::string> ||
    std::is_same_v<std::remove_cvref_t<T>, MemeValue> ||
    std::is_same_v<std::remove_cvref_t<T>, MemeStringList> ||
    std::is_same_v<std::remove_cvref_t<T>, const char *> ||
    std::is_same_v<std::remove_cvref_t<T>, char *>;

template <typename T, typename K>
requires (!MemeIndexExcluded<T>)
inline decltype(auto) meme_index(T &&target, K &&key) {
    return std::forward<T>(target)[std::forward<K>(key)];
}

template <typename T, typename K>
inline auto meme_index_read(const T &target, K &&key) {
    return meme_index(target, std::forward<K>(key));
}

// ==========================================
// 2. Standard Modules & Builtins
// ==========================================

struct FileModule {
    FileModule *operator->() { return this; }
    const FileModule *operator->() const { return this; }

    std::string read(const std::string &path) {
        std::ifstream f(path);
        if (!f.is_open()) {
            throw std::runtime_error("File gayab ho gayi! Path: '" + path + "'");
        }
        std::stringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }
    std::string read(const MemeValue &path) { return read(path.to_string()); }
    std::string padh(const std::string &path) { return read(path); }
    std::string padh(const MemeValue &path) { return read(path); }

    bool write(const std::string &path, const std::string &content) {
        std::ofstream f(path, std::ios::trunc);
        if (!f.is_open()) {
            throw std::runtime_error("File kholne mein lafda ho gaya! Path: '" + path + "'");
        }
        f << content;
        return true;
    }
    bool write(const MemeValue &path, const MemeValue &content) {
        return write(path.to_string(), content.to_string());
    }
    bool write(const std::string &path, const MemeValue &content) {
        return write(path, content.to_string());
    }
    bool write(const MemeValue &path, const std::string &content) {
        return write(path.to_string(), content);
    }
    bool likh(const std::string &path, const std::string &content) { return write(path, content); }
    bool likh(const MemeValue &path, const MemeValue &content) { return write(path, content); }
    bool likh(const std::string &path, const MemeValue &content) { return write(path, content); }
    bool likh(const MemeValue &path, const std::string &content) { return write(path, content); }

    bool append(const std::string &path, const std::string &content) {
        std::ofstream f(path, std::ios::app);
        if (!f.is_open()) {
            throw std::runtime_error("File kholne mein lafda ho gaya! Path: '" + path + "'");
        }
        f << content;
        return true;
    }
    bool append(const MemeValue &path, const MemeValue &content) {
        return append(path.to_string(), content.to_string());
    }
    bool append(const std::string &path, const MemeValue &content) {
        return append(path, content.to_string());
    }
    bool append(const MemeValue &path, const std::string &content) {
        return append(path.to_string(), content);
    }
    bool jod(const std::string &path, const std::string &content) { return append(path, content); }
    bool jod(const MemeValue &path, const MemeValue &content) { return append(path, content); }
    bool jod(const std::string &path, const MemeValue &content) { return append(path, content); }
    bool jod(const MemeValue &path, const std::string &content) { return append(path, content); }

    bool exists(const std::string &path) {
        std::error_code ec;
        return std::filesystem::exists(path, ec);
    }
    bool exists(const MemeValue &path) { return exists(path.to_string()); }
    bool hai_kya(const std::string &path) { return exists(path); }
    bool hai_kya(const MemeValue &path) { return exists(path); }

    MemeStringList lines(const std::string &path) {
        std::ifstream f(path);
        if (!f.is_open()) {
            throw std::runtime_error("File gayab ho gayi! Path: '" + path + "'");
        }
        MemeStringList res;
        std::string line;
        while (std::getline(f, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            res.push_back(line);
        }
        return res;
    }
    MemeStringList lines(const MemeValue &path) { return lines(path.to_string()); }
    MemeStringList read_lines(const std::string &path) { return lines(path); }
    MemeStringList read_lines(const MemeValue &path) { return lines(path); }

    bool remove(const std::string &path) {
        std::error_code ec;
        return std::filesystem::remove(path, ec);
    }
    bool remove(const MemeValue &path) { return remove(path.to_string()); }
    bool hatao(const std::string &path) { return remove(path); }
    bool hatao(const MemeValue &path) { return remove(path); }

    bool copy(const std::string &src, const std::string &dst) {
        std::error_code ec;
        return std::filesystem::copy_file(src, dst, std::filesystem::copy_options::overwrite_existing, ec);
    }
    bool copy(const MemeValue &src, const MemeValue &dst) {
        return copy(src.to_string(), dst.to_string());
    }
    bool nakal(const std::string &src, const std::string &dst) { return copy(src, dst); }
    bool nakal(const MemeValue &src, const MemeValue &dst) { return copy(src, dst); }

    bool rename(const std::string &src, const std::string &dst) {
        std::error_code ec;
        std::filesystem::rename(src, dst, ec);
        return !ec;
    }
    bool rename(const MemeValue &src, const MemeValue &dst) {
        return rename(src.to_string(), dst.to_string());
    }
    bool badlo(const std::string &src, const std::string &dst) { return rename(src, dst); }
    bool badlo(const MemeValue &src, const MemeValue &dst) { return rename(src, dst); }
};

struct SystummModule {
    SystummModule *operator->() { return this; }
    const SystummModule *operator->() const { return this; }

    MemeStringList args;

    void init(int argc, char *argv[]) {
        args.clear();
        for (int i = 0; i < argc; ++i) {
            args.push_back(argv[i] ? argv[i] : "");
        }
    }

    std::string exec(const std::string &cmd) {
        if (std::getenv("MEME_HEADLESS")) {
            return "[HEADLESS: executed " + cmd + "]";
        }
        FILE *pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            throw std::runtime_error("Systumm hang ho gaya! Command run nahi hui: " + cmd);
        }
        char buffer[256];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        pclose(pipe);
        return result;
    }
    std::string exec(const MemeValue &cmd) { return exec(cmd.to_string()); }
    std::string chala(const std::string &cmd) { return exec(cmd); }
    std::string chala(const MemeValue &cmd) { return exec(cmd); }

    std::string cwd() {
        return std::filesystem::current_path().string();
    }
    std::string kahan() { return cwd(); }

    std::string platform() {
#if defined(__EMSCRIPTEN__)
        return "wasm";
#elif defined(_WIN32)
        return "windows";
#elif defined(__APPLE__)
        return "mac";
#elif defined(__linux__)
        return "linux";
#else
        return "unknown";
#endif
    }
    std::string kaun() { return platform(); }

    void exit(int code = 0) {
        std::exit(code);
    }
    void exit(const MemeValue &code) {
        std::exit(static_cast<int>(code.as_int()));
    }
    void nikal(int code = 0) { exit(code); }
    void nikal(const MemeValue &code) { exit(code); }

    std::string env(const std::string &key, const std::string &def = "") {
        const char *v = std::getenv(key.c_str());
        return v ? std::string(v) : def;
    }
    std::string env(const MemeValue &key, const MemeValue &def = MemeValue("")) {
        return env(key.to_string(), def.to_string());
    }

    bool set_env(const std::string &key, const std::string &val) {
#ifdef _WIN32
        return _putenv_s(key.c_str(), val.c_str()) == 0;
#else
        return setenv(key.c_str(), val.c_str(), 1) == 0;
#endif
    }
    bool set_env(const MemeValue &key, const MemeValue &val) {
        return set_env(key.to_string(), val.to_string());
    }

    bool chdir(const std::string &path) {
        std::error_code ec;
        std::filesystem::current_path(path, ec);
        if (ec) {
            throw std::runtime_error("Rasta bhatak gaye! Directory change nahi ho saki: " + path);
        }
        return true;
    }
    bool chdir(const MemeValue &path) { return chdir(path.to_string()); }
    bool jao(const std::string &path) { return chdir(path); }
    bool jao(const MemeValue &path) { return chdir(path); }
};

inline FileModule file;
inline FileModule dastavez;
inline SystummModule sys;
inline SystummModule os;
inline SystummModule systumm;

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
        return val ? "true" : "false";
    } else if constexpr (std::is_same_v<std::decay_t<T>, MemeValue>) {
        if (val.type == MemeValue::TYPE_BOOL) {
            return val.b_val ? "true" : "false";
        }
        return val.to_string();
    } else if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
        return val;
    } else if constexpr (std::is_same_v<std::decay_t<T>, const char *>) {
        return std::string(val ? val : "");
    } else {
        std::ostringstream ss;
        ss << val;
        return ss.str();
    }
}

template <typename T>
inline std::string meme_to_string_print(const T &val) {
    if constexpr (std::is_same_v<std::decay_t<T>, bool>) {
        return val ? "han" : "nhi";
    } else if constexpr (std::is_same_v<std::decay_t<T>, MemeValue>) {
        return val.to_string();
    } else if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
        return val;
    } else if constexpr (std::is_same_v<std::decay_t<T>, const char *>) {
        return std::string(val ? val : "");
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
        std::cout << runtime_format_rich_text(meme_to_string_print(val));
    };
    (print_one(args), ...);
    std::cout << "\n";
}

template <typename E, typename S, typename St, typename... Args>
void print_dikha_opts(const E &endVal, const S &sepVal, const St &styleVal, const Args&... args) {
    std::string endStr = meme_to_string_print(endVal);
    std::string sepStr = meme_to_string_print(sepVal);
    std::string styleStr = meme_to_string_print(styleVal);
    std::string stylePrefix = runtime_resolve_style_ansi(styleStr);
    std::string styleReset = stylePrefix.empty() ? "" : "\033[0m";

    if (!stylePrefix.empty()) {
        std::cout << stylePrefix;
    }
    bool first = true;
    auto print_one = [&](const auto &val) {
        if (!first) std::cout << sepStr;
        first = false;
        std::cout << runtime_format_rich_text(meme_to_string_print(val));
    };
    (print_one(args), ...);
    if (!styleReset.empty()) {
        std::cout << styleReset;
    }
    std::cout << endStr;
}

#endif // MEME_RUNTIME_H
