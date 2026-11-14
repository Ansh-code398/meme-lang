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

    struct FinallyContext {
        std::string tryRetVar;
        std::string tryHasRetVar;
        std::string finRetVar;
        std::string finHasRetVar;
        std::string finallyLabel;
        std::string endLabel;
        bool inFinally = false;
    };
    std::vector<FinallyContext> finallyContexts;
    int finallyCounter = 0;
    bool isInsideFunction = false;

    struct FunctionScopeGuard {
        CodeGenerator &generator;
        bool prevInside;
        std::vector<FinallyContext> prevContexts;
        FunctionScopeGuard(CodeGenerator &g)
            : generator(g), prevInside(g.isInsideFunction), prevContexts(std::move(g.finallyContexts)) {
            generator.isInsideFunction = true;
            generator.finallyContexts.clear();
        }
        ~FunctionScopeGuard() {
            generator.isInsideFunction = prevInside;
            generator.finallyContexts = std::move(prevContexts);
        }
    };

    std::string indent() const {
        return std::string(indentLevel * 4, ' ');
    }

    std::string sanitizeIdentifier(const std::string &name) {
        if (name == "khud" || name == "apun" || name == "self" || name == "this") return "this";
        if (name == "nalla" || name == "null" || name == "nil") return "nullptr";
        if (name == "han" || name == "true") return "true";
        if (name == "nhi" || name == "false") return "false";
        if (name == "sach") return "true";
        if (name == "jhooth") return "false";

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

    struct PatternMatchCode {
        std::string condition = "true";
        std::vector<std::string> bindingDecls;
    };

    PatternMatchCode generatePatternMatch(const MemePattern *pattern, const std::string &target) {
        PatternMatchCode pmc;
        if (!pattern) return pmc;

        switch (pattern->getType()) {
        case PatternType::WILDCARD: {
            pmc.condition = "true";
            break;
        }
        case PatternType::LITERAL: {
            auto *lp = static_cast<const LiteralPattern *>(pattern);
            pmc.condition = "(" + target + " == " + generateExpr(lp->literalExpr.get()) + ")";
            break;
        }
        case PatternType::VARIABLE: {
            auto *vp = static_cast<const VariablePattern *>(pattern);
            if (vp->name != "_") {
                pmc.bindingDecls.push_back("auto " + sanitizeIdentifier(vp->name) + " = " + target + ";");
            }
            pmc.condition = "true";
            break;
        }
        case PatternType::VARIANT: {
            auto *vp = static_cast<const VariantPattern *>(pattern);
            std::string vName = vp->variantName;
            if (vName == "Some" || vName == "Kuch") {
                std::string cond = "(" + target + ".is_some())";
                if (!vp->subPatterns.empty()) {
                    std::string subTarget = "(" + target + ".unwrap())";
                    auto subPmc = generatePatternMatch(vp->subPatterns[0].get(), subTarget);
                    if (subPmc.condition != "true") cond = "(" + cond + " && " + subPmc.condition + ")";
                    pmc.bindingDecls.insert(pmc.bindingDecls.end(), subPmc.bindingDecls.begin(), subPmc.bindingDecls.end());
                }
                pmc.condition = cond;
            } else if (vName == "None" || vName == "Khali") {
                pmc.condition = "(" + target + ".is_none())";
            } else if (vName == "Ok" || vName == "Sahi") {
                std::string cond = "(" + target + ".is_ok())";
                if (!vp->subPatterns.empty()) {
                    std::string subTarget = "(" + target + ".unwrap())";
                    auto subPmc = generatePatternMatch(vp->subPatterns[0].get(), subTarget);
                    if (subPmc.condition != "true") cond = "(" + cond + " && " + subPmc.condition + ")";
                    pmc.bindingDecls.insert(pmc.bindingDecls.end(), subPmc.bindingDecls.begin(), subPmc.bindingDecls.end());
                }
                pmc.condition = cond;
            } else if (vName == "Err" || vName == "Galat") {
                std::string cond = "(" + target + ".is_err())";
                if (!vp->subPatterns.empty()) {
                    std::string subTarget = "(" + target + ".unwrap_err())";
                    auto subPmc = generatePatternMatch(vp->subPatterns[0].get(), subTarget);
                    if (subPmc.condition != "true") cond = "(" + cond + " && " + subPmc.condition + ")";
                    pmc.bindingDecls.insert(pmc.bindingDecls.end(), subPmc.bindingDecls.begin(), subPmc.bindingDecls.end());
                }
                pmc.condition = cond;
            } else {
                pmc.condition = "true";
            }
            break;
        }
        case PatternType::LIST: {
            auto *lp = static_cast<const ListPattern *>(pattern);
            size_t n = lp->elements.size();
            std::string cond = lp->hasRest ? ("(" + target + ".size() >= " + std::to_string(n) + "ULL)")
                                           : ("(" + target + ".size() == " + std::to_string(n) + "ULL)");
            for (size_t i = 0; i < n; ++i) {
                std::string elemTarget = "(" + target + "[" + std::to_string(i) + "])";
                auto elemPmc = generatePatternMatch(lp->elements[i].get(), elemTarget);
                if (elemPmc.condition != "true") cond = "(" + cond + " && " + elemPmc.condition + ")";
                pmc.bindingDecls.insert(pmc.bindingDecls.end(), elemPmc.bindingDecls.begin(), elemPmc.bindingDecls.end());
            }
            if (lp->hasRest && !lp->restName.empty() && lp->restName != "_") {
                std::string restDecl = "auto " + sanitizeIdentifier(lp->restName) + " = [&]() {\n"
                    "        std::vector<MemeValue> _rest_items;\n"
                    "        for (size_t _ri = " + std::to_string(n) + "; _ri < " + target + ".size(); ++_ri) {\n"
                    "            _rest_items.push_back(" + target + "[_ri]);\n"
                    "        }\n"
                    "        return MemeValue(_rest_items);\n"
                    "    }();";
                pmc.bindingDecls.push_back(restDecl);
            }
            pmc.condition = cond;
            break;
        }
        }
        return pmc;
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
                if (method == "length" || method == "lambai") {
                    method = "size";
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
                    objStr == "sys" || objStr == "systumm" || objStr == "os" || objStr == "file" ||
                    objStr == "env" || objStr == "paryavaran" || objStr == "dotenv" ||
                    method == "push_back" || method == "pop_back" || method == "size" || method == "length" || method == "lambai" ||
                    method == "is_ok" || method == "is_err" || method == "is_some" || method == "is_none" ||
                    method == "unwrap" || method == "unwrap_err" || method == "unwrap_or") {
                    return objStr + "." + method + "(" + args + ")";
                }
                // Map methods
                if (method == "keys" || method == "values" ||
                    method == "remove" || method == "get" ||
                    method == "chaabi" || method == "maal" || method == "hata" || method == "la") {
                    return objStr + "." + method + "(" + args + ")";
                }
                // String and container membership methods: wrap with MemeValue(...)
                static const std::unordered_set<std::string> strMethods = {
                    "charAt", "chars", "slice", "starts_with", "startsWith", "ends_with", "endsWith",
                    "find", "split", "replace", "trim", "to_upper", "to_lower",
                    "akshar", "akshar_list", "kaat", "shuru_hota", "khatam_hota", "dhund", "tod",
                    "badal", "saaf", "upper", "lower", "chilla", "dheere_bol", "substring", "indexOf",
                    "has", "contains", "hai_kya"
                };
                if (strMethods.count(method)) {
                    return "MemeValue(" + objStr + ")." + method + "(" + args + ")";
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
            if (objStr == "samay" || objStr == "itertools" || objStr == "math" ||
                objStr == "sys" || objStr == "systumm" || objStr == "os" || objStr == "file" ||
                objStr == "env" || objStr == "paryavaran" || objStr == "dotenv") {
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
            return "meme_index(" + generateExpr(e->target.get()) + ", " + generateExpr(e->index.get()) + ")";
        }
        if (auto *e = dynamic_cast<const IndexAssignExpr *>(expr)) {
            return "(meme_index(" + generateExpr(e->target.get()) + ", " + generateExpr(e->index.get()) + ") = " + generateExpr(e->value.get()) + ")";
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
                ss += " << meme_to_string_fmt(" + generateExpr(part.get()) + ")";
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
        if (auto *e = dynamic_cast<const MapLiteralExpr *>(expr)) {
            std::string entries;
            for (size_t i = 0; i < e->entries.size(); ++i) {
                if (i > 0) entries += ", ";
                entries += "{" + generateExpr(e->entries[i].first.get()) + ", " + generateExpr(e->entries[i].second.get()) + "}";
            }
            return "make_map({" + entries + "})";
        }
        if (auto *tpe = dynamic_cast<const TryPropagateExpr *>(expr)) {
            return "([&](auto &&_v) {\n"
                   "    if (_v.is_none() || _v.is_err()) throw MemeReturnSignal(_v);\n"
                   "    return _v.unwrap();\n"
                   "})(" + generateExpr(tpe->expression.get()) + ")";
        }
        if (auto *je = dynamic_cast<const JaanchExpr *>(expr)) {
            std::ostringstream ss;
            ss << "([&]() -> MemeValue {\n";
            int origLevel = indentLevel;
            indentLevel++;
            std::string targetVal = je->target ? generateExpr(je->target.get()) : "MemeValue()";
            ss << indent() << "auto _target = " << targetVal << ";\n";
            for (const auto &arm : je->arms) {
                PatternMatchCode pmc = generatePatternMatch(arm.pattern.get(), "_target");
                ss << indent() << "{\n";
                indentLevel++;
                if (!pmc.bindingDecls.empty()) {
                    if (pmc.condition != "true") {
                        ss << indent() << "if (" << pmc.condition << ") {\n";
                        indentLevel++;
                    }
                    for (const auto &decl : pmc.bindingDecls) ss << indent() << decl << "\n";
                    if (arm.guard) {
                        ss << indent() << "if (is_truthy(" << generateExpr(arm.guard.get()) << ")) {\n";
                        indentLevel++;
                    }
                    if (arm.bodyExpr) {
                        ss << indent() << "return " << generateExpr(arm.bodyExpr.get()) << ";\n";
                    } else if (arm.bodyStmt) {
                        std::ostringstream stmtOut;
                        generateStmt(arm.bodyStmt.get(), stmtOut);
                        ss << stmtOut.str();
                        ss << indent() << "return MemeValue();\n";
                    }
                    if (arm.guard) { indentLevel--; ss << indent() << "}\n"; }
                    if (pmc.condition != "true") { indentLevel--; ss << indent() << "}\n"; }
                } else {
                    std::string testCond = pmc.condition;
                    if (arm.guard) {
                        std::string gStr = "is_truthy(" + generateExpr(arm.guard.get()) + ")";
                        testCond = (testCond == "true") ? gStr : ("(" + testCond + " && " + gStr + ")");
                    }
                    if (testCond != "true") {
                        ss << indent() << "if (" << testCond << ") {\n";
                        indentLevel++;
                    }
                    if (arm.bodyExpr) {
                        ss << indent() << "return " << generateExpr(arm.bodyExpr.get()) << ";\n";
                    } else if (arm.bodyStmt) {
                        std::ostringstream stmtOut;
                        generateStmt(arm.bodyStmt.get(), stmtOut);
                        ss << stmtOut.str();
                        ss << indent() << "return MemeValue();\n";
                    }
                    if (testCond != "true") { indentLevel--; ss << indent() << "}\n"; }
                }
                indentLevel--;
                ss << indent() << "}\n";
            }
            if (je->defaultBodyExpr) {
                ss << indent() << "return " << generateExpr(je->defaultBodyExpr.get()) << ";\n";
            } else if (je->defaultBodyStmt) {
                std::ostringstream stmtOut;
                generateStmt(je->defaultBodyStmt.get(), stmtOut);
                ss << stmtOut.str();
                ss << indent() << "return MemeValue();\n";
            } else {
                ss << indent() << "return MemeValue();\n";
            }
            indentLevel = origLevel;
            ss << indent() << "})()";
            return ss.str();
        }

        return "MemeValue()";
    }

    bool hasReturnStmt(const Stmt *stmt) {
        if (!stmt) return false;
        if (dynamic_cast<const ReturnStmt *>(stmt)) return true;
        if (auto *b = dynamic_cast<const BlockStmt *>(stmt)) {
            for (const auto &s : b->statements) {
                if (hasReturnStmt(s.get())) return true;
            }
        }
        if (auto *i = dynamic_cast<const IfStmt *>(stmt)) {
            if (hasReturnStmt(i->thenBranch.get())) return true;
            if (i->elseBranch && hasReturnStmt(i->elseBranch.get())) return true;
        }
        if (auto *w = dynamic_cast<const WhileStmt *>(stmt)) {
            return hasReturnStmt(w->body.get());
        }
        if (auto *gm = dynamic_cast<const GhumaMeinStmt *>(stmt)) {
            return hasReturnStmt(gm->body.get());
        }
        if (auto *g = dynamic_cast<const GhoomStmt *>(stmt)) {
            return hasReturnStmt(g->body.get());
        }
        if (auto *tc = dynamic_cast<const TryCatchStmt *>(stmt)) {
            if (hasReturnStmt(tc->tryBlock.get())) return true;
            if (tc->catchBlock && hasReturnStmt(tc->catchBlock.get())) return true;
            if (tc->finallyBlock && hasReturnStmt(tc->finallyBlock.get())) return true;
        }
        return false;
    }

    bool hasFinallyWithReturn(const Stmt *stmt) {
        if (!stmt) return false;
        if (auto *tc = dynamic_cast<const TryCatchStmt *>(stmt)) {
            if (tc->finallyBlock && hasReturnStmt(tc->finallyBlock.get())) return true;
            if (hasFinallyWithReturn(tc->tryBlock.get())) return true;
            if (tc->catchBlock && hasFinallyWithReturn(tc->catchBlock.get())) return true;
            if (tc->finallyBlock && hasFinallyWithReturn(tc->finallyBlock.get())) return true;
        }
        if (auto *b = dynamic_cast<const BlockStmt *>(stmt)) {
            for (const auto &s : b->statements) {
                if (hasFinallyWithReturn(s.get())) return true;
            }
        }
        if (auto *i = dynamic_cast<const IfStmt *>(stmt)) {
            if (hasFinallyWithReturn(i->thenBranch.get())) return true;
            if (i->elseBranch && hasFinallyWithReturn(i->elseBranch.get())) return true;
        }
        if (auto *w = dynamic_cast<const WhileStmt *>(stmt)) {
            return hasFinallyWithReturn(w->body.get());
        }
        if (auto *gm = dynamic_cast<const GhumaMeinStmt *>(stmt)) {
            return hasFinallyWithReturn(gm->body.get());
        }
        if (auto *g = dynamic_cast<const GhoomStmt *>(stmt)) {
            return hasFinallyWithReturn(g->body.get());
        }
        return false;
    }

    void generateStmt(const Stmt *stmt, std::ostream &out) {
        if (!stmt) return;

        if (auto *v = dynamic_cast<const VarDeclStmt *>(stmt)) {
            std::string varName = sanitizeIdentifier(v->name);
            std::string typeStr = "auto";
            if (v->typeAnnotation) {
                std::string t = v->typeAnnotation->toString();
                if (t == "int" || t == "int64" || t == "ank") typeStr = "int64_t";
                else if (t == "float" || t == "float64" || t == "point_wala") typeStr = "double";
                else if (t == "bool" || t == "sahi_galat") typeStr = "bool";
                else if (t == "str" || t == "string" || t == "bol") typeStr = "std::string";
                else if (knownClasses.find(t) != knownClasses.end()) typeStr = "std::shared_ptr<" + t + ">";
            } else if (v->initializer && dynamic_cast<const NullExpr *>(v->initializer.get())) {
                typeStr = "MemeValue"; // Avoid deducing std::nullptr_t
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
            out << indent() << "for (int64_t " << var << " = " << start << "; " << var << (g->inclusive ? " <= " : " < ") << end << "; " << var << " += " << step << ") {\n";
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
            if (!finallyContexts.empty()) {
                auto &ctx = finallyContexts.back();
                if (!ctx.inFinally) {
                    if (r->value) {
                        out << indent() << ctx.tryRetVar << " = " << generateExpr(r->value.get()) << ";\n";
                    } else {
                        out << indent() << ctx.tryRetVar << " = MemeValue();\n";
                    }
                    out << indent() << ctx.tryHasRetVar << " = true;\n";
                    out << indent() << "goto " << ctx.finallyLabel << ";\n";
                    return;
                } else {
                    if (r->value) {
                        out << indent() << ctx.finRetVar << " = " << generateExpr(r->value.get()) << ";\n";
                    } else {
                        out << indent() << ctx.finRetVar << " = MemeValue();\n";
                    }
                    out << indent() << ctx.finHasRetVar << " = true;\n";
                    out << indent() << "goto " << ctx.endLabel << ";\n";
                    return;
                }
            }
            if (r->value) {
                out << indent() << "return " << generateExpr(r->value.get()) << ";\n";
            } else {
                if (isInsideFunction) {
                    out << indent() << "return MemeValue();\n";
                } else {
                    out << indent() << "return 0;\n";
                }
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

        if (auto *th = dynamic_cast<const ThrowStmt *>(stmt)) {
            if (th->expression) {
                out << indent() << "throw MemeValue(" << generateExpr(th->expression.get()) << ");\n";
            } else {
                out << indent() << "throw MemeValue(std::string(\"Error\"));\n";
            }
            return;
        }

        if (auto *tc = dynamic_cast<const TryCatchStmt *>(stmt)) {
            std::string cVar = tc->catchVar.empty() ? "err" : sanitizeIdentifier(tc->catchVar);
            std::string errTemp = "_e_" + std::to_string(stmt->line > 0 ? stmt->line : 1);

            bool hasFinally = (tc->finallyBlock != nullptr);
            bool finallyHasReturn = hasFinally && hasReturnStmt(tc->finallyBlock.get());

            if (finallyHasReturn) {
                std::string fid = std::to_string(++finallyCounter);
                FinallyContext ctx;
                ctx.tryRetVar = "_try_ret_val_" + fid;
                ctx.tryHasRetVar = "_try_has_ret_" + fid;
                ctx.finRetVar = "_fin_ret_val_" + fid;
                ctx.finHasRetVar = "_fin_has_ret_" + fid;
                ctx.finallyLabel = "_finally_label_" + fid;
                ctx.endLabel = "_end_finally_" + fid;
                ctx.inFinally = false;

                std::string pendingEx = "_pending_ex_" + fid;

                out << indent() << "{\n";
                indentLevel++;
                out << indent() << "bool " << ctx.tryHasRetVar << " = false;\n";
                out << indent() << "MemeValue " << ctx.tryRetVar << ";\n";
                out << indent() << "bool " << ctx.finHasRetVar << " = false;\n";
                out << indent() << "MemeValue " << ctx.finRetVar << ";\n";
                out << indent() << "std::exception_ptr " << pendingEx << " = nullptr;\n";

                finallyContexts.push_back(ctx);

                out << indent() << "try {\n";
                indentLevel++;
                if (auto *tb = dynamic_cast<const BlockStmt *>(tc->tryBlock.get())) {
                    for (const auto &s : tb->statements) generateStmt(s.get(), out);
                } else {
                    generateStmt(tc->tryBlock.get(), out);
                }
                indentLevel--;
                if (tc->catchBlock) {
                    out << indent() << "} catch (const MemeValue &" << errTemp << ") {\n";
                    indentLevel++;
                    out << indent() << "MemeValue " << cVar << " = " << errTemp << ";\n";
                    out << indent() << "try {\n";
                    indentLevel++;
                    if (auto *cb = dynamic_cast<const BlockStmt *>(tc->catchBlock.get())) {
                        for (const auto &s : cb->statements) generateStmt(s.get(), out);
                    } else {
                        generateStmt(tc->catchBlock.get(), out);
                    }
                    indentLevel--;
                    out << indent() << "} catch (...) {\n";
                    indentLevel++;
                    out << indent() << pendingEx << " = std::current_exception();\n";
                    out << indent() << "goto " << ctx.finallyLabel << ";\n";
                    indentLevel--;
                    out << indent() << "}\n";
                    indentLevel--;

                    out << indent() << "} catch (const std::exception &" << errTemp << ") {\n";
                    indentLevel++;
                    out << indent() << "MemeValue " << cVar << " = MemeValue(std::string(" << errTemp << ".what()));\n";
                    out << indent() << "try {\n";
                    indentLevel++;
                    if (auto *cb = dynamic_cast<const BlockStmt *>(tc->catchBlock.get())) {
                        for (const auto &s : cb->statements) generateStmt(s.get(), out);
                    } else {
                        generateStmt(tc->catchBlock.get(), out);
                    }
                    indentLevel--;
                    out << indent() << "} catch (...) {\n";
                    indentLevel++;
                    out << indent() << pendingEx << " = std::current_exception();\n";
                    out << indent() << "goto " << ctx.finallyLabel << ";\n";
                    indentLevel--;
                    out << indent() << "}\n";
                    indentLevel--;

                    out << indent() << "} catch (...) {\n";
                    indentLevel++;
                    out << indent() << pendingEx << " = std::current_exception();\n";
                    indentLevel--;
                    out << indent() << "}\n";
                } else {
                    out << indent() << "} catch (...) {\n";
                    indentLevel++;
                    out << indent() << pendingEx << " = std::current_exception();\n";
                    indentLevel--;
                    out << indent() << "}\n";
                }

                // Generate finally block
                out << indent() << ctx.finallyLabel << ":\n";
                out << indent() << "{\n";
                indentLevel++;
                finallyContexts.back().inFinally = true;
                if (auto *fb = dynamic_cast<const BlockStmt *>(tc->finallyBlock.get())) {
                    for (const auto &s : fb->statements) generateStmt(s.get(), out);
                } else {
                    generateStmt(tc->finallyBlock.get(), out);
                }
                indentLevel--;
                out << indent() << "}\n";

                finallyContexts.pop_back();

                out << indent() << ctx.endLabel << ":\n";
                out << indent() << "if (" << ctx.finHasRetVar << ") {\n";
                if (!finallyContexts.empty()) {
                    const auto &outer = finallyContexts.back();
                    out << indent() << "    " << outer.tryRetVar << " = " << ctx.finRetVar << ";\n";
                    out << indent() << "    " << outer.tryHasRetVar << " = true;\n";
                    out << indent() << "    goto " << outer.finallyLabel << ";\n";
                } else {
                    if (isInsideFunction) {
                        out << indent() << "    return " << ctx.finRetVar << ";\n";
                    } else {
                        out << indent() << "    return static_cast<int>(" << ctx.finRetVar << ".as_int());\n";
                    }
                }
                out << indent() << "}\n";
                out << indent() << "if (" << pendingEx << ") {\n";
                out << indent() << "    std::rethrow_exception(" << pendingEx << ");\n";
                out << indent() << "}\n";
                out << indent() << "if (" << ctx.tryHasRetVar << ") {\n";
                if (!finallyContexts.empty()) {
                    const auto &outer = finallyContexts.back();
                    out << indent() << "    " << outer.tryRetVar << " = " << ctx.tryRetVar << ";\n";
                    out << indent() << "    " << outer.tryHasRetVar << " = true;\n";
                    out << indent() << "    goto " << outer.finallyLabel << ";\n";
                } else {
                    if (isInsideFunction) {
                        out << indent() << "    return " << ctx.tryRetVar << ";\n";
                    } else {
                        out << indent() << "    return static_cast<int>(" << ctx.tryRetVar << ".as_int());\n";
                    }
                }
                out << indent() << "}\n";

                indentLevel--;
                out << indent() << "}\n";
                return;
            }

            out << indent() << "{\n";
            indentLevel++;
            if (tc->finallyBlock) {
                out << indent() << "auto _finally_guard = make_scope_guard([&]() {\n";
                indentLevel++;
                if (auto *fb = dynamic_cast<const BlockStmt *>(tc->finallyBlock.get())) {
                    for (const auto &s : fb->statements) generateStmt(s.get(), out);
                } else {
                    generateStmt(tc->finallyBlock.get(), out);
                }
                indentLevel--;
                out << indent() << "});\n";
            }
            out << indent() << "try {\n";
            indentLevel++;
            if (auto *tb = dynamic_cast<const BlockStmt *>(tc->tryBlock.get())) {
                for (const auto &s : tb->statements) generateStmt(s.get(), out);
            } else {
                generateStmt(tc->tryBlock.get(), out);
            }
            indentLevel--;
            out << indent() << "} catch (const MemeValue &" << errTemp << ") {\n";
            indentLevel++;
            out << indent() << "MemeValue " << cVar << " = " << errTemp << ";\n";
            if (tc->catchBlock) {
                if (auto *cb = dynamic_cast<const BlockStmt *>(tc->catchBlock.get())) {
                    for (const auto &s : cb->statements) generateStmt(s.get(), out);
                } else {
                    generateStmt(tc->catchBlock.get(), out);
                }
            }
            indentLevel--;
            out << indent() << "} catch (const std::exception &" << errTemp << ") {\n";
            indentLevel++;
            out << indent() << "MemeValue " << cVar << " = MemeValue(std::string(" << errTemp << ".what()));\n";
            if (tc->catchBlock) {
                if (auto *cb = dynamic_cast<const BlockStmt *>(tc->catchBlock.get())) {
                    for (const auto &s : cb->statements) generateStmt(s.get(), out);
                } else {
                    generateStmt(tc->catchBlock.get(), out);
                }
            }
            indentLevel--;
            out << indent() << "}\n";
            indentLevel--;
            out << indent() << "}\n";
            return;
        }

        if (auto *js = dynamic_cast<const JaanchStmt *>(stmt)) {
            std::string targetVal = js->target ? generateExpr(js->target.get()) : "MemeValue()";
            out << indent() << "{\n";
            indentLevel++;
            out << indent() << "auto _target = " << targetVal << ";\n";
            out << indent() << "bool _matched = false;\n";
            for (const auto &arm : js->arms) {
                PatternMatchCode pmc = generatePatternMatch(arm.pattern.get(), "_target");
                out << indent() << "if (!_matched) {\n";
                indentLevel++;
                if (!pmc.bindingDecls.empty()) {
                    if (pmc.condition != "true") {
                        out << indent() << "if (" << pmc.condition << ") {\n";
                        indentLevel++;
                    }
                    for (const auto &decl : pmc.bindingDecls) out << indent() << decl << "\n";
                    if (arm.guard) {
                        out << indent() << "if (is_truthy(" << generateExpr(arm.guard.get()) << ")) {\n";
                        indentLevel++;
                    }
                    out << indent() << "_matched = true;\n";
                    if (arm.bodyStmt) generateStmt(arm.bodyStmt.get(), out);
                    else if (arm.bodyExpr) out << indent() << generateExpr(arm.bodyExpr.get()) << ";\n";
                    if (arm.guard) { indentLevel--; out << indent() << "}\n"; }
                    if (pmc.condition != "true") { indentLevel--; out << indent() << "}\n"; }
                } else {
                    std::string testCond = pmc.condition;
                    if (arm.guard) {
                        std::string gStr = "is_truthy(" + generateExpr(arm.guard.get()) + ")";
                        testCond = (testCond == "true") ? gStr : ("(" + testCond + " && " + gStr + ")");
                    }
                    if (testCond != "true") {
                        out << indent() << "if (" << testCond << ") {\n";
                        indentLevel++;
                    }
                    out << indent() << "_matched = true;\n";
                    if (arm.bodyStmt) generateStmt(arm.bodyStmt.get(), out);
                    else if (arm.bodyExpr) out << indent() << generateExpr(arm.bodyExpr.get()) << ";\n";
                    if (testCond != "true") { indentLevel--; out << indent() << "}\n"; }
                }
                indentLevel--;
                out << indent() << "}\n";
            }
            if (js->defaultArm) {
                out << indent() << "if (!_matched) {\n";
                indentLevel++;
                generateStmt(js->defaultArm.get(), out);
                indentLevel--;
                out << indent() << "}\n";
            }
            indentLevel--;
            out << indent() << "}\n";
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
            } else if (imp->path == "sys" || imp->path == "systumm" || imp->path == "os") {
                if (alias != "sys" && alias != "systumm" && alias != "os") {
                    out << indent() << "auto &" << alias << " = sys;\n";
                }
            } else if (imp->path == "env" || imp->path == "paryavaran" || imp->path == "dotenv") {
                if (alias != "env" && alias != "paryavaran" && alias != "dotenv") {
                    out << indent() << "auto &" << alias << " = env;\n";
                }
            } else if (imp->path == "file") {
                if (alias != "file") {
                    out << indent() << "auto &" << alias << " = file;\n";
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

    bool hasTryPropagate(const Expr *e) {
        if (!e) return false;
        if (dynamic_cast<const TryPropagateExpr *>(e)) return true;
        if (auto *u = dynamic_cast<const UnaryExpr *>(e)) return hasTryPropagate(u->operand.get());
        if (auto *b = dynamic_cast<const BinaryExpr *>(e)) return hasTryPropagate(b->left.get()) || hasTryPropagate(b->right.get());
        if (auto *c = dynamic_cast<const CallExpr *>(e)) {
            if (hasTryPropagate(c->callee.get())) return true;
            for (const auto &arg : c->args) if (hasTryPropagate(arg.get())) return true;
        }
        if (auto *a = dynamic_cast<const AssignExpr *>(e)) return hasTryPropagate(a->value.get());
        if (auto *t = dynamic_cast<const TernaryExpr *>(e)) {
            return hasTryPropagate(t->condition.get()) || hasTryPropagate(t->thenBranch.get()) || hasTryPropagate(t->elseBranch.get());
        }
        return false;
    }

    bool hasTryPropagate(const Stmt *s) {
        if (!s) return false;
        if (auto *es = dynamic_cast<const ExprStmt *>(s)) return hasTryPropagate(es->expression.get());
        if (auto *vs = dynamic_cast<const VarDeclStmt *>(s)) return vs->initializer && hasTryPropagate(vs->initializer.get());
        if (auto *rs = dynamic_cast<const ReturnStmt *>(s)) return rs->value && hasTryPropagate(rs->value.get());
        if (auto *bs = dynamic_cast<const BlockStmt *>(s)) {
            for (const auto &stmt : bs->statements) if (hasTryPropagate(stmt.get())) return true;
        }
        if (auto *is = dynamic_cast<const IfStmt *>(s)) {
            if (hasTryPropagate(is->condition.get()) || hasTryPropagate(is->thenBranch.get())) return true;
            if (is->elseBranch && hasTryPropagate(is->elseBranch.get())) return true;
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

        bool hasZeroArgCtor = false;
        for (const auto &m : c->methods) {
            if ((m->name == "shuru" || m->name == "init" || m->name == "constructor") && m->paramList.empty()) {
                hasZeroArgCtor = true;
                break;
            }
        }
        if (!hasZeroArgCtor) {
            globalDecls << "\n    " << className << "() = default;\n";
        }

        // Constructor & Methods
        for (const auto &m : c->methods) {
            if (m->name == "shuru" || m->name == "init" || m->name == "constructor") {
                globalDecls << "    " << className << "(";
                for (size_t i = 0; i < m->paramList.size(); ++i) {
                    if (i > 0) globalDecls << ", ";
                    globalDecls << "auto " << sanitizeIdentifier(m->paramList[i].name);
                }
                globalDecls << ") {\n";
                indentLevel = 2;
                {
                    FunctionScopeGuard guard(*this);
                    if (auto *b = dynamic_cast<const BlockStmt *>(m->body.get())) {
                        for (const auto &s : b->statements) {
                            generateStmt(s.get(), globalDecls);
                        }
                    } else {
                        generateStmt(m->body.get(), globalDecls);
                    }
                }
                indentLevel = 1;
                globalDecls << "    }\n";
            } else if (m->name == "alvida" || m->name == "drop") {
                globalDecls << "    ~" << className << "() {\n";
                indentLevel = 2;
                {
                    FunctionScopeGuard guard(*this);
                    if (auto *b = dynamic_cast<const BlockStmt *>(m->body.get())) {
                        for (const auto &s : b->statements) {
                            generateStmt(s.get(), globalDecls);
                        }
                    } else {
                        generateStmt(m->body.get(), globalDecls);
                    }
                }
                indentLevel = 1;
                globalDecls << "    }\n";
            } else {
                // Ordinary method
                std::string methodRetType = "MemeValue";
                bool usesPropagate = hasTryPropagate(m->body.get());
                std::string classRet = findClassReturned(m->body.get());
                if (!classRet.empty()) {
                    methodRetType = "std::shared_ptr<" + classRet + ">";
                } else if (m->returnTypeAnnotation) {
                    std::string t = m->returnTypeAnnotation->toString();
                    if (t == "int" || t == "int64" || t == "ank") methodRetType = "int64_t";
                    else if (t == "float" || t == "float64" || t == "point_wala") methodRetType = "double";
                    else if (t == "bool" || t == "sahi_galat") methodRetType = "bool";
                    else if (t == "str" || t == "string" || t == "bol") methodRetType = "std::string";
                    else if (knownClasses.find(t) != knownClasses.end()) methodRetType = "std::shared_ptr<" + t + ">";
                } else if (hasGetExprInReturn(m->body.get()) || usesPropagate || hasFinallyWithReturn(m->body.get())) {
                    methodRetType = "MemeValue";
                }

                globalDecls << "    " << methodRetType << " " << sanitizeIdentifier(m->name) << "(";
                for (size_t i = 0; i < m->paramList.size(); ++i) {
                    if (i > 0) globalDecls << ", ";
                    globalDecls << "MemeValue " << sanitizeIdentifier(m->paramList[i].name);
                }
                globalDecls << ") {\n";
                indentLevel = 2;
                {
                    FunctionScopeGuard guard(*this);
                    if (usesPropagate) {
                        globalDecls << indent() << "try {\n";
                        indentLevel++;
                    }
                    if (auto *b = dynamic_cast<const BlockStmt *>(m->body.get())) {
                        for (const auto &s : b->statements) {
                            generateStmt(s.get(), globalDecls);
                        }
                    } else {
                        generateStmt(m->body.get(), globalDecls);
                    }
                    if (usesPropagate) {
                        indentLevel--;
                        globalDecls << indent() << "} catch (const MemeReturnSignal &_sig) {\n";
                        globalDecls << indent() << "    return *(_sig.value);\n";
                        globalDecls << indent() << "}\n";
                        globalDecls << indent() << "return MemeValue();\n";
                    } else if (methodRetType == "MemeValue") {
                        globalDecls << indent() << "return MemeValue();\n";
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
        std::string returnType = "MemeValue";
        bool usesPropagate = hasTryPropagate(f->body.get());

        std::string classRet = findClassReturned(f->body.get());
        if (!classRet.empty()) {
            returnType = "std::shared_ptr<" + classRet + ">";
        } else if (f->returnTypeAnnotation) {
            std::string t = f->returnTypeAnnotation->toString();
            if (t == "int" || t == "int64" || t == "ank") returnType = "int64_t";
            else if (t == "float" || t == "float64" || t == "point_wala") returnType = "double";
            else if (t == "bool" || t == "sahi_galat") returnType = "bool";
            else if (t == "str" || t == "string" || t == "bol") returnType = "std::string";
            else if (knownClasses.find(t) != knownClasses.end()) returnType = "std::shared_ptr<" + t + ">";
        } else if (hasGetExprInReturn(f->body.get()) || usesPropagate || hasFinallyWithReturn(f->body.get())) {
            returnType = "MemeValue";
        }

        std::string paramDecl;
        for (size_t i = 0; i < f->paramList.size(); ++i) {
            if (i > 0) paramDecl += ", ";
            paramDecl += "const MemeValue &" + sanitizeIdentifier(f->paramList[i].name);
        }

        globalDecls << "inline " << returnType << " " << fnName << "(" << paramDecl << ") {\n";
        indentLevel = 1;
        {
            FunctionScopeGuard guard(*this);
            if (usesPropagate) {
                globalDecls << indent() << "try {\n";
                indentLevel++;
            }
            if (auto *b = dynamic_cast<const BlockStmt *>(f->body.get())) {
                for (const auto &s : b->statements) {
                    generateStmt(s.get(), globalDecls);
                }
            } else {
                generateStmt(f->body.get(), globalDecls);
            }
            if (usesPropagate) {
                indentLevel--;
                globalDecls << indent() << "} catch (const MemeReturnSignal &_sig) {\n";
                globalDecls << indent() << "    return *(_sig.value);\n";
                globalDecls << indent() << "}\n";
                globalDecls << indent() << "return MemeValue();\n";
            } else if (returnType == "MemeValue") {
                globalDecls << indent() << "return MemeValue();\n";
            }
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
        out << "    sys.args.clear();\n";
        out << "    for (int _i = 0; _i < argc; ++_i) {\n";
        out << "        sys.args.push_back(std::string(argv[_i]));\n";
        out << "    }\n";
        out << "    env.auto_load();\n\n";
        out << "    try {\n";
        out << mainBody.str();
        out << "\n    } catch (const MemeReturnSignal &_sig) {\n";
        out << "        return 0;\n";
        out << "    }\n";
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
                } else if (imp->path == "sys" || imp->path == "systumm" || imp->path == "os") {
                    if (alias != "sys" && alias != "systumm" && alias != "os") {
                        globalDecls << "inline SystummModule &" << alias << " = sys;\n";
                    }
                } else if (imp->path == "env" || imp->path == "paryavaran" || imp->path == "dotenv") {
                    if (alias != "env" && alias != "paryavaran" && alias != "dotenv") {
                        globalDecls << "inline EnvModule &" << alias << " = env;\n";
                    }
                } else if (imp->path == "file") {
                    if (alias != "file") {
                        globalDecls << "inline FileModule &" << alias << " = file;\n";
                    }
                }
            }
        }

        // Pass 0A: Forward declarations for all classes
        for (const auto &stmt : program) {
            if (auto *c = dynamic_cast<const ClassDeclStmt *>(stmt.get())) {
                globalDecls << "struct " << sanitizeIdentifier(c->name) << ";\n";
            }
        }
        globalDecls << "\n";

        // Pass 0B: Forward declarations for all top-level functions
        for (const auto &stmt : program) {
            if (auto *f = dynamic_cast<const FuncDeclStmt *>(stmt.get())) {
                std::string fnName = sanitizeIdentifier(f->name);
                std::string retType = "MemeValue";
                std::string classRet = findClassReturned(f->body.get());
                if (!classRet.empty()) {
                    retType = "std::shared_ptr<" + classRet + ">";
                } else if (f->returnTypeAnnotation) {
                    std::string t = f->returnTypeAnnotation->toString();
                    if (t == "int" || t == "int64" || t == "ank") retType = "int64_t";
                    else if (t == "float" || t == "float64" || t == "point_wala") retType = "double";
                    else if (t == "bool" || t == "sahi_galat") retType = "bool";
                    else if (t == "str" || t == "string" || t == "bol") retType = "std::string";
                    else if (knownClasses.find(t) != knownClasses.end()) retType = "std::shared_ptr<" + t + ">";
                }
                std::string paramTypes;
                for (size_t i = 0; i < f->paramList.size(); ++i) {
                    if (i > 0) paramTypes += ", ";
                    paramTypes += "const MemeValue &" + sanitizeIdentifier(f->paramList[i].name);
                }
                globalDecls << "inline " << retType << " " << fnName << "(" << paramTypes << ");\n";
            }
        }
        globalDecls << "\n";

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
