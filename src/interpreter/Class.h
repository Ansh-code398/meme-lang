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
    Instance(std::shared_ptr<ClassCallable> klass) : klass(klass) {}

    std::shared_ptr<ClassCallable> getClass() const
    {
        return klass;
    }

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

    size_t arity() const override
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
        if (initializer != nullptr)
        {
            return initializer->arity();
        }
        return 0;
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

    Value call(Interpreter &interpreter, const std::vector<Value> &args) override
    {
        auto env = std::make_shared<Environment>(method->getClosure());
        env->define("apun", Value(instance));
        env->define("khud", Value(instance));
        env->define("apna", Value(instance));
        env->define("this", Value(instance));

        auto boundFunc = std::make_shared<UserFunction>(method->getDeclaration(), env);
        return boundFunc->call(interpreter, args);
    }

    std::string toString() const override
    {
        return "<method " + method->getName() + ">";
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

    throw std::runtime_error("Khopdi tod saale ka! '" + klass->getName() + "' biradari ke paas '" + name + "' naam ka kuch nahi hai!");
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
