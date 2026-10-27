#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <string>
#include <unordered_map>
#include <memory>
#include <stdexcept>
#include "Value.h"

struct VariableSlot
{
    Value value;
    bool isMoved = false;
    int sharedBorrows = 0;
    bool hasExclusiveBorrow = false;
    int depth = 0;
};

class Environment : public std::enable_shared_from_this<Environment>
{
private:
    std::shared_ptr<Environment> enclosing;
    std::unordered_map<std::string, VariableSlot> values;

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
        return values;
    }

    const std::unordered_map<std::string, VariableSlot> &getSlots() const
    {
        return values;
    }

    void define(const std::string &name, Value value)
    {
        VariableSlot slot;
        slot.value = std::move(value);
        slot.isMoved = false;
        slot.sharedBorrows = 0;
        slot.hasExclusiveBorrow = false;
        slot.depth = depth;
        values[name] = std::move(slot);
    }

    VariableSlot *findSlot(const std::string &name)
    {
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

    void assign(const std::string &name, Value value)
    {
        VariableSlot *slot = findSlot(name);
        if (!slot)
        {
            define(name, std::move(value));
            return;
        }

        // If the slot contains a reference, mutate through reference!
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

        // Direct assignment to variable
        if (slot->sharedBorrows > 0 || slot->hasExclusiveBorrow)
        {
            throw std::runtime_error(
                "Doglapan! '" + name + "' abhi udhaar (borrowed) pe hai, isko modify nahi kar sakte!");
        }

        slot->value = std::move(value);
        slot->isMoved = false;
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
        auto it = values.find(name);
        if (it != values.end())
        {
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
        if (values.find(name) != values.end())
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
    Value a = this->deref();
    Value b = other.deref();
    return std::visit(
        overload{
            [](std::monostate, std::monostate) -> bool { return true; },
            [](long long lhs, long long rhs) -> bool { return lhs == rhs; },
            [](double lhs, double rhs) -> bool { return lhs == rhs; },
            [](long long lhs, double rhs) -> bool { return static_cast<double>(lhs) == rhs; },
            [](double lhs, long long rhs) -> bool { return lhs == static_cast<double>(rhs); },
            [](const std::string &lhs, const std::string &rhs) -> bool { return lhs == rhs; },
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
            [](const auto &, const auto &) -> bool { return false; }
        },
        a.data,
        b.data);
}

#endif
