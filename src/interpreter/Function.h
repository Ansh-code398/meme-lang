#ifndef FUNCTION_H
#define FUNCTION_H

#include <vector>
#include <string>
#include <memory>
#include <stdexcept>
#include <functional>
#include "Callable.h"
#include "Value.h"
#include "Environment.h"
#include "../parser/Parser.h"

class Interpreter;

class UserFunction : public Callable
{
private:
    std::shared_ptr<const FuncDeclStmt> declarationOwner;
    const FuncDeclStmt *declaration;
    std::shared_ptr<Environment> closure;

public:
    UserFunction(const FuncDeclStmt *declaration, std::shared_ptr<Environment> closure)
        : declaration(declaration), closure(closure) {}

    UserFunction(std::shared_ptr<const FuncDeclStmt> declarationOwner, std::shared_ptr<Environment> closure)
        : declarationOwner(declarationOwner), declaration(declarationOwner.get()), closure(closure) {}

    size_t arity() const override
    {
        return declaration->minArity();
    }

    size_t minArity() const override
    {
        return declaration->minArity();
    }

    size_t maxArity() const override
    {
        return declaration->maxArity();
    }

    bool isVariadic() const override
    {
        return declaration->isVariadic();
    }

    bool checkArity(size_t count) const override
    {
        if (isVariadic())
        {
            return count >= minArity();
        }
        return count >= minArity() && count <= maxArity();
    }

    Value call(Interpreter &interpreter, const std::vector<Value> &args) override;

    std::string toString() const override
    {
        if (declaration->name.empty())
        {
            return "<jugaad lambda>";
        }
        return "<kar " + declaration->name + ">";
    }

    const std::string &getName() const
    {
        return declaration->name;
    }

    const FuncDeclStmt *getDeclaration() const
    {
        return declaration;
    }

    std::shared_ptr<Environment> getClosure() const
    {
        return closure;
    }
};

// Builtin: pata_laga(val) -> returns type name
class PataLagaBuiltin : public Callable
{
public:
    size_t arity() const override { return 1; }
    Value call(Interpreter &, const std::vector<Value> &args) override
    {
        return Value(args[0].typeName());
    }
    std::string toString() const override { return "<builtin pata_laga>"; }
};

// Builtin: lambai(val) -> returns length of string
class LambaiBuiltin : public Callable
{
public:
    size_t arity() const override { return 1; }
    Value call(Interpreter &interpreter, const std::vector<Value> &args) override;
    std::string toString() const override { return "<builtin size>"; }
};

// Builtin: bina_point(val) -> converts string/float/bool to int
class AnkBuiltin : public Callable
{
public:
    size_t arity() const override { return 1; }
    Value call(Interpreter &, const std::vector<Value> &args) override
    {
        Value val = args[0].deref();
        return std::visit(
            overload{
                [](long long v) -> Value { return Value(v); },
                [](double v) -> Value { return Value(static_cast<long long>(v)); },
                [](bool v) -> Value { return Value(v ? 1LL : 0LL); },
                [](const std::string &v) -> Value
                {
                    try
                    {
                        return Value(std::stoll(v));
                    }
                    catch (...)
                    {
                        throw std::runtime_error("Utha le re baba! '" + v + "' ko bina_point (integer) number mein kaise badlun?");
                    }
                },
                [](const auto &) -> Value
                {
                    throw std::runtime_error("Utha le re baba! Is type ko bina_point (integer) number mein convert nahi kar sakte!");
                }
            },
            val.data);
    }
    std::string toString() const override { return "<builtin bina_point>"; }
};

// Builtin: point_wala(val) -> converts string/int to float
class DashamlavBuiltin : public Callable
{
public:
    size_t arity() const override { return 1; }
    Value call(Interpreter &, const std::vector<Value> &args) override
    {
        Value val = args[0].deref();
        return std::visit(
            overload{
                [](double v) -> Value { return Value(v); },
                [](long long v) -> Value { return Value(static_cast<double>(v)); },
                [](bool v) -> Value { return Value(v ? 1.0 : 0.0); },
                [](const std::string &v) -> Value
                {
                    try
                    {
                        return Value(std::stod(v));
                    }
                    catch (...)
                    {
                        throw std::runtime_error("Daya, kuch toh gadbad hai! '" + v + "' ko point_wala (decimal) number mein convert nahi kar sakte!");
                    }
                },
                [](const auto &) -> Value
                {
                    throw std::runtime_error("Daya, kuch toh gadbad hai! Is type ko point_wala (decimal) number mein convert nahi kar sakte!");
                }
            },
            val.data);
    }
    std::string toString() const override { return "<builtin point_wala>"; }
};

// Builtin: nakal(val) -> explicit clone
class NakalBuiltin : public Callable
{
public:
    size_t arity() const override { return 1; }
    Value call(Interpreter &interpreter, const std::vector<Value> &args) override;
    std::string toString() const override { return "<builtin nakal>"; }
};

// Builtin: kundli(obj) -> returns dynamic fields map or class methods list
class KundliBuiltin : public Callable
{
public:
    size_t arity() const override { return 1; }
    Value call(Interpreter &interpreter, const std::vector<Value> &args) override;
    std::string toString() const override { return "<builtin kundli>"; }
};

#endif
