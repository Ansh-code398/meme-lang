#include "NativeCompiler.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <cstdlib>
#include <unordered_set>
#include "../lexer/Lexer.h"
#include "../types/TypeChecker.h"
#include "../platform/MemePlatform.h"

namespace {

std::filesystem::path getPchDirectory() {
    std::filesystem::path home = "";
    if (const char *h = std::getenv("HOME")) {
        home = h;
    } else {
        home = std::filesystem::temp_directory_path();
    }
    std::filesystem::path dir = home / ".meme" / "pch";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) {
        dir = std::filesystem::temp_directory_path() / "meme_pch";
        std::filesystem::create_directories(dir, ec);
    }
    return dir;
}

std::filesystem::path getRuntimeHeaderPath() {
    std::vector<std::filesystem::path> candidates;
    candidates.push_back(std::filesystem::current_path() / "src" / "compiler" / "MemeRuntime.h");
    candidates.push_back(std::filesystem::current_path() / "MemeRuntime.h");

    if (const char *rt = std::getenv("MEME_RUNTIME_DIR")) {
        candidates.push_back(std::filesystem::path(rt) / "MemeRuntime.h");
        candidates.push_back(std::filesystem::path(rt) / "src" / "compiler" / "MemeRuntime.h");
    }
    if (const char *mh = std::getenv("MEME_HOME")) {
        candidates.push_back(std::filesystem::path(mh) / "src" / "compiler" / "MemeRuntime.h");
    }

    candidates.push_back("/usr/local/include/meme/MemeRuntime.h");
    candidates.push_back("/usr/include/meme/MemeRuntime.h");

    for (const auto &p : candidates) {
        std::error_code ec;
        if (std::filesystem::exists(p, ec)) {
            return std::filesystem::canonical(p, ec);
        }
    }
    return "";
}

std::filesystem::path resolveRepoSourceFile(const std::string &relPath) {
    std::vector<std::filesystem::path> candidates;
    candidates.push_back(std::filesystem::current_path() / relPath);
    if (const char *mh = std::getenv("MEME_HOME")) {
        candidates.push_back(std::filesystem::path(mh) / relPath);
    }
    if (const char *rt = std::getenv("MEME_RUNTIME_DIR")) {
        candidates.push_back(std::filesystem::path(rt) / relPath);
    }
    for (const auto &p : candidates) {
        std::error_code ec;
        if (std::filesystem::exists(p, ec)) {
            return std::filesystem::canonical(p, ec);
        }
    }
    return std::filesystem::path(relPath);
}

bool ensurePchReady(const CompileOptions &options, std::filesystem::path &outPchPath) {
    if (!options.usePch) return false;

    auto headerPath = getRuntimeHeaderPath();
    if (headerPath.empty()) return false;

    std::string optSuffix = options.optimize ? "_O3" : "_O0";
    outPchPath = getPchDirectory() / ("MemeRuntime" + optSuffix + ".h.pch");

    std::error_code ec;
    bool needsBuild = !std::filesystem::exists(outPchPath, ec);
    if (!needsBuild) {
        auto headerMtime = std::filesystem::last_write_time(headerPath, ec);
        auto pchMtime = std::filesystem::last_write_time(outPchPath, ec);
        if (headerMtime > pchMtime) {
            needsBuild = true;
        }
    }

    if (needsBuild) {
        std::string optFlag = options.optimize ? "-O3 -DNDEBUG -DMEME_NO_DEPTH_GUARD" : "-O0";
        std::string pchCmd = "clang++ -std=c++20 " + optFlag + " -I. -x c++-header \"" +
                             headerPath.string() + "\" -o \"" + outPchPath.string() + "\" 2>&1";
        FILE *pipe = popen(pchCmd.c_str(), "r");
        if (!pipe) return false;
        char buf[256];
        while (fgets(buf, sizeof(buf), pipe) != nullptr) {}
        int status = pclose(pipe);
        if (status != 0) {
            std::filesystem::remove(outPchPath, ec);
            return false;
        }
    }
    return std::filesystem::exists(outPchPath, ec);
}

class CodeGenerator {
private:
    std::ostringstream globalDecls;
    std::ostringstream mainBody;
    std::string currentSourceFile;
    std::string targetPlatform;
    bool explicitTargetPlatform = false;
    int indentLevel = 1;
    std::unordered_set<std::string> knownClasses;
    std::unordered_set<std::string> knownFunctions;
    std::unordered_set<std::string> knownClassFields;
    std::unordered_set<std::string> knownClassMethods;
    std::unordered_set<std::string> topLevelVars;
    std::vector<std::unordered_set<std::string>> scopeDeclaredVars;
    std::unordered_map<std::string, std::string> renamedImports;
    std::unordered_set<std::string> emittedPass0NamedImports;
    bool programUsesUi = false;
    bool programUsesEval = false;
    bool programUsesJsBridge = false;
    bool inMainPass = false;
    int tempCounter = 0;

    void pushVarScope() { scopeDeclaredVars.emplace_back(); }
    void popVarScope() { if (!scopeDeclaredVars.empty()) scopeDeclaredVars.pop_back(); }
    void declareScopeVar(const std::string &n) {
        if (!scopeDeclaredVars.empty()) scopeDeclaredVars.back().insert(n);
    }
    bool isVarDeclaredInScope(const std::string &n) const {
        for (auto it = scopeDeclaredVars.rbegin(); it != scopeDeclaredVars.rend(); ++it) {
            if (it->count(n)) return true;
        }
        return false;
    }
    static bool isCollidingGlobalSymbol(const std::string &n) {
        static const std::unordered_set<std::string> kColl = {
            "os", "select", "sys", "systumm", "env", "file", "json", "csv",
            "net", "socket", "math", "samay", "itertools", "app", "native",
            "platform", "permissions", "perms", "ui", "view", "gui", "egui",
            "bind", "connect", "accept", "close", "read", "write", "time"
        };
        return kColl.count(n) > 0;
    }
    std::string resolveVarOrImportName(const std::string &n) {
        if (!isVarDeclaredInScope(n) && !topLevelVars.count(n)) {
            auto it = renamedImports.find(n);
            if (it != renamedImports.end()) return it->second;
        }
        return sanitizeIdentifier(n);
    }
    std::unordered_set<std::string> knownModuleAliases = {
        "samay", "itertools", "math", "sys", "systumm", "os", "file", "fs",
        "env", "paryavaran", "dotenv", "json", "csv", "dhaga", "thread",
        "net", "socket", "m_socket"
    };
    std::unordered_set<std::string> currentClassTypedFields;
    std::unordered_map<std::string, std::shared_ptr<Type>> currentClassFieldTypes;

    std::string toCppType(const std::shared_ptr<Type> &type, const std::vector<std::string> &typeParams = {}, bool isParam = false) const {
        if (!type || type->kind == TypeKind::ANY) {
            return isParam ? "const MemeValue &" : "MemeValue";
        }
        std::string raw;
        bool isRefParam = true;

        switch (type->kind) {
        case TypeKind::ANK:
            raw = "int64_t";
            isRefParam = false;
            break;
        case TypeKind::INT32:
            raw = "int32_t";
            isRefParam = false;
            break;
        case TypeKind::BYTE:
            raw = "uint8_t";
            isRefParam = false;
            break;
        case TypeKind::POINT:
            raw = "double";
            isRefParam = false;
            break;
        case TypeKind::FLOAT32:
            raw = "float";
            isRefParam = false;
            break;
        case TypeKind::SACH:
            raw = "bool";
            isRefParam = false;
            break;
        case TypeKind::BOL:
            raw = "std::string";
            isRefParam = true;
            break;
        case TypeKind::NALLA:
            return "void";
        case TypeKind::CLASS: {
            for (const auto &tp : typeParams) {
                if (tp == type->name) {
                    return isParam ? ("const " + tp + " &") : tp;
                }
            }
            if (knownClasses.find(type->name) != knownClasses.end()) {
                raw = "std::shared_ptr<" + type->name + ">";
                isRefParam = true;
            } else {
                std::string t = type->name;
                if (t == "int" || t == "int64" || t == "ank") { raw = "int64_t"; isRefParam = false; }
                else if (t == "float" || t == "float64" || t == "point" || t == "point_wala") { raw = "double"; isRefParam = false; }
                else if (t == "bool" || t == "sach" || t == "sahi_galat") { raw = "bool"; isRefParam = false; }
                else if (t == "str" || t == "string" || t == "bol") { raw = "std::string"; isRefParam = true; }
                else { raw = "MemeValue"; isRefParam = true; }
            }
            break;
        }
        case TypeKind::POINTER: {
            std::string pointeeCls;
            auto pointee = type->elementType;
            if (pointee) {
                if (pointee->kind == TypeKind::CLASS && knownClasses.find(pointee->name) != knownClasses.end()) {
                    pointeeCls = pointee->name;
                } else {
                    pointeeCls = toCppType(pointee, typeParams, false);
                }
            }
            if (pointeeCls.empty() || pointeeCls == "MemeValue") {
                raw = type->isMut ? "MemeValue*" : "const MemeValue*";
            } else {
                raw = type->isMut ? (pointeeCls + "*") : ("const " + pointeeCls + "*");
            }
            isRefParam = false;
            break;
        }
        case TypeKind::LIST: {
            if (type->elementType) {
                std::string elemCpp = toCppType(type->elementType, typeParams, false);
                if (elemCpp != "MemeValue") {
                    raw = "std::vector<" + elemCpp + ">";
                } else if (knownClasses.find(type->elementType->name) != knownClasses.end()) {
                    raw = "std::vector<std::shared_ptr<" + type->elementType->name + ">>";
                } else {
                    raw = "std::vector<MemeValue>";
                }
            } else {
                raw = "std::vector<MemeValue>";
            }
            isRefParam = true;
            break;
        }
        case TypeKind::MAP: {
            if (type->keyType && type->valueType) {
                std::string kCpp = toCppType(type->keyType, typeParams, false);
                std::string vCpp = toCppType(type->valueType, typeParams, false);
                if (kCpp == "std::string") {
                    raw = "std::unordered_map<std::string, " + vCpp + ", StringHash, StringEqual>";
                } else {
                    raw = "std::unordered_map<" + kCpp + ", " + vCpp + ">";
                }
            } else {
                raw = "MemeValue";
            }
            isRefParam = true;
            break;
        }
        default:
            raw = "MemeValue";
            isRefParam = true;
            break;
        }

        if (isParam && isRefParam) {
            return "const " + raw + " &";
        }
        return raw;
    }

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
    const Stmt *currentScopeBody = nullptr;
    const std::vector<std::unique_ptr<Stmt>> *currentProgram = nullptr;

    struct FunctionScopeGuard {
        CodeGenerator &generator;
        bool prevInside;
        std::vector<FinallyContext> prevContexts;
        const Stmt *prevScopeBody;
        FunctionScopeGuard(CodeGenerator &g, const Stmt *body = nullptr)
            : generator(g), prevInside(g.isInsideFunction), prevContexts(std::move(g.finallyContexts)), prevScopeBody(g.currentScopeBody) {
            generator.isInsideFunction = true;
            generator.finallyContexts.clear();
            if (body) generator.currentScopeBody = body;
        }
        ~FunctionScopeGuard() {
            generator.isInsideFunction = prevInside;
            generator.finallyContexts = std::move(prevContexts);
            generator.currentScopeBody = prevScopeBody;
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
        if (name == "main") return "meme_user_main";

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
            "xor", "xor_eq", "socket"
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
    explicit CodeGenerator(const std::string &sourceFile = "", const std::string &targetPlat = "")
        : currentSourceFile(sourceFile),
          targetPlatform(targetPlat.empty() ? MemePlatform::currentPlatform() : MemePlatform::normalizePlatformName(targetPlat)),
          explicitTargetPlatform(!targetPlat.empty() && MemePlatform::isKnownConcretePlatform(MemePlatform::normalizePlatformName(targetPlat))) {}

    bool detectUiInExpr(const Expr *expr) const {
        if (!expr) return false;
        if (dynamic_cast<const JsxElementExpr *>(expr)) return true;
        if (auto *id = dynamic_cast<const IdentifierExpr *>(expr)) {
            static const std::unordered_set<std::string> uiSyms = {
                "ui", "egui", "view", "gui", "useState", "state_bana", "useEffect",
                "useMemo", "useCallback", "useReducer", "useRef", "createContext",
                "useContext", "useRouter", "Router", "Route", "Link"
            };
            if (uiSyms.count(id->name)) return true;
        }
        if (auto *u = dynamic_cast<const UnaryExpr *>(expr)) return detectUiInExpr(u->operand.get());
        if (auto *b = dynamic_cast<const BinaryExpr *>(expr)) return detectUiInExpr(b->left.get()) || detectUiInExpr(b->right.get());
        if (auto *c = dynamic_cast<const CallExpr *>(expr)) {
            if (detectUiInExpr(c->callee.get())) return true;
            for (const auto &a : c->args) if (detectUiInExpr(a.get())) return true;
        }
        if (auto *g = dynamic_cast<const GetExpr *>(expr)) return detectUiInExpr(g->object.get());
        if (auto *a = dynamic_cast<const AssignExpr *>(expr)) return detectUiInExpr(a->value.get());
        if (auto *da = dynamic_cast<const DestructureAssignExpr *>(expr)) return detectUiInExpr(da->value.get());
        if (auto *arr = dynamic_cast<const ArrayLiteralExpr *>(expr)) {
            for (const auto &el : arr->elements) if (detectUiInExpr(el.get())) return true;
        }
        if (auto *mp = dynamic_cast<const MapLiteralExpr *>(expr)) {
            for (const auto &kv : mp->entries) {
                if (detectUiInExpr(kv.first.get()) || detectUiInExpr(kv.second.get())) return true;
            }
        }
        if (auto *l = dynamic_cast<const LambdaExpr *>(expr)) {
            if (l->declaration && detectUiInStmt(l->declaration->body.get())) return true;
        }
        if (auto *t = dynamic_cast<const TernaryExpr *>(expr)) {
            return detectUiInExpr(t->condition.get()) || detectUiInExpr(t->thenBranch.get()) || detectUiInExpr(t->elseBranch.get());
        }
        if (auto *sp = dynamic_cast<const SpreadExpr *>(expr)) return detectUiInExpr(sp->expr.get());
        return false;
    }

    bool detectUiInStmt(const Stmt *stmt) const {
        if (!stmt) return false;
        if (auto *imp = dynamic_cast<const ImportStmt *>(stmt)) {
            if (imp->path == "ui" || imp->path == "egui" || imp->path == "view" || imp->path == "gui" ||
                imp->path == "ui:shadcn" || imp->path == "ui:primitives" || imp->path == "primitives" ||
                imp->path == "ui:icons" || imp->path == "ui:motion" ||
                imp->path == "lucide-react" || imp->path == "framer-motion" ||
                imp->path == "@radix-ui/react-popover" || imp->path == "@radix-ui/react-dialog" ||
                imp->path == "@radix-ui/react-tabs" ||
                imp->path == "vercel:ui" || imp->path.rfind("ui:", 0) == 0) {
                return true;
            }
        }
        if (auto *v = dynamic_cast<const VarDeclStmt *>(stmt)) return detectUiInExpr(v->initializer.get());
        if (auto *ads = dynamic_cast<const ArrayDestructureStmt *>(stmt)) return detectUiInExpr(ads->initializer.get());
        if (auto *mds = dynamic_cast<const MapDestructureStmt *>(stmt)) return detectUiInExpr(mds->initializer.get());
        if (auto *e = dynamic_cast<const ExprStmt *>(stmt)) return detectUiInExpr(e->expression.get());
        if (auto *r = dynamic_cast<const ReturnStmt *>(stmt)) return detectUiInExpr(r->value.get());
        if (auto *b = dynamic_cast<const BlockStmt *>(stmt)) {
            for (const auto &s : b->statements) if (detectUiInStmt(s.get())) return true;
        }
        if (auto *f = dynamic_cast<const FuncDeclStmt *>(stmt)) return detectUiInStmt(f->body.get());
        if (auto *i = dynamic_cast<const IfStmt *>(stmt)) {
            return detectUiInExpr(i->condition.get()) || detectUiInStmt(i->thenBranch.get()) || detectUiInStmt(i->elseBranch.get());
        }
        if (auto *w = dynamic_cast<const WhileStmt *>(stmt)) return detectUiInExpr(w->condition.get()) || detectUiInStmt(w->body.get());
        if (auto *g = dynamic_cast<const GhoomStmt *>(stmt)) return detectUiInExpr(g->start.get()) || detectUiInExpr(g->end.get()) || detectUiInStmt(g->body.get());
        if (auto *gm = dynamic_cast<const GhumaMeinStmt *>(stmt)) return detectUiInExpr(gm->collection.get()) || detectUiInStmt(gm->body.get());
        return false;
    }

    bool detectEvalInExpr(const Expr *expr) const {
        if (!expr) return false;
        if (auto *id = dynamic_cast<const IdentifierExpr *>(expr)) {
            if (id->name == "meme") return true;
        }
        if (auto *u = dynamic_cast<const UnaryExpr *>(expr)) return detectEvalInExpr(u->operand.get());
        if (auto *b = dynamic_cast<const BinaryExpr *>(expr)) return detectEvalInExpr(b->left.get()) || detectEvalInExpr(b->right.get());
        if (auto *c = dynamic_cast<const CallExpr *>(expr)) {
            if (detectEvalInExpr(c->callee.get())) return true;
            for (const auto &a : c->args) if (detectEvalInExpr(a.get())) return true;
        }
        if (auto *g = dynamic_cast<const GetExpr *>(expr)) return detectEvalInExpr(g->object.get());
        if (auto *a = dynamic_cast<const AssignExpr *>(expr)) return detectEvalInExpr(a->value.get());
        if (auto *da = dynamic_cast<const DestructureAssignExpr *>(expr)) return detectEvalInExpr(da->value.get());
        if (auto *arr = dynamic_cast<const ArrayLiteralExpr *>(expr)) {
            for (const auto &el : arr->elements) if (detectEvalInExpr(el.get())) return true;
        }
        if (auto *mp = dynamic_cast<const MapLiteralExpr *>(expr)) {
            for (const auto &kv : mp->entries) {
                if (detectEvalInExpr(kv.first.get()) || detectEvalInExpr(kv.second.get())) return true;
            }
        }
        if (auto *l = dynamic_cast<const LambdaExpr *>(expr)) {
            if (l->declaration && detectEvalInStmt(l->declaration->body.get())) return true;
        }
        if (auto *t = dynamic_cast<const TernaryExpr *>(expr)) {
            return detectEvalInExpr(t->condition.get()) || detectEvalInExpr(t->thenBranch.get()) || detectEvalInExpr(t->elseBranch.get());
        }
        if (auto *sp = dynamic_cast<const SpreadExpr *>(expr)) return detectEvalInExpr(sp->expr.get());
        return false;
    }

    bool detectEvalInStmt(const Stmt *stmt) const {
        if (!stmt) return false;
        if (auto *imp = dynamic_cast<const ImportStmt *>(stmt)) {
            if (imp->path == "meme") return true;
        }
        if (auto *v = dynamic_cast<const VarDeclStmt *>(stmt)) return detectEvalInExpr(v->initializer.get());
        if (auto *ads = dynamic_cast<const ArrayDestructureStmt *>(stmt)) return detectEvalInExpr(ads->initializer.get());
        if (auto *mds = dynamic_cast<const MapDestructureStmt *>(stmt)) return detectEvalInExpr(mds->initializer.get());
        if (auto *e = dynamic_cast<const ExprStmt *>(stmt)) return detectEvalInExpr(e->expression.get());
        if (auto *r = dynamic_cast<const ReturnStmt *>(stmt)) return detectEvalInExpr(r->value.get());
        if (auto *b = dynamic_cast<const BlockStmt *>(stmt)) {
            for (const auto &s : b->statements) if (detectEvalInStmt(s.get())) return true;
        }
        if (auto *f = dynamic_cast<const FuncDeclStmt *>(stmt)) return detectEvalInStmt(f->body.get());
        if (auto *i = dynamic_cast<const IfStmt *>(stmt)) {
            return detectEvalInExpr(i->condition.get()) || detectEvalInStmt(i->thenBranch.get()) || detectEvalInStmt(i->elseBranch.get());
        }
        if (auto *w = dynamic_cast<const WhileStmt *>(stmt)) return detectEvalInExpr(w->condition.get()) || detectEvalInStmt(w->body.get());
        if (auto *g = dynamic_cast<const GhoomStmt *>(stmt)) return detectEvalInExpr(g->start.get()) || detectEvalInExpr(g->end.get()) || detectEvalInStmt(g->body.get());
        if (auto *gm = dynamic_cast<const GhumaMeinStmt *>(stmt)) return detectEvalInExpr(gm->collection.get()) || detectEvalInStmt(gm->body.get());
        return false;
    }

    void scanDeclarations(const std::vector<std::unique_ptr<Stmt>> &program) {
        for (const auto &stmt : program) {
            if (detectUiInStmt(stmt.get())) {
                programUsesUi = true;
            }
            if (detectEvalInStmt(stmt.get())) {
                programUsesEval = true;
            }
            if (auto *imp = dynamic_cast<const ImportStmt *>(stmt.get())) {
                if ((imp->path.rfind("npm:", 0) == 0 || imp->path.rfind("js:", 0) == 0) &&
                    imp->path != "npm:lucide-react" && imp->path != "npm:framer-motion" &&
                    imp->path != "npm:@radix-ui/react-popover" && imp->path != "npm:@radix-ui/react-dialog" &&
                    imp->path != "npm:@radix-ui/react-tabs") {
                    programUsesJsBridge = true;
                }
            }
            if (auto *c = dynamic_cast<const ClassDeclStmt *>(stmt.get())) {
                knownClasses.insert(c->name);
                for (const auto &cf : c->fields) {
                    knownClassFields.insert(cf.name);
                }
                for (const auto &m : c->methods) {
                    knownClassMethods.insert(sanitizeIdentifier(m->name));
                    if (m->name == "shuru" || m->name == "init" || m->name == "constructor" || m->name == "naya" || m->name == "paida") {
                        if (auto *b = dynamic_cast<const BlockStmt *>(m->body.get())) {
                            for (const auto &s : b->statements) {
                                if (auto *es = dynamic_cast<const ExprStmt *>(s.get())) {
                                    if (auto *set = dynamic_cast<const SetExpr *>(es->expression.get())) {
                                        knownClassFields.insert(set->name);
                                    }
                                }
                            }
                        }
                    }
                }
            } else if (auto *f = dynamic_cast<const FuncDeclStmt *>(stmt.get())) {
                knownFunctions.insert(f->name);
            } else if (auto *imp = dynamic_cast<const ImportStmt *>(stmt.get())) {
                if (imp->path != "ui" && imp->path != "egui" && imp->path != "view" && imp->path != "gui" &&
                    imp->path != "ui:shadcn" && imp->path != "ui:primitives" && imp->path != "primitives" && imp->path != "vercel:ui" &&
                    imp->path != "ui:icons" && imp->path != "ui:motion" && imp->path != "lucide-react" && imp->path != "framer-motion" &&
                    imp->path != "@radix-ui/react-popover" && imp->path != "@radix-ui/react-dialog" && imp->path != "@radix-ui/react-tabs" &&
                    imp->path.rfind("npm:", 0) != 0 && imp->path.rfind("js:", 0) != 0 &&
                    imp->path != "native" && imp->path != "app" && imp->path != "device" &&
                    imp->path != "platform" && imp->path != "permissions" && imp->path != "perms" && imp->path != "meme") {
                    std::string alias = imp->alias.empty() ? std::filesystem::path(imp->path).stem().string() : imp->alias;
                    if (!alias.empty() && imp->namedImports.empty()) {
                        knownModuleAliases.insert(sanitizeIdentifier(alias));
                    }
                }
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
            std::ostringstream oss;
            oss << std::setprecision(17) << e->value;
            std::string s = oss.str();
            if (s.find('.') == std::string::npos && s.find('e') == std::string::npos && s.find('E') == std::string::npos) {
                s += ".0";
            }
            return s;
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
        if (auto *e = dynamic_cast<const ByteExpr *>(expr)) {
            return "MemeValue(static_cast<int64_t>(" + std::to_string(static_cast<int>(e->value)) + "))";
        }
        if (auto *e = dynamic_cast<const BytesExpr *>(expr)) {
            std::string bytesList;
            for (size_t i = 0; i < e->value.size(); ++i) {
                if (i > 0) bytesList += ", ";
                bytesList += std::to_string(static_cast<int>(e->value[i]));
            }
            return "MemeValue(std::vector<uint8_t>{" + bytesList + "})";
        }
        if (auto *e = dynamic_cast<const IdentifierExpr *>(expr)) {
            if (knownFunctions.count(e->name) && !isVarDeclaredInScope(e->name) && !topLevelVars.count(e->name)) {
                return sanitizeIdentifier(e->name) + "_fn";
            }
            return resolveVarOrImportName(e->name);
        }
        if (auto *e = dynamic_cast<const AssignExpr *>(expr)) {
            return "meme_assign(" + sanitizeIdentifier(e->name) + ", " + generateExpr(e->value.get()) + ")";
        }
        if (auto *e = dynamic_cast<const UnaryExpr *>(expr)) {
            std::string opStr;
            if (e->op.type == TokenType::MINUS) opStr = "-";
            else if (e->op.type == TokenType::NOT) opStr = "!";
            else opStr = e->op.value;
            return "(" + opStr + generateExpr(e->operand.get()) + ")";
        }
        if (auto *e = dynamic_cast<const BinaryExpr *>(expr)) {
            auto isPureBoolOrCmp = [](const Expr *sub) -> bool {
                if (dynamic_cast<const BoolExpr *>(sub)) return true;
                if (auto *b = dynamic_cast<const BinaryExpr *>(sub)) {
                    return b->op.type == TokenType::EQUAL || b->op.type == TokenType::NOT_EQUAL ||
                           b->op.type == TokenType::LESS_THAN || b->op.type == TokenType::LESS_THAN_EQUAL ||
                           b->op.type == TokenType::GREATER_THAN || b->op.type == TokenType::GREATER_THAN_EQUAL ||
                           b->op.type == TokenType::AUR || b->op.type == TokenType::YA;
                }
                return false;
            };
            if (e->op.type == TokenType::AUR && !isPureBoolOrCmp(e->right.get())) {
                return "([&]() -> MemeValue { MemeValue _lv = MemeValue(" + generateExpr(e->left.get()) + "); return is_truthy(_lv) ? MemeValue(" + generateExpr(e->right.get()) + ") : _lv; })()";
            }
            if (e->op.type == TokenType::YA && !isPureBoolOrCmp(e->right.get())) {
                return "([&]() -> MemeValue { MemeValue _lv = MemeValue(" + generateExpr(e->left.get()) + "); return is_truthy(_lv) ? _lv : MemeValue(" + generateExpr(e->right.get()) + "); })()";
            }
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
            // Check if call is a constructor: ClassName(...) or GenericClass[T](...)
            std::string ctorClass;
            std::string typeArg;
            if (auto *ident = dynamic_cast<const IdentifierExpr *>(e->callee.get())) {
                if (knownClasses.find(ident->name) != knownClasses.end()) {
                    ctorClass = ident->name;
                }
            } else if (auto *ix = dynamic_cast<const IndexExpr *>(e->callee.get())) {
                if (auto *targetIdent = dynamic_cast<const IdentifierExpr *>(ix->target.get())) {
                    auto mapTypeName = [](const std::string &t) -> std::string {
                        if (t == "ank" || t == "int") return "int64_t";
                        if (t == "point" || t == "float") return "double";
                        if (t == "bol" || t == "str") return "std::string";
                        if (t == "sach" || t == "bool") return "bool";
                        return t;
                    };
                    if (knownClasses.find(targetIdent->name) != knownClasses.end()) {
                        ctorClass = targetIdent->name;
                        if (!ix->indices.empty()) {
                            for (size_t i = 0; i < ix->indices.size(); ++i) {
                                if (i > 0) typeArg += ", ";
                                if (auto *idExpr = dynamic_cast<const IdentifierExpr *>(ix->indices[i].get())) {
                                    typeArg += mapTypeName(idExpr->name);
                                } else {
                                    typeArg += "MemeValue";
                                }
                            }
                        } else if (auto *idxIdent = dynamic_cast<const IdentifierExpr *>(ix->index.get())) {
                            typeArg = mapTypeName(idxIdent->name);
                        }
                    } else if (knownFunctions.find(targetIdent->name) != knownFunctions.end()) {
                        std::string args;
                        for (size_t i = 0; i < e->args.size(); ++i) {
                            if (i > 0) args += ", ";
                            args += generateExpr(e->args[i].get());
                        }
                        std::string tArg;
                        if (!ix->indices.empty()) {
                            for (size_t i = 0; i < ix->indices.size(); ++i) {
                                if (i > 0) tArg += ", ";
                                if (auto *idExpr = dynamic_cast<const IdentifierExpr *>(ix->indices[i].get())) {
                                    tArg += mapTypeName(idExpr->name);
                                } else {
                                    tArg += "MemeValue";
                                }
                            }
                        } else if (auto *idxIdent = dynamic_cast<const IdentifierExpr *>(ix->index.get())) {
                            tArg = mapTypeName(idxIdent->name);
                        } else {
                            tArg = "MemeValue";
                        }
                        return targetIdent->name + "<" + tArg + ">(" + args + ")";
                    }
                }
            }
            if (!ctorClass.empty()) {
                std::string args;
                for (size_t i = 0; i < e->args.size(); ++i) {
                    if (i > 0) args += ", ";
                    args += generateExpr(e->args[i].get());
                }
                if (!typeArg.empty()) {
                    return "std::make_shared<" + ctorClass + "<" + typeArg + ">>(" + args + ")";
                }
                return "std::make_shared<" + ctorClass + ">(" + args + ")";
            }

            // Check if method call: obj.method(args)
            if (auto *get = dynamic_cast<const GetExpr *>(e->callee.get())) {
                std::string objStr = generateExpr(get->object.get());
                std::string method = sanitizeIdentifier(get->name);

                // Handle .push() on vectors
                if (method == "push" || method == "dhal" || method == "push_back") {
                    std::string pushArgs;
                    for (size_t i = 0; i < e->args.size(); ++i) {
                        if (i > 0) pushArgs += ", ";
                        pushArgs += generateExpr(e->args[i].get());
                    }
                    return "meme_push(" + objStr + ", " + pushArgs + ")";
                }
                if (method == "length" || method == "lambai" || method == "size") {
                    return "static_cast<int64_t>(" + objStr + ".size())";
                }

                // Map membership methods
                if (method == "has" || method == "contains" || method == "hai_kya" || method == "hai") {
                    std::string mapArgs;
                    for (size_t i = 0; i < e->args.size(); ++i) {
                        if (i > 0) mapArgs += ", ";
                        if (auto *se = dynamic_cast<const StringExpr *>(e->args[i].get())) {
                            mapArgs += "\"" + escapeString(se->value) + "\"";
                        } else {
                            mapArgs += generateExpr(e->args[i].get());
                        }
                    }
                    return "meme_has(" + objStr + ", " + mapArgs + ")";
                }
                if (method == "remove" || method == "hata") {
                    std::string mapArgs;
                    for (size_t i = 0; i < e->args.size(); ++i) {
                        if (i > 0) mapArgs += ", ";
                        if (auto *se = dynamic_cast<const StringExpr *>(e->args[i].get())) {
                            mapArgs += "\"" + escapeString(se->value) + "\"";
                        } else {
                            mapArgs += generateExpr(e->args[i].get());
                        }
                    }
                    return "meme_remove(" + objStr + ", " + mapArgs + ")";
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
                if (knownModuleAliases.count(objStr) ||
                    method == "pop_back" ||
                    method == "is_ok" || method == "is_err" || method == "is_some" || method == "is_none" ||
                    method == "unwrap" || method == "unwrap_err" || method == "unwrap_or") {
                    return objStr + "." + method + "(" + args + ")";
                }
                if (method == "pop") {
                    return "meme_pop(" + objStr + (args.empty() ? "" : (", " + args)) + ")";
                }
                // Map methods
                if (method == "keys" || method == "values" ||
                    method == "chaabi" || method == "maal" || method == "la" || method == "get") {
                    if (knownModuleAliases.count(objStr)) {
                        std::string mapArgs;
                        for (size_t i = 0; i < e->args.size(); ++i) {
                            if (i > 0) mapArgs += ", ";
                            if (auto *se = dynamic_cast<const StringExpr *>(e->args[i].get())) {
                                mapArgs += "\"" + escapeString(se->value) + "\"";
                            } else {
                                mapArgs += generateExpr(e->args[i].get());
                            }
                        }
                        return objStr + "." + method + "(" + mapArgs + ")";
                    }
                    std::string mArgs;
                    for (size_t i = 0; i < e->args.size(); ++i) {
                        if (i > 0) mArgs += ", ";
                        mArgs += "MemeValue(" + generateExpr(e->args[i].get()) + ")";
                    }
                    return "meme_call_method(" + objStr + ", \"" + escapeString(get->name) + "\", std::vector<MemeValue>{" + mArgs + "})";
                }
                // Mutex and thread methods
                if (method == "khol") {
                    if (knownModuleAliases.count(objStr)) return objStr + "." + method + "(" + args + ")";
                    return "meme_khol_or_call(" + objStr + (args.empty() ? "" : (", " + args)) + ")";
                }
                if (method == "lock" || method == "unlock" || method == "pakad" || method == "chhod" ||
                    method == "band" || method == "try_lock" || method == "koshish" ||
                    method == "surakshit" || method == "with_lock" || method == "join" || method == "rok" ||
                    method == "jod" || method == "wait" || method == "is_done" || method == "khatam" ||
                    method == "chal_raha" || method == "is_alive") {
                    return objStr + "." + method + "(" + args + ")";
                }
                // Socket methods
                if (method == "send") {
                    if (knownModuleAliases.count(objStr)) return objStr + "." + method + "(" + args + ")";
                    return "meme_send_or_call(" + objStr + (args.empty() ? "" : (", " + args)) + ")";
                }
                if (method == "recv" || method == "close" ||
                    method == "is_valid" || method == "is_closed" || method == "accept" ||
                    method == "set_nonblocking" || method == "set_timeout") {
                    return objStr + "." + method + "(" + args + ")";
                }
                // Byte buffer methods
                if (method == "read_u8" || method == "write_u8" ||
                    method == "read_u16_be" || method == "read_u16_le" ||
                    method == "write_u16_be" || method == "write_u16_le" ||
                    method == "read_u32_be" || method == "read_u32_le" ||
                    method == "write_u32_be" || method == "write_u32_le" ||
                    method == "read_u64_be" || method == "read_u64_le" ||
                    method == "write_u64_be" || method == "write_u64_le" ||
                    method == "read_i32_be" || method == "read_i32_le" ||
                    method == "write_i32_be" || method == "write_i32_le" ||
                    method == "read_f64_be" || method == "read_f64_le" ||
                    method == "write_f64_be" || method == "write_f64_le" ||
                    method == "to_hex" || method == "from_hex" || method == "to_string" ||
                    method == "read_bytes" || method == "padh_bytes" ||
                    method == "write_bytes" || method == "likh_bytes") {
                    return objStr + "." + method + "(" + args + ")";
                }
                // String and container membership methods: wrap with MemeValue(...)
                static const std::unordered_set<std::string> strToStringMethods = {
                    "replace", "trim", "to_upper", "to_lower", "pad_left", "pad_right",
                    "substring", "badal", "saaf", "upper", "lower", "chilla", "dheere_bol"
                };
                if (strToStringMethods.count(method)) {
                    return "MemeValue(" + objStr + ")." + method + "(" + args + ").to_string()";
                }
                static const std::unordered_set<std::string> strMethods = {
                    "slice", "kaat", "charAt", "chars", "starts_with", "startsWith", "ends_with", "endsWith",
                    "find", "split", "akshar", "akshar_list", "shuru_hota", "khatam_hota", "dhund", "tod",
                    "indexOf", "to_int", "toInt", "to_float", "toFloat"
                };
                if (strMethods.count(method)) {
                    return "MemeValue(" + objStr + ")." + method + "(" + args + ")";
                }
                if (knownClassMethods.count(method) || objStr.rfind("std::make_shared<", 0) == 0) {
                    return objStr + "->" + method + "(" + args + ")";
                }
                std::string mArgs;
                for (size_t i = 0; i < e->args.size(); ++i) {
                    if (i > 0) mArgs += ", ";
                    mArgs += "MemeValue(" + generateExpr(e->args[i].get()) + ")";
                }
                return "meme_call_method(" + objStr + ", \"" + escapeString(get->name) + "\", std::vector<MemeValue>{" + mArgs + "})";
            }

            // Regular function call
            std::string calleeStr;
            std::string spreadCalleeStr;
            if (auto *id = dynamic_cast<const IdentifierExpr *>(e->callee.get())) {
                if ((id->name == "size" || id->name == "len" || id->name == "length" || id->name == "lambai") &&
                    e->args.size() == 1 &&
                    knownFunctions.find(id->name) == knownFunctions.end() &&
                    !isVarDeclaredInScope(id->name) && !topLevelVars.count(id->name)) {
                    return "static_cast<int64_t>(meme_length(" + generateExpr(e->args[0].get()) + "))";
                }
                if (knownFunctions.count(id->name) && !isVarDeclaredInScope(id->name) && !topLevelVars.count(id->name)) {
                    calleeStr = sanitizeIdentifier(id->name);
                    spreadCalleeStr = sanitizeIdentifier(id->name) + "_fn";
                } else {
                    calleeStr = spreadCalleeStr = resolveVarOrImportName(id->name);
                }
            } else {
                calleeStr = spreadCalleeStr = generateExpr(e->callee.get());
            }
            bool hasSpreadArg = false;
            for (const auto &a : e->args) if (dynamic_cast<const SpreadExpr *>(a.get())) { hasSpreadArg = true; break; }
            if (hasSpreadArg) {
                std::string pieces;
                for (size_t i = 0; i < e->args.size(); ++i) {
                    if (i > 0) pieces += ", ";
                    if (auto *sp = dynamic_cast<const SpreadExpr *>(e->args[i].get())) {
                        pieces += "meme_list_spread(" + generateExpr(sp->expr.get()) + ")";
                    } else {
                        pieces += "meme_list_item(" + generateExpr(e->args[i].get()) + ")";
                    }
                }
                return "MemeValue(" + spreadCalleeStr + ").call_args(*meme_build_list({" + pieces + "}).list_data)";
            }
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
            if (knownModuleAliases.count(objStr)) {
                return objStr + "." + prop;
            }
            if (prop == "size" || prop == "length" || prop == "lambai") {
                return "static_cast<int64_t>(" + objStr + ".size())";
            }
            if (prop == "is_valid") {
                return objStr + ".is_valid()";
            }
            if (prop == "is_closed") {
                return objStr + ".is_closed()";
            }
            if (prop == "fd") {
                return objStr + ".get_fd()";
            }
            if (prop == "peer_ip") {
                return objStr + ".get_peer_ip()";
            }
            if (prop == "peer_port") {
                return objStr + ".get_peer_port()";
            }
            if (knownClassFields.count(prop)) {
                return objStr + "->" + prop;
            }
            return "meme_index(" + objStr + ", \"" + e->name + "\")";
        }
        if (auto *e = dynamic_cast<const SetExpr *>(expr)) {
            std::string objStr = generateExpr(e->object.get());
            std::string prop = sanitizeIdentifier(e->name);
            std::string targetLValue = (objStr == "this") ? ("this->" + prop) : (knownClassFields.count(prop) ? (objStr + "->" + prop) : ("meme_index(" + objStr + ", \"" + e->name + "\")"));

            if (auto *arr = dynamic_cast<const ArrayLiteralExpr *>(e->value.get())) {
                if (arr->elements.empty()) {
                    return "meme_assign_empty_list(" + targetLValue + ")";
                }
            }
            if (auto *mapLit = dynamic_cast<const MapLiteralExpr *>(e->value.get())) {
                if (mapLit->entries.empty()) {
                    return "meme_assign_empty_map(" + targetLValue + ")";
                }
            }
            std::string valStr = generateExpr(e->value.get());
            return "(" + targetLValue + " = " + valStr + ")";
        }
        if (auto *e = dynamic_cast<const IndexExpr *>(expr)) {
            if (auto *se = dynamic_cast<const StringExpr *>(e->index.get())) {
                return "meme_index(" + generateExpr(e->target.get()) + ", \"" + escapeString(se->value) + "\")";
            }
            return "meme_index(" + generateExpr(e->target.get()) + ", " + generateExpr(e->index.get()) + ")";
        }
        if (auto *e = dynamic_cast<const IndexAssignExpr *>(expr)) {
            std::string tgt = generateExpr(e->target.get());
            std::string idx = generateExpr(e->index.get());
            if (auto *se = dynamic_cast<const StringExpr *>(e->index.get())) {
                idx = "\"" + escapeString(se->value) + "\"";
            }
            std::string targetLValue = "meme_index(" + tgt + ", " + idx + ")";
            if (auto *arr = dynamic_cast<const ArrayLiteralExpr *>(e->value.get())) {
                if (arr->elements.empty()) {
                    return "meme_assign_empty_list(" + targetLValue + ")";
                }
            }
            if (auto *mapLit = dynamic_cast<const MapLiteralExpr *>(e->value.get())) {
                if (mapLit->entries.empty()) {
                    return "meme_assign_empty_map(" + targetLValue + ")";
                }
            }
            return "(" + targetLValue + " = " + generateExpr(e->value.get()) + ")";
        }
        if (auto *e = dynamic_cast<const ArrayLiteralExpr *>(expr)) {
            bool hasSpread = false;
            for (const auto &el : e->elements) if (dynamic_cast<const SpreadExpr *>(el.get())) { hasSpread = true; break; }
            if (hasSpread) {
                std::string pieces;
                for (size_t i = 0; i < e->elements.size(); ++i) {
                    if (i > 0) pieces += ", ";
                    if (auto *sp = dynamic_cast<const SpreadExpr *>(e->elements[i].get())) {
                        pieces += "meme_list_spread(" + generateExpr(sp->expr.get()) + ")";
                    } else {
                        pieces += "meme_list_item(" + generateExpr(e->elements[i].get()) + ")";
                    }
                }
                return "meme_build_list({" + pieces + "})";
            }
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
            return "MEME_TERNARY(" + generateExpr(e->condition.get()) + ", " +
                   generateExpr(e->thenBranch.get()) + ", " + generateExpr(e->elseBranch.get()) + ")";
        }
        if (auto *e = dynamic_cast<const MapLiteralExpr *>(expr)) {
            bool hasSpread = false;
            for (const auto &kv : e->entries) if (dynamic_cast<const SpreadExpr *>(kv.first.get()) || !kv.second) { hasSpread = true; break; }
            if (hasSpread) {
                std::string pieces;
                for (size_t i = 0; i < e->entries.size(); ++i) {
                    if (i > 0) pieces += ", ";
                    if (auto *sp = dynamic_cast<const SpreadExpr *>(e->entries[i].first.get())) {
                        pieces += "meme_map_spread(" + generateExpr(sp->expr.get()) + ")";
                    } else if (e->entries[i].second) {
                        pieces += "meme_map_kv(" + generateExpr(e->entries[i].first.get()) + ", " + generateExpr(e->entries[i].second.get()) + ")";
                    }
                }
                return "meme_build_map({" + pieces + "})";
            }
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

        if (auto *jsx = dynamic_cast<const JsxElementExpr *>(expr)) {
            std::string propsPairs = "{";
            for (size_t i = 0; i < jsx->props.size(); ++i) {
                if (i > 0) propsPairs += ", ";
                propsPairs += "{\"" + escapeString(jsx->props[i].first) + "\", MemeValue(" + generateExpr(jsx->props[i].second.get()) + ")}";
            }
            propsPairs += "}";

            std::string childrenVec = "{";
            for (size_t i = 0; i < jsx->children.size(); ++i) {
                if (i > 0) childrenVec += ", ";
                childrenVec += "MemeValue(" + generateExpr(jsx->children[i].get()) + ")";
            }
            childrenVec += "}";

            if (jsx->isFragment || jsx->tag == "Fragment" || jsx->tag == "fragment") {
                return "make_jsx_element(\"fragment\", true, {}, " + childrenVec + ")";
            }

            size_t dotPos = jsx->tag.find('.');
            if (dotPos != std::string::npos && jsx->tag.substr(dotPos + 1) == "Provider") {
                std::string ctxObj = sanitizeIdentifier(jsx->tag.substr(0, dotPos));
                std::string valExprStr = "MemeValue()";
                for (const auto &p : jsx->props) {
                    if (p.first == "value") {
                        valExprStr = "MemeValue(" + generateExpr(p.second.get()) + ")";
                        break;
                    }
                }
                return "([&]() -> MemeValue { meme_push_context(" + ctxObj + ", " + valExprStr + "); "
                       "MemeValue _p_res = make_jsx_element(\"fragment\", true, {}, " + childrenVec + "); "
                       "meme_pop_context(" + ctxObj + "); return _p_res; })()";
            }

            if (!jsx->tag.empty() && (std::isupper(static_cast<unsigned char>(jsx->tag[0])) || dotPos != std::string::npos)) {
                std::string compExprStr;
                if (dotPos != std::string::npos) {
                    std::string base = sanitizeIdentifier(jsx->tag.substr(0, dotPos));
                    std::string mem = jsx->tag.substr(dotPos + 1);
                    compExprStr = "meme_index(" + base + ", \"" + escapeString(mem) + "\")";
                } else {
                    if (knownFunctions.count(jsx->tag) && !isVarDeclaredInScope(jsx->tag) && !topLevelVars.count(jsx->tag)) {
                        compExprStr = sanitizeIdentifier(jsx->tag) + "_fn";
                    } else if (jsx->tag == "Router" || jsx->tag == "Route" || jsx->tag == "Link" || jsx->tag == "Platform") {
                        compExprStr = jsx->tag + "_fn";
                    } else {
                        compExprStr = sanitizeIdentifier(jsx->tag);
                    }
                }
                return "make_jsx_component(" + compExprStr + ", \"" + escapeString(jsx->tag) + "\", " + propsPairs + ", " + childrenVec + ")";
            }

            return "make_jsx_element(\"" + escapeString(jsx->tag) + "\", false, " + propsPairs + ", " + childrenVec + ")";
        }

        if (auto *le = dynamic_cast<const LambdaExpr *>(expr)) {
            if (!le->declaration) return "MemeValue()";
            const auto *f = le->declaration.get();
            std::ostringstream ss;
            ss << "meme_fn([=](const std::vector<MemeValue> &_args) mutable -> MemeValue {\n";
            int origLevel = indentLevel;
            indentLevel++;
            pushVarScope();
            for (size_t i = 0; i < f->paramList.size(); ++i) {
                const auto &p = f->paramList[i];
                if (p.isMapDestructure) {
                    std::string tmpM = "_lm_" + std::to_string(tempCounter++);
                    ss << indent() << "MemeValue " << tmpM << " = (" << i << " < _args.size()) ? _args[" << i << "] : make_map({});\n";
                    for (const auto &k : p.destructureKeys) {
                        std::string kVar = sanitizeIdentifier(k.name);
                        declareScopeVar(k.name);
                        std::string defExpr = k.defaultValue ? ("MemeValue(" + generateExpr(k.defaultValue.get()) + ")") : "MemeValue()";
                        ss << indent() << "MemeValue " << kVar << " = meme_map_destruct_get(" << tmpM << ", \"" << escapeString(k.name) << "\", " << defExpr << ");\n";
                    }
                } else if (p.isVariadic) {
                    std::string pVar = sanitizeIdentifier(p.name);
                    declareScopeVar(p.name);
                    ss << indent() << "MemeValue " << pVar << " = meme_destructure_rest(MemeValue(_args), " << i << ");\n";
                } else {
                    std::string pVar = sanitizeIdentifier(p.name);
                    declareScopeVar(p.name);
                    std::string defExpr = p.defaultValue ? ("MemeValue(" + generateExpr(p.defaultValue.get()) + ")") : "MemeValue()";
                    ss << indent() << "MemeValue " << pVar << " = (" << i << " < _args.size()) ? _args[" << i << "] : " << defExpr << ";\n";
                }
            }
            if (f->body) {
                std::ostringstream bss;
                generateStmt(f->body.get(), bss);
                ss << bss.str();
                if (!hasReturnStmt(f->body.get())) {
                    ss << indent() << "return MemeValue();\n";
                }
            } else {
                ss << indent() << "return MemeValue();\n";
            }
            popVarScope();
            indentLevel = origLevel;
            ss << indent() << "})";
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

        if (stmt->line > 0 && !currentSourceFile.empty() && !dynamic_cast<const BlockStmt *>(stmt)) {
            out << "#line " << stmt->line << " \"" << escapeString(currentSourceFile) << "\"\n";
        }

        if (auto *v = dynamic_cast<const VarDeclStmt *>(stmt)) {
            std::string varName = sanitizeIdentifier(v->name);
            if (inMainPass && topLevelVars.count(v->name)) {
                if (v->initializer) {
                    out << indent() << varName << " = MemeValue(" << generateExpr(v->initializer.get()) << ");\n";
                }
                return;
            }
            declareScopeVar(v->name);
            std::string typeStr = "auto";
            if (v->typeAnnotation) {
                std::string cppT = toCppType(v->typeAnnotation);
                if (cppT != "MemeValue") {
                    typeStr = cppT;
                }
            } else if (v->initializer && dynamic_cast<const NullExpr *>(v->initializer.get())) {
                typeStr = "MemeValue"; // Avoid deducing std::nullptr_t
            } else if (v->initializer && dynamic_cast<const IndexExpr *>(v->initializer.get())) {
                auto *ix = dynamic_cast<const IndexExpr *>(v->initializer.get());
                auto indexedType = getIndexedFieldType(ix);
                if (indexedType && (indexedType->kind == TypeKind::LIST || indexedType->kind == TypeKind::CLASS)) {
                    bool reassigned = currentScopeBody ? isAssignedIn(currentScopeBody, v->name)
                                                      : (currentProgram ? isAssignedInProgram(*currentProgram, v->name) : false);
                    if (!reassigned) {
                        typeStr = "const auto &";
                    }
                }
            }

            if (v->initializer) {
                if (auto *arr = dynamic_cast<const ArrayLiteralExpr *>(v->initializer.get())) {
                    if (arr->elements.empty() && typeStr != "auto" && typeStr != "MemeValue") {
                        out << indent() << typeStr << " " << varName << " = {};\n";
                        return;
                    }
                }
                if (auto *mapLit = dynamic_cast<const MapLiteralExpr *>(v->initializer.get())) {
                    if (mapLit->entries.empty() && typeStr != "auto" && typeStr != "MemeValue") {
                        out << indent() << typeStr << " " << varName << " = {};\n";
                        return;
                    }
                }
                if (typeStr != "auto" && typeStr != "MemeValue") {
                    out << indent() << typeStr << " " << varName << " = meme_cast<" << typeStr << ">(" << generateExpr(v->initializer.get()) << ");\n";
                } else {
                    out << indent() << typeStr << " " << varName << " = " << generateExpr(v->initializer.get()) << ";\n";
                }
            } else {
                out << indent() << (typeStr != "auto" ? typeStr : "MemeValue") << " " << varName << ";\n";
            }
            return;
        }

        auto emitArrayDestruct = [&](const std::vector<DestructureItem> &items, const Expr *initExpr) {
            std::string tmp = "_ads_" + std::to_string(tempCounter++);
            out << indent() << "MemeValue " << tmp << " = MemeValue(" << generateExpr(initExpr) << ");\n";
            size_t listIdx = 0;
            for (const auto &item : items) {
                std::string vn = sanitizeIdentifier(item.name);
                std::string rhs = item.isVariadic
                    ? ("meme_destructure_rest(" + tmp + ", " + std::to_string(listIdx) + ")")
                    : ("meme_destructure_get(" + tmp + ", " + std::to_string(listIdx++) + ")");
                if ((inMainPass && topLevelVars.count(item.name)) || isVarDeclaredInScope(item.name)) {
                    out << indent() << vn << " = " << rhs << ";\n";
                } else {
                    declareScopeVar(item.name);
                    out << indent() << "MemeValue " << vn << " = " << rhs << ";\n";
                }
            }
        };

        if (auto *ads = dynamic_cast<const ArrayDestructureStmt *>(stmt)) {
            emitArrayDestruct(ads->items, ads->initializer.get());
            return;
        }

        if (auto *mds = dynamic_cast<const MapDestructureStmt *>(stmt)) {
            std::string tmp = "_mds_" + std::to_string(tempCounter++);
            out << indent() << "MemeValue " << tmp << " = MemeValue(" << generateExpr(mds->initializer.get()) << ");\n";
            for (const auto &k : mds->keys) {
                std::string vn = sanitizeIdentifier(k);
                std::string rhs = "meme_map_destruct_get(" + tmp + ", \"" + escapeString(k) + "\")";
                if ((inMainPass && topLevelVars.count(k)) || isVarDeclaredInScope(k)) {
                    out << indent() << vn << " = " << rhs << ";\n";
                } else {
                    declareScopeVar(k);
                    out << indent() << "MemeValue " << vn << " = " << rhs << ";\n";
                }
            }
            return;
        }

        if (auto *e = dynamic_cast<const ExprStmt *>(stmt)) {
            if (auto *da = dynamic_cast<const DestructureAssignExpr *>(e->expression.get())) {
                emitArrayDestruct(da->items, da->value.get());
                return;
            }
            out << indent() << generateExpr(e->expression.get()) << ";\n";
            return;
        }

        if (auto *f = dynamic_cast<const FuncDeclStmt *>(stmt)) {
            std::string fnName = sanitizeIdentifier(f->name);
            std::string boxName = "_fbox_" + fnName + "_" + std::to_string(tempCounter++);
            declareScopeVar(f->name);
            out << indent() << "auto " << boxName << " = std::make_shared<MemeValue>();\n";
            out << indent() << "*" << boxName << " = meme_fn([=](const std::vector<MemeValue> &_args) mutable -> MemeValue {\n";
            indentLevel++;
            out << indent() << "MemeValue &" << fnName << " = *" << boxName << ";\n";
            out << indent() << "(void)" << fnName << "; (void)_args;\n";
            pushVarScope();
            for (size_t i = 0; i < f->paramList.size(); ++i) {
                const auto &p = f->paramList[i];
                if (p.isMapDestructure) {
                    std::string tmpM = "_nfm_" + std::to_string(tempCounter++);
                    out << indent() << "MemeValue " << tmpM << " = (" << i << " < _args.size()) ? _args[" << i << "] : make_map({});\n";
                    for (const auto &dk : p.destructureKeys) {
                        std::string kVar = sanitizeIdentifier(dk.name);
                        declareScopeVar(dk.name);
                        std::string defExpr = dk.defaultValue ? ("MemeValue(" + generateExpr(dk.defaultValue.get()) + ")") : "MemeValue()";
                        out << indent() << "MemeValue " << kVar << " = meme_map_destruct_get(" << tmpM << ", \"" << escapeString(dk.name) << "\", " << defExpr << ");\n";
                    }
                } else if (p.isVariadic) {
                    std::string pVar = sanitizeIdentifier(p.name);
                    declareScopeVar(p.name);
                    out << indent() << "MemeValue " << pVar << " = meme_destructure_rest(MemeValue(_args), " << i << ");\n";
                } else {
                    std::string pVar = sanitizeIdentifier(p.name);
                    declareScopeVar(p.name);
                    std::string defExpr = p.defaultValue ? ("MemeValue(" + generateExpr(p.defaultValue.get()) + ")") : "MemeValue()";
                    out << indent() << "MemeValue " << pVar << " = (" << i << " < _args.size()) ? _args[" << i << "] : " << defExpr << ";\n";
                }
            }
            if (f->body) {
                if (auto *fb = dynamic_cast<const BlockStmt *>(f->body.get())) {
                    for (const auto &s : fb->statements) generateStmt(s.get(), out);
                } else {
                    generateStmt(f->body.get(), out);
                }
                if (!hasReturnStmt(f->body.get())) {
                    out << indent() << "return MemeValue();\n";
                }
            } else {
                out << indent() << "return MemeValue();\n";
            }
            popVarScope();
            indentLevel--;
            out << indent() << "});\n";
            out << indent() << "MemeValue " << fnName << " = *" << boxName << ";\n";
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
            pushVarScope();
            for (const auto &s : b->statements) {
                generateStmt(s.get(), out);
            }
            popVarScope();
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
            std::string alias = imp->alias.empty() ? std::filesystem::path(imp->path).stem().string() : imp->alias;
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
            } else if (imp->path == "ui:shadcn") {
                for (const auto &ni : imp->namedImports) {
                    out << indent() << "MemeValue " << sanitizeIdentifier(ni) << " = meme_index(ui_shadcn, \"" << escapeString(ni) << "\");\n";
                }
            } else if (imp->path == "ui:icons" || imp->path == "lucide-react") {
                for (const auto &ni : imp->namedImports) {
                    out << indent() << "MemeValue " << sanitizeIdentifier(ni) << " = meme_index(ui_icons, \"" << escapeString(ni) << "\");\n";
                }
            } else if (imp->path == "ui:motion" || imp->path == "framer-motion") {
                for (const auto &ni : imp->namedImports) {
                    if (programUsesUi && ni == "useSpring") continue;
                    out << indent() << "MemeValue " << sanitizeIdentifier(ni) << " = meme_index(ui_motion, \"" << escapeString(ni) << "\");\n";
                }
            } else if (imp->path == "ui:primitives" || imp->path == "primitives" || imp->path == "vercel:ui") {
                for (const auto &ni : imp->namedImports) {
                    out << indent() << "MemeValue " << sanitizeIdentifier(ni) << " = meme_index(ui_primitives, \"" << escapeString(ni) << "\");\n";
                }
            } else if (imp->path == "ui" || imp->path == "egui" || imp->path == "view" || imp->path == "gui") {
                static const std::unordered_set<std::string> kUiGlobalFns = {
                    "useState", "state_bana", "useEffect", "useMemo", "useCallback",
                    "useReducer", "useRef", "createContext", "useContext", "useRouter",
                    "Router", "Route", "Link", "Platform"
                };
                for (const auto &ni : imp->namedImports) {
                    if (programUsesUi && kUiGlobalFns.count(ni)) continue;
                    out << indent() << "MemeValue " << sanitizeIdentifier(ni) << " = meme_index(" << (imp->path == "egui" ? "egui" : "ui") << ", \"" << escapeString(ni) << "\");\n";
                }
            } else if (imp->path == "native" || imp->path == "app" || imp->path == "device" ||
                       imp->path == "platform" || imp->path == "permissions" || imp->path == "perms") {
                std::string srcMod = (imp->path == "app") ? "app"
                    : (imp->path == "device" ? "meme_index(native, \"device\")"
                    : ((imp->path == "permissions" || imp->path == "perms") ? "permissions"
                    : (imp->path == "platform" ? "platform" : "native")));
                if (!imp->namedImports.empty()) {
                    for (const auto &ni : imp->namedImports) {
                        std::string targetSym = isCollidingGlobalSymbol(ni) ? ("_imp_" + sanitizeIdentifier(ni)) : sanitizeIdentifier(ni);
                        if (isCollidingGlobalSymbol(ni)) renamedImports[ni] = targetSym;
                        if (imp->path == "platform" && ni == "Platform" && programUsesUi) {
                            out << indent() << "MemeValue " << targetSym << " = Platform_fn;\n";
                        } else {
                            out << indent() << "MemeValue " << targetSym << " = meme_index(" << srcMod << ", \"" << escapeString(ni) << "\");\n";
                        }
                    }
                } else if (!alias.empty() && alias != "native" && alias != "app" && alias != "platform" && alias != "permissions" && alias != "perms") {
                    out << indent() << "MemeValue " << alias << " = " << srcMod << ";\n";
                }
            } else if (imp->path == "meme") {
                if (!imp->namedImports.empty()) {
                    for (const auto &ni : imp->namedImports) {
                        out << indent() << "MemeValue " << sanitizeIdentifier(ni) << " = meme_index(meme, \"" << escapeString(ni) << "\");\n";
                    }
                } else if (!alias.empty() && alias != "meme") {
                    out << indent() << "MemeValue " << alias << " = meme;\n";
                }
            } else if (imp->path == "@radix-ui/react-popover" || imp->path == "@radix-ui/react-dialog" ||
                       imp->path == "@radix-ui/react-tabs" || imp->path == "npm:@radix-ui/react-popover" ||
                       imp->path == "npm:@radix-ui/react-dialog" || imp->path == "npm:@radix-ui/react-tabs") {
                for (const auto &ni : imp->namedImports) {
                    out << indent() << "MemeValue " << sanitizeIdentifier(ni) << " = meme_index(ui_shadcn, \"" << escapeString(ni) << "\");\n";
                }
            } else if (imp->path.rfind("npm:", 0) == 0 || imp->path.rfind("js:", 0) == 0) {
                std::string pkgName = (imp->path.rfind("npm:", 0) == 0) ? imp->path.substr(4) : imp->path.substr(3);
                if (!imp->namedImports.empty()) {
                    std::string tmpMod = "_jsm_" + std::to_string(tempCounter++);
                    out << indent() << "MemeValue " << tmpMod << " = meme_load_npm_module(\"" << escapeString(pkgName) << "\");\n";
                    for (const auto &ni : imp->namedImports) {
                        out << indent() << "MemeValue " << sanitizeIdentifier(ni) << " = meme_index(" << tmpMod << ", \"" << escapeString(ni) << "\");\n";
                    }
                } else if (!alias.empty()) {
                    out << indent() << "MemeValue " << alias << " = meme_load_npm_module(\"" << escapeString(pkgName) << "\");\n";
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

    bool isLValue(const Expr *e) const {
        if (!e) return false;
        if (dynamic_cast<const IdentifierExpr *>(e)) return true;
        if (auto *g = dynamic_cast<const GetExpr *>(e)) {
            return isLValue(g->object.get());
        }
        if (auto *ix = dynamic_cast<const IndexExpr *>(e)) {
            return isLValue(ix->target.get());
        }
        return false;
    }

    std::shared_ptr<Type> getIndexedFieldType(const IndexExpr *ix) const {
        if (!ix) return nullptr;
        if (auto *g = dynamic_cast<const GetExpr *>(ix->target.get())) {
            if (auto *objId = dynamic_cast<const IdentifierExpr *>(g->object.get())) {
                if (objId->name == "khud" || objId->name == "apun" || objId->name == "self" || objId->name == "this") {
                    auto it = currentClassFieldTypes.find(g->name);
                    if (it != currentClassFieldTypes.end()) {
                        auto fieldType = it->second;
                        if (fieldType->kind == TypeKind::MAP && fieldType->valueType) {
                            return fieldType->valueType;
                        }
                        if (fieldType->kind == TypeKind::LIST && fieldType->elementType) {
                            return fieldType->elementType;
                        }
                    }
                }
            }
        }
        return nullptr;
    }

    bool isAssignedInExpr(const Expr *expr, const std::string &varName) const {
        if (!expr) return false;
        if (auto *a = dynamic_cast<const AssignExpr *>(expr)) {
            if (a->name == varName) return true;
            return isAssignedInExpr(a->value.get(), varName);
        }
        if (auto *u = dynamic_cast<const UnaryExpr *>(expr)) return isAssignedInExpr(u->operand.get(), varName);
        if (auto *b = dynamic_cast<const BinaryExpr *>(expr)) return isAssignedInExpr(b->left.get(), varName) || isAssignedInExpr(b->right.get(), varName);
        if (auto *c = dynamic_cast<const CallExpr *>(expr)) {
            if (isAssignedInExpr(c->callee.get(), varName)) return true;
            for (const auto &arg : c->args) if (isAssignedInExpr(arg.get(), varName)) return true;
            return false;
        }
        if (auto *g = dynamic_cast<const GetExpr *>(expr)) return isAssignedInExpr(g->object.get(), varName);
        if (auto *s = dynamic_cast<const SetExpr *>(expr)) {
            return isAssignedInExpr(s->object.get(), varName) || isAssignedInExpr(s->value.get(), varName);
        }
        if (auto *ix = dynamic_cast<const IndexExpr *>(expr)) {
            return isAssignedInExpr(ix->target.get(), varName) || isAssignedInExpr(ix->index.get(), varName);
        }
        if (auto *ia = dynamic_cast<const IndexAssignExpr *>(expr)) {
            return isAssignedInExpr(ia->target.get(), varName) || isAssignedInExpr(ia->index.get(), varName) || isAssignedInExpr(ia->value.get(), varName);
        }
        if (auto *t = dynamic_cast<const TernaryExpr *>(expr)) {
            return isAssignedInExpr(t->condition.get(), varName) || isAssignedInExpr(t->thenBranch.get(), varName) || isAssignedInExpr(t->elseBranch.get(), varName);
        }
        return false;
    }

    bool isAssignedIn(const Stmt *stmt, const std::string &varName) const {
        if (!stmt) return false;
        if (auto *b = dynamic_cast<const BlockStmt *>(stmt)) {
            for (const auto &s : b->statements) {
                if (isAssignedIn(s.get(), varName)) return true;
            }
            return false;
        }
        if (auto *es = dynamic_cast<const ExprStmt *>(stmt)) {
            return isAssignedInExpr(es->expression.get(), varName);
        }
        if (auto *vs = dynamic_cast<const VarDeclStmt *>(stmt)) {
            return vs->initializer && isAssignedInExpr(vs->initializer.get(), varName);
        }
        if (auto *is = dynamic_cast<const IfStmt *>(stmt)) {
            if (isAssignedInExpr(is->condition.get(), varName)) return true;
            if (isAssignedIn(is->thenBranch.get(), varName)) return true;
            if (is->elseBranch && isAssignedIn(is->elseBranch.get(), varName)) return true;
            return false;
        }
        if (auto *ws = dynamic_cast<const WhileStmt *>(stmt)) {
            if (isAssignedInExpr(ws->condition.get(), varName)) return true;
            if (isAssignedIn(ws->body.get(), varName)) return true;
            return false;
        }
        if (auto *gs = dynamic_cast<const GhoomStmt *>(stmt)) {
            if (isAssignedIn(gs->body.get(), varName)) return true;
            return false;
        }
        if (auto *gms = dynamic_cast<const GhumaMeinStmt *>(stmt)) {
            if (isAssignedIn(gms->body.get(), varName)) return true;
            return false;
        }
        if (auto *rs = dynamic_cast<const ReturnStmt *>(stmt)) {
            return rs->value && isAssignedInExpr(rs->value.get(), varName);
        }
        if (auto *th = dynamic_cast<const ThrowStmt *>(stmt)) {
            return th->expression && isAssignedInExpr(th->expression.get(), varName);
        }
        if (auto *tc = dynamic_cast<const TryCatchStmt *>(stmt)) {
            if (isAssignedIn(tc->tryBlock.get(), varName)) return true;
            if (tc->catchBlock && isAssignedIn(tc->catchBlock.get(), varName)) return true;
            if (tc->finallyBlock && isAssignedIn(tc->finallyBlock.get(), varName)) return true;
            return false;
        }
        return false;
    }

    bool isAssignedInProgram(const std::vector<std::unique_ptr<Stmt>> &stmts, const std::string &varName) const {
        for (const auto &s : stmts) {
            if (isAssignedIn(s.get(), varName)) return true;
        }
        return false;
    }

    void generateClass(const ClassDeclStmt *c) {
        currentClassTypedFields.clear();
        currentClassFieldTypes.clear();
        for (const auto &cf : c->fields) {
            if (cf.typeAnnotation) {
                currentClassFieldTypes[cf.name] = cf.typeAnnotation;
                std::string cppT = toCppType(cf.typeAnnotation, c->typeParams, false);
                if (cppT != "MemeValue") {
                    currentClassTypedFields.insert(cf.name);
                }
            }
        }
        std::string className = c->name;
        if (!c->typeParams.empty()) {
            globalDecls << "template<";
            for (size_t i = 0; i < c->typeParams.size(); ++i) {
                if (i > 0) globalDecls << ", ";
                globalDecls << "typename " << c->typeParams[i];
            }
            globalDecls << ">\n";
            std::string tArgs;
            for (size_t i = 0; i < c->typeParams.size(); ++i) {
                if (i > 0) tArgs += ", ";
                tArgs += c->typeParams[i];
            }
            if (c->line > 0 && !currentSourceFile.empty()) {
                globalDecls << "#line " << c->line << " \"" << escapeString(currentSourceFile) << "\"\n";
            }
            globalDecls << "struct " << className << " : public std::enable_shared_from_this<" << className << "<" << tArgs << ">> {\n";
        } else {
            if (c->line > 0 && !currentSourceFile.empty()) {
                globalDecls << "#line " << c->line << " \"" << escapeString(currentSourceFile) << "\"\n";
            }
            globalDecls << "struct " << className << " : public std::enable_shared_from_this<" << className << "> {\n";
        }

        // Collect fields from constructor assignments khud.x = ... and c->fields
        std::unordered_set<std::string> fields;
        for (const auto &f : c->fields) {
            fields.insert(f.name);
        }
        for (const auto &m : c->methods) {
            if (m->name == "shuru" || m->name == "init" || m->name == "constructor" || m->name == "naya" || m->name == "paida") {
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
        std::unordered_set<std::string> emittedFields;
        for (const auto &cf : c->fields) {
            std::string fType = "MemeValue";
            if (cf.typeAnnotation) {
                fType = toCppType(cf.typeAnnotation, c->typeParams, false);
            }
            globalDecls << "    " << fType << " " << sanitizeIdentifier(cf.name) << " = {};\n";
            emittedFields.insert(cf.name);
        }
        for (const auto &f : fields) {
            if (emittedFields.count(f)) continue;
            if (f == "left" || f == "right") {
                globalDecls << "    std::shared_ptr<" << className << "> " << f << " = nullptr;\n";
            } else {
                globalDecls << "    MemeValue " << f << " = 0LL;\n";
            }
        }

        bool hasZeroArgCtor = false;
        bool hasAnyCtor = false;
        for (const auto &m : c->methods) {
            if (m->name == "shuru" || m->name == "init" || m->name == "constructor" || m->name == "naya" || m->name == "paida") {
                hasAnyCtor = true;
                if (m->paramList.empty()) {
                    hasZeroArgCtor = true;
                }
            }
        }
        if (!hasZeroArgCtor) {
            globalDecls << "\n    " << className << "() = default;\n";
        }
        if (!hasAnyCtor && !c->fields.empty()) {
            globalDecls << "    " << className << "(";
            for (size_t i = 0; i < c->fields.size(); ++i) {
                if (i > 0) globalDecls << ", ";
                globalDecls << "auto " << sanitizeIdentifier(c->fields[i].name) << "_in";
            }
            globalDecls << ") : ";
            for (size_t i = 0; i < c->fields.size(); ++i) {
                if (i > 0) globalDecls << ", ";
                globalDecls << sanitizeIdentifier(c->fields[i].name) << "(" << sanitizeIdentifier(c->fields[i].name) << "_in)";
            }
            globalDecls << " {}\n";
        }

        // Constructor & Methods
        for (const auto &m : c->methods) {
            if (m->line > 0 && !currentSourceFile.empty()) {
                globalDecls << "#line " << m->line << " \"" << escapeString(currentSourceFile) << "\"\n";
            }
            if (m->name == "shuru" || m->name == "init" || m->name == "constructor" || m->name == "naya" || m->name == "paida") {
                globalDecls << "    " << className << "(";
                for (size_t i = 0; i < m->paramList.size(); ++i) {
                    if (i > 0) globalDecls << ", ";
                    globalDecls << "auto " << sanitizeIdentifier(m->paramList[i].name);
                }
                globalDecls << ") {\n";
                indentLevel = 2;
                {
                    FunctionScopeGuard guard(*this, m->body.get());
                    globalDecls << indent() << "MemeCallDepthGuard _depth_guard(\"" << sanitizeIdentifier(m->name) << "\");\n";
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
                    FunctionScopeGuard guard(*this, m->body.get());
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
                    methodRetType = toCppType(m->returnTypeAnnotation, c->typeParams, false);
                } else if (hasGetExprInReturn(m->body.get()) || usesPropagate || hasFinallyWithReturn(m->body.get())) {
                    methodRetType = "MemeValue";
                }

                globalDecls << "    " << methodRetType << " " << sanitizeIdentifier(m->name) << "(";
                for (size_t i = 0; i < m->paramList.size(); ++i) {
                    if (i > 0) globalDecls << ", ";
                    std::string pType = "const MemeValue &";
                    if (m->paramList[i].typeAnnotation) {
                        pType = toCppType(m->paramList[i].typeAnnotation, c->typeParams, /*isParam=*/true);
                    }
                    globalDecls << pType << " " << sanitizeIdentifier(m->paramList[i].name);
                }
                globalDecls << ") {\n";
                indentLevel = 2;
                {
                    FunctionScopeGuard guard(*this, m->body.get());
                    globalDecls << indent() << "MemeCallDepthGuard _depth_guard(\"" << sanitizeIdentifier(m->name) << "\");\n";
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
        currentClassTypedFields.clear();
        currentClassFieldTypes.clear();
    }

    std::string getParamTypeString(const FuncParam &param, const std::vector<std::string> &typeParams) {
        if (param.typeAnnotation) {
            std::string t = toCppType(param.typeAnnotation, typeParams, /*isParam=*/true);
            return t + " ";
        }
        return "const MemeValue &";
    }

    bool canHaveTrailingDefault(const std::vector<FuncParam> &plist, size_t idx) const {
        for (size_t j = idx; j < plist.size(); ++j) {
            if (!plist[j].isMapDestructure && !plist[j].defaultValue && !plist[j].isVariadic) {
                return false;
            }
        }
        return true;
    }

    std::string buildFuncParamList(const FuncDeclStmt *f, bool includeDefaults) {
        std::string paramDecl;
        for (size_t i = 0; i < f->paramList.size(); ++i) {
            if (i > 0) paramDecl += ", ";
            const auto &p = f->paramList[i];
            if (p.isMapDestructure) {
                paramDecl += "const MemeValue &_mp_" + std::to_string(i);
                if (includeDefaults && canHaveTrailingDefault(f->paramList, i)) {
                    paramDecl += " = MemeValue()";
                }
            } else {
                std::string tStr = getParamTypeString(p, f->typeParams);
                paramDecl += tStr + sanitizeIdentifier(p.name);
                if (includeDefaults && (p.defaultValue || (p.isVariadic && !p.typeAnnotation)) && canHaveTrailingDefault(f->paramList, i)) {
                    if (p.defaultValue && tStr == "const MemeValue &") {
                        paramDecl += " = MemeValue(" + const_cast<CodeGenerator *>(this)->generateExpr(p.defaultValue.get()) + ")";
                    } else if (!p.defaultValue && tStr == "const MemeValue &") {
                        paramDecl += " = MemeValue()";
                    }
                }
            }
        }
        return paramDecl;
    }

    void generateFunction(const FuncDeclStmt *f) {
        std::string fnName = sanitizeIdentifier(f->name);
        std::string returnType = "MemeValue";
        bool usesPropagate = hasTryPropagate(f->body.get());

        std::string classRet = findClassReturned(f->body.get());
        if (!classRet.empty()) {
            returnType = "std::shared_ptr<" + classRet + ">";
        } else if (f->returnTypeAnnotation) {
            returnType = toCppType(f->returnTypeAnnotation, f->typeParams, false);
        } else if (hasGetExprInReturn(f->body.get()) || usesPropagate || hasFinallyWithReturn(f->body.get())) {
            returnType = "MemeValue";
        }

        std::string paramDecl = buildFuncParamList(f, false);

        if (!f->typeParams.empty()) {
            globalDecls << "template<";
            for (size_t i = 0; i < f->typeParams.size(); ++i) {
                if (i > 0) globalDecls << ", ";
                globalDecls << "typename " << f->typeParams[i];
            }
            globalDecls << ">\n";
        }
        if (f->line > 0 && !currentSourceFile.empty()) {
            globalDecls << "#line " << f->line << " \"" << escapeString(currentSourceFile) << "\"\n";
        }
        globalDecls << "inline " << returnType << " " << fnName << "(" << paramDecl << ") {\n";
        indentLevel = 1;
        {
            FunctionScopeGuard guard(*this, f->body.get());
            pushVarScope();
            for (size_t i = 0; i < f->paramList.size(); ++i) {
                const auto &p = f->paramList[i];
                if (p.isMapDestructure) {
                    for (const auto &dk : p.destructureKeys) {
                        declareScopeVar(dk.name);
                        std::string defStr = dk.defaultValue ? ("MemeValue(" + generateExpr(dk.defaultValue.get()) + ")") : "MemeValue()";
                        globalDecls << indent() << "MemeValue " << sanitizeIdentifier(dk.name) << " = meme_map_destruct_get(_mp_" << i << ", \"" << escapeString(dk.name) << "\", " << defStr << ");\n";
                    }
                } else {
                    declareScopeVar(p.name);
                }
            }
            globalDecls << indent() << "MemeCallDepthGuard _depth_guard(\"" << fnName << "\");\n";
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
            popVarScope();
        }
        globalDecls << "}\n\n";

        if (f->typeParams.empty()) {
            globalDecls << "inline MemeValue " << fnName << "_wrap(const std::vector<MemeValue> &_args) {\n";
            if (f->paramList.empty()) {
                globalDecls << "    (void)_args;\n";
            }
            std::string callArgs;
            for (size_t i = 0; i < f->paramList.size(); ++i) {
                if (i > 0) callArgs += ", ";
                const auto &p = f->paramList[i];
                if (p.isMapDestructure) {
                    callArgs += "(" + std::to_string(i) + " < _args.size() ? _args[" + std::to_string(i) + "] : make_map({}))";
                } else {
                    std::string pt = getParamTypeString(p, f->typeParams);
                    std::string defExpr = p.defaultValue ? ("MemeValue(" + generateExpr(p.defaultValue.get()) + ")") : "MemeValue()";
                    std::string argVal = "(" + std::to_string(i) + " < _args.size() ? _args[" + std::to_string(i) + "] : " + defExpr + ")";
                    if (pt == "const MemeValue &" || pt == "MemeValue ") {
                        callArgs += argVal;
                    } else if (pt == "int64_t ") {
                        callArgs += argVal + ".as_int()";
                    } else if (pt == "double ") {
                        callArgs += argVal + ".as_double()";
                    } else if (pt == "bool ") {
                        callArgs += argVal + ".is_truthy()";
                    } else if (pt == "const std::string &" || pt == "std::string ") {
                        callArgs += argVal + ".to_string()";
                    } else if (pt.find("std::shared_ptr<") != std::string::npos) {
                        size_t lt = pt.find('<');
                        size_t gt = pt.rfind('>');
                        std::string innerT = (lt != std::string::npos && gt != std::string::npos && gt > lt) ? pt.substr(lt + 1, gt - lt - 1) : "void";
                        callArgs += "std::static_pointer_cast<" + innerT + ">(" + argVal + ".obj_val)";
                    } else if (pt.find("std::vector<MemeValue>") != std::string::npos) {
                        callArgs += "([&]() -> std::vector<MemeValue> { MemeValue _tmp = " + argVal + "; _tmp.ensure_list(); return *_tmp.list_data; })()";
                    } else if (pt.find("std::vector<uint8_t>") != std::string::npos) {
                        callArgs += "([&]() -> std::vector<uint8_t> { MemeValue _tmp = " + argVal + "; _tmp.ensure_bytes(); return *_tmp.bytes_val; })()";
                    } else {
                        callArgs += argVal;
                    }
                }
            }
            if (returnType == "void") {
                globalDecls << "    " << fnName << "(" << callArgs << ");\n    return MemeValue();\n";
            } else {
                globalDecls << "    return MemeValue(" << fnName << "(" << callArgs << "));\n";
            }
            globalDecls << "}\n\n";
        }
    }

    std::string buildSource() {
        std::ostringstream out;
        out << "// ==================================================================\n";
        out << "// Automatically generated by MEMElang AOT Native Compiler\n";
        out << "// ==================================================================\n\n";
        if (programUsesEval) {
            out << "#ifndef MEME_HAS_EVAL\n#define MEME_HAS_EVAL 1\n#endif\n";
        }
        if (programUsesJsBridge || programUsesEval) {
            out << "#ifndef MEME_HAS_JS_BRIDGE\n#define MEME_HAS_JS_BRIDGE 1\n#endif\n";
        }
        out << "#include \"src/compiler/MemeRuntime.h\"\n";
        if (programUsesUi) {
            out << "#include \"src/compiler/MemeRuntimeUi.h\"\n";
        }
        out << "\n";

        // Global declarations (classes, functions)
        out << globalDecls.str();

        // Main entrypoint
        out << "#ifdef MEME_HOST_PROVIDES_MAIN\n";
        out << "extern \"C\" int meme_user_entrypoint(int argc, char* argv[]) {\n";
        out << "#else\n";
        out << "int main(int argc, char* argv[]) {\n";
        out << "#endif\n";
        out << "    sys.args.clear();\n";
        out << "    for (int _i = 0; _i < argc; ++_i) {\n";
        out << "        sys.args.push_back(std::string(argv[_i]));\n";
        out << "    }\n";
        if (explicitTargetPlatform) {
            out << "    MemePlatform::setRuntimePlatformOverride(\"" << escapeString(targetPlatform) << "\");\n";
        }
        out << "    env.auto_load();\n\n";
        out << "    try {\n";
        out << mainBody.str();
        out << "\n    } catch (const MemeReturnSignal &_sig) {\n";
        out << "        return 0;\n";
        out << "    } catch (const MemeValue &_mv) {\n";
        out << "        std::cerr << \"\\n❌ Gadbad ho gayi:\\n\" << _mv.to_string() << \"\\n\\n\";\n";
        out << "        return 1;\n";
        out << "    } catch (const std::exception &_e) {\n";
        out << "        std::cerr << \"\\n❌ Gadbad ho gayi:\\n\" << _e.what() << \"\\n\\n\";\n";
        out << "        return 1;\n";
        out << "    }\n";
        out << "\n    return 0;\n";
        out << "}\n";

        return out.str();
    }

    std::unordered_set<std::string> importedModulesProcessed;
    std::unordered_map<std::string, std::string> processedModulePrimaryAlias;

    std::filesystem::path resolveModuleFile(const std::string &modPath) {
        std::vector<std::filesystem::path> candidates;
        if (modPath.rfind("@/", 0) == 0) {
            std::string rel = modPath.substr(2);
            std::filesystem::path root = std::filesystem::current_path();
            if (!currentSourceFile.empty()) {
                std::filesystem::path check = std::filesystem::path(currentSourceFile).parent_path();
                while (!check.empty()) {
                    if (std::filesystem::exists(check / "tapri.json") || std::filesystem::exists(check / ".meme_env")) {
                        root = check;
                        break;
                    }
                    if (!check.has_relative_path() || check == check.parent_path()) break;
                    check = check.parent_path();
                }
            }
            candidates.push_back(root / rel);
            candidates.push_back(root / "src" / rel);
        }
        if (!currentSourceFile.empty()) {
            std::filesystem::path baseDir = std::filesystem::path(currentSourceFile).parent_path();
            if (!baseDir.empty()) {
                candidates.push_back(baseDir / modPath);
            }
        }
        candidates.push_back(std::filesystem::path(".tapri_modules") / modPath);
        candidates.push_back(std::filesystem::path(".meme_env") / "pkgs" / modPath);
        candidates.push_back(std::filesystem::path(modPath));
        for (const auto &cand : candidates) {
            std::filesystem::path hit = MemePlatform::probeCandidateBase(cand, targetPlatform);
            if (!hit.empty()) {
                std::error_code ec;
                std::filesystem::path canon = std::filesystem::canonical(hit, ec);
                return ec ? hit : canon;
            }
        }
        return "";
    }

    void processModuleImport(const std::string &path, const std::string &alias = "") {
        std::filesystem::path resolved = resolveModuleFile(path);
        if (resolved.empty()) return;
        std::string canStr = resolved.string();
        if (importedModulesProcessed.find(canStr) != importedModulesProcessed.end()) {
            std::string reqAlias = alias.empty() ? std::filesystem::path(path).stem().string() : alias;
            reqAlias = sanitizeIdentifier(reqAlias);
            auto pit = processedModulePrimaryAlias.find(canStr);
            if (!reqAlias.empty() && pit != processedModulePrimaryAlias.end() && reqAlias != pit->second &&
                emittedPass0NamedImports.insert("_mod_" + reqAlias).second) {
                if (programUsesUi) {
                    knownModuleAliases.erase(reqAlias);
                    globalDecls << "inline MemeValue " << reqAlias << " = " << pit->second << ";\n";
                } else {
                    globalDecls << "inline auto &" << reqAlias << " = " << pit->second << ";\n";
                }
            }
            return;
        }
        importedModulesProcessed.insert(canStr);

        std::ifstream file(resolved);
        if (!file.is_open()) return;
        std::stringstream ss;
        ss << file.rdbuf();
        std::string prevSourceFile = currentSourceFile;
        currentSourceFile = resolved.string();
        try {
            Lexer modLexer(ss.str());
            auto modTokens = modLexer.tokenize();
            Parser modParser(modTokens);
            auto modStmts = modParser.parseProgram();

            scanDeclarations(modStmts);

            // Hoist module top-level vars if UI is active so nested functions can read module constants
            if (programUsesUi) {
                for (const auto &stmt : modStmts) {
                    if (auto *v = dynamic_cast<const VarDeclStmt *>(stmt.get())) {
                        std::string vn = sanitizeIdentifier(v->name);
                        if (!v->name.empty() && topLevelVars.insert(v->name).second) {
                            globalDecls << "inline MemeValue " << vn << ";\n";
                        }
                    }
                }
            }

            // Process nested imports first (both builtin modules and local .meme modules)
            for (const auto &stmt : modStmts) {
                if (auto *nestedImp = dynamic_cast<const ImportStmt *>(stmt.get())) {
                    if (!emitBuiltinOrStdImport(nestedImp)) {
                        std::string nAlias = nestedImp->alias.empty() ? std::filesystem::path(nestedImp->path).stem().string() : nestedImp->alias;
                        processModuleImport(nestedImp->path, sanitizeIdentifier(nAlias));
                    }
                }
            }

            // Forward declare classes from module
            for (const auto &stmt : modStmts) {
                if (auto *c = dynamic_cast<const ClassDeclStmt *>(stmt.get())) {
                    globalDecls << "struct " << sanitizeIdentifier(c->name) << ";\n";
                }
            }

            // Forward declare functions from module
            for (const auto &stmt : modStmts) {
                if (auto *f = dynamic_cast<const FuncDeclStmt *>(stmt.get())) {
                    std::string fnName = sanitizeIdentifier(f->name);
                    std::string paramTypes = buildFuncParamList(f, true);
                    globalDecls << "inline MemeValue " << fnName << "(" << paramTypes << ");\n";
                    if (f->typeParams.empty()) {
                        globalDecls << "inline MemeValue " << fnName << "_wrap(const std::vector<MemeValue> &_args);\n";
                        globalDecls << "inline const MemeValue " << fnName << "_fn = meme_fn(" << fnName << "_wrap);\n";
                    }
                }
            }

            // Emit classes and functions from module
            for (const auto &stmt : modStmts) {
                if (auto *c = dynamic_cast<const ClassDeclStmt *>(stmt.get())) {
                    generateClass(c);
                } else if (auto *f = dynamic_cast<const FuncDeclStmt *>(stmt.get())) {
                    generateFunction(f);
                } else if (!dynamic_cast<const ImportStmt *>(stmt.get())) {
                    inMainPass = true;
                    generateStmt(stmt.get(), mainBody);
                    inMainPass = false;
                }
            }

            std::string modAlias = alias;
            if (modAlias.empty()) {
                modAlias = std::filesystem::path(path).stem().string();
            }
            modAlias = sanitizeIdentifier(modAlias);
            if (!modAlias.empty() && emittedPass0NamedImports.insert("_mod_" + modAlias).second) {
                processedModulePrimaryAlias[canStr] = modAlias;
                std::vector<std::string> exportedFns;
                for (const auto &stmt : modStmts) {
                    if (auto *f = dynamic_cast<const FuncDeclStmt *>(stmt.get())) {
                        if (f->typeParams.empty()) exportedFns.push_back(sanitizeIdentifier(f->name));
                    }
                }
                if (programUsesUi) {
                    knownModuleAliases.erase(modAlias);
                    globalDecls << "inline MemeValue " << modAlias << " = make_map({";
                    for (size_t ei = 0; ei < exportedFns.size(); ++ei) {
                        if (ei > 0) globalDecls << ", ";
                        globalDecls << "{\"" << exportedFns[ei] << "\", " << exportedFns[ei] << "_fn}";
                    }
                    globalDecls << "});\n\n";
                } else {
                    globalDecls << "struct _MemeMod_" << modAlias << " {\n";
                    for (const auto &fnName : exportedFns) {
                        globalDecls << "    template<typename... Args>\n";
                        globalDecls << "    decltype(auto) " << fnName << "(Args&&... args) const {\n";
                        globalDecls << "        return ::" << fnName << "(std::forward<Args>(args)...);\n";
                        globalDecls << "    }\n";
                    }
                    globalDecls << "};\n";
                    globalDecls << "inline _MemeMod_" << modAlias << " " << modAlias << ";\n\n";
                }
            }
        } catch (...) {}
        currentSourceFile = prevSourceFile;
    }

    bool emitBuiltinOrStdImport(const ImportStmt *imp) {
        if (!imp) return false;
        std::string alias = imp->alias.empty() ? std::filesystem::path(imp->path).stem().string() : imp->alias;
        alias = sanitizeIdentifier(alias);
        if (imp->path == "samay") {
            if (alias != "samay" && emittedPass0NamedImports.insert("_mod_" + alias).second) {
                globalDecls << "inline SamayModule &" << alias << " = samay;\n";
            }
            return true;
        }
        if (imp->path == "itertools") {
            if (alias != "itertools" && emittedPass0NamedImports.insert("_mod_" + alias).second) {
                globalDecls << "inline ItertoolsModule &" << alias << " = itertools;\n";
            }
            return true;
        }
        if (imp->path == "math") {
            if (alias != "math" && emittedPass0NamedImports.insert("_mod_" + alias).second) {
                globalDecls << "inline MathModule &" << alias << " = math;\n";
            }
            return true;
        }
        if (imp->path == "sys" || imp->path == "systumm" || imp->path == "os") {
            if (alias != "sys" && alias != "systumm" && alias != "os" && emittedPass0NamedImports.insert("_mod_" + alias).second) {
                globalDecls << "inline SystummModule &" << alias << " = sys;\n";
            }
            return true;
        }
        if (imp->path == "env" || imp->path == "paryavaran" || imp->path == "dotenv") {
            if (alias != "env" && alias != "paryavaran" && alias != "dotenv" && emittedPass0NamedImports.insert("_mod_" + alias).second) {
                globalDecls << "inline EnvModule &" << alias << " = env;\n";
            }
            return true;
        }
        if (imp->path == "file" || imp->path == "fs" || imp->path == "dastavez") {
            if (alias != "file" && alias != "fs" && alias != "dastavez" && emittedPass0NamedImports.insert("_mod_" + alias).second) {
                globalDecls << "inline FileModule &" << alias << " = file;\n";
            }
            return true;
        }
        if (imp->path == "json") {
            if (alias != "json" && emittedPass0NamedImports.insert("_mod_" + alias).second) {
                globalDecls << "inline JsonModule &" << alias << " = json;\n";
            }
            return true;
        }
        if (imp->path == "csv") {
            if (alias != "csv" && emittedPass0NamedImports.insert("_mod_" + alias).second) {
                globalDecls << "inline CsvModule &" << alias << " = csv;\n";
            }
            return true;
        }
        if (imp->path == "net" || imp->path == "socket") {
            if (alias != "net" && alias != "m_socket" && alias != "socket" && emittedPass0NamedImports.insert("_mod_" + alias).second) {
                globalDecls << "inline NetModule &" << alias << " = net;\n";
            }
            return true;
        }
        if (imp->path == "ui:shadcn") {
            if (!imp->namedImports.empty()) {
                for (const auto &ni : imp->namedImports) {
                    std::string sym = sanitizeIdentifier(ni);
                    if (ni == "Router" || ni == "Route" || ni == "Link" || ni == "Platform") continue;
                    if (!emittedPass0NamedImports.insert(sym).second) continue;
                    globalDecls << "inline MemeValue " << sym << " = meme_index(ui_shadcn, \"" << escapeString(ni) << "\");\n";
                }
            } else if (!alias.empty() && alias != "ui_shadcn" && emittedPass0NamedImports.insert("_mod_" + alias).second) {
                globalDecls << "inline MemeValue " << alias << " = ui_shadcn;\n";
            }
            return true;
        }
        if (imp->path == "ui:icons" || imp->path == "lucide-react") {
            if (!imp->namedImports.empty()) {
                for (const auto &ni : imp->namedImports) {
                    std::string sym = sanitizeIdentifier(ni);
                    if (!emittedPass0NamedImports.insert(sym).second) continue;
                    globalDecls << "inline MemeValue " << sym << " = meme_index(ui_icons, \"" << escapeString(ni) << "\");\n";
                }
            } else if (!alias.empty() && alias != "ui_icons" && emittedPass0NamedImports.insert("_mod_" + alias).second) {
                globalDecls << "inline MemeValue " << alias << " = ui_icons;\n";
            }
            return true;
        }
        if (imp->path == "ui:motion" || imp->path == "framer-motion" || imp->path == "npm:framer-motion") {
            if (!imp->namedImports.empty()) {
                for (const auto &ni : imp->namedImports) {
                    std::string sym = sanitizeIdentifier(ni);
                    if (programUsesUi && ni == "useSpring") continue;
                    if (!emittedPass0NamedImports.insert(sym).second) continue;
                    globalDecls << "inline MemeValue " << sym << " = meme_index(ui_motion, \"" << escapeString(ni) << "\");\n";
                }
            } else if (!alias.empty() && alias != "ui_motion" && emittedPass0NamedImports.insert("_mod_" + alias).second) {
                globalDecls << "inline MemeValue " << alias << " = ui_motion;\n";
            }
            return true;
        }
        if (imp->path == "ui:icons" || imp->path == "lucide-react" || imp->path == "npm:lucide-react") {
            if (!imp->namedImports.empty()) {
                for (const auto &ni : imp->namedImports) {
                    std::string sym = sanitizeIdentifier(ni);
                    if (!emittedPass0NamedImports.insert(sym).second) continue;
                    globalDecls << "inline MemeValue " << sym << " = meme_index(ui_icons, \"" << escapeString(ni) << "\");\n";
                }
            } else if (!alias.empty() && alias != "ui_icons" && emittedPass0NamedImports.insert("_mod_" + alias).second) {
                globalDecls << "inline MemeValue " << alias << " = ui_icons;\n";
            }
            return true;
        }
        if (imp->path == "@radix-ui/react-popover" || imp->path == "@radix-ui/react-dialog" ||
            imp->path == "@radix-ui/react-tabs" || imp->path == "npm:@radix-ui/react-popover" ||
            imp->path == "npm:@radix-ui/react-dialog" || imp->path == "npm:@radix-ui/react-tabs") {
            if (!imp->namedImports.empty()) {
                for (const auto &ni : imp->namedImports) {
                    std::string sym = sanitizeIdentifier(ni);
                    if (!emittedPass0NamedImports.insert(sym).second) continue;
                    globalDecls << "inline MemeValue " << sym << " = meme_index(ui_shadcn, \"" << escapeString(ni) << "\");\n";
                }
            } else if (!alias.empty() && emittedPass0NamedImports.insert("_mod_" + alias).second) {
                globalDecls << "inline MemeValue " << alias << " = ui_shadcn;\n";
            }
            return true;
        }
        if (imp->path.rfind("npm:", 0) == 0 || imp->path.rfind("js:", 0) == 0) {
            std::string pkgName = (imp->path.rfind("npm:", 0) == 0) ? imp->path.substr(4) : imp->path.substr(3);
            std::string modVar = "_jsm_p0_" + sanitizeIdentifier(pkgName);
            if (emittedPass0NamedImports.insert("_jsm_loaded_" + pkgName).second) {
                globalDecls << "inline MemeValue " << modVar << " = meme_load_npm_module(\"" << escapeString(pkgName) << "\");\n";
            }
            if (!imp->namedImports.empty()) {
                for (const auto &ni : imp->namedImports) {
                    std::string sym = sanitizeIdentifier(ni);
                    if (!emittedPass0NamedImports.insert(sym).second) continue;
                    globalDecls << "inline MemeValue " << sym << " = meme_index(" << modVar << ", \"" << escapeString(ni) << "\");\n";
                }
            } else if (!alias.empty() && emittedPass0NamedImports.insert("_mod_" + alias).second) {
                globalDecls << "inline MemeValue " << alias << " = " << modVar << ";\n";
            }
            return true;
        }
        if (imp->path == "ui:primitives" || imp->path == "primitives" || imp->path == "vercel:ui") {
            if (!imp->namedImports.empty()) {
                for (const auto &ni : imp->namedImports) {
                    std::string sym = sanitizeIdentifier(ni);
                    if (!emittedPass0NamedImports.insert(sym).second) continue;
                    globalDecls << "inline MemeValue " << sym << " = meme_index(ui_primitives, \"" << escapeString(ni) << "\");\n";
                }
            } else if (!alias.empty() && alias != "ui_primitives" && emittedPass0NamedImports.insert("_mod_" + alias).second) {
                globalDecls << "inline MemeValue " << alias << " = ui_primitives;\n";
            }
            return true;
        }
        if (imp->path == "ui" || imp->path == "egui" || imp->path == "view" || imp->path == "gui") {
            std::string srcMod = (imp->path == "egui") ? "egui" : "ui";
            static const std::unordered_set<std::string> kUiGlobalFns = {
                "useState", "state_bana", "useEffect", "useMemo", "useCallback",
                "useReducer", "useRef", "createContext", "useContext", "useRouter",
                "Router", "Route", "Link", "Platform"
            };
            if (!imp->namedImports.empty()) {
                for (const auto &ni : imp->namedImports) {
                    if (programUsesUi && kUiGlobalFns.count(ni)) continue;
                    std::string sym = sanitizeIdentifier(ni);
                    if (!emittedPass0NamedImports.insert(sym).second) continue;
                    globalDecls << "inline MemeValue " << sym << " = meme_index(" << srcMod << ", \"" << escapeString(ni) << "\");\n";
                }
            } else if (!alias.empty() && alias != "ui" && alias != "egui" && alias != "view" && alias != "gui" &&
                       emittedPass0NamedImports.insert("_mod_" + alias).second) {
                globalDecls << "inline MemeValue " << alias << " = " << srcMod << ";\n";
            }
            return true;
        }
        if (imp->path == "native" || imp->path == "app" || imp->path == "device" ||
            imp->path == "platform" || imp->path == "permissions" || imp->path == "perms") {
            std::string srcMod = (imp->path == "app") ? "app"
                : (imp->path == "device" ? "meme_index(native, \"device\")"
                : ((imp->path == "permissions" || imp->path == "perms") ? "permissions"
                : (imp->path == "platform" ? "platform" : "native")));
            if (!imp->namedImports.empty()) {
                for (const auto &ni : imp->namedImports) {
                    std::string targetSym = isCollidingGlobalSymbol(ni) ? ("_imp_" + sanitizeIdentifier(ni)) : sanitizeIdentifier(ni);
                    if (isCollidingGlobalSymbol(ni)) renamedImports[ni] = targetSym;
                    if (!emittedPass0NamedImports.insert(targetSym).second) continue;
                    if (imp->path == "platform" && ni == "Platform" && programUsesUi) {
                        globalDecls << "inline MemeValue " << targetSym << " = Platform_fn;\n";
                    } else {
                        globalDecls << "inline MemeValue " << targetSym << " = meme_index(" << srcMod << ", \"" << escapeString(ni) << "\");\n";
                    }
                }
            } else if (!alias.empty() && alias != "native" && alias != "app" && alias != "platform" && alias != "permissions" && alias != "perms" &&
                       emittedPass0NamedImports.insert("_mod_" + alias).second) {
                globalDecls << "inline MemeValue " << alias << " = " << srcMod << ";\n";
            }
            return true;
        }
        if (imp->path == "meme") {
            if (!imp->namedImports.empty()) {
                for (const auto &ni : imp->namedImports) {
                    std::string sym = sanitizeIdentifier(ni);
                    if (!emittedPass0NamedImports.insert(sym).second) continue;
                    globalDecls << "inline MemeValue " << sym << " = meme_index(meme, \"" << escapeString(ni) << "\");\n";
                }
            } else if (!alias.empty() && alias != "meme" && emittedPass0NamedImports.insert("_mod_" + alias).second) {
                globalDecls << "inline MemeValue " << alias << " = meme;\n";
            }
            return true;
        }
        return false;
    }

    void generateProgram(const std::vector<std::unique_ptr<Stmt>> &program) {
        currentProgram = &program;
        currentScopeBody = nullptr;
        scanDeclarations(program);

        // Pass 0: Top-level imports
        for (const auto &stmt : program) {
            if (auto *imp = dynamic_cast<const ImportStmt *>(stmt.get())) {
                if (!emitBuiltinOrStdImport(imp)) {
                    std::string alias = imp->alias.empty() ? std::filesystem::path(imp->path).stem().string() : imp->alias;
                    processModuleImport(imp->path, sanitizeIdentifier(alias));
                }
            }
        }

        // Pass 0C: Hoist top-level variables for UI modules & components
        if (programUsesUi) {
            auto hoistVar = [&](const std::string &rawName) {
                std::string vn = sanitizeIdentifier(rawName);
                if (!rawName.empty() && topLevelVars.insert(rawName).second) {
                    globalDecls << "inline MemeValue " << vn << ";\n";
                }
            };
            for (const auto &stmt : program) {
                if (auto *v = dynamic_cast<const VarDeclStmt *>(stmt.get())) {
                    hoistVar(v->name);
                } else if (auto *ads = dynamic_cast<const ArrayDestructureStmt *>(stmt.get())) {
                    for (const auto &it : ads->items) hoistVar(it.name);
                } else if (auto *mds = dynamic_cast<const MapDestructureStmt *>(stmt.get())) {
                    for (const auto &k : mds->keys) hoistVar(k);
                } else if (auto *es = dynamic_cast<const ExprStmt *>(stmt.get())) {
                    if (auto *da = dynamic_cast<const DestructureAssignExpr *>(es->expression.get())) {
                        for (const auto &it : da->items) hoistVar(it.name);
                    }
                }
            }
        }

        // Pass 0A: Forward declarations for all classes
        for (const auto &stmt : program) {
            if (auto *c = dynamic_cast<const ClassDeclStmt *>(stmt.get())) {
                if (!c->typeParams.empty()) {
                    globalDecls << "template<";
                    for (size_t i = 0; i < c->typeParams.size(); ++i) {
                        if (i > 0) globalDecls << ", ";
                        globalDecls << "typename " << c->typeParams[i] << " = MemeValue";
                    }
                    globalDecls << ">\nstruct " << sanitizeIdentifier(c->name) << ";\n";
                } else {
                    globalDecls << "struct " << sanitizeIdentifier(c->name) << ";\n";
                }
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
                    retType = toCppType(f->returnTypeAnnotation, f->typeParams, false);
                }
                std::string paramTypes = buildFuncParamList(f, true);
                if (!f->typeParams.empty()) {
                    globalDecls << "template<";
                    for (size_t i = 0; i < f->typeParams.size(); ++i) {
                        if (i > 0) globalDecls << ", ";
                        globalDecls << "typename " << f->typeParams[i] << " = MemeValue";
                    }
                    globalDecls << ">\n";
                }
                globalDecls << "inline " << retType << " " << fnName << "(" << paramTypes << ");\n";
                if (f->typeParams.empty()) {
                    globalDecls << "inline MemeValue " << fnName << "_wrap(const std::vector<MemeValue> &_args);\n";
                    globalDecls << "inline const MemeValue " << fnName << "_fn = meme_fn(" << fnName << "_wrap);\n";
                }
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
        inMainPass = true;
        for (const auto &stmt : program) {
            if (!dynamic_cast<const ClassDeclStmt *>(stmt.get()) &&
                !dynamic_cast<const FuncDeclStmt *>(stmt.get()) &&
                !dynamic_cast<const ImportStmt *>(stmt.get())) {
                generateStmt(stmt.get(), mainBody);
            }
        }
        inMainPass = false;
        currentProgram = nullptr;
    }
};

} // namespace

std::string NativeCompiler::generateCpp(const std::vector<std::unique_ptr<Stmt>> &program,
                                       const std::string &sourceName,
                                       const std::string &targetPlatform) {
    CodeGenerator generator(sourceName, targetPlatform);
    generator.generateProgram(program);
    return generator.buildSource();
}

CompileResult NativeCompiler::compile(const std::vector<std::unique_ptr<Stmt>> &program,
                                      const CompileOptions &options,
                                      const std::string &sourceName) {
    auto startTime = std::chrono::high_resolution_clock::now();
    CompileResult result;

    try {
        std::string srcFile = !options.sourcePath.empty() ? options.sourcePath : (sourceName.find('.') != std::string::npos ? sourceName : sourceName + ".meme");
        std::string effTarget;
        if (!options.target.empty() && options.target != "host" && options.target != "native") {
            effTarget = MemePlatform::normalizePlatformName(options.target);
        }
        std::string cppCode = generateCpp(program, srcFile, effTarget);
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

        bool usesUi = (cppCode.find("#include \"src/compiler/MemeRuntimeUi.h\"") != std::string::npos);
        bool usesEval = (cppCode.find("#define MEME_HAS_EVAL 1") != std::string::npos);
        bool usesJsBridge = (cppCode.find("#define MEME_HAS_JS_BRIDGE 1") != std::string::npos) || usesEval;

        // Build clang++ command with PCH acceleration
        std::filesystem::path pchPath;
        bool usePch = (!usesUi && !usesEval && !usesJsBridge) && ensurePchReady(options, pchPath);

        auto buildClangCmd = [&](bool withPch) {
            std::string c = "clang++ -std=c++20 ";
            if (options.tsan) {
                c += "-fsanitize=thread -g -O1 ";
            } else {
                if (options.debug) {
                    c += "-g -O0 ";
                } else if (options.optimize) {
                    c += "-O3 -DNDEBUG -DMEME_NO_DEPTH_GUARD ";
                } else {
                    c += "-O0 ";
                }
            }
            c += "-I. ";
            if (usesEval) {
                c += "-DMEME_HAS_EVAL -DMEME_NO_INTERP_DESKTOP ";
            }
            if (usesJsBridge && !usesEval) {
                c += "-DMEME_HAS_JS_BRIDGE ";
            }
            if (withPch && !options.tsan) {
                c += "-include-pch \"" + pchPath.string() + "\" ";
            }
            if (usesUi) {
#if defined(__APPLE__)
                if (options.target == "host" || options.target == "mac") {
                    c += "-x objective-c++ ";
                }
#endif
            }
            c += "\"" + cppPath.string() + "\" ";
            if (usesEval) {
                c += "-x c++ \"" + resolveRepoSourceFile("src/interpreter/Interpreter.cpp").string() + "\" \"" +
                     resolveRepoSourceFile("src/types/TypeChecker.cpp").string() + "\" ";
            }
            if (usesUi) {
#if defined(__APPLE__)
                if (options.target == "host" || options.target == "mac") {
                    c += "-framework Cocoa -framework CoreGraphics -framework Metal -framework QuartzCore ";
                }
#endif
            }
            if (usesJsBridge) {
#if defined(__APPLE__)
                c += "-framework JavaScriptCore ";
#else
                std::filesystem::path qd = resolveRepoSourceFile("third_party/quickjs/quickjs.c").parent_path();
                std::filesystem::path qobjDir = std::filesystem::temp_directory_path() / "meme_qjs_host_objs";
                std::filesystem::create_directories(qobjDir);
                for (const char *qn : {"quickjs", "dtoa", "libregexp", "libunicode"}) {
                    std::filesystem::path cf = qd / (std::string(qn) + ".c");
                    std::filesystem::path of = qobjDir / (std::string(qn) + ".o");
                    if (!std::filesystem::exists(of)) {
                        std::string qcmd = "cc -std=c11 -O2 -D_GNU_SOURCE -I\"" + qd.string() + "\" -c \"" + cf.string() + "\" -o \"" + of.string() + "\"";
                        std::system(qcmd.c_str());
                    }
                    c += "\"" + of.string() + "\" ";
                }
                c += "-lm -ldl -lpthread ";
#endif
            }
            c += "-o \"" + binPath.string() + "\" 2>&1";
            return c;
        };

        std::string cmd = buildClangCmd(usePch);

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

        // Fallback retry if compilation with PCH failed
        if (status != 0 && usePch) {
            if (options.verbose) {
                std::cout << "PCH compilation failed, invalidating PCH and retrying directly..." << std::endl;
            }
            std::error_code ec;
            std::filesystem::remove(pchPath, ec);

            std::string fallbackCmd = buildClangCmd(false);
            if (options.verbose) {
                std::cout << "Executing fallback: " << fallbackCmd << std::endl;
            }
            FILE *fallbackPipe = popen(fallbackCmd.c_str(), "r");
            if (fallbackPipe) {
                compilerOutput.clear();
                while (fgets(buffer, sizeof(buffer), fallbackPipe) != nullptr) {
                    compilerOutput += buffer;
                }
                status = pclose(fallbackPipe);
            }
        }

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

        CompileOptions opts = options;
        if (opts.sourcePath.empty()) {
            opts.sourcePath = sourcePath;
        }

        return compile(program, opts, stem);
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
