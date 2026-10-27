#ifndef CALLABLE_H
#define CALLABLE_H

#include <vector>
#include <string>
#include <memory>

class Interpreter;
struct Value;

class Callable
{
public:
    virtual ~Callable() = default;
    virtual size_t arity() const = 0;
    virtual size_t minArity() const { return arity(); }
    virtual size_t maxArity() const { return arity(); }
    virtual bool isVariadic() const { return false; }
    virtual bool checkArity(size_t count) const { return count == arity(); }
    virtual Value call(Interpreter &interpreter, const std::vector<Value> &args) = 0;
    virtual std::string toString() const = 0;
};

#endif
