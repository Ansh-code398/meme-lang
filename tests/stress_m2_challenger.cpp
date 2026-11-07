#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <memory>
#include "src/lexer/Lexer.h"
#include "src/parser/Parser.h"

// Challenger Stress Harness for Milestone 2 Lexer and Parser Extensions

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

// Helper to parse program from code string
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

// 1. Single & vs double && vs triple &&&
void testAmpersandVariations() {
    std::cout << "\n--- Testing Ampersand Variations (&, &&, &&&) ---" << std::endl;

    // Tokenization
    {
        Lexer l("&&&");
        auto toks = l.tokenize();
        // Should be AUR ("&&") followed by ADDRESS_OF ("&") followed by EOF
        bool ok = (toks.size() == 3 && toks[0].type == TokenType::AUR && toks[1].type == TokenType::ADDRESS_OF);
        TEST_ASSERT(ok, "Lexer &&& token sequence", "Expected AUR then ADDRESS_OF");
    }

    {
        Lexer l("&&&&");
        auto toks = l.tokenize();
        bool ok = (toks.size() == 3 && toks[0].type == TokenType::AUR && toks[1].type == TokenType::AUR);
        TEST_ASSERT(ok, "Lexer &&&& token sequence", "Expected AUR then AUR");
    }

    {
        Lexer l("a && &b;");
        auto toks = l.tokenize();
        bool ok = (toks.size() == 6 &&
                   toks[0].type == TokenType::IDENTIFIER &&
                   toks[1].type == TokenType::AUR &&
                   toks[2].type == TokenType::ADDRESS_OF &&
                   toks[3].type == TokenType::IDENTIFIER &&
                   toks[4].type == TokenType::SEMICOLON);
        TEST_ASSERT(ok, "Lexer 'a && &b;' tokenization", "Expected IDENT, AUR, ADDRESS_OF, IDENT, SEMICOLON");
    }

    // Parsing a && &b;
    {
        bool err = false;
        auto prog = parseCode("a && &b;", err);
        bool ok = (!err && prog.size() == 1);
        if (ok) {
            auto *stmt = dynamic_cast<ExprStmt*>(prog[0].get());
            ok = (stmt != nullptr);
            if (ok) {
                auto *bin = dynamic_cast<BinaryExpr*>(stmt->expression.get());
                ok = (bin != nullptr && bin->op.type == TokenType::AUR);
                if (ok) {
                    auto *left = dynamic_cast<IdentifierExpr*>(bin->left.get());
                    auto *right = dynamic_cast<AddressOfExpr*>(bin->right.get());
                    ok = (left != nullptr && left->name == "a" && right != nullptr);
                    if (ok) {
                        auto *rId = dynamic_cast<IdentifierExpr*>(right->operand.get());
                        ok = (rId != nullptr && rId->name == "b");
                    }
                }
            }
        }
        TEST_ASSERT(ok, "Parser 'a && &b;' AST", "BinaryExpr(a, AUR, AddressOfExpr(b))");
    }

    // Parsing a &&& b; (compact whitespace)
    {
        bool err = false;
        auto prog = parseCode("a &&& b;", err);
        bool ok = (!err && prog.size() == 1);
        if (ok) {
            auto *stmt = dynamic_cast<ExprStmt*>(prog[0].get());
            auto *bin = stmt ? dynamic_cast<BinaryExpr*>(stmt->expression.get()) : nullptr;
            ok = (bin != nullptr && bin->op.type == TokenType::AUR);
            if (ok) {
                auto *right = dynamic_cast<AddressOfExpr*>(bin->right.get());
                ok = (right != nullptr);
            }
        }
        TEST_ASSERT(ok, "Parser compact 'a &&& b;' AST", "Expected BinaryExpr with AddressOf right operand");
    }

    // &x && &y
    {
        bool err = false;
        auto prog = parseCode("&x && &y;", err);
        bool ok = (!err && prog.size() == 1);
        if (ok) {
            auto *stmt = dynamic_cast<ExprStmt*>(prog[0].get());
            auto *bin = stmt ? dynamic_cast<BinaryExpr*>(stmt->expression.get()) : nullptr;
            ok = (bin != nullptr && bin->op.type == TokenType::AUR);
            if (ok) {
                auto *left = dynamic_cast<AddressOfExpr*>(bin->left.get());
                auto *right = dynamic_cast<AddressOfExpr*>(bin->right.get());
                ok = (left != nullptr && right != nullptr);
            }
        }
        TEST_ASSERT(ok, "Parser '&x && &y;' AST", "Expected BinaryExpr with two AddressOf operands");
    }

    // Ampersand in closure: () => &x
    {
        bool err = false;
        auto prog = parseCode("pakad closure_fn = () => &x;", err);
        bool ok = (!err && prog.size() == 1);
        if (ok) {
            auto *var = dynamic_cast<VarDeclStmt*>(prog[0].get());
            auto *lam = var ? dynamic_cast<LambdaExpr*>(var->initializer.get()) : nullptr;
            ok = (lam != nullptr && lam->declaration != nullptr);
            if (ok) {
                auto *blk = dynamic_cast<BlockStmt*>(lam->declaration->body.get());
                ok = (blk != nullptr && blk->statements.size() == 1);
                if (ok) {
                    auto *ret = dynamic_cast<ReturnStmt*>(blk->statements[0].get());
                    auto *addr = ret ? dynamic_cast<AddressOfExpr*>(ret->value.get()) : nullptr;
                    ok = (addr != nullptr);
                }
            }
        }
        TEST_ASSERT(ok, "Closure returning &x", "Lambda returning AddressOfExpr");
    }

    // Invalid unary &&: &&x; should throw syntax error
    {
        bool err = false;
        parseCode("&&x;", err);
        TEST_ASSERT(err, "Syntax error on unary '&&x;'", "Expected parser error on leading &&");
    }

    // Invalid unary &&&: &&&x; should throw syntax error
    {
        bool err = false;
        parseCode("&&&x;", err);
        TEST_ASSERT(err, "Syntax error on unary '&&&x;'", "Expected parser error on leading &&&");
    }
}

// 2. Prefix * dereference vs infix * multiplication vs call argument spread *args
void testPointerMultiplicationAndSpread() {
    std::cout << "\n--- Testing Prefix * vs Infix * vs Spread * ---" << std::endl;

    // Dereference: *p;
    {
        bool err = false;
        auto prog = parseCode("*p;", err);
        bool ok = (!err && prog.size() == 1);
        auto *stmt = ok ? dynamic_cast<ExprStmt*>(prog[0].get()) : nullptr;
        auto *deref = stmt ? dynamic_cast<DerefExpr*>(stmt->expression.get()) : nullptr;
        TEST_ASSERT(deref != nullptr, "Prefix * dereference '*p;'", "Expected DerefExpr");
    }

    // Multiplication: a * b;
    {
        bool err = false;
        auto prog = parseCode("a * b;", err);
        bool ok = (!err && prog.size() == 1);
        auto *stmt = ok ? dynamic_cast<ExprStmt*>(prog[0].get()) : nullptr;
        auto *bin = stmt ? dynamic_cast<BinaryExpr*>(stmt->expression.get()) : nullptr;
        bool multOk = (bin != nullptr && bin->op.type == TokenType::MULTIPLY);
        TEST_ASSERT(multOk, "Infix multiplication 'a * b;'", "Expected BinaryExpr(MULTIPLY)");
    }

    // Infix with dereferenced operands: *a * *b;
    {
        bool err = false;
        auto prog = parseCode("*a * *b;", err);
        bool ok = (!err && prog.size() == 1);
        auto *stmt = ok ? dynamic_cast<ExprStmt*>(prog[0].get()) : nullptr;
        auto *bin = stmt ? dynamic_cast<BinaryExpr*>(stmt->expression.get()) : nullptr;
        bool structOk = false;
        if (bin && bin->op.type == TokenType::MULTIPLY) {
            auto *lDeref = dynamic_cast<DerefExpr*>(bin->left.get());
            auto *rDeref = dynamic_cast<DerefExpr*>(bin->right.get());
            structOk = (lDeref != nullptr && rDeref != nullptr);
        }
        TEST_ASSERT(structOk, "Mixed '*a * *b;'", "BinaryExpr(DerefExpr(a), MULTIPLY, DerefExpr(b))");
    }

    // Spread in call arguments: foo(*args);
    {
        bool err = false;
        auto prog = parseCode("foo(*args);", err);
        bool ok = (!err && prog.size() == 1);
        auto *stmt = ok ? dynamic_cast<ExprStmt*>(prog[0].get()) : nullptr;
        auto *call = stmt ? dynamic_cast<CallExpr*>(stmt->expression.get()) : nullptr;
        bool spreadOk = false;
        if (call && call->args.size() == 1) {
            auto *sp = dynamic_cast<SpreadExpr*>(call->args[0].get());
            spreadOk = (sp != nullptr);
        }
        TEST_ASSERT(spreadOk, "Call argument spread 'foo(*args);'", "Expected SpreadExpr in call argument");
    }

    // Dereference in call arguments using parentheses: foo((*p));
    {
        bool err = false;
        auto prog = parseCode("foo((*p));", err);
        bool ok = (!err && prog.size() == 1);
        auto *stmt = ok ? dynamic_cast<ExprStmt*>(prog[0].get()) : nullptr;
        auto *call = stmt ? dynamic_cast<CallExpr*>(stmt->expression.get()) : nullptr;
        bool derefOk = false;
        if (call && call->args.size() == 1) {
            auto *deref = dynamic_cast<DerefExpr*>(call->args[0].get());
            derefOk = (deref != nullptr);
        }
        TEST_ASSERT(derefOk, "Parenthesized deref argument 'foo((*p));'", "Expected DerefExpr inside call argument");
    }

    // Dereference in call arguments using 'asli': foo(asli p);
    {
        bool err = false;
        auto prog = parseCode("foo(asli p);", err);
        bool ok = (!err && prog.size() == 1);
        auto *stmt = ok ? dynamic_cast<ExprStmt*>(prog[0].get()) : nullptr;
        auto *call = stmt ? dynamic_cast<CallExpr*>(stmt->expression.get()) : nullptr;
        bool asliOk = false;
        if (call && call->args.size() == 1) {
            auto *deref = dynamic_cast<DerefExpr*>(call->args[0].get());
            asliOk = (deref != nullptr);
        }
        TEST_ASSERT(asliOk, "Keyword deref argument 'foo(asli p);'", "Expected DerefExpr from 'asli p'");
    }

    // Variadic rest parameter: jugaad f(a, *rest) {}
    {
        bool err = false;
        auto prog = parseCode("jugaad f(a, *rest) {}", err);
        bool ok = (!err && prog.size() == 1);
        auto *fn = ok ? dynamic_cast<FuncDeclStmt*>(prog[0].get()) : nullptr;
        bool varOk = false;
        if (fn && fn->paramList.size() == 2) {
            varOk = (fn->paramList[0].name == "a" &&
                     !fn->paramList[0].isVariadic &&
                     fn->paramList[1].name == "rest" &&
                     fn->paramList[1].isVariadic);
        }
        TEST_ASSERT(varOk, "Variadic function parameter 'jugaad f(a, *rest)'", "Second param isVariadic == true");
    }
}

// 3. 'val' as dereference operator vs 'val' as variable/parameter identifier
void testValContextualDisambiguation() {
    std::cout << "\n--- Testing 'val' Contextual Disambiguation ---" << std::endl;

    // val as dereference prefix: val p;
    {
        bool err = false;
        auto prog = parseCode("val p;", err);
        bool ok = (!err && prog.size() == 1);
        auto *stmt = ok ? dynamic_cast<ExprStmt*>(prog[0].get()) : nullptr;
        auto *deref = stmt ? dynamic_cast<DerefExpr*>(stmt->expression.get()) : nullptr;
        bool derefOk = (deref != nullptr);
        if (derefOk) {
            auto *id = dynamic_cast<IdentifierExpr*>(deref->operand.get());
            derefOk = (id != nullptr && id->name == "p");
        }
        TEST_ASSERT(derefOk, "'val p;' as DerefExpr", "Expected DerefExpr with operand 'p'");
    }

    // pakad val = 100;
    {
        bool err = false;
        auto prog = parseCode("pakad val = 100;", err);
        bool ok = (!err && prog.size() == 1);
        auto *var = ok ? dynamic_cast<VarDeclStmt*>(prog[0].get()) : nullptr;
        bool varOk = (var != nullptr && var->name == "val");
        TEST_ASSERT(varOk, "Variable declaration 'pakad val = 100;'", "VarDeclStmt with name 'val'");
    }

    // Assignment: val = 42;
    {
        bool err = false;
        auto prog = parseCode("val = 42;", err);
        bool ok = (!err && prog.size() == 1);
        auto *stmt = ok ? dynamic_cast<ExprStmt*>(prog[0].get()) : nullptr;
        auto *assign = stmt ? dynamic_cast<AssignExpr*>(stmt->expression.get()) : nullptr;
        bool assignOk = (assign != nullptr && assign->name == "val");
        TEST_ASSERT(assignOk, "Variable assignment 'val = 42;'", "AssignExpr to name 'val'");
    }

    // Parameter in function: jugaad f(val) { ye_le val; }
    {
        bool err = false;
        auto prog = parseCode("jugaad f(val) { ye_le val; }", err);
        bool ok = (!err && prog.size() == 1);
        auto *fn = ok ? dynamic_cast<FuncDeclStmt*>(prog[0].get()) : nullptr;
        bool paramOk = (fn != nullptr && fn->paramList.size() == 1 && fn->paramList[0].name == "val");
        TEST_ASSERT(paramOk, "Function parameter named 'val'", "FuncParam name == 'val'");
    }

    // Arrow lambda: val => val * 2;
    {
        bool err = false;
        auto prog = parseCode("pakad double_it = val => val * 2;", err);
        bool ok = (!err && prog.size() == 1);
        auto *var = ok ? dynamic_cast<VarDeclStmt*>(prog[0].get()) : nullptr;
        auto *lam = var ? dynamic_cast<LambdaExpr*>(var->initializer.get()) : nullptr;
        bool lamOk = false;
        if (lam && lam->declaration) {
            lamOk = (lam->declaration->paramList.size() == 1 &&
                     lam->declaration->paramList[0].name == "val");
        }
        TEST_ASSERT(lamOk, "Arrow lambda with bare param 'val => val * 2'", "LambdaExpr param 'val'");
    }

    // Parenthesized lambda: (val) => val + 1;
    {
        bool err = false;
        auto prog = parseCode("pakad inc = (val) => val + 1;", err);
        bool ok = (!err && prog.size() == 1);
        auto *var = ok ? dynamic_cast<VarDeclStmt*>(prog[0].get()) : nullptr;
        auto *lam = var ? dynamic_cast<LambdaExpr*>(var->initializer.get()) : nullptr;
        bool lamOk = false;
        if (lam && lam->declaration) {
            lamOk = (lam->declaration->paramList.size() == 1 &&
                     lam->declaration->paramList[0].name == "val");
        }
        TEST_ASSERT(lamOk, "Parenthesized arrow lambda '(val) => val + 1'", "LambdaExpr param 'val'");
    }

    // Loop variable in ghoom range: ghoom val se 1 se 10 {}
    {
        bool err = false;
        auto prog = parseCode("ghoom val se 1 se 10 {}", err);
        bool ok = (!err && prog.size() == 1);
        auto *ghoom = ok ? dynamic_cast<GhoomStmt*>(prog[0].get()) : nullptr;
        bool ghoomOk = (ghoom != nullptr && ghoom->varName == "val");
        TEST_ASSERT(ghoomOk, "Range loop variable 'ghoom val se 1 se 10'", "GhoomStmt with varName 'val'");
    }

    // Loop variable in ghoom collection: ghoom val mein items {}
    {
        bool err = false;
        auto prog = parseCode("ghoom val mein items {}", err);
        bool ok = (!err && prog.size() == 1);
        auto *ghoomIn = ok ? dynamic_cast<GhumaMeinStmt*>(prog[0].get()) : nullptr;
        bool inOk = (ghoomIn != nullptr && ghoomIn->varName == "val");
        TEST_ASSERT(inOk, "Collection loop variable 'ghoom val mein items'", "GhumaMeinStmt with varName 'val'");
    }

    // Destructuring into val: pakad [val, other] = arr;
    {
        bool err = false;
        auto prog = parseCode("pakad [val, other] = arr;", err);
        bool ok = (!err && prog.size() == 1);
        auto *destr = ok ? dynamic_cast<ArrayDestructureStmt*>(prog[0].get()) : nullptr;
        bool destrOk = (destr != nullptr && destr->items.size() == 2 && destr->items[0].name == "val");
        TEST_ASSERT(destrOk, "Array destructuring 'pakad [val, other]'", "ArrayDestructureStmt item 'val'");
    }

    // Binary expressions with val: val + 5; val * 2;
    {
        bool err = false;
        auto prog = parseCode("val + 5;", err);
        bool ok = (!err && prog.size() == 1);
        auto *stmt = ok ? dynamic_cast<ExprStmt*>(prog[0].get()) : nullptr;
        auto *bin = stmt ? dynamic_cast<BinaryExpr*>(stmt->expression.get()) : nullptr;
        bool binOk = false;
        if (bin && bin->op.type == TokenType::PLUS) {
            auto *id = dynamic_cast<IdentifierExpr*>(bin->left.get());
            binOk = (id != nullptr && id->name == "val");
        }
        TEST_ASSERT(binOk, "Binary addition 'val + 5;'", "BinaryExpr with left IdentifierExpr('val')");
    }

    // val val x; (nested deref via keyword)
    {
        bool err = false;
        auto prog = parseCode("val val x;", err);
        bool ok = (!err && prog.size() == 1);
        auto *stmt = ok ? dynamic_cast<ExprStmt*>(prog[0].get()) : nullptr;
        auto *d1 = stmt ? dynamic_cast<DerefExpr*>(stmt->expression.get()) : nullptr;
        bool nestOk = false;
        if (d1) {
            auto *d2 = dynamic_cast<DerefExpr*>(d1->operand.get());
            if (d2) {
                auto *id = dynamic_cast<IdentifierExpr*>(d2->operand.get());
                nestOk = (id != nullptr && id->name == "x");
            }
        }
        TEST_ASSERT(nestOk, "Chained deref 'val val x;'", "DerefExpr(DerefExpr(x))");
    }

    // Address-of val: &val;
    {
        bool err = false;
        auto prog = parseCode("&val;", err);
        bool ok = (!err && prog.size() == 1);
        auto *stmt = ok ? dynamic_cast<ExprStmt*>(prog[0].get()) : nullptr;
        auto *addr = stmt ? dynamic_cast<AddressOfExpr*>(stmt->expression.get()) : nullptr;
        bool addrOk = false;
        if (addr) {
            auto *id = dynamic_cast<IdentifierExpr*>(addr->operand.get());
            addrOk = (id != nullptr && id->name == "val");
        }
        TEST_ASSERT(addrOk, "Address-of 'val' identifier '&val;'", "AddressOfExpr(IdentifierExpr('val'))");
    }
}

// 4. Chained pointer operations: &(*p), *(&x), **pp, *p = 10, **pp = 20
void testChainedPointerOperations() {
    std::cout << "\n--- Testing Chained Pointer Operations ---" << std::endl;

    // &(*p)
    {
        bool err = false;
        auto prog = parseCode("&(*p);", err);
        bool ok = (!err && prog.size() == 1);
        auto *stmt = ok ? dynamic_cast<ExprStmt*>(prog[0].get()) : nullptr;
        auto *addr = stmt ? dynamic_cast<AddressOfExpr*>(stmt->expression.get()) : nullptr;
        bool structOk = false;
        if (addr) {
            auto *deref = dynamic_cast<DerefExpr*>(addr->operand.get());
            if (deref) {
                auto *id = dynamic_cast<IdentifierExpr*>(deref->operand.get());
                structOk = (id != nullptr && id->name == "p");
            }
        }
        TEST_ASSERT(structOk, "Chained '&(*p);'", "AddressOfExpr(DerefExpr('p'))");
    }

    // *(&x)
    {
        bool err = false;
        auto prog = parseCode("*(&x);", err);
        bool ok = (!err && prog.size() == 1);
        auto *stmt = ok ? dynamic_cast<ExprStmt*>(prog[0].get()) : nullptr;
        auto *deref = stmt ? dynamic_cast<DerefExpr*>(stmt->expression.get()) : nullptr;
        bool structOk = false;
        if (deref) {
            auto *addr = dynamic_cast<AddressOfExpr*>(deref->operand.get());
            if (addr) {
                auto *id = dynamic_cast<IdentifierExpr*>(addr->operand.get());
                structOk = (id != nullptr && id->name == "x");
            }
        }
        TEST_ASSERT(structOk, "Chained '*(&x);'", "DerefExpr(AddressOfExpr('x'))");
    }

    // **pp
    {
        bool err = false;
        auto prog = parseCode("**pp;", err);
        bool ok = (!err && prog.size() == 1);
        auto *stmt = ok ? dynamic_cast<ExprStmt*>(prog[0].get()) : nullptr;
        auto *d1 = stmt ? dynamic_cast<DerefExpr*>(stmt->expression.get()) : nullptr;
        bool structOk = false;
        if (d1) {
            auto *d2 = dynamic_cast<DerefExpr*>(d1->operand.get());
            if (d2) {
                auto *id = dynamic_cast<IdentifierExpr*>(d2->operand.get());
                structOk = (id != nullptr && id->name == "pp");
            }
        }
        TEST_ASSERT(structOk, "Double deref '**pp;'", "DerefExpr(DerefExpr('pp'))");
    }

    // ***ppp
    {
        bool err = false;
        auto prog = parseCode("***ppp;", err);
        bool ok = (!err && prog.size() == 1);
        auto *stmt = ok ? dynamic_cast<ExprStmt*>(prog[0].get()) : nullptr;
        auto *d1 = stmt ? dynamic_cast<DerefExpr*>(stmt->expression.get()) : nullptr;
        bool structOk = false;
        if (d1) {
            auto *d2 = dynamic_cast<DerefExpr*>(d1->operand.get());
            if (d2) {
                auto *d3 = dynamic_cast<DerefExpr*>(d2->operand.get());
                if (d3) {
                    auto *id = dynamic_cast<IdentifierExpr*>(d3->operand.get());
                    structOk = (id != nullptr && id->name == "ppp");
                }
            }
        }
        TEST_ASSERT(structOk, "Triple deref '***ppp;'", "DerefExpr(DerefExpr(DerefExpr('ppp')))");
    }

    // Dereference assignment: *p = 10;
    {
        bool err = false;
        auto prog = parseCode("*p = 10;", err);
        bool ok = (!err && prog.size() == 1);
        auto *stmt = ok ? dynamic_cast<ExprStmt*>(prog[0].get()) : nullptr;
        auto *dAssign = stmt ? dynamic_cast<DerefAssignExpr*>(stmt->expression.get()) : nullptr;
        bool assignOk = false;
        if (dAssign) {
            auto *ptr = dynamic_cast<IdentifierExpr*>(dAssign->pointer.get());
            auto *val = dynamic_cast<IntExpr*>(dAssign->value.get());
            assignOk = (ptr != nullptr && ptr->name == "p" && val != nullptr && val->value == 10);
        }
        TEST_ASSERT(assignOk, "Dereference assignment '*p = 10;'", "DerefAssignExpr(p, 10)");
    }

    // Double dereference assignment: **pp = 20;
    {
        bool err = false;
        auto prog = parseCode("**pp = 20;", err);
        bool ok = (!err && prog.size() == 1);
        auto *stmt = ok ? dynamic_cast<ExprStmt*>(prog[0].get()) : nullptr;
        auto *dAssign = stmt ? dynamic_cast<DerefAssignExpr*>(stmt->expression.get()) : nullptr;
        bool assignOk = false;
        if (dAssign) {
            auto *innerDeref = dynamic_cast<DerefExpr*>(dAssign->pointer.get());
            auto *val = dynamic_cast<IntExpr*>(dAssign->value.get());
            assignOk = (innerDeref != nullptr && val != nullptr && val->value == 20);
        }
        TEST_ASSERT(assignOk, "Double deref assignment '**pp = 20;'", "DerefAssignExpr(DerefExpr(pp), 20)");
    }

    // Keyword deref assignment: asli p = 30;
    {
        bool err = false;
        auto prog = parseCode("asli p = 30;", err);
        bool ok = (!err && prog.size() == 1);
        auto *stmt = ok ? dynamic_cast<ExprStmt*>(prog[0].get()) : nullptr;
        auto *dAssign = stmt ? dynamic_cast<DerefAssignExpr*>(stmt->expression.get()) : nullptr;
        bool assignOk = false;
        if (dAssign) {
            auto *ptr = dynamic_cast<IdentifierExpr*>(dAssign->pointer.get());
            assignOk = (ptr != nullptr && ptr->name == "p");
        }
        TEST_ASSERT(assignOk, "Keyword deref assignment 'asli p = 30;'", "DerefAssignExpr from 'asli'");
    }

    // Deref to deref assignment: *p = *q;
    {
        bool err = false;
        auto prog = parseCode("*p = *q;", err);
        bool ok = (!err && prog.size() == 1);
        auto *stmt = ok ? dynamic_cast<ExprStmt*>(prog[0].get()) : nullptr;
        auto *dAssign = stmt ? dynamic_cast<DerefAssignExpr*>(stmt->expression.get()) : nullptr;
        bool assignOk = false;
        if (dAssign) {
            auto *ptr = dynamic_cast<IdentifierExpr*>(dAssign->pointer.get());
            auto *rhsDeref = dynamic_cast<DerefExpr*>(dAssign->value.get());
            assignOk = (ptr != nullptr && ptr->name == "p" && rhsDeref != nullptr);
        }
        TEST_ASSERT(assignOk, "Deref to deref assignment '*p = *q;'", "DerefAssignExpr(p, DerefExpr(q))");
    }
}

// 5. Overloaded function declarations
void testOverloadedFunctionDeclarations() {
    std::cout << "\n--- Testing Overloaded Function Declarations ---" << std::endl;

    // Desi: bahuroop jugaad f() {}
    {
        bool err = false;
        auto prog = parseCode("bahuroop jugaad f() {}", err);
        bool ok = (!err && prog.size() == 1);
        auto *fn = ok ? dynamic_cast<FuncDeclStmt*>(prog[0].get()) : nullptr;
        bool fnOk = (fn != nullptr && fn->name == "f" && fn->isOverload == true && fn->paramList.empty());
        TEST_ASSERT(fnOk, "Desi overload 'bahuroop jugaad f() {}'", "FuncDeclStmt(name='f', isOverload=true)");
    }

    // English: overload fn g(x) {}
    {
        bool err = false;
        auto prog = parseCode("overload fn g(x) {}", err);
        bool ok = (!err && prog.size() == 1);
        auto *fn = ok ? dynamic_cast<FuncDeclStmt*>(prog[0].get()) : nullptr;
        bool fnOk = (fn != nullptr && fn->name == "g" && fn->isOverload == true && fn->paramList.size() == 1);
        TEST_ASSERT(fnOk, "English overload 'overload fn g(x) {}'", "FuncDeclStmt(name='g', isOverload=true)");
    }

    // Aliases: aur_ek def h(x, y) {}
    {
        bool err = false;
        auto prog = parseCode("aur_ek def h(x, y) {}", err);
        bool ok = (!err && prog.size() == 1);
        auto *fn = ok ? dynamic_cast<FuncDeclStmt*>(prog[0].get()) : nullptr;
        bool fnOk = (fn != nullptr && fn->name == "h" && fn->isOverload == true && fn->paramList.size() == 2);
        TEST_ASSERT(fnOk, "Alias overload 'aur_ek def h(x, y) {}'", "FuncDeclStmt(name='h', isOverload=true)");
    }

    // multimethod kaam m() {}
    {
        bool err = false;
        auto prog = parseCode("multimethod kaam m() {}", err);
        bool ok = (!err && prog.size() == 1);
        auto *fn = ok ? dynamic_cast<FuncDeclStmt*>(prog[0].get()) : nullptr;
        bool fnOk = (fn != nullptr && fn->name == "m" && fn->isOverload == true);
        TEST_ASSERT(fnOk, "Multimethod overload 'multimethod kaam m() {}'", "FuncDeclStmt(name='m', isOverload=true)");
    }

    // Bare overload: overload f(x, y, z) {} (without fn keyword)
    {
        bool err = false;
        auto prog = parseCode("overload f(x, y, z) {}", err);
        bool ok = (!err && prog.size() == 1);
        auto *fn = ok ? dynamic_cast<FuncDeclStmt*>(prog[0].get()) : nullptr;
        bool fnOk = (fn != nullptr && fn->name == "f" && fn->isOverload == true && fn->paramList.size() == 3);
        TEST_ASSERT(fnOk, "Bare overload 'overload f(x, y, z) {}'", "FuncDeclStmt without 'fn' keyword");
    }

    // Sequence of overloads sharing name in program
    {
        std::string code = 
            "bahuroop jugaad calc(x) { ye_le x * 2; }\n"
            "bahuroop jugaad calc(x, y) { ye_le x + y; }\n"
            "bahuroop jugaad calc(x, y, z) { ye_le x + y + z; }\n";
        bool err = false;
        auto prog = parseCode(code, err);
        bool ok = (!err && prog.size() == 3);
        if (ok) {
            for (size_t i = 0; i < 3; ++i) {
                auto *fn = dynamic_cast<FuncDeclStmt*>(prog[i].get());
                if (!fn || fn->name != "calc" || !fn->isOverload || fn->paramList.size() != (i + 1)) {
                    ok = false;
                    break;
                }
            }
        }
        TEST_ASSERT(ok, "Multiple overloads for 'calc' (arities 1, 2, 3)", "3 FuncDeclStmt AST nodes sharing name 'calc'");
    }
}

// 6. Operator declarations in class bodies
void testOperatorDeclarationsInClassBodies() {
    std::cout << "\n--- Testing Operator Declarations in Class Bodies ---" << std::endl;

    // Binary + and -
    {
        std::string code = 
            "biradari Vec2 {\n"
            "    tantrik + (other) { ye_le 1; }\n"
            "    operator - (other) { ye_le 2; }\n"
            "}";
        bool err = false;
        auto prog = parseCode(code, err);
        bool ok = (!err && prog.size() == 1);
        auto *cls = ok ? dynamic_cast<ClassDeclStmt*>(prog[0].get()) : nullptr;
        bool methodsOk = false;
        if (cls && cls->methods.size() == 2) {
            auto *op1 = dynamic_cast<OperatorDeclStmt*>(cls->methods[0].get());
            auto *op2 = dynamic_cast<OperatorDeclStmt*>(cls->methods[1].get());
            methodsOk = (op1 && op1->opSymbol == "+" && op1->name == "__jod__" && !op1->isUnary &&
                         op2 && op2->opSymbol == "-" && op2->name == "__ghata__" && !op2->isUnary);
        }
        TEST_ASSERT(methodsOk, "Class operators '+' (__jod__) and '-' (__ghata__)", "OperatorDeclStmt mapped correctly");
    }

    // Unary - (__ulta__)
    {
        std::string code = 
            "biradari Vec2 {\n"
            "    tantrik - () { ye_le 0; }\n"
            "}";
        bool err = false;
        auto prog = parseCode(code, err);
        bool ok = (!err && prog.size() == 1);
        auto *cls = ok ? dynamic_cast<ClassDeclStmt*>(prog[0].get()) : nullptr;
        bool unaryOk = false;
        if (cls && cls->methods.size() == 1) {
            auto *op = dynamic_cast<OperatorDeclStmt*>(cls->methods[0].get());
            unaryOk = (op && op->opSymbol == "-" && op->name == "__ulta__" && op->isUnary && op->paramList.empty());
        }
        TEST_ASSERT(unaryOk, "Unary '-' operator (__ulta__)", "OperatorDeclStmt with isUnary == true");
    }

    // Bracket indexing: [] (__la__) and []= (__daal__)
    {
        std::string code = 
            "biradari Matrix {\n"
            "    tantrik [] (idx) { ye_le 42; }\n"
            "    kriya []= (idx, val) { ye_le 0; }\n"
            "}";
        bool err = false;
        auto prog = parseCode(code, err);
        bool ok = (!err && prog.size() == 1);
        auto *cls = ok ? dynamic_cast<ClassDeclStmt*>(prog[0].get()) : nullptr;
        bool bracketOk = false;
        if (cls && cls->methods.size() == 2) {
            auto *opGet = dynamic_cast<OperatorDeclStmt*>(cls->methods[0].get());
            auto *opSet = dynamic_cast<OperatorDeclStmt*>(cls->methods[1].get());
            bracketOk = (opGet && opGet->opSymbol == "[]" && opGet->name == "__la__" &&
                         opSet && opSet->opSymbol == "[]=" && opSet->name == "__daal__");
        }
        TEST_ASSERT(bracketOk, "Indexing operators '[]' (__la__) and '[]=' (__daal__)", "Bracket get/set dunders");
    }

    // Comparison operators: ==, !=, <, <=, >, >=
    {
        std::string code = 
            "biradari Ord {\n"
            "    tantrik == (other) {}\n"
            "    tantrik != (other) {}\n"
            "    tantrik < (other) {}\n"
            "    tantrik <= (other) {}\n"
            "    tantrik > (other) {}\n"
            "    tantrik >= (other) {}\n"
            "}";
        bool err = false;
        auto prog = parseCode(code, err);
        bool ok = (!err && prog.size() == 1);
        auto *cls = ok ? dynamic_cast<ClassDeclStmt*>(prog[0].get()) : nullptr;
        bool allCompOk = false;
        if (cls && cls->methods.size() == 6) {
            std::vector<std::string> expectedNames = {
                "__barabar__", "__alag__", "__chota__", "__chota_barabar__", "__bada__", "__bada_barabar__"
            };
            allCompOk = true;
            for (size_t i = 0; i < 6; ++i) {
                auto *op = dynamic_cast<OperatorDeclStmt*>(cls->methods[i].get());
                if (!op || op->name != expectedNames[i]) {
                    allCompOk = false;
                    break;
                }
            }
        }
        TEST_ASSERT(allCompOk, "Comparison operators (==, !=, <, <=, >, >=)", "Mapped to correct comparison dunders");
    }

    // Overloaded methods inside class body
    {
        std::string code = 
            "biradari Multi {\n"
            "    bahuroop jugaad item() {}\n"
            "    bahuroop jugaad item(index) {}\n"
            "}";
        bool err = false;
        auto prog = parseCode(code, err);
        bool ok = (!err && prog.size() == 1);
        auto *cls = ok ? dynamic_cast<ClassDeclStmt*>(prog[0].get()) : nullptr;
        bool overOk = false;
        if (cls && cls->methods.size() == 2) {
            auto *m1 = cls->methods[0].get();
            auto *m2 = cls->methods[1].get();
            overOk = (m1 && m1->name == "item" && m1->isOverload &&
                      m2 && m2->name == "item" && m2->isOverload);
        }
        TEST_ASSERT(overOk, "Overloaded methods inside class body", "FuncDeclStmt inside class with isOverload == true");
    }
}

// 7. Memory model constructs: rawana, khatam, daayra
void testMemoryModelConstructs() {
    std::cout << "\n--- Testing Memory Model Constructs (rawana, khatam, daayra) ---" << std::endl;

    // Move expressions with all aliases: rawana, khisko, chal_nikal, move
    {
        std::string code = 
            "pakad a = rawana b;\n"
            "pakad c = khisko d;\n"
            "pakad e = chal_nikal f;\n"
            "pakad g = move h;\n";
        bool err = false;
        auto prog = parseCode(code, err);
        bool ok = (!err && prog.size() == 4);
        if (ok) {
            for (size_t i = 0; i < 4; ++i) {
                auto *var = dynamic_cast<VarDeclStmt*>(prog[i].get());
                auto *mov = var ? dynamic_cast<MoveExpr*>(var->initializer.get()) : nullptr;
                if (!mov) { ok = false; break; }
            }
        }
        TEST_ASSERT(ok, "Move expressions (rawana, khisko, chal_nikal, move)", "Parsed into MoveExpr nodes");
    }

    // Drop statements with all aliases: khatam, alvida, visarjan, drop, dispose
    {
        std::string code = 
            "khatam a;\n"
            "alvida b;\n"
            "visarjan c;\n"
            "drop d;\n"
            "dispose e;\n";
        bool err = false;
        auto prog = parseCode(code, err);
        bool ok = (!err && prog.size() == 5);
        if (ok) {
            for (size_t i = 0; i < 5; ++i) {
                auto *drop = dynamic_cast<DropStmt*>(prog[i].get());
                if (!drop) { ok = false; break; }
            }
        }
        TEST_ASSERT(ok, "Drop statements (khatam, alvida, visarjan, drop, dispose)", "Parsed into DropStmt nodes");
    }

    // Scope block statements with all aliases: daayra, seema, elaqa, scope, lifetime
    {
        std::string code = 
            "daayra { pakad x = 1; }\n"
            "seema { pakad y = 2; }\n"
            "elaqa { pakad z = 3; }\n"
            "scope { pakad w = 4; }\n"
            "lifetime { pakad v = 5; }\n";
        bool err = false;
        auto prog = parseCode(code, err);
        bool ok = (!err && prog.size() == 5);
        if (ok) {
            for (size_t i = 0; i < 5; ++i) {
                auto *sc = dynamic_cast<ScopeBlockStmt*>(prog[i].get());
                if (!sc || sc->statements.size() != 1) { ok = false; break; }
            }
        }
        TEST_ASSERT(ok, "Scope block statements (daayra, seema, elaqa, scope, lifetime)", "Parsed into ScopeBlockStmt nodes");
    }

    // Variable declarations with modifiers: malik, badal, atal
    {
        std::string code = 
            "malik badal x = 1;\n"
            "own const y = 2;\n"
            "kabza atal z: patta[ank] = 3;\n";
        bool err = false;
        auto prog = parseCode(code, err);
        bool ok = (!err && prog.size() == 3);
        if (ok) {
            auto *v1 = dynamic_cast<VarDeclStmt*>(prog[0].get());
            auto *v2 = dynamic_cast<VarDeclStmt*>(prog[1].get());
            auto *v3 = dynamic_cast<VarDeclStmt*>(prog[2].get());
            bool v1Ok = (v1 && v1->isOwned && v1->isMut && !v1->isConst);
            bool v2Ok = (v2 && v2->isOwned && !v2->isMut && v2->isConst);
            bool v3Ok = (v3 && v3->isOwned && !v3->isMut && v3->isConst && v3->typeAnnotation != nullptr);
            ok = (v1Ok && v2Ok && v3Ok);
        }
        TEST_ASSERT(ok, "VarDecl modifiers (malik, own, badal, const, atal, pointer type)", "isOwned, isMut, isConst correctly flags");
    }
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   Milestone 2 Challenger Stress Suite  " << std::endl;
    std::cout << "========================================" << std::endl;

    testAmpersandVariations();
    testPointerMultiplicationAndSpread();
    testValContextualDisambiguation();
    testChainedPointerOperations();
    testOverloadedFunctionDeclarations();
    testOperatorDeclarationsInClassBodies();
    testMemoryModelConstructs();

    std::cout << "\n========================================" << std::endl;
    std::cout << "Stress Test Summary: " << testsPassed << " PASSED, " << testsFailed << " FAILED" << std::endl;
    std::cout << "========================================" << std::endl;

    return (testsFailed == 0) ? 0 : 1;
}
