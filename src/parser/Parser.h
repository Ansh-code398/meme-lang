#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <stdexcept>
#include "../lexer/Lexer.h"

// ==========================================
// 1. Expression AST Hierarchy (Expr)
// ==========================================

class Expr
{
public:
    virtual ~Expr() = default;
    virtual std::string toString() const = 0;
};

class IntExpr : public Expr
{
public:
    long long value;
    IntExpr(long long value) : value(value) {}
    std::string toString() const override
    {
        return std::to_string(value);
    }
};

class FloatExpr : public Expr
{
public:
    double value;
    FloatExpr(double value) : value(value) {}
    std::string toString() const override
    {
        return std::to_string(value);
    }
};

class StringExpr : public Expr
{
public:
    std::string value;
    StringExpr(const std::string &value) : value(value) {}
    std::string toString() const override
    {
        return "\"" + value + "\"";
    }
};

class BoolExpr : public Expr
{
public:
    bool value;
    BoolExpr(bool value) : value(value) {}
    std::string toString() const override
    {
        return value ? "han" : "nhi";
    }
};

class NullExpr : public Expr
{
public:
    std::string toString() const override
    {
        return "kuch_nhi";
    }
};

class IdentifierExpr : public Expr
{
public:
    std::string name;
    IdentifierExpr(const std::string &name) : name(name) {}
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

    AssignExpr(const std::string &name, std::unique_ptr<Expr> value)
        : name(name), value(std::move(value)) {}

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

    UnaryExpr(Token op, std::unique_ptr<Expr> operand)
        : op(op), operand(std::move(operand)) {}

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

    BinaryExpr(std::unique_ptr<Expr> left, Token op, std::unique_ptr<Expr> right)
        : left(std::move(left)), op(op), right(std::move(right)) {}

    std::string toString() const override
    {
        return "(" + left->toString() + " " + op.value + " " + right->toString() + ")";
    }
};

class CallExpr : public Expr
{
public:
    std::unique_ptr<Expr> callee;
    std::vector<std::unique_ptr<Expr>> args;

    CallExpr(std::unique_ptr<Expr> callee, std::vector<std::unique_ptr<Expr>> args)
        : callee(std::move(callee)), args(std::move(args)) {}

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

    PoochExpr(std::unique_ptr<Expr> prompt = nullptr)
        : prompt(std::move(prompt)) {}

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

    GetExpr(std::unique_ptr<Expr> object, const std::string &name)
        : object(std::move(object)), name(name) {}

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

    SetExpr(std::unique_ptr<Expr> object, const std::string &name, std::unique_ptr<Expr> value)
        : object(std::move(object)), name(name), value(std::move(value)) {}

    std::string toString() const override
    {
        return "(" + object->toString() + "." + name + " = " + value->toString() + ")";
    }
};

// ==========================================
// 2. Statement AST Hierarchy (Stmt)
// ==========================================

class Stmt
{
public:
    virtual ~Stmt() = default;
    virtual std::string toString() const = 0;
};

// 1. Variable Declaration: `le x = 5;` or `le x;`
class VarDeclStmt : public Stmt
{
public:
    std::string name;
    std::unique_ptr<Expr> initializer; // nullptr if uninitialized (defaults to kuch_nhi)

    VarDeclStmt(const std::string &name, std::unique_ptr<Expr> initializer = nullptr)
        : name(name), initializer(std::move(initializer)) {}

    std::string toString() const override
    {
        if (initializer)
        {
            return "le " + name + " = " + initializer->toString() + ";";
        }
        return "le " + name + " = kuch_nhi;";
    }
};

// 2. Expression Statement: `x + 1;` or `x = 10;`
class ExprStmt : public Stmt
{
public:
    std::unique_ptr<Expr> expression;

    ExprStmt(std::unique_ptr<Expr> expression)
        : expression(std::move(expression)) {}

    std::string toString() const override
    {
        return expression->toString() + ";";
    }
};

// 3. Print Statement: `dikha a, b;`
class PrintStmt : public Stmt
{
public:
    std::vector<std::unique_ptr<Expr>> args;

    PrintStmt(std::vector<std::unique_ptr<Expr>> args)
        : args(std::move(args)) {}

    std::string toString() const override
    {
        std::string result = "dikha ";
        for (size_t i = 0; i < args.size(); ++i)
        {
            if (i > 0)
                result += ", ";
            result += args[i]->toString();
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

    BlockStmt(std::vector<std::unique_ptr<Stmt>> statements)
        : statements(std::move(statements)) {}

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

// 5. If / Else-If / Else Statement: `dekh cond { ... } warna dekh cond2 { ... } warna { ... }`
class IfStmt : public Stmt
{
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch; // nullptr, BlockStmt, or nested IfStmt (for `warna dekh`)

    IfStmt(std::unique_ptr<Expr> condition,
           std::unique_ptr<Stmt> thenBranch,
           std::unique_ptr<Stmt> elseBranch = nullptr)
        : condition(std::move(condition)),
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

    WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body)
        : condition(std::move(condition)), body(std::move(body)) {}

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

    ReturnStmt(std::unique_ptr<Expr> value = nullptr)
        : value(std::move(value)) {}

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
    std::string toString() const override
    {
        return "bas;";
    }
};

class ContinueStmt : public Stmt
{
public:
    std::string toString() const override
    {
        return "chal;";
    }
};

// 9. Function Declaration: `kar add(a, b) { ... }`
class FuncDeclStmt : public Stmt
{
public:
    std::string name;
    std::vector<std::string> params;
    std::unique_ptr<Stmt> body;

    FuncDeclStmt(const std::string &name,
                 std::vector<std::string> params,
                 std::unique_ptr<Stmt> body)
        : name(name), params(std::move(params)), body(std::move(body)) {}

    std::string toString() const override
    {
        std::string result = "kar " + name + "(";
        for (size_t i = 0; i < params.size(); ++i)
        {
            if (i > 0)
                result += ", ";
            result += params[i];
        }
        result += ") " + body->toString();
        return result;
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

    GhoomStmt(const std::string &varName,
              std::unique_ptr<Expr> start,
              std::unique_ptr<Expr> end,
              std::unique_ptr<Expr> step,
              std::unique_ptr<Stmt> body)
        : varName(varName),
          start(std::move(start)),
          end(std::move(end)),
          step(std::move(step)),
          body(std::move(body)) {}

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

// 11. Class Declaration: `sancha Name [se SuperName] { ... }`
class ClassDeclStmt : public Stmt
{
public:
    std::string name;
    std::string superclass;
    std::vector<std::unique_ptr<FuncDeclStmt>> methods;

    ClassDeclStmt(const std::string &name,
                  const std::string &superclass,
                  std::vector<std::unique_ptr<FuncDeclStmt>> methods)
        : name(name), superclass(superclass), methods(std::move(methods)) {}

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

// ==========================================
// 3. Parser Class
// ==========================================

class Parser
{
public:
    Parser(const std::vector<Token> &tokens) : tokens(tokens), current(0) {}

    // Parse an entire program (list of statements until END_OF_FILE)
    std::vector<std::unique_ptr<Stmt>> parseProgram()
    {
        std::vector<std::unique_ptr<Stmt>> statements;

        while (peek().type != TokenType::END_OF_FILE)
        {
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
            return parseVarDecl();
        case TokenType::DIKHA:
            return parsePrintStmt();
        case TokenType::OPEN_BRACE:
            return parseBlockStmt();
        case TokenType::DEKH:
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
            return parseFuncDeclStmt();
        case TokenType::GHOOM:
            return parseGhoomStmt();
        case TokenType::SANCHA:
            return parseClassDeclStmt();
        default:
            return parseExprStmt();
        }
    }

    // 1. `le x = 5;` or `le x;`
    std::unique_ptr<Stmt> parseVarDecl()
    {
        current++; // consume 'le'
        checkNotError();

        if (peek().type != TokenType::IDENTIFIER)
        {
            throw std::runtime_error(
                "Hawa pakdoge kya? 'pakad' ke baad variable ka naam toh likho! "
                "Mila: '" + peek().value + "'");
        }

        std::string varName = peek().value;
        current++; // consume variable name

        checkNotError();
        std::unique_ptr<Expr> initializer = nullptr;
        if (peek().type == TokenType::ASSIGN)
        {
            current++; // consume '='
            initializer = parseAssignment();
        }

        consumeSemicolon();
        return std::make_unique<VarDeclStmt>(varName, std::move(initializer));
    }

    // 2. `dikha expr1, expr2;`
    std::unique_ptr<Stmt> parsePrintStmt()
    {
        current++; // consume 'dikha'
        checkNotError();
        rejectAssignment("dikha");

        if (peek().type == TokenType::SEMICOLON)
        {
            throw std::runtime_error(
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
                throw std::runtime_error(
                    "'dikha()' khali kyun chhod diya? Andar kuch masala toh daal!");
            }

            std::unique_ptr<Expr> firstArg = parseLogicalOr();
            rejectAssignment("dikha");

            if (peek().type == TokenType::COMMA)
            {
                std::vector<std::unique_ptr<Expr>> dummyArgs;
                dummyArgs.push_back(std::move(firstArg));
                parseRemainingCommaArgs(dummyArgs, "dikha");

                // Confirm ')' first! On `dikha(1, 2 3);`, this throws the bracket error on '3'.
                consumeCloseParen("dikha ka bracket '('");

                // Once ')' is confirmed, reject parenthesized multi-arg `dikha(a, b)`
                throw std::runtime_error(
                    "'dikha(a, b)' mein brackets allowed nahi hain! "
                    "Brackets hata ke seedha 'dikha a, b;' likh.");
            }

            // Not a multi-arg `dikha(a, b)` -> rewind to '(' and parse normally
            current = savedPos;
        }

        std::vector<std::unique_ptr<Expr>> args;
        args.push_back(parseLogicalOr());
        rejectAssignment("dikha");
        parseRemainingCommaArgs(args, "dikha");

        consumeSemicolon();
        return std::make_unique<PrintStmt>(std::move(args));
    }

    // 3. `{ stmt1; stmt2; }` (No trailing semicolon)
    std::unique_ptr<Stmt> parseBlockStmt()
    {
        checkNotError();
        if (peek().type != TokenType::OPEN_BRACE)
        {
            throw std::runtime_error(
                "Block shuru karne ke liye '{' lagana padta hai! "
                "Mila: '" + peek().value + "'");
        }
        current++; // consume '{'

        std::vector<std::unique_ptr<Stmt>> statements;

        while (peek().type != TokenType::CLOSE_BRACE &&
               peek().type != TokenType::END_OF_FILE)
        {
            statements.push_back(parseStatement());
        }

        checkNotError();
        if (peek().type == TokenType::END_OF_FILE)
        {
            throw std::runtime_error(
                "Daya, darwaza खुला chhod diya! '{' khola hai toh '}' bhi laga.");
        }

        current++; // consume '}'
        return std::make_unique<BlockStmt>(std::move(statements));
    }

    // 4. `dekh cond { ... } warna dekh cond2 { ... } warna { ... }`
    std::unique_ptr<Stmt> parseIfStmt()
    {
        current++; // consume 'dekh'

        std::unique_ptr<Expr> condition = parseLogicalOr();
        rejectAssignment("dekh ki condition");

        std::unique_ptr<Stmt> thenBranch = parseBlockStmt();

        checkNotError();
        std::unique_ptr<Stmt> elseBranch = nullptr;
        if (peek().type == TokenType::WARNA)
        {
            current++; // consume 'warna'
            checkNotError();

            if (peek().type == TokenType::DEKH)
            {
                elseBranch = parseIfStmt();
            }
            else
            {
                elseBranch = parseBlockStmt();
            }
        }

        return std::make_unique<IfStmt>(
            std::move(condition),
            std::move(thenBranch),
            std::move(elseBranch));
    }

    // 5. `jabtak cond { ... }`
    std::unique_ptr<Stmt> parseWhileStmt()
    {
        current++; // consume 'jabtak'

        std::unique_ptr<Expr> condition = parseLogicalOr();
        rejectAssignment("jabtak ki condition");

        std::unique_ptr<Stmt> body = parseBlockStmt();
        return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
    }

    // 6. `wapas expr;` or `wapas;`
    std::unique_ptr<Stmt> parseReturnStmt()
    {
        current++; // consume 'wapas'
        checkNotError();

        std::unique_ptr<Expr> value = nullptr;
        if (peek().type != TokenType::SEMICOLON)
        {
            value = parseLogicalOr();
            rejectAssignment("wapas");
        }

        consumeSemicolon();
        return std::make_unique<ReturnStmt>(std::move(value));
    }

    // 7. `bas;`
    std::unique_ptr<Stmt> parseBreakStmt()
    {
        current++; // consume 'bas'
        consumeSemicolon();
        return std::make_unique<BreakStmt>();
    }

    // 8. `chal;`
    std::unique_ptr<Stmt> parseContinueStmt()
    {
        current++; // consume 'chal'
        consumeSemicolon();
        return std::make_unique<ContinueStmt>();
    }

    // 9. `kar funcName(a, b) { ... }`
    std::unique_ptr<Stmt> parseFuncDeclStmt()
    {
        current++; // consume 'kar'
        checkNotError();

        if (peek().type != TokenType::IDENTIFIER)
        {
            throw std::runtime_error(
                "Bina naam ka jugaad? 'jugaad' ke baad function ka naam toh de! Mila: '" + peek().value + "'");
        }
        std::string funcName = peek().value;
        current++; // consume function name

        checkNotError();
        if (peek().type != TokenType::OPEN_PAREN)
        {
            throw std::runtime_error(
                "Jugaad '" + funcName + "' ke baad '(' lagana bhool gaye! Bina bracket ke kaise chalega?");
        }
        current++; // consume '('

        std::vector<std::string> params;
        checkNotError();
        if (peek().type != TokenType::CLOSE_PAREN)
        {
            if (peek().type != TokenType::IDENTIFIER)
            {
                throw std::runtime_error("Ye kaisa ulta-pulta parameter naam hai: '" + peek().value + "'? Sahi naam rakho!");
            }
            params.push_back(peek().value);
            current++;

            while (peek().type == TokenType::COMMA)
            {
                current++; // consume ','
                checkNotError();
                if (peek().type != TokenType::IDENTIFIER)
                {
                    throw std::runtime_error("',' lagaya hai toh aage parameter ka naam bhi likho, suspense mat banao!");
                }
                params.push_back(peek().value);
                current++;
            }
        }

        consumeCloseParen("function '" + funcName + "' ke parameters");

        std::unique_ptr<Stmt> body = parseBlockStmt();
        return std::make_unique<FuncDeclStmt>(funcName, std::move(params), std::move(body));
    }

    // 10. `ghoom i se 1 se 10 { ... }` (optional step: `ghoom i se 1 se 10 se 2 { ... }`)
    std::unique_ptr<Stmt> parseGhoomStmt()
    {
        current++; // consume 'ghoom'
        checkNotError();

        if (peek().type != TokenType::IDENTIFIER)
        {
            throw std::runtime_error(
                "Kisko ghumana hai? 'ghuma' ke baad loop variable ka naam toh likh! Mila: '" + peek().value + "'");
        }
        std::string varName = peek().value;
        current++; // consume variable name

        checkNotError();
        if (peek().type != TokenType::SE)
        {
            throw std::runtime_error(
                "Chakkar aa jayega! 'ghuma " + varName + "' ke baad 'se' lagana zaroori hai (Jaise: ghuma " + varName + " se 1 se 10)");
        }
        current++; // consume 'se'

        std::unique_ptr<Expr> start = parseLogicalOr();
        checkNotError();

        if (peek().type != TokenType::SE)
        {
            throw std::runtime_error(
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
        return std::make_unique<GhoomStmt>(varName, std::move(start), std::move(end), std::move(step), std::move(body));
    }

    // 11. `biradari Name [nepo SuperName] { jugaad method() { ... } }`
    std::unique_ptr<Stmt> parseClassDeclStmt()
    {
        current++; // consume 'sancha' / 'biradari' / 'class'
        checkNotError();

        if (peek().type != TokenType::IDENTIFIER)
        {
            throw std::runtime_error(
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
                throw std::runtime_error("Nepo kid kaunse khandaan ka hai? 'nepo' ke baad parent biradari ka naam chahiye!");
            }
            superclass = peek().value;
            current++; // consume superclass name
        }

        checkNotError();
        if (peek().type != TokenType::OPEN_BRACE)
        {
            throw std::runtime_error(
                "Biradari ke andar ghusne ke liye '{' lagana padega! Mila: '" + peek().value + "'");
        }
        current++; // consume '{'

        std::vector<std::unique_ptr<FuncDeclStmt>> methods;
        while (peek().type != TokenType::CLOSE_BRACE && peek().type != TokenType::END_OF_FILE)
        {
            checkNotError();
            if (peek().type != TokenType::KAR)
            {
                throw std::runtime_error(
                    "Biradari ke andar sirf methods ('jugaad naam() { ... }') chalte hain! Ye '" + peek().value + "' kahan se aaya?");
            }
            auto method = parseFuncDeclStmt();
            methods.push_back(std::unique_ptr<FuncDeclStmt>(static_cast<FuncDeclStmt *>(method.release())));
        }

        if (peek().type == TokenType::END_OF_FILE)
        {
            throw std::runtime_error(
                "Khatam, tata, bye-bye! Biradari ka '{' khola par '}' band karna bhool gaye.");
        }
        current++; // consume '}'

        return std::make_unique<ClassDeclStmt>(className, superclass, std::move(methods));
    }

    // 12. Expression Statement: `x = 10;` or `x + 1;`
    std::unique_ptr<Stmt> parseExprStmt()
    {
        std::unique_ptr<Expr> expr = parseAssignment();
        consumeSemicolon();
        return std::make_unique<ExprStmt>(std::move(expr));
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
            throw std::runtime_error(remaining.value);
        }

        if (remaining.type != TokenType::END_OF_FILE)
        {
            throw std::runtime_error(
                "Goli beta, masti nahi! Expression khatam ho chuki hai, "
                "piche ye extra '" +
                remaining.value + "' kyun chipka diya?");
        }

        return expr;
    }

    // Assignment expression: `x = expr` (right-associative)
    // Rule: The left-hand side must be a bare identifier (not parenthesized like `(x) = 5`),
    // and assignments are disallowed inside sub-expressions, conditions, or argument lists.
    std::unique_ptr<Expr> parseAssignment()
    {
        bool startsWithBareIdentifier = (peek().type == TokenType::IDENTIFIER);
        std::unique_ptr<Expr> expr = parseLogicalOr();

        if (peek().type == TokenType::ASSIGN)
        {
            current++; // consume '='
            std::unique_ptr<Expr> value = parseAssignment();

            if (startsWithBareIdentifier)
            {
                if (auto *ident = dynamic_cast<IdentifierExpr *>(expr.get()))
                {
                    return std::make_unique<AssignExpr>(ident->name, std::move(value));
                }
            }

            if (auto *get = dynamic_cast<GetExpr *>(expr.get()))
            {
                return std::make_unique<SetExpr>(std::move(get->object), get->name, std::move(value));
            }

            throw std::runtime_error(
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
            left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
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
            left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
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
            left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
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
            left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
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
            left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
        }

        return left;
    }

    // Unary prefix: `!`, `-` (right-associative, calls parseCall() below it)
    std::unique_ptr<Expr> parseUnary()
    {
        if (peek().type == TokenType::NOT ||
            peek().type == TokenType::MINUS)
        {
            Token op = peek();
            current++;

            std::unique_ptr<Expr> operand = parseUnary();
            return std::make_unique<UnaryExpr>(op, std::move(operand));
        }

        return parseCall();
    }

    // Postfix function call & property access layer: `f(1, 2)`, `obj.prop`, `obj.method(1)`
    std::unique_ptr<Expr> parseCall()
    {
        std::unique_ptr<Expr> expr = parsePrimary();

        while (true)
        {
            if (peek().type == TokenType::OPEN_PAREN)
            {
                current++; // consume '('
                std::vector<std::unique_ptr<Expr>> args = parseArgumentList("function call");
                expr = std::make_unique<CallExpr>(std::move(expr), std::move(args));
            }
            else if (peek().type == TokenType::DOT)
            {
                current++; // consume '.'
                checkNotError();
                if (peek().type != TokenType::IDENTIFIER)
                {
                    throw std::runtime_error(
                        "'.' ke baad property ya method ka naam chahiye! Mila: '" + peek().value + "'");
                }
                std::string propName = peek().value;
                current++; // consume property name
                expr = std::make_unique<GetExpr>(std::move(expr), propName);
            }
            else
            {
                break;
            }
        }

        return expr;
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
                auto expr = std::make_unique<IntExpr>(
                    std::stoll(token.value));

                current++;
                return expr;
            }
            catch (const std::invalid_argument &)
            {
                throw std::runtime_error(
                    "Aayein? Ye kaisa number likh diya tune? Calculator bhi ro pada: " +
                    token.value);
            }
            catch (const std::out_of_range &)
            {
                throw std::runtime_error(
                    "7 Crore!! Itna bada number RAM ki aukaat ke bahar hai, thoda chota fek: " +
                    token.value);
            }
        }

        case TokenType::FLOAT_LITERAL:
        {
            try
            {
                auto expr = std::make_unique<FloatExpr>(
                    std::stod(token.value));

                current++;
                return expr;
            }
            catch (const std::invalid_argument &)
            {
                throw std::runtime_error(
                    "Arre kehna kya chahte ho? Ye point wala number toh kisi angle se nahi lag raha: " +
                    token.value);
            }
            catch (const std::out_of_range &)
            {
                throw std::runtime_error(
                    "Ye kis line mein aa gaye aap? Itna bada decimal dekh ke CPU ka dimaag phat gaya: " +
                    token.value);
            }
        }

        case TokenType::STRING_LITERAL:
            current++;
            return std::make_unique<StringExpr>(token.value);

        case TokenType::HAN:
            current++;
            return std::make_unique<BoolExpr>(true);

        case TokenType::NHI:
            current++;
            return std::make_unique<BoolExpr>(false);

        case TokenType::KUCH_NHI:
            current++;
            return std::make_unique<NullExpr>();

        case TokenType::POOCH:
        {
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
            return std::make_unique<PoochExpr>(std::move(prompt));
        }

        case TokenType::IDENTIFIER:
            current++;
            return std::make_unique<IdentifierExpr>(token.value);

        
        case TokenType::OPEN_PAREN:
        {
            current++; // consume '('

            std::unique_ptr<Expr> expr = parseLogicalOr();

            checkNotError();
            rejectAssignment("bracket '('");

            if (peek().type == TokenType::COMMA)
            {
                throw std::runtime_error(
                    "Bracket '(' ke andar ',' allowed nahi hai! "
                    "Grouping bracket mein sirf ek expression aa sakta hai.");
            }

            consumeCloseParen("bracket '('");
            return expr;
        }

        case TokenType::ERROR:
            // Do not advance; pass the lexer's error message straight through
            throw std::runtime_error(token.value);

        case TokenType::END_OF_FILE:
            // Invariant preserved: never advance past END_OF_FILE
            throw std::runtime_error(
                "Moye Moye! Expression adhoori chhod ke code beech mein hi khatam kar diya.");

        default:
            throw std::runtime_error(
                "Aayein? Baigan! 😭\n"
                "Ye '" +
                token.value + "' beech mein kahan se tapak pada?\n"
                "Saala ye dukh kahe khatam nahi hota be... dhang se syntax likh le!");
        }
    }

private:
    const std::vector<Token> tokens;
    size_t current;

    // Invariant: tokens always ends with TokenType::END_OF_FILE,
    // and no parser rule ever advances `current` past TokenType::END_OF_FILE.
    const Token &peek() const
    {
        return tokens[current];
    }

    void checkNotError() const
    {
        if (peek().type == TokenType::ERROR)
        {
            throw std::runtime_error(peek().value);
        }
    }

    void rejectAssignment(const std::string &context) const
    {
        if (peek().type == TokenType::ASSIGN)
        {
            throw std::runtime_error(
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
                throw std::runtime_error(
                    "',' lagaya hai toh aage argument bhi do! ')' mila.");
            }

            if (peek().type == TokenType::SEMICOLON)
            {
                throw std::runtime_error(
                    "',' lagaya hai toh aage argument bhi do! ';' mila.");
            }

            args.push_back(parseLogicalOr());
            rejectAssignment(context);
        }
    }

    // Shared helper to consume ')' with consistent EOF and wrong-token messages
    void consumeCloseParen(const std::string &context)
    {
        checkNotError();

        if (peek().type == TokenType::END_OF_FILE)
        {
            throw std::runtime_error(
                context + " ka '(' khol ke bhool gaye! ')' band karna bhool gaye.");
        }

        if (peek().type != TokenType::CLOSE_PAREN)
        {
            throw std::runtime_error(
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
            args.push_back(parseLogicalOr());
            rejectAssignment(context);
            parseRemainingCommaArgs(args, context);
        }

        consumeCloseParen(context);
        return args;
    }

    void consumeSemicolon()
    {
        checkNotError();

        if (peek().type != TokenType::SEMICOLON)
        {
            throw std::runtime_error(
                "Chhoti bachi ho kya?! Line ke end mein ';' lagana bhool gaye!");
        }

        current++; // consume ';'
    }
};

#endif