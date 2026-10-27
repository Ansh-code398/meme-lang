#ifndef VALUE_H
#define VALUE_H

#include <string>
#include <variant>
#include <memory>
#include <unordered_map>
#include "Callable.h"

class Instance;
class Environment;
struct MemeList;
struct MemeMap;
std::string instanceToString(const std::shared_ptr<Instance> &instance);
std::string listToString(const std::shared_ptr<MemeList> &list);
std::string mapToString(const std::shared_ptr<MemeMap> &map);

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
    long long,                   // int
    double,                      // float
    std::string,                 // string
    bool,                        // bool (han / nhi)
    std::shared_ptr<Callable>,   // function / class / method
    std::shared_ptr<Instance>,   // object instance
    std::shared_ptr<Reference>,  // udhaar / udhaar_badal (boxed for compact 32-byte Value)
    std::shared_ptr<MemeList>,   // list / array
    std::shared_ptr<MemeMap>     // map / dict / khata
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
    Value(int v) : data(static_cast<long long>(v)) {}
    Value(double v) : data(v) {}
    Value(const std::string &v) : data(v) {}
    Value(const char *v) : data(std::string(v)) {}
    Value(bool v) : data(v) {}
    Value(std::shared_ptr<Callable> v) : data(v) {}
    Value(std::shared_ptr<Instance> v) : data(v) {}
    Value(Reference v) : data(std::make_shared<Reference>(std::move(v))) {}
    Value(std::shared_ptr<Reference> v) : data(std::move(v)) {}
    Value(std::shared_ptr<MemeList> v) : data(v) {}
    Value(std::shared_ptr<MemeMap> v) : data(v) {}

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
        return std::holds_alternative<std::monostate>(data) ||
               std::holds_alternative<long long>(data) ||
               std::holds_alternative<double>(data) ||
               std::holds_alternative<bool>(data);
    }

    bool isMoveType() const
    {
        return false;
    }

    bool isInstance() const
    {
        return std::holds_alternative<std::shared_ptr<Instance>>(data);
    }

    std::shared_ptr<Instance> getInstance() const
    {
        return std::get<std::shared_ptr<Instance>>(data);
    }

    bool isList() const
    {
        return std::holds_alternative<std::shared_ptr<MemeList>>(data);
    }

    std::shared_ptr<MemeList> getList() const
    {
        return std::get<std::shared_ptr<MemeList>>(data);
    }

    bool isMap() const
    {
        return std::holds_alternative<std::shared_ptr<MemeMap>>(data);
    }

    std::shared_ptr<MemeMap> getMap() const
    {
        return std::get<std::shared_ptr<MemeMap>>(data);
    }

    long long asInt() const
    {
        Value d = deref();
        if (std::holds_alternative<long long>(d.data))
            return std::get<long long>(d.data);
        if (std::holds_alternative<double>(d.data))
            return static_cast<long long>(std::get<double>(d.data));
        return 0;
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
                [](double) -> std::string { return "float"; },
                [](const std::string &) -> std::string { return "string"; },
                [](bool) -> std::string { return "bool"; },
                [](const std::shared_ptr<Callable> &) -> std::string { return "function"; },
                [](const std::shared_ptr<Instance> &) -> std::string { return "object"; },
                [](const std::shared_ptr<Reference> &r) -> std::string { return (r && r->isMutable) ? "udhaar_badal" : "udhaar"; },
                [](const std::shared_ptr<MemeList> &) -> std::string { return "list"; },
                [](const std::shared_ptr<MemeMap> &) -> std::string { return "khata"; }
            },
            data);
    }

    // Truthiness rule:
    // - nalla, nhi (false), 0, 0.0, and "" are falsy.
    // - Empty lists [] and empty maps {} are falsy.
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
                [](double v) -> std::string { return formatFloat(v); },
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
            [](double v) -> bool { return v != 0.0; },
            [](const std::string &v) -> bool { return !v.empty(); },
            [](bool v) -> bool { return v; },
            [](const std::shared_ptr<Callable> &v) -> bool { return v != nullptr; },
            [](const std::shared_ptr<Instance> &v) -> bool { return v != nullptr; },
            [](const std::shared_ptr<Reference> &) -> bool { return true; },
            [](const std::shared_ptr<MemeList> &v) -> bool { return v != nullptr && !v->elements.empty(); },
            [](const std::shared_ptr<MemeMap> &v) -> bool { return v != nullptr && !v->entries.empty(); }
        },
        data);
}

// Compile-time invariant: Value must be compact (<= 40 bytes) for memory efficiency
static_assert(sizeof(Value) <= 40, "Value size exceeded 40 bytes!");

#endif