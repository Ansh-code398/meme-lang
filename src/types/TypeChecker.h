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
    bool isConst = false;
    std::vector<std::shared_ptr<Type>> overloads;
};

class TypeEnvironment : public std::enable_shared_from_this<TypeEnvironment>
{
private:
    std::shared_ptr<TypeEnvironment> enclosing;
    std::unordered_map<std::string, TypeSymbol> bindings;

public:
    TypeEnvironment(std::shared_ptr<TypeEnvironment> enclosing = nullptr);

    void define(const std::string &name, std::shared_ptr<Type> type, bool isExplicit, bool isConst = false);
    void defineOverload(const std::string &name, std::shared_ptr<Type> fnType);
    bool assign(const std::string &name, std::shared_ptr<Type> type, int line);
    std::shared_ptr<TypeSymbol> lookup(const std::string &name);
    bool hasBinding(const std::string &name) const;
};

class TypeChecker
{
private:
    std::shared_ptr<TypeEnvironment> globals;
    std::shared_ptr<Type> currentFunctionReturnType = nullptr;

    void initGlobals();

    void checkStmt(const Stmt *stmt, std::shared_ptr<TypeEnvironment> env);
    std::shared_ptr<Type> checkExpr(const Expr *expr, std::shared_ptr<TypeEnvironment> env);

    static bool isLValue(const Expr *expr);
    static int computeTypeDistance(const std::shared_ptr<Type> &arg, const std::shared_ptr<Type> &param);
    std::shared_ptr<Type> resolveOverload(
        const std::vector<std::shared_ptr<Type>> &candidates,
        const std::vector<std::shared_ptr<Type>> &argTypes,
        const std::string &funcName,
        int line);

public:
    TypeChecker();

    void check(const std::vector<std::unique_ptr<Stmt>> &statements);
};

#endif
