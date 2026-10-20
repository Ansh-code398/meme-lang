#ifndef VALUE_H
#define VALUE_H

#include <string>
#include <variant>
#include <memory>
#include "Callable.h"

class Instance;
std::string instanceToString(const std::shared_ptr<Instance> &instance);

template <class... Ts>
struct overload : Ts...
{
    using Ts::operator()...;
};

template <class... Ts>
overload(Ts...) -> overload<Ts...>;

using ValueVariant = std::variant<
    std::monostate,              // kuch_nhi
    long long,                   // int
    double,                      // float
    std::string,                 // string
    bool,                        // bool (han / nhi)
    std::shared_ptr<Callable>,   // function / class / method
    std::shared_ptr<Instance>    // object instance
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

    std::string typeName() const
    {
        return std::visit(
            overload{
                [](std::monostate) -> std::string { return "nalla"; },
                [](long long) -> std::string { return "int"; },
                [](double) -> std::string { return "float"; },
                [](const std::string &) -> std::string { return "string"; },
                [](bool) -> std::string { return "bool"; },
                [](const std::shared_ptr<Callable> &) -> std::string { return "function"; },
                [](const std::shared_ptr<Instance> &) -> std::string { return "object"; }
            },
            data);
    }

    // Truthiness rule:
    // - nalla, nhi (false), 0, 0.0, and "" are falsy.
    // - All other values are truthy.
    bool isTruthy() const
    {
        return std::visit(
            overload{
                [](std::monostate) -> bool { return false; },
                [](long long v) -> bool { return v != 0; },
                [](double v) -> bool { return v != 0.0; },
                [](const std::string &v) -> bool { return !v.empty(); },
                [](bool v) -> bool { return v; },
                [](const std::shared_ptr<Callable> &v) -> bool { return v != nullptr; },
                [](const std::shared_ptr<Instance> &v) -> bool { return v != nullptr; }
            },
            data);
    }

    std::string toString() const
    {
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
                }
            },
            data);
    }

    bool operator==(const Value &other) const
    {
        return std::visit(
            overload{
                [](std::monostate, std::monostate) -> bool { return true; },
                [](long long a, long long b) -> bool { return a == b; },
                [](double a, double b) -> bool { return a == b; },
                [](long long a, double b) -> bool { return static_cast<double>(a) == b; },
                [](double a, long long b) -> bool { return a == static_cast<double>(b); },
                [](const std::string &a, const std::string &b) -> bool { return a == b; },
                [](bool a, bool b) -> bool { return a == b; },
                [](const std::shared_ptr<Callable> &a, const std::shared_ptr<Callable> &b) -> bool {
                    return a == b;
                },
                [](const std::shared_ptr<Instance> &a, const std::shared_ptr<Instance> &b) -> bool {
                    return a == b;
                },
                [](const auto &, const auto &) -> bool { return false; }
            },
            data,
            other.data);
    }

    bool operator!=(const Value &other) const
    {
        return !(*this == other);
    }
};

#endif