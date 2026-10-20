#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include "Value.h"
#include "Environment.h"
#include "Callable.h"
#include "Function.h"
#include "Class.h"
#include "Operators.h"
#include "Signals.h"
#include "../parser/Parser.h"

class Interpreter
{
private:
    std::shared_ptr<Environment> globals;
    std::shared_ptr<Environment> environment;
    std::ostream &out;
    std::istream &in;

public:
    Interpreter(std::ostream &out = std::cout, std::istream &in = std::cin);

    void interpret(const std::vector<std::unique_ptr<Stmt>> &statements);
    Value evaluate(const Expr *expr);
    void execute(const Stmt *stmt);
    void executeBlock(const std::vector<std::unique_ptr<Stmt>> &statements, std::shared_ptr<Environment> newEnv);

    std::shared_ptr<Environment> getGlobals() { return globals; }
    std::shared_ptr<Environment> getEnvironment() { return environment; }
    std::ostream &getOutputStream() { return out; }
    std::istream &getInputStream() { return in; }
};

#endif
