#include "Interpreter.h"
#include <stdexcept>
#include <iostream>

std::string instanceToString(const std::shared_ptr<Instance> &instance)
{
    return instance ? instance->toString() : "<object>";
}

Value UserFunction::call(Interpreter &interpreter, const std::vector<Value> &args)
{
    auto env = std::make_shared<Environment>(closure);
    for (size_t i = 0; i < declaration->params.size(); ++i)
    {
        env->define(declaration->params[i], args[i]);
    }

    try
    {
        if (auto *block = dynamic_cast<const BlockStmt *>(declaration->body.get()))
        {
            interpreter.executeBlock(block->statements, env);
        }
        else
        {
            // Execute non-block statement if any
            interpreter.execute(declaration->body.get());
        }
    }
    catch (const ReturnSignal &sig)
    {
        return sig.value;
    }

    return Value(std::monostate{});
}

Interpreter::Interpreter(std::ostream &out, std::istream &in)
    : globals(std::make_shared<Environment>()), environment(globals), out(out), in(in)
{
    // Type checking
    auto typeFunc = Value(std::make_shared<PataLagaBuiltin>());
    globals->define("kya_hai", typeFunc);
    globals->define("aukaat", typeFunc);
    globals->define("pata_laga", typeFunc);

    // String length
    auto lenFunc = Value(std::make_shared<LambaiBuiltin>());
    globals->define("size", lenFunc);
    globals->define("kitna_lamba", lenFunc);
    globals->define("lambai", lenFunc);

    // Whole integer conversion
    auto intFunc = Value(std::make_shared<AnkBuiltin>());
    globals->define("bina_point", intFunc);
    globals->define("pura_num", intFunc);
    globals->define("ank", intFunc);

    // Decimal float conversion
    auto floatFunc = Value(std::make_shared<DashamlavBuiltin>());
    globals->define("point_wala", floatFunc);
    globals->define("float_bana", floatFunc);
    globals->define("dashamlav", floatFunc);
}

void Interpreter::interpret(const std::vector<std::unique_ptr<Stmt>> &statements)
{
    try
    {
        for (const auto &stmt : statements)
        {
            execute(stmt.get());
        }
    }
    catch (const BreakSignal &)
    {
        throw std::runtime_error("Kisko rok rahe ho? Koi loop chal hi nahi raha toh 'bas_kar;' kyun likha?");
    }
    catch (const ContinueSignal &)
    {
        throw std::runtime_error("Kahan aage badhna hai? Loop ke bahar 'aage_badh;' kaam nahi karta!");
    }
    catch (const ReturnSignal &)
    {
        throw std::runtime_error("Kisko de rahe ho? Function ('jugaad') ke bahar 'ye_le;' nahi chalta!");
    }
}

void Interpreter::executeBlock(const std::vector<std::unique_ptr<Stmt>> &statements, std::shared_ptr<Environment> newEnv)
{
    auto previous = this->environment;
    this->environment = newEnv;
    try
    {
        for (const auto &stmt : statements)
        {
            execute(stmt.get());
        }
        this->environment = previous;
    }
    catch (...)
    {
        this->environment = previous;
        throw;
    }
}

void Interpreter::execute(const Stmt *stmt)
{
    if (!stmt)
        return;

    // 1. Variable declaration: `le x = 5;`
    if (auto *v = dynamic_cast<const VarDeclStmt *>(stmt))
    {
        Value val = v->initializer ? evaluate(v->initializer.get()) : Value(std::monostate{});
        environment->define(v->name, std::move(val));
        return;
    }

    // 2. Expression statement: `x = 10;` or `x + 1;`
    if (auto *e = dynamic_cast<const ExprStmt *>(stmt))
    {
        evaluate(e->expression.get());
        return;
    }

    // 3. Print statement: `dikha a, b;`
    if (auto *p = dynamic_cast<const PrintStmt *>(stmt))
    {
        for (size_t i = 0; i < p->args.size(); ++i)
        {
            if (i > 0)
                out << " ";
            Value val = evaluate(p->args[i].get());
            out << val.toString();
        }
        out << "\n";
        return;
    }

    // 4. Block statement: `{ ... }`
    if (auto *b = dynamic_cast<const BlockStmt *>(stmt))
    {
        executeBlock(b->statements, std::make_shared<Environment>(environment));
        return;
    }

    // 5. If statement: `dekh cond { ... } warna { ... }`
    if (auto *i = dynamic_cast<const IfStmt *>(stmt))
    {
        Value cond = evaluate(i->condition.get());
        if (cond.isTruthy())
        {
            execute(i->thenBranch.get());
        }
        else if (i->elseBranch)
        {
            execute(i->elseBranch.get());
        }
        return;
    }

    // 6. While statement: `jabtak cond { ... }`
    if (auto *w = dynamic_cast<const WhileStmt *>(stmt))
    {
        while (evaluate(w->condition.get()).isTruthy())
        {
            try
            {
                execute(w->body.get());
            }
            catch (const BreakSignal &)
            {
                break;
            }
            catch (const ContinueSignal &)
            {
                continue;
            }
        }
        return;
    }

    // 7. For/Range statement: `ghoom i se 1 se 10 { ... }`
    if (auto *g = dynamic_cast<const GhoomStmt *>(stmt))
    {
        Value startVal = evaluate(g->start.get());
        Value endVal = evaluate(g->end.get());

        long long start = 0;
        long long end = 0;

        if (std::holds_alternative<long long>(startVal.data))
        {
            start = std::get<long long>(startVal.data);
        }
        else if (std::holds_alternative<double>(startVal.data))
        {
            start = static_cast<long long>(std::get<double>(startVal.data));
        }
        else
        {
            throw std::runtime_error("Range shuru kahan se karein? 'ghuma' loop ka start number hona chahiye! Mila: '" + startVal.typeName() + "'");
        }

        if (std::holds_alternative<long long>(endVal.data))
        {
            end = std::get<long long>(endVal.data);
        }
        else if (std::holds_alternative<double>(endVal.data))
        {
            end = static_cast<long long>(std::get<double>(endVal.data));
        }
        else
        {
            throw std::runtime_error("Kahan tak jaana hai? 'ghuma' loop ka end number hona chahiye! Mila: '" + endVal.typeName() + "'");
        }

        long long step = (start <= end) ? 1 : -1;
        if (g->step)
        {
            Value stepVal = evaluate(g->step.get());
            if (std::holds_alternative<long long>(stepVal.data))
            {
                step = std::get<long long>(stepVal.data);
            }
            else if (std::holds_alternative<double>(stepVal.data))
            {
                step = static_cast<long long>(std::get<double>(stepVal.data));
            }
            else
            {
                throw std::runtime_error("Kitne kadam chalne hain? 'ghuma' loop ka step number hona chahiye!");
            }
            if (step == 0)
            {
                throw std::runtime_error("Step 0 rakhoge toh loop qayamat tak chalega! Infinite loop se bacho.");
            }
        }

        auto loopEnv = std::make_shared<Environment>(environment);
        for (long long cur = start; (step > 0 ? cur <= end : cur >= end); cur += step)
        {
            loopEnv->define(g->varName, Value(cur));
            auto prev = environment;
            environment = loopEnv;
            try
            {
                execute(g->body.get());
            }
            catch (const BreakSignal &)
            {
                environment = prev;
                break;
            }
            catch (const ContinueSignal &)
            {
                environment = prev;
                continue;
            }
            environment = prev;
        }
        return;
    }

    // 8. Return statement: `wapas expr;` or `wapas;`
    if (auto *r = dynamic_cast<const ReturnStmt *>(stmt))
    {
        Value val = r->value ? evaluate(r->value.get()) : Value(std::monostate{});
        throw ReturnSignal(val);
    }

    // 9. Break statement: `bas;`
    if (dynamic_cast<const BreakStmt *>(stmt))
    {
        throw BreakSignal();
    }

    // 10. Continue statement: `chal;`
    if (dynamic_cast<const ContinueStmt *>(stmt))
    {
        throw ContinueSignal();
    }

    // 11. Function declaration: `kar fname(a, b) { ... }`
    if (auto *f = dynamic_cast<const FuncDeclStmt *>(stmt))
    {
        auto func = std::make_shared<UserFunction>(f, environment);
        environment->define(f->name, Value(func));
        return;
    }

    // 12. Class declaration: `sancha Name [se SuperName] { ... }`
    if (auto *c = dynamic_cast<const ClassDeclStmt *>(stmt))
    {
        std::shared_ptr<ClassCallable> superclass = nullptr;
        if (!c->superclass.empty())
        {
            Value superVal = environment->get(c->superclass);
            if (!std::holds_alternative<std::shared_ptr<Callable>>(superVal.data))
            {
                throw std::runtime_error("Fake nepotism! '" + c->superclass + "' koi biradari (class) nahi hai, isko inherit kaise karoge?");
            }
            auto superCallable = std::get<std::shared_ptr<Callable>>(superVal.data);
            superclass = std::dynamic_pointer_cast<ClassCallable>(superCallable);
            if (!superclass)
            {
                throw std::runtime_error("Fake nepotism! '" + c->superclass + "' koi biradari (class) nahi hai!");
            }
        }

        std::unordered_map<std::string, std::shared_ptr<UserFunction>> methods;
        for (const auto &method : c->methods)
        {
            auto func = std::make_shared<UserFunction>(method.get(), environment);
            methods[method->name] = func;
        }

        auto klass = std::make_shared<ClassCallable>(c->name, superclass, std::move(methods));
        environment->define(c->name, Value(klass));
        return;
    }

    throw std::runtime_error("Ye kaisa anokha statement hai? Syntax samajh nahi aaya!");
}

Value Interpreter::evaluate(const Expr *expr)
{
    if (!expr)
        return Value(std::monostate{});

    // Literals
    if (auto *e = dynamic_cast<const IntExpr *>(expr))
        return Value(e->value);

    if (auto *e = dynamic_cast<const FloatExpr *>(expr))
        return Value(e->value);

    if (auto *e = dynamic_cast<const StringExpr *>(expr))
        return Value(e->value);

    if (auto *e = dynamic_cast<const BoolExpr *>(expr))
        return Value(e->value);

    if (dynamic_cast<const NullExpr *>(expr))
        return Value(std::monostate{});

    // Identifier lookup
    if (auto *e = dynamic_cast<const IdentifierExpr *>(expr))
        return environment->get(e->name);

    // Assignment: `x = value`
    if (auto *e = dynamic_cast<const AssignExpr *>(expr))
    {
        Value val = evaluate(e->value.get());
        environment->assign(e->name, val);
        return val;
    }

    // Unary expression: `-x`, `!x`
    if (auto *e = dynamic_cast<const UnaryExpr *>(expr))
    {
        Value operand = evaluate(e->operand.get());
        if (e->op.type == TokenType::MINUS)
            return negate(operand);
        if (e->op.type == TokenType::NOT)
            return logicalNot(operand);

        throw std::runtime_error("Ye operator '" + e->op.value + "' unary mein nahi chalta!");
    }

    // Binary expression
    if (auto *e = dynamic_cast<const BinaryExpr *>(expr))
    {
        // Short-circuiting logical AND: `aur`
        if (e->op.type == TokenType::AUR)
        {
            Value left = evaluate(e->left.get());
            if (!left.isTruthy())
                return left;
            return evaluate(e->right.get());
        }

        // Short-circuiting logical OR: `ya`
        if (e->op.type == TokenType::YA)
        {
            Value left = evaluate(e->left.get());
            if (left.isTruthy())
                return left;
            return evaluate(e->right.get());
        }

        Value left = evaluate(e->left.get());
        Value right = evaluate(e->right.get());

        switch (e->op.type)
        {
        case TokenType::PLUS:
            return add(left, right);
        case TokenType::MINUS:
            return subtract(left, right);
        case TokenType::MULTIPLY:
            return multiply(left, right);
        case TokenType::DIVIDE:
            return divide(left, right);
        case TokenType::MOD:
            return modulo(left, right);
        case TokenType::EQUAL:
            return equal(left, right);
        case TokenType::NOT_EQUAL:
            return notEqual(left, right);
        case TokenType::LESS_THAN:
            return lessThan(left, right);
        case TokenType::LESS_THAN_EQUAL:
            return lessThanEqual(left, right);
        case TokenType::GREATER_THAN:
            return greaterThan(left, right);
        case TokenType::GREATER_THAN_EQUAL:
            return greaterThanEqual(left, right);
        default:
            throw std::runtime_error("Ye kaisa operator ghusa diya: " + e->op.value);
        }
    }

    // Function call: `f(a, b)`
    if (auto *e = dynamic_cast<const CallExpr *>(expr))
    {
        Value callee = evaluate(e->callee.get());
        if (!std::holds_alternative<std::shared_ptr<Callable>>(callee.data))
        {
            throw std::runtime_error("Beta, tumse na ho payega! '" + callee.toString() + "' koi jugaad (function) thodi hai jo call kar rahe ho!");
        }

        auto function = std::get<std::shared_ptr<Callable>>(callee.data);
        if (!function)
        {
            throw std::runtime_error("Jugaad toh 'nalla' (null) nikla! Khali cheez ko call kaise karoge?");
        }

        if (e->args.size() != function->arity())
        {
            throw std::runtime_error(
                "Gaddari korbe! Jugaad ne " + std::to_string(function->arity()) +
                " arguments maange the, aur tumne " + std::to_string(e->args.size()) + " chipka diye!");
        }

        std::vector<Value> arguments;
        arguments.reserve(e->args.size());
        for (const auto &arg : e->args)
        {
            arguments.push_back(evaluate(arg.get()));
        }

        return function->call(*this, arguments);
    }

    // Input: `pooch()` or `pooch("Prompt: ")`
    if (auto *e = dynamic_cast<const PoochExpr *>(expr))
    {
        if (e->prompt)
        {
            Value p = evaluate(e->prompt.get());
            out << p.toString();
            out.flush();
        }

        std::string line;
        if (!std::getline(in, line))
        {
            return Value("");
        }
        return Value(line);
    }

    // Property get: `object.property`
    if (auto *g = dynamic_cast<const GetExpr *>(expr))
    {
        Value objVal = evaluate(g->object.get());
        if (!std::holds_alternative<std::shared_ptr<Instance>>(objVal.data))
        {
            throw std::runtime_error(
                "Aura -10000! '" + objVal.toString() + "' (" + objVal.typeName() +
                ") koi object thodi hai jo uski property nikaal rahe ho!");
        }
        auto instance = std::get<std::shared_ptr<Instance>>(objVal.data);
        if (!instance)
        {
            throw std::runtime_error("Object toh pura 'nalla' (null) hai! Nalle se kya property maang rahe ho?");
        }
        return instance->get(g->name);
    }

    // Property set: `object.property = value`
    if (auto *s = dynamic_cast<const SetExpr *>(expr))
    {
        Value objVal = evaluate(s->object.get());
        if (!std::holds_alternative<std::shared_ptr<Instance>>(objVal.data))
        {
            throw std::runtime_error(
                "Zabardasti hai kya? '" + objVal.toString() + "' (" + objVal.typeName() +
                ") koi object nahi hai, uspe property set nahi kar sakte!");
        }
        auto instance = std::get<std::shared_ptr<Instance>>(objVal.data);
        if (!instance)
        {
            throw std::runtime_error("Object 'nalla' (null) hai! Hawa mein property set nahi hoti.");
        }
        Value val = evaluate(s->value.get());
        instance->set(s->name, val);
        return val;
    }

    throw std::runtime_error("Ye expression samajh se pare hai!");
}
