/**
 * ==============================================================================
 * PROJECT: MEMElang - Minimalist Parody Object-Oriented Language
 * PURPOSE: Minimalist College OOP Submission (Single File, Fits in ~4-6 Pages)
 *
 * CORE OOP PILLARS DEMONSTRATED IN THIS CODE:
 * 1. ENCAPSULATION: Scope isolation in Environment, object fields in MemeInstance.
 * 2. ABSTRACTION  : Pure virtual base classes (Expr, Stmt) defining AST nodes.
 * 3. INHERITANCE  : 
 *    - C++ Level    : Hierarchies for AST Nodes (ClassStmt, MethodStmt, CallExpr, PoochExpr).
 *    - Language Level: Subclassing via 'se' keyword with dynamic method resolution.
 * 4. POLYMORPHISM : Dynamic dispatch via virtual eval()/exec() & method overrides.
 *
 * INPUT HANDLING:
 * - 'pooch()' / 'puch()' captures full lines including all spaces, just like Python's input().
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

using namespace std;

// --- Forward Declarations ---
struct Value;
class Environment;
class MemeClass;
class MemeInstance;
class Stmt;
class MethodStmt;

// ==============================================================================
// 1. RUNTIME VALUE & ENCAPSULATION (Environment & Scope)
// ==============================================================================
struct Value {
    enum Type { NIL, NUM, STR, INSTANCE, CLASS } type = NIL;
    double num = 0;
    string str = "";
    shared_ptr<MemeInstance> inst = nullptr;
    shared_ptr<MemeClass> cls = nullptr;

    Value() : type(NIL) {}
    Value(double n) : type(NUM), num(n) {}
    Value(string s) : type(STR), str(std::move(s)) {}
    Value(const char* s) : type(STR), str(s) {}
    Value(shared_ptr<MemeInstance> i) : type(INSTANCE), inst(i) {}
    Value(shared_ptr<MemeClass> c) : type(CLASS), cls(c) {}

    void print() const {
        if (type == NUM) {
            if (num == (long long)num) cout << (long long)num;
            else cout << num;
        } else if (type == STR) cout << str;
        else if (type == INSTANCE) cout << "<object>";
        else if (type == CLASS) cout << "<class>";
        else cout << "null";
    }
};

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
// 2. OOP CORE: CLASSES, INSTANCES & DYNAMIC DISPATCH
// ==============================================================================
class MemeClass {
public:
    string name;
    shared_ptr<MemeClass> superclass; // Language-level Inheritance
    map<string, shared_ptr<MethodStmt>> methods;

    MemeClass(string n, shared_ptr<MemeClass> sup = nullptr) : name(std::move(n)), superclass(sup) {}
    shared_ptr<MethodStmt> findMethod(const string& mName);
};

class MemeInstance {
public:
    shared_ptr<MemeClass> klass;
    map<string, Value> fields; // Encapsulated object state

    MemeInstance(shared_ptr<MemeClass> k) : klass(k) {}
    Value get(const string& prop) { return fields.count(prop) ? fields[prop] : Value(); }
    void set(const string& prop, Value val) { fields[prop] = val; }
};

// ==============================================================================
// 3. ABSTRACTION & POLYMORPHISM: AST HIERARCHY
// ==============================================================================
class Expr {
public:
    virtual ~Expr() = default;
    virtual Value eval(shared_ptr<Environment> env) = 0; // Pure Virtual Method
};

class Stmt {
public:
    virtual ~Stmt() = default;
    virtual void exec(shared_ptr<Environment> env) = 0; // Pure Virtual Method
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

class GetExpr : public Expr {
public:
    shared_ptr<Expr> object;
    string name;
    GetExpr(shared_ptr<Expr> o, string n) : object(o), name(std::move(n)) {}
    Value eval(shared_ptr<Environment> env) override {
        Value o = object->eval(env);
        return (o.type == Value::INSTANCE && o.inst) ? o.inst->get(name) : Value();
    }
};

class SetExpr : public Expr {
    shared_ptr<Expr> object;
    string name;
    shared_ptr<Expr> valExpr;
public:
    SetExpr(shared_ptr<Expr> o, string n, shared_ptr<Expr> v) : object(o), name(std::move(n)), valExpr(v) {}
    Value eval(shared_ptr<Environment> env) override {
        Value v = valExpr->eval(env), o = object->eval(env);
        if (o.type == Value::INSTANCE && o.inst) o.inst->set(name, v);
        return v;
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

class BlockStmt : public Stmt {
public:
    vector<shared_ptr<Stmt>> stmts;
    void exec(shared_ptr<Environment> env) override { for (auto& s : stmts) s->exec(env); }
};

class MethodStmt : public Stmt {
public:
    string name;
    vector<string> params;
    shared_ptr<BlockStmt> body;
    MethodStmt(string n, vector<string> p, shared_ptr<BlockStmt> b) : name(std::move(n)), params(std::move(p)), body(b) {}
    void exec(shared_ptr<Environment>) override {}
};

shared_ptr<MethodStmt> MemeClass::findMethod(const string& mName) {
    if (methods.count(mName)) return methods[mName];
    return superclass ? superclass->findMethod(mName) : nullptr; // Inherited lookup
}

class CallExpr : public Expr {
    shared_ptr<Expr> callee;
    vector<shared_ptr<Expr>> args;
public:
    CallExpr(shared_ptr<Expr> c, vector<shared_ptr<Expr>> a) : callee(c), args(std::move(a)) {}
    Value eval(shared_ptr<Environment> env) override {
        // Dynamic Method Invocation (obj.method(...))
        if (auto getExpr = dynamic_pointer_cast<GetExpr>(callee)) {
            Value objVal = getExpr->object->eval(env);
            if (objVal.type == Value::INSTANCE && objVal.inst) {
                if (auto method = objVal.inst->klass->findMethod(getExpr->name)) {
                    auto methodEnv = make_shared<Environment>(env);
                    methodEnv->define("khud", objVal); // Bind 'khud' (this) pointer
                    for (size_t i = 0; i < method->params.size(); i++)
                        methodEnv->define(method->params[i], (i < args.size()) ? args[i]->eval(env) : Value());
                    method->body->exec(methodEnv);
                    return Value();
                }
            }
        }
        // Object Instantiation (Class(...))
        Value cVal = callee->eval(env);
        if (cVal.type == Value::CLASS && cVal.cls) {
            auto inst = make_shared<MemeInstance>(cVal.cls);
            Value instVal(inst);
            if (auto init = cVal.cls->findMethod("shuru")) { // Constructor call
                auto initEnv = make_shared<Environment>(env);
                initEnv->define("khud", instVal);
                for (size_t i = 0; i < init->params.size(); i++)
                    initEnv->define(init->params[i], (i < args.size()) ? args[i]->eval(env) : Value());
                init->body->exec(initEnv);
            }
            return instVal;
        }
        return Value();
    }
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

class ClassStmt : public Stmt {
    string name, superName;
    vector<shared_ptr<MethodStmt>> methods;
public:
    ClassStmt(string n, string s, vector<shared_ptr<MethodStmt>> m) : name(std::move(n)), superName(std::move(s)), methods(std::move(m)) {}
    void exec(shared_ptr<Environment> env) override {
        shared_ptr<MemeClass> super = nullptr;
        if (!superName.empty()) {
            Value sVal = env->get(superName);
            if (sVal.type == Value::CLASS) super = sVal.cls;
        }
        auto cls = make_shared<MemeClass>(name, super);
        for (auto& m : methods) cls->methods[m->name] = m;
        env->define(name, Value(cls));
    }
};

class ExprStmt : public Stmt {
    shared_ptr<Expr> expr;
public:
    ExprStmt(shared_ptr<Expr> e) : expr(e) {}
    void exec(shared_ptr<Environment> env) override { expr->eval(env); }
};

// ==============================================================================
// 4. MEME LEXER & PARSER
// ==============================================================================
enum TokType { IDENT, STR, NUM, KW_SANCHA, KW_SE, KW_KAR, KW_LE, KW_DIKHA, KW_KHUD, KW_PUCH, SYM, T_EOF };
struct Token { TokType type; string text; double num = 0; };

vector<Token> tokenize(const string& src) {
    vector<Token> tokens;
    size_t i = 0, n = src.size();
    while (i < n) {
        if (isspace(src[i])) { i++; continue; }
        if (src[i] == '/' && i + 1 < n && src[i + 1] == '/') { while (i < n && src[i] != '\n') i++; continue; }
        if (src[i] == '"') {
            string s; i++;
            while (i < n && src[i] != '"') { if (src[i] == '\\' && i + 1 < n) i++; s += src[i++]; }
            if (i < n) i++;
            tokens.push_back({TokType::STR, s, 0}); continue;
        }
        if (isdigit(src[i])) {
            size_t start = i;
            while (i < n && (isdigit(src[i]) || src[i] == '.')) i++;
            tokens.push_back({TokType::NUM, "", stod(src.substr(start, i - start))}); continue;
        }
        if (isalpha(src[i]) || src[i] == '_') {
            size_t start = i;
            while (i < n && (isalnum(src[i]) || src[i] == '_')) i++;
            string w = src.substr(start, i - start);
            if (w == "sancha") tokens.push_back({TokType::KW_SANCHA, w});
            else if (w == "se") tokens.push_back({TokType::KW_SE, w});
            else if (w == "kar") tokens.push_back({TokType::KW_KAR, w});
            else if (w == "le") tokens.push_back({TokType::KW_LE, w});
            else if (w == "dikha" || w == "flex") tokens.push_back({TokType::KW_DIKHA, w});
            else if (w == "khud") tokens.push_back({TokType::KW_KHUD, w});
            else if (w == "puch" || w == "pooch") tokens.push_back({TokType::KW_PUCH, w});
            else tokens.push_back({TokType::IDENT, w}); continue;
        }
        tokens.push_back({TokType::SYM, string(1, src[i++]), 0});
    }
    tokens.push_back({TokType::T_EOF, "", 0});
    return tokens;
}

class Parser {
    vector<Token> tokens;
    size_t cur = 0;
    Token advance() { return tokens[cur++]; }
    bool check(TokType t, const string& s = "") { return cur < tokens.size() && tokens[cur].type == t && (s.empty() || tokens[cur].text == s); }
    bool match(TokType t, const string& s = "") { if (check(t, s)) { advance(); return true; } return false; }
    Token consume(TokType t, const string& s = "") { return check(t, s) ? advance() : Token{TokType::T_EOF, ""}; }

    shared_ptr<Expr> primary() {
        if (check(TokType::NUM)) return make_shared<LiteralExpr>(Value(advance().num));
        if (check(TokType::STR)) return make_shared<LiteralExpr>(Value(advance().text));
        if (check(TokType::KW_KHUD)) { advance(); return make_shared<VarExpr>("khud"); }
        if (match(TokType::KW_PUCH)) {
            shared_ptr<Expr> prompt = nullptr;
            if (match(TokType::SYM, "(")) {
                if (!check(TokType::SYM, ")")) prompt = expression();
                consume(TokType::SYM, ")");
            } else if (check(TokType::STR)) {
                prompt = primary();
            }
            return make_shared<PoochExpr>(prompt);
        }
        if (check(TokType::IDENT)) return make_shared<VarExpr>(advance().text);
        if (match(TokType::SYM, "(")) { auto e = expression(); consume(TokType::SYM, ")"); return e; }
        return nullptr;
    }
    shared_ptr<Expr> postfix() {
        auto expr = primary();
        while (true) {
            if (match(TokType::SYM, ".")) {
                expr = make_shared<GetExpr>(expr, consume(TokType::IDENT).text);
            } else if (match(TokType::SYM, "(")) {
                vector<shared_ptr<Expr>> args;
                if (!check(TokType::SYM, ")")) { do { args.push_back(expression()); } while (match(TokType::SYM, ",")); }
                consume(TokType::SYM, ")");
                expr = make_shared<CallExpr>(expr, args);
            } else break;
        }
        return expr;
    }
public:
    Parser(vector<Token> t) : tokens(std::move(t)) {}
    shared_ptr<Expr> expression() {
        auto expr = postfix();
        if (match(TokType::SYM, "=")) {
            auto val = expression();
            if (auto v = dynamic_pointer_cast<VarExpr>(expr)) return make_shared<AssignExpr>(v->name, val);
            if (auto g = dynamic_pointer_cast<GetExpr>(expr)) return make_shared<SetExpr>(g->object, g->name, val);
        }
        return expr;
    }
    shared_ptr<BlockStmt> block() {
        auto b = make_shared<BlockStmt>();
        consume(TokType::SYM, "{");
        while (!check(TokType::SYM, "}") && !check(TokType::T_EOF)) b->stmts.push_back(statement());
        consume(TokType::SYM, "}");
        return b;
    }
    shared_ptr<Stmt> statement() {
        if (match(TokType::KW_DIKHA)) {
            vector<shared_ptr<Expr>> exprs;
            if (!check(TokType::SYM, ";")) { do { exprs.push_back(expression()); } while (match(TokType::SYM, ",")); }
            consume(TokType::SYM, ";");
            return make_shared<PrintStmt>(exprs);
        }
        if (match(TokType::KW_LE)) {
            string name = consume(TokType::IDENT).text;
            shared_ptr<Expr> init = match(TokType::SYM, "=") ? expression() : nullptr;
            consume(TokType::SYM, ";");
            return make_shared<VarStmt>(name, init);
        }
        if (match(TokType::KW_SANCHA)) {
            string name = consume(TokType::IDENT).text;
            string super = match(TokType::KW_SE) ? consume(TokType::IDENT).text : "";
            consume(TokType::SYM, "{");
            vector<shared_ptr<MethodStmt>> methods;
            while (!check(TokType::SYM, "}") && !check(TokType::T_EOF)) {
                consume(TokType::KW_KAR);
                string mName = consume(TokType::IDENT).text;
                consume(TokType::SYM, "(");
                vector<string> params;
                if (!check(TokType::SYM, ")")) { do { params.push_back(consume(TokType::IDENT).text); } while (match(TokType::SYM, ",")); }
                consume(TokType::SYM, ")");
                methods.push_back(make_shared<MethodStmt>(mName, params, block()));
            }
            consume(TokType::SYM, "}");
            return make_shared<ClassStmt>(name, super, methods);
        }
        if (check(TokType::SYM, "{")) return block();
        auto e = expression();
        consume(TokType::SYM, ";");
        return make_shared<ExprStmt>(e);
    }
    vector<shared_ptr<Stmt>> parse() {
        vector<shared_ptr<Stmt>> stmts;
        while (!check(TokType::T_EOF)) stmts.push_back(statement());
        return stmts;
    }
};

// ==============================================================================
// 5. MAIN PROGRAM & EMBEDDED DEMO (Zero-Dependency Run)
// ==============================================================================
int main(int argc, char* argv[]) {
    string code;
    if (argc > 2 && string(argv[1]) == "-e") {
        code = argv[2];
    } else if (argc > 1) {
        ifstream file(argv[1]);
        if (!file.is_open()) { cerr << "Error: Cannot open " << argv[1] << "\n"; return 1; }
        stringstream ss; ss << file.rdbuf(); code = ss.str();
    } else {
        code = R"(
            dikha "=== MEMElang Parody OOP Showcase ===";
            // 1. Base Class: Banda (Encapsulation of State & Methods)
            sancha Banda {
                kar shuru(naam, umar) { khud.naam = naam; khud.umar = umar; }
                kar parichay() { dikha "Namaste! Mera naam", khud.naam, "hai aur umar", khud.umar, "saal hai."; }
                kar kaam() { dikha khud.naam, "seedha sadha kaam karta hai."; }
            }
            // 2. Subclass: Don (Inheritance via 'se' & Polymorphic Overriding)
            sancha Don se Banda {
                kar shuru(naam, umar, ilaka) { khud.naam = naam; khud.umar = umar; khud.ilaka = ilaka; }
                kar kaam() { dikha "Khatarnak kaam! Mai", khud.ilaka, "ka Don hoon!"; }
                kar dhamki() { dikha "Utha le re baba!", khud.naam, "se panga mat lena!"; }
            }
            // Instantiation & Dynamic Dispatch
            le raju = Banda("Raju", 25);
            raju.parichay();
            raju.kaam();
            dikha "";
            le baburao = Don("Baburao Apte", 50, "Khadakwasla");
            baburao.parichay(); // Inherited method
            baburao.kaam();      // Overridden method
            baburao.dhamki();    // Subclass specific method
            baburao.dialogue = "Khopdi tod saale ka!"; // Dynamic Field Assignment
            dikha "Baburao dialogue:", baburao.dialogue;
        )";
    }
    auto tokens = tokenize(code);
    Parser parser(tokens);
    auto program = parser.parse();
    auto globalScope = make_shared<Environment>();
    for (auto& stmt : program) stmt->exec(globalScope);
    return 0;
}
