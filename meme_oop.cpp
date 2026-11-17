/**
 * ==============================================================================
 * PROJECT: MEMElang (AN INTERPRETED OBJECT-ORIENTED PROGRAMMING LANGUAGE)
 * MADE BY: ANSH SHARMA & SARTHAK JAIN
 * SUBJECT: OBJECT ORIENTED PROGRAMMING (OOP) IN C++
==============================================================================
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
// 1. CONTROL FLOW SIGNALS
// ==============================================================================
class ReturnSignal : public std::exception {
public:
    shared_ptr<Value> value;
    ReturnSignal(Value v);
    const char* what() const noexcept override { return "Arey kehna kya chahte ho? 'return' function ke bahar kahan le ja rahe ho?"; }
};
class BreakSignal : public std::exception {
public:
    const char* what() const noexcept override { return "Arey arey arey! 'break' loop ke bahar kya kar raha hai?"; }
};
class ContinueSignal : public std::exception {
public:
    const char* what() const noexcept override { return "Control Majnu! 'continue' loop ke bahar nahi chal sakta!"; }
};

// ==============================================================================
// 2. RUNTIME VALUES & OPERATOR SEMANTICS (OFFICIAL MEME ERRORS)
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
            case NIL: return "null";
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
            case NIL: return "null";
            case BOOL: return bVal ? "true" : "false";
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
        if (!a.isFloat && !b.isFloat) {
            long long ia = (long long)a.numVal, ib = (long long)b.numVal, res = 0;
            if (__builtin_add_overflow(ia, ib, &res)) {
                throw runtime_error("25 din mein paisa double karte-karte integer overflow ho gaya! Aukaat ke bahar chala gaya jod.");
            }
            return Value(res);
        }
        return Value(a.numVal + b.numVal, true);
    }
    if (a.type == Value::STR || b.type == Value::STR) {
        return Value(a.toString() + b.toString());
    }
    throw runtime_error("Seb ko santre se mat tolo! '" + a.typeName() + "' aur '" + b.typeName() + "' ko add (+) nahi kar sakte!");
}

Value valSub(const Value& a, const Value& b) {
    if (a.type == Value::NUM && b.type == Value::NUM) {
        return Value(a.numVal - b.numVal, a.isFloat || b.isFloat);
    }
    throw runtime_error("Seb ko santre se mat tolo! '" + a.typeName() + "' aur '" + b.typeName() + "' ke beech subtraction (-) nahi ho sakta!");
}

Value valMul(const Value& a, const Value& b) {
    if (a.type == Value::NUM && b.type == Value::NUM) {
        return Value(a.numVal * b.numVal, a.isFloat || b.isFloat);
    }
    if (a.type == Value::STR && b.type == Value::NUM) {
        long long count = (long long)b.numVal;
        if (count < 0) throw runtime_error("Delulu is not the solulu! String ko negative baar kaise multiply karoge?");
        string res = "";
        for (long long i = 0; i < count; i++) res += a.strVal;
        return Value(res);
    }
    if (a.type == Value::NUM && b.type == Value::STR) {
        long long count = (long long)a.numVal;
        if (count < 0) throw runtime_error("Delulu is not the solulu! String ko negative baar kaise multiply karoge?");
        string res = "";
        for (long long i = 0; i < count; i++) res += b.strVal;
        return Value(res);
    }
    throw runtime_error("Ye sab doglapan hai! In types ke beech multiplication (*) nahi ho sakta!");
}

Value valDiv(const Value& a, const Value& b) {
    if (a.type == Value::NUM && b.type == Value::NUM) {
        if (b.numVal == 0.0) throw runtime_error("Kya coder banega re tu? 0 se divide karke brahmand khatam karega kya!");
        double res = a.numVal / b.numVal;
        bool flt = a.isFloat || b.isFloat || (floor(res) != res);
        return Value(res, flt);
    }
    throw runtime_error("Seb ko santre se mat tolo! '" + a.typeName() + "' ko '" + b.typeName() + "' se divide (/) nahi kar sakte!");
}

Value valMod(const Value& a, const Value& b) {
    if (a.type == Value::NUM && b.type == Value::NUM) {
        long long ib = (long long)b.numVal;
        if (ib == 0) throw runtime_error("0 se modulo (%) karne chale ho? Maths teacher ko heart attack aa jayega!");
        return Value((long long)a.numVal % ib);
    }
    throw runtime_error("Seb ko santre se mat tolo! Modulo (%) sirf numbers ke beech hota hai, '" + a.typeName() + "' aur '" + b.typeName() + "' pe nahi!");
}

bool valLess(const Value& a, const Value& b) {
    if (a.type == Value::NUM && b.type == Value::NUM) return a.numVal < b.numVal;
    if (a.type == Value::STR && b.type == Value::STR) return a.strVal < b.strVal;
    throw runtime_error("Seb ko santre se mat tolo! '" + a.typeName() + "' aur '" + b.typeName() + "' ko compare nahi kar sakte!");
}

// ==============================================================================
// 3. ENCAPSULATION: SCOPING & ENVIRONMENT
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
        throw runtime_error("Chuna laga diya! '" + name + "' naam ka koi variable hi nahi hai. Pehle 'let " + name + ";' likh!");
    }
    Value get(const string& name) {
        if (bindings.count(name)) return bindings[name];
        if (parent) return parent->get(name);
        throw runtime_error("Rasode mein kaun tha?! Ye '" + name + "' bina bataye kahan se aaya? Pehle 'let " + name + ";' se declare kar!");
    }
};

// ==============================================================================
// 4. OOP CORE: CALLABLES, CLASSES & INSTANCES
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

    MemeClass(string n, shared_ptr<MemeClass> sup = nullptr) : name(std::move(n)), superclass(sup) {}

    shared_ptr<MemeFunction> findMethod(const string& mName) {
        if (methods.count(mName)) return methods[mName];
        return superclass ? superclass->findMethod(mName) : nullptr;
    }

    shared_ptr<MemeFunction> findConstructor() {
        if (methods.count("constructor")) return methods["constructor"];
        if (methods.count("init")) return methods["init"];
        return superclass ? superclass->findConstructor() : nullptr;
    }

    int arity() const override {
        auto init = const_cast<MemeClass*>(this)->findConstructor();
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
        throw runtime_error("Khopdi tod saale ka! '" + (klass ? klass->name : "object") + "' ke paas '" + prop + "' naam ka kuch nahi hai!");
    }
    void set(const string& prop, Value val) { fields[prop] = val; }
};

Value MemeClass::call(shared_ptr<Environment> env, const vector<Value>& args) {
    auto inst = make_shared<MemeInstance>(shared_from_this());
    Value instVal(inst);
    if (auto init = findConstructor()) {
        init->bind(instVal)->call(env, args);
    }
    return instVal;
}

shared_ptr<MemeFunction> MemeFunction::bind(Value instance) {
    auto boundEnv = make_shared<Environment>(closure);
    boundEnv->define("this", instance);
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

class GroupExpr : public Expr {
public:
    shared_ptr<Expr> inner;
    GroupExpr(shared_ptr<Expr> i) : inner(i) {}
    Value eval(shared_ptr<Environment> env) override { return inner->eval(env); }
};

class VarExpr : public Expr {
public:
    string name;
    VarExpr(string n) : name(std::move(n)) {}
    Value eval(shared_ptr<Environment> env) override { return env->get(name); }
};

class AssignExpr : public Expr {
public:
    string name;
    shared_ptr<Expr> valExpr;
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
            throw runtime_error("'" + r.typeName() + "' ke aage minus (-) kaise laga diya!");
        }
        if (op == "!" || op == "not") return Value(!r.isTruthy());
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
        Value l = left->eval(env), r = right->eval(env);
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
        if (op == "||" || op == "or") { if (l.isTruthy()) return l; }
        else { if (!l.isTruthy()) return l; }
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
        throw runtime_error("Khopdi tod saale ka! Null ya non-object pe property '" + name + "' dhundh rahe ho!");
    }
};

class SetExpr : public Expr {
public:
    shared_ptr<Expr> object;
    string name;
    shared_ptr<Expr> valExpr;
    SetExpr(shared_ptr<Expr> o, string n, shared_ptr<Expr> v)
        : object(o), name(std::move(n)), valExpr(v) {}
    Value eval(shared_ptr<Environment> env) override {
        Value v = valExpr->eval(env), o = object->eval(env);
        if (o.type == Value::INSTANCE && o.inst) { o.inst->set(name, v); return v; }
        throw runtime_error("Ye sab doglapan hai! Non-object pe property assign nahi hoti!");
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
        throw runtime_error("Beta, tumse na ho payega! '" + cVal.toString() + "' koi function thodi hai jo call kar rahe ho!");
    }
};

class InputExpr : public Expr {
    shared_ptr<Expr> prompt;
public:
    InputExpr(shared_ptr<Expr> p = nullptr) : prompt(p) {}
    Value eval(shared_ptr<Environment> env) override {
        if (prompt) { prompt->eval(env).print(); cout << flush; }
        string line;
        if (!getline(cin, line)) return Value("");
        return Value(line);
    }
};

class BlockStmt : public Stmt {
public:
    vector<shared_ptr<Stmt>> stmts;
    void exec(shared_ptr<Environment> env) override { for (auto& s : stmts) s->exec(env); }
};

Value MemeFunction::call(shared_ptr<Environment>, const vector<Value>& args) {
    if (args.size() != params.size()) {
        throw runtime_error("Gaddari korbe! Function ne " + to_string(params.size()) + " arguments maange the, aur tumne " + to_string(args.size()) + " chipka diye!");
    }
    auto env = make_shared<Environment>(closure);
    for (size_t i = 0; i < params.size(); i++) env->define(params[i], args[i]);
    try { body->exec(env); } catch (const ReturnSignal& sig) { return *sig.value; }
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
    void exec(shared_ptr<Environment> env) override { env->define(name, init ? init->eval(env) : Value()); }
};

class IfStmt : public Stmt {
    shared_ptr<Expr> cond;
    shared_ptr<Stmt> thenBranch, elseBranch;
public:
    IfStmt(shared_ptr<Expr> c, shared_ptr<Stmt> t, shared_ptr<Stmt> e = nullptr)
        : cond(c), thenBranch(t), elseBranch(e) {}
    void exec(shared_ptr<Environment> env) override {
        if (cond->eval(env).isTruthy()) thenBranch->exec(env);
        else if (elseBranch) elseBranch->exec(env);
    }
};

class WhileStmt : public Stmt {
    shared_ptr<Expr> cond;
    shared_ptr<Stmt> body;
public:
    WhileStmt(shared_ptr<Expr> c, shared_ptr<Stmt> b) : cond(c), body(b) {}
    void exec(shared_ptr<Environment> env) override {
        while (cond->eval(env).isTruthy()) {
            try { body->exec(env); }
            catch (const BreakSignal&) { break; }
            catch (const ContinueSignal&) { continue; }
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
        long long start = (long long)startExpr->eval(env).numVal, end = (long long)endExpr->eval(env).numVal;
        long long step = (start <= end) ? 1 : -1;
        if (stepExpr) step = (long long)stepExpr->eval(env).numVal;
        if (step == 0) throw runtime_error("Step 0 rakhoge toh loop qayamat tak chalega! Infinite loop se bacho dost!");
        auto loopEnv = make_shared<Environment>(env);
        for (long long cur = start; (step > 0 ? cur <= end : cur >= end); cur += step) {
            loopEnv->define(varName, Value(cur));
            try { body->exec(loopEnv); }
            catch (const BreakSignal&) { break; }
            catch (const ContinueSignal&) { continue; }
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
    void exec(shared_ptr<Environment> env) override { throw ReturnSignal(expr ? expr->eval(env) : Value()); }
};

class FuncDeclStmt : public Stmt {
public:
    string name;
    vector<string> params;
    shared_ptr<BlockStmt> body;
    FuncDeclStmt(string n, vector<string> p, shared_ptr<BlockStmt> b)
        : name(std::move(n)), params(std::move(p)), body(b) {}
    void exec(shared_ptr<Environment> env) override {
        env->define(name, Value(make_shared<MemeFunction>(name, params, body, env)));
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
            if (!super) throw runtime_error("Fake nepotism! '" + superName + "' koi class nahi hai, isko inherit kaise karoge?");
        }
        auto cls = make_shared<MemeClass>(name, super);
        for (auto& m : methods) cls->methods[m->name] = make_shared<MemeFunction>(m->name, m->params, m->body, env);
        env->define(name, Value(cls));
    }
};

// ==============================================================================
// 6. TOKENIZER (CLEAN ENGLISH KEYWORDS, NO ALIASES, OFFICIAL MEME ERRORS)
// ==============================================================================
enum TokType {
    IDENT, STR, NUM,
    KW_CLASS, KW_EXTENDS, KW_FUNCTION, KW_LET, KW_THIS,
    KW_IF, KW_ELSE, KW_WHILE, KW_FOR, KW_IN, KW_TO, KW_STEP,
    KW_RETURN, KW_BREAK, KW_CONTINUE, KW_PRINT, KW_INPUT,
    KW_TRUE, KW_FALSE, KW_NULL, KW_AND, KW_OR, KW_NOT,
    EQUAL_EQUAL, BANG_EQUAL, LESS_EQUAL, GREATER_EQUAL,
    LESS, GREATER, PLUS, MINUS, STAR, SLASH, PERCENT,
    BANG, EQUAL, LEFT_BRACE, RIGHT_BRACE, LEFT_PAREN, RIGHT_PAREN,
    COMMA, DOT, SEMICOLON, T_EOF
};

struct Token {
    TokType type;
    string text;
    double num = 0;
    bool isFloat = false;
    int line = 1;
};

vector<Token> tokenize(const string& src) {
    vector<Token> tokens;
    size_t i = 0, n = src.size();
    int currentLine = 1;

    while (i < n) {
        if (src[i] == '\n') { currentLine++; i++; continue; }
        if (isspace(src[i])) { i++; continue; }
        if (src[i] == '/' && i + 1 < n && src[i + 1] == '/') {
            while (i < n && src[i] != '\n') i++;
            continue;
        }
        if (src[i] == '"' || src[i] == '\'') {
            char quote = src[i++];
            string s;
            bool closed = false;
            while (i < n) {
                if (src[i] == '\n') currentLine++;
                if (src[i] == quote) { closed = true; i++; break; }
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
            if (!closed) throw runtime_error("😒 String khol ke chhod diya... band tera baap karega?");
            tokens.push_back({TokType::STR, s, 0, false, currentLine});
            continue;
        }
        if (isdigit(src[i])) {
            size_t start = i;
            bool flt = false;
            while (i < n && (isdigit(src[i]) || src[i] == '.')) {
                if (src[i] == '.') flt = true;
                i++;
            }
            tokens.push_back({TokType::NUM, "", stod(src.substr(start, i - start)), flt, currentLine});
            continue;
        }
        if (isalpha(src[i]) || src[i] == '_') {
            size_t start = i;
            while (i < n && (isalnum(src[i]) || src[i] == '_')) i++;
            string w = src.substr(start, i - start);
            if (w == "class") tokens.push_back({TokType::KW_CLASS, w, 0, false, currentLine});
            else if (w == "extends") tokens.push_back({TokType::KW_EXTENDS, w, 0, false, currentLine});
            else if (w == "function") tokens.push_back({TokType::KW_FUNCTION, w, 0, false, currentLine});
            else if (w == "let") tokens.push_back({TokType::KW_LET, w, 0, false, currentLine});
            else if (w == "this") tokens.push_back({TokType::KW_THIS, w, 0, false, currentLine});
            else if (w == "if") tokens.push_back({TokType::KW_IF, w, 0, false, currentLine});
            else if (w == "else") tokens.push_back({TokType::KW_ELSE, w, 0, false, currentLine});
            else if (w == "while") tokens.push_back({TokType::KW_WHILE, w, 0, false, currentLine});
            else if (w == "for") tokens.push_back({TokType::KW_FOR, w, 0, false, currentLine});
            else if (w == "in") tokens.push_back({TokType::KW_IN, w, 0, false, currentLine});
            else if (w == "to") tokens.push_back({TokType::KW_TO, w, 0, false, currentLine});
            else if (w == "step") tokens.push_back({TokType::KW_STEP, w, 0, false, currentLine});
            else if (w == "return") tokens.push_back({TokType::KW_RETURN, w, 0, false, currentLine});
            else if (w == "break") tokens.push_back({TokType::KW_BREAK, w, 0, false, currentLine});
            else if (w == "continue") tokens.push_back({TokType::KW_CONTINUE, w, 0, false, currentLine});
            else if (w == "print") tokens.push_back({TokType::KW_PRINT, w, 0, false, currentLine});
            else if (w == "input") tokens.push_back({TokType::KW_INPUT, w, 0, false, currentLine});
            else if (w == "true") tokens.push_back({TokType::KW_TRUE, w, 0, false, currentLine});
            else if (w == "false") tokens.push_back({TokType::KW_FALSE, w, 0, false, currentLine});
            else if (w == "null") tokens.push_back({TokType::KW_NULL, w, 0, false, currentLine});
            else if (w == "and") tokens.push_back({TokType::KW_AND, w, 0, false, currentLine});
            else if (w == "or") tokens.push_back({TokType::KW_OR, w, 0, false, currentLine});
            else if (w == "not") tokens.push_back({TokType::KW_NOT, w, 0, false, currentLine});
            else tokens.push_back({TokType::IDENT, w, 0, false, currentLine});
            continue;
        }

        if (i + 1 < n) {
            string op2 = src.substr(i, 2);
            if (op2 == "==") { tokens.push_back({TokType::EQUAL_EQUAL, op2, 0, false, currentLine}); i += 2; continue; }
            if (op2 == "!=") { tokens.push_back({TokType::BANG_EQUAL, op2, 0, false, currentLine}); i += 2; continue; }
            if (op2 == "<=") { tokens.push_back({TokType::LESS_EQUAL, op2, 0, false, currentLine}); i += 2; continue; }
            if (op2 == ">=") { tokens.push_back({TokType::GREATER_EQUAL, op2, 0, false, currentLine}); i += 2; continue; }
            if (op2 == "&&") { tokens.push_back({TokType::KW_AND, op2, 0, false, currentLine}); i += 2; continue; }
            if (op2 == "||") { tokens.push_back({TokType::KW_OR, op2, 0, false, currentLine}); i += 2; continue; }
        }

        char c = src[i++];
        switch (c) {
            case '{': tokens.push_back({TokType::LEFT_BRACE, "{", 0, false, currentLine}); break;
            case '}': tokens.push_back({TokType::RIGHT_BRACE, "}", 0, false, currentLine}); break;
            case '(': tokens.push_back({TokType::LEFT_PAREN, "(", 0, false, currentLine}); break;
            case ')': tokens.push_back({TokType::RIGHT_PAREN, ")", 0, false, currentLine}); break;
            case ',': tokens.push_back({TokType::COMMA, ",", 0, false, currentLine}); break;
            case '.': tokens.push_back({TokType::DOT, ".", 0, false, currentLine}); break;
            case ';': tokens.push_back({TokType::SEMICOLON, ";", 0, false, currentLine}); break;
            case '=': tokens.push_back({TokType::EQUAL, "=", 0, false, currentLine}); break;
            case '+': tokens.push_back({TokType::PLUS, "+", 0, false, currentLine}); break;
            case '-': tokens.push_back({TokType::MINUS, "-", 0, false, currentLine}); break;
            case '*': tokens.push_back({TokType::STAR, "*", 0, false, currentLine}); break;
            case '/': tokens.push_back({TokType::SLASH, "/", 0, false, currentLine}); break;
            case '%': tokens.push_back({TokType::PERCENT, "%", 0, false, currentLine}); break;
            case '!': tokens.push_back({TokType::BANG, "!", 0, false, currentLine}); break;
            case '<': tokens.push_back({TokType::LESS, "<", 0, false, currentLine}); break;
            case '>': tokens.push_back({TokType::GREATER, ">", 0, false, currentLine}); break;
            default:
                throw runtime_error("😭 Chi sasura! Ye kaisa alien symbol ghusa diye ho code mein: " + string(1, c));
        }
    }
    tokens.push_back({TokType::T_EOF, "", 0, false, currentLine});
    return tokens;
}

// ==============================================================================
// 7. PARSER (PYTHON/JS STYLE WITH EXACT MEME ERRORS FROM DOCS)
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
        throw runtime_error(err);
    }

    void checkSemicolon() {
        if (match(TokType::SEMICOLON)) return;
        if (cur > 0 && cur < tokens.size() && tokens[cur].type != TokType::RIGHT_BRACE && tokens[cur].type != TokType::T_EOF) {
            if (tokens[cur].line == tokens[cur - 1].line) {
                throw runtime_error("Chhoti bachi ho kya?! Line ke end mein ';' lagana bhool gaye!");
            }
        }
    }

    shared_ptr<Expr> primary() {
        if (check(TokType::T_EOF)) {
            throw runtime_error("Moye Moye! Expression adhoori chhod ke code beech mein hi khatam kar diya.");
        }
        if (match(TokType::NUM)) return make_shared<LiteralExpr>(Value(previous().num, previous().isFloat));
        if (match(TokType::STR)) return make_shared<LiteralExpr>(Value(previous().text));
        if (match(TokType::KW_TRUE)) return make_shared<LiteralExpr>(Value(true));
        if (match(TokType::KW_FALSE)) return make_shared<LiteralExpr>(Value(false));
        if (match(TokType::KW_NULL)) return make_shared<LiteralExpr>(Value());
        if (match(TokType::KW_THIS)) return make_shared<VarExpr>("this");
        if (match(TokType::IDENT)) return make_shared<VarExpr>(previous().text);
        if (match(TokType::KW_INPUT)) {
            shared_ptr<Expr> prompt = nullptr;
            if (match(TokType::LEFT_PAREN)) {
                if (!check(TokType::RIGHT_PAREN)) prompt = expression();
                consume(TokType::RIGHT_PAREN, "Bracket '(' khol ke chhod diya... band tera baap karega?");
            }
            return make_shared<InputExpr>(prompt);
        }
        if (match(TokType::LEFT_PAREN)) {
            auto expr = expression();
            consume(TokType::RIGHT_PAREN, "Bracket '(' khol ke chhod diya... band tera baap karega?");
            return make_shared<GroupExpr>(expr);
        }
        throw runtime_error("Aayein? Baigan! 😭\nYe '" + peek().text + "' beech mein kahan se tapak pada?\nSaala ye dukh kahe khatam nahi hota be... dhang se syntax likh le!");
    }

    shared_ptr<Expr> postfix() {
        auto expr = primary();
        while (true) {
            if (match(TokType::DOT)) {
                string name = consume(TokType::IDENT, "Ye sab doglapan hai! Dot '.' ke baad property ka naam chahiye!").text;
                expr = make_shared<GetExpr>(expr, name);
            } else if (match(TokType::LEFT_PAREN)) {
                vector<shared_ptr<Expr>> args;
                if (!check(TokType::RIGHT_PAREN)) {
                    do { args.push_back(expression()); } while (match(TokType::COMMA));
                }
                consume(TokType::RIGHT_PAREN, "Bracket '(' khol ke chhod diya... band tera baap karega?");
                expr = make_shared<CallExpr>(expr, args);
            } else {
                break;
            }
        }
        return expr;
    }

    shared_ptr<Expr> unary() {
        if (match(TokType::BANG) || match(TokType::MINUS) || match(TokType::KW_NOT)) {
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
        while (match(TokType::KW_AND)) {
            string op = previous().text;
            auto right = equality();
            expr = make_shared<LogicalExpr>(expr, op, right);
        }
        return expr;
    }

    shared_ptr<Expr> logicalOr() {
        auto expr = logicalAnd();
        while (match(TokType::KW_OR)) {
            string op = previous().text;
            auto right = logicalAnd();
            expr = make_shared<LogicalExpr>(expr, op, right);
        }
        return expr;
    }

    shared_ptr<Expr> assignment() {
        auto expr = logicalOr();
        if (match(TokType::EQUAL)) {
            if (dynamic_pointer_cast<GroupExpr>(expr)) {
                throw runtime_error("Ye sab doglapan hai! '=' ke left side mein sirf seedha variable ya property aata hai, koi equation ya bracket nahi!");
            }
            auto val = assignment();
            if (auto v = dynamic_pointer_cast<VarExpr>(expr)) return make_shared<AssignExpr>(v->name, val);
            if (auto g = dynamic_pointer_cast<GetExpr>(expr)) return make_shared<SetExpr>(g->object, g->name, val);
            throw runtime_error("Ye sab doglapan hai! '=' ke left side mein sirf seedha variable ya property aata hai, koi equation ya bracket nahi!");
        }
        return expr;
    }

public:
    Parser(vector<Token> t) : tokens(std::move(t)) {}

    shared_ptr<Expr> expression() { return assignment(); }

    shared_ptr<BlockStmt> block() {
        auto b = make_shared<BlockStmt>();
        consume(TokType::LEFT_BRACE, "Curly brace '{' khol ke chhod diya... band tera baap karega?");
        while (!check(TokType::RIGHT_BRACE) && !check(TokType::T_EOF)) {
            b->stmts.push_back(statement());
        }
        consume(TokType::RIGHT_BRACE, "Curly brace '{' khol ke chhod diya... band tera baap karega?");
        return b;
    }

    shared_ptr<Stmt> statement() {
        if (match(TokType::KW_PRINT)) {
            vector<shared_ptr<Expr>> exprs;
            if (match(TokType::LEFT_PAREN)) {
                if (!check(TokType::RIGHT_PAREN)) {
                    do { exprs.push_back(expression()); } while (match(TokType::COMMA));
                }
                consume(TokType::RIGHT_PAREN, "Bracket '(' khol ke chhod diya... band tera baap karega?");
            } else {
                if (!check(TokType::SEMICOLON) && !check(TokType::RIGHT_BRACE) && !check(TokType::T_EOF)) {
                    do { exprs.push_back(expression()); } while (match(TokType::COMMA));
                }
            }
            checkSemicolon();
            return make_shared<PrintStmt>(exprs);
        }
        if (match(TokType::KW_LET)) {
            string name = consume(TokType::IDENT, "Ye sab doglapan hai! 'let' ke baad variable ka naam hona chahiye!").text;
            shared_ptr<Expr> init = match(TokType::EQUAL) ? expression() : nullptr;
            checkSemicolon();
            return make_shared<VarStmt>(name, init);
        }
        if (match(TokType::KW_IF)) {
            bool paren = match(TokType::LEFT_PAREN);
            auto cond = expression();
            if (dynamic_pointer_cast<AssignExpr>(cond)) {
                throw runtime_error("Control Uday, control! 'if' mein '=' (assign) nahi, '==' (compare) lagta hai!");
            }
            if (paren) consume(TokType::RIGHT_PAREN, "Bracket '(' khol ke chhod diya... band tera baap karega?");
            auto thenBranch = block();
            shared_ptr<Stmt> elseBranch = nullptr;
            if (match(TokType::KW_ELSE)) {
                if (check(TokType::KW_IF)) elseBranch = statement();
                else elseBranch = block();
            }
            return make_shared<IfStmt>(cond, thenBranch, elseBranch);
        }
        if (match(TokType::KW_WHILE)) {
            bool paren = match(TokType::LEFT_PAREN);
            auto cond = expression();
            if (dynamic_pointer_cast<AssignExpr>(cond)) {
                throw runtime_error("Control Uday, control! 'while' mein '=' (assign) nahi, '==' (compare) lagta hai!");
            }
            if (paren) consume(TokType::RIGHT_PAREN, "Bracket '(' khol ke chhod diya... band tera baap karega?");
            auto body = block();
            return make_shared<WhileStmt>(cond, body);
        }
        if (match(TokType::KW_FOR)) {
            string varName = consume(TokType::IDENT, "Kisko ghumana hai? 'for' ke baad loop variable ka naam likh!").text;
            consume(TokType::KW_IN, "Ye sab doglapan hai! 'for' loop mein 'in' lagana zaroori hai!");
            auto start = expression();
            consume(TokType::KW_TO, "Ye sab doglapan hai! Range ke liye 'to' lagana zaroori hai (Jaise: for i in 1 to 10)!");
            auto end = expression();
            shared_ptr<Expr> step = nullptr;
            if (match(TokType::KW_STEP)) step = expression();
            auto body = block();
            return make_shared<ForRangeStmt>(varName, start, end, step, body);
        }
        if (match(TokType::KW_BREAK)) {
            checkSemicolon();
            return make_shared<BreakStmt>();
        }
        if (match(TokType::KW_CONTINUE)) {
            checkSemicolon();
            return make_shared<ContinueStmt>();
        }
        if (match(TokType::KW_RETURN)) {
            shared_ptr<Expr> val = nullptr;
            if (!check(TokType::SEMICOLON) && !check(TokType::RIGHT_BRACE)) val = expression();
            checkSemicolon();
            return make_shared<ReturnStmt>(val);
        }
        if (match(TokType::KW_FUNCTION)) {
            string name = consume(TokType::IDENT, "Ye sab doglapan hai! 'function' ke baad valid function name likh!").text;
            consume(TokType::LEFT_PAREN, "Bracket '(' khol ke chhod diya... band tera baap karega?");
            vector<string> params;
            if (!check(TokType::RIGHT_PAREN)) {
                do { params.push_back(consume(TokType::IDENT, "Parameter ka valid naam likh!").text); } while (match(TokType::COMMA));
            }
            consume(TokType::RIGHT_PAREN, "Bracket '(' khol ke chhod diya... band tera baap karega?");
            auto body = block();
            return make_shared<FuncDeclStmt>(name, params, body);
        }
        if (match(TokType::KW_CLASS)) {
            string name = consume(TokType::IDENT, "Kiski class le rahe ho? 'class' ke baad naam toh likh mere dost!").text;
            string super = match(TokType::KW_EXTENDS) ? consume(TokType::IDENT, "Fake nepotism! 'extends' ke baad parent class ka naam kaun likhega?").text : "";
            consume(TokType::LEFT_BRACE, "Curly brace '{' khol ke chhod diya... band tera baap karega?");
            vector<shared_ptr<FuncDeclStmt>> methods;
            while (!check(TokType::RIGHT_BRACE) && !check(TokType::T_EOF)) {
                match(TokType::KW_FUNCTION);
                string mName = consume(TokType::IDENT, "Class method ka valid naam likh!").text;
                consume(TokType::LEFT_PAREN, "Bracket '(' khol ke chhod diya... band tera baap karega?");
                vector<string> params;
                if (!check(TokType::RIGHT_PAREN)) {
                    do { params.push_back(consume(TokType::IDENT, "Parameter ka naam likho!").text); } while (match(TokType::COMMA));
                }
                consume(TokType::RIGHT_PAREN, "Bracket '(' khol ke chhod diya... band tera baap karega?");
                auto body = block();
                methods.push_back(make_shared<FuncDeclStmt>(mName, params, body));
            }
            consume(TokType::RIGHT_BRACE, "Curly brace '{' khol ke chhod diya... band tera baap karega?");
            return make_shared<ClassStmt>(name, super, methods);
        }
        if (check(TokType::LEFT_BRACE)) return block();

        auto e = expression();
        checkSemicolon();
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
    env->define("type", Value(make_shared<NativeFunction>("type", 1, [](const vector<Value>& args) {
        return Value(args.empty() ? "null" : args[0].typeName());
    })));
    env->define("len", Value(make_shared<NativeFunction>("len", 1, [](const vector<Value>& args) {
        if (args.empty() || args[0].type != Value::STR) return Value(0);
        return Value((long long)args[0].strVal.size());
    })));
    env->define("float", Value(make_shared<NativeFunction>("float", 1, [](const vector<Value>& args) {
        if (args.empty()) return Value(0.0, true);
        if (args[0].type == Value::NUM) return Value(args[0].numVal, true);
        if (args[0].type == Value::STR) { try { return Value(stod(args[0].strVal), true); } catch (...) {} }
        return Value(0.0, true);
    })));
    env->define("int", Value(make_shared<NativeFunction>("int", 1, [](const vector<Value>& args) {
        if (args.empty()) return Value(0);
        if (args[0].type == Value::NUM) return Value((long long)args[0].numVal);
        if (args[0].type == Value::STR) { try { return Value((long long)stoll(args[0].strVal)); } catch (...) {} }
        return Value(0);
    })));
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
        if (!file.is_open()) {
            cerr << "\n❌ Gadbad ho gayi:\nFile '" << argv[1] << "' gayab hai! Sahi path daalo warna main isse out hoon.\n\n";
            return 1;
        }
        stringstream ss; ss << file.rdbuf(); code = ss.str();
    } else {
        code = R"(
            print("=== Welcome to Minimal OOP Language Showcase ===");

            // 1. Base Class: Person (Encapsulation of State & Behavior)
            class Person {
                constructor(name, age) {
                    this.name = name;
                    this.age = age;
                }

                intro() {
                    print("Namaste! My name is", this.name, "and my age is", this.age, "years.");
                }

                work() {
                    print(this.name, "does simple honest work.");
                }
            }

            // 2. Subclass: Don (Inheritance via 'extends' & Polymorphic Overriding)
            class Don extends Person {
                constructor(name, age, city) {
                    this.name = name;
                    this.age = age;
                    this.city = city;
                }

                work() {
                    print("Dangerous work! I am the Don of", this.city + "!");
                }

                threat() {
                    print("Utha le re baba!", this.name, "se panga mat lena!");
                }
            }

            let raju = Person("Raju", 25);
            raju.intro();
            raju.work();

            print("");

            let baburao = Don("Baburao Ganpatrao Apte", 50, "Khadakwasla");
            baburao.intro();  // Inherited method from Person
            baburao.work();   // Overridden method from Don
            baburao.threat(); // Subclass specific method

            baburao.dialogue = "Khopdi tod saale ka!";
            print("Baburao favorite dialogue:", baburao.dialogue);

            print("\n--- Loops & Control Flow ---");
            for i in 1 to 3 {
                print("Loop count:", i);
            }

            function add(a, b) {
                return a + b;
            }
            print("20 + 26 =", add(20, 26));
        )";
    }

    try {
        auto tokens = tokenize(code);
        Parser parser(tokens);
        auto program = parser.parse();
        auto globalScope = createGlobalEnvironment();
        for (auto& stmt : program) stmt->exec(globalScope);
    } catch (const exception& e) {
        cerr << "\n❌ Gadbad ho gayi:\n" << e.what() << "\n\n";
        return 1;
    }

    return 0;
}
