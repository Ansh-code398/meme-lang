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

// Thrown by `chillao expr;` (user exception)
class MemeUserException : public std::exception
{
public:
    Value value;
    int line;
    mutable std::string formattedMsg;

    MemeUserException(Value val, int line = 0)
        : value(std::move(val)), line(line)
    {
        formattedMsg = "Lafda ho gaya! Uncaught exception: " + value.toString();
        if (line > 0)
        {
            formattedMsg = "Line " + std::to_string(line) + ": " + formattedMsg;
        }
    }

    const char *what() const noexcept override
    {
        return formattedMsg.c_str();
    }
};

// Thrown by `systumm.nikal(code)` or `exit(code)` when running in non-CLI/WASM/test environments
class MemeExitSignal : public std::exception
{
public:
    long long exitCode;
    explicit MemeExitSignal(long long code = 0) : exitCode(code) {}
    const char *what() const noexcept override
    {
        return "Program exited cleanly via systumm.nikal()";
    }
};

#endif
