#ifndef CLASS_H
#define CLASS_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <stdexcept>
#include "Callable.h"
#include "Value.h"
#include "Function.h"

class ClassCallable;
class BoundMethod;

class Instance : public std::enable_shared_from_this<Instance>
{
private:
    std::shared_ptr<ClassCallable> klass;
    std::unordered_map<std::string, Value> fields;

public:
    bool isDropped = false;

    Instance(std::shared_ptr<ClassCallable> klass) : klass(klass) {}

    std::shared_ptr<ClassCallable> getClass() const
    {
        return klass;
    }

    const std::unordered_map<std::string, Value> &getFields() const
    {
        return fields;
    }

    bool hasField(const std::string &name) const
    {
        return fields.find(name) != fields.end();
    }

    std::shared_ptr<Instance> clone() const;

    Value get(const std::string &name);

    void set(const std::string &name, Value value)
    {
        fields[name] = std::move(value);
    }

    std::string toString() const;
};

class ClassCallable : public Callable, public std::enable_shared_from_this<ClassCallable>
{
private:
    std::string name;
    std::shared_ptr<ClassCallable> superclass;
    std::unordered_map<std::string, std::shared_ptr<UserFunction>> methods;

public:
    ClassCallable(const std::string &name,
                  std::shared_ptr<ClassCallable> superclass,
                  std::unordered_map<std::string, std::shared_ptr<UserFunction>> methods)
        : name(name), superclass(superclass), methods(std::move(methods)) {}

    const std::string &getName() const
    {
        return name;
    }

    std::shared_ptr<ClassCallable> getSuperclass() const
    {
        return superclass;
    }

    std::shared_ptr<UserFunction> findMethod(const std::string &methodName) const
    {
        auto it = methods.find(methodName);
        if (it != methods.end())
        {
            return it->second;
        }
        if (superclass != nullptr)
        {
            return superclass->findMethod(methodName);
        }
        return nullptr;
    }

    std::vector<std::string> getMethodNames() const
    {
        std::vector<std::string> names;
        for (const auto &pair : methods)
        {
            names.push_back(pair.first);
        }
        if (superclass != nullptr)
        {
            auto superNames = superclass->getMethodNames();
            for (const auto &sn : superNames)
            {
                if (methods.find(sn) == methods.end())
                {
                    names.push_back(sn);
                }
            }
        }
        return names;
    }

    std::shared_ptr<UserFunction> findInitializer() const
    {
        auto initializer = findMethod("shuru");
        if (initializer == nullptr)
        {
            initializer = findMethod("paida");
        }
        if (initializer == nullptr)
        {
            initializer = findMethod("init");
        }
        return initializer;
    }

    size_t arity() const override
    {
        auto initializer = findInitializer();
        if (initializer != nullptr)
        {
            return initializer->arity();
        }
        return 0;
    }

    size_t minArity() const override
    {
        auto initializer = findInitializer();
        if (initializer != nullptr)
        {
            return initializer->minArity();
        }
        return 0;
    }

    size_t maxArity() const override
    {
        auto initializer = findInitializer();
        if (initializer != nullptr)
        {
            return initializer->maxArity();
        }
        return 0;
    }

    bool isVariadic() const override
    {
        auto initializer = findInitializer();
        if (initializer != nullptr)
        {
            return initializer->isVariadic();
        }
        return false;
    }

    bool checkArity(size_t count) const override
    {
        auto initializer = findInitializer();
        if (initializer != nullptr)
        {
            return initializer->checkArity(count);
        }
        return count == 0;
    }

    Value call(Interpreter &interpreter, const std::vector<Value> &args) override;

    std::string toString() const override
    {
        return "<biradari " + name + ">";
    }
};

class BoundMethod : public Callable
{
private:
    std::shared_ptr<Instance> instance;
    std::shared_ptr<UserFunction> method;

public:
    BoundMethod(std::shared_ptr<Instance> instance, std::shared_ptr<UserFunction> method)
        : instance(instance), method(method) {}

    size_t arity() const override
    {
        return method->arity();
    }

    size_t minArity() const override
    {
        return method->minArity();
    }

    size_t maxArity() const override
    {
        return method->maxArity();
    }

    bool isVariadic() const override
    {
        return method->isVariadic();
    }

    bool checkArity(size_t count) const override
    {
        return method->checkArity(count);
    }

    Value call(Interpreter &interpreter, const std::vector<Value> &args) override
    {
        auto env = std::make_shared<Environment>(method->getClosure());
        env->define("apun", Value(instance));
        env->define("khud", Value(instance));
        env->define("apna", Value(instance));
        env->define("this", Value(instance));
        env->define("self", Value(instance));

        auto boundFunc = std::make_shared<UserFunction>(method->getDeclaration(), env);
        return boundFunc->call(interpreter, args);
    }

    std::string toString() const override
    {
        return "<method " + method->getName() + ">";
    }
};

class InstanceCloneCallable : public Callable
{
private:
    std::shared_ptr<Instance> instance;

public:
    InstanceCloneCallable(std::shared_ptr<Instance> instance) : instance(instance) {}

    size_t arity() const override { return 0; }

    Value call(Interpreter &, const std::vector<Value> &) override
    {
        return Value(instance->clone());
    }

    std::string toString() const override
    {
        return "<method nakal>";
    }
};

inline Value Instance::get(const std::string &name)
{
    auto it = fields.find(name);
    if (it != fields.end())
    {
        return it->second;
    }

    auto method = klass->findMethod(name);
    if (method != nullptr)
    {
        return Value(std::make_shared<BoundMethod>(shared_from_this(), method));
    }

    if (name == "nakal" || name == "clone")
    {
        return Value(std::make_shared<InstanceCloneCallable>(shared_from_this()));
    }

    throw std::runtime_error("Khopdi tod saale ka! '" + klass->getName() + "' biradari ke paas '" + name + "' naam ka kuch nahi hai!");
}

inline std::shared_ptr<Instance> Instance::clone() const
{
    auto copy = std::make_shared<Instance>(klass);
    for (const auto &pair : fields)
    {
        if (pair.second.isInstance())
        {
            auto child = pair.second.getInstance();
            copy->set(pair.first, child ? Value(child->clone()) : pair.second);
        }
        else
        {
            copy->set(pair.first, pair.second);
        }
    }
    return copy;
}

inline std::string Instance::toString() const
{
    return "<" + klass->getName() + " ka object>";
}

inline Value ClassCallable::call(Interpreter &interpreter, const std::vector<Value> &args)
{
    auto instance = std::make_shared<Instance>(shared_from_this());

    auto initializer = findMethod("shuru");
    if (initializer == nullptr)
    {
        initializer = findMethod("paida");
    }
    if (initializer == nullptr)
    {
        initializer = findMethod("init");
    }

    if (initializer != nullptr)
    {
        BoundMethod bound(instance, initializer);
        bound.call(interpreter, args);
    }

    return Value(instance);
}

#endif
