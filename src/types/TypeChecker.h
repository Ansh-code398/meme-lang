#ifndef TYPECHECKER_H
#define TYPECHECKER_H

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include "Type.h"
#include "../parser/Parser.h"

struct TypeSymbol
{
    std::shared_ptr<Type> type;
    bool isExplicit = false;
};

class TypeEnvironment : public std::enable_shared_from_this<TypeEnvironment>
{
private:
    std::shared_ptr<TypeEnvironment> enclosing;
    std::unordered_map<std::string, TypeSymbol> bindings;

public:
    TypeEnvironment(std::shared_ptr<TypeEnvironment> enclosing = nullptr);

    void define(const std::string &name, std::shared_ptr<Type> type, bool isExplicit);
    bool assign(const std::string &name, std::shared_ptr<Type> type, int line);
    std::shared_ptr<TypeSymbol> lookup(const std::string &name);
};

class TypeChecker
{
private:
    std::shared_ptr<TypeEnvironment> globals;
    std::shared_ptr<Type> currentFunctionReturnType = nullptr;

    void initGlobals();

    void checkStmt(const Stmt *stmt, std::shared_ptr<TypeEnvironment> env);
    std::shared_ptr<Type> checkExpr(const Expr *expr, std::shared_ptr<TypeEnvironment> env);

public:
    TypeChecker();

    void check(const std::vector<std::unique_ptr<Stmt>> &statements);
};

#endif
