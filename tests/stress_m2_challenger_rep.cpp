#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <memory>
#include <sstream>
#include "src/lexer/Lexer.h"
#include "src/parser/Parser.h"

// Challenger 1 (Replacement) Empirical Stress Harness for Milestone 2 Iteration 3
// Comprehensive Syntax Fuzzing & Adversarial Testing

static int g_passCount = 0;
static int g_failCount = 0;

void recordPass(const std::string &testName) {
    g_passCount++;
    std::cout << "  [PASS] " << testName << "\n";
}

void recordFail(const std::string &testName, const std::string &reason) {
    g_failCount++;
    std::cerr << "  [FAIL] " << testName << " -> " << reason << "\n";
}

#define ASSERT_TRUE(cond, name, msg) do { \
    if (cond) { recordPass(name); } \
    else { recordFail(name, msg); } \
} while(0)

#define ASSERT_PARSE_OK(code, name) do { \
    bool errorOccurred = false; \
    std::string errMessage = ""; \
    try { \
        Lexer lex(code); \
        auto tokens = lex.tokenize(); \
        Parser parser(tokens); \
        auto ast = parser.parseProgram(); \
        if (ast.empty()) { \
            recordFail(name, "Parsed empty AST"); \
        } else { \
            recordPass(name); \
        } \
    } catch (const std::exception &e) { \
        recordFail(name, std::string("Exception thrown: ") + e.what()); \
    } \
} while(0)

#define ASSERT_PARSE_FAIL(code, name) do { \
    bool errorOccurred = false; \
    try { \
        Lexer lex(code); \
        auto tokens = lex.tokenize(); \
        Parser parser(tokens); \
        auto ast = parser.parseProgram(); \
    } catch (const std::exception &e) { \
        errorOccurred = true; \
    } \
    if (errorOccurred) { \
        recordPass(name); \
    } else { \
        recordFail(name, "Expected parse failure, but parsing succeeded unexpectedly"); \
    } \
} while(0)

// =========================================================================
// Category 1: Method Declarations vs Drop Statements
// =========================================================================
void testCategory1_MethodsVsDrops() {
    std::cout << "\n=== Category 1: Method Declarations vs Drop Statements ===\n";

    // 1.1 All canonical forms of destructor declarations in class bodies
    std::string code1 =
        "biradari A {\n"
        "    jugaad alvida() {}\n"
        "    jugaad drop() {}\n"
        "    alvida() {}\n"
        "    drop() {}\n"
        "}\n";
    ASSERT_PARSE_OK(code1, "1.1 Class with jugaad alvida, jugaad drop, bare alvida, bare drop");

    // 1.2 Valid keywords and aliases for class methods & destructors
    std::string code2 =
        "class Resource {\n"
        "    fn drop() {}\n"
        "    def alvida() {}\n"
        "    kaam visarjan() {}\n"
        "    dispose() {}\n"
        "    khatam() {}\n"
        "}\n";
    ASSERT_PARSE_OK(code2, "1.2 Class with aliases fn drop, def alvida, kaam visarjan, dispose, khatam");

    // 1.3 Methods with drop statements inside their bodies
    std::string code3 =
        "biradari Managed {\n"
        "    jugaad alvida() {\n"
        "        alvida khud.sub_resource;\n"
        "        drop other;\n"
        "        khatam z;\n"
        "        visarjan w;\n"
        "        dispose v;\n"
        "    }\n"
        "    drop() {\n"
        "        khatam khud;\n"
        "    }\n"
        "}\n";
    ASSERT_PARSE_OK(code3, "1.3 Class destructors containing drop statements inside bodies");

    // 1.4 Top-level function declarations named alvida/drop vs drop statements
    std::string code4 =
        "kar alvida(x) { wapas x; }\n"
        "fn drop(y) { wapas y; }\n"
        "pakad res = 42;\n"
        "alvida res;\n"
        "drop res;\n"
        "khatam res;\n";
    ASSERT_PARSE_OK(code4, "1.4 Top-level functions named alvida/drop alongside top-level drop statements");

    // 1.5 Destructor with return type and parameters (if user defines overloaded signature)
    std::string code5 =
        "biradari CustomDtor {\n"
        "    jugaad alvida(code: ank): sunya {\n"
        "        dikha code;\n"
        "    }\n"
        "    drop(reason: bol): sunya {\n"
        "        dikha reason;\n"
        "    }\n"
        "}\n";
    ASSERT_PARSE_OK(code5, "1.5 Destructor methods with parameters and explicit return types");

    // 1.6 Negative: Drop statement directly in class body (must be rejected)
    std::string code6_bad =
        "biradari Illegal {\n"
        "    alvida x;\n"
        "}\n";
    ASSERT_PARSE_FAIL(code6_bad, "1.6 Negative: Drop statement directly in class body rejected");

    std::string code7_bad =
        "biradari Illegal2 {\n"
        "    drop y;\n"
        "}\n";
    ASSERT_PARSE_FAIL(code7_bad, "1.7 Negative: 'drop y;' directly in class body rejected");
}

// =========================================================================
// Category 2: Nested Scopes with Drop
// =========================================================================
void testCategory2_NestedScopesWithDrop() {
    std::cout << "\n=== Category 2: Nested Scopes with Drop ===\n";

    // 2.1 The exact dispatch example
    std::string code1 =
        "daayra {\n"
        "    alvida x;\n"
        "    scope {\n"
        "        drop y;\n"
        "        khatam z;\n"
        "    }\n"
        "}\n";
    ASSERT_PARSE_OK(code1, "2.1 Nested daayra and scope with alvida, drop, khatam");

    // 2.2 Deeply nested scopes (8 levels) with drop and lifetime aliases
    std::string code2 =
        "daayra {\n"
        "    pakad a = 1;\n"
        "    scope {\n"
        "        pakad b = 2;\n"
        "        seema {\n"
        "            pakad c = 3;\n"
        "            elaqa {\n"
        "                pakad d = 4;\n"
        "                lifetime {\n"
        "                    pakad e = 5;\n"
        "                    daayra {\n"
        "                        pakad f = 6;\n"
        "                        scope {\n"
        "                            pakad g = 7;\n"
        "                            daayra {\n"
        "                                drop g;\n"
        "                                khatam f;\n"
        "                                alvida e;\n"
        "                                visarjan d;\n"
        "                                dispose c;\n"
        "                            }\n"
        "                        }\n"
        "                    }\n"
        "                }\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "}\n";
    ASSERT_PARSE_OK(code2, "2.2 Deeply nested multi-alias scopes (daayra, scope, seema, elaqa, lifetime)");

    // 2.3 Scopes with control flow, return, break, continue
    std::string code3 =
        "kar scoped_logic() {\n"
        "    daayra {\n"
        "        pakad x = 100;\n"
        "        jabtak x > 0 {\n"
        "            scope {\n"
        "                dekh x == 50 {\n"
        "                    drop x;\n"
        "                    wapas 42;\n"
        "                }\n"
        "                x = x - 1;\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "}\n";
    ASSERT_PARSE_OK(code3, "2.3 Scopes with loops, conditionals, drop, and early return");

    // 2.4 Empty scopes
    std::string code4 =
        "daayra {}\n"
        "scope {}\n"
        "seema {}\n"
        "elaqa {}\n"
        "lifetime {}\n";
    ASSERT_PARSE_OK(code4, "2.4 Empty scope blocks with all 5 keywords");

    // 2.5 Negative: Scope without block '{'
    std::string code5_bad = "daayra x = 10;";
    ASSERT_PARSE_FAIL(code5_bad, "2.5 Negative: 'daayra' without block rejected");

    std::string code6_bad = "scope;";
    ASSERT_PARSE_FAIL(code6_bad, "2.6 Negative: bare 'scope;' rejected");

    // 2.6 Negative: Drop without expression
    std::string code7_bad = "daayra { alvida; }";
    ASSERT_PARSE_FAIL(code7_bad, "2.7 Negative: 'alvida;' without expression rejected");
}

// =========================================================================
// Category 3: Variables Named ptr, addr, ref, val, mut_ref
// =========================================================================
void testCategory3_VariablesNamedKeywords() {
    std::cout << "\n=== Category 3: Variables Named ptr, addr, ref, val, mut_ref ===\n";

    // 3.1 The exact dispatch example
    std::string code1 =
        "pakad ptr = 1;\n"
        "pakad addr = 2;\n"
        "pakad ref = 3;\n"
        "pakad mut_ref = 4;\n"
        "pakad val = 5;\n"
        "dikha ptr + addr + ref + mut_ref + val;\n";
    ASSERT_PARSE_OK(code1, "3.1 Declarations and sum expression with ptr, addr, ref, mut_ref, val");

    // 3.2 With qualifiers: atal, badal, malik, const, mut, own
    std::string code2 =
        "pakad atal ptr = 10;\n"
        "pakad badal addr = 20;\n"
        "pakad malik ref = 30;\n"
        "pakad mut mut_ref = 40;\n"
        "pakad const val = 50;\n"
        "addr = addr + 1;\n"
        "mut_ref = mut_ref + 1;\n";
    ASSERT_PARSE_OK(code2, "3.2 Qualified declarations (atal, badal, malik, mut, const) & reassignments");

    // 3.3 Type annotations on these variables
    std::string code3 =
        "pakad ptr: ank = 100;\n"
        "pakad addr: ank = 200;\n"
        "pakad ref: ank = 300;\n"
        "pakad mut_ref: ank = 400;\n"
        "pakad val: ank = 500;\n";
    ASSERT_PARSE_OK(code3, "3.3 Explicit type annotations on ptr, addr, ref, mut_ref, val");

    // 3.4 In function parameter lists and call arguments
    std::string code4 =
        "kar compute(ptr, addr, ref, mut_ref, val) {\n"
        "    wapas ptr * addr + ref - mut_ref / val;\n"
        "}\n"
        "pakad res = compute(1, 2, 3, 4, 5);\n";
    ASSERT_PARSE_OK(code4, "3.4 Function parameters and call arguments with contextual variable names");

    // 3.5 In class fields and methods
    std::string code5 =
        "biradari StateHolder {\n"
        "    jugaad update(ptr, addr) {\n"
        "        khud.ptr = ptr;\n"
        "        khud.addr = addr;\n"
        "        khud.val = khud.ptr + khud.addr;\n"
        "    }\n"
        "}\n";
    ASSERT_PARSE_OK(code5, "3.5 Class property access and methods using ptr, addr, val");

    // 3.6 In lambdas
    std::string code6 =
        "pakad f1 = ptr => ptr * 2;\n"
        "pakad f2 = addr => addr + 10;\n"
        "pakad f3 = val => val - 5;\n"
        "pakad f4 = (ref, mut_ref) => ref + mut_ref;\n";
    ASSERT_PARSE_OK(code6, "3.6 Arrow lambdas with parameters ptr, addr, val, ref, mut_ref");
}

// =========================================================================
// Category 4: Prefix Unary Operations vs Arithmetic Disambiguation
// =========================================================================
void testCategory4_UnaryVsArithmetic() {
    std::cout << "\n=== Category 4: Prefix Unary Operations vs Arithmetic ===\n";

    // 4.1 The exact dispatch example
    std::string code1 =
        "pakad ptr = 1;\n"
        "pakad addr = 2;\n"
        "pakad p = &ptr;\n"
        "pakad a = &addr;\n"
        "pakad v = *p;\n";
    ASSERT_PARSE_OK(code1, "4.1 Prefix unary &ptr, &addr, *p on contextual variables");

    // 4.2 Desi prefix keywords: patta, thikana, asli, deref
    std::string code2 =
        "pakad x = 42;\n"
        "pakad p1 = patta x;\n"
        "pakad p2 = thikana x;\n"
        "pakad v1 = asli p1;\n"
        "pakad v2 = deref p2;\n"
        "pakad v3 = dekh_patta p1;\n"
        "pakad v4 = uthha p2;\n";
    ASSERT_PARSE_OK(code2, "4.2 Desi prefix keywords (patta, thikana, asli, deref, dekh_patta, uthha)");

    // 4.3 Mixing unary & and * with binary arithmetic on same line
    std::string code3 =
        "pakad ptr = 10;\n"
        "pakad addr = 20;\n"
        "pakad p = &ptr;\n"
        "pakad q = &addr;\n"
        "pakad sum = *p + *q;\n"
        "pakad prod = *p * *q;\n"
        "pakad diff = *q - *p;\n"
        "pakad quo = *q / *p;\n"
        "pakad rem = *q % *p;\n";
    ASSERT_PARSE_OK(code3, "4.3 Dereference expressions combined with all binary arithmetic operators");

    // 4.4 Unary deref vs binary multiply on variable named 'val'
    std::string code4 =
        "pakad val = 5;\n"
        "pakad r1 = val * 2;\n"
        "pakad r2 = 2 * val;\n"
        "pakad r3 = val * val;\n"
        "pakad p = &val;\n"
        "pakad r4 = *p * val;\n"
        "pakad r5 = val * *p;\n";
    ASSERT_PARSE_OK(code4, "4.4 Multiplying variable 'val' (val * 2, 2 * val, val * val, *p * val)");

    // 4.5 Unary address-of vs binary bitwise/logical (&& vs &)
    std::string code5 =
        "pakad a = 10;\n"
        "pakad b = 20;\n"
        "pakad cond = (a > 0) && (b > 0);\n"
        "pakad pa = &a;\n"
        "pakad pb = &b;\n"
        "pakad cond2 = (&a != nalla) && (&b != nalla);\n";
    ASSERT_PARSE_OK(code5, "4.5 Single '&' (address-of) vs '&&' (logical and) in expressions");

    // 4.6 Borrow expressions: udhaar, borrow, udhaar_badal, borrow_mut
    std::string code6 =
        "pakad count = 100;\n"
        "pakad b1 = udhaar count;\n"
        "pakad b2 = borrow count;\n"
        "pakad b3 = udhaar_badal count;\n"
        "pakad b4 = borrow_mut count;\n";
    ASSERT_PARSE_OK(code6, "4.6 Shared and mutable borrow keywords (udhaar, borrow, udhaar_badal, borrow_mut)");
}

// =========================================================================
// Category 5: Double/Triple Dereferences and Address-of Expressions
// =========================================================================
void testCategory5_MultiDerefAndAddressOf() {
    std::cout << "\n=== Category 5: Double/Triple Dereferences and Address-of ===\n";

    // 5.1 Multiple dereferences: **pp, ***ppp, ****pppp
    std::string code1 =
        "pakad x = 42;\n"
        "pakad p = &x;\n"
        "pakad pp = &p;\n"
        "pakad ppp = &pp;\n"
        "pakad pppp = &ppp;\n"
        "pakad v1 = *p;\n"
        "pakad v2 = **pp;\n"
        "pakad v3 = ***ppp;\n"
        "pakad v4 = ****pppp;\n";
    ASSERT_PARSE_OK(code1, "5.1 Deep dereferences (*p, **pp, ***ppp, ****pppp)");

    // 5.2 Parenthesized combinations: &(*p), *(&x), &(*(&x))
    std::string code2 =
        "pakad x = 100;\n"
        "pakad p = &x;\n"
        "pakad p_again = &(*p);\n"
        "pakad x_val = *(&x);\n"
        "pakad round_trip = &(*(&x));\n"
        "pakad deref_trip = *(&(*(p)));\n";
    ASSERT_PARSE_OK(code2, "5.2 Parenthesized combinations &(*p), *(&x), &(*(&x)), *(&(*(p)))");

    // 5.3 Alternating prefix operators without parens: &*&x, *&*p, **&pp
    std::string code3 =
        "pakad x = 50;\n"
        "pakad p = &x;\n"
        "pakad pp = &p;\n"
        "pakad a1 = &*&x;\n"
        "pakad a2 = *&*p;\n"
        "pakad a3 = **&pp;\n"
        "pakad a4 = &**pp;\n";
    ASSERT_PARSE_OK(code3, "5.3 Alternating prefix operators without parens (&*&x, *&*p, **&pp, &**pp)");

    // 5.4 Dereference assignment: *p = val, **pp = val, ***ppp = val
    std::string code4 =
        "pakad x = 1;\n"
        "pakad p = &x;\n"
        "pakad pp = &p;\n"
        "pakad ppp = &pp;\n"
        "*p = 10;\n"
        "**pp = 20;\n"
        "***ppp = 30;\n"
        "asli p = 40;\n"
        "asli asli pp = 50;\n";
    ASSERT_PARSE_OK(code4, "5.4 Multi-level dereference assignments (*p = 10, **pp = 20, ***ppp = 30, asli)");

    // 5.5 Dereference assignment to parenthesized expressions: *(&x) = 99
    std::string code5 =
        "pakad x = 0;\n"
        "*(&x) = 99;\n";
    ASSERT_PARSE_OK(code5, "5.5 Dereference assignment to address-of *(&x) = 99");

    // 5.6 Negative: Address-of rvalue without target
    std::string code6_bad = "pakad p = &;";
    ASSERT_PARSE_FAIL(code6_bad, "5.6 Negative: bare '&;' rejected");

    std::string code7_bad = "pakad p = *;";
    ASSERT_PARSE_FAIL(code7_bad, "5.7 Negative: bare '*;' rejected");

    std::string code8_bad = "* = 10;";
    ASSERT_PARSE_FAIL(code8_bad, "5.8 Negative: '* = 10;' rejected");
}

// =========================================================================
// Category 6: Operator Overload Declarations in Class Bodies
// =========================================================================
void testCategory6_OperatorOverloadDeclarations() {
    std::cout << "\n=== Category 6: Operator Overload Declarations in Class Bodies ===\n";

    // 6.1 The exact dispatch examples: tantrik +, operator -, tantrik [], tantrik []=
    std::string code1 =
        "biradari Vector {\n"
        "    tantrik +(other) {\n"
        "        wapas other;\n"
        "    }\n"
        "    operator -(other) {\n"
        "        wapas other;\n"
        "    }\n"
        "    tantrik [](idx) {\n"
        "        wapas idx;\n"
        "    }\n"
        "    tantrik []=(idx, v) {\n"
        "        khud.data = v;\n"
        "    }\n"
        "}\n";
    ASSERT_PARSE_OK(code1, "6.1 Vector class with tantrik +, operator -, tantrik [], tantrik []=");

    // 6.2 All comparison and arithmetic operators with Desi and English keywords
    std::string code2 =
        "class Matrix {\n"
        "    tantrik +(other) {}\n"
        "    operator -(other) {}\n"
        "    kriya *(other) {}\n"
        "    op /(other) {}\n"
        "    tantrik %(other) {}\n"
        "    tantrik ==(other) {}\n"
        "    tantrik !=(other) {}\n"
        "    tantrik <(other) {}\n"
        "    tantrik <=(other) {}\n"
        "    tantrik >(other) {}\n"
        "    tantrik >=(other) {}\n"
        "    tantrik -() {}\n"
        "    tantrik [](idx) {}\n"
        "    tantrik []=(idx, val) {}\n"
        "}\n";
    ASSERT_PARSE_OK(code2, "6.2 Complete operator set: +, -, *, /, %, ==, !=, <, <=, >, >=, unary -, [], []=");

    // 6.3 Operator declarations with type annotations and return types
    std::string code3 =
        "biradari Complex {\n"
        "    tantrik +(other: Complex): Complex {\n"
        "        wapas other;\n"
        "    }\n"
        "    operator -(other: Complex) -> Complex {\n"
        "        wapas other;\n"
        "    }\n"
        "    tantrik -(): Complex {\n"
        "        wapas khud;\n"
        "    }\n"
        "    tantrik [](idx: ank): ank {\n"
        "        wapas 0;\n"
        "    }\n"
        "    tantrik []=(idx: ank, v: ank): sunya {\n"
        "    }\n"
        "}\n";
    ASSERT_PARSE_OK(code3, "6.3 Operator overloads with typed parameters and return types (:, ->)");

    // 6.4 Mixing operator overloads with destructors, methods, and constructors
    std::string code4 =
        "biradari MegaContainer {\n"
        "    MegaContainer(capacity) {\n"
        "        khud.cap = capacity;\n"
        "    }\n"
        "    jugaad push(item) {}\n"
        "    tantrik [](idx) { wapas khud.cap; }\n"
        "    tantrik []=(idx, v) {}\n"
        "    tantrik +(other) { wapas khud; }\n"
        "    jugaad alvida() {\n"
        "        dikha 0;\n"
        "    }\n"
        "    drop() {}\n"
        "}\n";
    ASSERT_PARSE_OK(code4, "6.4 Class mixing constructor, regular methods, operator overloads, and destructors");

    // 6.5 Negative: Malformed operator declaration
    std::string code5_bad =
        "biradari Bad {\n"
        "    tantrik + + () {}\n"
        "}\n";
    ASSERT_PARSE_FAIL(code5_bad, "6.5 Negative: 'tantrik + +' rejected");

    std::string code6_bad =
        "biradari Bad2 {\n"
        "    tantrik [ (idx) {}\n"
        "}\n";
    ASSERT_PARSE_FAIL(code6_bad, "6.6 Negative: 'tantrik [' missing closing ']' rejected");

    std::string code7_bad =
        "tantrik +(other) {}\n";
    ASSERT_PARSE_FAIL(code7_bad, "6.7 Negative: 'tantrik' at top level outside class rejected");
}

// =========================================================================
// Category 7: AST Verification for Disambiguation Precision
// =========================================================================
void testCategory7_ASTStructuralVerification() {
    std::cout << "\n=== Category 7: AST Structural Node Verification ===\n";

    // 7.1 Verify ClassDeclStmt contains both FuncDeclStmt (alvida, drop) and OperatorDeclStmt
    {
        std::string code =
            "biradari MyClass {\n"
            "    jugaad alvida() {}\n"
            "    drop() {}\n"
            "    tantrik +(other) {}\n"
            "    tantrik -() {}\n"
            "}\n";
        Lexer lex(code);
        Parser parser(lex.tokenize());
        auto ast = parser.parseProgram();
        bool ok = false;
        if (ast.size() == 1) {
            if (auto *cls = dynamic_cast<ClassDeclStmt *>(ast[0].get())) {
                if (cls->methods.size() == 4) {
                    auto *m0 = dynamic_cast<FuncDeclStmt *>(cls->methods[0].get());
                    auto *m1 = dynamic_cast<FuncDeclStmt *>(cls->methods[1].get());
                    auto *op2 = dynamic_cast<OperatorDeclStmt *>(cls->methods[2].get());
                    auto *op3 = dynamic_cast<OperatorDeclStmt *>(cls->methods[3].get());
                    if (m0 && m0->name == "alvida" &&
                        m1 && m1->name == "drop" &&
                        op2 && op2->opSymbol == "+" && op2->name == "__jod__" && !op2->isUnary &&
                        op3 && op3->opSymbol == "-" && op3->name == "__ulta__" && op3->isUnary) {
                        ok = true;
                    }
                }
            }
        }
        ASSERT_TRUE(ok, "7.1 AST verification: ClassDeclStmt methods and operators exact types/names",
                    "Expected 2 FuncDeclStmt (alvida, drop) and 2 OperatorDeclStmt (+ as __jod__, unary - as __ulta__)");
    }

    // 7.2 Verify nested ScopeBlockStmt contains DropStmt
    {
        std::string code =
            "daayra {\n"
            "    alvida x;\n"
            "    scope {\n"
            "        drop y;\n"
            "    }\n"
            "}\n";
        Lexer lex(code);
        Parser parser(lex.tokenize());
        auto ast = parser.parseProgram();
        bool ok = false;
        if (ast.size() == 1) {
            if (auto *s1 = dynamic_cast<ScopeBlockStmt *>(ast[0].get())) {
                if (s1->statements.size() == 2) {
                    auto *d1 = dynamic_cast<DropStmt *>(s1->statements[0].get());
                    auto *s2 = dynamic_cast<ScopeBlockStmt *>(s1->statements[1].get());
                    if (d1 && s2 && s2->statements.size() == 1) {
                        auto *d2 = dynamic_cast<DropStmt *>(s2->statements[0].get());
                        if (d2) ok = true;
                    }
                }
            }
        }
        ASSERT_TRUE(ok, "7.2 AST verification: nested ScopeBlockStmt and DropStmt structure",
                    "Expected ScopeBlockStmt containing DropStmt and child ScopeBlockStmt");
    }

    // 7.3 Verify DerefAssignExpr vs AssignExpr vs AddressOfExpr
    {
        std::string code =
            "pakad p = &ptr;\n"
            "*p = 10;\n"
            "**pp = 20;\n";
        Lexer lex(code);
        Parser parser(lex.tokenize());
        auto ast = parser.parseProgram();
        bool ok = false;
        if (ast.size() == 3) {
            auto *v = dynamic_cast<VarDeclStmt *>(ast[0].get());
            auto *e1 = dynamic_cast<ExprStmt *>(ast[1].get());
            auto *e2 = dynamic_cast<ExprStmt *>(ast[2].get());
            if (v && e1 && e2) {
                auto *addr = dynamic_cast<AddressOfExpr *>(v->initializer.get());
                auto *da1 = dynamic_cast<DerefAssignExpr *>(e1->expression.get());
                auto *da2 = dynamic_cast<DerefAssignExpr *>(e2->expression.get());
                if (addr && da1 && da2) {
                    auto *ident = dynamic_cast<IdentifierExpr *>(addr->operand.get());
                    auto *da2_inner = dynamic_cast<DerefExpr *>(da2->pointer.get());
                    if (ident && ident->name == "ptr" && da2_inner) {
                        ok = true;
                    }
                }
            }
        }
        ASSERT_TRUE(ok, "7.3 AST verification: AddressOfExpr on 'ptr', DerefAssignExpr for *p and **pp",
                    "Expected exact AST node types for pointer assignment & dereference");
    }
}

// =========================================================================
// Category 8: Memory Safety & Leak Stress Test
// =========================================================================
void testCategory8_MemorySafetyStress() {
    std::cout << "\n=== Category 8: Memory Safety & Leak Stress Test ===\n";

    // 8.1 Allocate and parse 2,000 nested scope and drop statements in a loop
    bool ok = true;
    for (int i = 0; i < 2000; i++) {
        std::string code =
            "daayra {\n"
            "    pakad x = " + std::to_string(i) + ";\n"
            "    scope {\n"
            "        pakad y = &x;\n"
            "        drop y;\n"
            "    }\n"
            "    alvida x;\n"
            "}\n";
        try {
            Lexer lex(code);
            Parser parser(lex.tokenize());
            auto ast = parser.parseProgram();
            if (ast.empty()) {
                ok = false;
                break;
            }
        } catch (...) {
            ok = false;
            break;
        }
    }
    ASSERT_TRUE(ok, "8.1 2,000 iterations of ScopeBlockStmt & DropStmt allocation and release",
                "Loop failed or threw an exception");
}

int main() {
    std::cout << "========================================================\n";
    std::cout << "🧪 Challenger 1 (Replacement) Milestone 2 Iteration 3\n";
    std::cout << "   Aggressive Adversarial Syntax & Fuzzing Test Suite\n";
    std::cout << "========================================================\n";

    testCategory1_MethodsVsDrops();
    testCategory2_NestedScopesWithDrop();
    testCategory3_VariablesNamedKeywords();
    testCategory4_UnaryVsArithmetic();
    testCategory5_MultiDerefAndAddressOf();
    testCategory6_OperatorOverloadDeclarations();
    testCategory7_ASTStructuralVerification();
    testCategory8_MemorySafetyStress();

    std::cout << "\n========================================================\n";
    std::cout << "RESULTS: " << g_passCount << " PASSED, " << g_failCount << " FAILED\n";
    std::cout << "========================================================\n";

    return (g_failCount == 0) ? 0 : 1;
}
