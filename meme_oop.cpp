/**
 * ==============================================================================
 * PROJECT: MEMElang - Full Parody Object-Oriented Language Engine
 * PURPOSE: Comprehensive Single-File College Project Submission (< 1200 lines)
 *
 * OOP CONCEPTS DEMONSTRATED IN THIS CODEBASE:
 * 1. ENCAPSULATION: Scope isolation in Environment, object state in MemeInstance.
 * 2. ABSTRACTION  : Pure virtual base classes (Expr, Stmt, MemeCallable) defining AST nodes.
 * 3. INHERITANCE  : 
 *    - C++ Level    : Deep hierarchies for AST Nodes, Callables, and Control-flow Signals.
 *    - Language Level: Subclassing via 'se' / 'nepo' with dynamic method resolution.
 * 4. POLYMORPHISM : Dynamic dispatch via virtual eval()/exec(), method overriding,
 *                   and dynamic 'khud'/'apun' (this) pointer binding.
 *
 * COMPLETE FEATURE SET:
 * - Full OOP: Classes (sancha/biradari), Inheritance (se/nepo), Methods (kar/jugaad),
 *             Constructors (shuru), this (khud/apun), Dynamic properties, Overriding.
 * - Control Flow: dekh / warna (if / else if / else), jabtak (while), ghoom (for-range),
 *                 wapas / ye_le (return), bas / bas_kar (break), chal (continue).
 * - Full Operator Precedence: +, -, *, /, %, ==, !=, <, <=, >, >=, aur (&&), ya (||), !.
 * - Types & I/O: Strings with repeat ("ha"*5), numbers (int/float), booleans (haan/nahi),
 *                null (kuch_nahi/nalla), dikha/flex (print), puch/pooch (input),
 *                built-ins (point_wala, kya_hai, gin).
 * ==============================================================================
 */

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <sstream>
#include <fstream>
#include <cctype>
#include <cmath>
#include <stdexcept>
#include <functional>

using namespace std;

// --- Forward Declarations ---
struct Value;
class Environment;
class MemeCallable;
class MemeClass;
class MemeInstance;
class Stmt;
class BlockStmt;
class Expr;

// ==============================================================================
// 1. CONTROL FLOW SIGNALS (C++ Exception-Based Stack Unwinding)
// ==============================================================================
class ReturnSignal : public std::exception {
public:
    shared_ptr<Value> value;
    ReturnSignal(Value v);
};
class BreakSignal : public std::exception {};
class ContinueSignal : public std::exception {};

// ==============================================================================
// 2. RUNTIME VALUES & OPERATOR SEMANTICS
// ==============================================================================
struct Value {
    enum Type { NIL, BOOL, NUM, STR, CALLABLE, INSTANCE } type = NIL;
    bool bVal = false;
    double numVal = 0.0;
    bool isFloat = false;
    string strVal = "";
    shared_ptr<MemeCallable> callable = nullptr;
    shared_ptr<MemeInstance> inst = nullptr;

    Value() : type(NIL) {}
    Value(bool b) : type(BOOL), bVal(b) {}
    Value(long long n) : type(NUM), numVal((double)n), isFloat(false) {}
    Value(int n) : type(NUM), numVal((double)n), isFloat(false) {}
    Value(double d, bool flt = true) : type(NUM), numVal(d), isFloat(flt) {}
    Value(string s) : type(STR), strVal(std::move(s)) {}
    Value(const char* s) : type(STR), strVal(s) {}
    Value(shared_ptr<MemeCallable> c) : type(CALLABLE), callable(std::move(c)) {}
    Value(shared_ptr<MemeInstance> i) : type(INSTANCE), inst(std::move(i)) {}

    string typeName() const {
        switch (type) {
            case NIL: return "nalla";
            case BOOL: return "bool";
            case NUM: return isFloat ? "float" : "int";
            case STR: return "string";
            case CALLABLE: return "function";
            case INSTANCE: return "object";
        }
        return "unknown";
    }

    bool isTruthy() const {
        switch (type) {
            case NIL: return false;
            case BOOL: return bVal;
            case NUM: return numVal != 0.0;
            case STR: return !strVal.empty();
            default: return true;
        }
    }

    string toString() const {
        switch (type) {
            case NIL: return "nalla";
            case BOOL: return bVal ? "han" : "nhi";
            case NUM: {
                if (!isFloat) return to_string((long long)numVal);
                string s = to_string(numVal);
                if (s.find('.') != string::npos) {
                    while (s.size() > 1 && s.back() == '0') s.pop_back();
                    if (s.back() == '.') s.push_back('0');
                }
                return s;
            }
            case STR: return strVal;
            case CALLABLE: return "<function>";
            case INSTANCE: return "<object>";
        }
        return "";
    }

    void print() const { cout << toString(); }
};

ReturnSignal::ReturnSignal(Value v) : value(make_shared<Value>(std::move(v))) {}

bool valEquals(const Value& a, const Value& b) {
    if (a.type != b.type) {
        if (a.type == Value::NUM && b.type == Value::NUM) return a.numVal == b.numVal;
        return false;
    }
    switch (a.type) {
        case Value::NIL: return true;
        case Value::BOOL: return a.bVal == b.bVal;
        case Value::NUM: return a.numVal == b.numVal;
        case Value::STR: return a.strVal == b.strVal;
        case Value::CALLABLE: return a.callable == b.callable;
        case Value::INSTANCE: return a.inst == b.inst;
    }
    return false;
}

Value valAdd(const Value& a, const Value& b) {
    if (a.type == Value::NUM && b.type == Value::NUM) {
        return Value(a.numVal + b.numVal, a.isFloat || b.isFloat);
    }
    if (a.type == Value::STR || b.type == Value::STR) {
        return Value(a.toString() + b.toString());
    }
    throw runtime_error("Invalid operands for + operator: " + a.typeName() + " and " + b.typeName());
}

Value valSub(const Value& a, const Value& b) {
    if (a.type == Value::NUM && b.type == Value::NUM) {
        return Value(a.numVal - b.numVal, a.isFloat || b.isFloat);
    }
    throw runtime_error("Invalid operands for - operator");
}

Value valMul(const Value& a, const Value& b) {
    if (a.type == Value::NUM && b.type == Value::NUM) {
        return Value(a.numVal * b.numVal, a.isFloat || b.isFloat);
    }
    if (a.type == Value::STR && b.type == Value::NUM) {
        long long count = (long long)b.numVal;
        string res = "";
        for (long long i = 0; i < count; i++) res += a.strVal;
        return Value(res);
    }
    if (a.type == Value::NUM && b.type == Value::STR) {
        long long count = (long long)a.numVal;
        string res = "";
        for (long long i = 0; i < count; i++) res += b.strVal;
        return Value(res);
    }
    throw runtime_error("Invalid operands for * operator");
}

Value valDiv(const Value& a, const Value& b) {
    if (a.type == Value::NUM && b.type == Value::NUM) {
        if (b.numVal == 0.0) throw runtime_error("Bhai zero se divide karega? Math ro rahi hai kone mein!");
        double res = a.numVal / b.numVal;
        bool flt = a.isFloat || b.isFloat || (floor(res) != res);
        return Value(res, flt);
    }
    throw runtime_error("Invalid operands for / operator");
}

Value valMod(const Value& a, const Value& b) {
    if (a.type == Value::NUM && b.type == Value::NUM) {
        long long ib = (long long)b.numVal;
        if (ib == 0) throw runtime_error("Zero se modulo nahi hota bhai!");
        return Value((long long)a.numVal % ib);
    }
    throw runtime_error("Invalid operands for % operator");
}

bool valLess(const Value& a, const Value& b) {
    if (a.type == Value::NUM && b.type == Value::NUM) return a.numVal < b.numVal;
    if (a.type == Value::STR && b.type == Value::STR) return a.strVal < b.strVal;
    throw runtime_error("Invalid operands for < operator");
}

// ==============================================================================
// 3. ENCAPSULATION: ENVIRONMENT & SCOPING
// ==============================================================================
class Environment : public enable_shared_from_this<Environment> {
    shared_ptr<Environment> parent;
    map<string, Value> bindings;
public:
    Environment(shared_ptr<Environment> p = nullptr) : parent(p) {}
    void define(const string& name, Value val) { bindings[name] = val; }
    void assign(const string& name, Value val) {
        if (bindings.count(name)) { bindings[name] = val; return; }
        if (parent) { parent->assign(name, val); return; }
        bindings[name] = val;
    }
    Value get(const string& name) {
        if (bindings.count(name)) return bindings[name];
        return parent ? parent->get(name) : Value();
    }
};

// ==============================================================================
// 4. OOP ABSTRACTION: CALLABLES, CLASSES & INSTANCES
// ==============================================================================
class MemeCallable {
public:
    virtual ~MemeCallable() = default;
    virtual Value call(shared_ptr<Environment> env, const vector<Value>& args) = 0;
    virtual int arity() const = 0;
    virtual string toString() const = 0;
};

class MemeFunction : public MemeCallable {
public:
    string name;
    vector<string> params;
    shared_ptr<BlockStmt> body;
    shared_ptr<Environment> closure;

    MemeFunction(string n, vector<string> p, shared_ptr<BlockStmt> b, shared_ptr<Environment> clo)
        : name(std::move(n)), params(std::move(p)), body(b), closure(clo) {}

    int arity() const override { return (int)params.size(); }
    string toString() const override { return "<function " + name + ">"; }
    Value call(shared_ptr<Environment> callerEnv, const vector<Value>& args) override;
    shared_ptr<MemeFunction> bind(Value instance);
};

class MemeClass : public MemeCallable, public enable_shared_from_this<MemeClass> {
public:
    string name;
    shared_ptr<MemeClass> superclass;
    map<string, shared_ptr<MemeFunction>> methods;

    MemeClass(string n, shared_ptr<MemeClass> sup = nullptr)
        : name(std::move(n)), superclass(sup) {}

    shared_ptr<MemeFunction> findMethod(const string& mName) {
        if (methods.count(mName)) return methods[mName];
        return superclass ? superclass->findMethod(mName) : nullptr;
    }

    int arity() const override {
        auto init = const_cast<MemeClass*>(this)->findMethod("shuru");
        return init ? init->arity() : 0;
    }
    string toString() const override { return "<class " + name + ">"; }
    Value call(shared_ptr<Environment> env, const vector<Value>& args) override;
};

class MemeInstance : public enable_shared_from_this<MemeInstance> {
public:
    shared_ptr<MemeClass> klass;
    map<string, Value> fields;

    MemeInstance(shared_ptr<MemeClass> k) : klass(k) {}

    Value get(const string& prop) {
        if (fields.count(prop)) return fields[prop];
        if (auto method = klass->findMethod(prop)) {
            return Value(method->bind(Value(shared_from_this())));
        }
        return Value();
    }
    void set(const string& prop, Value val) { fields[prop] = val; }
};

Value MemeClass::call(shared_ptr<Environment> env, const vector<Value>& args) {
    auto inst = make_shared<MemeInstance>(shared_from_this());
    Value instVal(inst);
    if (auto init = findMethod("shuru")) {
        init->bind(instVal)->call(env, args);
    }
    return instVal;
}

shared_ptr<MemeFunction> MemeFunction::bind(Value instance) {
    auto boundEnv = make_shared<Environment>(closure);
    boundEnv->define("khud", instance);
    boundEnv->define("apun", instance); // meme alias for khud
    return make_shared<MemeFunction>(name, params, body, boundEnv);
}

class NativeFunction : public MemeCallable {
    string name;
    int paramCount;
    function<Value(const vector<Value>&)> fn;
public:
    NativeFunction(string n, int p, function<Value(const vector<Value>&)> f)
        : name(std::move(n)), paramCount(p), fn(std::move(f)) {}
    int arity() const override { return paramCount; }
    string toString() const override { return "<native " + name + ">"; }
    Value call(shared_ptr<Environment>, const vector<Value>& args) override { return fn(args); }
};

// ==============================================================================
// 5. AST ABSTRACTION & POLYMORPHISM (EXPRESSIONS & STATEMENTS)
// ==============================================================================
class Expr {
public:
    virtual ~Expr() = default;
    virtual Value eval(shared_ptr<Environment> env) = 0;
};

class Stmt {
public:
    virtual ~Stmt() = default;
    virtual void exec(shared_ptr<Environment> env) = 0;
};

class LiteralExpr : public Expr {
    Value val;
public:
    LiteralExpr(Value v) : val(v) {}
    Value eval(shared_ptr<Environment>) override { return val; }
};

class VarExpr : public Expr {
public:
    string name;
    VarExpr(string n) : name(std::move(n)) {}
    Value eval(shared_ptr<Environment> env) override { return env->get(name); }
};

class AssignExpr : public Expr {
    string name;
    shared_ptr<Expr> valExpr;
public:
    AssignExpr(string n, shared_ptr<Expr> v) : name(std::move(n)), valExpr(v) {}
    Value eval(shared_ptr<Environment> env) override {
        Value v = valExpr->eval(env);
        env->assign(name, v);
        return v;
    }
};

class UnaryExpr : public Expr {
    string op;
    shared_ptr<Expr> right;
public:
    UnaryExpr(string o, shared_ptr<Expr> r) : op(std::move(o)), right(r) {}
    Value eval(shared_ptr<Environment> env) override {
        Value r = right->eval(env);
        if (op == "-") {
            if (r.type == Value::NUM) return Value(-r.numVal, r.isFloat);
            throw runtime_error("Invalid operand for unary -");
        }
        if (op == "!" || op == "na") return Value(!r.isTruthy());
        return Value();
    }
};

class BinaryExpr : public Expr {
    shared_ptr<Expr> left;
    string op;
    shared_ptr<Expr> right;
public:
    BinaryExpr(shared_ptr<Expr> l, string o, shared_ptr<Expr> r)
        : left(l), op(std::move(o)), right(r) {}
    Value eval(shared_ptr<Environment> env) override {
        Value l = left->eval(env);
        Value r = right->eval(env);
        if (op == "+") return valAdd(l, r);
        if (op == "-") return valSub(l, r);
        if (op == "*") return valMul(l, r);
        if (op == "/") return valDiv(l, r);
        if (op == "%") return valMod(l, r);
        if (op == "==") return Value(valEquals(l, r));
        if (op == "!=") return Value(!valEquals(l, r));
        if (op == "<")  return Value(valLess(l, r));
        if (op == "<=") return Value(valLess(l, r) || valEquals(l, r));
        if (op == ">")  return Value(!valLess(l, r) && !valEquals(l, r));
        if (op == ">=") return Value(!valLess(l, r));
        return Value();
    }
};

class LogicalExpr : public Expr {
    shared_ptr<Expr> left;
    string op;
    shared_ptr<Expr> right;
public:
    LogicalExpr(shared_ptr<Expr> l, string o, shared_ptr<Expr> r)
        : left(l), op(std::move(o)), right(r) {}
    Value eval(shared_ptr<Environment> env) override {
        Value l = left->eval(env);
        if (op == "ya" || op == "||") {
            if (l.isTruthy()) return l;
        } else { // "aur" or "&&"
            if (!l.isTruthy()) return l;
        }
        return right->eval(env);
    }
};

class GetExpr : public Expr {
public:
    shared_ptr<Expr> object;
    string name;
    GetExpr(shared_ptr<Expr> o, string n) : object(o), name(std::move(n)) {}
    Value eval(shared_ptr<Environment> env) override {
        Value o = object->eval(env);
        if (o.type == Value::INSTANCE && o.inst) return o.inst->get(name);
        return Value();
    }
};

class SetExpr : public Expr {
    shared_ptr<Expr> object;
    string name;
    shared_ptr<Expr> valExpr;
public:
    SetExpr(shared_ptr<Expr> o, string n, shared_ptr<Expr> v)
        : object(o), name(std::move(n)), valExpr(v) {}
    Value eval(shared_ptr<Environment> env) override {
        Value v = valExpr->eval(env);
        Value o = object->eval(env);
        if (o.type == Value::INSTANCE && o.inst) o.inst->set(name, v);
        return v;
    }
};

class CallExpr : public Expr {
    shared_ptr<Expr> callee;
    vector<shared_ptr<Expr>> args;
public:
    CallExpr(shared_ptr<Expr> c, vector<shared_ptr<Expr>> a) : callee(c), args(std::move(a)) {}
    Value eval(shared_ptr<Environment> env) override {
        Value cVal = callee->eval(env);
        vector<Value> evaluatedArgs;
        for (auto& a : args) evaluatedArgs.push_back(a->eval(env));
        if (cVal.type == Value::CALLABLE && cVal.callable) {
            return cVal.callable->call(env, evaluatedArgs);
        }
        throw runtime_error("Can only call functions or classes! Found: " + cVal.typeName());
    }
};

class PoochExpr : public Expr {
    shared_ptr<Expr> prompt;
public:
    PoochExpr(shared_ptr<Expr> p = nullptr) : prompt(p) {}
    Value eval(shared_ptr<Environment> env) override {
        if (prompt) { prompt->eval(env).print(); cout << flush; }
        string line;
        if (!getline(cin, line)) return Value("");
        // Like Python's input(), captures entire line with all spaces preserved
        return Value(line);
    }
};

// Statements
class BlockStmt : public Stmt {
public:
    vector<shared_ptr<Stmt>> stmts;
    void exec(shared_ptr<Environment> env) override {
        for (auto& s : stmts) s->exec(env);
    }
};

Value MemeFunction::call(shared_ptr<Environment>, const vector<Value>& args) {
    auto env = make_shared<Environment>(closure);
    for (size_t i = 0; i < params.size(); i++) {
        env->define(params[i], (i < args.size()) ? args[i] : Value());
    }
    try {
        body->exec(env);
    } catch (const ReturnSignal& sig) {
        return *sig.value;
    }
    return Value();
}

class ExprStmt : public Stmt {
    shared_ptr<Expr> expr;
public:
    ExprStmt(shared_ptr<Expr> e) : expr(e) {}
    void exec(shared_ptr<Environment> env) override { expr->eval(env); }
};

class PrintStmt : public Stmt {
    vector<shared_ptr<Expr>> exprs;
public:
    PrintStmt(vector<shared_ptr<Expr>> e) : exprs(std::move(e)) {}
    void exec(shared_ptr<Environment> env) override {
        for (size_t i = 0; i < exprs.size(); i++) {
            exprs[i]->eval(env).print();
            if (i + 1 < exprs.size()) cout << " ";
        }
        cout << "\n";
    }
};

class VarStmt : public Stmt {
    string name;
    shared_ptr<Expr> init;
public:
    VarStmt(string n, shared_ptr<Expr> i) : name(std::move(n)), init(i) {}
    void exec(shared_ptr<Environment> env) override {
        env->define(name, init ? init->eval(env) : Value());
    }
};

class IfStmt : public Stmt {
    shared_ptr<Expr> cond;
    shared_ptr<Stmt> thenBranch;
    shared_ptr<Stmt> elseBranch;
public:
    IfStmt(shared_ptr<Expr> c, shared_ptr<Stmt> t, shared_ptr<Stmt> e = nullptr)
        : cond(c), thenBranch(t), elseBranch(e) {}
    void exec(shared_ptr<Environment> env) override {
        if (cond->eval(env).isTruthy()) {
            thenBranch->exec(env);
        } else if (elseBranch) {
            elseBranch->exec(env);
        }
    }
};

class WhileStmt : public Stmt {
    shared_ptr<Expr> cond;
    shared_ptr<Stmt> body;
public:
    WhileStmt(shared_ptr<Expr> c, shared_ptr<Stmt> b) : cond(c), body(b) {}
    void exec(shared_ptr<Environment> env) override {
        while (cond->eval(env).isTruthy()) {
            try {
                body->exec(env);
            } catch (const BreakSignal&) {
                break;
            } catch (const ContinueSignal&) {
                continue;
            }
        }
    }
};

class ForRangeStmt : public Stmt {
    string varName;
    shared_ptr<Expr> startExpr, endExpr, stepExpr;
    shared_ptr<Stmt> body;
public:
    ForRangeStmt(string v, shared_ptr<Expr> s, shared_ptr<Expr> e, shared_ptr<Expr> st, shared_ptr<Stmt> b)
        : varName(std::move(v)), startExpr(s), endExpr(e), stepExpr(st), body(b) {}
    void exec(shared_ptr<Environment> env) override {
        long long start = (long long)startExpr->eval(env).numVal;
        long long end = (long long)endExpr->eval(env).numVal;
        long long step = (start <= end) ? 1 : -1;
        if (stepExpr) step = (long long)stepExpr->eval(env).numVal;
        if (step == 0) throw runtime_error("Step cannot be zero in loop!");

        auto loopEnv = make_shared<Environment>(env);
        for (long long cur = start; (step > 0 ? cur <= end : cur >= end); cur += step) {
            loopEnv->define(varName, Value(cur));
            try {
                body->exec(loopEnv);
            } catch (const BreakSignal&) {
                break;
            } catch (const ContinueSignal&) {
                continue;
            }
        }
    }
};

class BreakStmt : public Stmt {
public:
    void exec(shared_ptr<Environment>) override { throw BreakSignal(); }
};

class ContinueStmt : public Stmt {
public:
    void exec(shared_ptr<Environment>) override { throw ContinueSignal(); }
};

class ReturnStmt : public Stmt {
    shared_ptr<Expr> expr;
public:
    ReturnStmt(shared_ptr<Expr> e) : expr(e) {}
    void exec(shared_ptr<Environment> env) override {
        throw ReturnSignal(expr ? expr->eval(env) : Value());
    }
};

class FuncDeclStmt : public Stmt {
public:
    string name;
    vector<string> params;
    shared_ptr<BlockStmt> body;
    FuncDeclStmt(string n, vector<string> p, shared_ptr<BlockStmt> b)
        : name(std::move(n)), params(std::move(p)), body(b) {}
    void exec(shared_ptr<Environment> env) override {
        auto fn = make_shared<MemeFunction>(name, params, body, env);
        env->define(name, Value(fn));
    }
};

class ClassStmt : public Stmt {
    string name, superName;
    vector<shared_ptr<FuncDeclStmt>> methods;
public:
    ClassStmt(string n, string s, vector<shared_ptr<FuncDeclStmt>> m)
        : name(std::move(n)), superName(std::move(s)), methods(std::move(m)) {}
    void exec(shared_ptr<Environment> env) override {
        shared_ptr<MemeClass> super = nullptr;
        if (!superName.empty()) {
            Value sVal = env->get(superName);
            if (sVal.type == Value::CALLABLE) super = dynamic_pointer_cast<MemeClass>(sVal.callable);
        }
        auto cls = make_shared<MemeClass>(name, super);
        for (auto& m : methods) {
            cls->methods[m->name] = make_shared<MemeFunction>(m->name, m->params, m->body, env);
        }
        env->define(name, Value(cls));
    }
};

// ==============================================================================
// 6. MEME LEXER (TOKENIZER)
// ==============================================================================
enum TokType {
    IDENT, STR, NUM,
    // Keywords & meme aliases
    KW_SANCHA, KW_SE, KW_KAR, KW_LE, KW_DIKHA, KW_KHUD,
    KW_DEKH, KW_WARNA, KW_JABTAK, KW_GHOOM, KW_BAS, KW_CHAL, KW_WAPAS,
    KW_AUR, KW_YA, KW_HAAN, KW_NAHI, KW_KUCH_NAHI, KW_PUCH,
    // Operators & symbols
    EQUAL_EQUAL, BANG_EQUAL, LESS_EQUAL, GREATER_EQUAL,
    LESS, GREATER, PLUS, MINUS, STAR, SLASH, PERCENT,
    BANG, EQUAL,
    LEFT_BRACE, RIGHT_BRACE, LEFT_PAREN, RIGHT_PAREN,
    COMMA, DOT, SEMICOLON, T_EOF
};

struct Token {
    TokType type;
    string text;
    double num = 0;
    bool isFloat = false;
};

vector<Token> tokenize(const string& src) {
    vector<Token> tokens;
    size_t i = 0, n = src.size();
    while (i < n) {
        if (isspace(src[i])) { i++; continue; }
        if (src[i] == '/' && i + 1 < n && src[i + 1] == '/') {
            while (i < n && src[i] != '\n') i++;
            continue;
        }
        if (src[i] == '"') {
            string s; i++;
            while (i < n && src[i] != '"') {
                if (src[i] == '\\' && i + 1 < n) {
                    i++;
                    if (src[i] == 'n') s += '\n';
                    else if (src[i] == 't') s += '\t';
                    else s += src[i];
                    i++;
                    continue;
                }
                s += src[i++];
            }
            if (i < n) i++;
            tokens.push_back({TokType::STR, s, 0, false});
            continue;
        }
        if (isdigit(src[i])) {
            size_t start = i;
            bool flt = false;
            while (i < n && (isdigit(src[i]) || src[i] == '.')) {
                if (src[i] == '.') flt = true;
                i++;
            }
            tokens.push_back({TokType::NUM, "", stod(src.substr(start, i - start)), flt});
            continue;
        }
        if (isalpha(src[i]) || src[i] == '_') {
            size_t start = i;
            while (i < n && (isalnum(src[i]) || src[i] == '_')) i++;
            string w = src.substr(start, i - start);
            if (w == "sancha" || w == "biradari") tokens.push_back({TokType::KW_SANCHA, w});
            else if (w == "se" || w == "nepo") tokens.push_back({TokType::KW_SE, w});
            else if (w == "kar" || w == "jugaad") tokens.push_back({TokType::KW_KAR, w});
            else if (w == "le" || w == "pakad") tokens.push_back({TokType::KW_LE, w});
            else if (w == "dikha" || w == "flex") tokens.push_back({TokType::KW_DIKHA, w});
            else if (w == "khud" || w == "apun") tokens.push_back({TokType::KW_KHUD, w});
            else if (w == "dekh") tokens.push_back({TokType::KW_DEKH, w});
            else if (w == "warna") tokens.push_back({TokType::KW_WARNA, w});
            else if (w == "jabtak") tokens.push_back({TokType::KW_JABTAK, w});
            else if (w == "ghoom" || w == "ghuma") tokens.push_back({TokType::KW_GHOOM, w});
            else if (w == "bas" || w == "bas_kar") tokens.push_back({TokType::KW_BAS, w});
            else if (w == "chal") tokens.push_back({TokType::KW_CHAL, w});
            else if (w == "wapas" || w == "ye_le") tokens.push_back({TokType::KW_WAPAS, w});
            else if (w == "aur") tokens.push_back({TokType::KW_AUR, w});
            else if (w == "ya") tokens.push_back({TokType::KW_YA, w});
            else if (w == "haan" || w == "han" || w == "real" || w == "true") tokens.push_back({TokType::KW_HAAN, w});
            else if (w == "nahi" || w == "nhi" || w == "cap" || w == "false") tokens.push_back({TokType::KW_NAHI, w});
            else if (w == "kuch_nahi" || w == "kuch_nhi" || w == "nalla" || w == "null") tokens.push_back({TokType::KW_KUCH_NAHI, w});
            else if (w == "puch" || w == "pooch") tokens.push_back({TokType::KW_PUCH, w});
            else tokens.push_back({TokType::IDENT, w});
            continue;
        }

        // Two-character operators
        if (i + 1 < n) {
            string op2 = src.substr(i, 2);
            if (op2 == "==") { tokens.push_back({TokType::EQUAL_EQUAL, op2}); i += 2; continue; }
            if (op2 == "!=") { tokens.push_back({TokType::BANG_EQUAL, op2}); i += 2; continue; }
            if (op2 == "<=") { tokens.push_back({TokType::LESS_EQUAL, op2}); i += 2; continue; }
            if (op2 == ">=") { tokens.push_back({TokType::GREATER_EQUAL, op2}); i += 2; continue; }
            if (op2 == "&&") { tokens.push_back({TokType::KW_AUR, op2}); i += 2; continue; }
            if (op2 == "||") { tokens.push_back({TokType::KW_YA, op2}); i += 2; continue; }
        }

        // Single character symbols
        char c = src[i++];
        switch (c) {
            case '{': tokens.push_back({TokType::LEFT_BRACE, "{"}); break;
            case '}': tokens.push_back({TokType::RIGHT_BRACE, "}"}); break;
            case '(': tokens.push_back({TokType::LEFT_PAREN, "("}); break;
            case ')': tokens.push_back({TokType::RIGHT_PAREN, ")"}); break;
            case ',': tokens.push_back({TokType::COMMA, ","}); break;
            case '.': tokens.push_back({TokType::DOT, "."}); break;
            case ';': tokens.push_back({TokType::SEMICOLON, ";"}); break;
            case '=': tokens.push_back({TokType::EQUAL, "="}); break;
            case '+': tokens.push_back({TokType::PLUS, "+"}); break;
            case '-': tokens.push_back({TokType::MINUS, "-"}); break;
            case '*': tokens.push_back({TokType::STAR, "*"}); break;
            case '/': tokens.push_back({TokType::SLASH, "/"}); break;
            case '%': tokens.push_back({TokType::PERCENT, "%"}); break;
            case '!': tokens.push_back({TokType::BANG, "!"}); break;
            case '<': tokens.push_back({TokType::LESS, "<"}); break;
            case '>': tokens.push_back({TokType::GREATER, ">"}); break;
            default: break;
        }
    }
    tokens.push_back({TokType::T_EOF, ""});
    return tokens;
}

// ==============================================================================
// 7. MEME PARSER (PRECEDENCE CLIMBING & RECURSIVE DESCENT)
// ==============================================================================
class Parser {
    vector<Token> tokens;
    size_t cur = 0;

    Token peek() { return tokens[cur]; }
    Token previous() { return tokens[cur - 1]; }
    Token advance() { return tokens[cur++]; }
    bool check(TokType t) { return cur < tokens.size() && tokens[cur].type == t; }
    bool match(TokType t) { if (check(t)) { advance(); return true; } return false; }
    Token consume(TokType t, const string& err) {
        if (check(t)) return advance();
        throw runtime_error("Parse Error: " + err + " (found: '" + peek().text + "')");
    }

    // Expressions
    shared_ptr<Expr> primary() {
        if (match(TokType::NUM)) return make_shared<LiteralExpr>(Value(previous().num, previous().isFloat));
        if (match(TokType::STR)) return make_shared<LiteralExpr>(Value(previous().text));
        if (match(TokType::KW_HAAN)) return make_shared<LiteralExpr>(Value(true));
        if (match(TokType::KW_NAHI)) return make_shared<LiteralExpr>(Value(false));
        if (match(TokType::KW_KUCH_NAHI)) return make_shared<LiteralExpr>(Value());
        if (match(TokType::KW_KHUD)) return make_shared<VarExpr>("khud");
        if (match(TokType::IDENT)) return make_shared<VarExpr>(previous().text);
        if (match(TokType::KW_PUCH)) {
            shared_ptr<Expr> prompt = nullptr;
            if (match(TokType::LEFT_PAREN)) {
                if (!check(TokType::RIGHT_PAREN)) prompt = expression();
                consume(TokType::RIGHT_PAREN, "Expected ')' after pooch");
            } else if (check(TokType::STR)) {
                prompt = primary();
            }
            return make_shared<PoochExpr>(prompt);
        }
        if (match(TokType::LEFT_PAREN)) {
            auto expr = expression();
            consume(TokType::RIGHT_PAREN, "Expected ')' after expression");
            return expr;
        }
        throw runtime_error("Unexpected token in expression: '" + peek().text + "'");
    }

    shared_ptr<Expr> postfix() {
        auto expr = primary();
        while (true) {
            if (match(TokType::DOT)) {
                string name = consume(TokType::IDENT, "Expected property name after '.'").text;
                expr = make_shared<GetExpr>(expr, name);
            } else if (match(TokType::LEFT_PAREN)) {
                vector<shared_ptr<Expr>> args;
                if (!check(TokType::RIGHT_PAREN)) {
                    do { args.push_back(expression()); } while (match(TokType::COMMA));
                }
                consume(TokType::RIGHT_PAREN, "Expected ')' after arguments");
                expr = make_shared<CallExpr>(expr, args);
            } else {
                break;
            }
        }
        return expr;
    }

    shared_ptr<Expr> unary() {
        if (match(TokType::BANG) || match(TokType::MINUS)) {
            string op = previous().text;
            return make_shared<UnaryExpr>(op, unary());
        }
        return postfix();
    }

    shared_ptr<Expr> factor() {
        auto expr = unary();
        while (match(TokType::STAR) || match(TokType::SLASH) || match(TokType::PERCENT)) {
            string op = previous().text;
            auto right = unary();
            expr = make_shared<BinaryExpr>(expr, op, right);
        }
        return expr;
    }

    shared_ptr<Expr> term() {
        auto expr = factor();
        while (match(TokType::PLUS) || match(TokType::MINUS)) {
            string op = previous().text;
            auto right = factor();
            expr = make_shared<BinaryExpr>(expr, op, right);
        }
        return expr;
    }

    shared_ptr<Expr> comparison() {
        auto expr = term();
        while (match(TokType::LESS) || match(TokType::LESS_EQUAL) ||
               match(TokType::GREATER) || match(TokType::GREATER_EQUAL)) {
            string op = previous().text;
            auto right = term();
            expr = make_shared<BinaryExpr>(expr, op, right);
        }
        return expr;
    }

    shared_ptr<Expr> equality() {
        auto expr = comparison();
        while (match(TokType::EQUAL_EQUAL) || match(TokType::BANG_EQUAL)) {
            string op = previous().text;
            auto right = comparison();
            expr = make_shared<BinaryExpr>(expr, op, right);
        }
        return expr;
    }

    shared_ptr<Expr> logicalAnd() {
        auto expr = equality();
        while (match(TokType::KW_AUR)) {
            string op = previous().text;
            auto right = equality();
            expr = make_shared<LogicalExpr>(expr, op, right);
        }
        return expr;
    }

    shared_ptr<Expr> logicalOr() {
        auto expr = logicalAnd();
        while (match(TokType::KW_YA)) {
            string op = previous().text;
            auto right = logicalAnd();
            expr = make_shared<LogicalExpr>(expr, op, right);
        }
        return expr;
    }

    shared_ptr<Expr> assignment() {
        auto expr = logicalOr();
        if (match(TokType::EQUAL)) {
            auto val = assignment();
            if (auto v = dynamic_pointer_cast<VarExpr>(expr)) return make_shared<AssignExpr>(v->name, val);
            if (auto g = dynamic_pointer_cast<GetExpr>(expr)) return make_shared<SetExpr>(g->object, g->name, val);
            throw runtime_error("Invalid assignment target");
        }
        return expr;
    }

public:
    Parser(vector<Token> t) : tokens(std::move(t)) {}

    shared_ptr<Expr> expression() { return assignment(); }

    shared_ptr<BlockStmt> block() {
        auto b = make_shared<BlockStmt>();
        consume(TokType::LEFT_BRACE, "Expected '{' before block");
        while (!check(TokType::RIGHT_BRACE) && !check(TokType::T_EOF)) {
            b->stmts.push_back(statement());
        }
        consume(TokType::RIGHT_BRACE, "Expected '}' after block");
        return b;
    }

    shared_ptr<Stmt> statement() {
        if (match(TokType::KW_DIKHA)) {
            vector<shared_ptr<Expr>> exprs;
            if (!check(TokType::SEMICOLON)) {
                do { exprs.push_back(expression()); } while (match(TokType::COMMA));
            }
            consume(TokType::SEMICOLON, "Expected ';' after dikha");
            return make_shared<PrintStmt>(exprs);
        }
        if (match(TokType::KW_LE)) {
            string name = consume(TokType::IDENT, "Expected variable name after 'le'").text;
            shared_ptr<Expr> init = match(TokType::EQUAL) ? expression() : nullptr;
            consume(TokType::SEMICOLON, "Expected ';' after variable declaration");
            return make_shared<VarStmt>(name, init);
        }
        if (match(TokType::KW_DEKH)) {
            auto cond = expression();
            auto thenBranch = block();
            shared_ptr<Stmt> elseBranch = nullptr;
            if (match(TokType::KW_WARNA)) {
                if (check(TokType::KW_DEKH)) {
                    advance();
                    auto nextCond = expression();
                    auto nextThen = block();
                    shared_ptr<Stmt> nextElse = nullptr;
                    if (match(TokType::KW_WARNA)) {
                        nextElse = check(TokType::KW_DEKH) ? statement() : block();
                    }
                    elseBranch = make_shared<IfStmt>(nextCond, nextThen, nextElse);
                } else {
                    elseBranch = block();
                }
            }
            return make_shared<IfStmt>(cond, thenBranch, elseBranch);
        }
        if (match(TokType::KW_JABTAK)) {
            auto cond = expression();
            auto body = block();
            return make_shared<WhileStmt>(cond, body);
        }
        if (match(TokType::KW_GHOOM)) {
            string varName = consume(TokType::IDENT, "Expected loop variable name after 'ghoom'").text;
            consume(TokType::KW_SE, "Expected 'se' before start range");
            auto start = expression();
            consume(TokType::KW_SE, "Expected 'se' before end range");
            auto end = expression();
            shared_ptr<Expr> step = nullptr;
            if (match(TokType::KW_SE)) step = expression();
            auto body = block();
            return make_shared<ForRangeStmt>(varName, start, end, step, body);
        }
        if (match(TokType::KW_BAS)) {
            consume(TokType::SEMICOLON, "Expected ';' after bas");
            return make_shared<BreakStmt>();
        }
        if (match(TokType::KW_CHAL)) {
            consume(TokType::SEMICOLON, "Expected ';' after chal");
            return make_shared<ContinueStmt>();
        }
        if (match(TokType::KW_WAPAS)) {
            shared_ptr<Expr> val = nullptr;
            if (!check(TokType::SEMICOLON)) val = expression();
            consume(TokType::SEMICOLON, "Expected ';' after return");
            return make_shared<ReturnStmt>(val);
        }
        if (match(TokType::KW_KAR)) {
            string name = consume(TokType::IDENT, "Expected function name after 'kar'").text;
            consume(TokType::LEFT_PAREN, "Expected '(' after function name");
            vector<string> params;
            if (!check(TokType::RIGHT_PAREN)) {
                do { params.push_back(consume(TokType::IDENT, "Expected parameter name").text); } while (match(TokType::COMMA));
            }
            consume(TokType::RIGHT_PAREN, "Expected ')' after parameters");
            auto body = block();
            return make_shared<FuncDeclStmt>(name, params, body);
        }
        if (match(TokType::KW_SANCHA)) {
            string name = consume(TokType::IDENT, "Expected class name after 'sancha'").text;
            string super = match(TokType::KW_SE) ? consume(TokType::IDENT, "Expected superclass name after 'se'").text : "";
            consume(TokType::LEFT_BRACE, "Expected '{' before class body");
            vector<shared_ptr<FuncDeclStmt>> methods;
            while (!check(TokType::RIGHT_BRACE) && !check(TokType::T_EOF)) {
                consume(TokType::KW_KAR, "Expected 'kar' before method declaration");
                string mName = consume(TokType::IDENT, "Expected method name").text;
                consume(TokType::LEFT_PAREN, "Expected '(' after method name");
                vector<string> params;
                if (!check(TokType::RIGHT_PAREN)) {
                    do { params.push_back(consume(TokType::IDENT, "Expected parameter name").text); } while (match(TokType::COMMA));
                }
                consume(TokType::RIGHT_PAREN, "Expected ')' after parameters");
                auto body = block();
                methods.push_back(make_shared<FuncDeclStmt>(mName, params, body));
            }
            consume(TokType::RIGHT_BRACE, "Expected '}' after class body");
            return make_shared<ClassStmt>(name, super, methods);
        }
        if (check(TokType::LEFT_BRACE)) return block();

        auto e = expression();
        consume(TokType::SEMICOLON, "Expected ';' after expression");
        return make_shared<ExprStmt>(e);
    }

    vector<shared_ptr<Stmt>> parse() {
        vector<shared_ptr<Stmt>> stmts;
        while (!check(TokType::T_EOF)) stmts.push_back(statement());
        return stmts;
    }
};

// ==============================================================================
// 8. BUILT-INS INITIALIZER & ENVIRONMENT SETUP
// ==============================================================================
shared_ptr<Environment> createGlobalEnvironment() {
    auto env = make_shared<Environment>();
    // kya_hai: returns the type of the value
    env->define("kya_hai", Value(make_shared<NativeFunction>("kya_hai", 1, [](const vector<Value>& args) {
        return Value(args.empty() ? "nalla" : args[0].typeName());
    })));

    // point_wala: converts string or int to float
    env->define("point_wala", Value(make_shared<NativeFunction>("point_wala", 1, [](const vector<Value>& args) {
        if (args.empty()) return Value(0.0, true);
        if (args[0].type == Value::NUM) return Value(args[0].numVal, true);
        if (args[0].type == Value::STR) {
            try { return Value(stod(args[0].strVal), true); } catch (...) {}
        }
        return Value(0.0, true);
    })));

    // gin: string length
    env->define("gin", Value(make_shared<NativeFunction>("gin", 1, [](const vector<Value>& args) {
        if (args.empty() || args[0].type != Value::STR) return Value(0);
        return Value((long long)args[0].strVal.size());
    })));

    // pooch / puch: input like Python (captures whole line with spaces)
    auto poochFn = Value(make_shared<NativeFunction>("pooch", 0, [](const vector<Value>& args) {
        if (!args.empty()) { args[0].print(); cout << flush; }
        string line;
        if (!getline(cin, line)) return Value("");
        return Value(line);
    }));
    env->define("pooch", poochFn);
    env->define("puch", poochFn);

    return env;
}

// ==============================================================================
// 9. MAIN PROGRAM (DEMO & FILE RUNNER)
// ==============================================================================
int main(int argc, char* argv[]) {
    string code;
    if (argc > 2 && string(argv[1]) == "-e") {
        code = argv[2];
    } else if (argc > 1) {
        ifstream file(argv[1]);
        if (!file.is_open()) { cerr << "Error: Cannot open file " << argv[1] << "\n"; return 1; }
        stringstream ss; ss << file.rdbuf(); code = ss.str();
    } else {
        // Built-in Complete Parody OOP Showcase
        code = R"(
            dikha "=== MEMElang Complete Feature Showcase ===";

            // 1. Base Class with Constructor & Method
            sancha Banda {
                kar shuru(naam, umar) {
                    khud.naam = naam;
                    khud.umar = umar;
                }
                kar parichay() {
                    dikha "Namaste! Mera naam", khud.naam, "hai aur umar", khud.umar, "saal hai.";
                }
                kar kaam() {
                    dikha khud.naam, "seedha sadha kaam karta hai.";
                }
            }

            // 2. Subclass with Inheritance & Method Overriding
            sancha Don se Banda {
                kar shuru(naam, umar, ilaka) {
                    khud.naam = naam;
                    khud.umar = umar;
                    khud.ilaka = ilaka;
                }
                kar kaam() {
                    dikha "Khatarnak kaam! Mai", khud.ilaka, "ka Don hoon!";
                }
                kar dhamki() {
                    dikha "Utha le re baba!", khud.naam, "se panga mat lena!";
                }
            }

            le baburao = Don("Baburao Apte", 50, "Khadakwasla");
            baburao.parichay();
            baburao.kaam();
            baburao.dhamki();
            baburao.dialogue = "Khopdi tod saale ka!";
            dikha "Dialogue:", baburao.dialogue;

            // 3. Functions, Recursion, Loops & Break/Continue
            dikha "\n--- Loop & Fibonacci Demonstration ---";
            kar fib(n) {
                dekh n <= 0 { wapas 0; }
                dekh n == 1 { wapas 1; }
                wapas fib(n - 1) + fib(n - 2);
            }

            ghoom i se 0 se 5 {
                dikha "fib(", i, ") =", fib(i);
            }

            dikha "\n--- While Loop (jabtak) with Break (bas) ---";
            le count = 1;
            jabtak count <= 10 {
                dekh count == 4 {
                    dikha "Count reached 4, breaking out!";
                    bas;
                }
                dikha "Step:", count;
                count = count + 1;
            }
        )";
    }

    try {
        auto tokens = tokenize(code);
        Parser parser(tokens);
        auto program = parser.parse();
        auto globalScope = createGlobalEnvironment();
        for (auto& stmt : program) stmt->exec(globalScope);
    } catch (const exception& e) {
        cerr << "Runtime Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
