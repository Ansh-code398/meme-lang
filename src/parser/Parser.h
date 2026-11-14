#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <stdexcept>
#include "../lexer/Lexer.h"
#include "../types/Type.h"

// ==========================================
// 1. Expression AST Hierarchy (Expr)
// ==========================================

class Expr
{
public:
    int line = 0;
    Expr(int line = 0) : line(line) {}
    virtual ~Expr() = default;
    virtual std::string toString() const = 0;
};

class IntExpr : public Expr
{
public:
    long long value;
    IntExpr(long long value, int line = 0) : Expr(line), value(value) {}
    std::string toString() const override
    {
        return std::to_string(value);
    }
};

class ByteExpr : public Expr
{
public:
    uint8_t value;
    ByteExpr(uint8_t value, int line = 0) : Expr(line), value(value) {}
    std::string toString() const override
    {
        return std::to_string(static_cast<int>(value)) + "b";
    }
};

class BytesExpr : public Expr
{
public:
    std::vector<uint8_t> value;
    BytesExpr(std::vector<uint8_t> value, int line = 0) : Expr(line), value(std::move(value)) {}
    std::string toString() const override
    {
        return "b\"...\"";
    }
};

class FloatExpr : public Expr
{
public:
    double value;
    bool isFloat32 = false;
    FloatExpr(double value, int line = 0, bool isFloat32 = false)
        : Expr(line), value(value), isFloat32(isFloat32) {}
    std::string toString() const override
    {
        return std::to_string(value) + (isFloat32 ? "f" : "");
    }
};

class StringExpr : public Expr
{
public:
    std::string value;
    StringExpr(const std::string &value, int line = 0) : Expr(line), value(value) {}
    std::string toString() const override
    {
        return "\"" + value + "\"";
    }
};

class BoolExpr : public Expr
{
public:
    bool value;
    BoolExpr(bool value, int line = 0) : Expr(line), value(value) {}
    std::string toString() const override
    {
        return value ? "han" : "nhi";
    }
};

class NullExpr : public Expr
{
public:
    NullExpr(int line = 0) : Expr(line) {}
    std::string toString() const override
    {
        return "kuch_nhi";
    }
};

class IdentifierExpr : public Expr
{
public:
    std::string name;
    IdentifierExpr(const std::string &name, int line = 0) : Expr(line), name(name) {}
    std::string toString() const override
    {
        return name;
    }
};

class AssignExpr : public Expr
{
public:
    std::string name;
    std::unique_ptr<Expr> value;

    AssignExpr(const std::string &name, std::unique_ptr<Expr> value, int line = 0)
        : Expr(line), name(name), value(std::move(value)) {}

    std::string toString() const override
    {
        return "(" + name + " = " + value->toString() + ")";
    }
};

class UnaryExpr : public Expr
{
public:
    Token op;
    std::unique_ptr<Expr> operand;

    UnaryExpr(Token op, std::unique_ptr<Expr> operand, int line = 0)
        : Expr(line), op(op), operand(std::move(operand)) {}

    std::string toString() const override
    {
        return "(" + op.value + operand->toString() + ")";
    }
};

class BinaryExpr : public Expr
{
public:
    std::unique_ptr<Expr> left;
    Token op;
    std::unique_ptr<Expr> right;

    BinaryExpr(std::unique_ptr<Expr> left, Token op, std::unique_ptr<Expr> right, int line = 0)
        : Expr(line), left(std::move(left)), op(op), right(std::move(right)) {}

    std::string toString() const override
    {
        return "(" + left->toString() + " " + op.value + " " + right->toString() + ")";
    }
};

class TernaryExpr : public Expr
{
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> thenBranch;
    std::unique_ptr<Expr> elseBranch;

    TernaryExpr(std::unique_ptr<Expr> condition,
                std::unique_ptr<Expr> thenBranch,
                std::unique_ptr<Expr> elseBranch,
                int line = 0)
        : Expr(line), condition(std::move(condition)), thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}

    std::string toString() const override
    {
        return "(" + condition->toString() + " ? " + thenBranch->toString() + " : " + elseBranch->toString() + ")";
    }
};

class CallExpr : public Expr
{
public:
    std::unique_ptr<Expr> callee;
    std::vector<std::unique_ptr<Expr>> args;

    CallExpr(std::unique_ptr<Expr> callee, std::vector<std::unique_ptr<Expr>> args, int line = 0)
        : Expr(line), callee(std::move(callee)), args(std::move(args)) {}

    std::string toString() const override
    {
        std::string result = callee->toString() + "(";
        for (size_t i = 0; i < args.size(); ++i)
        {
            if (i > 0)
                result += ", ";
            result += args[i]->toString();
        }
        result += ")";
        return result;
    }
};

class PoochExpr : public Expr
{
public:
    std::unique_ptr<Expr> prompt;

    PoochExpr(std::unique_ptr<Expr> prompt = nullptr, int line = 0)
        : Expr(line), prompt(std::move(prompt)) {}

    std::string toString() const override
    {
        if (prompt)
            return "pooch(" + prompt->toString() + ")";
        return "pooch()";
    }
};

class GetExpr : public Expr
{
public:
    std::unique_ptr<Expr> object;
    std::string name;

    GetExpr(std::unique_ptr<Expr> object, const std::string &name, int line = 0)
        : Expr(line), object(std::move(object)), name(name) {}

    std::string toString() const override
    {
        return object->toString() + "." + name;
    }
};

class SetExpr : public Expr
{
public:
    std::unique_ptr<Expr> object;
    std::string name;
    std::unique_ptr<Expr> value;

    SetExpr(std::unique_ptr<Expr> object, const std::string &name, std::unique_ptr<Expr> value, int line = 0)
        : Expr(line), object(std::move(object)), name(name), value(std::move(value)) {}

    std::string toString() const override
    {
        return "(" + object->toString() + "." + name + " = " + value->toString() + ")";
    }
};

class BorrowExpr : public Expr
{
public:
    std::unique_ptr<Expr> operand;
    bool isMutable;

    BorrowExpr(std::unique_ptr<Expr> operand, bool isMutable = false, int line = 0)
        : Expr(line), operand(std::move(operand)), isMutable(isMutable) {}

    std::string toString() const override
    {
        return (isMutable ? "udhaar_badal " : "udhaar ") + (operand ? operand->toString() : "");
    }
};

class NakalExpr : public Expr
{
public:
    std::unique_ptr<Expr> operand;

    NakalExpr(std::unique_ptr<Expr> operand, int line = 0)
        : Expr(line), operand(std::move(operand)) {}

    std::string toString() const override
    {
        return "nakal(" + (operand ? operand->toString() : "") + ")";
    }
};

class AddressOfExpr : public Expr
{
public:
    std::unique_ptr<Expr> operand;

    AddressOfExpr(std::unique_ptr<Expr> operand, int line = 0)
        : Expr(line), operand(std::move(operand)) {}

    std::string toString() const override
    {
        return "&" + (operand ? operand->toString() : "");
    }
};

class DerefExpr : public Expr
{
public:
    std::unique_ptr<Expr> operand;

    DerefExpr(std::unique_ptr<Expr> operand, int line = 0)
        : Expr(line), operand(std::move(operand)) {}

    std::string toString() const override
    {
        return "*" + (operand ? operand->toString() : "");
    }
};

class DerefAssignExpr : public Expr
{
public:
    std::unique_ptr<Expr> pointer;
    std::unique_ptr<Expr> value;

    DerefAssignExpr(std::unique_ptr<Expr> pointer, std::unique_ptr<Expr> value, int line = 0)
        : Expr(line), pointer(std::move(pointer)), value(std::move(value)) {}

    std::string toString() const override
    {
        return "*" + (pointer ? pointer->toString() : "") + " = " + (value ? value->toString() : "");
    }
};

class MoveExpr : public Expr
{
public:
    std::unique_ptr<Expr> operand;

    MoveExpr(std::unique_ptr<Expr> operand, int line = 0)
        : Expr(line), operand(std::move(operand)) {}

    std::string toString() const override
    {
        return "rawana " + (operand ? operand->toString() : "");
    }
};

class ArrayLiteralExpr : public Expr
{
public:
    std::vector<std::unique_ptr<Expr>> elements;

    ArrayLiteralExpr(std::vector<std::unique_ptr<Expr>> elements, int line = 0)
        : Expr(line), elements(std::move(elements)) {}

    std::string toString() const override
    {
        std::string res = "[";
        for (size_t i = 0; i < elements.size(); ++i)
        {
            if (i > 0) res += ", ";
            res += elements[i]->toString();
        }
        res += "]";
        return res;
    }
};

class MapLiteralExpr : public Expr
{
public:
    std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> entries;

    MapLiteralExpr(std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> entries, int line = 0)
        : Expr(line), entries(std::move(entries)) {}

    std::string toString() const override
    {
        std::string res = "{";
        for (size_t i = 0; i < entries.size(); ++i)
        {
            if (i > 0) res += ", ";
            res += entries[i].first->toString() + ": " + entries[i].second->toString();
        }
        res += "}";
        return res;
    }
};

class TupleExpr : public Expr
{
public:
    std::vector<std::unique_ptr<Expr>> elements;

    TupleExpr(std::vector<std::unique_ptr<Expr>> elements, int line = 0)
        : Expr(line), elements(std::move(elements)) {}

    std::string toString() const override
    {
        std::string res = "(";
        for (size_t i = 0; i < elements.size(); ++i)
        {
            if (i > 0) res += ", ";
            res += elements[i]->toString();
        }
        if (elements.size() == 1) res += ",";
        res += ")";
        return res;
    }
};

class SetLiteralExpr : public Expr
{
public:
    std::vector<std::unique_ptr<Expr>> elements;

    SetLiteralExpr(std::vector<std::unique_ptr<Expr>> elements, int line = 0)
        : Expr(line), elements(std::move(elements)) {}

    std::string toString() const override
    {
        std::string res = "{";
        for (size_t i = 0; i < elements.size(); ++i)
        {
            if (i > 0) res += ", ";
            res += elements[i]->toString();
        }
        res += "}";
        return res;
    }
};

class IndexExpr : public Expr
{
public:
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> index;

    IndexExpr(std::unique_ptr<Expr> target, std::unique_ptr<Expr> index, int line = 0)
        : Expr(line), target(std::move(target)), index(std::move(index)) {}

    std::string toString() const override
    {
        return target->toString() + "[" + index->toString() + "]";
    }
};

class IndexAssignExpr : public Expr
{
public:
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> index;
    std::unique_ptr<Expr> value;

    IndexAssignExpr(std::unique_ptr<Expr> target, std::unique_ptr<Expr> index, std::unique_ptr<Expr> value, int line = 0)
        : Expr(line), target(std::move(target)), index(std::move(index)), value(std::move(value)) {}

    std::string toString() const override
    {
        return target->toString() + "[" + index->toString() + "] = " + value->toString();
    }
};

class SpreadExpr : public Expr
{
public:
    std::unique_ptr<Expr> expr;

    SpreadExpr(std::unique_ptr<Expr> expr, int line = 0)
        : Expr(line), expr(std::move(expr)) {}

    std::string toString() const override
    {
        return "*" + (expr ? expr->toString() : "");
    }
};

class FStringExpr : public Expr
{
public:
    std::vector<std::unique_ptr<Expr>> parts;

    FStringExpr(std::vector<std::unique_ptr<Expr>> parts, int line = 0)
        : Expr(line), parts(std::move(parts)) {}

    std::string toString() const override
    {
        std::string s = "f\"";
        for (const auto &p : parts)
        {
            s += p->toString();
        }
        s += "\"";
        return s;
    }
};

class AwaitExpr : public Expr
{
public:
    std::unique_ptr<Expr> expression;

    AwaitExpr(std::unique_ptr<Expr> expression, int line = 0)
        : Expr(line), expression(std::move(expression)) {}

    std::string toString() const override
    {
        return "sabar " + (expression ? expression->toString() : "");
    }
};

class ScopedAccessExpr : public Expr
{
public:
    std::unique_ptr<Expr> scope;
    std::string member;

    ScopedAccessExpr(std::unique_ptr<Expr> scope, std::string member, int line = 0)
        : Expr(line), scope(std::move(scope)), member(std::move(member)) {}

    std::string toString() const override
    {
        return (scope ? scope->toString() : "") + "::" + member;
    }
};

class ScopedSetExpr : public Expr
{
public:
    std::unique_ptr<Expr> scope;
    std::string member;
    std::unique_ptr<Expr> value;

    ScopedSetExpr(std::unique_ptr<Expr> scope, std::string member, std::unique_ptr<Expr> value, int line = 0)
        : Expr(line), scope(std::move(scope)), member(std::move(member)), value(std::move(value)) {}

    std::string toString() const override
    {
        return "(" + (scope ? scope->toString() : "") + "::" + member + " = " + (value ? value->toString() : "") + ")";
    }
};

class TryPropagateExpr : public Expr
{
public:
    std::unique_ptr<Expr> expression;

    TryPropagateExpr(std::unique_ptr<Expr> expression, int line = 0)
        : Expr(line), expression(std::move(expression)) {}

    std::string toString() const override
    {
        return (expression ? expression->toString() : "") + "?";
    }
};

// ==========================================
// 2. Statement AST Hierarchy (Stmt)
// ==========================================

class Stmt
{
public:
    int line = 0;
    Stmt(int line = 0) : line(line) {}
    virtual ~Stmt() = default;
    virtual std::string toString() const = 0;
};

// 1. Variable Declaration: `le x = 5;` or `le x;`
class VarDeclStmt : public Stmt
{
public:
    std::string name;
    std::unique_ptr<Expr> initializer; // nullptr if uninitialized (defaults to kuch_nhi)
    std::shared_ptr<Type> typeAnnotation; // nullptr if untyped
    bool isOwned = false;
    bool isMut = false;
    bool isConst = false;

    VarDeclStmt(const std::string &name,
                std::unique_ptr<Expr> initializer = nullptr,
                std::shared_ptr<Type> typeAnnotation = nullptr,
                int line = 0,
                bool isOwned = false,
                bool isMut = false,
                bool isConst = false)
        : Stmt(line), name(name), initializer(std::move(initializer)),
          typeAnnotation(std::move(typeAnnotation)),
          isOwned(isOwned), isMut(isMut), isConst(isConst) {}

    VarDeclStmt(const std::string &name,
                std::unique_ptr<Expr> initializer,
                int line)
        : VarDeclStmt(name, std::move(initializer), nullptr, line, false, false, false) {}

    std::string toString() const override
    {
        std::string res = "le ";
        if (isOwned) res += "malik ";
        if (isMut) res += "badal ";
        if (isConst) res += "atal ";
        res += name;
        if (typeAnnotation)
        {
            res += ": " + typeAnnotation->toString();
        }
        if (initializer)
        {
            res += " = " + initializer->toString() + ";";
        }
        else
        {
            res += " = kuch_nhi;";
        }
        return res;
    }
};

struct DestructureItem
{
    std::string name;
    bool isVariadic = false;
};

class ArrayDestructureStmt : public Stmt
{
public:
    std::vector<DestructureItem> items;
    std::unique_ptr<Expr> initializer;

    ArrayDestructureStmt(std::vector<DestructureItem> items, std::unique_ptr<Expr> initializer, int line = 0)
        : Stmt(line), items(std::move(items)), initializer(std::move(initializer)) {}

    std::string toString() const override
    {
        std::string s = "pakad [";
        for (size_t i = 0; i < items.size(); ++i)
        {
            if (i > 0) s += ", ";
            if (items[i].isVariadic) s += "*";
            s += items[i].name;
        }
        s += "] = " + (initializer ? initializer->toString() : "nalla") + ";";
        return s;
    }
};

class DestructureAssignExpr : public Expr
{
public:
    std::vector<DestructureItem> items;
    std::unique_ptr<Expr> value;

    DestructureAssignExpr(std::vector<DestructureItem> items, std::unique_ptr<Expr> value, int line = 0)
        : Expr(line), items(std::move(items)), value(std::move(value)) {}

    std::string toString() const override
    {
        return "[...] = ...";
    }
};

class MapDestructureStmt : public Stmt
{
public:
    std::vector<std::string> keys;
    std::unique_ptr<Expr> initializer;

    MapDestructureStmt(std::vector<std::string> keys, std::unique_ptr<Expr> initializer, int line = 0)
        : Stmt(line), keys(std::move(keys)), initializer(std::move(initializer)) {}

    std::string toString() const override
    {
        std::string s = "pakad {";
        for (size_t i = 0; i < keys.size(); ++i)
        {
            if (i > 0) s += ", ";
            s += keys[i];
        }
        s += "} = " + (initializer ? initializer->toString() : "nalla") + ";";
        return s;
    }
};

// 2. Expression Statement: `x + 1;` or `x = 10;`
class ExprStmt : public Stmt
{
public:
    std::unique_ptr<Expr> expression;

    ExprStmt(std::unique_ptr<Expr> expression, int line = 0)
        : Stmt(line), expression(std::move(expression)) {}

    std::string toString() const override
    {
        return expression->toString() + ";";
    }
};

// 3. Print Statement: `dikha a, b;` with optional options (end/ant, sep/alag, color/rang, bina_line)
class PrintStmt : public Stmt
{
public:
    std::vector<std::unique_ptr<Expr>> args;
    std::unique_ptr<Expr> endExpr = nullptr;
    std::unique_ptr<Expr> sepExpr = nullptr;
    std::unique_ptr<Expr> styleExpr = nullptr;
    bool noNewline = false;

    PrintStmt(std::vector<std::unique_ptr<Expr>> args, int line = 0)
        : Stmt(line), args(std::move(args)) {}

    PrintStmt(std::vector<std::unique_ptr<Expr>> args,
              std::unique_ptr<Expr> endExpr,
              std::unique_ptr<Expr> sepExpr,
              std::unique_ptr<Expr> styleExpr,
              bool noNewline = false,
              int line = 0)
        : Stmt(line),
          args(std::move(args)),
          endExpr(std::move(endExpr)),
          sepExpr(std::move(sepExpr)),
          styleExpr(std::move(styleExpr)),
          noNewline(noNewline) {}

    std::string toString() const override
    {
        std::string result = "dikha ";
        for (size_t i = 0; i < args.size(); ++i)
        {
            if (i > 0)
                result += ", ";
            result += args[i]->toString();
        }
        if (sepExpr)
        {
            if (!args.empty()) result += ", ";
            result += "sep=" + sepExpr->toString();
        }
        if (endExpr)
        {
            if (!args.empty() || sepExpr) result += ", ";
            result += "end=" + endExpr->toString();
        }
        if (styleExpr)
        {
            if (!args.empty() || sepExpr || endExpr) result += ", ";
            result += "color=" + styleExpr->toString();
        }
        if (noNewline)
        {
            result += " bina_line";
        }
        result += ";";
        return result;
    }
};

// 4. Block Statement: `{ stmt1; stmt2; }`
class BlockStmt : public Stmt
{
public:
    std::vector<std::unique_ptr<Stmt>> statements;

    BlockStmt(std::vector<std::unique_ptr<Stmt>> statements, int line = 0)
        : Stmt(line), statements(std::move(statements)) {}

    std::string toString() const override
    {
        std::string result = "{ ";
        for (size_t i = 0; i < statements.size(); ++i)
        {
            if (i > 0)
                result += " ";
            result += statements[i]->toString();
        }
        result += " }";
        return result;
    }
};

class ScopeBlockStmt : public BlockStmt
{
public:
    ScopeBlockStmt(std::vector<std::unique_ptr<Stmt>> statements, int line = 0)
        : BlockStmt(std::move(statements), line) {}

    std::string toString() const override
    {
        return "daayra " + BlockStmt::toString();
    }
};

class DropStmt : public Stmt
{
public:
    std::unique_ptr<Expr> expr;

    DropStmt(std::unique_ptr<Expr> expr, int line = 0)
        : Stmt(line), expr(std::move(expr)) {}

    std::string toString() const override
    {
        return "khatam " + (expr ? expr->toString() : "") + ";";
    }
};

// 5. If / Else-If / Else Statement: `dekh cond { ... } warna dekh cond2 { ... } warna { ... }`
class IfStmt : public Stmt
{
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch; // nullptr, BlockStmt, or nested IfStmt (for `warna dekh`)

    IfStmt(std::unique_ptr<Expr> condition,
           std::unique_ptr<Stmt> thenBranch,
           std::unique_ptr<Stmt> elseBranch = nullptr,
           int line = 0)
        : Stmt(line),
          condition(std::move(condition)),
          thenBranch(std::move(thenBranch)),
          elseBranch(std::move(elseBranch)) {}

    std::string toString() const override
    {
        std::string result = "dekh " + condition->toString() + " " + thenBranch->toString();
        if (elseBranch)
        {
            result += " warna " + elseBranch->toString();
        }
        return result;
    }
};

// 6. While Loop: `jabtak cond { ... }`
class WhileStmt : public Stmt
{
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;

    WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body, int line = 0)
        : Stmt(line), condition(std::move(condition)), body(std::move(body)) {}

    std::string toString() const override
    {
        return "jabtak " + condition->toString() + " " + body->toString();
    }
};

// 7. Return Statement: `wapas expr;` or `wapas;`
class ReturnStmt : public Stmt
{
public:
    std::unique_ptr<Expr> value; // nullptr if `wapas;` with no expression

    ReturnStmt(std::unique_ptr<Expr> value = nullptr, int line = 0)
        : Stmt(line), value(std::move(value)) {}

    std::string toString() const override
    {
        if (value)
        {
            return "wapas " + value->toString() + ";";
        }
        return "wapas;";
    }
};

// 8. Break (`bas;`) and Continue (`chal;`) Statements
class BreakStmt : public Stmt
{
public:
    BreakStmt(int line = 0) : Stmt(line) {}
    std::string toString() const override
    {
        return "bas;";
    }
};

class ContinueStmt : public Stmt
{
public:
    ContinueStmt(int line = 0) : Stmt(line) {}
    std::string toString() const override
    {
        return "chal;";
    }
};

// 9. Function Declaration: `kar add(a, b = 10, *rest) { ... }`
struct DestructureParamKey
{
    std::string name;
    std::unique_ptr<Expr> defaultValue;
};

struct FuncParam
{
    std::string name;
    std::unique_ptr<Expr> defaultValue = nullptr;
    bool isVariadic = false;
    std::shared_ptr<Type> typeAnnotation = nullptr;
    bool isMapDestructure = false;
    std::vector<DestructureParamKey> destructureKeys {};

    FuncParam() = default;
    FuncParam(std::string name,
              std::unique_ptr<Expr> defaultValue = nullptr,
              bool isVariadic = false,
              std::shared_ptr<Type> typeAnnotation = nullptr,
              bool isMapDestructure = false,
              std::vector<DestructureParamKey> destructureKeys = {})
        : name(std::move(name)),
          defaultValue(std::move(defaultValue)),
          isVariadic(isVariadic),
          typeAnnotation(std::move(typeAnnotation)),
          isMapDestructure(isMapDestructure),
          destructureKeys(std::move(destructureKeys)) {}
};

class FuncDeclStmt : public Stmt
{
public:
    std::string name;
    std::vector<std::string> params;
    std::vector<FuncParam> paramList;
    std::unique_ptr<Stmt> body;
    std::shared_ptr<Type> returnTypeAnnotation;
    bool isAsync = false;
    bool isOverload = false;

    FuncDeclStmt(const std::string &name,
                 std::vector<FuncParam> paramList,
                 std::unique_ptr<Stmt> body,
                 std::shared_ptr<Type> returnTypeAnnotation,
                 bool isAsync,
                 int line = 0,
                 bool isOverload = false)
        : Stmt(line), name(name), paramList(std::move(paramList)), body(std::move(body)),
          returnTypeAnnotation(std::move(returnTypeAnnotation)), isAsync(isAsync), isOverload(isOverload)
    {
        for (const auto &p : this->paramList)
        {
            params.push_back(p.name);
        }
    }

    FuncDeclStmt(const std::string &name,
                 std::vector<FuncParam> paramList,
                 std::unique_ptr<Stmt> body,
                 std::shared_ptr<Type> returnTypeAnnotation,
                 int line)
        : FuncDeclStmt(name, std::move(paramList), std::move(body), std::move(returnTypeAnnotation), false, line, false) {}

    FuncDeclStmt(const std::string &name,
                 std::vector<FuncParam> paramList,
                 std::unique_ptr<Stmt> body,
                 int line)
        : FuncDeclStmt(name, std::move(paramList), std::move(body), nullptr, false, line, false) {}

    FuncDeclStmt(const std::string &name,
                 std::vector<std::string> oldParams,
                 std::unique_ptr<Stmt> body,
                 int line = 0)
        : Stmt(line), name(name), params(std::move(oldParams)), body(std::move(body)),
          returnTypeAnnotation(nullptr), isAsync(false), isOverload(false)
    {
        for (const auto &p : this->params)
        {
            paramList.push_back(FuncParam{p, nullptr, false, nullptr, false, {}});
        }
    }

    size_t minArity() const
    {
        size_t count = 0;
        for (const auto &p : paramList)
        {
            if (!p.defaultValue && !p.isVariadic && !p.isMapDestructure)
            {
                count++;
            }
        }
        return count;
    }

    size_t maxArity() const
    {
        size_t count = 0;
        for (const auto &p : paramList)
        {
            if (!p.isVariadic)
            {
                count++;
            }
        }
        return count;
    }

    bool isVariadic() const
    {
        return !paramList.empty() && paramList.back().isVariadic;
    }

    std::string toString() const override
    {
        std::string result = (name.empty() ? "jugaad" : "kar " + name) + "(";
        for (size_t i = 0; i < paramList.size(); ++i)
        {
            if (i > 0)
                result += ", ";
            if (paramList[i].isVariadic)
                result += "*";
            result += paramList[i].name;
            if (paramList[i].defaultValue)
                result += " = " + paramList[i].defaultValue->toString();
        }
        result += ") " + (body ? body->toString() : "{}");
        return result;
    }
};

class OperatorDeclStmt : public FuncDeclStmt
{
public:
    std::string opSymbol;
    bool isUnary = false;

    OperatorDeclStmt(const std::string &opSymbol,
                     const std::string &methodName,
                     std::vector<FuncParam> paramList,
                     std::unique_ptr<Stmt> body,
                     std::shared_ptr<Type> returnTypeAnnotation,
                     bool isUnary,
                     int line = 0)
        : FuncDeclStmt(methodName, std::move(paramList), std::move(body), std::move(returnTypeAnnotation), false, line, true),
          opSymbol(opSymbol), isUnary(isUnary) {}

    std::string toString() const override
    {
        return "tantrik " + opSymbol + "(" + (paramList.empty() ? "" : "...") + ") { ... }";
    }
};

class LambdaExpr : public Expr
{
public:
    std::shared_ptr<FuncDeclStmt> declaration;

    LambdaExpr(std::shared_ptr<FuncDeclStmt> declaration, int line = 0)
        : Expr(line), declaration(std::move(declaration)) {}

    std::string toString() const override
    {
        return declaration ? declaration->toString() : "jugaad() {}";
    }
};

class JsxElementExpr : public Expr
{
public:
    std::string tag;
    std::vector<std::pair<std::string, std::unique_ptr<Expr>>> props;
    std::vector<std::unique_ptr<Expr>> children;
    bool isFragment = false;

    JsxElementExpr(std::string tag,
                   std::vector<std::pair<std::string, std::unique_ptr<Expr>>> props,
                   std::vector<std::unique_ptr<Expr>> children,
                   bool isFragment = false,
                   int line = 0)
        : Expr(line), tag(std::move(tag)), props(std::move(props)), children(std::move(children)), isFragment(isFragment) {}

    std::string toString() const override
    {
        return "<" + tag + "...>";
    }
};

// 10. For/Range Loop: `ghoom i se 1 se 10 { ... }`
class GhoomStmt : public Stmt
{
public:
    std::string varName;
    std::unique_ptr<Expr> start;
    std::unique_ptr<Expr> end;
    std::unique_ptr<Expr> step; // nullptr if default (+1 or -1 depending on direction)
    std::unique_ptr<Stmt> body;
    bool inclusive = true;

    GhoomStmt(const std::string &varName,
              std::unique_ptr<Expr> start,
              std::unique_ptr<Expr> end,
              std::unique_ptr<Expr> step,
              std::unique_ptr<Stmt> body,
              int line = 0,
              bool inclusive = true)
        : Stmt(line),
          varName(varName),
          start(std::move(start)),
          end(std::move(end)),
          step(std::move(step)),
          body(std::move(body)),
          inclusive(inclusive) {}

    std::string toString() const override
    {
        std::string result = "ghoom " + varName + " se " + start->toString() + " se " + end->toString();
        if (step)
        {
            result += " se " + step->toString();
        }
        result += " " + body->toString();
        return result;
    }
};

// 10b. For-Each Loop: `ghuma item mein collection { ... }`
class GhumaMeinStmt : public Stmt
{
public:
    std::string varName;
    std::unique_ptr<Expr> collection;
    std::unique_ptr<Stmt> body;

    GhumaMeinStmt(const std::string &varName,
                  std::unique_ptr<Expr> collection,
                  std::unique_ptr<Stmt> body,
                  int line = 0)
        : Stmt(line),
          varName(varName),
          collection(std::move(collection)),
          body(std::move(body)) {}

    std::string toString() const override
    {
        return "ghuma " + varName + " mein " + collection->toString() + " " + body->toString();
    }
};

// 11. Class Declaration: `sancha Name [se SuperName] { ... }`
class ClassDeclStmt : public Stmt
{
public:
    std::string name;
    std::string superclass;
    std::vector<std::unique_ptr<FuncDeclStmt>> methods;

    ClassDeclStmt(const std::string &name,
                  const std::string &superclass,
                  std::vector<std::unique_ptr<FuncDeclStmt>> methods,
                  int line = 0)
        : Stmt(line), name(name), superclass(superclass), methods(std::move(methods)) {}

    std::string toString() const override
    {
        std::string result = "sancha " + name;
        if (!superclass.empty())
        {
            result += " se " + superclass;
        }
        result += " { ... }";
        return result;
    }
};

// 12. Try-Catch-Finally Statement: `panga { ... } lafda err { ... } [pakka { ... }]`
class TryCatchStmt : public Stmt
{
public:
    std::unique_ptr<Stmt> tryBlock;
    std::string catchVar;
    std::unique_ptr<Stmt> catchBlock;
    std::unique_ptr<Stmt> finallyBlock; // nullptr if absent

    TryCatchStmt(std::unique_ptr<Stmt> tryBlock,
                 std::string catchVar,
                 std::unique_ptr<Stmt> catchBlock,
                 std::unique_ptr<Stmt> finallyBlock = nullptr,
                 int line = 0)
        : Stmt(line),
          tryBlock(std::move(tryBlock)),
          catchVar(std::move(catchVar)),
          catchBlock(std::move(catchBlock)),
          finallyBlock(std::move(finallyBlock)) {}

    std::string toString() const override
    {
        std::string result = "panga " + tryBlock->toString() + " lafda " + catchVar + " " + catchBlock->toString();
        if (finallyBlock)
        {
            result += " pakka " + finallyBlock->toString();
        }
        return result;
    }
};

// 13. Throw Statement: `chillao expr;`
class ThrowStmt : public Stmt
{
public:
    std::unique_ptr<Expr> expression;

    ThrowStmt(std::unique_ptr<Expr> expression, int line = 0)
        : Stmt(line), expression(std::move(expression)) {}

    std::string toString() const override
    {
        return "chillao " + expression->toString() + ";";
    }
};

// 14. Import Statement: `bula "path" [jaise alias | jaise { a, b, c }];`
class ImportStmt : public Stmt
{
public:
    std::string path;
    std::string alias;
    std::vector<std::string> namedImports;

    ImportStmt(std::string path, std::string alias = "", std::vector<std::string> namedImports = {}, int line = 0)
        : Stmt(line), path(std::move(path)), alias(std::move(alias)), namedImports(std::move(namedImports)) {}

    std::string toString() const override
    {
        std::string res = "bula \"" + path + "\"";
        if (!namedImports.empty())
        {
            res += " jaise { ";
            for (size_t i = 0; i < namedImports.size(); ++i)
            {
                if (i > 0) res += ", ";
                res += namedImports[i];
            }
            res += " }";
        }
        else if (!alias.empty())
        {
            res += " jaise " + alias;
        }
        res += ";";
        return res;
    }
};

// 15. Enum Declaration: `avtar Status { Pehla, Dusra [= val] }`
struct EnumMember
{
    std::string name;
    std::unique_ptr<Expr> value; // nullptr if auto-increment
};

class PackageDeclStmt : public Stmt
{
public:
    std::string name;

    PackageDeclStmt(std::string name, int line = 0)
        : Stmt(line), name(std::move(name)) {}

    std::string toString() const override
    {
        return "potli " + name + ";";
    }
};

class ExportStmt : public Stmt
{
public:
    std::unique_ptr<Stmt> decl;

    ExportStmt(std::unique_ptr<Stmt> decl, int line = 0)
        : Stmt(line), decl(std::move(decl)) {}

    std::string toString() const override
    {
        return "bech " + (decl ? decl->toString() : "");
    }
};

class NamespaceDeclStmt : public Stmt
{
public:
    std::string name;
    std::vector<std::unique_ptr<Stmt>> body;

    NamespaceDeclStmt(std::string name, std::vector<std::unique_ptr<Stmt>> body, int line = 0)
        : Stmt(line), name(std::move(name)), body(std::move(body)) {}

    std::string toString() const override
    {
        std::string res = "mohalla " + name + " {\n";
        for (const auto &s : body)
        {
            if (s) res += "  " + s->toString() + "\n";
        }
        res += "}";
        return res;
    }
};

class EnumDeclStmt : public Stmt
{
public:
    std::string name;
    std::vector<EnumMember> members;

    EnumDeclStmt(std::string name, std::vector<EnumMember> members, int line = 0)
        : Stmt(line), name(std::move(name)), members(std::move(members)) {}

    std::string toString() const override
    {
        return "avtar " + name + " { ... };";
    }
};

// ==========================================
// Advanced Pattern Matching AST Nodes
// ==========================================

enum class PatternType {
    WILDCARD,
    LITERAL,
    VARIABLE,
    VARIANT,
    LIST
};

class MemePattern {
public:
    int line = 0;
    explicit MemePattern(int line = 0) : line(line) {}
    virtual ~MemePattern() = default;
    virtual PatternType getType() const = 0;
    virtual std::string toString() const = 0;
};

class WildcardPattern : public MemePattern {
public:
    explicit WildcardPattern(int line = 0) : MemePattern(line) {}
    PatternType getType() const override { return PatternType::WILDCARD; }
    std::string toString() const override { return "_"; }
};

class LiteralPattern : public MemePattern {
public:
    std::unique_ptr<Expr> literalExpr;
    explicit LiteralPattern(std::unique_ptr<Expr> expr, int line = 0)
        : MemePattern(line), literalExpr(std::move(expr)) {}
    PatternType getType() const override { return PatternType::LITERAL; }
    std::string toString() const override { return literalExpr ? literalExpr->toString() : ""; }
};

class VariablePattern : public MemePattern {
public:
    std::string name;
    explicit VariablePattern(std::string name, int line = 0)
        : MemePattern(line), name(std::move(name)) {}
    PatternType getType() const override { return PatternType::VARIABLE; }
    std::string toString() const override { return name; }
};

class VariantPattern : public MemePattern {
public:
    std::string variantName;
    std::vector<std::shared_ptr<MemePattern>> subPatterns;
    VariantPattern(std::string name, std::vector<std::shared_ptr<MemePattern>> subs, int line = 0)
        : MemePattern(line), variantName(std::move(name)), subPatterns(std::move(subs)) {}
    PatternType getType() const override { return PatternType::VARIANT; }
    std::string toString() const override {
        std::string s = variantName;
        if (!subPatterns.empty()) {
            s += "(";
            for (size_t i = 0; i < subPatterns.size(); ++i) {
                if (i > 0) s += ", ";
                s += subPatterns[i]->toString();
            }
            s += ")";
        }
        return s;
    }
};

class ListPattern : public MemePattern {
public:
    std::vector<std::shared_ptr<MemePattern>> elements;
    bool hasRest = false;
    std::string restName;
    ListPattern(std::vector<std::shared_ptr<MemePattern>> elems, bool hasRest = false, std::string restName = "", int line = 0)
        : MemePattern(line), elements(std::move(elems)), hasRest(hasRest), restName(std::move(restName)) {}
    PatternType getType() const override { return PatternType::LIST; }
    std::string toString() const override {
        std::string s = "[";
        for (size_t i = 0; i < elements.size(); ++i) {
            if (i > 0) s += ", ";
            s += elements[i]->toString();
        }
        if (hasRest) {
            if (!elements.empty()) s += ", ";
            s += "..." + restName;
        }
        s += "]";
        return s;
    }
};

struct JaanchArm {
    std::shared_ptr<MemePattern> pattern; // nullptr if fallback (warna/baaki/default)
    std::unique_ptr<Expr> guard;       // nullptr if no guard ('agar' / 'if')
    std::unique_ptr<Expr> bodyExpr;    // expression arm body
    std::unique_ptr<Stmt> bodyStmt;    // block arm body: `{ ... }`
};

class JaanchExpr : public Expr {
public:
    std::unique_ptr<Expr> target;
    std::vector<JaanchArm> arms;
    std::unique_ptr<Expr> defaultBodyExpr;
    std::unique_ptr<Stmt> defaultBodyStmt;

    JaanchExpr(std::unique_ptr<Expr> target,
               std::vector<JaanchArm> arms,
               std::unique_ptr<Expr> defaultBodyExpr = nullptr,
               std::unique_ptr<Stmt> defaultBodyStmt = nullptr,
               int line = 0)
        : Expr(line),
          target(std::move(target)),
          arms(std::move(arms)),
          defaultBodyExpr(std::move(defaultBodyExpr)),
          defaultBodyStmt(std::move(defaultBodyStmt)) {}

    std::string toString() const override {
        return "jaanch " + (target ? target->toString() : "") + " { ... }";
    }
};

class JaanchStmt : public Stmt
{
public:
    std::unique_ptr<Expr> target;
    std::vector<JaanchArm> arms;
    std::unique_ptr<Stmt> defaultArm;

    JaanchStmt(std::unique_ptr<Expr> target,
               std::vector<JaanchArm> arms,
               std::unique_ptr<Stmt> defaultArm = nullptr,
               int line = 0)
        : Stmt(line), target(std::move(target)), arms(std::move(arms)), defaultArm(std::move(defaultArm)) {}

    std::string toString() const override
    {
        return "jaanch " + (target ? target->toString() : "") + " { ... }";
    }
};

// ==========================================
// 3. Parser Class
// ==========================================

class Parser
{
public:
    [[noreturn]] void errorAt(int line, const std::string &message) const
    {
        throw std::runtime_error("Line " + std::to_string(line) + ": " + message);
    }

    [[noreturn]] void errorAt(const Token &token, const std::string &message) const
    {
        errorAt(token.line, message);
    }

    Parser(const std::vector<Token> &tokens) : tokens(tokens), current(0) {}

    bool isPotentialTagName(const Token &tok) const
    {
        if (tok.type == TokenType::END_OF_FILE || tok.type == TokenType::ERROR) return false;
        if (tok.value.empty()) return false;
        char c = tok.value[0];
        return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
    }

    bool isIdentifierOrVal(const Token &tok) const
    {
        return tok.type == TokenType::IDENTIFIER ||
               (tok.type == TokenType::DEREF && tok.value == "val") ||
               (tok.type == TokenType::ADDRESS_OF && (tok.value == "ptr" || tok.value == "addr")) ||
               (tok.type == TokenType::UDHAAR && tok.value == "ref") ||
               (tok.type == TokenType::UDHAAR_BADAL && tok.value == "mut_ref") ||
               (tok.type == TokenType::POOCH && tok.value == "input");
    }

    bool isFunctionName(const Token &tok) const
    {
        return tok.type == TokenType::IDENTIFIER ||
               tok.type == TokenType::DROP ||
               (tok.type == TokenType::DEREF && (tok.value == "val" || tok.value == "deref")) ||
               (tok.type == TokenType::ADDRESS_OF && (tok.value == "ptr" || tok.value == "addr"));
    }

    bool canStartUnaryOperand(const Token &tok) const
    {
        return tok.type == TokenType::IDENTIFIER ||
               tok.type == TokenType::ADDRESS_OF ||
               tok.type == TokenType::DEREF;
    }

    // Parse an entire program (list of statements until END_OF_FILE)
    std::vector<std::unique_ptr<Stmt>> parseProgram()
    {
        std::vector<std::unique_ptr<Stmt>> statements;

        while (peek().type != TokenType::END_OF_FILE)
        {
            if (peek().type == TokenType::SEMICOLON)
            {
                current++;
                continue;
            }
            statements.push_back(parseStatement());
        }

        return statements;
    }

    // Single point of dispatch (and single entry-level ERROR check) for statements
    std::unique_ptr<Stmt> parseStatement()
    {
        checkNotError();

        switch (peek().type)
        {
        case TokenType::LE:
        case TokenType::OWNED:
        case TokenType::MUT:
        case TokenType::CONST:
            return parseVarDecl();
        case TokenType::OVERLOAD:
        {
            current++; // consume 'bahuroop' / 'aur_ek' / 'overload' / 'multimethod'
            checkNotError();
            bool isAsync = false;
            if (peek().type == TokenType::ASYNC)
            {
                isAsync = true;
                current++;
                checkNotError();
            }
            if (peek().type == TokenType::KAR)
            {
                current++;
                checkNotError();
            }
            return parseFuncDeclStmt(isAsync, true);
        }
        case TokenType::DROP:
            return parseDropStmt();
        case TokenType::SCOPE:
            return parseScopeBlockStmt();
        case TokenType::DIKHA:
            if (peekNext().type == TokenType::DOT)
            {
                return parseExprStmt();
            }
            return parsePrintStmt();
        case TokenType::OPEN_BRACE:
            return parseBlockStmt();
        case TokenType::DEKH:
        case TokenType::ELIF:
            return parseIfStmt();
        case TokenType::JABTAK:
            return parseWhileStmt();
        case TokenType::WAPAS:
            return parseReturnStmt();
        case TokenType::BAS:
            return parseBreakStmt();
        case TokenType::CHAL:
            return parseContinueStmt();
        case TokenType::KAR:
            if (isFunctionName(peekNext()))
            {
                return parseFuncDeclStmt();
            }
            return parseExprStmt();
        case TokenType::GHOOM:
            return parseGhoomStmt();
        case TokenType::SANCHA:
            return parseClassDeclStmt();
        case TokenType::ASYNC:
        {
            current++; // consume 'async' / 'side_mein'
            if (peek().type == TokenType::KAR && isFunctionName(peekNext()))
            {
                return parseFuncDeclStmt(true);
            }
            if (isFunctionName(peek()) && peekNext().type == TokenType::OPEN_PAREN)
            {
                return parseFuncDeclStmt(true);
            }
            current--; // backtrack
            return parseExprStmt();
        }
        case TokenType::PANGA:
            return parseTryCatchStmt();
        case TokenType::CHILLAO:
            return parseThrowStmt();
        case TokenType::IMPORT:
            return parseImportStmt();
        case TokenType::PACKAGE:
            return parsePackageDeclStmt();
        case TokenType::EXPORT:
            return parseExportStmt();
        case TokenType::NAMESPACE:
            return parseNamespaceDeclStmt();
        case TokenType::AVTAR:
            return parseEnumDeclStmt();
        case TokenType::JAANCH:
            return parseJaanchStmt();
        case TokenType::OPEN_SQUARE_BRACKET:
            if (isArrayDestructureAhead())
            {
                return parseArrayDestructureStmtWithoutKeyword();
            }
            return parseExprStmt();
        default:
            if (peek().type == TokenType::IDENTIFIER && peekNext().type == TokenType::COLON)
            {
                return parseTypedVarDeclWithoutKeyword();
            }
            return parseExprStmt();
        }
    }

    std::shared_ptr<Type> parseTypeExpression()
    {
        checkNotError();
        Token token = peek();

        if (token.type == TokenType::KUCH_NHI)
        {
            current++;
            return Type::Nalla();
        }

        if (token.type == TokenType::DIKHA && token.value == "bol")
        {
            current++;
            return Type::Bol();
        }

        if (token.type == TokenType::HAN || token.type == TokenType::NHI)
        {
            current++;
            return Type::Sach();
        }

        if (token.type == TokenType::KAR)
        {
            current++;
            checkNotError();
            std::vector<std::shared_ptr<Type>> paramTypes;
            std::shared_ptr<Type> retType = Type::Any();
            if (peek().type == TokenType::OPEN_PAREN)
            {
                current++; // consume '('
                while (peek().type != TokenType::CLOSE_PAREN && peek().type != TokenType::END_OF_FILE)
                {
                    paramTypes.push_back(parseTypeExpression());
                    checkNotError();
                    if (peek().type == TokenType::COMMA)
                    {
                        current++;
                    }
                    else
                    {
                        break;
                    }
                }
                consumeCloseParen("jugaad type parameters");
                checkNotError();
                if (peek().type == TokenType::THIN_ARROW || peek().type == TokenType::ARROW || peek().type == TokenType::COLON)
                {
                    current++;
                    retType = parseTypeExpression();
                }
            }
            return Type::Function(std::move(paramTypes), retType);
        }

        if (token.type == TokenType::OPEN_PAREN)
        {
            current++; // consume '('
            std::vector<std::shared_ptr<Type>> paramTypes;
            while (peek().type != TokenType::CLOSE_PAREN && peek().type != TokenType::END_OF_FILE)
            {
                paramTypes.push_back(parseTypeExpression());
                checkNotError();
                if (peek().type == TokenType::COMMA)
                {
                    current++;
                }
                else
                {
                    break;
                }
            }
            consumeCloseParen("function type parameters");
            checkNotError();
            std::shared_ptr<Type> retType = Type::Any();
            if (peek().type == TokenType::THIN_ARROW || peek().type == TokenType::ARROW || peek().type == TokenType::COLON)
            {
                current++;
                retType = parseTypeExpression();
            }
            return Type::Function(std::move(paramTypes), retType);
        }

        if (token.type == TokenType::MULTIPLY)
        {
            current++; // consume '*'
            bool isMut = false;
            if (peek().type == TokenType::MUT ||
                (peek().type == TokenType::IDENTIFIER && (peek().value == "badal" || peek().value == "mut")))
            {
                isMut = true;
                current++;
            }
            else if (peek().type == TokenType::CONST ||
                     (peek().type == TokenType::IDENTIFIER && (peek().value == "atal" || peek().value == "const")))
            {
                isMut = false;
                current++;
            }
            auto pointeeType = parseTypeExpression();
            return Type::pointer(pointeeType, isMut);
        }

        if (token.type == TokenType::ADDRESS_OF ||
            (token.type == TokenType::IDENTIFIER &&
             (token.value == "patta" || token.value == "thikana" || token.value == "addr" || token.value == "ptr" || token.value == "pointer")))
        {
            current++; // consume type token
            checkNotError();
            if (peek().type == TokenType::OPEN_SQUARE_BRACKET)
            {
                current++; // consume '['
                bool isMut = false;
                if (peek().type == TokenType::MUT ||
                    (peek().type == TokenType::IDENTIFIER && (peek().value == "badal" || peek().value == "mut")))
                {
                    isMut = true;
                    current++;
                }
                else if (peek().type == TokenType::CONST ||
                         (peek().type == TokenType::IDENTIFIER && (peek().value == "atal" || peek().value == "const")))
                {
                    isMut = false;
                    current++;
                }
                auto pointeeType = parseTypeExpression();
                checkNotError();
                if (peek().type != TokenType::CLOSE_SQUARE_BRACKET)
                {
                    errorAt(peek(), "Pointer type mein ']' band karna bhool gaye! Mila: '" + peek().value + "'");
                }
                current++; // consume ']'
                return Type::pointer(pointeeType, isMut);
            }
            if (token.type == TokenType::ADDRESS_OF &&
                (peek().type == TokenType::MUT ||
                 (peek().type == TokenType::IDENTIFIER && (peek().value == "badal" || peek().value == "mut"))))
            {
                current++;
                auto pointeeType = parseTypeExpression();
                return Type::pointer(pointeeType, true);
            }
            if (token.type == TokenType::ADDRESS_OF &&
                peek().type != TokenType::SEMICOLON && peek().type != TokenType::ASSIGN &&
                peek().type != TokenType::COMMA && peek().type != TokenType::CLOSE_PAREN &&
                peek().type != TokenType::CLOSE_SQUARE_BRACKET && peek().type != TokenType::COLON)
            {
                auto pointeeType = parseTypeExpression();
                return Type::pointer(pointeeType, false);
            }
            return Type::pointer(Type::Any(), false);
        }

        if (token.type != TokenType::IDENTIFIER)
        {
            errorAt(token, "Valid type ka naam chahiye! Mila: '" + token.value + "'");
        }

        std::string name = token.value;
        current++; // consume identifier

        if (name == "byte" || name == "chota_ank" || name == "u8")
        {
            return Type::Byte();
        }
        if (name == "int32" || name == "ank32" || name == "i32")
        {
            return Type::Int32();
        }
        if (name == "ank" || name == "bina_point" || name == "sankhya" || name == "int" || name == "long" || name == "i64" || name == "bada_ank")
        {
            return Type::Ank();
        }
        if (name == "float32" || name == "f32")
        {
            return Type::Float32();
        }
        if (name == "point" || name == "point_wala" || name == "dashamlav" || name == "float" || name == "double" || name == "f64")
        {
            return Type::Point();
        }
        if (name == "char" || name == "akshar")
        {
            return Type::Char();
        }
        if (name == "bytes" || name == "kaccha_data" || name == "bytearray")
        {
            return Type::Bytes();
        }
        if (name == "set" || name == "jhund" || name == "mandali")
        {
            checkNotError();
            if (peek().type == TokenType::OPEN_SQUARE_BRACKET)
            {
                current++; // consume '['
                auto elemType = parseTypeExpression();
                checkNotError();
                if (peek().type != TokenType::CLOSE_SQUARE_BRACKET)
                {
                    errorAt(peek(), "Set type mein ']' band karna bhool gaye! Mila: '" + peek().value + "'");
                }
                current++; // consume ']'
                return Type::Set(elemType);
            }
            return Type::Set(Type::Any());
        }
        if (name == "tuple" || name == "jodi")
        {
            checkNotError();
            if (peek().type == TokenType::OPEN_SQUARE_BRACKET)
            {
                current++; // consume '['
                std::vector<std::shared_ptr<Type>> elemTypes;
                while (peek().type != TokenType::CLOSE_SQUARE_BRACKET && peek().type != TokenType::END_OF_FILE)
                {
                    elemTypes.push_back(parseTypeExpression());
                    checkNotError();
                    if (peek().type == TokenType::COMMA)
                    {
                        current++;
                    }
                    else break;
                }
                if (peek().type != TokenType::CLOSE_SQUARE_BRACKET)
                {
                    errorAt(peek(), "Tuple type mein ']' band karna bhool gaye! Mila: '" + peek().value + "'");
                }
                current++; // consume ']'
                return Type::Tuple(std::move(elemTypes));
            }
            return Type::Tuple();
        }
        if (name == "bol" || name == "shabd" || name == "vakya" || name == "baat" || name == "string" || name == "str")
        {
            return Type::Bol();
        }
        if (name == "sach" || name == "faisla" || name == "vibe_check" || name == "bool")
        {
            return Type::Sach();
        }
        if (name == "nalla" || name == "kuch_nhi" || name == "khali" || name == "void" || name == "null")
        {
            return Type::Nalla();
        }
        if (name == "har_kism" || name == "joker" || name == "sab" || name == "kuch_bhi" || name == "any")
        {
            return Type::Any();
        }

        if (name == "soochi" || name == "tolli" || name == "gang" || name == "list")
        {
            checkNotError();
            if (peek().type == TokenType::OPEN_SQUARE_BRACKET)
            {
                current++; // consume '['
                auto elemType = parseTypeExpression();
                checkNotError();
                if (peek().type != TokenType::CLOSE_SQUARE_BRACKET)
                {
                    errorAt(peek(), "List type mein ']' band karna bhool gaye! Mila: '" + peek().value + "'");
                }
                current++; // consume ']'
                return Type::List(elemType);
            }
            return Type::List(Type::Any());
        }

        if (name == "khata" || name == "kundli" || name == "map")
        {
            checkNotError();
            if (peek().type == TokenType::OPEN_SQUARE_BRACKET)
            {
                current++; // consume '['
                auto keyType = parseTypeExpression();
                checkNotError();
                if (peek().type != TokenType::COMMA)
                {
                    errorAt(peek(), "Map type mein key aur value type ke beech ',' lagao! Mila: '" + peek().value + "'");
                }
                current++; // consume ','
                auto valType = parseTypeExpression();
                checkNotError();
                if (peek().type != TokenType::CLOSE_SQUARE_BRACKET)
                {
                    errorAt(peek(), "Map type mein ']' band karna bhool gaye! Mila: '" + peek().value + "'");
                }
                current++; // consume ']'
                return Type::Map(keyType, valType);
            }
            return Type::Map(Type::Any(), Type::Any());
        }

        if (name == "jugaad" || name == "kar" || name == "fn" || name == "function")
        {
            checkNotError();
            std::vector<std::shared_ptr<Type>> paramTypes;
            std::shared_ptr<Type> retType = Type::Any();
            if (peek().type == TokenType::OPEN_PAREN)
            {
                current++; // consume '('
                while (peek().type != TokenType::CLOSE_PAREN && peek().type != TokenType::END_OF_FILE)
                {
                    paramTypes.push_back(parseTypeExpression());
                    checkNotError();
                    if (peek().type == TokenType::COMMA)
                    {
                        current++;
                    }
                    else
                    {
                        break;
                    }
                }
                consumeCloseParen("jugaad type parameters");
                checkNotError();
                if (peek().type == TokenType::THIN_ARROW || peek().type == TokenType::ARROW || peek().type == TokenType::COLON)
                {
                    current++;
                    retType = parseTypeExpression();
                }
            }
            return Type::Function(std::move(paramTypes), retType);
        }

        return Type::Class(name);
    }

    // 1. `le x = 5;` or `le x;` or `pakad [a, b, *baaki] = expr;` or `pakad {naam, umar} = expr;`
    std::unique_ptr<Stmt> parseVarDecl()
    {
        int line = peek().line;
        if (peek().type == TokenType::LE)
        {
            current++; // consume 'le'
            checkNotError();
        }

        bool isOwned = false;
        bool isMut = false;
        bool isConst = false;

        while (peek().type == TokenType::OWNED ||
               peek().type == TokenType::MUT ||
               peek().type == TokenType::CONST)
        {
            if (peek().type == TokenType::OWNED) isOwned = true;
            else if (peek().type == TokenType::MUT) isMut = true;
            else if (peek().type == TokenType::CONST) isConst = true;
            current++;
            checkNotError();
        }

        if (peek().type == TokenType::OPEN_SQUARE_BRACKET)
        {
            current++; // consume '['
            std::vector<DestructureItem> items;
            bool seenVariadic = false;
            while (peek().type != TokenType::CLOSE_SQUARE_BRACKET && peek().type != TokenType::END_OF_FILE)
            {
                checkNotError();
                if (seenVariadic)
                {
                    errorAt(peek(), "Destructuring mein '*baaki' ke baad aur koi variable nahi aa sakta!");
                }
                bool isVar = false;
                if (peek().type == TokenType::MULTIPLY)
                {
                    isVar = true;
                    current++; // consume '*'
                    checkNotError();
                }
                if (!isIdentifierOrVal(peek()))
                {
                    errorAt(peek(), "Destructuring mein variable ka naam chahiye! Mila: '" + peek().value + "'");
                }
                std::string name = peek().value;
                current++; // consume identifier
                if (isVar)
                {
                    seenVariadic = true;
                }
                items.push_back(DestructureItem{name, isVar});

                checkNotError();
                if (peek().type == TokenType::COMMA)
                {
                    current++; // consume ','
                    checkNotError();
                    if (peek().type == TokenType::CLOSE_SQUARE_BRACKET)
                    {
                        break; // trailing comma
                    }
                }
                else
                {
                    break;
                }
            }
            if (peek().type != TokenType::CLOSE_SQUARE_BRACKET)
            {
                errorAt(peek(), "Destructuring bracket ']' band karna bhool gaye!");
            }
            current++; // consume ']'

            checkNotError();
            if (peek().type != TokenType::ASSIGN)
            {
                errorAt(peek(), "Destructuring mein '=' lagana zaroori hai!");
            }
            current++; // consume '='

            auto initializer = parseAssignment();
            consumeSemicolon();
            return std::make_unique<ArrayDestructureStmt>(std::move(items), std::move(initializer), line);
        }

        if (peek().type == TokenType::OPEN_BRACE)
        {
            current++; // consume '{'
            std::vector<std::string> keys;
            while (peek().type != TokenType::CLOSE_BRACE && peek().type != TokenType::END_OF_FILE)
            {
                checkNotError();
                if (!isIdentifierOrVal(peek()))
                {
                    errorAt(peek(), "Map destructuring mein variable ka naam chahiye! Mila: '" + peek().value + "'");
                }
                keys.push_back(peek().value);
                current++; // consume identifier

                checkNotError();
                if (peek().type == TokenType::COMMA)
                {
                    current++; // consume ','
                    checkNotError();
                    if (peek().type == TokenType::CLOSE_BRACE)
                    {
                        break; // trailing comma
                    }
                }
                else
                {
                    break;
                }
            }
            if (peek().type != TokenType::CLOSE_BRACE)
            {
                errorAt(peek(), "Map destructuring brace '}' band karna bhool gaye!");
            }
            current++; // consume '}'

            checkNotError();
            if (peek().type != TokenType::ASSIGN)
            {
                errorAt(peek(), "Map destructuring mein '=' lagana zaroori hai!");
            }
            current++; // consume '='

            auto initializer = parseAssignment();
            consumeSemicolon();
            return std::make_unique<MapDestructureStmt>(std::move(keys), std::move(initializer), line);
        }

        if (!isIdentifierOrVal(peek()))
        {
            errorAt(peek(),
                "Hawa pakdoge kya? 'pakad' ke baad variable ka naam toh likho! "
                "Mila: '" + peek().value + "'");
        }

        std::string varName = peek().value;
        current++; // consume variable name

        std::shared_ptr<Type> typeAnnotation = nullptr;
        checkNotError();
        if (peek().type == TokenType::COLON)
        {
            current++; // consume ':'
            typeAnnotation = parseTypeExpression();
        }

        checkNotError();
        std::unique_ptr<Expr> initializer = nullptr;
        if (peek().type == TokenType::ASSIGN)
        {
            current++; // consume '='
            initializer = parseAssignment();
        }

        consumeSemicolon();
        return std::make_unique<VarDeclStmt>(varName, std::move(initializer), typeAnnotation, line, isOwned, isMut, isConst);
    }

    bool isArrayDestructureAhead()
    {
        if (peek().type != TokenType::OPEN_SQUARE_BRACKET) return false;
        size_t idx = current;
        int depth = 0;
        while (idx < tokens.size() && tokens[idx].type != TokenType::END_OF_FILE)
        {
            if (tokens[idx].type == TokenType::OPEN_SQUARE_BRACKET) depth++;
            else if (tokens[idx].type == TokenType::CLOSE_SQUARE_BRACKET)
            {
                depth--;
                if (depth == 0)
                {
                    if (idx + 1 < tokens.size() && tokens[idx + 1].type == TokenType::ASSIGN)
                    {
                        return true;
                    }
                    return false;
                }
            }
            else if (tokens[idx].type == TokenType::SEMICOLON)
            {
                return false;
            }
            idx++;
        }
        return false;
    }

    std::unique_ptr<Stmt> parseArrayDestructureStmtWithoutKeyword()
    {
        int line = peek().line;
        current++; // consume '['
        std::vector<DestructureItem> items;
        bool seenVariadic = false;
        while (peek().type != TokenType::CLOSE_SQUARE_BRACKET && peek().type != TokenType::END_OF_FILE)
        {
            checkNotError();
            if (seenVariadic)
            {
                errorAt(peek(), "Destructuring mein '*baaki' ke baad aur koi variable nahi aa sakta!");
            }
            bool isVar = false;
            if (peek().type == TokenType::MULTIPLY)
            {
                isVar = true;
                current++; // consume '*'
                checkNotError();
            }
            if (peek().type != TokenType::IDENTIFIER)
            {
                errorAt(peek(), "Destructuring mein variable ka naam chahiye! Mila: '" + peek().value + "'");
            }
            std::string name = peek().value;
            current++; // consume identifier
            if (isVar)
            {
                seenVariadic = true;
            }
            items.push_back(DestructureItem{name, isVar});

            checkNotError();
            if (peek().type == TokenType::COMMA)
            {
                current++; // consume ','
                checkNotError();
                if (peek().type == TokenType::CLOSE_SQUARE_BRACKET)
                {
                    break; // trailing comma
                }
            }
            else
            {
                break;
            }
        }
        if (peek().type != TokenType::CLOSE_SQUARE_BRACKET)
        {
            errorAt(peek(), "Destructuring bracket ']' band karna bhool gaye!");
        }
        current++; // consume ']'

        checkNotError();
        if (peek().type != TokenType::ASSIGN)
        {
            errorAt(peek(), "Destructuring mein '=' lagana zaroori hai!");
        }
        current++; // consume '='

        auto initializer = parseAssignment();
        consumeSemicolon();
        return std::make_unique<ArrayDestructureStmt>(std::move(items), std::move(initializer), line);
    }

    std::unique_ptr<Stmt> parseTypedVarDeclWithoutKeyword()
    {
        int line = peek().line;
        std::string varName = peek().value;
        current++; // consume identifier
        current++; // consume ':'
        auto typeAnnotation = parseTypeExpression();
        std::unique_ptr<Expr> initializer = nullptr;
        checkNotError();
        if (peek().type == TokenType::ASSIGN)
        {
            current++; // consume '='
            initializer = parseAssignment();
        }
        consumeSemicolon();
        return std::make_unique<VarDeclStmt>(varName, std::move(initializer), typeAnnotation, line);
    }

    // 2. `dikha expr1, expr2;`
    std::unique_ptr<Stmt> parsePrintStmt()
    {
        int line = peek().line;
        current++; // consume 'dikha'
        checkNotError();
        rejectAssignment("dikha");

        if (peek().type == TokenType::SEMICOLON)
        {
            errorAt(line,
                "'dikha' ke baad kuch toh likh! Khali ';' mein Mr. India dikhaun kya?");
        }

        // If `dikha` starts with `(`, check whether the user wrote `dikha(a, b)`.
        // We validate the full argument list and closing `)` FIRST so syntax errors
        // like `dikha(1, 2 3);` report the bracket error on '3' before the `dikha` form hint.
        if (peek().type == TokenType::OPEN_PAREN)
        {
            size_t savedPos = current;
            current++; // tentatively consume '('
            checkNotError();

            if (peek().type == TokenType::CLOSE_PAREN)
            {
                errorAt(peek(),
                    "'dikha()' khali kyun chhod diya? Andar kuch masala toh daal!");
            }

            std::unique_ptr<Expr> firstArg = parseTernary();
            rejectAssignment("dikha");

            if (peek().type == TokenType::COMMA)
            {
                std::vector<std::unique_ptr<Expr>> dummyArgs;
                dummyArgs.push_back(std::move(firstArg));
                parseRemainingCommaArgs(dummyArgs, "dikha");

                // Confirm ')' first! On `dikha(1, 2 3);`, this throws the bracket error on '3'.
                consumeCloseParen("dikha ka bracket '('");

                // Once ')' is confirmed, reject parenthesized multi-arg `dikha(a, b)`
                errorAt(line,
                    "'dikha(a, b)' mein brackets allowed nahi hain! "
                    "Brackets hata ke seedha 'dikha a, b;' likh.");
            }

            // Not a multi-arg `dikha(a, b)` -> rewind to '(' and parse normally
            current = savedPos;
        }

        std::vector<std::unique_ptr<Expr>> args;
        std::unique_ptr<Expr> endExpr = nullptr;
        std::unique_ptr<Expr> sepExpr = nullptr;
        std::unique_ptr<Expr> styleExpr = nullptr;
        bool noNewline = false;

        auto isNamedOption = [](const std::string &val) {
            return val == "end" || val == "ant" || val == "khatam" ||
                   val == "sep" || val == "alag" || val == "judo" ||
                   val == "color" || val == "rang" || val == "style";
        };

        while (peek().type != TokenType::SEMICOLON && peek().type != TokenType::END_OF_FILE)
        {
            if (peek().type == TokenType::IDENTIFIER && (peek().value == "bina_line" || peek().value == "no_newline"))
            {
                current++;
                noNewline = true;
                if (peek().type == TokenType::COMMA)
                {
                    current++;
                }
                continue;
            }

            if ((peek().type == TokenType::IDENTIFIER || peek().type == TokenType::DROP) && peekNext().type == TokenType::ASSIGN)
            {
                std::string optName = peek().value;
                if (isNamedOption(optName))
                {
                    current += 2; // consume optName and '='
                    auto expr = parseTernary();
                    if (optName == "end" || optName == "ant" || optName == "khatam")
                    {
                        endExpr = std::move(expr);
                    }
                    else if (optName == "sep" || optName == "alag" || optName == "judo")
                    {
                        sepExpr = std::move(expr);
                    }
                    else if (optName == "color" || optName == "rang" || optName == "style")
                    {
                        styleExpr = std::move(expr);
                    }
                    if (peek().type == TokenType::COMMA)
                    {
                        current++;
                    }
                    continue;
                }
            }

            args.push_back(parseTernary());
            rejectAssignment("dikha");

            if (peek().type == TokenType::IDENTIFIER && (peek().value == "bina_line" || peek().value == "no_newline"))
            {
                current++;
                noNewline = true;
            }

            if (peek().type == TokenType::COMMA)
            {
                current++;
            }
            else
            {
                break;
            }
        }

        if (peek().type == TokenType::IDENTIFIER && (peek().value == "bina_line" || peek().value == "no_newline"))
        {
            current++;
            noNewline = true;
        }

        if (args.empty() && !endExpr && !sepExpr && !styleExpr && !noNewline)
        {
            errorAt(line,
                "'dikha' ke baad kuch toh likh! Khali ';' mein Mr. India dikhaun kya?");
        }

        consumeSemicolon();
        return std::make_unique<PrintStmt>(std::move(args), std::move(endExpr), std::move(sepExpr), std::move(styleExpr), noNewline, line);
    }

    // 3. `{ stmt1; stmt2; }` (No trailing semicolon)
    std::unique_ptr<Stmt> parseBlockStmt()
    {
        checkNotError();
        if (peek().type != TokenType::OPEN_BRACE)
        {
            errorAt(peek(),
                "Block shuru karne ke liye '{' lagana padta hai! "
                "Mila: '" + peek().value + "'");
        }
        int openBraceLine = peek().line;
        current++; // consume '{'

        std::vector<std::unique_ptr<Stmt>> statements;

        while (peek().type != TokenType::CLOSE_BRACE &&
               peek().type != TokenType::END_OF_FILE)
        {
            if (peek().type == TokenType::SEMICOLON)
            {
                current++;
                continue;
            }
            statements.push_back(parseStatement());
        }

        checkNotError();
        if (peek().type == TokenType::END_OF_FILE)
        {
            errorAt(openBraceLine,
                "Daya, darwaza खुला chhod diya! '{' khola hai toh '}' bhi laga.");
        }

        current++; // consume '}'
        return std::make_unique<BlockStmt>(std::move(statements), openBraceLine);
    }

    std::unique_ptr<Stmt> parseScopeBlockStmt()
    {
        int line = peek().line;
        current++; // consume 'daayra' / 'scope' / 'seema' / 'elaqa' / 'lifetime'
        checkNotError();

        // If followed by semicolon, newline, or '}', treat as continue statement
        if (peek().type == TokenType::SEMICOLON ||
            peek().type == TokenType::CLOSE_BRACE ||
            peek().type == TokenType::END_OF_FILE ||
            (current > 0 && peek().line > tokens[current - 1].line))
        {
            consumeSemicolon();
            return std::make_unique<ContinueStmt>(line);
        }

        if (peek().type != TokenType::OPEN_BRACE)
        {
            errorAt(peek(),
                "Daayra (scope) ke baad block '{' lagana padta hai! Mila: '" + peek().value + "'");
        }

        auto block = parseBlockStmt();
        auto *b = static_cast<BlockStmt *>(block.get());
        return std::make_unique<ScopeBlockStmt>(std::move(b->statements), line);
    }

    std::unique_ptr<Stmt> parseDropStmt()
    {
        int line = peek().line;
        current++; // consume 'khatam' / 'drop' / 'visarjan' / 'dispose' / 'alvida'
        checkNotError();

        // If followed by semicolon, newline, or '}', treat as break statement
        if (peek().type == TokenType::SEMICOLON ||
            peek().type == TokenType::CLOSE_BRACE ||
            peek().type == TokenType::END_OF_FILE ||
            (current > 0 && peek().line > tokens[current - 1].line))
        {
            consumeSemicolon();
            return std::make_unique<BreakStmt>(line);
        }

        auto expr = parseAssignment();
        consumeSemicolon();
        return std::make_unique<DropStmt>(std::move(expr), line);
    }

    // 4. `dekh cond { ... } warna dekh cond2 { ... } warna { ... }` or `agar ... warna ...` / `if ... elif ... else`
    std::unique_ptr<Stmt> parseIfStmt()
    {
        int line = peek().line;
        current++; // consume 'dekh' / 'agar' / 'if' / 'elif'

        std::unique_ptr<Expr> condition = parseTernary();
        rejectAssignment("dekh ki condition");

        std::unique_ptr<Stmt> thenBranch = parseBlockStmt();

        checkNotError();
        std::unique_ptr<Stmt> elseBranch = nullptr;
        if (peek().type == TokenType::WARNA)
        {
            current++; // consume 'warna' / 'else'
            checkNotError();

            if (peek().type == TokenType::DEKH || peek().type == TokenType::ELIF)
            {
                elseBranch = parseIfStmt();
            }
            else
            {
                elseBranch = parseBlockStmt();
            }
        }
        else if (peek().type == TokenType::ELIF)
        {
            elseBranch = parseIfStmt();
        }

        return std::make_unique<IfStmt>(
            std::move(condition),
            std::move(thenBranch),
            std::move(elseBranch),
            line);
    }

    // 5. `jabtak cond { ... }`
    std::unique_ptr<Stmt> parseWhileStmt()
    {
        int line = peek().line;
        current++; // consume 'jabtak'

        std::unique_ptr<Expr> condition = parseTernary();
        rejectAssignment("jabtak ki condition");

        std::unique_ptr<Stmt> body = parseBlockStmt();
        return std::make_unique<WhileStmt>(std::move(condition), std::move(body), line);
    }

    // 6. `wapas expr;` or `wapas;` / `de expr`
    std::unique_ptr<Stmt> parseReturnStmt()
    {
        int line = peek().line;
        current++; // consume 'wapas'
        checkNotError();

        std::unique_ptr<Expr> value = nullptr;
        if (peek().type != TokenType::SEMICOLON &&
            peek().type != TokenType::CLOSE_BRACE &&
            peek().type != TokenType::END_OF_FILE &&
            !(current > 0 && peek().line > tokens[current - 1].line))
        {
            value = parseTernary();
            rejectAssignment("wapas");
        }

        consumeSemicolon();
        return std::make_unique<ReturnStmt>(std::move(value), line);
    }

    // 7. `bas;`
    std::unique_ptr<Stmt> parseBreakStmt()
    {
        int line = peek().line;
        current++; // consume 'bas'
        consumeSemicolon();
        return std::make_unique<BreakStmt>(line);
    }

    // 8. `chal;`
    std::unique_ptr<Stmt> parseContinueStmt()
    {
        int line = peek().line;
        current++; // consume 'chal'
        consumeSemicolon();
        return std::make_unique<ContinueStmt>(line);
    }

    // 9. `kar funcName(a, b = 10, *rest) { ... }`
    std::unique_ptr<Stmt> parseFuncDeclStmt(bool isAsync = false, bool isOverload = false)
    {
        int line = peek().line;
        if (peek().type == TokenType::KAR)
        {
            current++; // consume 'kar'
            checkNotError();
        }

        if (!isFunctionName(peek()))
        {
            errorAt(peek(),
                "Bina naam ka jugaad? 'jugaad' ke baad function ka naam toh de! Mila: '" + peek().value + "'");
        }
        std::string funcName = peek().value;
        current++; // consume function name

        checkNotError();
        if (peek().type != TokenType::OPEN_PAREN)
        {
            errorAt(peek(),
                "Jugaad '" + funcName + "' ke baad '(' lagana bhool gaye! Bina bracket ke kaise chalega?");
        }
        current++; // consume '('

        std::vector<FuncParam> params = parseParameterList(funcName);
        consumeCloseParen("function '" + funcName + "' ke parameters");

        if (peek().type == TokenType::ASYNC)
        {
            isAsync = true;
            current++; // consume 'asar'
        }

        std::shared_ptr<Type> returnType = nullptr;
        checkNotError();
        if (peek().type == TokenType::COLON || peek().type == TokenType::THIN_ARROW || peek().type == TokenType::ARROW)
        {
            current++; // consume ':', '->', or '=>'
            returnType = parseTypeExpression();
        }

        std::unique_ptr<Stmt> body = parseBlockStmt();
        return std::make_unique<FuncDeclStmt>(funcName, std::move(params), std::move(body), returnType, isAsync, line, isOverload);
    }

    // 10. `ghoom i se 1 se 10 { ... }` (optional step: `ghoom i se 1 se 10 se 2 { ... }`)
    std::unique_ptr<Stmt> parseGhoomStmt()
    {
        int line = peek().line;
        current++; // consume 'ghoom'
        checkNotError();

        if (!isIdentifierOrVal(peek()))
        {
            errorAt(peek(),
                "Kisko ghumana hai? 'ghuma' ke baad loop variable ka naam toh likh! Mila: '" + peek().value + "'");
        }
        std::string varName = peek().value;
        current++; // consume variable name

        checkNotError();
        if (peek().type == TokenType::MEIN)
        {
            current++; // consume 'mein' / 'andar'
            std::unique_ptr<Expr> startOrCollection = parseLogicalOr();
            if (peek().type == TokenType::DOT_DOT)
            {
                current++; // consume '..'
                std::unique_ptr<Expr> endExpr = parseLogicalOr();
                std::unique_ptr<Stmt> body = parseBlockStmt();
                return std::make_unique<GhoomStmt>(varName, std::move(startOrCollection), std::move(endExpr), nullptr, std::move(body), line, false);
            }
            std::unique_ptr<Stmt> body = parseBlockStmt();
            return std::make_unique<GhumaMeinStmt>(varName, std::move(startOrCollection), std::move(body), line);
        }

        if (peek().type != TokenType::SE)
        {
            errorAt(peek(),
                "Chakkar aa jayega! 'ghuma " + varName + "' ke baad 'se' (range) ya 'mein' (collection) lagana zaroori hai (Jaise: ghuma " + varName + " se 1 se 10 ya ghuma item mein list)!");
        }
        current++; // consume 'se'

        std::unique_ptr<Expr> start = parseLogicalOr();
        checkNotError();

        if (peek().type != TokenType::SE)
        {
            errorAt(peek(),
                "Kahan tak ghumana hai? Range ke liye dusra 'se' lagao! (Jaise: ghuma " + varName + " se 1 se 10)");
        }
        current++; // consume 'se'

        std::unique_ptr<Expr> end = parseLogicalOr();
        checkNotError();

        std::unique_ptr<Expr> step = nullptr;
        if (peek().type == TokenType::SE)
        {
            current++; // consume optional 'se' for step
            step = parseLogicalOr();
        }

        std::unique_ptr<Stmt> body = parseBlockStmt();
        return std::make_unique<GhoomStmt>(varName, std::move(start), std::move(end), std::move(step), std::move(body), line);
    }

    // 11. `biradari Name [nepo SuperName] { jugaad method() { ... } }`
    std::unique_ptr<Stmt> parseClassDeclStmt()
    {
        int line = peek().line;
        current++; // consume 'sancha' / 'biradari' / 'class'
        checkNotError();

        if (peek().type != TokenType::IDENTIFIER)
        {
            errorAt(peek(),
                "Bina naam ki biradari? 'biradari' ke baad class ka naam toh likh! Mila: '" + peek().value + "'");
        }
        std::string className = peek().value;
        current++; // consume class name

        std::string superclass = "";
        checkNotError();
        if (peek().type == TokenType::SE || peek().type == TokenType::NEPO)
        {
            current++; // consume 'se' or 'nepo'
            checkNotError();
            if (peek().type != TokenType::IDENTIFIER)
            {
                errorAt(peek(), "Nepo kid kaunse khandaan ka hai? 'nepo' ke baad parent biradari ka naam chahiye!");
            }
            superclass = peek().value;
            current++; // consume superclass name
        }

        checkNotError();
        if (peek().type != TokenType::OPEN_BRACE)
        {
            errorAt(peek(),
                "Biradari ke andar ghusne ke liye '{' lagana padega! Mila: '" + peek().value + "'");
        }
        int openBraceLine = peek().line;
        current++; // consume '{'

        std::vector<std::unique_ptr<FuncDeclStmt>> methods;
        while (peek().type != TokenType::CLOSE_BRACE && peek().type != TokenType::END_OF_FILE)
        {
            checkNotError();
            if (peek().type == TokenType::OPERATOR)
            {
                methods.push_back(parseOperatorDeclStmt());
            }
            else if (peek().type == TokenType::OVERLOAD)
            {
                current++; // consume 'bahuroop' / 'overload'
                checkNotError();
                if (peek().type == TokenType::KAR)
                {
                    current++;
                }
                auto method = parseFuncDeclStmt(false, true);
                methods.push_back(std::unique_ptr<FuncDeclStmt>(static_cast<FuncDeclStmt *>(method.release())));
            }
            else if (peek().type == TokenType::KAR)
            {
                auto method = parseFuncDeclStmt();
                methods.push_back(std::unique_ptr<FuncDeclStmt>(static_cast<FuncDeclStmt *>(method.release())));
            }
            else if (isFunctionName(peek()) && peekNext().type == TokenType::OPEN_PAREN)
            {
                auto method = parseFuncDeclStmt();
                methods.push_back(std::unique_ptr<FuncDeclStmt>(static_cast<FuncDeclStmt *>(method.release())));
            }
            else
            {
                errorAt(peek(),
                    "Biradari ke andar sirf methods ('jugaad naam() { ... }') ya operators ('tantrik + (...) { ... }') chalte hain! Ye '" + peek().value + "' kahan se aaya?");
            }
        }

        if (peek().type == TokenType::END_OF_FILE)
        {
            errorAt(openBraceLine,
                "Khatam, tata, bye-bye! Biradari ka '{' khola par '}' band karna bhool gaye.");
        }
        current++; // consume '}'

        return std::make_unique<ClassDeclStmt>(className, superclass, std::move(methods), line);
    }

    std::unique_ptr<FuncDeclStmt> parseOperatorDeclStmt()
    {
        int line = peek().line;
        current++; // consume 'tantrik' / 'kriya' / 'operator' / 'op'
        checkNotError();

        std::string opSymbol = "";
        std::string methodName = "";

        // 1. Bracket indexing: '[]' and '[]='
        if (peek().type == TokenType::OPEN_SQUARE_BRACKET)
        {
            current++; // consume '['
            checkNotError();
            if (peek().type != TokenType::CLOSE_SQUARE_BRACKET)
            {
                errorAt(peek(), "Operator '[]' mein ']' band karna bhool gaye! Mila: '" + peek().value + "'");
            }
            current++; // consume ']'
            checkNotError();
            if (peek().type == TokenType::ASSIGN)
            {
                current++; // consume '='
                opSymbol = "[]=";
                methodName = "__daal__";
            }
            else
            {
                opSymbol = "[]";
                methodName = "__la__";
            }
        }
        // 2. Binary / Unary arithmetic and comparison operators
        else if (peek().type == TokenType::PLUS)
        {
            current++; opSymbol = "+"; methodName = "__jod__";
        }
        else if (peek().type == TokenType::MINUS)
        {
            current++; opSymbol = "-";
        }
        else if (peek().type == TokenType::MULTIPLY)
        {
            current++; opSymbol = "*"; methodName = "__guna__";
        }
        else if (peek().type == TokenType::DIVIDE)
        {
            current++; opSymbol = "/"; methodName = "__bhaag__";
        }
        else if (peek().type == TokenType::MOD)
        {
            current++; opSymbol = "%"; methodName = "__bacha__";
        }
        else if (peek().type == TokenType::EQUAL)
        {
            current++; opSymbol = "=="; methodName = "__barabar__";
        }
        else if (peek().type == TokenType::NOT_EQUAL)
        {
            current++; opSymbol = "!="; methodName = "__alag__";
        }
        else if (peek().type == TokenType::LESS_THAN)
        {
            current++; opSymbol = "<"; methodName = "__chota__";
        }
        else if (peek().type == TokenType::LESS_THAN_EQUAL)
        {
            current++; opSymbol = "<="; methodName = "__chota_barabar__";
        }
        else if (peek().type == TokenType::GREATER_THAN)
        {
            current++; opSymbol = ">"; methodName = "__bada__";
        }
        else if (peek().type == TokenType::GREATER_THAN_EQUAL)
        {
            current++; opSymbol = ">="; methodName = "__bada_barabar__";
        }
        else if (peek().type == TokenType::IDENTIFIER)
        {
            methodName = peek().value;
            opSymbol = methodName;
            current++;
        }
        else
        {
            errorAt(peek(), "Tantrik operator overload mein valid operator chahiye! Mila: '" + peek().value + "'");
        }

        checkNotError();
        if (peek().type != TokenType::OPEN_PAREN)
        {
            errorAt(peek(), "Operator '" + opSymbol + "' ke baad '(' lagana zaroori hai! Mila: '" + peek().value + "'");
        }
        current++; // consume '('

        std::vector<FuncParam> params = parseParameterList(opSymbol);
        consumeCloseParen("operator '" + opSymbol + "' ke parameters");

        bool isUnary = false;
        if (opSymbol == "-")
        {
            if (params.empty())
            {
                isUnary = true;
                methodName = "__ulta__";
            }
            else
            {
                methodName = "__ghata__";
            }
        }

        std::shared_ptr<Type> returnType = nullptr;
        checkNotError();
        if (peek().type == TokenType::COLON || peek().type == TokenType::THIN_ARROW || peek().type == TokenType::ARROW)
        {
            current++; // consume ':', '->', or '=>'
            returnType = parseTypeExpression();
        }

        std::unique_ptr<Stmt> body = parseBlockStmt();

        return std::make_unique<OperatorDeclStmt>(opSymbol, methodName, std::move(params), std::move(body), returnType, isUnary, line);
    }

    // 12. `panga { ... } lafda err { ... } [pakka { ... }]`
    std::unique_ptr<Stmt> parseTryCatchStmt()
    {
        int line = peek().line;
        current++; // consume 'panga'
        checkNotError();

        if (peek().type != TokenType::OPEN_BRACE)
        {
            errorAt(peek(),
                "Panga lene ke liye block '{ ... }' banana padega! Mila: '" + peek().value + "'");
        }
        std::unique_ptr<Stmt> tryBlock = parseBlockStmt();
        checkNotError();

        if (peek().type != TokenType::LAFDA)
        {
            errorAt(peek(),
                "Panga liya hai toh lafda sambhalna padega! 'lafda' (catch) block kahan hai? Mila: '" + peek().value + "'");
        }
        current++; // consume 'lafda'
        checkNotError();

        std::string catchVar = "err";
        if (peek().type == TokenType::OPEN_PAREN)
        {
            current++; // consume '('
            checkNotError();
            if (peek().type == TokenType::IDENTIFIER)
            {
                catchVar = peek().value;
                current++;
            }
            if (peek().type == TokenType::CLOSE_PAREN)
            {
                current++;
            }
        }
        else if (peek().type == TokenType::IDENTIFIER)
        {
            catchVar = peek().value;
            current++;
        }
        checkNotError();

        if (peek().type != TokenType::OPEN_BRACE)
        {
            errorAt(peek(),
                "Lafda sambhalne ke liye block '{ ... }' lagao! Mila: '" + peek().value + "'");
        }
        std::unique_ptr<Stmt> catchBlock = parseBlockStmt();

        std::unique_ptr<Stmt> finallyBlock = nullptr;
        checkNotError();
        if (peek().type == TokenType::PAKKA)
        {
            current++; // consume 'pakka'
            checkNotError();
            if (peek().type != TokenType::OPEN_BRACE)
            {
                errorAt(peek(),
                    "Pakka block ke liye '{ ... }' lagao! Mila: '" + peek().value + "'");
            }
            finallyBlock = parseBlockStmt();
        }

        return std::make_unique<TryCatchStmt>(std::move(tryBlock), catchVar, std::move(catchBlock), std::move(finallyBlock), line);
    }

    // 13. `chillao expr;`
    std::unique_ptr<Stmt> parseThrowStmt()
    {
        int line = peek().line;
        current++; // consume 'chillao'
        checkNotError();

        auto expr = parseAssignment();
        consumeSemicolon();
        return std::make_unique<ThrowStmt>(std::move(expr), line);
    }

    // 14. `bula "path" [jaise alias | jaise { a, b, c }];`
    std::unique_ptr<Stmt> parseImportStmt()
    {
        int line = peek().line;
        current++; // consume 'bula'
        checkNotError();

        if (peek().type != TokenType::STRING_LITERAL)
        {
            errorAt(peek(),
                "Kahan se bulana hai? 'bula' ke baad file ka path string mein likho! Mila: '" + peek().value + "'");
        }
        std::string path = peek().value;
        current++; // consume string literal
        checkNotError();

        std::string alias = "";
        std::vector<std::string> namedImports;
        if (peek().type == TokenType::AS)
        {
            current++; // consume 'jaise' / 'as'
            checkNotError();
            if (peek().type == TokenType::OPEN_BRACE)
            {
                current++; // consume '{'
                while (peek().type != TokenType::CLOSE_BRACE && peek().type != TokenType::END_OF_FILE)
                {
                    checkNotError();
                    if (peek().type != TokenType::IDENTIFIER)
                    {
                        errorAt(peek(), "Destructuring import mein identifier chahiye! Mila: '" + peek().value + "'");
                    }
                    namedImports.push_back(peek().value);
                    current++; // consume identifier
                    checkNotError();
                    if (peek().type == TokenType::COMMA)
                    {
                        current++; // consume ','
                    }
                    else if (peek().type != TokenType::CLOSE_BRACE)
                    {
                        break;
                    }
                }
                if (peek().type != TokenType::CLOSE_BRACE)
                {
                    errorAt(peek(), "Destructuring import band karne ke liye '}' lagao!");
                }
                current++; // consume '}'
            }
            else if (peek().type == TokenType::IDENTIFIER)
            {
                alias = peek().value;
                current++; // consume alias identifier
            }
            else
            {
                errorAt(peek(),
                    "Kis naam se bulana hai? 'jaise' ke baad alias ka naam ya '{ a, b }' likho! Mila: '" + peek().value + "'");
            }
        }

        consumeSemicolon();
        return std::make_unique<ImportStmt>(path, alias, namedImports, line);
    }

    // 15. `avtar Status { Pehla, Dusra [= val] }`
    std::unique_ptr<Stmt> parseEnumDeclStmt()
    {
        int line = peek().line;
        current++; // consume 'avtar'
        checkNotError();

        if (peek().type != TokenType::IDENTIFIER)
        {
            errorAt(peek(), "Bina naam ka avtar? 'avtar' ke baad enum ka naam likho! Mila: '" + peek().value + "'");
        }
        std::string enumName = peek().value;
        current++; // consume enum name
        checkNotError();

        if (peek().type != TokenType::OPEN_BRACE)
        {
            errorAt(peek(), "Avtar shuru karne ke liye '{' lagao! Mila: '" + peek().value + "'");
        }
        int openBraceLine = peek().line;
        current++; // consume '{'
        checkNotError();

        std::vector<EnumMember> members;
        while (peek().type != TokenType::CLOSE_BRACE && peek().type != TokenType::END_OF_FILE)
        {
            checkNotError();
            if (peek().type != TokenType::IDENTIFIER)
            {
                errorAt(peek(), "Avtar member ka sahi naam rakho! Mila: '" + peek().value + "'");
            }
            std::string memberName = peek().value;
            current++; // consume identifier
            checkNotError();

            std::unique_ptr<Expr> valExpr = nullptr;
            if (peek().type == TokenType::ASSIGN)
            {
                current++; // consume '='
                valExpr = parseLogicalOr();
            }

            members.push_back({memberName, std::move(valExpr)});

            if (peek().type == TokenType::COMMA)
            {
                current++; // consume ','
            }
            else if (peek().type != TokenType::CLOSE_BRACE)
            {
                errorAt(peek(), "Avtar members ke beech ',' lagao! Mila: '" + peek().value + "'");
            }
        }

        if (peek().type == TokenType::END_OF_FILE)
        {
            errorAt(openBraceLine, "Avtar ka '{' khola par '}' band karna bhool gaye!");
        }
        current++; // consume '}'

        return std::make_unique<EnumDeclStmt>(enumName, std::move(members), line);
    }

    std::unique_ptr<Stmt> parsePackageDeclStmt()
    {
        checkNotError();
        int line = peek().line;
        current++; // consume 'potli' / 'package'
        checkNotError();
        if (peek().type != TokenType::IDENTIFIER)
        {
            errorAt(peek(), "Potli (package) ka naam likho! Mila: '" + peek().value + "'");
        }
        std::string name = peek().value;
        current++; // consume package name
        checkNotError();
        if (peek().type == TokenType::SEMICOLON)
        {
            current++;
        }
        return std::make_unique<PackageDeclStmt>(name, line);
    }

    std::unique_ptr<Stmt> parseExportStmt()
    {
        checkNotError();
        int line = peek().line;
        current++; // consume 'bech' / 'export'
        checkNotError();
        auto inner = parseStatement();
        return std::make_unique<ExportStmt>(std::move(inner), line);
    }

    std::unique_ptr<Stmt> parseNamespaceDeclStmt()
    {
        checkNotError();
        int line = peek().line;
        current++; // consume 'mohalla' / 'namespace'
        checkNotError();
        if (peek().type != TokenType::IDENTIFIER)
        {
            errorAt(peek(), "Mohalla (namespace) ka naam likho! Mila: '" + peek().value + "'");
        }
        std::string name = peek().value;
        current++; // consume namespace name
        checkNotError();
        if (peek().type != TokenType::OPEN_BRACE)
        {
            errorAt(peek(), "Mohalla body shuru karne ke liye '{' lagana padta hai! Mila: '" + peek().value + "'");
        }
        current++; // consume '{'
        std::vector<std::unique_ptr<Stmt>> body;
        while (peek().type != TokenType::CLOSE_BRACE && peek().type != TokenType::END_OF_FILE)
        {
            if (peek().type == TokenType::SEMICOLON)
            {
                current++;
                continue;
            }
            body.push_back(parseStatement());
        }
        checkNotError();
        if (peek().type == TokenType::END_OF_FILE)
        {
            errorAt(line, "Mohalla ka darwaza khula chhod diya! '}' lagao.");
        }
        current++; // consume '}'
        if (peek().type == TokenType::SEMICOLON)
        {
            current++;
        }
        return std::make_unique<NamespaceDeclStmt>(name, std::move(body), line);
    }

    std::shared_ptr<MemePattern> parsePattern()
    {
        checkNotError();
        Token tok = peek();

        // 1. Wildcard: `_`
        if (isIdentifierOrVal(tok) && tok.value == "_")
        {
            current++;
            return std::make_shared<WildcardPattern>(tok.line);
        }

        // 2. List Pattern: `[]`, `[x]`, `[a, b]`, `[head, ...tail]`, `[head, *tail]`
        if (tok.type == TokenType::OPEN_SQUARE_BRACKET)
        {
            int listLine = tok.line;
            current++; // consume '['
            std::vector<std::shared_ptr<MemePattern>> elements;
            bool hasRest = false;
            std::string restName = "";

            while (peek().type != TokenType::CLOSE_SQUARE_BRACKET && peek().type != TokenType::END_OF_FILE)
            {
                checkNotError();
                if (peek().type == TokenType::ELLIPSIS || peek().type == TokenType::DEREF || peek().type == TokenType::MULTIPLY)
                {
                    current++; // consume '...' or '*'
                    checkNotError();
                    if (isIdentifierOrVal(peek()))
                    {
                        restName = peek().value;
                        current++;
                    }
                    hasRest = true;
                    if (peek().type == TokenType::COMMA) current++;
                    break;
                }

                elements.push_back(parsePattern());
                if (peek().type == TokenType::COMMA)
                {
                    current++; // consume ','
                }
                else
                {
                    break;
                }
            }

            if (peek().type != TokenType::CLOSE_SQUARE_BRACKET)
            {
                errorAt(peek(), "Pattern matching mein list band karne ke liye ']' lagana bhool gaye!");
            }
            current++; // consume ']'
            return std::make_shared<ListPattern>(std::move(elements), hasRest, restName, listLine);
        }

        // 3. Variant or Variable Identifier
        if (isIdentifierOrVal(tok))
        {
            std::string name = tok.value;
            int line = tok.line;

            // Nullary Option variants: `Khali`, `None`
            if ((name == "Khali" || name == "None") && peekNext().type != TokenType::OPEN_PAREN)
            {
                current++;
                return std::make_shared<VariantPattern>(name, std::vector<std::shared_ptr<MemePattern>>{}, line);
            }

            // Variant with parameters: `Some(...)`, `Kuch(...)`, `Ok(...)`, `Sahi(...)`, `Err(...)`, `Galat(...)`, `VariantName(...)`
            if (peekNext().type == TokenType::OPEN_PAREN)
            {
                current++; // consume identifier
                current++; // consume '('
                std::vector<std::shared_ptr<MemePattern>> subs;
                if (peek().type != TokenType::CLOSE_PAREN)
                {
                    while (true)
                    {
                        subs.push_back(parsePattern());
                        if (peek().type == TokenType::COMMA)
                        {
                            current++;
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                if (peek().type != TokenType::CLOSE_PAREN)
                {
                    errorAt(peek(), "Variant pattern ke arguments band karne ke liye ')' lagao!");
                }
                current++; // consume ')'
                return std::make_shared<VariantPattern>(name, std::move(subs), line);
            }

            // Simple variable binding pattern: `x`, `head`, `reason`
            current++;
            return std::make_shared<VariablePattern>(name, line);
        }

        // 4. Literals: ints, negative ints, floats, strings, booleans, null
        if (tok.type == TokenType::INT_LITERAL || tok.type == TokenType::BYTE_LITERAL ||
            tok.type == TokenType::FLOAT_LITERAL || tok.type == TokenType::STRING_LITERAL ||
            tok.type == TokenType::FSTRING_LITERAL || tok.type == TokenType::HAN ||
            tok.type == TokenType::NHI || tok.type == TokenType::KUCH_NHI ||
            tok.type == TokenType::MINUS)
        {
            std::unique_ptr<Expr> litExpr = parseUnary();
            return std::make_shared<LiteralPattern>(std::move(litExpr), tok.line);
        }

        errorAt(tok, "Pattern syntax galat hai! Mila: '" + tok.value + "'");
        return nullptr;
    }

    std::unique_ptr<Expr> parseJaanchExpr()
    {
        int line = peek().line;
        current++; // consume 'jaanch' / 'match'
        checkNotError();

        auto target = parseLogicalOr();
        checkNotError();

        if (peek().type != TokenType::OPEN_BRACE)
        {
            errorAt(peek(), "Jaanch ke liye '{' lagao! Mila: '" + peek().value + "'");
        }
        int openBraceLine = peek().line;
        current++; // consume '{'
        checkNotError();

        std::vector<JaanchArm> arms;
        std::unique_ptr<Expr> defaultBodyExpr = nullptr;
        std::unique_ptr<Stmt> defaultBodyStmt = nullptr;

        while (peek().type != TokenType::CLOSE_BRACE && peek().type != TokenType::END_OF_FILE)
        {
            checkNotError();

            // Check for fallback keywords: `warna`, `nahi_toh`, `else`, `baaki`, `default`
            bool isDefault = (peek().type == TokenType::WARNA || peek().type == TokenType::BAAKI);
            if (isDefault)
            {
                current++; // consume default keyword
                checkNotError();
                if (peek().type != TokenType::ARROW)
                {
                    errorAt(peek(), "Pattern ke baad '=>' lagana bhool gaye! Mila: '" + peek().value + "'");
                }
                current++; // consume '=>'
                checkNotError();

                if (peek().type == TokenType::OPEN_BRACE)
                {
                    defaultBodyStmt = parseBlockStmt();
                }
                else
                {
                    defaultBodyExpr = parseAssignment();
                }
            }
            else
            {
                auto pattern = parsePattern();
                checkNotError();

                std::unique_ptr<Expr> guard = nullptr;
                // Guard expression: `agar cond` or `if cond` (lexed as TokenType::DEKH)
                if (peek().type == TokenType::DEKH)
                {
                    current++; // consume 'agar' / 'if'
                    checkNotError();
                    bool prevInGuard = inJaanchGuard;
                    inJaanchGuard = true;
                    guard = parseLogicalOr();
                    inJaanchGuard = prevInGuard;
                    checkNotError();
                }

                if (peek().type != TokenType::ARROW)
                {
                    errorAt(peek(), "Pattern ke baad '=>' lagana bhool gaye! Mila: '" + peek().value + "'");
                }
                current++; // consume '=>'
                checkNotError();

                std::unique_ptr<Expr> armExpr = nullptr;
                std::unique_ptr<Stmt> armStmt = nullptr;
                if (peek().type == TokenType::OPEN_BRACE)
                {
                    armStmt = parseBlockStmt();
                }
                else
                {
                    armExpr = parseAssignment();
                }

                arms.push_back({std::move(pattern), std::move(guard), std::move(armExpr), std::move(armStmt)});
            }

            // Optional trailing comma or semicolon
            if (peek().type == TokenType::COMMA || peek().type == TokenType::SEMICOLON)
            {
                current++;
            }
        }

        if (peek().type == TokenType::END_OF_FILE)
        {
            errorAt(openBraceLine, "Jaanch ka '{' khola par '}' band karna bhool gaye!");
        }
        current++; // consume '}'

        return std::make_unique<JaanchExpr>(std::move(target), std::move(arms), std::move(defaultBodyExpr), std::move(defaultBodyStmt), line);
    }

    // 16. `jaanch expr { val1 => { ... } baaki => { ... } }`
    std::unique_ptr<Stmt> parseJaanchStmt()
    {
        int line = peek().line;
        auto expr = parseJaanchExpr();
        return std::make_unique<ExprStmt>(std::move(expr), line);
    }

    // 17. Expression Statement: `x = 10;` or `x + 1;`
    std::unique_ptr<Stmt> parseExprStmt()
    {
        std::unique_ptr<Expr> expr = parseAssignment();
        int line = expr ? expr->line : (current > 0 ? tokens[current - 1].line : 1);
        consumeSemicolon();
        return std::make_unique<ExprStmt>(std::move(expr), line);
    }

    // ==========================================
    // Expression Parsing Layers
    // ==========================================

    // Single-expression entry point
    std::unique_ptr<Expr> parse()
    {
        std::unique_ptr<Expr> expr = parseAssignment();

        const Token &remaining = peek();

        if (remaining.type == TokenType::ERROR)
        {
            errorAt(remaining, remaining.value);
        }

        if (remaining.type != TokenType::END_OF_FILE)
        {
            errorAt(remaining,
                "Goli beta, masti nahi! Expression khatam ho chuki hai, "
                "piche ye extra '" +
                remaining.value + "' kyun chipka diya?");
        }

        return expr;
    }

    // Ternary expression: `cond ? then_branch : else_branch` (right-associative)
    std::unique_ptr<Expr> parseTernary()
    {
        std::unique_ptr<Expr> expr = parseLogicalOr();

        if (peek().type == TokenType::QUESTION)
        {
            Token qToken = peek();
            current++; // consume '?'
            std::unique_ptr<Expr> thenBranch = parseAssignment();
            if (peek().type != TokenType::COLON)
            {
                errorAt(peek(), "Ternary expression mein ':' gayab hai! 'condition ? then_branch : else_branch' likh!");
            }
            current++; // consume ':'
            std::unique_ptr<Expr> elseBranch = parseAssignment();
            return std::make_unique<TernaryExpr>(std::move(expr), std::move(thenBranch), std::move(elseBranch), qToken.line);
        }

        return expr;
    }

    // Assignment expression: `x = expr` (right-associative)
    // Rule: The left-hand side must be a bare identifier (not parenthesized like `(x) = 5`),
    // and assignments are disallowed inside sub-expressions, conditions, or argument lists.
    std::unique_ptr<Expr> parseAssignment()
    {
        bool startsWithBareIdentifier = isIdentifierOrVal(peek());
        std::unique_ptr<Expr> expr = parseTernary();

        if (peek().type == TokenType::ASSIGN)
        {
            int eqLine = peek().line;
            current++; // consume '='
            std::unique_ptr<Expr> value = parseAssignment();

            if (startsWithBareIdentifier)
            {
                if (auto *ident = dynamic_cast<IdentifierExpr *>(expr.get()))
                {
                    return std::make_unique<AssignExpr>(ident->name, std::move(value), eqLine);
                }
            }

            if (auto *get = dynamic_cast<GetExpr *>(expr.get()))
            {
                return std::make_unique<SetExpr>(std::move(get->object), get->name, std::move(value), eqLine);
            }

            if (auto *scoped = dynamic_cast<ScopedAccessExpr *>(expr.get()))
            {
                return std::make_unique<ScopedSetExpr>(std::move(scoped->scope), scoped->member, std::move(value), eqLine);
            }

            if (auto *idx = dynamic_cast<IndexExpr *>(expr.get()))
            {
                return std::make_unique<IndexAssignExpr>(std::move(idx->target), std::move(idx->index), std::move(value), eqLine);
            }

            if (auto *deref = dynamic_cast<DerefExpr *>(expr.get()))
            {
                return std::make_unique<DerefAssignExpr>(std::move(deref->operand), std::move(value), eqLine);
            }

            if (auto *arr = dynamic_cast<ArrayLiteralExpr *>(expr.get()))
            {
                std::vector<DestructureItem> items;
                bool valid = true;
                for (const auto &el : arr->elements)
                {
                    if (auto *id = dynamic_cast<IdentifierExpr *>(el.get()))
                    {
                        items.push_back({id->name, false});
                    }
                    else
                    {
                        valid = false;
                        break;
                    }
                }
                if (valid)
                {
                    return std::make_unique<DestructureAssignExpr>(std::move(items), std::move(value), eqLine);
                }
            }

            errorAt(eqLine,
                "Ye sab doglapan hai! '=' ke left side mein sirf seedha variable ya property aata hai, koi equation ya bracket nahi!");
        }

        return expr;
    }

    // Logical OR: `ya`
    std::unique_ptr<Expr> parseLogicalOr()
    {
        std::unique_ptr<Expr> left = parseLogicalAnd();

        while (peek().type == TokenType::YA)
        {
            Token op = peek();
            current++;

            std::unique_ptr<Expr> right = parseLogicalAnd();
            left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right), op.line);
        }

        return left;
    }

    // Logical AND: `aur`
    std::unique_ptr<Expr> parseLogicalAnd()
    {
        std::unique_ptr<Expr> left = parseComparison();

        while (peek().type == TokenType::AUR)
        {
            Token op = peek();
            current++;

            std::unique_ptr<Expr> right = parseComparison();
            left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right), op.line);
        }

        return left;
    }

    // Comparison: `==`, `!=`, `<`, `>`, `<=`, `>=`
    std::unique_ptr<Expr> parseComparison()
    {
        std::unique_ptr<Expr> left = parseExpression();

        while (peek().type == TokenType::EQUAL ||
               peek().type == TokenType::NOT_EQUAL ||
               peek().type == TokenType::LESS_THAN ||
               peek().type == TokenType::GREATER_THAN ||
               peek().type == TokenType::LESS_THAN_EQUAL ||
               peek().type == TokenType::GREATER_THAN_EQUAL)
        {
            Token op = peek();
            current++;

            std::unique_ptr<Expr> right = parseExpression();
            left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right), op.line);
        }

        return left;
    }

    // Additive: `+`, `-`
    std::unique_ptr<Expr> parseExpression()
    {
        std::unique_ptr<Expr> left = parseTerm();

        while (peek().type == TokenType::PLUS ||
               peek().type == TokenType::MINUS)
        {
            Token op = peek();
            current++;

            std::unique_ptr<Expr> right = parseTerm();
            left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right), op.line);
        }

        return left;
    }

    // Multiplicative: `*`, `/`, `%`
    std::unique_ptr<Expr> parseTerm()
    {
        std::unique_ptr<Expr> left = parseUnary();

        while (peek().type == TokenType::MULTIPLY ||
               peek().type == TokenType::DIVIDE ||
               peek().type == TokenType::MOD)
        {
            Token op = peek();
            current++;

            std::unique_ptr<Expr> right = parseUnary();
            left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right), op.line);
        }

        return left;
    }

    // Unary prefix: `!`, `-`, `udhaar`, `udhaar_badal`, `nakal` (right-associative, calls parseCall() below it)
    std::unique_ptr<Expr> parseUnary()
    {
        if (peek().type == TokenType::NOT ||
            peek().type == TokenType::MINUS)
        {
            Token op = peek();
            current++;

            std::unique_ptr<Expr> operand = parseUnary();
            return std::make_unique<UnaryExpr>(op, std::move(operand), op.line);
        }

        if (peek().type == TokenType::UDHAAR)
        {
            if (peek().value == "ref")
            {
                if (current + 1 >= tokens.size() || !canStartUnaryOperand(tokens[current + 1]))
                {
                    return parseCall();
                }
            }
            Token op = peek();
            current++;
            std::unique_ptr<Expr> operand = parseUnary();
            return std::make_unique<BorrowExpr>(std::move(operand), false, op.line);
        }

        if (peek().type == TokenType::UDHAAR_BADAL)
        {
            if (peek().value == "mut_ref")
            {
                if (current + 1 >= tokens.size() || !canStartUnaryOperand(tokens[current + 1]))
                {
                    return parseCall();
                }
            }
            Token op = peek();
            current++;
            std::unique_ptr<Expr> operand = parseUnary();
            return std::make_unique<BorrowExpr>(std::move(operand), true, op.line);
        }

        if (peek().type == TokenType::NAKAL)
        {
            Token op = peek();
            current++;
            std::unique_ptr<Expr> operand = parseUnary();
            return std::make_unique<NakalExpr>(std::move(operand), op.line);
        }

        if (peek().type == TokenType::ELLIPSIS)
        {
            Token op = peek();
            current++;
            std::unique_ptr<Expr> operand = parseUnary();
            return std::make_unique<SpreadExpr>(std::move(operand), op.line);
        }

        if (peek().type == TokenType::SABAR)
        {
            Token op = peek();
            current++;
            std::unique_ptr<Expr> operand = parseUnary();
            return std::make_unique<AwaitExpr>(std::move(operand), op.line);
        }

        if (peek().type == TokenType::ADDRESS_OF)
        {
            if (peek().value == "ptr" || peek().value == "addr")
            {
                if (current + 1 >= tokens.size() || !canStartUnaryOperand(tokens[current + 1]))
                {
                    return parseCall();
                }
            }
            Token op = peek();
            current++;
            std::unique_ptr<Expr> operand = parseUnary();
            return std::make_unique<AddressOfExpr>(std::move(operand), op.line);
        }

        if (peek().type == TokenType::DEREF || peek().type == TokenType::MULTIPLY)
        {
            if (peek().type == TokenType::DEREF && peek().value == "val")
            {
                if (current + 1 >= tokens.size() || !canStartUnaryOperand(tokens[current + 1]))
                {
                    return parseCall();
                }
            }
            Token op = peek();
            current++;
            std::unique_ptr<Expr> operand = parseUnary();
            return std::make_unique<DerefExpr>(std::move(operand), op.line);
        }

        if (peek().type == TokenType::MOVE)
        {
            Token op = peek();
            current++;
            std::unique_ptr<Expr> operand = parseUnary();
            return std::make_unique<MoveExpr>(std::move(operand), op.line);
        }

        if (peek().type == TokenType::IDENTIFIER && (peek().value == "new" || peek().value == "naya"))
        {
            current++;
            return parseUnary();
        }

        return parseCall();
    }

    bool isTernaryQuestionAhead() const
    {
        if (peek().type != TokenType::QUESTION) return false;
        if (current + 1 < tokens.size() && tokens[current + 1].type == TokenType::QUESTION)
        {
            return false;
        }
        int parenDepth = 0;
        int bracketDepth = 0;
        int braceDepth = 0;
        for (size_t i = current + 1; i < tokens.size(); ++i)
        {
            const auto &tok = tokens[i];
            if (tok.type == TokenType::SEMICOLON || tok.type == TokenType::END_OF_FILE)
            {
                return false;
            }
            if (tok.type == TokenType::OPEN_PAREN) parenDepth++;
            else if (tok.type == TokenType::CLOSE_PAREN)
            {
                if (parenDepth == 0) return false;
                parenDepth--;
            }
            else if (tok.type == TokenType::OPEN_SQUARE_BRACKET) bracketDepth++;
            else if (tok.type == TokenType::CLOSE_SQUARE_BRACKET)
            {
                if (bracketDepth == 0) return false;
                bracketDepth--;
            }
            else if (tok.type == TokenType::OPEN_BRACE) braceDepth++;
            else if (tok.type == TokenType::CLOSE_BRACE)
            {
                if (braceDepth == 0) return false;
                braceDepth--;
            }
            else if (parenDepth == 0 && bracketDepth == 0 && braceDepth == 0)
            {
                if (tok.type == TokenType::COLON) return true;
                if (tok.type == TokenType::COMMA) return false;
            }
        }
        return false;
    }

    // Postfix function call & property access layer: `f(1, 2)`, `obj.prop`, `obj.method(1)`
    std::unique_ptr<Expr> parseCall()
    {
        std::unique_ptr<Expr> expr = parsePrimary();

        while (true)
        {
            if (peek().type == TokenType::OPEN_PAREN)
            {
                // CRITICAL FIX: If optional semicolons are used, a '(' on a new line
                // is NOT a postfix call, but starts a new parenthesized expression or arrow lambda.
                if (current > 0 && peek().line > tokens[current - 1].line)
                {
                    break;
                }
                int callLine = peek().line;
                current++; // consume '('
                std::vector<std::unique_ptr<Expr>> args = parseArgumentList("function call");
                expr = std::make_unique<CallExpr>(std::move(expr), std::move(args), callLine);
            }
            else if (peek().type == TokenType::DOT)
            {
                int dotLine = peek().line;
                current++; // consume '.'
                checkNotError();
                if (peek().type != TokenType::IDENTIFIER &&
                    (peek().value.empty() || (!std::isalpha(static_cast<unsigned char>(peek().value[0])) && peek().value[0] != '_')))
                {
                    errorAt(peek(),
                        "'.' ke baad property ya method ka naam chahiye! Mila: '" + peek().value + "'");
                }
                std::string propName = peek().value;
                current++; // consume property name
                expr = std::make_unique<GetExpr>(std::move(expr), propName, dotLine);
            }
            else if (peek().type == TokenType::DOUBLE_COLON)
            {
                int colonLine = peek().line;
                current++; // consume '::'
                checkNotError();
                if (peek().type != TokenType::IDENTIFIER &&
                    (peek().value.empty() || (!std::isalpha(static_cast<unsigned char>(peek().value[0])) && peek().value[0] != '_')))
                {
                    errorAt(peek(), "'::' ke baad member ya identifier ka naam chahiye! Mila: '" + peek().value + "'");
                }
                std::string memberName = peek().value;
                current++; // consume member name
                expr = std::make_unique<ScopedAccessExpr>(std::move(expr), memberName, colonLine);
            }
            else if (peek().type == TokenType::OPEN_SQUARE_BRACKET)
            {
                // CRITICAL FIX: If optional semicolons are used, a '[' on a new line
                // is NOT an index access, but starts an array literal on the new line.
                if (current > 0 && peek().line > tokens[current - 1].line)
                {
                    break;
                }
                int bracketLine = peek().line;
                current++; // consume '['
                auto index = parseAssignment();
                checkNotError();
                if (peek().type != TokenType::CLOSE_SQUARE_BRACKET)
                {
                    errorAt(peek(), "Index access mein ']' lagana bhool gaye! Mila: '" + peek().value + "'");
                }
                current++; // consume ']'
                expr = std::make_unique<IndexExpr>(std::move(expr), std::move(index), bracketLine);
            }
            else if (peek().type == TokenType::QUESTION && !isTernaryQuestionAhead())
            {
                int qLine = peek().line;
                current++; // consume '?'
                expr = std::make_unique<TryPropagateExpr>(std::move(expr), qLine);
            }
            else
            {
                break;
            }
        }

        return expr;
    }

    bool isJsxElementAhead() const
    {
        if (current + 1 >= tokens.size()) return false;
        const auto &next = tokens[current + 1];
        return (next.type == TokenType::GREATER_THAN || isPotentialTagName(next));
    }

    std::unique_ptr<Expr> parseJsxElement()
    {
        int line = peek().line;
        current++; // consume '<'

        bool isFragment = false;
        std::string tagName = "";

        if (peek().type == TokenType::GREATER_THAN)
        {
            isFragment = true;
            tagName = "Fragment";
            current++; // consume '>'
        }
        else if (isPotentialTagName(peek()))
        {
            tagName = peek().value;
            current++; // consume tag name
            while (peek().type == TokenType::DOT && current + 1 < tokens.size() && isPotentialTagName(tokens[current + 1]))
            {
                current++; // consume '.'
                tagName += "." + peek().value;
                current++; // consume identifier
            }
        }
        else
        {
            errorAt(peek(), "JSX tag name chahiye! Mila: '" + peek().value + "'");
        }

        std::vector<std::pair<std::string, std::unique_ptr<Expr>>> props;

        if (!isFragment)
        {
            // Parse props
            while (peek().type != TokenType::GREATER_THAN &&
                   peek().type != TokenType::DIVIDE &&
                   peek().type != TokenType::END_OF_FILE)
            {
                if (peek().type == TokenType::OPEN_BRACE)
                {
                    current++; // consume '{'
                    if (peek().type == TokenType::ELLIPSIS)
                    {
                        current++; // consume '...'
                        auto spreadVal = parseAssignment();
                        if (peek().type != TokenType::CLOSE_BRACE)
                        {
                            errorAt(peek(), "JSX spread expression band karne ke liye '}' lagao!");
                        }
                        current++; // consume '}'
                        props.push_back({"...", std::move(spreadVal)});
                        continue;
                    }
                    else if (peek().type == TokenType::DOT && current + 2 < tokens.size() &&
                             tokens[current + 1].type == TokenType::DOT && tokens[current + 2].type == TokenType::DOT)
                    {
                        current += 3; // consume '.' '.' '.'
                        auto spreadVal = parseAssignment();
                        if (peek().type != TokenType::CLOSE_BRACE)
                        {
                            errorAt(peek(), "JSX spread expression band karne ke liye '}' lagao!");
                        }
                        current++; // consume '}'
                        props.push_back({"...", std::move(spreadVal)});
                        continue;
                    }
                }

                std::string propName = peek().value;
                current++; // consume prop name

                while (peek().type == TokenType::MINUS && current + 1 < tokens.size() && tokens[current + 1].type == TokenType::IDENTIFIER)
                {
                    current++; // consume '-'
                    propName += "-" + peek().value;
                    current++; // consume identifier
                }

                std::unique_ptr<Expr> propVal;
                if (peek().type == TokenType::ASSIGN)
                {
                    current++; // consume '='
                    if (peek().type == TokenType::STRING_LITERAL)
                    {
                        propVal = std::make_unique<StringExpr>(peek().value, peek().line);
                        current++;
                    }
                    else if (peek().type == TokenType::OPEN_BRACE)
                    {
                        current++; // consume '{'
                        propVal = parseAssignment();
                        if (peek().type != TokenType::CLOSE_BRACE)
                        {
                            errorAt(peek(), "JSX prop expression band karne ke liye '}' lagao!");
                        }
                        current++; // consume '}'
                    }
                    else
                    {
                        propVal = parsePrimary();
                    }
                }
                else
                {
                    // Boolean prop, e.g. <Button disabled />
                    propVal = std::make_unique<BoolExpr>(true, line);
                }
                props.push_back({propName, std::move(propVal)});
            }

            // Check self-closing `/>`
            if (peek().type == TokenType::DIVIDE)
            {
                current++; // consume '/'
                if (peek().type != TokenType::GREATER_THAN)
                {
                    errorAt(peek(), "Self-closing JSX tag band karne ke liye '>' lagao! Mila: '" + peek().value + "'");
                }
                current++; // consume '>'
                return std::make_unique<JsxElementExpr>(tagName, std::move(props), std::vector<std::unique_ptr<Expr>>{}, false, line);
            }

            if (peek().type != TokenType::GREATER_THAN)
            {
                errorAt(peek(), "JSX tag band karne ke liye '>' lagao! Mila: '" + peek().value + "'");
            }
            current++; // consume '>'
        }

        // Parse children until closing tag `</tagName>` or `</>`
        std::vector<std::unique_ptr<Expr>> children;
        while (peek().type != TokenType::END_OF_FILE)
        {
            if (peek().type == TokenType::LESS_THAN)
            {
                if (current + 1 < tokens.size() && tokens[current + 1].type == TokenType::DIVIDE)
                {
                    // Closing tag `</`
                    current += 2; // consume '<' and '/'
                    if (!isFragment)
                    {
                        while (isPotentialTagName(peek()) || peek().type == TokenType::DOT)
                        {
                            current++; // consume closing tag name or dots
                        }
                    }
                    if (peek().type == TokenType::GREATER_THAN)
                    {
                        current++; // consume '>'
                    }
                    break;
                }
                else
                {
                    // Nested JSX element
                    children.push_back(parseJsxElement());
                }
            }
            else if (peek().type == TokenType::OPEN_BRACE)
            {
                // Interpolated expression: `{expr}`
                current++; // consume '{'
                children.push_back(parseAssignment());
                if (peek().type == TokenType::CLOSE_BRACE)
                {
                    current++; // consume '}'
                }
                else
                {
                    errorAt(peek(), "JSX interpolation band karne ke liye '}' lagao!");
                }
            }
            else
            {
                // Raw text words! Accumulate all tokens until next '<' or '{'
                std::string textWords = "";
                int textLine = peek().line;
                while (peek().type != TokenType::LESS_THAN &&
                       peek().type != TokenType::OPEN_BRACE &&
                       peek().type != TokenType::END_OF_FILE)
                {
                    if (!textWords.empty()) textWords += " ";
                    textWords += peek().value;
                    current++;
                }
                if (!textWords.empty())
                {
                    children.push_back(std::make_unique<StringExpr>(textWords, textLine));
                }
            }
        }

        return std::make_unique<JsxElementExpr>(tagName, std::move(props), std::move(children), isFragment, line);
    }

    // Primary: literals, identifiers, parenthesized expressions
    std::unique_ptr<Expr> parsePrimary()
    {
        const Token &token = peek();

        switch (token.type)
        {
        case TokenType::INT_LITERAL:
        {
            try
            {
                long long val = 0;
                std::string v = token.value;
                if (v.rfind("0x", 0) == 0 || v.rfind("0X", 0) == 0)
                {
                    val = std::stoll(v, nullptr, 16);
                }
                else if (v.rfind("0b", 0) == 0 || v.rfind("0B", 0) == 0)
                {
                    val = std::stoll(v.substr(2), nullptr, 2);
                }
                else
                {
                    val = std::stoll(v);
                }
                auto expr = std::make_unique<IntExpr>(val, token.line);
                current++;
                return expr;
            }
            catch (const std::invalid_argument &)
            {
                errorAt(token,
                    "Aayein? Ye kaisa number likh diya tune? Calculator bhi ro pada: " +
                    token.value);
            }
            catch (const std::out_of_range &)
            {
                errorAt(token,
                    "7 Crore!! Itna bada number RAM ki aukaat ke bahar hai, thoda chota fek: " +
                    token.value);
            }
        }

        case TokenType::BYTE_LITERAL:
        {
            try
            {
                std::string v = token.value;
                uint8_t byteVal = 0;
                if (v.rfind("0x", 0) == 0 || v.rfind("0X", 0) == 0)
                {
                    byteVal = static_cast<uint8_t>(std::stoul(v, nullptr, 16) & 0xFF);
                }
                else if (v.rfind("0b", 0) == 0 || v.rfind("0B", 0) == 0)
                {
                    byteVal = static_cast<uint8_t>(std::stoul(v.substr(2), nullptr, 2) & 0xFF);
                }
                else
                {
                    byteVal = static_cast<uint8_t>(std::stoul(v) & 0xFF);
                }
                auto expr = std::make_unique<ByteExpr>(byteVal, token.line);
                current++;
                return expr;
            }
            catch (...)
            {
                errorAt(token, "Aayein? Byte literal valid nahi hai: " + token.value);
            }
        }

        case TokenType::BYTES_LITERAL:
        {
            std::vector<uint8_t> data(token.value.begin(), token.value.end());
            auto expr = std::make_unique<BytesExpr>(std::move(data), token.line);
            current++;
            return expr;
        }

        case TokenType::FLOAT_LITERAL:
        {
            try
            {
                std::string v = token.value;
                bool isF32 = false;
                if (!v.empty() && (v.back() == 'f' || v.back() == 'F'))
                {
                    isF32 = true;
                    v.pop_back();
                }
                auto expr = std::make_unique<FloatExpr>(
                    std::stod(v), token.line, isF32);

                current++;
                return expr;
            }
            catch (const std::invalid_argument &)
            {
                errorAt(token,
                    "Arre kehna kya chahte ho? Ye point wala number toh kisi angle se nahi lag raha: " +
                    token.value);
            }
            catch (const std::out_of_range &)
            {
                errorAt(token,
                    "Ye kis line mein aa gaye aap? Itna bada decimal dekh ke CPU ka dimaag phat gaya: " +
                    token.value);
            }
        }

        case TokenType::STRING_LITERAL:
            current++;
            return std::make_unique<StringExpr>(token.value, token.line);

        case TokenType::FSTRING_LITERAL:
        {
            int fstringLine = token.line;
            std::string raw = token.value;
            current++; // consume token

            std::vector<std::unique_ptr<Expr>> parts;
            std::string currentLit = "";
            size_t i = 0;
            while (i < raw.size())
            {
                if (raw[i] == '{')
                {
                    if (i + 1 < raw.size() && raw[i + 1] == '{')
                    {
                        currentLit += '{';
                        i += 2;
                        continue;
                    }

                    if (!currentLit.empty())
                    {
                        parts.push_back(std::make_unique<StringExpr>(currentLit, fstringLine));
                        currentLit = "";
                    }

                    size_t start = i + 1;
                    size_t end = start;
                    int depth = 1;
                    while (end < raw.size())
                    {
                        if (raw[end] == '{')
                        {
                            depth++;
                        }
                        else if (raw[end] == '}')
                        {
                            depth--;
                            if (depth == 0)
                            {
                                break;
                            }
                        }
                        end++;
                    }

                    if (depth != 0 || end == start)
                    {
                        errorAt(fstringLine, "F-string mein '{' khol ke bhool gaye ya khali chhod diya! Andar expression toh daalo!");
                    }

                    std::string exprStr = raw.substr(start, end - start);
                    size_t first = exprStr.find_first_not_of(" \t\r\n");
                    if (first == std::string::npos)
                    {
                        errorAt(fstringLine, "F-string mein '{' khol ke bhool gaye ya khali chhod diya! Andar expression toh daalo!");
                    }

                    Lexer subLexer(exprStr);
                    std::vector<Token> subTokens = subLexer.tokenize();
                    Parser subParser(subTokens);
                    auto exprNode = subParser.parse();
                    parts.push_back(std::move(exprNode));
                    i = end + 1;
                }
                else if (raw[i] == '}')
                {
                    if (i + 1 < raw.size() && raw[i + 1] == '}')
                    {
                        currentLit += '}';
                        i += 2;
                        continue;
                    }
                    errorAt(fstringLine, "F-string mein bina '{' ke '}' tapak pada!");
                }
                else
                {
                    currentLit += raw[i++];
                }
            }

            if (!currentLit.empty())
            {
                parts.push_back(std::make_unique<StringExpr>(currentLit, fstringLine));
            }

            return std::make_unique<FStringExpr>(std::move(parts), fstringLine);
        }

        case TokenType::HAN:
            current++;
            return std::make_unique<BoolExpr>(true, token.line);

        case TokenType::NHI:
            current++;
            return std::make_unique<BoolExpr>(false, token.line);

        case TokenType::KUCH_NHI:
            current++;
            return std::make_unique<NullExpr>(token.line);

        case TokenType::JAANCH:
            return parseJaanchExpr();

        case TokenType::POOCH:
        {
            int poochLine = token.line;
            if (token.value == "input" && peek().type != TokenType::OPEN_PAREN)
            {
                current++;
                return std::make_unique<IdentifierExpr>(token.value, poochLine);
            }
            current++; // consume 'pooch'
            std::unique_ptr<Expr> prompt = nullptr;
            if (peek().type == TokenType::OPEN_PAREN)
            {
                current++; // consume '('
                checkNotError();
                if (peek().type != TokenType::CLOSE_PAREN)
                {
                    prompt = parseLogicalOr();
                }
                consumeCloseParen("pooch()");
            }
            return std::make_unique<PoochExpr>(std::move(prompt), poochLine);
        }

        case TokenType::ASYNC:
        {
            int funcLine = token.line;
            current++; // consume 'async'
            checkNotError();

            if (peek().type == TokenType::KAR)
            {
                current++; // consume 'kar' / 'kaam' / 'fn'
                checkNotError();

                std::string funcName = "";
                if (isFunctionName(peek()))
                {
                    funcName = peek().value;
                    current++;
                }

                checkNotError();
                if (peek().type != TokenType::OPEN_PAREN)
                {
                    errorAt(peek(), "Async function ke baad '(' lagana bhool gaye!");
                }
                current++; // consume '('

                std::vector<FuncParam> params = parseParameterList(funcName);
                consumeCloseParen("async function ke parameters");

                std::shared_ptr<Type> returnType = nullptr;
                checkNotError();
                if (peek().type == TokenType::COLON || peek().type == TokenType::THIN_ARROW || peek().type == TokenType::ARROW)
                {
                    current++;
                    returnType = parseTypeExpression();
                }

                checkNotError();
                if (peek().type != TokenType::OPEN_BRACE)
                {
                    errorAt(peek(), "Async function ka body '{' se shuru hona chahiye! Mila: '" + peek().value + "'");
                }
                std::unique_ptr<Stmt> body = parseBlockStmt();

                auto decl = std::make_shared<FuncDeclStmt>(funcName, std::move(params), std::move(body), returnType, true, funcLine);
                return std::make_unique<LambdaExpr>(decl, funcLine);
            }

            if (isArrowLambdaAhead())
            {
                current++; // consume '('
                std::vector<FuncParam> params = parseParameterList("");
                consumeCloseParen("async arrow lambda parameters");

                std::shared_ptr<Type> returnType = nullptr;
                checkNotError();
                if (peek().type == TokenType::COLON || peek().type == TokenType::THIN_ARROW)
                {
                    current++;
                    returnType = parseTypeExpression();
                }

                checkNotError();
                if (peek().type != TokenType::ARROW)
                {
                    errorAt(peek(), "Async arrow lambda mein '=>' lagana zaroori hai! Mila: '" + peek().value + "'");
                }
                int arrowLine = peek().line;
                current++; // consume '=>'

                std::unique_ptr<Stmt> body;
                checkNotError();
                if (peek().type == TokenType::OPEN_BRACE)
                {
                    body = parseBlockStmt();
                }
                else
                {
                    int exprLine = peek().line;
                    auto expr = parseTernary();
                    std::vector<std::unique_ptr<Stmt>> stmts;
                    stmts.push_back(std::make_unique<ReturnStmt>(std::move(expr), exprLine));
                    body = std::make_unique<BlockStmt>(std::move(stmts), exprLine);
                }

                auto decl = std::make_shared<FuncDeclStmt>("", std::move(params), std::move(body), returnType, true, arrowLine);
                return std::make_unique<LambdaExpr>(decl, arrowLine);
            }

            if (peek().type == TokenType::IDENTIFIER && peekNext().type == TokenType::ARROW)
            {
                int arrowLine = peek().line;
                std::string paramName = peek().value;
                current++; // consume identifier
                current++; // consume '=>'

                std::vector<FuncParam> params;
                params.push_back(FuncParam{paramName, nullptr, false});

                std::unique_ptr<Stmt> body;
                checkNotError();
                if (peek().type == TokenType::OPEN_BRACE)
                {
                    body = parseBlockStmt();
                }
                else
                {
                    int exprLine = peek().line;
                    auto expr = parseTernary();
                    std::vector<std::unique_ptr<Stmt>> stmts;
                    stmts.push_back(std::make_unique<ReturnStmt>(std::move(expr), exprLine));
                    body = std::make_unique<BlockStmt>(std::move(stmts), exprLine);
                }

                auto decl = std::make_shared<FuncDeclStmt>("", std::move(params), std::move(body), nullptr, true, arrowLine);
                return std::make_unique<LambdaExpr>(decl, arrowLine);
            }

            errorAt(token, "'async' ke baad function ya arrow lambda chahiye!");
        }

        case TokenType::KAR:
        {
            int funcLine = token.line;
            current++; // consume 'kar' / 'jugaad'
            checkNotError();

            std::string funcName = "";
            if (isFunctionName(peek()))
            {
                funcName = peek().value;
                current++; // consume optional function name
            }

            checkNotError();
            if (peek().type != TokenType::OPEN_PAREN)
            {
                errorAt(peek(),
                    "Jugaad ke baad '(' lagana bhool gaye! Bina bracket ke kaise chalega?");
            }
            current++; // consume '('

            std::vector<FuncParam> params = parseParameterList(funcName);
            consumeCloseParen("jugaad ke parameters");

            std::shared_ptr<Type> returnType = nullptr;
            checkNotError();
            if (peek().type == TokenType::COLON || peek().type == TokenType::THIN_ARROW || peek().type == TokenType::ARROW)
            {
                current++;
                returnType = parseTypeExpression();
            }

            checkNotError();
            if (peek().type != TokenType::OPEN_BRACE)
            {
                errorAt(peek(),
                    "Jugaad ka body '{' se shuru hona chahiye! Mila: '" + peek().value + "'");
            }
            std::unique_ptr<Stmt> body = parseBlockStmt();

            auto decl = std::make_shared<FuncDeclStmt>(funcName, std::move(params), std::move(body), returnType, false, funcLine);
            return std::make_unique<LambdaExpr>(decl, funcLine);
        }

        case TokenType::DIKHA:
        {
            int line = token.line;
            std::string name = token.value;
            current++;
            return std::make_unique<IdentifierExpr>(name, line);
        }

        case TokenType::DEREF:
        {
            if (token.value == "val")
            {
                if (peekNext().type == TokenType::ARROW)
                {
                    int arrowLine = token.line;
                    std::string paramName = token.value;
                    current++; // consume identifier
                    current++; // consume '=>'

                    std::vector<FuncParam> params;
                    params.push_back(FuncParam{paramName, nullptr, false});

                    std::unique_ptr<Stmt> body;
                    checkNotError();
                    if (peek().type == TokenType::OPEN_BRACE)
                    {
                        body = parseBlockStmt();
                    }
                    else
                    {
                        int exprLine = peek().line;
                        auto expr = parseTernary();
                        std::vector<std::unique_ptr<Stmt>> stmts;
                        stmts.push_back(std::make_unique<ReturnStmt>(std::move(expr), exprLine));
                        body = std::make_unique<BlockStmt>(std::move(stmts), exprLine);
                    }

                    auto decl = std::make_shared<FuncDeclStmt>("", std::move(params), std::move(body), nullptr, false, arrowLine);
                    return std::make_unique<LambdaExpr>(decl, arrowLine);
                }

                current++;
                return std::make_unique<IdentifierExpr>(token.value, token.line);
            }
            errorAt(token, "Unexpected dereference operator in expression!");
        }

        case TokenType::UDHAAR:
        {
            if (token.value == "ref")
            {
                current++;
                return std::make_unique<IdentifierExpr>(token.value, token.line);
            }
            errorAt(token, "Unexpected borrow operator in expression!");
        }

        case TokenType::UDHAAR_BADAL:
        {
            if (token.value == "mut_ref")
            {
                current++;
                return std::make_unique<IdentifierExpr>(token.value, token.line);
            }
            errorAt(token, "Unexpected mutable borrow operator in expression!");
        }

        case TokenType::ADDRESS_OF:
        {
            if (token.value == "ptr" || token.value == "addr")
            {
                if (peekNext().type == TokenType::ARROW)
                {
                    int arrowLine = token.line;
                    std::string paramName = token.value;
                    current++; // consume identifier
                    current++; // consume '=>'

                    std::vector<FuncParam> params;
                    params.push_back(FuncParam{paramName, nullptr, false});

                    std::unique_ptr<Stmt> body;
                    checkNotError();
                    if (peek().type == TokenType::OPEN_BRACE)
                    {
                        body = parseBlockStmt();
                    }
                    else
                    {
                        int exprLine = peek().line;
                        auto expr = parseTernary();
                        std::vector<std::unique_ptr<Stmt>> stmts;
                        stmts.push_back(std::make_unique<ReturnStmt>(std::move(expr), exprLine));
                        body = std::make_unique<BlockStmt>(std::move(stmts), exprLine);
                    }

                    auto decl = std::make_shared<FuncDeclStmt>("", std::move(params), std::move(body), nullptr, false, arrowLine);
                    return std::make_unique<LambdaExpr>(decl, arrowLine);
                }

                current++;
                return std::make_unique<IdentifierExpr>(token.value, token.line);
            }
            errorAt(token, "Unexpected address-of operator in expression!");
        }

        case TokenType::IDENTIFIER:
        {
            if (!inJaanchGuard && peekNext().type == TokenType::ARROW)
            {
                int arrowLine = token.line;
                std::string paramName = token.value;
                current++; // consume identifier
                current++; // consume '=>'

                std::vector<FuncParam> params;
                params.push_back(FuncParam{paramName, nullptr, false});

                std::unique_ptr<Stmt> body;
                checkNotError();
                if (peek().type == TokenType::OPEN_BRACE)
                {
                    body = parseBlockStmt();
                }
                else
                {
                    int exprLine = peek().line;
                    auto expr = parseTernary();
                    std::vector<std::unique_ptr<Stmt>> stmts;
                    stmts.push_back(std::make_unique<ReturnStmt>(std::move(expr), exprLine));
                    body = std::make_unique<BlockStmt>(std::move(stmts), exprLine);
                }

                auto decl = std::make_shared<FuncDeclStmt>("", std::move(params), std::move(body), nullptr, false, arrowLine);
                return std::make_unique<LambdaExpr>(decl, arrowLine);
            }

            current++;
            return std::make_unique<IdentifierExpr>(token.value, token.line);
        }

        
        case TokenType::OPEN_SQUARE_BRACKET:
        {
            int arrayLine = token.line;
            current++; // consume '['
            std::vector<std::unique_ptr<Expr>> elements;
            checkNotError();
            if (peek().type != TokenType::CLOSE_SQUARE_BRACKET)
            {
                while (true)
                {
                    elements.push_back(parseAssignment());
                    checkNotError();
                    if (peek().type == TokenType::COMMA)
                    {
                        current++; // consume ','
                        checkNotError();
                        if (peek().type == TokenType::CLOSE_SQUARE_BRACKET)
                        {
                            break; // trailing comma
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
            if (peek().type != TokenType::CLOSE_SQUARE_BRACKET)
            {
                errorAt(peek(), "Array literal band karne ke liye ']' chahiye! Mila: '" + peek().value + "'");
            }
            current++; // consume ']'
            return std::make_unique<ArrayLiteralExpr>(std::move(elements), arrayLine);
        }

        case TokenType::OPEN_BRACE:
        {
            int mapLine = token.line;
            current++; // consume '{'
            checkNotError();
            if (peek().type == TokenType::CLOSE_BRACE)
            {
                current++; // consume '}'
                return std::make_unique<MapLiteralExpr>(
                    std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>>{}, mapLine);
            }

            // If begins with an identifier followed directly by ':', it's an unquoted-key map: { foo: 1 }
            if (peek().type == TokenType::IDENTIFIER && peekNext().type == TokenType::COLON)
            {
                std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> entries;
                while (true)
                {
                    if (peek().type == TokenType::ELLIPSIS)
                    {
                        Token elToken = peek();
                        current++; // consume '...'
                        std::unique_ptr<Expr> spreadTarget = parseAssignment();
                        auto spExpr = std::make_unique<SpreadExpr>(std::move(spreadTarget), elToken.line);
                        entries.emplace_back(std::move(spExpr), nullptr);
                    }
                    else
                    {
                        std::unique_ptr<Expr> key;
                        if (peek().type == TokenType::IDENTIFIER && peekNext().type == TokenType::COLON)
                        {
                            key = std::make_unique<StringExpr>(peek().value, peek().line);
                            current++;
                        }
                        else
                        {
                            key = parseAssignment();
                        }
                        checkNotError();
                        if (peek().type != TokenType::COLON)
                        {
                            errorAt(peek(), "Khata (map) mein key aur value ke beech ':' lagana zaroori hai! Mila: '" + peek().value + "'");
                        }
                        current++; // consume ':'
                        std::unique_ptr<Expr> val = parseAssignment();
                        entries.emplace_back(std::move(key), std::move(val));
                    }
                    checkNotError();
                    if (peek().type == TokenType::COMMA)
                    {
                        current++; // consume ','
                        checkNotError();
                        if (peek().type == TokenType::CLOSE_BRACE)
                        {
                            break; // trailing comma
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                if (peek().type != TokenType::CLOSE_BRACE)
                {
                    errorAt(peek(), "Khata (map) band karne ke liye '}' chahiye! Mila: '" + peek().value + "'");
                }
                current++; // consume '}'
                return std::make_unique<MapLiteralExpr>(std::move(entries), mapLine);
            }

            // If begins with ellipsis, it's a map spread
            if (peek().type == TokenType::ELLIPSIS)
            {
                std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> entries;
                while (true)
                {
                    if (peek().type == TokenType::ELLIPSIS)
                    {
                        Token elToken = peek();
                        current++; // consume '...'
                        std::unique_ptr<Expr> spreadTarget = parseAssignment();
                        auto spExpr = std::make_unique<SpreadExpr>(std::move(spreadTarget), elToken.line);
                        entries.emplace_back(std::move(spExpr), nullptr);
                    }
                    else
                    {
                        std::unique_ptr<Expr> key;
                        if (peek().type == TokenType::IDENTIFIER && peekNext().type == TokenType::COLON)
                        {
                            key = std::make_unique<StringExpr>(peek().value, peek().line);
                            current++;
                        }
                        else
                        {
                            key = parseAssignment();
                        }
                        checkNotError();
                        if (peek().type != TokenType::COLON)
                        {
                            errorAt(peek(), "Khata (map) mein key aur value ke beech ':' lagana zaroori hai! Mila: '" + peek().value + "'");
                        }
                        current++; // consume ':'
                        std::unique_ptr<Expr> val = parseAssignment();
                        entries.emplace_back(std::move(key), std::move(val));
                    }
                    checkNotError();
                    if (peek().type == TokenType::COMMA)
                    {
                        current++; // consume ','
                        checkNotError();
                        if (peek().type == TokenType::CLOSE_BRACE)
                        {
                            break; // trailing comma
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                if (peek().type != TokenType::CLOSE_BRACE)
                {
                    errorAt(peek(), "Khata (map) band karne ke liye '}' chahiye! Mila: '" + peek().value + "'");
                }
                current++; // consume '}'
                return std::make_unique<MapLiteralExpr>(std::move(entries), mapLine);
            }

            // Parse first element to distinguish map vs set
            auto firstExpr = parseAssignment();
            checkNotError();

            if (peek().type == TokenType::COLON)
            {
                // It's a map! { key: val, ... }
                current++; // consume ':'
                std::unique_ptr<Expr> firstVal = parseAssignment();
                std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> entries;
                entries.emplace_back(std::move(firstExpr), std::move(firstVal));
                while (peek().type == TokenType::COMMA)
                {
                    current++; // consume ','
                    checkNotError();
                    if (peek().type == TokenType::CLOSE_BRACE)
                    {
                        break; // trailing comma
                    }
                    if (peek().type == TokenType::ELLIPSIS)
                    {
                        Token elToken = peek();
                        current++; // consume '...'
                        std::unique_ptr<Expr> spreadTarget = parseAssignment();
                        auto spExpr = std::make_unique<SpreadExpr>(std::move(spreadTarget), elToken.line);
                        entries.emplace_back(std::move(spExpr), nullptr);
                    }
                    else
                    {
                        std::unique_ptr<Expr> key;
                        if (peek().type == TokenType::IDENTIFIER && peekNext().type == TokenType::COLON)
                        {
                            key = std::make_unique<StringExpr>(peek().value, peek().line);
                            current++;
                        }
                        else
                        {
                            key = parseAssignment();
                        }
                        checkNotError();
                        if (peek().type != TokenType::COLON)
                        {
                            errorAt(peek(), "Khata (map) mein key aur value ke beech ':' lagana zaroori hai! Mila: '" + peek().value + "'");
                        }
                        current++; // consume ':'
                        std::unique_ptr<Expr> val = parseAssignment();
                        entries.emplace_back(std::move(key), std::move(val));
                    }
                    checkNotError();
                }
                if (peek().type != TokenType::CLOSE_BRACE)
                {
                    errorAt(peek(), "Khata (map) band karne ke liye '}' chahiye! Mila: '" + peek().value + "'");
                }
                current++; // consume '}'
                return std::make_unique<MapLiteralExpr>(std::move(entries), mapLine);
            }
            else
            {
                // It's a set! { elem1, elem2, ... }
                std::vector<std::unique_ptr<Expr>> elements;
                elements.push_back(std::move(firstExpr));
                while (peek().type == TokenType::COMMA)
                {
                    current++; // consume ','
                    checkNotError();
                    if (peek().type == TokenType::CLOSE_BRACE)
                    {
                        break; // trailing comma
                    }
                    elements.push_back(parseAssignment());
                    checkNotError();
                }
                if (peek().type != TokenType::CLOSE_BRACE)
                {
                    errorAt(peek(), "Set (jhund) band karne ke liye '}' chahiye! Mila: '" + peek().value + "'");
                }
                current++; // consume '}'
                return std::make_unique<SetLiteralExpr>(std::move(elements), mapLine);
            }
        }

        case TokenType::OPEN_PAREN:
        {
            if (isArrowLambdaAhead())
            {
                int arrowLine = token.line;
                current++; // consume '('

                std::vector<FuncParam> params = parseParameterList("");
                consumeCloseParen("arrow lambda parameters");

                std::shared_ptr<Type> returnType = nullptr;
                checkNotError();
                if (peek().type == TokenType::COLON || peek().type == TokenType::THIN_ARROW)
                {
                    current++;
                    returnType = parseTypeExpression();
                }

                checkNotError();
                if (peek().type != TokenType::ARROW)
                {
                    errorAt(peek(), "Arrow lambda mein '=>' lagana zaroori hai! Mila: '" + peek().value + "'");
                }
                current++; // consume '=>'

                std::unique_ptr<Stmt> body;
                checkNotError();
                if (peek().type == TokenType::OPEN_BRACE)
                {
                    body = parseBlockStmt();
                }
                else
                {
                    int exprLine = peek().line;
                    auto expr = parseTernary();
                    std::vector<std::unique_ptr<Stmt>> stmts;
                    stmts.push_back(std::make_unique<ReturnStmt>(std::move(expr), exprLine));
                    body = std::make_unique<BlockStmt>(std::move(stmts), exprLine);
                }

                auto decl = std::make_shared<FuncDeclStmt>("", std::move(params), std::move(body), returnType, false, arrowLine);
                return std::make_unique<LambdaExpr>(decl, arrowLine);
            }

            int parenLine = token.line;
            current++; // consume '('

            // Check for empty tuple: ()
            if (peek().type == TokenType::CLOSE_PAREN)
            {
                current++; // consume ')'
                return std::make_unique<TupleExpr>(std::vector<std::unique_ptr<Expr>>{}, parenLine);
            }

            std::unique_ptr<Expr> expr = parseTernary();

            checkNotError();
            rejectAssignment("bracket '('");

            if (peek().type == TokenType::COMMA)
            {
                std::vector<std::unique_ptr<Expr>> elements;
                elements.push_back(std::move(expr));
                while (peek().type == TokenType::COMMA)
                {
                    current++; // consume ','
                    checkNotError();
                    if (peek().type == TokenType::CLOSE_PAREN)
                    {
                        break; // trailing comma, e.g. (a,)
                    }
                    elements.push_back(parseTernary());
                    checkNotError();
                }
                consumeCloseParen("tuple bracket ')'");
                return std::make_unique<TupleExpr>(std::move(elements), parenLine);
            }

            consumeCloseParen("bracket '('");
            return expr;
        }

        case TokenType::LESS_THAN:
        {
            if (isJsxElementAhead())
            {
                return parseJsxElement();
            }
            errorAt(token,
                "Aayein? Baigan! 😭\n"
                "Ye '<' beech mein kahan se tapak pada?\n"
                "Saala ye dukh kahe khatam nahi hota be... dhang se syntax likh le!");
        }

        case TokenType::ERROR:
            // Do not advance; pass the lexer's error message straight through
            errorAt(token, token.value);

        case TokenType::END_OF_FILE:
            // Invariant preserved: never advance past END_OF_FILE
            errorAt(current > 0 ? tokens[current - 1].line : token.line,
                "Moye Moye! Expression adhoori chhod ke code beech mein hi khatam kar diya.");

        default:
            errorAt(token,
                "Aayein? Baigan! 😭\n"
                "Ye '" +
                token.value + "' beech mein kahan se tapak pada?\n"
                "Saala ye dukh kahe khatam nahi hota be... dhang se syntax likh le!");
        }
    }

private:
    const std::vector<Token> tokens;
    size_t current;
    bool inJaanchGuard = false;

    // Invariant: tokens always ends with TokenType::END_OF_FILE,
    // and no parser rule ever advances `current` past TokenType::END_OF_FILE.
    const Token &peek() const
    {
        return tokens[current];
    }

    const Token &peekNext() const
    {
        if (current + 1 < tokens.size())
            return tokens[current + 1];
        return tokens.back();
    }

    const Token &peekAt(size_t offset) const
    {
        if (current + offset < tokens.size())
            return tokens[current + offset];
        return tokens.back();
    }

    bool isArrowLambdaAhead(size_t offset = 0) const
    {
        if (current + offset >= tokens.size()) return false;
        if (tokens[current + offset].type != TokenType::OPEN_PAREN)
            return false;

        size_t i = current + offset + 1;
        int parenDepth = 1;
        bool hasComparisonOrLogic = false;
        while (i < tokens.size() && tokens[i].type != TokenType::END_OF_FILE)
        {
            if (parenDepth == 1)
            {
                if (tokens[i].type == TokenType::GREATER_THAN ||
                    tokens[i].type == TokenType::LESS_THAN ||
                    tokens[i].type == TokenType::EQUAL ||
                    tokens[i].type == TokenType::NOT_EQUAL ||
                    tokens[i].type == TokenType::LESS_THAN_EQUAL ||
                    tokens[i].type == TokenType::GREATER_THAN_EQUAL ||
                    tokens[i].type == TokenType::AUR ||
                    tokens[i].type == TokenType::YA)
                {
                    hasComparisonOrLogic = true;
                }
            }
            if (tokens[i].type == TokenType::OPEN_PAREN)
            {
                parenDepth++;
            }
            else if (tokens[i].type == TokenType::CLOSE_PAREN)
            {
                parenDepth--;
                if (parenDepth == 0)
                {
                    if (hasComparisonOrLogic) return false;
                    if (i + 1 < tokens.size() && tokens[i + 1].type == TokenType::ARROW)
                    {
                        return true;
                    }
                    if (i + 1 < tokens.size() && (tokens[i + 1].type == TokenType::COLON || tokens[i + 1].type == TokenType::THIN_ARROW))
                    {
                        size_t j = i + 2;
                        while (j < tokens.size() && tokens[j].type != TokenType::SEMICOLON &&
                               tokens[j].type != TokenType::OPEN_BRACE && tokens[j].type != TokenType::END_OF_FILE)
                        {
                            if (tokens[j].type == TokenType::ARROW)
                            {
                                return true;
                            }
                            j++;
                        }
                    }
                    return false;
                }
            }
            i++;
        }
        return false;
    }

    std::vector<FuncParam> parseParameterList(const std::string &)
    {
        std::vector<FuncParam> params;
        bool seenDefault = false;
        bool seenVariadic = false;

        checkNotError();
        if (peek().type == TokenType::CLOSE_PAREN)
        {
            return params;
        }

        while (peek().type != TokenType::CLOSE_PAREN && peek().type != TokenType::END_OF_FILE)
        {
            checkNotError();
            if (seenVariadic)
            {
                errorAt(peek(), "Variadic parameter ('*') ke baad aur koi parameter nahi aa sakta!");
            }

            if (peek().type == TokenType::OPEN_BRACE)
            {
                current++; // consume '{'
                FuncParam mapParam;
                mapParam.isMapDestructure = true;
                mapParam.name = "__destruct_" + std::to_string(current);
                while (peek().type != TokenType::CLOSE_BRACE && peek().type != TokenType::END_OF_FILE)
                {
                    checkNotError();
                    if (peek().type != TokenType::IDENTIFIER)
                    {
                        errorAt(peek(), "Parameter destructuring mein variable ka naam chahiye! Mila: '" + peek().value + "'");
                    }
                    std::string keyName = peek().value;
                    current++; // consume identifier
                    std::unique_ptr<Expr> defVal = nullptr;
                    checkNotError();
                    if (peek().type == TokenType::ASSIGN)
                    {
                        current++; // consume '='
                        defVal = parseAssignment();
                    }
                    mapParam.destructureKeys.push_back({keyName, std::move(defVal)});
                    checkNotError();
                    if (peek().type == TokenType::COMMA)
                    {
                        current++; // consume ','
                        checkNotError();
                        if (peek().type == TokenType::CLOSE_BRACE) break;
                    }
                    else
                    {
                        break;
                    }
                }
                if (peek().type != TokenType::CLOSE_BRACE)
                {
                    errorAt(peek(), "Parameter destructuring band karne ke liye '}' lagao!");
                }
                current++; // consume '}'
                params.push_back(std::move(mapParam));

                checkNotError();
                if (peek().type == TokenType::COMMA)
                {
                    current++; // consume ','
                    checkNotError();
                    if (peek().type == TokenType::CLOSE_PAREN) break;
                }
                else
                {
                    break;
                }
                continue;
            }

            bool isVar = false;
            if (peek().type == TokenType::MULTIPLY)
            {
                isVar = true;
                current++; // consume '*'
                checkNotError();
            }

            if (!isIdentifierOrVal(peek()))
            {
                errorAt(peek(), "Ye kaisa ulta-pulta parameter naam hai: '" + peek().value + "'? Sahi naam rakho!");
            }
            std::string paramName = peek().value;
            current++; // consume identifier

            std::shared_ptr<Type> paramType = nullptr;
            checkNotError();
            if (peek().type == TokenType::COLON)
            {
                current++; // consume ':'
                paramType = parseTypeExpression();
            }

            std::unique_ptr<Expr> defaultVal = nullptr;
            checkNotError();
            if (peek().type == TokenType::ASSIGN)
            {
                if (isVar)
                {
                    errorAt(peek(), "Variadic parameter ('*" + paramName + "') mein default value nahi de sakte!");
                }
                current++; // consume '='
                defaultVal = parseAssignment();
                seenDefault = true;
            }
            else
            {
                if (seenDefault && !isVar)
                {
                    errorAt(peek(), "Ye sab doglapan hai! Default parameter ke baad bina default wala parameter nahi aa sakta!");
                }
            }

            if (isVar)
            {
                seenVariadic = true;
            }

            params.push_back(FuncParam{paramName, std::move(defaultVal), isVar, paramType});

            checkNotError();
            if (peek().type == TokenType::COMMA)
            {
                current++; // consume ','
                checkNotError();
                if (!isIdentifierOrVal(peek()) && peek().type != TokenType::MULTIPLY)
                {
                    errorAt(peek(), "',' lagaya hai toh aage parameter ka naam bhi likho, suspense mat banao!");
                }
            }
            else
            {
                break;
            }
        }

        return params;
    }

    std::unique_ptr<Expr> parseArgument()
    {
        checkNotError();
        if (peek().type == TokenType::MULTIPLY)
        {
            int spreadLine = peek().line;
            current++; // consume '*'
            auto expr = parseTernary();
            return std::make_unique<SpreadExpr>(std::move(expr), spreadLine);
        }
        return parseTernary();
    }

    void checkNotError() const
    {
        if (peek().type == TokenType::ERROR)
        {
            errorAt(peek(), peek().value);
        }
    }

    void rejectAssignment(const std::string &context) const
    {
        if (peek().type == TokenType::ASSIGN)
        {
            errorAt(peek(),
                "Control Uday, control! '" + context + "' mein '=' (assign) nahi, "
                "'==' (compare) lagta hai!");
        }
    }

    // Shared helper to parse `, expr` items into an argument list
    void parseRemainingCommaArgs(std::vector<std::unique_ptr<Expr>> &args,
                                 const std::string &context)
    {
        while (peek().type == TokenType::COMMA)
        {
            current++; // consume ','
            checkNotError();

            if (peek().type == TokenType::CLOSE_PAREN)
            {
                errorAt(peek(),
                    "',' lagaya hai toh aage argument bhi do! ')' mila.");
            }

            if (peek().type == TokenType::SEMICOLON)
            {
                errorAt(peek(),
                    "',' lagaya hai toh aage argument bhi do! ';' mila.");
            }

            args.push_back(parseArgument());
            rejectAssignment(context);
        }
    }

    // Shared helper to consume ')' with consistent EOF and wrong-token messages
    void consumeCloseParen(const std::string &context)
    {
        checkNotError();

        if (peek().type == TokenType::END_OF_FILE)
        {
            int line = current > 0 ? tokens[current - 1].line : peek().line;
            errorAt(line,
                context + " ka '(' khol ke bhool gaye! ')' band karna bhool gaye.");
        }

        if (peek().type != TokenType::CLOSE_PAREN)
        {
            errorAt(peek(),
                context + " ka '(' khol ke bhool gaye! ')' ki jagah mila: '" + peek().value + "'");
        }

        current++; // consume ')'
    }

    // Shared helper for `(` argument lists up to and including `)`
    std::vector<std::unique_ptr<Expr>> parseArgumentList(const std::string &context)
    {
        checkNotError();
        std::vector<std::unique_ptr<Expr>> args;

        if (peek().type != TokenType::CLOSE_PAREN)
        {
            args.push_back(parseArgument());
            rejectAssignment(context);
            parseRemainingCommaArgs(args, context);
        }

        consumeCloseParen(context);
        return args;
    }

    void consumeSemicolon()
    {
        checkNotError();

        if (peek().type == TokenType::SEMICOLON)
        {
            while (peek().type == TokenType::SEMICOLON)
            {
                current++;
            }
            return;
        }

        // Optional semicolon: statement ended by '}', EOF, or a newline
        if (peek().type == TokenType::CLOSE_BRACE || peek().type == TokenType::END_OF_FILE)
        {
            return;
        }

        if (current > 0 && peek().line > tokens[current - 1].line)
        {
            return;
        }

        int line = current > 0 ? tokens[current - 1].line : peek().line;
        errorAt(line,
            "Chhoti bachi ho kya?! Line ke end mein ';' lagana bhool gaye!");
    }
};

#endif