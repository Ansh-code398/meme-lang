#include "NativeCompiler.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <cstdlib>
#include <unordered_set>
#include "../lexer/Lexer.h"
#include "../types/TypeChecker.h"

namespace {

class CodeGenerator {
private:
    std::ostringstream globalDecls;
    std::ostringstream mainBody;
    int indentLevel = 1;
    std::unordered_set<std::string> knownClasses;
    std::unordered_set<std::string> knownFunctions;

    std::string indent() const {
        return std::string(indentLevel * 4, ' ');
    }

    std::string sanitizeIdentifier(const std::string &name) {
        if (name == "khud" || name == "apun" || name == "self" || name == "this") return "this";
        if (name == "nalla" || name == "null" || name == "nil") return "nullptr";
        if (name == "han" || name == "true") return "true";
        if (name == "nhi" || name == "false") return "false";

        // C++ reserved keywords
        static const std::unordered_set<std::string> cppKeywords = {
            "alignas", "alignof", "and", "and_eq", "asm", "atomic_cancel", "atomic_commit",
            "atomic_noexcept", "auto", "bitand", "bitor", "bool", "break", "case", "catch",
            "char", "char8_t", "char16_t", "char32_t", "class", "compl", "concept", "const",
            "consteval", "constexpr", "constinit", "const_cast", "continue", "co_await",
            "co_return", "co_yield", "decltype", "default", "delete", "do", "double",
            "dynamic_cast", "else", "enum", "explicit", "export", "extern", "false", "float",
            "for", "friend", "goto", "if", "inline", "int", "long", "mutable", "namespace",
            "new", "noexcept", "not", "not_eq", "nullptr", "operator", "or", "or_eq",
            "private", "protected", "public", "reflexpr", "register", "reinterpret_cast",
            "requires", "return", "short", "signed", "sizeof", "static", "static_assert",
            "static_cast", "struct", "switch", "synchronized", "template", "this",
            "thread_local", "throw", "true", "try", "typedef", "typeid", "typename",
            "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t", "while",
            "xor", "xor_eq"
        };

        if (cppKeywords.find(name) != cppKeywords.end()) {
            return "m_" + name;
        }
        return name;
    }

    std::string escapeString(const std::string &str) {
        std::string res;
        for (char c : str) {
            switch (c) {
            case '"': res += "\\\""; break;
            case '\\': res += "\\\\"; break;
            case '\n': res += "\\n"; break;
            case '\t': res += "\\t"; break;
            case '\r': res += "\\r"; break;
            default: res += c; break;
            }
        }
        return res;
    }

public:
    void scanDeclarations(const std::vector<std::unique_ptr<Stmt>> &program) {
        for (const auto &stmt : program) {
            if (auto *c = dynamic_cast<const ClassDeclStmt *>(stmt.get())) {
                knownClasses.insert(c->name);
            } else if (auto *f = dynamic_cast<const FuncDeclStmt *>(stmt.get())) {
                knownFunctions.insert(f->name);
            }
        }
    }

    std::string generateExpr(const Expr *expr) {
        if (!expr) return "MemeValue()";

        if (auto *e = dynamic_cast<const IntExpr *>(expr)) {
            return std::to_string(e->value) + "LL";
        }
        if (auto *e = dynamic_cast<const FloatExpr *>(expr)) {
            return std::to_string(e->value);
        }
        if (auto *e = dynamic_cast<const BoolExpr *>(expr)) {
            return e->value ? "true" : "false";
        }
        if (auto *e = dynamic_cast<const StringExpr *>(expr)) {
            return "std::string(\"" + escapeString(e->value) + "\")";
        }
        if (dynamic_cast<const NullExpr *>(expr)) {
            return "nullptr";
        }
        if (auto *e = dynamic_cast<const IdentifierExpr *>(expr)) {
            return sanitizeIdentifier(e->name);
        }
        if (auto *e = dynamic_cast<const AssignExpr *>(expr)) {
            return "(" + sanitizeIdentifier(e->name) + " = " + generateExpr(e->value.get()) + ")";
        }
        if (auto *e = dynamic_cast<const UnaryExpr *>(expr)) {
            std::string opStr;
            if (e->op.type == TokenType::MINUS) opStr = "-";
            else if (e->op.type == TokenType::NOT) opStr = "!";
            else opStr = e->op.value;
            return "(" + opStr + generateExpr(e->operand.get()) + ")";
        }
        if (auto *e = dynamic_cast<const BinaryExpr *>(expr)) {
            std::string opStr;
            switch (e->op.type) {
            case TokenType::PLUS: opStr = "+"; break;
            case TokenType::MINUS: opStr = "-"; break;
            case TokenType::MULTIPLY: opStr = "*"; break;
            case TokenType::DIVIDE: opStr = "/"; break;
            case TokenType::MOD: opStr = "%"; break;
            case TokenType::LESS_THAN: opStr = "<"; break;
            case TokenType::LESS_THAN_EQUAL: opStr = "<="; break;
            case TokenType::GREATER_THAN: opStr = ">"; break;
            case TokenType::GREATER_THAN_EQUAL: opStr = ">="; break;
            case TokenType::EQUAL: opStr = "=="; break;
            case TokenType::NOT_EQUAL: opStr = "!="; break;
            case TokenType::AUR: opStr = "&&"; break;
            case TokenType::YA: opStr = "||"; break;
            default: opStr = e->op.value; break;
            }
            return "(" + generateExpr(e->left.get()) + " " + opStr + " " + generateExpr(e->right.get()) + ")";
        }
        if (auto *e = dynamic_cast<const CallExpr *>(expr)) {
            // Check if call is a constructor: ClassName(...)
            if (auto *ident = dynamic_cast<const IdentifierExpr *>(e->callee.get())) {
                if (knownClasses.find(ident->name) != knownClasses.end()) {
                    std::string args;
                    for (size_t i = 0; i < e->args.size(); ++i) {
                        if (i > 0) args += ", ";
                        args += generateExpr(e->args[i].get());
                    }
                    return "std::make_shared<" + ident->name + ">(" + args + ")";
                }
            }

            // Check if method call: obj.method(args)
            if (auto *get = dynamic_cast<const GetExpr *>(e->callee.get())) {
                std::string objStr = generateExpr(get->object.get());
                std::string method = sanitizeIdentifier(get->name);

                // Handle .push() on vectors
                if (method == "push" || method == "dhal") {
                    method = "push_back";
                }

                std::string args;
                for (size_t i = 0; i < e->args.size(); ++i) {
                    if (i > 0) args += ", ";
                    args += generateExpr(e->args[i].get());
                }

                // If accessing khud / this or a shared pointer
                if (objStr == "this") {
                    return "this->" + method + "(" + args + ")";
                }
                if (objStr == "samay" || objStr == "itertools" || objStr == "math" ||
                    method == "push_back" || method == "pop_back" || method == "size" || method == "length") {
                    return objStr + "." + method + "(" + args + ")";
                }
                return objStr + "->" + method + "(" + args + ")";
            }

            // Regular function call
            std::string calleeStr = generateExpr(e->callee.get());
            std::string args;
            for (size_t i = 0; i < e->args.size(); ++i) {
                if (i > 0) args += ", ";
                args += generateExpr(e->args[i].get());
            }
            return calleeStr + "(" + args + ")";
        }
        if (auto *e = dynamic_cast<const GetExpr *>(expr)) {
            std::string objStr = generateExpr(e->object.get());
            std::string prop = sanitizeIdentifier(e->name);
            if (objStr == "this") {
                return "this->" + prop;
            }
            if (objStr == "samay" || objStr == "itertools" || objStr == "math") {
                return objStr + "." + prop;
            }
            return objStr + "->" + prop;
        }
        if (auto *e = dynamic_cast<const SetExpr *>(expr)) {
            std::string objStr = generateExpr(e->object.get());
            std::string prop = sanitizeIdentifier(e->name);
            std::string valStr = generateExpr(e->value.get());
            if (objStr == "this") {
                return "(this->" + prop + " = " + valStr + ")";
            }
            return "(" + objStr + "->" + prop + " = " + valStr + ")";
        }
        if (auto *e = dynamic_cast<const IndexExpr *>(expr)) {
            return "(" + generateExpr(e->target.get()) + "[" + generateExpr(e->index.get()) + "])";
        }
        if (auto *e = dynamic_cast<const IndexAssignExpr *>(expr)) {
            return "(" + generateExpr(e->target.get()) + "[" + generateExpr(e->index.get()) + "] = " + generateExpr(e->value.get()) + ")";
        }
        if (auto *e = dynamic_cast<const ArrayLiteralExpr *>(expr)) {
            std::string elems;
            for (size_t i = 0; i < e->elements.size(); ++i) {
                if (i > 0) elems += ", ";
                elems += generateExpr(e->elements[i].get());
            }
            return "std::vector<MemeValue>{" + elems + "}";
        }
        if (auto *e = dynamic_cast<const FStringExpr *>(expr)) {
            std::string ss = "((std::ostringstream()";
            for (const auto &part : e->parts) {
                ss += " << " + generateExpr(part.get());
            }
            ss += ").str())";
            return ss;
        }
        if (auto *e = dynamic_cast<const AddressOfExpr *>(expr)) {
            return "(&" + generateExpr(e->operand.get()) + ")";
        }
        if (auto *e = dynamic_cast<const DerefExpr *>(expr)) {
            return "(*" + generateExpr(e->operand.get()) + ")";
        }
        if (auto *e = dynamic_cast<const DerefAssignExpr *>(expr)) {
            return "(*" + generateExpr(e->pointer.get()) + " = " + generateExpr(e->value.get()) + ")";
        }
        if (auto *e = dynamic_cast<const MoveExpr *>(expr)) {
            return "std::move(" + generateExpr(e->operand.get()) + ")";
        }
        if (auto *e = dynamic_cast<const TernaryExpr *>(expr)) {
            return "(is_truthy(" + generateExpr(e->condition.get()) + ") ? " +
                   generateExpr(e->thenBranch.get()) + " : " + generateExpr(e->elseBranch.get()) + ")";
        }

        return "MemeValue()";
    }

    void generateStmt(const Stmt *stmt, std::ostream &out) {
        if (!stmt) return;

        if (auto *v = dynamic_cast<const VarDeclStmt *>(stmt)) {
            std::string varName = sanitizeIdentifier(v->name);
            std::string typeStr = "auto";
            if (v->typeAnnotation) {
                std::string t = v->typeAnnotation->toString();
                if (t == "int" || t == "int64") typeStr = "int64_t";
                else if (t == "float" || t == "float64") typeStr = "double";
                else if (t == "bool") typeStr = "bool";
                else if (t == "str" || t == "string") typeStr = "std::string";
            }

            if (v->initializer) {
                out << indent() << typeStr << " " << varName << " = " << generateExpr(v->initializer.get()) << ";\n";
            } else {
                out << indent() << "MemeValue " << varName << ";\n";
            }
            return;
        }

        if (auto *e = dynamic_cast<const ExprStmt *>(stmt)) {
            out << indent() << generateExpr(e->expression.get()) << ";\n";
            return;
        }

        if (auto *p = dynamic_cast<const PrintStmt *>(stmt)) {
            bool hasCustom = p->endExpr != nullptr || p->sepExpr != nullptr || p->styleExpr != nullptr || p->noNewline;
            if (!hasCustom) {
                out << indent() << "print_dikha(";
                for (size_t i = 0; i < p->args.size(); ++i) {
                    if (i > 0) out << ", ";
                    out << generateExpr(p->args[i].get());
                }
                out << ");\n";
            } else {
                std::string endExprStr = p->endExpr ? generateExpr(p->endExpr.get()) : (p->noNewline ? "std::string(\"\")" : "std::string(\"\\n\")");
                std::string sepExprStr = p->sepExpr ? generateExpr(p->sepExpr.get()) : "std::string(\" \")";
                std::string styleExprStr = p->styleExpr ? generateExpr(p->styleExpr.get()) : "std::string(\"\")";
                out << indent() << "print_dikha_opts(" << endExprStr << ", " << sepExprStr << ", " << styleExprStr;
                for (size_t i = 0; i < p->args.size(); ++i) {
                    out << ", " << generateExpr(p->args[i].get());
                }
                out << ");\n";
            }
            return;
        }

        if (auto *b = dynamic_cast<const BlockStmt *>(stmt)) {
            out << indent() << "{\n";
            indentLevel++;
            for (const auto &s : b->statements) {
                generateStmt(s.get(), out);
            }
            indentLevel--;
            out << indent() << "}\n";
            return;
        }

        if (auto *i = dynamic_cast<const IfStmt *>(stmt)) {
            out << indent() << "if (is_truthy(" << generateExpr(i->condition.get()) << ")) {\n";
            indentLevel++;
            generateStmt(i->thenBranch.get(), out);
            indentLevel--;
            if (i->elseBranch) {
                out << indent() << "} else {\n";
                indentLevel++;
                generateStmt(i->elseBranch.get(), out);
                indentLevel--;
            }
            out << indent() << "}\n";
            return;
        }

        if (auto *w = dynamic_cast<const WhileStmt *>(stmt)) {
            out << indent() << "while (is_truthy(" << generateExpr(w->condition.get()) << ")) {\n";
            indentLevel++;
            generateStmt(w->body.get(), out);
            indentLevel--;
            out << indent() << "}\n";
            return;
        }

        if (auto *g = dynamic_cast<const GhoomStmt *>(stmt)) {
            std::string var = sanitizeIdentifier(g->varName);
            std::string start = generateExpr(g->start.get());
            std::string end = generateExpr(g->end.get());
            std::string step = g->step ? generateExpr(g->step.get()) : "1LL";
            out << indent() << "for (int64_t " << var << " = " << start << "; " << var << " <= " << end << "; " << var << " += " << step << ") {\n";
            indentLevel++;
            generateStmt(g->body.get(), out);
            indentLevel--;
            out << indent() << "}\n";
            return;
        }

        if (auto *gm = dynamic_cast<const GhumaMeinStmt *>(stmt)) {
            std::string item = sanitizeIdentifier(gm->varName);
            std::string coll = generateExpr(gm->collection.get());
            out << indent() << "for (auto &&" << item << " : " << coll << ") {\n";
            indentLevel++;
            generateStmt(gm->body.get(), out);
            indentLevel--;
            out << indent() << "}\n";
            return;
        }

        if (auto *r = dynamic_cast<const ReturnStmt *>(stmt)) {
            if (r->value) {
                out << indent() << "return " << generateExpr(r->value.get()) << ";\n";
            } else {
                out << indent() << "return;\n";
            }
            return;
        }

        if (dynamic_cast<const BreakStmt *>(stmt)) {
            out << indent() << "break;\n";
            return;
        }

        if (dynamic_cast<const ContinueStmt *>(stmt)) {
            out << indent() << "continue;\n";
            return;
        }

        if (auto *d = dynamic_cast<const DropStmt *>(stmt)) {
            if (d->expr) {
                if (auto *get = dynamic_cast<const GetExpr *>(d->expr.get())) {
                    out << indent() << generateExpr(get) << " = nullptr;\n";
                } else {
                    out << indent() << "khatam(" << generateExpr(d->expr.get()) << ");\n";
                }
            }
            return;
        }

        if (auto *imp = dynamic_cast<const ImportStmt *>(stmt)) {
            std::string alias = imp->alias.empty() ? imp->path : imp->alias;
            alias = sanitizeIdentifier(alias);
            if (imp->path == "samay") {
                if (alias != "samay") {
                    out << indent() << "auto &" << alias << " = samay;\n";
                }
            } else if (imp->path == "itertools") {
                if (alias != "itertools") {
                    out << indent() << "auto &" << alias << " = itertools;\n";
                }
            } else if (imp->path == "math") {
                if (alias != "math") {
                    out << indent() << "auto &" << alias << " = math;\n";
                }
            }
            return;
        }
    }

    bool hasGetExpr(const Expr *e) {
        if (!e) return false;
        if (dynamic_cast<const GetExpr *>(e)) return true;
        if (auto *u = dynamic_cast<const UnaryExpr *>(e)) return hasGetExpr(u->operand.get());
        if (auto *b = dynamic_cast<const BinaryExpr *>(e)) return hasGetExpr(b->left.get()) || hasGetExpr(b->right.get());
        if (auto *c = dynamic_cast<const CallExpr *>(e)) {
            if (hasGetExpr(c->callee.get())) return true;
            for (const auto &arg : c->args) {
                if (hasGetExpr(arg.get())) return true;
            }
        }
        return false;
    }

    bool hasGetExprInReturn(const Stmt *stmt) {
        if (!stmt) return false;
        if (auto *r = dynamic_cast<const ReturnStmt *>(stmt)) {
            if (r->value && hasGetExpr(r->value.get())) return true;
        }
        if (auto *b = dynamic_cast<const BlockStmt *>(stmt)) {
            for (const auto &s : b->statements) {
                if (hasGetExprInReturn(s.get())) return true;
            }
        }
        if (auto *i = dynamic_cast<const IfStmt *>(stmt)) {
            if (hasGetExprInReturn(i->thenBranch.get())) return true;
            if (i->elseBranch && hasGetExprInReturn(i->elseBranch.get())) return true;
        }
        if (auto *w = dynamic_cast<const WhileStmt *>(stmt)) {
            return hasGetExprInReturn(w->body.get());
        }
        return false;
    }

    std::string findClassReturned(const Stmt *stmt) {
        if (!stmt) return "";
        if (auto *r = dynamic_cast<const ReturnStmt *>(stmt)) {
            if (r->value) {
                if (auto *c = dynamic_cast<const CallExpr *>(r->value.get())) {
                    if (auto *id = dynamic_cast<const IdentifierExpr *>(c->callee.get())) {
                        if (knownClasses.find(id->name) != knownClasses.end()) {
                            return id->name;
                        }
                    }
                }
            }
        }
        if (auto *b = dynamic_cast<const BlockStmt *>(stmt)) {
            for (const auto &s : b->statements) {
                std::string res = findClassReturned(s.get());
                if (!res.empty()) return res;
            }
        }
        if (auto *i = dynamic_cast<const IfStmt *>(stmt)) {
            std::string res = findClassReturned(i->thenBranch.get());
            if (!res.empty()) return res;
            if (i->elseBranch) {
                res = findClassReturned(i->elseBranch.get());
                if (!res.empty()) return res;
            }
        }
        if (auto *w = dynamic_cast<const WhileStmt *>(stmt)) {
            return findClassReturned(w->body.get());
        }
        return "";
    }

    void generateClass(const ClassDeclStmt *c) {
        std::string className = c->name;
        globalDecls << "struct " << className << " : public std::enable_shared_from_this<" << className << "> {\n";

        // Collect fields from constructor assignments khud.x = ...
        std::unordered_set<std::string> fields;
        for (const auto &m : c->methods) {
            if (m->name == "shuru" || m->name == "init" || m->name == "constructor") {
                // Scan method body for assignments to khud.x
                if (auto *b = dynamic_cast<const BlockStmt *>(m->body.get())) {
                    for (const auto &s : b->statements) {
                        if (auto *es = dynamic_cast<const ExprStmt *>(s.get())) {
                            if (auto *set = dynamic_cast<const SetExpr *>(es->expression.get())) {
                                fields.insert(set->name);
                            }
                        }
                    }
                }
            }
        }

        // Emit fields
        for (const auto &f : fields) {
            if (f == "left" || f == "right") {
                globalDecls << "    std::shared_ptr<" << className << "> " << f << " = nullptr;\n";
            } else {
                globalDecls << "    MemeValue " << f << " = 0LL;\n";
            }
        }
        globalDecls << "\n    " << className << "() = default;\n";

        // Constructor
        for (const auto &m : c->methods) {
            if (m->name == "shuru" || m->name == "init" || m->name == "constructor") {
                globalDecls << "    " << className << "(";
                for (size_t i = 0; i < m->paramList.size(); ++i) {
                    if (i > 0) globalDecls << ", ";
                    globalDecls << "auto " << sanitizeIdentifier(m->paramList[i].name);
                }
                globalDecls << ") {\n";
                indentLevel = 2;
                if (auto *b = dynamic_cast<const BlockStmt *>(m->body.get())) {
                    for (const auto &s : b->statements) {
                        generateStmt(s.get(), globalDecls);
                    }
                }
                indentLevel = 1;
                globalDecls << "    }\n";
            } else if (m->name == "alvida" || m->name == "drop") {
                globalDecls << "    ~" << className << "() {\n";
                indentLevel = 2;
                if (auto *b = dynamic_cast<const BlockStmt *>(m->body.get())) {
                    for (const auto &s : b->statements) {
                        generateStmt(s.get(), globalDecls);
                    }
                }
                indentLevel = 1;
                globalDecls << "    }\n";
            } else {
                // Ordinary method
                globalDecls << "    auto " << sanitizeIdentifier(m->name) << "(";
                for (size_t i = 0; i < m->paramList.size(); ++i) {
                    if (i > 0) globalDecls << ", ";
                    globalDecls << "MemeValue " << sanitizeIdentifier(m->paramList[i].name);
                }
                globalDecls << ") {\n";
                indentLevel = 2;
                if (auto *b = dynamic_cast<const BlockStmt *>(m->body.get())) {
                    for (const auto &s : b->statements) {
                        generateStmt(s.get(), globalDecls);
                    }
                }
                indentLevel = 1;
                globalDecls << "    }\n";
            }
        }

        globalDecls << "};\n\n";
    }

    void generateFunction(const FuncDeclStmt *f) {
        std::string fnName = sanitizeIdentifier(f->name);
        std::string returnType = "auto";

        std::string classRet = findClassReturned(f->body.get());
        if (!classRet.empty()) {
            returnType = "std::shared_ptr<" + classRet + ">";
        } else if (f->returnTypeAnnotation) {
            std::string t = f->returnTypeAnnotation->toString();
            if (t == "int" || t == "int64") returnType = "int64_t";
            else if (t == "float" || t == "float64") returnType = "double";
            else if (t == "bool") returnType = "bool";
            else if (t == "str" || t == "string") returnType = "std::string";
            else if (knownClasses.find(t) != knownClasses.end()) returnType = "std::shared_ptr<" + t + ">";
        } else if (hasGetExprInReturn(f->body.get())) {
            returnType = "MemeValue";
        }

        std::string paramDecl;
        for (size_t i = 0; i < f->paramList.size(); ++i) {
            if (i > 0) paramDecl += ", ";
            paramDecl += "const auto &" + sanitizeIdentifier(f->paramList[i].name);
        }

        globalDecls << "inline " << returnType << " " << fnName << "(" << paramDecl << ") {\n";
        indentLevel = 1;
        if (auto *b = dynamic_cast<const BlockStmt *>(f->body.get())) {
            for (const auto &s : b->statements) {
                generateStmt(s.get(), globalDecls);
            }
        } else {
            generateStmt(f->body.get(), globalDecls);
        }
        globalDecls << "}\n\n";
    }

    std::string buildSource() {
        std::ostringstream out;
        out << "// ==================================================================\n";
        out << "// Automatically generated by MEMElang AOT Native Compiler\n";
        out << "// ==================================================================\n\n";
        out << "#include \"src/compiler/MemeRuntime.h\"\n\n";

        // Global declarations (classes, functions)
        out << globalDecls.str();

        // Main entrypoint
        out << "int main(int argc, char* argv[]) {\n";
        out << "    (void)argc;\n";
        out << "    (void)argv;\n\n";
        out << mainBody.str();
        out << "\n    return 0;\n";
        out << "}\n";

        return out.str();
    }

    void generateProgram(const std::vector<std::unique_ptr<Stmt>> &program) {
        scanDeclarations(program);

        // Pass 0: Top-level imports
        for (const auto &stmt : program) {
            if (auto *imp = dynamic_cast<const ImportStmt *>(stmt.get())) {
                std::string alias = imp->alias.empty() ? imp->path : imp->alias;
                alias = sanitizeIdentifier(alias);
                if (imp->path == "samay") {
                    if (alias != "samay") {
                        globalDecls << "inline SamayModule &" << alias << " = samay;\n";
                    }
                } else if (imp->path == "itertools") {
                    if (alias != "itertools") {
                        globalDecls << "inline ItertoolsModule &" << alias << " = itertools;\n";
                    }
                } else if (imp->path == "math") {
                    if (alias != "math") {
                        globalDecls << "inline MathModule &" << alias << " = math;\n";
                    }
                }
            }
        }

        // First pass: Classes
        for (const auto &stmt : program) {
            if (auto *c = dynamic_cast<const ClassDeclStmt *>(stmt.get())) {
                generateClass(c);
            }
        }

        // Second pass: Functions
        for (const auto &stmt : program) {
            if (auto *f = dynamic_cast<const FuncDeclStmt *>(stmt.get())) {
                generateFunction(f);
            }
        }

        // Third pass: Main top-level statements
        for (const auto &stmt : program) {
            if (!dynamic_cast<const ClassDeclStmt *>(stmt.get()) &&
                !dynamic_cast<const FuncDeclStmt *>(stmt.get()) &&
                !dynamic_cast<const ImportStmt *>(stmt.get())) {
                generateStmt(stmt.get(), mainBody);
            }
        }
    }
};

} // namespace

std::string NativeCompiler::generateCpp(const std::vector<std::unique_ptr<Stmt>> &program,
                                       const std::string &) {
    CodeGenerator generator;
    generator.generateProgram(program);
    return generator.buildSource();
}

CompileResult NativeCompiler::compile(const std::vector<std::unique_ptr<Stmt>> &program,
                                      const CompileOptions &options,
                                      const std::string &sourceName) {
    auto startTime = std::chrono::high_resolution_clock::now();
    CompileResult result;

    try {
        std::string cppCode = generateCpp(program, sourceName);
        result.generatedCppCode = cppCode;

        std::filesystem::path cppPath;
        if (!options.outputPath.empty()) {
            cppPath = options.outputPath + ".cpp";
        } else {
            cppPath = std::filesystem::temp_directory_path() / (sourceName + ".cpp");
        }
        result.generatedCppPath = cppPath.string();

        std::ofstream out(cppPath);
        if (!out.is_open()) {
            result.errorMessage = "Failed to create temporary C++ file: " + cppPath.string();
            return result;
        }
        out << cppCode;
        out.close();

        std::filesystem::path binPath;
        if (!options.outputPath.empty()) {
            binPath = options.outputPath;
        } else {
            binPath = std::filesystem::temp_directory_path() / sourceName;
        }
        result.binaryPath = binPath.string();

        // Build clang++ command
        std::string cmd = "clang++ -std=c++20 ";
        if (options.optimize) {
            cmd += "-O3 ";
        }
        if (options.debug) {
            cmd += "-g ";
        }
        cmd += "-I. ";
        cmd += "\"" + cppPath.string() + "\" ";
        cmd += "-o \"" + binPath.string() + "\" 2>&1";

        if (options.verbose) {
            std::cout << "Executing: " << cmd << std::endl;
        }

        FILE *pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            result.errorMessage = "Failed to invoke clang++ compiler.";
            return result;
        }

        char buffer[256];
        std::string compilerOutput;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            compilerOutput += buffer;
        }
        int status = pclose(pipe);

        if (status != 0) {
            result.success = false;
            result.errorMessage = "clang++ compilation failed:\n" + compilerOutput;
            return result;
        }

        if (!options.keepCppSource && options.outputPath.empty()) {
            std::filesystem::remove(cppPath);
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        result.compileTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        result.success = true;

        if (options.runAfterCompile) {
            std::string runCmd = "\"" + binPath.string() + "\"";
            for (const auto &arg : options.runArgs) {
                runCmd += " \"" + arg + "\"";
            }
            result.exitCode = std::system(runCmd.c_str());
        }

        return result;
    } catch (const std::exception &e) {
        result.success = false;
        result.errorMessage = e.what();
        return result;
    }
}

CompileResult NativeCompiler::compileFile(const std::string &sourcePath,
                                         const CompileOptions &options) {
    CompileResult result;
    std::ifstream file(sourcePath);
    if (!file.is_open()) {
        result.errorMessage = "Cannot open source file: " + sourcePath;
        return result;
    }

    std::stringstream ss;
    ss << file.rdbuf();
    std::string source = ss.str();

    try {
        Lexer lexer(source);
        std::vector<Token> tokens = lexer.tokenize();

        Parser parser(tokens);
        std::vector<std::unique_ptr<Stmt>> program = parser.parseProgram();

        std::filesystem::path p(sourcePath);
        std::string stem = p.stem().string();

        return compile(program, options, stem);
    } catch (const std::exception &e) {
        result.success = false;
        result.errorMessage = e.what();
        return result;
    }
}

int NativeCompiler::runNative(const std::string &sourcePath,
                              const std::vector<std::string> &args,
                              bool verbose) {
    CompileOptions options;
    options.runAfterCompile = true;
    options.runArgs = args;
    options.optimize = true;
    options.verbose = verbose;

    CompileResult res = compileFile(sourcePath, options);
    if (!res.success) {
        std::cerr << "❌ Native Compilation Failed:\n" << res.errorMessage << std::endl;
        return 1;
    }
    return res.exitCode;
}
