#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <string>
#include <unordered_map>
#include <memory>
#include <stdexcept>
#include <mutex>
#include "Value.h"

class UserFunction;

struct VariableSlot
{
    Value value;
    bool isMoved = false;
    bool isOwned = false;
    bool isMut = false;
    bool isConst = false;
    int sharedBorrows = 0;
    bool hasExclusiveBorrow = false;
    std::string typeAnnotation = "";
    int depth = 0;
    std::shared_ptr<Environment> sourceEnv = nullptr;
    std::string sourceVarName = "";
    bool isParameter = false;

    int sharedBorrowCount() const { return sharedBorrows; }
};

class Environment : public std::enable_shared_from_this<Environment>
{
public:
    static inline std::atomic<int> activeThreadCount{0};

private:
    mutable std::recursive_mutex envMutex;
    std::shared_ptr<Environment> enclosing;
    std::unordered_map<std::string, VariableSlot> values;

    struct ScopedEnvLock
    {
        std::recursive_mutex &m;
        bool locked;
        ScopedEnvLock(std::recursive_mutex &mutex)
            : m(mutex), locked(Environment::activeThreadCount.load(std::memory_order_relaxed) > 0)
        {
            if (locked) m.lock();
        }
        ~ScopedEnvLock()
        {
            if (locked) m.unlock();
        }
    };

public:
    int depth = 0;

    Environment(std::shared_ptr<Environment> enclosing = nullptr, int depth = 0)
        : enclosing(enclosing), depth(depth) {}

    std::shared_ptr<Environment> getEnclosing() const
    {
        return enclosing;
    }

    std::unordered_map<std::string, VariableSlot> &getSlots()
    {
        ScopedEnvLock lock(envMutex);
        return values;
    }

    const std::unordered_map<std::string, VariableSlot> &getSlots() const
    {
        ScopedEnvLock lock(envMutex);
        return values;
    }

    void define(const std::string &name, Value value,
                bool isOwned = false, bool isMut = false, bool isConst = false,
                const std::string &typeAnnotation = "")
    {
        ScopedEnvLock lock(envMutex);
        VariableSlot slot;
        slot.value = std::move(value);
        slot.isMoved = false;
        slot.isOwned = isOwned;
        slot.isMut = isMut;
        slot.isConst = isConst;
        slot.sharedBorrows = 0;
        slot.hasExclusiveBorrow = false;
        slot.typeAnnotation = typeAnnotation;
        slot.depth = depth;
        values[name] = std::move(slot);
    }

    VariableSlot *findSlot(const std::string &name)
    {
        ScopedEnvLock lock(envMutex);
        auto it = values.find(name);
        if (it != values.end())
        {
            return &it->second;
        }
        if (enclosing != nullptr)
        {
            return enclosing->findSlot(name);
        }
        return nullptr;
    }

    const VariableSlot *findSlot(const std::string &name) const
    {
        ScopedEnvLock lock(envMutex);
        auto it = values.find(name);
        if (it != values.end())
        {
            return &it->second;
        }
        if (enclosing != nullptr)
        {
            return enclosing->findSlot(name);
        }
        return nullptr;
    }

    std::shared_ptr<Environment> findOwnerEnv(const std::string &name)
    {
        ScopedEnvLock lock(envMutex);
        auto it = values.find(name);
        if (it != values.end())
        {
            return shared_from_this();
        }
        if (enclosing != nullptr)
        {
            return enclosing->findOwnerEnv(name);
        }
        return nullptr;
    }

    void defineOverload(const std::string &name, std::shared_ptr<UserFunction> func);

    void assignToSlot(VariableSlot *slot, Value value, const std::string &name)
    {
        if (slot->value.isReference())
        {
            const auto &ref = slot->value.getReference();
            if (!ref.isMutable)
            {
                throw std::runtime_error(
                    "Doglapan! '" + ref.varName + "' abhi udhaar (borrowed) pe hai, isko modify nahi kar sakte!");
            }
            if (!ref.env)
            {
                throw std::runtime_error("Doglapan! '" + ref.varName + "' ka environment expire ho chuka hai!");
            }
            ref.env->setDirect(ref.varName, std::move(value));
            return;
        }

        if (slot->isConst)
        {
            throw std::runtime_error(
                "Shanti gadadhari Bheem! Constant / atal variable '" + name + "' ko modify nahi kar sakte!");
        }

        // Direct assignment to variable
        if (slot->sharedBorrows > 0 || slot->hasExclusiveBorrow)
        {
            throw std::runtime_error(
                "Doglapan! '" + name + "' abhi udhaar (borrowed) pe hai, isko modify nahi kar sakte!");
        }

        slot->value = std::move(value);
        slot->isMoved = false;
    }

    void assign(const std::string &name, Value value)
    {
        VariableSlot *slot = findSlot(name);
        if (!slot)
        {
            define(name, std::move(value));
            return;
        }
        assignToSlot(slot, std::move(value), name);
    }

    Value get(const std::string &name) const
    {
        const VariableSlot *slot = findSlot(name);
        if (!slot)
        {
            throw std::runtime_error(
                "Rasode mein kaun tha?! Ye '" + name + "' bina bataye kahan se aaya? Pehle 'pakad " + name + ";' se declare kar!");
        }

        if (slot->isMoved)
        {
            throw std::runtime_error(
                "Pehli fursat mein nikal! '" + name + "' ki value move ho chuki hai! Ownership transfer ke baad use karna mana hai!");
        }

        if (slot->hasExclusiveBorrow)
        {
            throw std::runtime_error(
                "Beech mein mat bolo! '" + name + "' abhi kisi aur ko udhaar diya hua hai!");
        }

        return slot->value;
    }

    Value getDirect(const std::string &name) const
    {
        ScopedEnvLock lock(envMutex);
        auto it = values.find(name);
        if (it != values.end())
        {
            return it->second.value;
        }
        if (enclosing != nullptr)
        {
            return enclosing->getDirect(name);
        }
        throw std::runtime_error("Rasode mein kaun tha?! Ye '" + name + "' bina bataye kahan se aaya? Pehle 'pakad " + name + ";' se declare kar!");
    }

    void setDirect(const std::string &name, Value value)
    {
        ScopedEnvLock lock(envMutex);
        auto it = values.find(name);
        if (it != values.end())
        {
            if (it->second.isConst)
            {
                throw std::runtime_error(
                    "Shanti gadadhari Bheem! Constant / atal variable '" + name + "' ko modify nahi kar sakte!");
            }
            it->second.value = std::move(value);
            it->second.isMoved = false;
            return;
        }
        if (enclosing != nullptr)
        {
            enclosing->setDirect(name, std::move(value));
            return;
        }
        throw std::runtime_error("Chuna laga diya! '" + name + "' naam ka koi variable hi nahi hai. Pehle 'pakad " + name + ";' likh!");
    }

    bool contains(const std::string &name) const
    {
        std::lock_guard<std::recursive_mutex> lock(envMutex);
        auto it = values.find(name);
        if (it != values.end())
        {
            return true;
        }
        if (enclosing != nullptr)
        {
            return enclosing->contains(name);
        }
        return false;
    }
};

inline Value Value::deref() const
{
    if (!isReference())
    {
        return *this;
    }
    const auto &ref = getReference();
    if (!ref.env)
    {
        return Value(std::monostate{});
    }
    Value target = ref.env->getDirect(ref.varName);
    return target.deref();
}

inline bool Value::operator==(const Value &other) const
{
    if (std::holds_alternative<long long>(data) && std::holds_alternative<long long>(other.data))
    {
        return std::get<long long>(data) == std::get<long long>(other.data);
    }
    if (std::holds_alternative<bool>(data) && std::holds_alternative<bool>(other.data))
    {
        return std::get<bool>(data) == std::get<bool>(other.data);
    }
    Value a = this->deref();
    Value b = other.deref();

    // Fast path: numeric comparison across any numeric types (int, long long, uint8_t, double, float, char)
    if (a.isNumeric() && b.isNumeric())
    {
        if (a.isInteger() && b.isInteger())
        {
            return a.asInt() == b.asInt();
        }
        return a.asDouble() == b.asDouble();
    }

    // String / Char equality
    if (std::holds_alternative<std::string>(a.data) && std::holds_alternative<std::string>(b.data))
    {
        return std::get<std::string>(a.data) == std::get<std::string>(b.data);
    }
    if (std::holds_alternative<char>(a.data) && std::holds_alternative<std::string>(b.data))
    {
        const auto &s = std::get<std::string>(b.data);
        return s.size() == 1 && s[0] == std::get<char>(a.data);
    }
    if (std::holds_alternative<std::string>(a.data) && std::holds_alternative<char>(b.data))
    {
        const auto &s = std::get<std::string>(a.data);
        return s.size() == 1 && s[0] == std::get<char>(b.data);
    }

    // Error map vs String comparison (allows comparing caught error map directly to expected message)
    if (a.isMap() && std::holds_alternative<std::string>(b.data))
    {
        auto m = a.getMap();
        if (m && m->has("__is_error__"))
        {
            if (m->has("message") && m->get("message").deref().toString() == std::get<std::string>(b.data)) return true;
            if (m->has("baat") && m->get("baat").deref().toString() == std::get<std::string>(b.data)) return true;
        }
    }
    if (std::holds_alternative<std::string>(a.data) && b.isMap())
    {
        auto m = b.getMap();
        if (m && m->has("__is_error__"))
        {
            if (m->has("message") && m->get("message").deref().toString() == std::get<std::string>(a.data)) return true;
            if (m->has("baat") && m->get("baat").deref().toString() == std::get<std::string>(a.data)) return true;
        }
    }

    // Bytes comparison
    if (a.isBytes() && b.isBytes())
    {
        auto ba = a.getBytes();
        auto bb = b.getBytes();
        if (ba == bb) return true;
        if (!ba || !bb) return false;
        return ba->data == bb->data;
    }

    // Set comparison
    if (a.isSet() && b.isSet())
    {
        auto sa = a.getSet();
        auto sb = b.getSet();
        if (sa == sb) return true;
        if (!sa || !sb) return false;
        if (sa->elements.size() != sb->elements.size()) return false;
        for (const auto &elem : sa->elements)
        {
            if (!sb->contains(elem)) return false;
        }
        return true;
    }

    // Tuple comparison
    if (a.isTuple() && b.isTuple())
    {
        auto ta = a.getTuple();
        auto tb = b.getTuple();
        if (ta == tb) return true;
        if (!ta || !tb) return false;
        if (ta->elements.size() != tb->elements.size()) return false;
        for (size_t i = 0; i < ta->elements.size(); ++i)
        {
            if (ta->elements[i] != tb->elements[i]) return false;
        }
        return true;
    }

    return std::visit(
        overload{
            [](std::monostate, std::monostate) -> bool { return true; },
            [](bool lhs, bool rhs) -> bool { return lhs == rhs; },
            [](const std::shared_ptr<Callable> &lhs, const std::shared_ptr<Callable> &rhs) -> bool {
                return lhs == rhs;
            },
            [](const std::shared_ptr<Instance> &lhs, const std::shared_ptr<Instance> &rhs) -> bool {
                return lhs == rhs;
            },
            [](const std::shared_ptr<MemeList> &lhs, const std::shared_ptr<MemeList> &rhs) -> bool {
                if (lhs == rhs) return true;
                if (!lhs || !rhs) return false;
                if (lhs->elements.size() != rhs->elements.size()) return false;
                for (size_t i = 0; i < lhs->elements.size(); ++i) {
                    if (lhs->elements[i] != rhs->elements[i]) return false;
                }
                return true;
            },
            [](const std::shared_ptr<MemeMap> &lhs, const std::shared_ptr<MemeMap> &rhs) -> bool {
                if (lhs == rhs) return true;
                if (!lhs || !rhs) return false;
                if (lhs->entries.size() != rhs->entries.size()) return false;
                for (const auto &pair : lhs->entries) {
                    auto it = rhs->entries.find(pair.first);
                    if (it == rhs->entries.end() || it->second != pair.second) return false;
                }
                return true;
            },
            [](const std::shared_ptr<Pointer> &lhs, const std::shared_ptr<Pointer> &rhs) -> bool {
                if (lhs == rhs) return true;
                if (!lhs && !rhs) return true;
                if (!lhs || !rhs) return false;
                return *lhs == *rhs;
            },
            [](const std::shared_ptr<Pointer> &lhs, std::monostate) -> bool {
                return !lhs || lhs->isNull();
            },
            [](std::monostate, const std::shared_ptr<Pointer> &rhs) -> bool {
                return !rhs || rhs->isNull();
            },
            [](const std::shared_ptr<MemeOption> &lhs, const std::shared_ptr<MemeOption> &rhs) -> bool {
                if (lhs == rhs) return true;
                if (!lhs || !rhs) return false;
                if (lhs->hasValue != rhs->hasValue) return false;
                if (!lhs->hasValue) return true;
                return lhs->value == rhs->value;
            },
            [](const std::shared_ptr<MemeResult> &lhs, const std::shared_ptr<MemeResult> &rhs) -> bool {
                if (lhs == rhs) return true;
                if (!lhs || !rhs) return false;
                if (lhs->isOkVal != rhs->isOkVal) return false;
                return lhs->value == rhs->value;
            },
            [](const auto &, const auto &) -> bool { return false; }
        },
        a.data,
        b.data);
}

#endif
