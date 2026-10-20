#ifndef OPERATORS_H
#define OPERATORS_H

#include <stdexcept>
#include <string>
#include <variant>
#include <limits>
#include "Value.h"

inline Value add(const Value &a, const Value &b)
{
    return std::visit(
        overload{
            [](long long lhs, long long rhs) -> Value
            {
                long long result = 0;
                if (__builtin_add_overflow(lhs, rhs, &result))
                {
                    throw std::runtime_error(
                        "25 din mein paisa double karte-karte integer overflow ho gaya! Aukaat ke bahar chala gaya jod.");
                }
                return Value(result);
            },
            [](long long lhs, double rhs) -> Value
            {
                return Value(static_cast<double>(lhs) + rhs);
            },
            [](double lhs, long long rhs) -> Value
            {
                return Value(lhs + static_cast<double>(rhs));
            },
            [](double lhs, double rhs) -> Value
            {
                return Value(lhs + rhs);
            },
            [](const std::string &lhs, const std::string &rhs) -> Value
            {
                return Value(lhs + rhs);
            },
            [&](const auto &, const auto &) -> Value
            {
                throw std::runtime_error(
                    "Ye kya bawasir bana diye ho? '" + a.typeName() +
                    "' aur '" + b.typeName() + "' ko aapas mein jod (+) nahi sakte!");
            }
        },
        a.data,
        b.data);
}

inline Value subtract(const Value &a, const Value &b)
{
    return std::visit(
        overload{
            [](long long lhs, long long rhs) -> Value
            {
                long long result = 0;
                if (__builtin_sub_overflow(lhs, rhs, &result))
                {
                    throw std::runtime_error(
                        "Ghatate-ghatate patal lok pahunch gaye! Integer underflow/overflow ho gaya.");
                }
                return Value(result);
            },
            [](long long lhs, double rhs) -> Value
            {
                return Value(static_cast<double>(lhs) - rhs);
            },
            [](double lhs, long long rhs) -> Value
            {
                return Value(lhs - static_cast<double>(rhs));
            },
            [](double lhs, double rhs) -> Value
            {
                return Value(lhs - rhs);
            },
            [&](const auto &, const auto &) -> Value
            {
                throw std::runtime_error(
                    "Zor-zabardasti mat karo! '" + a.typeName() +
                    "' mein se '" + b.typeName() + "' ko ghata (-) nahi sakte!");
            }
        },
        a.data,
        b.data);
}

inline Value multiply(const Value &a, const Value &b)
{
    return std::visit(
        overload{
            [](long long lhs, long long rhs) -> Value
            {
                long long result = 0;
                if (__builtin_mul_overflow(lhs, rhs, &result))
                {
                    throw std::runtime_error(
                        "Hisab-kitab aukaat ke bahar! Guna (*) karte-karte integer overflow ho gaya.");
                }
                return Value(result);
            },
            [](long long lhs, double rhs) -> Value
            {
                return Value(static_cast<double>(lhs) * rhs);
            },
            [](double lhs, long long rhs) -> Value
            {
                return Value(lhs * static_cast<double>(rhs));
            },
            [](double lhs, double rhs) -> Value
            {
                return Value(lhs * rhs);
            },
            [](const std::string &lhs, long long rhs) -> Value
            {
                if (rhs < 0)
                {
                    throw std::runtime_error("Delulu is not the solulu! String ko negative baar kaise multiply karoge?");
                }
                std::string res;
                for (long long i = 0; i < rhs; ++i)
                {
                    res += lhs;
                }
                return Value(res);
            },
            [](long long lhs, const std::string &rhs) -> Value
            {
                if (lhs < 0)
                {
                    throw std::runtime_error("Delulu is not the solulu! String ko negative baar kaise multiply karoge?");
                }
                std::string res;
                for (long long i = 0; i < lhs; ++i)
                {
                    res += rhs;
                }
                return Value(res);
            },
            [&](const auto &, const auto &) -> Value
            {
                throw std::runtime_error(
                    "Kuch bhi?! '" + a.typeName() +
                    "' aur '" + b.typeName() + "' ka aapas mein guna (*) nahi ho sakta!");
            }
        },
        a.data,
        b.data);
}

inline Value divide(const Value &a, const Value &b)
{
    return std::visit(
        overload{
            [](long long lhs, long long rhs) -> Value
            {
                if (rhs == 0)
                {
                    throw std::runtime_error("Kya coder banega re tu? 0 se divide karke brahmand khatam karega kya!");
                }
                if (lhs == std::numeric_limits<long long>::min() && rhs == -1)
                {
                    throw std::runtime_error("Miracle! Miracle! Division karte-karte hi integer overflow ho gaya!");
                }
                return Value(lhs / rhs);
            },
            [](long long lhs, double rhs) -> Value
            {
                if (rhs == 0.0)
                {
                    throw std::runtime_error("Kya coder banega re tu? 0 se divide karke brahmand khatam karega kya!");
                }
                return Value(static_cast<double>(lhs) / rhs);
            },
            [](double lhs, long long rhs) -> Value
            {
                if (rhs == 0)
                {
                    throw std::runtime_error("Kya coder banega re tu? 0 se divide karke brahmand khatam karega kya!");
                }
                return Value(lhs / static_cast<double>(rhs));
            },
            [](double lhs, double rhs) -> Value
            {
                if (rhs == 0.0)
                {
                    throw std::runtime_error("Kya coder banega re tu? 0 se divide karke brahmand khatam karega kya!");
                }
                return Value(lhs / rhs);
            },
            [&](const auto &, const auto &) -> Value
            {
                throw std::runtime_error(
                    "Aap apna aur mera time waste kar rahe ho! '" + a.typeName() +
                    "' aur '" + b.typeName() + "' ko divide (/) nahi kar sakte!");
            }
        },
        a.data,
        b.data);
}

inline Value modulo(const Value &a, const Value &b)
{
    return std::visit(
        overload{
            [](long long lhs, long long rhs) -> Value
            {
                if (rhs == 0)
                {
                    throw std::runtime_error("0 se modulo (%) karne chale ho? Maths teacher ko heart attack aa jayega!");
                }
                return Value(lhs % rhs);
            },
            [&](const auto &, const auto &) -> Value
            {
                throw std::runtime_error(
                    "Modulo (%) sirf bina_point (int) numbers ke beech hota hai, '" + a.typeName() +
                    "' aur '" + b.typeName() + "' pe nahi!");
            }
        },
        a.data,
        b.data);
}

inline Value negate(const Value &a)
{
    return std::visit(
        overload{
            [](long long v) -> Value
            {
                if (v == std::numeric_limits<long long>::min())
                {
                    throw std::runtime_error("Integer overflow ho gaya!");
                }
                return Value(-v);
            },
            [](double v) -> Value
            {
                return Value(-v);
            },
            [&](const auto &) -> Value
            {
                throw std::runtime_error(
                    "'" + a.typeName() + "' ke aage minus (-) kaise laga diya tune?");
            }
        },
        a.data);
}

inline Value logicalNot(const Value &a)
{
    return Value(!a.isTruthy());
}

inline Value equal(const Value &a, const Value &b)
{
    return Value(a == b);
}

inline Value notEqual(const Value &a, const Value &b)
{
    return Value(a != b);
}

inline Value lessThan(const Value &a, const Value &b)
{
    return std::visit(
        overload{
            [](long long lhs, long long rhs) -> Value { return Value(lhs < rhs); },
            [](long long lhs, double rhs) -> Value { return Value(static_cast<double>(lhs) < rhs); },
            [](double lhs, long long rhs) -> Value { return Value(lhs < static_cast<double>(rhs)); },
            [](double lhs, double rhs) -> Value { return Value(lhs < rhs); },
            [](const std::string &lhs, const std::string &rhs) -> Value { return Value(lhs < rhs); },
            [&](const auto &, const auto &) -> Value
            {
                throw std::runtime_error(
                    "Seb ko santre se mat tolo! '" + a.typeName() + "' aur '" + b.typeName() + "' ko compare (<) nahi kar sakte!");
            }
        },
        a.data,
        b.data);
}

inline Value lessThanEqual(const Value &a, const Value &b)
{
    return std::visit(
        overload{
            [](long long lhs, long long rhs) -> Value { return Value(lhs <= rhs); },
            [](long long lhs, double rhs) -> Value { return Value(static_cast<double>(lhs) <= rhs); },
            [](double lhs, long long rhs) -> Value { return Value(lhs <= static_cast<double>(rhs)); },
            [](double lhs, double rhs) -> Value { return Value(lhs <= rhs); },
            [](const std::string &lhs, const std::string &rhs) -> Value { return Value(lhs <= rhs); },
            [&](const auto &, const auto &) -> Value
            {
                throw std::runtime_error(
                    "Seb ko santre se mat tolo! '" + a.typeName() + "' aur '" + b.typeName() + "' ko compare (<=) nahi kar sakte!");
            }
        },
        a.data,
        b.data);
}

inline Value greaterThan(const Value &a, const Value &b)
{
    return std::visit(
        overload{
            [](long long lhs, long long rhs) -> Value { return Value(lhs > rhs); },
            [](long long lhs, double rhs) -> Value { return Value(static_cast<double>(lhs) > rhs); },
            [](double lhs, long long rhs) -> Value { return Value(lhs > static_cast<double>(rhs)); },
            [](double lhs, double rhs) -> Value { return Value(lhs > rhs); },
            [](const std::string &lhs, const std::string &rhs) -> Value { return Value(lhs > rhs); },
            [&](const auto &, const auto &) -> Value
            {
                throw std::runtime_error(
                    "Seb ko santre se mat tolo! '" + a.typeName() + "' aur '" + b.typeName() + "' ko compare (>) nahi kar sakte!");
            }
        },
        a.data,
        b.data);
}

inline Value greaterThanEqual(const Value &a, const Value &b)
{
    return std::visit(
        overload{
            [](long long lhs, long long rhs) -> Value { return Value(lhs >= rhs); },
            [](long long lhs, double rhs) -> Value { return Value(static_cast<double>(lhs) >= rhs); },
            [](double lhs, long long rhs) -> Value { return Value(lhs >= static_cast<double>(rhs)); },
            [](double lhs, double rhs) -> Value { return Value(lhs >= rhs); },
            [](const std::string &lhs, const std::string &rhs) -> Value { return Value(lhs >= rhs); },
            [&](const auto &, const auto &) -> Value
            {
                throw std::runtime_error(
                    "Seb ko santre se mat tolo! '" + a.typeName() + "' aur '" + b.typeName() + "' ko compare (>=) nahi kar sakte!");
            }
        },
        a.data,
        b.data);
}

#endif