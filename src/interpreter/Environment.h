#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <string>
#include <unordered_map>
#include <memory>
#include <stdexcept>
#include "Value.h"

class Environment : public std::enable_shared_from_this<Environment>
{
private:
    std::shared_ptr<Environment> enclosing;
    std::unordered_map<std::string, Value> values;

public:
    Environment(std::shared_ptr<Environment> enclosing = nullptr)
        : enclosing(enclosing) {}

    std::shared_ptr<Environment> getEnclosing() const
    {
        return enclosing;
    }

    void define(const std::string &name, Value value)
    {
        values[name] = std::move(value);
    }

    void assign(const std::string &name, Value value)
    {
        auto it = values.find(name);
        if (it != values.end())
        {
            it->second = std::move(value);
            return;
        }

        if (enclosing != nullptr)
        {
            enclosing->assign(name, std::move(value));
            return;
        }

        throw std::runtime_error(
            "Chuna laga diya! '" + name + "' naam ka koi variable hi nahi hai. Pehle 'pakad " + name + ";' likh!");
    }

    Value get(const std::string &name) const
    {
        auto it = values.find(name);
        if (it != values.end())
        {
            return it->second;
        }

        if (enclosing != nullptr)
        {
            return enclosing->get(name);
        }

        throw std::runtime_error(
            "Rasode mein kaun tha?! Ye '" + name + "' bina bataye kahan se aaya? Pehle 'pakad " + name + ";' se declare kar!");
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

#endif
