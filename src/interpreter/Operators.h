#ifndef OPERATORS_H
#define OPERATORS_H

#include <stdexcept>
#include <string>
#include <variant>
#include <limits>
#include "Value.h"
#include "Environment.h"

inline Value add(const Value &a, const Value &b)
{
    Value da = a.deref();
    Value db = b.deref();
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
                    "Ye kya bawasir bana diye ho? '" + da.typeName() +
                    "' aur '" + db.typeName() + "' ko aapas mein jod (+) nahi sakte!");
            }
        },
        da.data,
        db.data);
}

inline Value subtract(const Value &a, const Value &b)
{
    Value da = a.deref();
    Value db = b.deref();
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
                    "Zor-zabardasti mat karo! '" + da.typeName() +
                    "' mein se '" + db.typeName() + "' ko ghata (-) nahi sakte!");
            }
        },
        da.data,
        db.data);
}

inline Value multiply(const Value &a, const Value &b)
{
    Value da = a.deref();
    Value db = b.deref();
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
                    throw std::runtime_error(
                        "String ko negative baar guna (*) karne chale ho? Time machine mein piche jaana hai kya?");
                }
                if (rhs == 0)
                {
                    return Value("");
                }
                if (lhs.empty())
                {
                    return Value("");
                }

                if (static_cast<unsigned long long>(rhs) > 1000000000ULL / lhs.length())
                {
                    throw std::runtime_error("Itna lamba string multiply karoge toh RAM dharamsala thodi hai jo sab jhel lega!");
                }

                std::string result;
                result.reserve(lhs.length() * rhs);
                for (long long i = 0; i < rhs; ++i)
                {
                    result += lhs;
                }
                return Value(result);
            },
            [](long long lhs, const std::string &rhs) -> Value
            {
                if (lhs < 0)
                {
                    throw std::runtime_error(
                        "String ko negative baar guna (*) karne chale ho? Time machine mein piche jaana hai kya?");
                }
                if (lhs == 0)
                {
                    return Value("");
                }
                if (rhs.empty())
                {
                    return Value("");
                }

                if (static_cast<unsigned long long>(lhs) > 1000000000ULL / rhs.length())
                {
                    throw std::runtime_error("Itna lamba string multiply karoge toh RAM dharamsala thodi hai jo sab jhel lega!");
                }

                std::string result;
                result.reserve(rhs.length() * lhs);
                for (long long i = 0; i < lhs; ++i)
                {
                    result += rhs;
                }
                return Value(result);
            },
            [&](const auto &, const auto &) -> Value
            {
                throw std::runtime_error(
                    "Ye kya hisaab hai? '" + da.typeName() +
                    "' ko '" + db.typeName() + "' se guna (*) nahi kar sakte!");
            }
        },
        da.data,
        db.data);
}

inline Value divide(const Value &a, const Value &b)
{
    Value da = a.deref();
    Value db = b.deref();
    return std::visit(
        overload{
            [](long long lhs, long long rhs) -> Value
            {
                if (rhs == 0)
                {
                    throw std::runtime_error(
                        "Arre re re! 0 se divide karoge toh Brahmaand phat jayega! Zero division mana hai!");
                }
                if (lhs == std::numeric_limits<long long>::min() && rhs == -1)
                {
                    throw std::runtime_error(
                        "Integer overflow ho gaya! Is hisaab ko sambhalne ki aukaat nahi hai.");
                }
                return Value(lhs / rhs);
            },
            [](long long lhs, double rhs) -> Value
            {
                if (rhs == 0.0)
                {
                    throw std::runtime_error(
                        "Arre re re! 0 se divide karoge toh Brahmaand phat jayega! Zero division mana hai!");
                }
                return Value(static_cast<double>(lhs) / rhs);
            },
            [](double lhs, long long rhs) -> Value
            {
                if (rhs == 0)
                {
                    throw std::runtime_error(
                        "Arre re re! 0 se divide karoge toh Brahmaand phat jayega! Zero division mana hai!");
                }
                return Value(lhs / static_cast<double>(rhs));
            },
            [](double lhs, double rhs) -> Value
            {
                if (rhs == 0.0)
                {
                    throw std::runtime_error(
                        "Arre re re! 0 se divide karoge toh Brahmaand phat jayega! Zero division mana hai!");
                }
                return Value(lhs / rhs);
            },
            [&](const auto &, const auto &) -> Value
            {
                throw std::runtime_error(
                    "Aukaat ke bahar! '" + da.typeName() +
                    "' ko '" + db.typeName() + "' se divide (/) nahi kar sakte!");
            }
        },
        da.data,
        db.data);
}

inline Value modulo(const Value &a, const Value &b)
{
    Value da = a.deref();
    Value db = b.deref();
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
                    "Modulo (%) sirf bina_point (int) numbers ke beech hota hai, '" + da.typeName() +
                    "' aur '" + db.typeName() + "' pe nahi!");
            }
        },
        da.data,
        db.data);
}

inline Value negate(const Value &a)
{
    Value da = a.deref();
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
                    "'" + da.typeName() + "' ke aage minus (-) kaise laga diya tune?");
            }
        },
        da.data);
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
    Value da = a.deref();
    Value db = b.deref();
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
                    "Seb ko santre se mat tolo! '" + da.typeName() + "' aur '" + db.typeName() + "' ko compare (<) nahi kar sakte!");
            }
        },
        da.data,
        db.data);
}

inline Value lessThanEqual(const Value &a, const Value &b)
{
    Value da = a.deref();
    Value db = b.deref();
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
                    "Seb ko santre se mat tolo! '" + da.typeName() + "' aur '" + db.typeName() + "' ko compare (<=) nahi kar sakte!");
            }
        },
        da.data,
        db.data);
}

inline Value greaterThan(const Value &a, const Value &b)
{
    Value da = a.deref();
    Value db = b.deref();
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
                    "Seb ko santre se mat tolo! '" + da.typeName() + "' aur '" + db.typeName() + "' ko compare (>) nahi kar sakte!");
            }
        },
        da.data,
        db.data);
}

inline Value greaterThanEqual(const Value &a, const Value &b)
{
    Value da = a.deref();
    Value db = b.deref();
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
                    "Seb ko santre se mat tolo! '" + da.typeName() + "' aur '" + db.typeName() + "' ko compare (>=) nahi kar sakte!");
            }
        },
        da.data,
        db.data);
}

#endif
