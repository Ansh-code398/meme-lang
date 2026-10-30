#ifndef OPERATORS_H
#define OPERATORS_H

#include <stdexcept>
#include <string>
#include <variant>
#include <limits>
#include <algorithm>
#include "Value.h"
#include "Environment.h"

inline Value add(const Value &a, const Value &b)
{
    Value da = a.deref();
    Value db = b.deref();

    // Fast Numeric Dispatch
    if (da.isNumeric() && db.isNumeric())
    {
        if (da.isInteger() && db.isInteger())
        {
            // Byte fast path
            if (da.isByte() && db.isByte())
            {
                return Value(static_cast<uint8_t>(da.asByte() + db.asByte()));
            }
            // 32-bit int fast path
            if (da.isInt32() && db.isInt32())
            {
                int res32 = 0;
                if (!__builtin_add_overflow(da.asInt32(), db.asInt32(), &res32))
                {
                    return Value(res32);
                }
                return Value(static_cast<long long>(da.asInt32()) + db.asInt32());
            }
            // 64-bit int path
            long long lhs = da.asInt();
            long long rhs = db.asInt();
            long long result = 0;
            if (__builtin_add_overflow(lhs, rhs, &result))
            {
                throw std::runtime_error(
                    "25 din mein paisa double karte-karte integer overflow ho gaya! Aukaat ke bahar chala gaya jod.");
            }
            return Value(result);
        }
        // Float32 fast path
        if (da.isFloat() && db.isFloat())
        {
            return Value(da.asFloat() + db.asFloat());
        }
        return Value(da.asDouble() + db.asDouble());
    }

    // String concatenation
    if (std::holds_alternative<std::string>(da.data) && std::holds_alternative<std::string>(db.data))
    {
        return Value(std::get<std::string>(da.data) + std::get<std::string>(db.data));
    }

    // Bytes concatenation
    if (da.isBytes() && db.isBytes())
    {
        auto ba = da.getBytes();
        auto bb = db.getBytes();
        std::vector<uint8_t> merged = ba ? ba->data : std::vector<uint8_t>{};
        if (bb)
        {
            merged.insert(merged.end(), bb->data.begin(), bb->data.end());
        }
        return Value(std::make_shared<MemeBytes>(std::move(merged)));
    }

    // Tuple concatenation
    if (da.isTuple() && db.isTuple())
    {
        auto ta = da.getTuple();
        auto tb = db.getTuple();
        std::vector<Value> merged = ta ? ta->elements : std::vector<Value>{};
        if (tb)
        {
            merged.insert(merged.end(), tb->elements.begin(), tb->elements.end());
        }
        return Value(std::make_shared<MemeTuple>(std::move(merged)));
    }

    // List concatenation
    if (da.isList() && db.isList())
    {
        auto la = da.getList();
        auto lb = db.getList();
        std::vector<Value> merged = la ? la->elements : std::vector<Value>{};
        if (lb)
        {
            merged.insert(merged.end(), lb->elements.begin(), lb->elements.end());
        }
        return Value(std::make_shared<MemeList>(std::move(merged)));
    }

    // Set union
    if (da.isSet() && db.isSet())
    {
        auto sa = da.getSet();
        auto sb = db.getSet();
        auto res = std::make_shared<MemeSet>(sa ? *sa : MemeSet{});
        if (sb)
        {
            for (const auto &el : sb->elements)
            {
                res->add(el);
            }
        }
        return Value(res);
    }

    throw std::runtime_error(
        "Ye kya bawasir bana diye ho? '" + da.typeName() +
        "' aur '" + db.typeName() + "' ko aapas mein jod (+) nahi sakte!");
}

inline Value subtract(const Value &a, const Value &b)
{
    Value da = a.deref();
    Value db = b.deref();

    if (da.isNumeric() && db.isNumeric())
    {
        if (da.isInteger() && db.isInteger())
        {
            if (da.isByte() && db.isByte())
            {
                return Value(static_cast<uint8_t>(da.asByte() - db.asByte()));
            }
            if (da.isInt32() && db.isInt32())
            {
                int res32 = 0;
                if (!__builtin_sub_overflow(da.asInt32(), db.asInt32(), &res32))
                {
                    return Value(res32);
                }
                return Value(static_cast<long long>(da.asInt32()) - db.asInt32());
            }
            long long lhs = da.asInt();
            long long rhs = db.asInt();
            long long result = 0;
            if (__builtin_sub_overflow(lhs, rhs, &result))
            {
                throw std::runtime_error(
                    "Ghatate-ghatate patal lok pahunch gaye! Integer underflow/overflow ho gaya.");
            }
            return Value(result);
        }
        if (da.isFloat() && db.isFloat())
        {
            return Value(da.asFloat() - db.asFloat());
        }
        return Value(da.asDouble() - db.asDouble());
    }

    // Set difference
    if (da.isSet() && db.isSet())
    {
        auto sa = da.getSet();
        auto sb = db.getSet();
        auto res = std::make_shared<MemeSet>();
        if (sa)
        {
            for (const auto &el : sa->elements)
            {
                if (!sb || !sb->contains(el))
                {
                    res->add(el);
                }
            }
        }
        return Value(res);
    }

    throw std::runtime_error(
        "Zor-zabardasti mat karo! '" + da.typeName() +
        "' mein se '" + db.typeName() + "' ko ghata (-) nahi sakte!");
}

inline Value multiply(const Value &a, const Value &b)
{
    Value da = a.deref();
    Value db = b.deref();

    if (da.isNumeric() && db.isNumeric())
    {
        if (da.isInteger() && db.isInteger())
        {
            if (da.isByte() && db.isByte())
            {
                return Value(static_cast<uint8_t>(da.asByte() * db.asByte()));
            }
            if (da.isInt32() && db.isInt32())
            {
                int res32 = 0;
                if (!__builtin_mul_overflow(da.asInt32(), db.asInt32(), &res32))
                {
                    return Value(res32);
                }
                return Value(static_cast<long long>(da.asInt32()) * db.asInt32());
            }
            long long lhs = da.asInt();
            long long rhs = db.asInt();
            long long result = 0;
            if (__builtin_mul_overflow(lhs, rhs, &result))
            {
                throw std::runtime_error(
                    "Hisab-kitab aukaat ke bahar! Guna (*) karte-karte integer overflow ho gaya.");
            }
            return Value(result);
        }
        if (da.isFloat() && db.isFloat())
        {
            return Value(da.asFloat() * db.asFloat());
        }
        return Value(da.asDouble() * db.asDouble());
    }

    // String repetition: string * int or int * string
    auto repeatString = [](const std::string &lhs, long long rhs) -> Value {
        if (rhs < 0)
        {
            throw std::runtime_error(
                "String ko negative baar guna (*) karne chale ho? Time machine mein piche jaana hai kya?");
        }
        if (rhs == 0 || lhs.empty())
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
    };

    if (std::holds_alternative<std::string>(da.data) && db.isInteger())
    {
        return repeatString(std::get<std::string>(da.data), db.asInt());
    }
    if (da.isInteger() && std::holds_alternative<std::string>(db.data))
    {
        return repeatString(std::get<std::string>(db.data), da.asInt());
    }

    // Bytes repetition: bytes * int or int * bytes
    auto repeatBytes = [](const std::shared_ptr<MemeBytes> &b, long long times) -> Value {
        if (times < 0) throw std::runtime_error("Bytes ko negative baar guna (*) nahi kar sakte!");
        auto res = std::make_shared<MemeBytes>();
        if (!b || times == 0) return Value(res);
        res->data.reserve(b->data.size() * times);
        for (long long i = 0; i < times; ++i)
        {
            res->data.insert(res->data.end(), b->data.begin(), b->data.end());
        }
        return Value(res);
    };
    if (da.isBytes() && db.isInteger()) return repeatBytes(da.getBytes(), db.asInt());
    if (da.isInteger() && db.isBytes()) return repeatBytes(db.getBytes(), da.asInt());

    // List repetition: list * int or int * list
    auto repeatList = [](const std::shared_ptr<MemeList> &l, long long times) -> Value {
        if (times < 0) throw std::runtime_error("List ko negative baar guna (*) nahi kar sakte!");
        auto res = std::make_shared<MemeList>();
        if (!l || times == 0) return Value(res);
        res->elements.reserve(l->elements.size() * times);
        for (long long i = 0; i < times; ++i)
        {
            res->elements.insert(res->elements.end(), l->elements.begin(), l->elements.end());
        }
        return Value(res);
    };
    if (da.isList() && db.isInteger()) return repeatList(da.getList(), db.asInt());
    if (da.isInteger() && db.isList()) return repeatList(db.getList(), da.asInt());

    // Tuple repetition: tuple * int or int * tuple
    auto repeatTuple = [](const std::shared_ptr<MemeTuple> &t, long long times) -> Value {
        if (times < 0) throw std::runtime_error("Tuple ko negative baar guna (*) nahi kar sakte!");
        auto res = std::make_shared<MemeTuple>();
        if (!t || times == 0) return Value(res);
        res->elements.reserve(t->elements.size() * times);
        for (long long i = 0; i < times; ++i)
        {
            res->elements.insert(res->elements.end(), t->elements.begin(), t->elements.end());
        }
        return Value(res);
    };
    if (da.isTuple() && db.isInteger()) return repeatTuple(da.getTuple(), db.asInt());
    if (da.isInteger() && db.isTuple()) return repeatTuple(db.getTuple(), da.asInt());

    throw std::runtime_error(
        "Ye kya hisaab hai? '" + da.typeName() +
        "' ko '" + db.typeName() + "' se guna (*) nahi kar sakte!");
}

inline Value divide(const Value &a, const Value &b)
{
    Value da = a.deref();
    Value db = b.deref();

    if (da.isNumeric() && db.isNumeric())
    {
        if (da.isInteger() && db.isInteger())
        {
            long long rhs = db.asInt();
            if (rhs == 0)
            {
                throw std::runtime_error(
                    "Arre re re! 0 se divide karoge toh Brahmaand phat jayega! Zero division mana hai!");
            }
            long long lhs = da.asInt();
            if (lhs == std::numeric_limits<long long>::min() && rhs == -1)
            {
                throw std::runtime_error(
                    "Integer overflow ho gaya! Is hisaab ko sambhalne ki aukaat nahi hai.");
            }
            return Value(lhs / rhs);
        }
        double rhs = db.asDouble();
        if (rhs == 0.0)
        {
            throw std::runtime_error(
                "Arre re re! 0 se divide karoge toh Brahmaand phat jayega! Zero division mana hai!");
        }
        return Value(da.asDouble() / rhs);
    }

    throw std::runtime_error(
        "Aukaat ke bahar! '" + da.typeName() +
        "' ko '" + db.typeName() + "' se divide (/) nahi kar sakte!");
}

inline Value modulo(const Value &a, const Value &b)
{
    Value da = a.deref();
    Value db = b.deref();

    if (da.isInteger() && db.isInteger())
    {
        long long rhs = db.asInt();
        if (rhs == 0)
        {
            throw std::runtime_error("0 se modulo (%) karne chale ho? Maths teacher ko heart attack aa jayega!");
        }
        long long lhs = da.asInt();
        return Value(lhs % rhs);
    }

    throw std::runtime_error(
        "Modulo (%) sirf bina_point (int) numbers ke beech hota hai, '" + da.typeName() +
        "' aur '" + db.typeName() + "' pe nahi!");
}

inline Value negate(const Value &a)
{
    Value da = a.deref();
    if (da.isInteger())
    {
        long long v = da.asInt();
        if (v == std::numeric_limits<long long>::min())
        {
            throw std::runtime_error("Integer overflow ho gaya!");
        }
        if (da.isInt32())
        {
            return Value(-da.asInt32());
        }
        return Value(-v);
    }
    if (da.isNumeric())
    {
        if (da.isFloat())
        {
            return Value(-da.asFloat());
        }
        return Value(-da.asDouble());
    }

    throw std::runtime_error(
        "'" + da.typeName() + "' ke aage minus (-) kaise laga diya tune?");
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
    if (da.isNumeric() && db.isNumeric())
    {
        if (da.isInteger() && db.isInteger())
        {
            return Value(da.asInt() < db.asInt());
        }
        return Value(da.asDouble() < db.asDouble());
    }
    if (std::holds_alternative<std::string>(da.data) && std::holds_alternative<std::string>(db.data))
    {
        return Value(std::get<std::string>(da.data) < std::get<std::string>(db.data));
    }
    if (da.isBytes() && db.isBytes())
    {
        return Value(da.getBytes()->data < db.getBytes()->data);
    }
    throw std::runtime_error(
        "Seb ko santre se mat tolo! '" + da.typeName() + "' aur '" + db.typeName() + "' ko compare (<) nahi kar sakte!");
}

inline Value lessThanEqual(const Value &a, const Value &b)
{
    Value da = a.deref();
    Value db = b.deref();
    if (da.isNumeric() && db.isNumeric())
    {
        if (da.isInteger() && db.isInteger())
        {
            return Value(da.asInt() <= db.asInt());
        }
        return Value(da.asDouble() <= db.asDouble());
    }
    if (std::holds_alternative<std::string>(da.data) && std::holds_alternative<std::string>(db.data))
    {
        return Value(std::get<std::string>(da.data) <= std::get<std::string>(db.data));
    }
    if (da.isBytes() && db.isBytes())
    {
        return Value(da.getBytes()->data <= db.getBytes()->data);
    }
    throw std::runtime_error(
        "Seb ko santre se mat tolo! '" + da.typeName() + "' aur '" + db.typeName() + "' ko compare (<=) nahi kar sakte!");
}

inline Value greaterThan(const Value &a, const Value &b)
{
    Value da = a.deref();
    Value db = b.deref();
    if (da.isNumeric() && db.isNumeric())
    {
        if (da.isInteger() && db.isInteger())
        {
            return Value(da.asInt() > db.asInt());
        }
        return Value(da.asDouble() > db.asDouble());
    }
    if (std::holds_alternative<std::string>(da.data) && std::holds_alternative<std::string>(db.data))
    {
        return Value(std::get<std::string>(da.data) > std::get<std::string>(db.data));
    }
    if (da.isBytes() && db.isBytes())
    {
        return Value(da.getBytes()->data > db.getBytes()->data);
    }
    throw std::runtime_error(
        "Seb ko santre se mat tolo! '" + da.typeName() + "' aur '" + db.typeName() + "' ko compare (>) nahi kar sakte!");
}

inline Value greaterThanEqual(const Value &a, const Value &b)
{
    Value da = a.deref();
    Value db = b.deref();
    if (da.isNumeric() && db.isNumeric())
    {
        if (da.isInteger() && db.isInteger())
        {
            return Value(da.asInt() >= db.asInt());
        }
        return Value(da.asDouble() >= db.asDouble());
    }
    if (std::holds_alternative<std::string>(da.data) && std::holds_alternative<std::string>(db.data))
    {
        return Value(std::get<std::string>(da.data) >= std::get<std::string>(db.data));
    }
    if (da.isBytes() && db.isBytes())
    {
        return Value(da.getBytes()->data >= db.getBytes()->data);
    }
    throw std::runtime_error(
        "Seb ko santre se mat tolo! '" + da.typeName() + "' aur '" + db.typeName() + "' ko compare (>=) nahi kar sakte!");
}

#endif
