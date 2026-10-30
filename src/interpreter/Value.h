#ifndef VALUE_H
#define VALUE_H

#include <string>
#include <variant>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "Callable.h"

class Instance;
class Environment;
struct MemeList;
struct MemeMap;
struct MemeBytes;
struct MemeSet;
struct MemeTuple;
std::string instanceToString(const std::shared_ptr<Instance> &instance);
std::string listToString(const std::shared_ptr<MemeList> &list);
std::string mapToString(const std::shared_ptr<MemeMap> &map);
std::string bytesToString(const std::shared_ptr<MemeBytes> &bytes);
std::string setToString(const std::shared_ptr<MemeSet> &set);
std::string tupleToString(const std::shared_ptr<MemeTuple> &tuple);

template <class... Ts>
struct overload : Ts...
{
    using Ts::operator()...;
};

template <class... Ts>
overload(Ts...) -> overload<Ts...>;

struct Reference
{
    std::string varName;
    std::shared_ptr<Environment> env;
    bool isMutable = false;
    int scopeDepth = 0;
    uint64_t borrowId = 0;
};

using ValueVariant = std::variant<
    std::monostate,              // kuch_nhi / nalla
    long long,                   // int / long
    int,                         // int32
    uint8_t,                     // byte / u8
    double,                      // float / double
    float,                       // float32
    char,                        // char
    std::string,                 // string
    bool,                        // bool (han / nhi)
    std::shared_ptr<Callable>,   // function / class / method
    std::shared_ptr<Instance>,   // object instance
    std::shared_ptr<Reference>,  // udhaar / udhaar_badal (boxed for compact 32-byte Value)
    std::shared_ptr<MemeList>,   // list / array
    std::shared_ptr<MemeMap>,    // map / dict / khata
    std::shared_ptr<MemeBytes>,  // raw bytes buffer
    std::shared_ptr<MemeSet>,    // set / jhund
    std::shared_ptr<MemeTuple>   // tuple / jodi
>;

inline std::string formatFloat(double v)
{
    std::string s = std::to_string(v);
    if (s.find('.') != std::string::npos)
    {
        while (s.size() > 1 && s.back() == '0')
        {
            s.pop_back();
        }
        if (s.back() == '.')
        {
            s.push_back('0');
        }
    }
    return s;
}

struct Value
{
    ValueVariant data;

    // Constructors
    Value() : data(std::monostate{}) {}
    Value(std::monostate) : data(std::monostate{}) {}
    Value(long long v) : data(v) {}
    Value(int v) : data(v) {}
    Value(uint8_t v) : data(v) {}
    Value(unsigned long v) : data(static_cast<long long>(v)) {}
    Value(unsigned int v) : data(static_cast<int>(v)) {}
    Value(double v) : data(v) {}
    Value(float v) : data(v) {}
    Value(char v) : data(v) {}
    Value(const std::string &v) : data(v) {}
    Value(const char *v) : data(std::string(v)) {}
    Value(bool v) : data(v) {}
    Value(std::shared_ptr<Callable> v) : data(std::move(v)) {}
    Value(std::shared_ptr<Instance> v) : data(std::move(v)) {}
    Value(Reference v) : data(std::make_shared<Reference>(std::move(v))) {}
    Value(std::shared_ptr<Reference> v) : data(std::move(v)) {}
    Value(std::shared_ptr<MemeList> v) : data(std::move(v)) {}
    Value(std::shared_ptr<MemeMap> v) : data(std::move(v)) {}
    Value(std::shared_ptr<MemeBytes> v) : data(std::move(v)) {}
    Value(std::shared_ptr<MemeSet> v) : data(std::move(v)) {}
    Value(std::shared_ptr<MemeTuple> v) : data(std::move(v)) {}

    bool isReference() const
    {
        return std::holds_alternative<std::shared_ptr<Reference>>(data);
    }

    bool isNil() const
    {
        return std::holds_alternative<std::monostate>(deref().data);
    }

    bool isNull() const
    {
        return isNil();
    }

    const Reference &getReference() const
    {
        return *std::get<std::shared_ptr<Reference>>(data);
    }

    Reference &getReference()
    {
        return *std::get<std::shared_ptr<Reference>>(data);
    }

    bool isPrimitive() const
    {
        Value d = deref();
        return std::holds_alternative<std::monostate>(d.data) ||
               std::holds_alternative<long long>(d.data) ||
               std::holds_alternative<int>(d.data) ||
               std::holds_alternative<uint8_t>(d.data) ||
               std::holds_alternative<double>(d.data) ||
               std::holds_alternative<float>(d.data) ||
               std::holds_alternative<char>(d.data) ||
               std::holds_alternative<bool>(d.data);
    }

    bool isMoveType() const
    {
        return false;
    }

    bool isInstance() const
    {
        return std::holds_alternative<std::shared_ptr<Instance>>(deref().data);
    }

    std::shared_ptr<Instance> getInstance() const
    {
        return std::get<std::shared_ptr<Instance>>(deref().data);
    }

    bool isList() const
    {
        return std::holds_alternative<std::shared_ptr<MemeList>>(deref().data);
    }

    std::shared_ptr<MemeList> getList() const
    {
        return std::get<std::shared_ptr<MemeList>>(deref().data);
    }

    bool isMap() const
    {
        return std::holds_alternative<std::shared_ptr<MemeMap>>(deref().data);
    }

    std::shared_ptr<MemeMap> getMap() const
    {
        return std::get<std::shared_ptr<MemeMap>>(deref().data);
    }

    bool isBytes() const
    {
        return std::holds_alternative<std::shared_ptr<MemeBytes>>(deref().data);
    }

    std::shared_ptr<MemeBytes> getBytes() const
    {
        return std::get<std::shared_ptr<MemeBytes>>(deref().data);
    }

    bool isSet() const
    {
        return std::holds_alternative<std::shared_ptr<MemeSet>>(deref().data);
    }

    std::shared_ptr<MemeSet> getSet() const
    {
        return std::get<std::shared_ptr<MemeSet>>(deref().data);
    }

    bool isTuple() const
    {
        return std::holds_alternative<std::shared_ptr<MemeTuple>>(deref().data);
    }

    std::shared_ptr<MemeTuple> getTuple() const
    {
        return std::get<std::shared_ptr<MemeTuple>>(deref().data);
    }

    bool isByte() const
    {
        return std::holds_alternative<uint8_t>(deref().data);
    }

    bool isInt32() const
    {
        return std::holds_alternative<int>(deref().data);
    }

    bool isInt64() const
    {
        return std::holds_alternative<long long>(deref().data);
    }

    bool isFloat() const
    {
        return std::holds_alternative<float>(deref().data);
    }

    bool isDouble() const
    {
        return std::holds_alternative<double>(deref().data);
    }

    bool isChar() const
    {
        return std::holds_alternative<char>(deref().data);
    }

    bool isString() const
    {
        return std::holds_alternative<std::string>(deref().data);
    }

    bool isBool() const
    {
        return std::holds_alternative<bool>(deref().data);
    }

    bool isInteger() const
    {
        Value d = deref();
        return std::holds_alternative<long long>(d.data) ||
               std::holds_alternative<int>(d.data) ||
               std::holds_alternative<uint8_t>(d.data) ||
               std::holds_alternative<char>(d.data);
    }

    bool isNumeric() const
    {
        Value d = deref();
        return isInteger() ||
               std::holds_alternative<double>(d.data) ||
               std::holds_alternative<float>(d.data);
    }

    bool isCallable() const
    {
        Value d = deref();
        return std::holds_alternative<std::shared_ptr<Callable>>(d.data);
    }

    std::shared_ptr<Callable> getCallable() const
    {
        Value d = deref();
        return std::get<std::shared_ptr<Callable>>(d.data);
    }

    long long asInt() const
    {
        Value d = deref();
        if (std::holds_alternative<long long>(d.data))
            return std::get<long long>(d.data);
        if (std::holds_alternative<int>(d.data))
            return static_cast<long long>(std::get<int>(d.data));
        if (std::holds_alternative<uint8_t>(d.data))
            return static_cast<long long>(std::get<uint8_t>(d.data));
        if (std::holds_alternative<char>(d.data))
            return static_cast<long long>(static_cast<unsigned char>(std::get<char>(d.data)));
        if (std::holds_alternative<double>(d.data))
            return static_cast<long long>(std::get<double>(d.data));
        if (std::holds_alternative<float>(d.data))
            return static_cast<long long>(std::get<float>(d.data));
        if (std::holds_alternative<bool>(d.data))
            return std::get<bool>(d.data) ? 1 : 0;
        return 0;
    }

    int asInt32() const
    {
        return static_cast<int>(asInt());
    }

    uint8_t asByte() const
    {
        return static_cast<uint8_t>(asInt() & 0xFF);
    }

    double asDouble() const
    {
        Value d = deref();
        if (std::holds_alternative<double>(d.data))
            return std::get<double>(d.data);
        if (std::holds_alternative<float>(d.data))
            return static_cast<double>(std::get<float>(d.data));
        if (std::holds_alternative<long long>(d.data))
            return static_cast<double>(std::get<long long>(d.data));
        if (std::holds_alternative<int>(d.data))
            return static_cast<double>(std::get<int>(d.data));
        if (std::holds_alternative<uint8_t>(d.data))
            return static_cast<double>(std::get<uint8_t>(d.data));
        if (std::holds_alternative<char>(d.data))
            return static_cast<double>(static_cast<unsigned char>(std::get<char>(d.data)));
        if (std::holds_alternative<bool>(d.data))
            return std::get<bool>(d.data) ? 1.0 : 0.0;
        return 0.0;
    }

    float asFloat() const
    {
        return static_cast<float>(asDouble());
    }

    char asChar() const
    {
        Value d = deref();
        if (std::holds_alternative<char>(d.data))
            return std::get<char>(d.data);
        if (std::holds_alternative<std::string>(d.data))
        {
            const auto &s = std::get<std::string>(d.data);
            return s.empty() ? '\0' : s[0];
        }
        return static_cast<char>(asInt() & 0xFF);
    }

    Value deref() const;

    std::string typeName() const
    {
        if (isReference())
        {
            return deref().typeName();
        }
        return std::visit(
            overload{
                [](std::monostate) -> std::string { return "nalla"; },
                [](long long) -> std::string { return "int"; },
                [](int) -> std::string { return "int32"; },
                [](uint8_t) -> std::string { return "byte"; },
                [](double) -> std::string { return "float"; },
                [](float) -> std::string { return "float32"; },
                [](char) -> std::string { return "char"; },
                [](const std::string &) -> std::string { return "string"; },
                [](bool) -> std::string { return "bool"; },
                [](const std::shared_ptr<Callable> &) -> std::string { return "function"; },
                [](const std::shared_ptr<Instance> &) -> std::string { return "object"; },
                [](const std::shared_ptr<Reference> &r) -> std::string { return (r && r->isMutable) ? "udhaar_badal" : "udhaar"; },
                [](const std::shared_ptr<MemeList> &) -> std::string { return "list"; },
                [](const std::shared_ptr<MemeMap> &) -> std::string { return "khata"; },
                [](const std::shared_ptr<MemeBytes> &) -> std::string { return "bytes"; },
                [](const std::shared_ptr<MemeSet> &) -> std::string { return "set"; },
                [](const std::shared_ptr<MemeTuple> &) -> std::string { return "tuple"; }
            },
            data);
    }

    // Truthiness rule:
    // - nalla, nhi (false), 0, 0.0, and "" are falsy.
    // - Empty lists [], empty maps {}, empty bytes b"", empty sets, empty tuples are falsy.
    // - All other values are truthy.
    bool isTruthy() const;

    std::string toString() const
    {
        if (isReference())
        {
            return deref().toString();
        }
        return std::visit(
            overload{
                [](std::monostate) -> std::string { return "nalla"; },
                [](long long v) -> std::string { return std::to_string(v); },
                [](int v) -> std::string { return std::to_string(v); },
                [](uint8_t v) -> std::string { return std::to_string(static_cast<int>(v)); },
                [](double v) -> std::string { return formatFloat(v); },
                [](float v) -> std::string { return formatFloat(static_cast<double>(v)); },
                [](char v) -> std::string { return std::string(1, v); },
                [](const std::string &v) -> std::string { return v; },
                [](bool v) -> std::string { return v ? "han" : "nhi"; },
                [](const std::shared_ptr<Callable> &v) -> std::string {
                    return v ? v->toString() : "<function>";
                },
                [](const std::shared_ptr<Instance> &v) -> std::string {
                    return instanceToString(v);
                },
                [](const std::shared_ptr<Reference> &) -> std::string {
                    return "<reference>";
                },
                [](const std::shared_ptr<MemeList> &v) -> std::string {
                    return listToString(v);
                },
                [](const std::shared_ptr<MemeMap> &v) -> std::string {
                    return mapToString(v);
                },
                [](const std::shared_ptr<MemeBytes> &v) -> std::string {
                    return bytesToString(v);
                },
                [](const std::shared_ptr<MemeSet> &v) -> std::string {
                    return setToString(v);
                },
                [](const std::shared_ptr<MemeTuple> &v) -> std::string {
                    return tupleToString(v);
                }
            },
            data);
    }

    bool operator==(const Value &other) const;

    bool operator!=(const Value &other) const
    {
        return !(*this == other);
    }
};

struct MemeList
{
    std::vector<Value> elements;
    MemeList() = default;
    MemeList(std::vector<Value> elements) : elements(std::move(elements)) {}
};

struct MemeMap
{
    std::unordered_map<std::string, Value> entries;
    std::vector<std::string> keyOrder;
    bool isModule = false;
    MemeMap() = default;

    void set(const std::string &key, Value val)
    {
        if (entries.find(key) == entries.end())
        {
            keyOrder.push_back(key);
        }
        entries[key] = std::move(val);
    }

    bool remove(const std::string &key)
    {
        auto it = entries.find(key);
        if (it == entries.end())
            return false;
        entries.erase(it);
        for (auto kit = keyOrder.begin(); kit != keyOrder.end(); ++kit)
        {
            if (*kit == key)
            {
                keyOrder.erase(kit);
                break;
            }
        }
        return true;
    }

    bool has(const std::string &key) const
    {
        return entries.find(key) != entries.end();
    }

    Value get(const std::string &key) const
    {
        auto it = entries.find(key);
        if (it != entries.end())
            return it->second;
        return Value(std::monostate{});
    }
};

struct MemeBytes
{
    std::vector<uint8_t> data;
    MemeBytes() = default;
    MemeBytes(std::vector<uint8_t> d) : data(std::move(d)) {}
    MemeBytes(size_t sz, uint8_t init = 0) : data(sz, init) {}
};

struct MemeTuple
{
    std::vector<Value> elements;
    MemeTuple() = default;
    MemeTuple(std::vector<Value> elems) : elements(std::move(elems)) {}
};

struct MemeSet
{
    std::vector<Value> elements;
    std::unordered_set<std::string> keys;

    MemeSet() = default;

    std::string makeKey(const Value &val) const
    {
        Value d = val.deref();
        if (d.isNumeric())
        {
            if (d.isInteger())
            {
                return "int:" + std::to_string(d.asInt());
            }
            double dbl = d.asDouble();
            if (static_cast<double>(static_cast<long long>(dbl)) == dbl)
            {
                return "int:" + std::to_string(static_cast<long long>(dbl));
            }
            return "float:" + std::to_string(dbl);
        }
        if (std::holds_alternative<std::string>(d.data))
        {
            return "str:" + std::get<std::string>(d.data);
        }
        if (std::holds_alternative<bool>(d.data))
        {
            return std::get<bool>(d.data) ? "bool:true" : "bool:false";
        }
        return d.typeName() + ":" + d.toString();
    }

    bool add(const Value &val)
    {
        std::string k = makeKey(val);
        if (keys.find(k) != keys.end())
        {
            return false;
        }
        keys.insert(k);
        elements.push_back(val);
        return true;
    }

    bool contains(const Value &val) const
    {
        std::string k = makeKey(val);
        return keys.find(k) != keys.end();
    }

    bool remove(const Value &val)
    {
        std::string k = makeKey(val);
        auto it = keys.find(k);
        if (it == keys.end())
        {
            return false;
        }
        keys.erase(it);
        for (auto eit = elements.begin(); eit != elements.end(); ++eit)
        {
            if (*eit == val)
            {
                elements.erase(eit);
                break;
            }
        }
        return true;
    }

    void clear()
    {
        elements.clear();
        keys.clear();
    }
};

inline bool Value::isTruthy() const
{
    if (isReference())
    {
        return deref().isTruthy();
    }
    return std::visit(
        overload{
            [](std::monostate) -> bool { return false; },
            [](long long v) -> bool { return v != 0; },
            [](int v) -> bool { return v != 0; },
            [](uint8_t v) -> bool { return v != 0; },
            [](double v) -> bool { return v != 0.0; },
            [](float v) -> bool { return v != 0.0f; },
            [](char v) -> bool { return v != 0; },
            [](const std::string &v) -> bool { return !v.empty(); },
            [](bool v) -> bool { return v; },
            [](const std::shared_ptr<Callable> &v) -> bool { return v != nullptr; },
            [](const std::shared_ptr<Instance> &v) -> bool { return v != nullptr; },
            [](const std::shared_ptr<Reference> &) -> bool { return true; },
            [](const std::shared_ptr<MemeList> &v) -> bool { return v != nullptr && !v->elements.empty(); },
            [](const std::shared_ptr<MemeMap> &v) -> bool { return v != nullptr && !v->entries.empty(); },
            [](const std::shared_ptr<MemeBytes> &v) -> bool { return v != nullptr && !v->data.empty(); },
            [](const std::shared_ptr<MemeSet> &v) -> bool { return v != nullptr && !v->elements.empty(); },
            [](const std::shared_ptr<MemeTuple> &v) -> bool { return v != nullptr && !v->elements.empty(); }
        },
        data);
}

// Compile-time invariant: Value must be compact (<= 40 bytes) and exactly 32 bytes on 64-bit systems
static_assert(sizeof(Value) <= 40, "Value size exceeded 40 bytes!");
#if defined(__EMSCRIPTEN__) || defined(__wasm32__) || (INTPTR_MAX == INT32_MAX)
static_assert(sizeof(Value) == 24, "Value size must be 24 bytes on 32-bit systems!");
#else
static_assert(sizeof(Value) == 32, "Value size must be 32 bytes on 64-bit systems!");
#endif

#endif