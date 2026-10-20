#ifndef SIGNALS_H
#define SIGNALS_H

#include <exception>
#include "Value.h"

// Thrown by `wapas expr;` or `wapas;`
class ReturnSignal : public std::exception
{
public:
    Value value;
    ReturnSignal(Value value) : value(std::move(value)) {}
};

// Thrown by `bas;` (break)
class BreakSignal : public std::exception
{
};

// Thrown by `chal;` (continue)
class ContinueSignal : public std::exception
{
};

#endif
