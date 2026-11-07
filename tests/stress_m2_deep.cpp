#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <memory>
#include "src/lexer/Lexer.h"
#include "src/parser/Parser.h"

int testsPassed = 0;
int testsFailed = 0;

void recordPass(const std::string &name) {
    testsPassed++;
    std::cout << "  [PASS] " << name << std::endl;
}

void recordFail(const std::string &name, const std::string &reason) {
    testsFailed++;
    std::cerr << "  [FAIL] " << name << " -> " << reason << std::endl;
}

#define TEST_ASSERT(cond, name, msg) do { \
    if (cond) { recordPass(name); } \
    else { recordFail(name, msg); } \
} while(0)

std::vector<std::unique_ptr<Stmt>> parseCode(const std::string &code, bool &errorOccurred) {
    errorOccurred = false;
    try {
        Lexer lex(code);
        auto tokens = lex.tokenize();
        Parser parser(tokens);
        return parser.parseProgram();
    } catch (const std::exception &e) {
        errorOccurred = true;
        return {};
    }
}

void testExtremePointersAndDerefs() {
    std::cout << "\n--- Extreme Pointers and Derefs ---" << std::endl;

    // Deep nesting: *(&(*(p)))
    {
        bool err = false;
        auto prog = parseCode("*(&(*(p)));", err);
        bool ok = (!err && prog.size() == 1);
        auto *stmt = ok ? dynamic_cast<ExprStmt*>(prog[0].get()) : nullptr;
        auto *topDeref = stmt ? dynamic_cast<DerefExpr*>(stmt->expression.get()) : nullptr;
        bool nestOk = false;
        if (topDeref) {
            auto *addr = dynamic_cast<AddressOfExpr*>(topDeref->operand.get());
            if (addr) {
                auto *innerDeref = dynamic_cast<DerefExpr*>(addr->operand.get());
                if (innerDeref) {
                    auto *id = dynamic_cast<IdentifierExpr*>(innerDeref->operand.get());
                    nestOk = (id && id->name == "p");
                }
            }
        }
        TEST_ASSERT(nestOk, "Deep nesting '*(&(*(p)))'", "DerefExpr(AddressOfExpr(DerefExpr(p)))");
    }

    // val &val (deref of address of variable val)
    {
        bool err = false;
        auto prog = parseCode("val &val;", err);
        bool ok = (!err && prog.size() == 1);
        auto *stmt = ok ? dynamic_cast<ExprStmt*>(prog[0].get()) : nullptr;
        auto *deref = stmt ? dynamic_cast<DerefExpr*>(stmt->expression.get()) : nullptr;
        bool valOk = false;
        if (deref) {
            auto *addr = dynamic_cast<AddressOfExpr*>(deref->operand.get());
            if (addr) {
                auto *id = dynamic_cast<IdentifierExpr*>(addr->operand.get());
                valOk = (id && id->name == "val");
            }
        }
        TEST_ASSERT(valOk, "Self-referencing 'val &val;'", "DerefExpr(AddressOfExpr(IdentifierExpr('val')))");
    }

    // Complex deref assignment: **pp = *p + *q;
    {
        bool err = false;
        auto prog = parseCode("**pp = *p + *q;", err);
        bool ok = (!err && prog.size() == 1);
        auto *stmt = ok ? dynamic_cast<ExprStmt*>(prog[0].get()) : nullptr;
        auto *dAssign = stmt ? dynamic_cast<DerefAssignExpr*>(stmt->expression.get()) : nullptr;
        bool assignOk = false;
        if (dAssign) {
            auto *lhs = dynamic_cast<DerefExpr*>(dAssign->pointer.get());
            auto *rhs = dynamic_cast<BinaryExpr*>(dAssign->value.get());
            assignOk = (lhs != nullptr && rhs != nullptr && rhs->op.type == TokenType::PLUS);
        }
        TEST_ASSERT(assignOk, "Complex deref assignment '**pp = *p + *q;'", "DerefAssignExpr(DerefExpr(pp), BinaryExpr)");
    }

    // Chained pointer types in variable declaration:
    // pakad p: patta[patta[ank]] = &ptr;
    {
        bool err = false;
        auto prog = parseCode("pakad p: patta[patta[ank]] = &target_var;", err);
        bool ok = (!err && prog.size() == 1);
        auto *var = ok ? dynamic_cast<VarDeclStmt*>(prog[0].get()) : nullptr;
        bool typeOk = (var && var->typeAnnotation != nullptr && var->initializer != nullptr);
        TEST_ASSERT(typeOk, "Nested pointer type 'patta[patta[ank]]'", "VarDeclStmt with nested pointer type annotation");
    }

    // Drop with deref: khatam *p;
    {
        bool err = false;
        auto prog = parseCode("khatam *p;", err);
        bool ok = (!err && prog.size() == 1);
        auto *drop = ok ? dynamic_cast<DropStmt*>(prog[0].get()) : nullptr;
        bool dropOk = false;
        if (drop) {
            auto *d = dynamic_cast<DerefExpr*>(drop->expr.get());
            dropOk = (d != nullptr);
        }
        TEST_ASSERT(dropOk, "Drop dereferenced pointer 'khatam *p;'", "DropStmt with DerefExpr target");
    }

    // Move with deref: pakad x = rawana *p;
    {
        bool err = false;
        auto prog = parseCode("pakad x = rawana *p;", err);
        bool ok = (!err && prog.size() == 1);
        auto *var = ok ? dynamic_cast<VarDeclStmt*>(prog[0].get()) : nullptr;
        bool movOk = false;
        if (var) {
            auto *mov = dynamic_cast<MoveExpr*>(var->initializer.get());
            if (mov) {
                auto *d = dynamic_cast<DerefExpr*>(mov->operand.get());
                movOk = (d != nullptr);
            }
        }
        TEST_ASSERT(movOk, "Move dereferenced target 'rawana *p'", "MoveExpr with DerefExpr operand");
    }
}

void testExtremeOverloadsAndClasses() {
    std::cout << "\n--- Extreme Overloads and Classes ---" << std::endl;

    // Overloaded function with typed params and return type
    {
        std::string code = "bahuroop jugaad add(a: ank, b: ank): ank { ye_le a + b; }";
        bool err = false;
        auto prog = parseCode(code, err);
        bool ok = (!err && prog.size() == 1);
        auto *fn = ok ? dynamic_cast<FuncDeclStmt*>(prog[0].get()) : nullptr;
        bool typedOk = (fn && fn->isOverload && fn->paramList.size() == 2 &&
                        fn->paramList[0].typeAnnotation != nullptr &&
                        fn->returnTypeAnnotation != nullptr);
        TEST_ASSERT(typedOk, "Typed overload 'bahuroop jugaad add(...): ank'", "FuncDeclStmt with typed params and return type");
    }

    // Async overload: bahuroop asar jugaad fetch_it() {}
    {
        std::string code = "bahuroop asar jugaad fetch_it() {}";
        bool err = false;
        auto prog = parseCode(code, err);
        bool ok = (!err && prog.size() == 1);
        auto *fn = ok ? dynamic_cast<FuncDeclStmt*>(prog[0].get()) : nullptr;
        bool asyncOk = (fn && fn->isOverload && fn->isAsync);
        TEST_ASSERT(asyncOk, "Async overload 'bahuroop asar jugaad fetch_it()'", "FuncDeclStmt with isOverload=true, isAsync=true");
    }

    // Operator with return type: tantrik + (other: Vec) -> Vec { ye_le other; }
    {
        std::string code = 
            "biradari Vec {\n"
            "    tantrik + (other: Vec) -> Vec { ye_le other; }\n"
            "}";
        bool err = false;
        auto prog = parseCode(code, err);
        bool ok = (!err && prog.size() == 1);
        auto *cls = ok ? dynamic_cast<ClassDeclStmt*>(prog[0].get()) : nullptr;
        bool opOk = false;
        if (cls && cls->methods.size() == 1) {
            auto *op = dynamic_cast<OperatorDeclStmt*>(cls->methods[0].get());
            opOk = (op && op->opSymbol == "+" && op->returnTypeAnnotation != nullptr);
        }
        TEST_ASSERT(opOk, "Operator overload with return type 'tantrik + (...) -> Vec'", "OperatorDeclStmt with returnTypeAnnotation");
    }

    // Nested scopes: daayra { seema { elaqa { pakad x = 1; } } }
    {
        std::string code = "daayra { seema { elaqa { pakad x = 1; } } }";
        bool err = false;
        auto prog = parseCode(code, err);
        bool ok = (!err && prog.size() == 1);
        auto *s1 = ok ? dynamic_cast<ScopeBlockStmt*>(prog[0].get()) : nullptr;
        bool nestOk = false;
        if (s1 && s1->statements.size() == 1) {
            auto *s2 = dynamic_cast<ScopeBlockStmt*>(s1->statements[0].get());
            if (s2 && s2->statements.size() == 1) {
                auto *s3 = dynamic_cast<ScopeBlockStmt*>(s2->statements[0].get());
                if (s3 && s3->statements.size() == 1) {
                    auto *var = dynamic_cast<VarDeclStmt*>(s3->statements[0].get());
                    nestOk = (var && var->name == "x");
                }
            }
        }
        TEST_ASSERT(nestOk, "Deeply nested scopes 'daayra { seema { elaqa { ... } } }'", "3 nested ScopeBlockStmt nodes");
    }
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Milestone 2 Challenger Deep Suite     " << std::endl;
    std::cout << "========================================" << std::endl;

    testExtremePointersAndDerefs();
    testExtremeOverloadsAndClasses();

    std::cout << "\n========================================" << std::endl;
    std::cout << "Deep Suite Summary: " << testsPassed << " PASSED, " << testsFailed << " FAILED" << std::endl;
    std::cout << "========================================" << std::endl;

    return (testsFailed == 0) ? 0 : 1;
}
