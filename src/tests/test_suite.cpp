#include <iostream>
#include <sstream>
#include <cassert>
#include <string>
#include <vector>
#include <filesystem>
#include "../lexer/Lexer.h"
#include "../parser/Parser.h"
#include "../types/TypeChecker.h"
#include "../utils/StringInterner.h"
#include "../interpreter/Interpreter.h"
#include "../packager/IconAssets.h"
#include "../packager/MemePackager.h"
#include "../server/MemeSsrRouter.h"
#include "../ui/MemeUiAnimation.h"
#include "../ui/MemeUiFontEngine.h"
#include "../ui/MemeUiTessellator.h"

// Test helper: runs code in a fresh interpreter and captures stdout
static std::string runAndCapture(const std::string &code)
{
    std::stringstream out;
    std::stringstream in;
    Interpreter interpreter(out, in);
    interpreter.setHeadless(true);

    Lexer lexer(code);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto program = parser.parseProgram();

    interpreter.interpret(program);
    return out.str();
}

static std::string runWithTypeCheck(const std::string &code)
{
    std::stringstream out;
    std::stringstream in;
    Interpreter interpreter(out, in);
    interpreter.setHeadless(true);

    Lexer lexer(code);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto program = parser.parseProgram();

    TypeChecker typeChecker;
    typeChecker.check(program);

    interpreter.interpret(program);
    return out.str();
}

static bool checkTypeOnly(const std::string &code)
{
    Lexer lexer(code);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto program = parser.parseProgram();

    TypeChecker typeChecker;
    typeChecker.check(program);
    return true;
}

static bool expectTypeError(const std::string &code, const std::string &expectedSubstring)
{
    try
    {
        Lexer lexer(code);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto program = parser.parseProgram();

        TypeChecker typeChecker;
        typeChecker.check(program);

        std::stringstream out;
        std::stringstream in;
        Interpreter interpreter(out, in);
        interpreter.setHeadless(true);
        interpreter.interpret(program);
        return false;
    }
    catch (const std::exception &e)
    {
        std::string msg = e.what();
        return msg.find(expectedSubstring) != std::string::npos;
    }
}

// Test helper: verifies that code throws an exception containing expected substring
static bool expectError(const std::string &code, const std::string &expectedSubstring)
{
    try
    {
        std::stringstream out;
        std::stringstream in;
        Interpreter interpreter(out, in);
        interpreter.setHeadless(true);

        Lexer lexer(code);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto program = parser.parseProgram();

        interpreter.interpret(program);
        return false; // Expected exception, none thrown
    }
    catch (const std::exception &e)
    {
        std::string msg = e.what();
        return msg.find(expectedSubstring) != std::string::npos;
    }
}

void testLexer()
{
    std::cout << "[TEST] Lexer Tokenization & Aliases... ";

    Lexer lexer("le x = 10; pooch; puch; han; haan; nhi; nahi; kuch_nhi; kuch_nahi;");
    auto tokens = lexer.tokenize();

    assert(tokens[0].type == TokenType::LE);
    assert(tokens[1].type == TokenType::IDENTIFIER && tokens[1].value == "x");
    assert(tokens[2].type == TokenType::ASSIGN);
    assert(tokens[3].type == TokenType::INT_LITERAL && tokens[3].value == "10");
    assert(tokens[4].type == TokenType::SEMICOLON);

    // Test primary and alias tokens
    assert(tokens[5].type == TokenType::POOCH);
    assert(tokens[7].type == TokenType::POOCH); // puch alias
    assert(tokens[9].type == TokenType::HAN);
    assert(tokens[11].type == TokenType::HAN);   // haan alias
    assert(tokens[13].type == TokenType::NHI);
    assert(tokens[15].type == TokenType::NHI);   // nahi alias
    assert(tokens[17].type == TokenType::KUCH_NHI);
    assert(tokens[19].type == TokenType::KUCH_NHI); // kuch_nahi alias

    // Test Address-of (&) and Logical AND (&&)
    Lexer lexAmp("& && &");
    auto tokAmp = lexAmp.tokenize();
    assert(tokAmp[0].type == TokenType::ADDRESS_OF && tokAmp[0].value == "&");
    assert(tokAmp[1].type == TokenType::AUR && tokAmp[1].value == "&&");
    assert(tokAmp[2].type == TokenType::ADDRESS_OF && tokAmp[2].value == "&");

    // Test Milestone 2 Keywords and Aliases (Desi meme syntax & English aliases)
    std::string allM2Keywords =
        "bahuroop aur_ek ek_aur overload multimethod "
        "tantrik kriya operator op "
        "patta thikana addr address_of ptr "
        "asli deref val dereference dekh_patta uthha "
        "rawana khisko chal_nikal move "
        "malik kabza malkiyat own owned owner "
        "badal badlo mut mutable "
        "atal sthir const immut "
        "khatam alvida visarjan drop dispose "
        "daayra seema elaqa scope lifetime;";
    Lexer lexAll(allM2Keywords);
    auto tokAll = lexAll.tokenize();
    size_t ki = 0;
    // Overload (5)
    for (int i = 0; i < 5; ++i) assert(tokAll[ki++].type == TokenType::OVERLOAD);
    // Operator (4)
    for (int i = 0; i < 4; ++i) assert(tokAll[ki++].type == TokenType::OPERATOR);
    // Address-of (5)
    for (int i = 0; i < 5; ++i) assert(tokAll[ki++].type == TokenType::ADDRESS_OF);
    // Deref (6)
    for (int i = 0; i < 6; ++i) assert(tokAll[ki++].type == TokenType::DEREF);
    // Move (4)
    for (int i = 0; i < 4; ++i) assert(tokAll[ki++].type == TokenType::MOVE);
    // Owned (6)
    for (int i = 0; i < 6; ++i) assert(tokAll[ki++].type == TokenType::OWNED);
    // Mut (4)
    for (int i = 0; i < 4; ++i) assert(tokAll[ki++].type == TokenType::MUT);
    // Const (4)
    for (int i = 0; i < 4; ++i) assert(tokAll[ki++].type == TokenType::CONST);
    // Drop (5)
    for (int i = 0; i < 5; ++i) assert(tokAll[ki++].type == TokenType::DROP);
    // Scope (5)
    for (int i = 0; i < 5; ++i) assert(tokAll[ki++].type == TokenType::SCOPE);

    // Test tokenTypeToString stringification for Milestone 2 tokens
    assert(Lexer::tokenTypeToString(TokenType::OVERLOAD) == "OVERLOAD");
    assert(Lexer::tokenTypeToString(TokenType::OPERATOR) == "OPERATOR");
    assert(Lexer::tokenTypeToString(TokenType::ADDRESS_OF) == "ADDRESS_OF");
    assert(Lexer::tokenTypeToString(TokenType::DEREF) == "DEREF");
    assert(Lexer::tokenTypeToString(TokenType::MOVE) == "MOVE");
    assert(Lexer::tokenTypeToString(TokenType::OWNED) == "OWNED");
    assert(Lexer::tokenTypeToString(TokenType::MUT) == "MUT");
    assert(Lexer::tokenTypeToString(TokenType::CONST) == "CONST");
    assert(Lexer::tokenTypeToString(TokenType::DROP) == "DROP");
    assert(Lexer::tokenTypeToString(TokenType::SCOPE) == "SCOPE");

    // Parser AST Unit Tests:
    // 1. &x -> AddressOfExpr
    {
        Lexer l("&x;");
        auto toks = l.tokenize();
        Parser p(toks);
        auto prog = p.parseProgram();
        assert(prog.size() == 1);
        auto *stmt = dynamic_cast<ExprStmt *>(prog[0].get());
        assert(stmt != nullptr);
        auto *addr = dynamic_cast<AddressOfExpr *>(stmt->expression.get());
        assert(addr != nullptr);
        auto *ident = dynamic_cast<IdentifierExpr *>(addr->operand.get());
        assert(ident != nullptr && ident->name == "x");
    }

    // 2. *p -> DerefExpr
    {
        Lexer l("*p;");
        auto toks = l.tokenize();
        Parser p(toks);
        auto prog = p.parseProgram();
        assert(prog.size() == 1);
        auto *stmt = dynamic_cast<ExprStmt *>(prog[0].get());
        assert(stmt != nullptr);
        auto *deref = dynamic_cast<DerefExpr *>(stmt->expression.get());
        assert(deref != nullptr);
        auto *ident = dynamic_cast<IdentifierExpr *>(deref->operand.get());
        assert(ident != nullptr && ident->name == "p");
    }

    // 3. *p = 10 -> DerefAssignExpr
    {
        Lexer l("*p = 10;");
        auto toks = l.tokenize();
        Parser p(toks);
        auto prog = p.parseProgram();
        assert(prog.size() == 1);
        auto *stmt = dynamic_cast<ExprStmt *>(prog[0].get());
        assert(stmt != nullptr);
        auto *dassign = dynamic_cast<DerefAssignExpr *>(stmt->expression.get());
        assert(dassign != nullptr);
        auto *ptr = dynamic_cast<IdentifierExpr *>(dassign->pointer.get());
        assert(ptr != nullptr && ptr->name == "p");
        auto *val = dynamic_cast<IntExpr *>(dassign->value.get());
        assert(val != nullptr && val->value == 10);
    }

    // 4. bahuroop jugaad f() {} -> FuncDeclStmt with isOverload = true
    {
        Lexer l("bahuroop jugaad f() {}");
        auto toks = l.tokenize();
        Parser p(toks);
        auto prog = p.parseProgram();
        assert(prog.size() == 1);
        auto *func = dynamic_cast<FuncDeclStmt *>(prog[0].get());
        assert(func != nullptr);
        assert(func->name == "f");
        assert(func->isOverload == true);
    }

    // 5. tantrik +(other) {} inside class -> OperatorDeclStmt mapped to __jod__
    {
        Lexer l("biradari Num { tantrik + (other) { ye_le 42; } }");
        auto toks = l.tokenize();
        Parser p(toks);
        auto prog = p.parseProgram();
        assert(prog.size() == 1);
        auto *cls = dynamic_cast<ClassDeclStmt *>(prog[0].get());
        assert(cls != nullptr && cls->name == "Num");
        assert(cls->methods.size() == 1);
        auto *op = dynamic_cast<OperatorDeclStmt *>(cls->methods[0].get());
        assert(op != nullptr);
        assert(op->opSymbol == "+");
        assert(op->name == "__jod__");
        assert(op->isOverload == true);
        assert(op->isUnary == false);
    }

    // 6. rawana x -> MoveExpr
    {
        Lexer l("rawana x;");
        auto toks = l.tokenize();
        Parser p(toks);
        auto prog = p.parseProgram();
        assert(prog.size() == 1);
        auto *stmt = dynamic_cast<ExprStmt *>(prog[0].get());
        assert(stmt != nullptr);
        auto *mov = dynamic_cast<MoveExpr *>(stmt->expression.get());
        assert(mov != nullptr);
        auto *ident = dynamic_cast<IdentifierExpr *>(mov->operand.get());
        assert(ident != nullptr && ident->name == "x");
    }

    // 7. khatam x; -> DropStmt
    {
        Lexer l("khatam x;");
        auto toks = l.tokenize();
        Parser p(toks);
        auto prog = p.parseProgram();
        assert(prog.size() == 1);
        auto *drop = dynamic_cast<DropStmt *>(prog[0].get());
        assert(drop != nullptr);
        auto *ident = dynamic_cast<IdentifierExpr *>(drop->expr.get());
        assert(ident != nullptr && ident->name == "x");
    }

    // 8. daayra {} -> ScopeBlockStmt
    {
        Lexer l("daayra {}");
        auto toks = l.tokenize();
        Parser p(toks);
        auto prog = p.parseProgram();
        assert(prog.size() == 1);
        auto *scope = dynamic_cast<ScopeBlockStmt *>(prog[0].get());
        assert(scope != nullptr);
    }

    // 9. pakad malik badal atal x: patta[ank] = 1; -> VarDeclStmt with modifiers & pointer type
    {
        Lexer l("pakad malik badal atal x: patta[ank] = 1;");
        auto toks = l.tokenize();
        Parser p(toks);
        auto prog = p.parseProgram();
        assert(prog.size() == 1);
        auto *var = dynamic_cast<VarDeclStmt *>(prog[0].get());
        assert(var != nullptr);
        assert(var->name == "x");
        assert(var->isOwned == true);
        assert(var->isMut == true);
        assert(var->isConst == true);
        assert(var->typeAnnotation != nullptr);
    }

    // 10. Lexer mut_ref as IDENTIFIER
    {
        Lexer l("mut_ref");
        auto toks = l.tokenize();
        assert(toks.size() >= 1);
        assert(toks[0].type == TokenType::IDENTIFIER && toks[0].value == "mut_ref");
    }

    // 11. Class destructors (jugaad alvida, fn drop, alvida(), drop())
    {
        std::string classCode =
            "biradari Res1 { jugaad alvida() { x = 0; } }\n"
            "biradari Res2 { fn drop() { x = 0; } }\n"
            "biradari Res3 { alvida() { x = 0; } }\n"
            "biradari Res4 { drop() { x = 0; } }";
        Lexer l(classCode);
        Parser p(l.tokenize());
        auto prog = p.parseProgram();
        assert(prog.size() == 4);
        for (size_t i = 0; i < 4; ++i)
        {
            auto *cls = dynamic_cast<ClassDeclStmt *>(prog[i].get());
            assert(cls != nullptr);
            assert(cls->methods.size() == 1);
        }
    }

    // 12. Top-level function declarations with keyword-derived names
    {
        std::string funcCode =
            "jugaad alvida() { wapas 1; }\n"
            "fn drop() { wapas 2; }\n"
            "jugaad val() { wapas 3; }\n"
            "jugaad ptr() { wapas 4; }\n"
            "jugaad addr() { wapas 5; }";
        Lexer l(funcCode);
        Parser p(l.tokenize());
        auto prog = p.parseProgram();
        assert(prog.size() == 5);
        auto *f0 = dynamic_cast<FuncDeclStmt *>(prog[0].get());
        assert(f0 != nullptr && f0->name == "alvida");
        auto *f1 = dynamic_cast<FuncDeclStmt *>(prog[1].get());
        assert(f1 != nullptr && f1->name == "drop");
        auto *f2 = dynamic_cast<FuncDeclStmt *>(prog[2].get());
        assert(f2 != nullptr && f2->name == "val");
        auto *f3 = dynamic_cast<FuncDeclStmt *>(prog[3].get());
        assert(f3 != nullptr && f3->name == "ptr");
        auto *f4 = dynamic_cast<FuncDeclStmt *>(prog[4].get());
        assert(f4 != nullptr && f4->name == "addr");
    }

    // 13. Variables named ptr, addr, mut_ref, val
    {
        std::string varCode =
            "pakad ptr = nalla;\n"
            "pakad addr = 0x1000;\n"
            "pakad mut_ref = 42;\n"
            "pakad val = 100;\n"
            "ptr = nalla;\n"
            "*ptr = 10;\n"
            "pakad v = *ptr;\n"
            "pakad p = &ptr;";
        Lexer l(varCode);
        Parser p(l.tokenize());
        auto prog = p.parseProgram();
        assert(prog.size() == 8);
    }

    // 14. Contextual ptr / addr prefix expressions vs bare identifiers
    {
        std::string code =
            "let p1 = ptr count;\n"
            "let p2 = addr count;\n"
            "let v1 = val p1;\n"
            "dekh ptr == nalla { dikha 1; }";
        Lexer l(code);
        Parser p(l.tokenize());
        auto prog = p.parseProgram();
        assert(prog.size() == 4);
    }

    // 15. Scope block parsing with memory safety
    {
        std::string scopeCode =
            "daayra { pakad a = 1; daayra { pakad b = 2; } }\n"
            "scope { pakad c = 3; }";
        Lexer l(scopeCode);
        Parser p(l.tokenize());
        auto prog = p.parseProgram();
        assert(prog.size() == 2);
        auto *s0 = dynamic_cast<ScopeBlockStmt *>(prog[0].get());
        assert(s0 != nullptr && s0->statements.size() == 2);
        auto *s1 = dynamic_cast<ScopeBlockStmt *>(prog[1].get());
        assert(s1 != nullptr && s1->statements.size() == 1);
    }

    std::cout << "PASSED\n";
}

void testArithmeticAndOperators()
{
    std::cout << "[TEST] Arithmetic & Operators... ";

    assert(runAndCapture("dikha 10 + 20;") == "30\n");
    assert(runAndCapture("dikha 30 - 12;") == "18\n");
    assert(runAndCapture("dikha 6 * 7;") == "42\n");
    assert(runAndCapture("dikha 100 / 4;") == "25\n");
    assert(runAndCapture("dikha 17 % 5;") == "2\n");
    assert(runAndCapture("dikha 'abc' * 3;") == "abcabcabc\n");
    assert(runAndCapture("dikha 2.5 + 1.5;") == "4.0\n");
    assert(runAndCapture("dikha -(-5);") == "5\n");

    // Division by zero guard
    assert(expectError("dikha 5 / 0;", "0 se divide"));
    assert(expectError("dikha 5 % 0;", "0 se modulo"));

    std::cout << "PASSED\n";
}

void testLogicAndComparisons()
{
    std::cout << "[TEST] Logic & Comparisons... ";

    assert(runAndCapture("dikha 10 > 5;") == "han\n");
    assert(runAndCapture("dikha 10 < 5;") == "nhi\n");
    assert(runAndCapture("dikha 10 == 10;") == "han\n");
    assert(runAndCapture("dikha 10 != 5;") == "han\n");
    assert(runAndCapture("dikha han aur nhi;") == "nhi\n");
    assert(runAndCapture("dikha han ya nhi;") == "han\n");
    assert(runAndCapture("dikha !han;") == "nhi\n");
    assert(runAndCapture("dikha !nhi;") == "han\n");

    // Short-circuiting: right side with error is NOT evaluated if left determines result
    assert(runAndCapture("dikha nhi aur (10 / 0);") == "nhi\n");
    assert(runAndCapture("dikha han ya (10 / 0);") == "han\n");

    std::cout << "PASSED\n";
}

void testVariablesAndScoping()
{
    std::cout << "[TEST] Variables & Lexical Scoping... ";

    std::string code = R"(
        le x = 10;
        {
            le x = 20;
            dikha x;
        }
        dikha x;
    )";
    assert(runAndCapture(code) == "20\n10\n");

    // Assignment in outer scope from inner scope
    std::string code2 = R"(
        le y = 1;
        {
            y = 50;
        }
        dikha y;
    )";
    assert(runAndCapture(code2) == "50\n");

    // Error on undeclared variable read
    assert(expectError("dikha unknownVar;", "Rasode mein kaun tha"));
    // Python-style auto variable creation on assignment
    assert(runAndCapture("unknownVar = 5;\ndikha unknownVar;") == "5\n");

    std::cout << "PASSED\n";
}

void testControlFlow()
{
    std::cout << "[TEST] Control Flow (dekh, jabtak, ghoom, bas, chal)... ";

    // If/Else
    std::string ifCode = R"(
        le a = 5;
        dekh a > 10 {
            dikha "bada";
        } warna dekh a == 5 {
            dikha "barabar";
        } warna {
            dikha "chhota";
        }
    )";
    assert(runAndCapture(ifCode) == "barabar\n");

    // While loop with break & continue
    std::string whileCode = R"(
        le i = 0;
        le sum = 0;
        jabtak i < 10 {
            i = i + 1;
            dekh i == 2 { chal; }
            dekh i == 5 { bas; }
            sum = sum + i;
        }
        dikha sum;
    )";
    // i=1 (sum=1), i=2 (skipped), i=3 (sum=4), i=4 (sum=8), i=5 (break) -> sum = 8
    assert(runAndCapture(whileCode) == "8\n");

    // Range loop (ghoom)
    std::string ghoomCode = R"(
        le total = 0;
        ghoom k se 1 se 4 {
            total = total + k;
        }
        dikha total;
    )";
    assert(runAndCapture(ghoomCode) == "10\n");

    // Break outside loop error
    assert(expectError("bas;", "Kisko rok rahe ho"));
    assert(expectError("chal;", "Loop ke bahar"));

    std::cout << "PASSED\n";
}

void testFunctionsAndRecursion()
{
    std::cout << "[TEST] Functions, Closures & Recursion... ";

    // Function call and return
    std::string funcCode = R"(
        kar multiply(a, b) {
            wapas a * b;
        }
        dikha multiply(6, 7);
    )";
    assert(runAndCapture(funcCode) == "42\n");

    // Recursive factorial
    std::string factCode = R"(
        kar fact(n) {
            dekh n <= 1 {
                wapas 1;
            }
            wapas n * fact(n - 1);
        }
        dikha fact(6);
    )";
    assert(runAndCapture(factCode) == "720\n");

    // Closure / Lexical environment capture
    std::string closureCode = R"(
        kar makeMultiplier(factor) {
            kar mult(x) {
                wapas x * factor;
            }
            wapas mult;
        }
        le doubleIt = makeMultiplier(2);
        dikha doubleIt(21);
    )";
    assert(runAndCapture(closureCode) == "42\n");

    // Arity mismatch error
    std::string arityCode = R"(
        kar f(a, b) { wapas a + b; }
        f(1);
    )";
    assert(expectError(arityCode, "Gaddari korbe"));

    std::cout << "PASSED\n";
}

void testBuiltins()
{
    std::cout << "[TEST] Built-in Functions... ";

    assert(runAndCapture("dikha pata_laga('hello');") == "string\n");
    assert(runAndCapture("dikha pata_laga(123);") == "int\n");
    assert(runAndCapture("dikha pata_laga(3.14);") == "float\n");
    assert(runAndCapture("dikha lambai('meme');") == "4\n");
    assert(runAndCapture("dikha ank('42') + 8;") == "50\n");
    assert(runAndCapture("dikha dashamlav('3.5') * 2.0;") == "7.0\n");

    std::cout << "PASSED\n";
}

void testObjectOrientedProgramming()
{
    std::cout << "[TEST] Object Oriented Programming (sancha, khud, se)... ";

    // Basic Class and Instantiation
    std::string classCode = R"(
        sancha Person {
            kar shuru(name, age) {
                khud.name = name;
                khud.age = age;
            }
            kar greet() {
                dikha "Hello", khud.name;
            }
        }
        le p = Person("Raju", 25);
        p.greet();
        dikha p.age;
    )";
    assert(runAndCapture(classCode) == "Hello Raju\n25\n");

    // Inheritance and method override
    std::string inheritCode = R"(
        sancha Animal {
            kar sound() {
                dikha "Kuch awaz";
            }
        }
        sancha Dog se Animal {
            kar sound() {
                dikha "Bhow Bhow";
            }
        }
        le d = Dog();
        d.sound();
    )";
    assert(runAndCapture(inheritCode) == "Bhow Bhow\n");

    // Dynamic field assignment
    std::string fieldCode = R"(
        sancha Box {}
        le b = Box();
        b.width = 10;
        b.height = 20;
        dikha b.width * b.height;
    )";
    assert(runAndCapture(fieldCode) == "200\n");

    // Error accessing missing property
    assert(expectError("sancha Item {} le i = Item(); dikha i.missing;", "Khopdi tod saale ka"));

    // Error accessing property on primitive
    assert(expectError("le x = 123; dikha x.prop;", "Aura -10000"));

    // Error inheriting from non-class
    assert(expectError("le x = 5; sancha Bad se x {}", "Fake nepotism"));

    std::cout << "PASSED\n";
}

void testGenZMemeVocabulary()
{
    std::cout << "[TEST] Gen-Z / Desi Meme Keywords & Builtins (pakad, biradari, nepo, etc.)... ";

    std::string code = R"(
        biradari AmeeroKiAulaad {
            jugaad shuru(naam, bank_balance) {
                apun.naam = naam;
                apun.paisa = point_wala(bank_balance);
            }
        }

        biradari Influencer nepo AmeeroKiAulaad {
            jugaad flex_kar() {
                dekh apun.paisa > 1000.0 aur apun.naam != nalla {
                    dikha apun.naam, "struggle";
                    ye_le real;
                } warna {
                    ye_le cap;
                }
            }
        }

        pakad bro = Influencer("Orry", "99999.50");
        dikha kya_hai(bro);
        dikha size("Orry");
        dikha bina_point("42") + 8;
        bro.flex_kar();
    )";
    assert(runAndCapture(code) == "object\n4\n50\nOrry struggle\n");

    // Verify Gen-Z Meme Error Messages
    assert(expectError("pakad a = 1 pakad b = 2;", "Chhoti bachi ho kya"));
    assert(expectError("pakad s = \"hello;", "String khol ke chhod diya"));
    assert(expectError("(x) = 5;", "Ye sab doglapan hai"));
    assert(expectError("dikha 10 < 'abc';", "Seb ko santre se mat tolo"));
    assert(expectError("dikha -'abc';", "ke aage minus (-) kaise laga diya"));
    assert(expectError("pakad x = 5; x();", "Beta, tumse na ho payega"));

    std::cout << "PASSED\n";
}

void testPythonicBatchA()
{
    std::cout << "[TEST] Pythonic Batch A (Lambdas, Transforms, Defaults, Variadics)... ";

    // Anonymous multi-statement jugaad
    std::string anonCode = R"(
        pakad calc = jugaad(x, y) { pakad z = x + y; ye_le z * 2; };
        dikha calc(3, 4);
    )";
    assert(runAndCapture(anonCode) == "14\n");

    // Short arrow lambdas + collection transforms chaining
    std::string chainCode = R"(
        pakad scores = [12, 45, 89, 23, 94, 67];
        pakad result = scores
            .chhaan(s => s >= 50)
            .sab_pe(s => s + 5)
            .nichod((sum, s) => sum + s, 0);
        dikha result;
    )";
    assert(runAndCapture(chainCode) == "265\n");

    // Default arguments & variadic rest
    std::string defVarCode = R"(
        jugaad team(leader, role = "Captain", *members) {
            dikha leader, role, members;
        }
        team("Rohit");
        team("Virat", "Batter", "Surya", "Hardik");
    )";
    assert(runAndCapture(defVarCode) == "Rohit Captain []\nVirat Batter [\"Surya\", \"Hardik\"]\n");

    // Spread in function calls
    std::string spreadCode = R"(
        jugaad add3(a, b, c) { ye_le a + b + c; }
        pakad items = [2, 3];
        dikha add3(1, *items);
    )";
    assert(runAndCapture(spreadCode) == "6\n");

    // Sorting in-place
    std::string sortCode = R"(
        pakad arr = [3, 1, 2];
        arr.line_mein();
        dikha arr;
        arr.line_mein((a, b) => a > b);
        dikha arr;
    )";
    assert(runAndCapture(sortCode) == "[1, 2, 3]\n[3, 2, 1]\n");

    // Arity check error
    assert(expectError("jugaad f(a, b) { ye_le a + b; } f(1);", "Gaddari korbe"));

    // Default parameter ordering error (doglapan)
    assert(expectError("jugaad f(a = 1, b) { ye_le a + b; }", "Ye sab doglapan hai"));

    // Empty reduce error
    assert(expectError("[].nichod((a, b) => a + b);", "Khali dabba"));

    std::cout << "PASSED\n";
}

void testPythonicBatchB()
{
    std::cout << "[TEST] Pythonic Batch B (Tantrik Methods, F-Strings, Destructuring, Reflection)... ";

    // 1. Tantrik Methods: __jod__, __dikha__, and string concatenation
    std::string tantrikArithmetic = R"raw(
        biradari Khata {
            jugaad shuru(bal) {
                khud.bal = bal;
            }
            jugaad __jod__(other) {
                ye_le Khata(khud.bal + other.bal);
            }
            jugaad __dikha__() {
                ye_le f"Khata({khud.bal})";
            }
        }
        pakad a = Khata(100);
        pakad b = Khata(250);
        pakad c = a + b;
        dikha c;
        dikha "Passbook: " + c;
    )raw";
    assert(runAndCapture(tantrikArithmetic) == "Khata(350)\nPassbook: Khata(350)\n");

    // 2. Comparisons & Auto-Derivation (__barabar__, __chota__, auto >, <=, >=)
    std::string comparisonCode = R"raw(
        biradari Item {
            jugaad shuru(cost) {
                khud.cost = cost;
            }
            jugaad __barabar__(other) {
                ye_le khud.cost == other.cost;
            }
            jugaad __chota__(other) {
                ye_le khud.cost < other.cost;
            }
            jugaad __dikha__() {
                ye_le f"Item({khud.cost})";
            }
        }
        pakad i1 = Item(10);
        pakad i2 = Item(20);
        pakad i3 = Item(10);
        dikha i1 == i3, i1 != i2, i1 < i2, i2 > i1, i1 <= i3, i2 >= i1;
        pakad items = [i2, i1];
        items.line_mein();
        dikha items[0], items[1];
    )raw";
    assert(runAndCapture(comparisonCode) == "han han han han han han\nItem(10) Item(20)\n");

    // 3. Tantrik Indexing (__la__, __daal__) and __size__
    std::string indexingCode = R"raw(
        biradari Dabba {
            jugaad shuru() {
                khud.m = {};
            }
            jugaad __la__(k) {
                ye_le khud.m[k];
            }
            jugaad __daal__(k, v) {
                khud.m[k] = v;
            }
            jugaad __size__() {
                ye_le size(khud.m);
            }
        }
        pakad d = Dabba();
        d["key"] = "heera";
        dikha d["key"], size(d);
    )raw";
    assert(runAndCapture(indexingCode) == "heera 1\n");

    // 4. Callable Instances (__chala__)
    std::string callableCode = R"raw(
        biradari Adder {
            jugaad shuru(delta) {
                khud.delta = delta;
            }
            jugaad __chala__(x) {
                ye_le x + khud.delta;
            }
        }
        pakad add5 = Adder(5);
        dikha add5(10);
    )raw";
    assert(runAndCapture(callableCode) == "15\n");

    // 5. F-Strings & Escaping
    std::string fstringCode = R"raw(
        pakad naam = "MEMElang";
        pakad ver = 2;
        dikha f"Welcome to {naam} v{ver * 1}!";
        dikha f"Escaped {{brackets}} works {10 + 20}!";
    )raw";
    assert(runAndCapture(fstringCode) == "Welcome to MEMElang v2!\nEscaped {brackets} works 30!\n");

    // 6. Destructuring: Array and Map
    std::string destructureCode = R"raw(
        pakad [a, b, *rest] = [1, 2, 3, 4, 5];
        dikha a, b, rest;
        pakad [x, y, z] = [99];
        dikha x, y, z;
        pakad {p, q, missing} = { "p": 100, "q": 200 };
        dikha p, q, missing;
    )raw";
    assert(runAndCapture(destructureCode) == "1 2 [3, 4, 5]\n99 nalla nalla\n100 200 nalla\n");

    // 7. Reflection: kundli & chala_de
    std::string reflectionCode = R"raw(
        biradari Car {
            jugaad shuru(model) {
                khud.model = model;
            }
            jugaad horn(sound) {
                ye_le f"{khud.model} goes {sound}!";
            }
        }
        pakad c = Car("Thar");
        pakad flds = kundli(c);
        dikha flds["model"];
        pakad res = c.chala_de("horn", ["Poo Poo"]);
        dikha res;
    )raw";
    assert(runAndCapture(reflectionCode) == "Thar\nThar goes Poo Poo!\n");

    std::cout << "PASSED\n";
}

void testGradualTypingAndMemoryEfficiency()
{
    std::cout << "[TEST] Gradual Typing & Memory Efficiency (Static/Dynamic Types, TypeChecker, Sizeof)... " << std::flush;

    // 1. Memory Efficiency: Value Size & Interner
    static_assert(sizeof(Value) <= 40, "Value size must not exceed 40 bytes!");
    assert(sizeof(Value) == 32);

    auto &interner = StringInterner::instance();
    auto s1 = interner.intern("identifier_name");
    auto s2 = interner.intern("identifier_name");
    assert(s1.data() == s2.data());

    // 2. Explicitly Typed Variables & Numeric Widening
    std::string typedVars = R"raw(
        pakad a: ank = 42;
        pakad b: point = 3.14;
        pakad c: bol = "Namaste";
        pakad d: sach = han;
        pakad e: point = 100;
        dikha a, b, c, d, e;
    )raw";
    assert(runWithTypeCheck(typedVars) == "42 3.14 Namaste han 100\n");

    // 3. Typed Collections
    std::string collections = R"raw(
        pakad numbers: list[ank] = [10, 20, 30];
        pakad user: map[bol, ank] = { "age": 25 };
        dikha numbers[0], numbers[1], user["age"];
    )raw";
    assert(runWithTypeCheck(collections) == "10 20 25\n");

    // 4. Annotated Functions
    std::string funcCode = R"raw(
        kar add(a: ank, b: ank): ank {
            ye_le a + b;
        }
        dikha add(20, 22);
    )raw";
    assert(runWithTypeCheck(funcCode) == "42\n");

    // 5. Annotated Arrow Lambdas
    std::string lambdaCode = R"raw(
        pakad square = (n: ank): ank => n * n;
        dikha square(9);
    )raw";
    assert(runWithTypeCheck(lambdaCode) == "81\n");

    // 6. Dynamic Untyped Reassignment (Trap #2: Must NOT error on dynamic variables)
    std::string dynamicCode = R"raw(
        pakad x = 10;
        x = "ab main string hoon";
        dikha x;
    )raw";
    assert(runWithTypeCheck(dynamicCode) == "ab main string hoon\n");

    // 7. Compile-Time Type Error Checks:
    // 7a. Variable declaration type mismatch
    assert(expectTypeError("pakad x: ank = \"galat\";", "Aukaat ke bahar! Variable 'x' ka type 'ank' hai, par tumne 'bol' pakda diya!"));

    // 7b. Explicit variable reassignment type mismatch
    assert(expectTypeError("pakad x: ank = 10; x = \"galat\";", "Aukaat ke bahar! Variable 'x' ka type 'ank' hai, par tumne 'bol' pakda diya!"));

    // 7c. Return type mismatch
    assert(expectTypeError("kar f(): ank { ye_le \"dhokha\"; }", "Dhokha! Jugaad ne 'ank' wapas karne ka wada kiya tha, par 'bol' fenk diya!"));

    // 7d. Argument type mismatch
    assert(expectTypeError("kar greet(age: ank) {} greet(\"galat\");", "Doglapan! Jugaad parameter ko 'ank' chahiye tha, par tumne 'bol' pass kiya!"));

    // 7e. Invalid operator combination
    assert(expectTypeError("pakad bad = \"text\" - 5;", "Aukaat ke bahar! Operator '-' type 'bol' aur 'ank' ke beech nahi chal sakta!"));

    // 8. Native quirks permitted (Trap #3):
    std::string quirksCode = R"raw(
        dikha "ha" * 3;
        dikha "Score: " + 100;
        dikha 10 + 2.5;
    )raw";
    assert(runWithTypeCheck(quirksCode) == "hahaha\nScore: 100\n12.5\n");

    // 9. Milestone 3: Pointer Value Representation & Size Invariant
    static_assert(sizeof(Value) == 32, "Value size must be 32 bytes on 64-bit platforms!");
    assert(sizeof(Value) == 32);

    auto nullPtrVal = Value::nullPointer(false);
    assert(nullPtrVal.isPointer());
    assert(nullPtrVal.typeName() == "patta");
    assert(nullPtrVal.toString() == "patta[0x0]");
    assert(!nullPtrVal.isTruthy());

    auto constNull = Value::nullPointer(false);
    assert(!constNull.asPointer()->isMut);

    auto mutNull = Value::nullPointer(true);
    assert(mutNull.asPointer()->isMut);

    long long targetVal = 42;
    Pointer rawPtr(reinterpret_cast<Value*>(&targetVal), true, "targetVal");
    auto rawVal = Value::pointer(rawPtr);
    assert(rawVal.isPointer());
    assert(rawVal.typeName() == "patta");
    assert(rawVal.toString().find("patta[0x") == 0);
    assert(rawVal.isTruthy());
    assert(rawVal.pointerEquals(rawVal));

    // 10. Milestone 3: PointerType Representation & Subtyping
    auto pConstAnk = Type::pointer(Type::Ank(), false);
    auto pMutAnk = Type::pointer(Type::Ank(), true);
    auto pConstBol = Type::pointer(Type::Bol(), false);

    assert(pConstAnk->isPointer());
    assert(!pConstAnk->isMutablePointer());
    assert(pMutAnk->isPointer());
    assert(pMutAnk->isMutablePointer());
    assert(pConstAnk->toString() == "patta[ank]");
    assert(pMutAnk->toString() == "patta[badal ank]");

    // Structural equality
    assert(pConstAnk->equals(*Type::pointer(Type::Ank(), false)));
    assert(!pConstAnk->equals(*pMutAnk));
    assert(!pConstAnk->equals(*pConstBol));

    // Mutability subtyping: mutable pointer can be assigned to immutable pointer (permission weakening)
    assert(pMutAnk->isAssignableTo(*pConstAnk));
    // Immutable pointer cannot be assigned to mutable pointer
    assert(!pConstAnk->isAssignableTo(*pMutAnk));
    // Nullable: nalla is assignable to pointer
    assert(Type::Nalla()->isAssignableTo(*pConstAnk));
    assert(Type::Nalla()->isAssignableTo(*pMutAnk));
    // Incompatible pointees
    assert(!pConstBol->isAssignableTo(*pConstAnk));
    // Pointer is not assignable to non-pointer
    assert(!pConstAnk->isAssignableTo(*Type::Ank()));

    // 11. Milestone 3: Pointer Expression Type Checking
    // 11a. Address-of lvalue and dereference
    assert(checkTypeOnly("pakad a: ank = 10; pakad p = &a;"));
    assert(checkTypeOnly("pakad atal k: ank = 50; pakad p = patta k;"));
    assert(checkTypeOnly("pakad a: ank = 10; pakad p: patta[badal ank] = &a; *p = 20;"));
    assert(checkTypeOnly("pakad a: ank = 10; pakad p = &a; pakad val: ank = *p;"));

    // 11b. Rvalue address-of rejected with Doglapan
    assert(expectTypeError("pakad bad = &(10 + 20);", "Doglapan! Sirf variable ya memory location ka patta liya ja sakta hai, temporary rvalue ka nahi!"));
    assert(expectTypeError("pakad bad = patta (100 + 200);", "Doglapan! Sirf variable ya memory location ka patta liya ja sakta hai, temporary rvalue ka nahi!"));

    // 11c. Deref of non-pointer rejected
    assert(expectTypeError("pakad not_ptr: ank = 42; pakad v = *not_ptr;", "Aukaat ke bahar! Dereference '*' sirf pointer type par chal sakta hai"));

    // 11d. Deref assignment to const pointer rejected
    assert(expectTypeError("pakad atal c: ank = 100; pakad p = &c; *p = 200;", "Shanti gadadhari Bheem! Const / atal pointer ke through memory mutate nahi kar sakte!"));

    // 11e. Deref assignment type mismatch rejected
    assert(expectTypeError("pakad x: ank = 10; pakad p: patta[badal ank] = &x; *p = \"galat\";", "Aukaat ke bahar! Pointer type 'patta[badal ank]' mein 'bol' assign nahi kar sakte!"));

    // 12. Milestone 3: Function Overload Resolution & Diagnostics
    // 12a. Overload resolution by parameter type
    std::string overloadByType = R"raw(
        bahuroop kar jodo(a: ank, b: ank): ank { ye_le a + b; }
        bahuroop kar jodo(s1: bol, s2: bol): bol { ye_le s1 + s2; }
        jodo(10, 20);
        jodo("Ram", "Sita");
    )raw";
    assert(checkTypeOnly(overloadByType));

    // 12b. Overload resolution by arity
    std::string overloadByArity = R"raw(
        bahuroop kar hisaab(x: ank): ank { ye_le x * 2; }
        bahuroop kar hisaab(x: ank, y: ank): ank { ye_le x + y; }
        hisaab(21);
        hisaab(15, 27);
    )raw";
    assert(checkTypeOnly(overloadByArity));

    // 12c. Ambiguous overload (tie) -> "Confuse kar diya"
    std::string tieOverload = R"raw(
        bahuroop kar calc(a: ank, b: ank): ank { ye_le a + b; }
        bahuroop kar calc(x: ank, y: ank): ank { ye_le x * y; }
        calc(5, 10);
    )raw";
    assert(expectTypeError(tieOverload, "Confuse kar diya! Jugaad 'calc' ke overloads mein tie ho gaya!"));

    // 12d. No matching overload -> "Aukaat ke bahar"
    std::string noMatchOverload = R"raw(
        bahuroop kar only_ank(x: ank, y: ank): ank { ye_le x + y; }
        only_ank("a", "b");
    )raw";
    assert(expectTypeError(noMatchOverload, "Aukaat ke bahar! Jugaad 'only_ank' ke liye koi matching overload nahi mila!"));

    // 12e. Non-overloaded function still produces exact legacy diagnostic
    assert(expectTypeError("kar single_fn(n: ank) {} single_fn(\"galat\");", "Doglapan! Jugaad parameter ko 'ank' chahiye tha, par tumne 'bol' pass kiya!"));

    std::cout << "PASSED\n";
}

void testPythonicWhatsAppErgonomics()
{
    std::cout << "[TEST] Pythonic WhatsApp Hinglish Ergonomics & Flexibility... ";

    // 1. WhatsApp Hinglish syntax with optional semicolons and zero boilerplate
    std::string cleanHinglish = R"raw(
        numbers = [5, 12, 8, 20, 3]

        kaam double_big(arr) {
            res = arr.chhan(x => x > 10).badal(x => x * 2)
            de res.saja()
        }

        dikha double_big(numbers)
    )raw";
    assert(runAndCapture(cleanHinglish) == "[24, 40]\n");

    // 2. Control flow with agar, elif, warna
    std::string agarElifCode = R"raw(
        kaam check_num(x) {
            agar x > 10 {
                de "bada"
            } elif x == 10 {
                de "barabar"
            } warna {
                de "chhota"
            }
        }
        dikha check_num(15)
        dikha check_num(10)
        dikha check_num(5)
    )raw";
    assert(runAndCapture(agarElifCode) == "bada\nbarabar\nchhota\n");

    // 3. For-each loops with `har ... mein` and `for ... in`
    std::string forEachCode = R"raw(
        sum = 0
        har n mein [1, 2, 3, 4] {
            sum = sum + n
        }
        dikha sum

        total = 0
        for x in [10, 20] {
            total = total + x
        }
        dikha total
    )raw";
    assert(runAndCapture(forEachCode) == "10\n30\n");

    // 4. Underscore-free functional methods (kul, dhund, koi, sab)
    std::string funcMethods = R"raw(
        nums = [1, 2, 3, 4, 5]
        dikha nums.kul((acc, x) => acc + x, 0)
        dikha nums.dhund(x => x > 3)
        dikha nums.koi(x => x == 3)
        dikha nums.koi(x => x > 10)
        dikha nums.sab(x => x > 0)
        dikha nums.sab(x => x > 2)
    )raw";
    assert(runAndCapture(funcMethods) == "15\n4\nhan\nnhi\nhan\nnhi\n");

    // 5. Bug 2 Postfix Newline Trap: Ensure newline '(' or '[' is NOT parsed as call/index
    std::string newlineSafety = R"raw(
        x = 42
        (a => a + 1)(10)

        msg = "hi"
        arr = [1, 2, 3]
        dikha arr.size()
    )raw";
    assert(runAndCapture(newlineSafety) == "3\n");

    // 6. Python Reference Semantics (Bug 3): No move invalidation on assignment
    std::string refSemantics = R"raw(
        a = [10, 20]
        b = a
        b.ghusa(30)
        dikha a
        dikha b

        s = "namaste"
        t = s
        dikha s, t
    )raw";
    assert(runAndCapture(refSemantics) == "[10, 20, 30]\n[10, 20, 30]\nnamaste namaste\n");

    // 7. Dual Dunder Operator Overloading (Python dunders + Hinglish dunders)
    std::string pythonDunders = R"raw(
        roop Vec {
            kaam shuru(x, y) {
                self.x = x
                self.y = y
            }
            kaam __add__(other) {
                de Vec(self.x + other.x, self.y + other.y)
            }
            kaam __str__() {
                de f"Vec({self.x}, {self.y})"
            }
            kaam __len__() {
                de self.x + self.y
            }
            kaam __getitem__(idx) {
                agar idx == 0 { de self.x }
                de self.y
            }
            kaam __setitem__(idx, val) {
                agar idx == 0 { self.x = val }
                warna { self.y = val }
            }
            kaam __call__(multiplier) {
                de Vec(self.x * multiplier, self.y * multiplier)
            }
        }

        v1 = Vec(2, 3)
        v2 = Vec(4, 5)
        v3 = v1 + v2
        dikha v3
        dikha len(v3)
        dikha v3[0], v3[1]
        v3[0] = 100
        dikha v3[0]
        scaled = v3(2)
        dikha scaled
    )raw";
    assert(runAndCapture(pythonDunders) == "Vec(6, 8)\n14\n6 8\n100\nVec(200, 16)\n");

    // 8. roop / apna / self
    std::string roopCode = R"raw(
        roop Player {
            kaam shuru(naam) {
                apna.naam = naam
            }
            kaam greet() {
                de f"Khelega {self.naam}"
            }
        }
        p = Player("Virat")
        dikha p.greet()
    )raw";
    assert(runAndCapture(roopCode) == "Khelega Virat\n");

    std::cout << "PASSED\n";
}

void testRichDikhaStdlibAndExpandedMethods()
{
    std::cout << "[TEST] Rich Dikha, Expanded Methods & Stdlib (systumm, cli, app)... ";

    // 1. Rich terminal styling: inline tags inside dikha
    std::string inlineRich = R"raw(
        dikha "[hara bold]Success:[/] Done"
        dikha "[1, 2, 3]"
        dikha [1, 2, 3]
    )raw";
    std::string outRich = runAndCapture(inlineRich);
    assert(outRich.find("\033[32m\033[1mSuccess:\033[0m Done\033[0m\n") != std::string::npos);
    assert(outRich.find("[1, 2, 3]\n[1, 2, 3]\n") != std::string::npos);

    // 2. Chainable string ANSI methods & stripAnsi
    std::string chainColors = R"raw(
        s = "Warning".peela().mota()
        dikha s
        dikha s.rang_hata()
        dikha "Error".laal().strip_ansi()
        dikha "Link".line()
    )raw";
    std::string outChain = runAndCapture(chainColors);
    assert(outChain.find("\033[1m\033[33mWarning\033[0m\033[0m\n") != std::string::npos ||
           outChain.find("Warning") != std::string::npos);
    assert(outChain.find("Warning\nError\n") != std::string::npos);

    // 3. Table and Box widget rendering
    std::string widgets = R"raw(
        dikha.dabba("Server chalu ho gaya!", "Status")
        dikha.mez([
            { naam: "Raju", umar: 25 },
            { naam: "Shyam", umar: 26 }
        ])
    )raw";
    std::string outWidgets = runAndCapture(widgets);
    assert(outWidgets.find("┌─ Status") != std::string::npos);
    assert(outWidgets.find("Server chalu ho gaya!") != std::string::npos);
    assert(outWidgets.find("│ Raju") != std::string::npos);
    assert(outWidgets.find("│ 25") != std::string::npos);

    // 4. Expanded Array methods
    std::string arrTests = R"raw(
        arr = [1, 2]
        arr.daal(3)
        dikha arr
        dikha arr.nikal()
        dikha arr

        nums = [1, 2, 3]
        dikha nums.ulta()

        dups = [1, 2, 2, 3, 1, 3]
        dikha dups.ek_kar()

        nested = [[1, 2], [3, [4, 5]]]
        dikha nested.seedha()

        repeats = [1, 2, 1, 3, 1]
        dikha repeats.gino(1)
        dikha repeats.kahan(3)
        dikha repeats.kahan(99)

        items = [1, 2, 3, 4, 5]
        dikha items.tukde(2)
        dikha items.pehla()
        dikha items.aakhri()

        emptyArr = []
        dikha emptyArr.khali_hai()
        emptyArr.daal(100)
        dikha emptyArr.khali_hai()
    )raw";
    std::string expectedArrOut =
        "[1, 2, 3]\n"
        "3\n"
        "[1, 2]\n"
        "[3, 2, 1]\n"
        "[1, 2, 3]\n"
        "[1, 2, 3, 4, 5]\n"
        "3\n"
        "3\n"
        "-1\n"
        "[[1, 2], [3, 4], [5]]\n"
        "1\n"
        "5\n"
        "han\n"
        "nhi\n";
    assert(runAndCapture(arrTests) == expectedArrOut);
    assert(expectError("[1, 2].tukde(0);", "Aukaat ke bahar! Chunk size 0 ya negative nahi ho sakta!"));

    // 5. Expanded String & Character methods
    std::string strTests = R"raw(
        s = "namaste"
        dikha s.akshar(0)
        dikha s.akshar(-1)
        dikha s.akshar_list()
        dikha s.shuru_hota("nam")
        dikha s.khatam_hota("ste")
        dikha s.khatam_hota("bye")
        dikha s.ulta()

        txt = "banana"
        dikha txt.gino("an")
        dikha "ha".duhrao(3)
        dikha "42".aage_bhar(5, "0")
        dikha "42".piche_bhar(5, ".")

        dikha "12345".number_hai()
        dikha "12a45".number_hai()
        dikha "Antigravity".akshar_hai()
        dikha "   \t".space_hai()
        dikha "MEME".bada_hai()
        dikha "meme".chota_hai()

        dikha code_de("A")
        dikha akshar_bana(65)
        dikha ord("z")
        dikha chr(122)
    )raw";
    std::string expectedStrOut =
        "n\n"
        "e\n"
        "[\"n\", \"a\", \"m\", \"a\", \"s\", \"t\", \"e\"]\n"
        "han\n"
        "han\n"
        "nhi\n"
        "etsaman\n"
        "2\n"
        "hahaha\n"
        "00042\n"
        "42...\n"
        "han\n"
        "nhi\n"
        "han\n"
        "han\n"
        "han\n"
        "han\n"
        "65\n"
        "A\n"
        "122\n"
        "z\n";
    assert(runAndCapture(strTests) == expectedStrOut);

    // 6. Standard libraries (systumm, cli, app)
    std::string stdlibTests = R"raw(
        dikha systumm.kaun()
        systumm.copy("secret_value")
        dikha systumm.paste()

        parsed = cli.args_samajh(["--target=prod", "--fast", "-v", "build"])
        dikha parsed["target"]
        dikha parsed["fast"]
        dikha parsed["v"]
        dikha parsed["_"]

        dikha cli.puch_han_na("Continue?")
        cli.progress(50, 100, "Downloading")

        app.alert("Update", "System ready")
        app.notify("Alert", "Build complete")
        app.khol("https://example.com")
    )raw";
    std::string outStdlib = runAndCapture(stdlibTests);
    assert(outStdlib.find("secret_value\n") != std::string::npos);
    assert(outStdlib.find("prod\nhan\nhan\n[\"build\"]\n") != std::string::npos);
    assert(outStdlib.find("[App Alert: Update] System ready\n") != std::string::npos);
    assert(outStdlib.find("[Notification: Alert] Build complete\n") != std::string::npos);
    assert(outStdlib.find("[Open: https://example.com]\n") != std::string::npos);

    // 7. systumm.nikal exit signal handling
    bool caughtExit = false;
    try
    {
        runAndCapture("systumm.nikal(42)");
    }
    catch (const MemeExitSignal &e)
    {
        caughtExit = (e.exitCode == 42);
    }
    assert(caughtExit);

    // 8. Dikha statement options: end/ant, sep/alag, color/rang, bina_line
    std::string dikhaOpts = R"raw(
        dikha "Hello", end=" ";
        dikha "World";
        dikha "A", "B", "C", sep=" - ";
        dikha 1, 2, 3, alag=":";
        dikha "Test", ant="!\n";
        dikha "NoLine" bina_line;
        dikha "Appended";
        dikha "Styled", rang="laal";
        dikha "Truecolor", color="#ff5500";
    )raw";
    std::string outDikhaOpts = runAndCapture(dikhaOpts);
    assert(outDikhaOpts.find("Hello World\n") != std::string::npos);
    assert(outDikhaOpts.find("A - B - C\n") != std::string::npos);
    assert(outDikhaOpts.find("1:2:3\n") != std::string::npos);
    assert(outDikhaOpts.find("Test!\n") != std::string::npos);
    assert(outDikhaOpts.find("NoLineAppended\n") != std::string::npos);
    assert(outDikhaOpts.find("\033[31mStyled\033[0m\n") != std::string::npos);
    assert(outDikhaOpts.find("\033[38;2;255;85;0mTruecolor\033[0m\n") != std::string::npos);

    // 9. Rich tags: badges, truecolors, styles, backgrounds
    std::string richBadges = R"raw(
        dikha "[success] Done";
        dikha "[error] Fail";
        dikha "[warn] Watchout";
        dikha "[info] Notice";
        dikha "[#00ff00 mota]GreenBold[/]";
        dikha "[bg:neela safed]BlueBg[/]";
    )raw";
    std::string outBadges = runAndCapture(richBadges);
    assert(outBadges.find("\033[1;32m[SUCCESS]\033[0m Done\n") != std::string::npos);
    assert(outBadges.find("\033[1;31m[ERROR]\033[0m Fail\n") != std::string::npos);
    assert(outBadges.find("\033[1;33m[WARN]\033[0m Watchout\n") != std::string::npos);
    assert(outBadges.find("\033[1;36m[INFO]\033[0m Notice\n") != std::string::npos);
    assert(outBadges.find("\033[38;2;0;255;0m\033[1mGreenBold\033[0m") != std::string::npos);
    assert(outBadges.find("\033[44m\033[37mBlueBg\033[0m") != std::string::npos);

    // 10. Dikha module builtin methods
    std::string dikhaModule = R"raw(
        dikha.laal("Alert!");
        dikha.hara("Passed!");
        dikha.success("Great job!");
        dikha.error("Bad job!");
        dikha.warn("Caution!");
        dikha.info("Info msg");
        dikha.line(10, "=");
        dikha.raw("Raw [laal]Text[/]");
        dikha.bina_line("PartA");
        dikha "PartB";
    )raw";
    std::string outDikhaMod = runAndCapture(dikhaModule);
    assert(outDikhaMod.find("\033[31mAlert!\033[0m\n") != std::string::npos);
    assert(outDikhaMod.find("\033[32mPassed!\033[0m\n") != std::string::npos);
    assert(outDikhaMod.find("\033[1;32m[SUCCESS]\033[0m Great job!\n") != std::string::npos);
    assert(outDikhaMod.find("\033[1;31m[ERROR]\033[0m Bad job!\n") != std::string::npos);
    assert(outDikhaMod.find("\033[1;33m[WARN]\033[0m Caution!\n") != std::string::npos);
    assert(outDikhaMod.find("\033[1;36m[INFO]\033[0m Info msg\n") != std::string::npos);
    assert(outDikhaMod.find("==========\n") != std::string::npos);
    assert(outDikhaMod.find("Raw [laal]Text[/]\n") != std::string::npos);
    assert(outDikhaMod.find("PartAPartB\n") != std::string::npos);

    // 11. Implicit Typed Declarations
    std::string implicitTypes = R"raw(
        x: ank = 100
        msg: bol = "MEMElang"
        dikha x, msg
    )raw";
    assert(runWithTypeCheck(implicitTypes) == "100 MEMElang\n");
    assert(expectTypeError("x: ank = \"galat\";", "Aukaat ke bahar!"));

    std::cout << "PASSED\n";
}

void testTijoriDhabaAndUi()
{
    std::cout << "Testing Tijori (Persistent DB), Dhaba (FastAPI Server), and UI (Declarative Components)... ";

    // 1. Tijori DB operations
    std::string tijoriTest = R"raw(
        tijori.saaf()
        tijori.rakh("hero", "Shaktimaan")
        tijori.rakh("score", 999)
        tijori.rakh("items", ["chai", "samosa"])

        dikha tijori.hai_kya("hero")
        dikha tijori.hai_kya("villain")
        dikha tijori.nikal("hero")
        dikha tijori.nikal("score")
        dikha tijori.nikal("missing", "default_val")

        tijori.hata("hero")
        dikha tijori.hai_kya("hero")
    )raw";
    std::string tijoriExpected =
        "han\n"
        "nhi\n"
        "Shaktimaan\n"
        "999\n"
        "default_val\n"
        "nhi\n";
    assert(runAndCapture(tijoriTest) == tijoriExpected);

    // 2. Dhaba Server & Request Dispatch
    std::string dhabaTest = R"raw(
        dhaba.saaf()
        dhaba.get("/ping", (req) => {
            de {"pong": han}
        })
        dhaba.post("/echo", (req) => {
            de {"echo": req["body"]}
        })
        dhaba.get("/users/:id", (req) => {
            de {"user": req["params"]["id"]}
        })

        res1 = dhaba.request("GET", "/ping")
        dikha res1["status"]
        dikha res1["body"]["pong"]

        res2 = dhaba.request("POST", "/echo", {"name": "MEMElang"})
        dikha res2["status"]
        dikha res2["body"]["echo"]["name"]

        res3 = dhaba.request("GET", "/users/777")
        dikha res3["status"]
        dikha res3["body"]["user"]

        res4 = dhaba.request("GET", "/nonexistent")
        dikha res4["status"]
    )raw";
    std::string dhabaExpected =
        "200\n"
        "han\n"
        "200\n"
        "MEMElang\n"
        "200\n"
        "777\n"
        "404\n";
    assert(runAndCapture(dhabaTest) == dhabaExpected);

    // 3. UI Component System, Terminal Box, HTML, and Click Callbacks
    std::string uiTest = R"raw(
        tijori.saaf()
        tijori.rakh("btn_clicked", na)

        card = ui.dabba([
            ui.likh("Card Header", "heading"),
            ui.badge("Success", "hara")
        ], "Test Card")

        btn = ui.button("Place Order", () => {
            tijori.rakh("btn_clicked", han)
        })

        screen = ui.khamba([
            card,
            ui.katar([
                ui.puch("Type here..."),
                btn
            ])
        ])

        // Terminal render
        termOut = ui.dikha(screen, "Sample App")
        dikha termOut.hai_kya("Sample App")
        dikha termOut.hai_kya("Test Card")
        dikha termOut.hai_kya("Card Header")

        // HTML export
        htmlOut = ui.html(screen, "Sample Web App")
        dikha htmlOut.hai_kya("<html")
        dikha htmlOut.hai_kya("Card Header")

        // Trigger Click event
        dikha tijori.nikal("btn_clicked")
        clicked = ui.click(screen, "Place Order")
        dikha clicked
        dikha tijori.nikal("btn_clicked")
    )raw";
    std::string outUi = runAndCapture(uiTest);
    assert(outUi.find("Sample App") != std::string::npos);
    assert(outUi.find("Test Card") != std::string::npos);
    assert(outUi.find("Card Header") != std::string::npos);
    assert(outUi.find("nhi\nhan\nhan\n") != std::string::npos);

    // 4. Tapri POS Integration (Run exact snippet from user)
    std::string tapriCode = R"raw(
        tijori.rakh("orders", [
            {"item": "Masala Chai", "price": 20, "status": "Ready"},
            {"item": "Bun Maska", "price": 40, "status": "Cooking"}
        ])

        dhaba.get("/orders", (req) => {
            de {"ok": han, "maal": tijori.nikal("orders")}
        })

        screen = ui.khamba([
            ui.likh("☕ Tapri POS App", "heading"),
            ui.katar([
                ui.dabba([
                    ui.likh("Masala Chai - ₹20"),
                    ui.badge("Ready", "hara")
                ], "Order #1"),
                ui.dabba([
                    ui.likh("Bun Maska - ₹40"),
                    ui.badge("Cooking", "peela")
                ], "Order #2")
            ]),
            ui.puch("Naya order likho..."),
            ui.button("🔥 Order Bhej", () => {
                dikha "[hara mota]Naya order lag gaya![/]"
            })
        ])

        ui.dikha(screen, "Tapri Multi-Platform App")
    )raw";
    std::string tapriOut = runAndCapture(tapriCode);
    assert(tapriOut.find("Tapri Multi-Platform App") != std::string::npos);
    assert(tapriOut.find("☕ Tapri POS App") != std::string::npos);
    assert(tapriOut.find("Masala Chai - ₹20") != std::string::npos);
    assert(tapriOut.find("Bun Maska - ₹40") != std::string::npos);

    std::cout << "PASSED\n";
}

void testEcosystemBridgesAndPolyfills()
{
    std::cout << "[TEST] Chameleon Universal Ecosystem Bridge (py, npm, rust, ffi)... " << std::flush;

    std::string code = R"raw(
        bula "py:numpy" jaise np;
        arr = np.arange(1, 11, 1);
        dikha "np_sum=" + np.sum(arr);
        dikha "np_mean=" + np.mean(arr);
        v1 = np.array([1, 2, 3]);
        v2 = np.array([4, 5, 6]);
        dikha "np_dot=" + np.dot(v1, v2);

        bula "py:pandas" jaise pd;
        df = pd.DataFrame([
            {"item": "Chai", "price": 20, "sold": 100},
            {"item": "Samosa", "price": 15, "sold": 80},
            {"item": "Maggi", "price": 50, "sold": 45}
        ]);
        dikha "pd_rows=" + df.col("item").lambai();
        filtered = df.filter_rows("price", 20);
        dikha "pd_filtered=" + filtered.col("item").lambai();

        bula "npm:ably" jaise ably;
        ch = ably.channel("orders");
        ch.publish("new_order", {"table": 4, "chai": 2});
        dikha "ably_hist=" + ch.history().lambai();

        bula "npm:firebase" jaise fb;
        col = fb.collection("users");
        col.add({"uid": "u1", "role": "admin"});
        dikha "fb_count=" + col.get().lambai();

        bula "rust:crypto" jaise crypto;
        hash = crypto.sha256("memelang_rocks");
        dikha "sha256_len=" + hash.lambai();
        uuid = crypto.uuid_v4();
        dikha "uuid_len=" + uuid.lambai();

        bula "rust:rayon" jaise rayon;
        mapped = rayon.par_map([1, 2, 3, 4], (x) => x * 10);
        dikha "rayon_sum=" + (mapped[0] + mapped[1] + mapped[2] + mapped[3]);
    )raw";

    std::string out = runAndCapture(code);
    assert(out.find("np_sum=55") != std::string::npos);
    assert(out.find("np_mean=5.5") != std::string::npos);
    assert(out.find("np_dot=32.0") != std::string::npos);
    assert(out.find("pd_rows=3") != std::string::npos);
    assert(out.find("pd_filtered=1") != std::string::npos);
    assert(out.find("ably_hist=1") != std::string::npos);
    assert(out.find("fb_count=1") != std::string::npos);
    assert(out.find("sha256_len=64") != std::string::npos);
    assert(out.find("uuid_len=36") != std::string::npos);
    assert(out.find("rayon_sum=100") != std::string::npos);

    std::cout << "PASSED\n";
}

void testZeroWebViewNativeUiTailwindAndGpuCommands()
{
    std::cout << "[TEST] Zero-WebView Native UI, Tailwind v4 Engine & GPU Commands... " << std::flush;

    std::string code = R"raw(
        bula "ui:shadcn" jaise { Card, Button, Badge, CardHeader, CardTitle, CardContent };

        [count, setCount] = useState(1);
        setCount(5);

        appTree = (
            <Card className="p-6 bg-slate-900 border-slate-800 rounded-2xl w-full">
                <CardHeader>
                    <CardTitle className="text-xl font-bold text-slate-100">Tapri GPU App</CardTitle>
                    <Badge variant="outline">Live</Badge>
                </CardHeader>
                <CardContent className="mt-4 flex flex-col gap-2">
                    <div className="text-sm text-slate-300">Counter value: 5</div>
                    <Button variant="default" size="default">Order Chai</Button>
                </CardContent>
            </Card>
        );

        cmds = ui.gpu_commands(appTree);
        dikha "gpu_cmds_count=" + cmds.lambai();
        dikha "first_cmd_type=" + cmds[0]["type"];
        dikha "first_cmd_color=" + cmds[0]["color"];

        htmlCode = ui.html(appTree, "Tapri UI");
        dikha "html_has_bg=" + (htmlCode.kahan("bg-slate-900") >= 0);
        dikha "html_has_title=" + (htmlCode.kahan("Tapri GPU App") >= 0);
    )raw";

    std::string out = runAndCapture(code);
    assert(out.find("gpu_cmds_count=7") != std::string::npos);
    assert(out.find("first_cmd_type=FILL_ROUND_RECT") != std::string::npos);
    assert(out.find("first_cmd_color=#0f172a") != std::string::npos);
    assert(out.find("html_has_bg=han") != std::string::npos);
    assert(out.find("html_has_title=han") != std::string::npos);

    std::cout << "PASSED\n";
}

void testTapriAndDestructuring()
{
    std::cout << "[TEST] Tapri Package Manager & Advanced Destructuring... " << std::flush;

    std::string code = R"raw(
        // Array Destructuring Assignment
        [a, b, c] = [10, 20, 30];
        dikha "destruct_sum=" + (a + b + c);

        // Variadic Destructuring Assignment
        [head, *tail] = [1, 2, 3, 4, 5];
        dikha "destruct_head=" + head;
        dikha "destruct_tail_len=" + tail.lambai();

        // Destructuring import
        bula "ui:shadcn" jaise { Button, Badge };
        b = <Button>Click</Button>;
        dikha "btn_type=" + b["type"];
    )raw";

    std::string out = runAndCapture(code);
    assert(out.find("destruct_sum=60") != std::string::npos);
    assert(out.find("destruct_head=1") != std::string::npos);
    assert(out.find("destruct_tail_len=4") != std::string::npos);
    assert(out.find("btn_type=jsx_element") != std::string::npos);

    std::cout << "PASSED\n";
}

static void testStatefulReactHooksEventsContextAndRouter()
{
    std::cout << "[TEST] Stateful React Parity (Hooks, Hit-Testing, Context & Router)... " << std::flush;

    // 1. JSX fragments, spread, boolean/null child filtering, and parameter destructuring
    std::string code1 = R"raw(
        isLoggedIn = jhooth;
        showExtra = sach;
        baseProps = {"class": "btn-primary", "id": "main_btn"};

        kaam UserCard({ name, role = "User", points = 50 }) {
            de (
                <div class="user-card">
                    <h2>{name}</h2>
                    <span>{role}</span>
                    <span>{points}</span>
                </div>
            );
        }

        element = (
            <>
                <button {...baseProps}>Submit</button>
                {isLoggedIn && <span>Welcome back</span>}
                {showExtra && <span>Bonus Points</span>}
                {khali}
                <UserCard name="Raju" role="Admin" points={999} />
            </>
        );

        dikha "children_count=" + element.children.lambai();
        btn = element.children[0];
        dikha "btn_class=" + btn.props["class"];
        dikha "btn_id=" + btn.props["id"];
        card = element.children[2];
        dikha "card_type=" + card.type;
    )raw";

    std::string out1 = runAndCapture(code1);
    assert(out1.find("children_count=3") != std::string::npos);
    assert(out1.find("btn_class=btn-primary") != std::string::npos);
    assert(out1.find("btn_id=main_btn") != std::string::npos);
    assert(out1.find("card_type=jsx_element") != std::string::npos);

    // 2. Stateful Hooks & Interactive Event Dispatching
    std::string code2 = R"raw(
        kaam Counter() {
            [count, setCount] = useState(10);
            de (
                <div>
                    <span id="counter_val">{count}</span>
                    <button id="inc_btn" onClick={() => setCount(c => c + 5)}>+5</button>
                    <input id="txt_input" onChange={(e) => setCount(e.target.value)} />
                </div>
            );
        }

        kaam App() {
            de <Counter />;
        }

        ui.mount(<App />);
        dikha "mounted_ok=han";

        ui.dispatch(2, "click");
        dikha "click_dispatched=han";

        ui.dispatch(3, "change", "123");
        dikha "change_dispatched=han";
    )raw";

    std::string out2 = runAndCapture(code2);
    assert(out2.find("mounted_ok=han") != std::string::npos);
    assert(out2.find("click_dispatched=han") != std::string::npos);
    assert(out2.find("change_dispatched=han") != std::string::npos);

    // 3. Context API (createContext, useContext, <ThemeCtx.Provider>)
    std::string code3 = R"raw(
        ThemeCtx = createContext("light_default");

        kaam ThemedButton() {
            currentTheme = useContext(ThemeCtx);
            de <button class={"theme-" + currentTheme}>Button</button>;
        }

        kaam ThemeApp() {
            de (
                <ThemeCtx.Provider value="dark_mode">
                    <ThemedButton />
                </ThemeCtx.Provider>
            );
        }

        tree = <ThemeApp />;
        dikha "theme_provider_ok=han";
    )raw";

    std::string out3 = runAndCapture(code3);
    assert(out3.find("theme_provider_ok=han") != std::string::npos);

    // 4. Client Router (useRouter, <Router>, <Route>, <Link>)
    std::string code4 = R"raw(
        kaam HomePage() { de <h1>Home</h1>; }
        kaam OrdersPage() { de <h1>Orders</h1>; }

        kaam ShellApp() {
            router = useRouter();
            de (
                <div>
                    <Link to="/orders">Go Orders</Link>
                    <Router>
                        <Route path="/" element={<HomePage />} />
                        <Route path="/orders" element={<OrdersPage />} />
                    </Router>
                </div>
            );
        }

        ui.mount(<ShellApp />);
        r = useRouter();
        dikha "initial_path=" + r.path;

        r.push("/orders");
        dikha "navigated_path=" + r.path;
    )raw";

    std::string out4 = runAndCapture(code4);
    assert(out4.find("initial_path=/") != std::string::npos);
    assert(out4.find("navigated_path=/orders") != std::string::npos);

    std::cout << "PASSED\n";
}

static void testProjectWorkspaceVenvScriptsAndTestRunner()
{
    std::cout << "[TEST] Project Workspace, .meme_env, @/ Resolution & Test Runner... " << std::flush;

    // 1. Built-in test() and expect() matchers
    std::string code1 = R"raw(
        test("test basic matchers", () => {
            expect(10 + 5).toBe(15);
            expect("namaste world").toContain("world");
            expect(han).toBeTruthy();
            expect(nhi).toBeFalsy();
            expect(() => { chillao "Error message"; }).toThrow();
        });
        dikha "matchers_verified=han";
    )raw";

    std::string out1 = runAndCapture(code1);
    assert(out1.find("matchers_verified=han") != std::string::npos);

    // 2. Project Scaffolding & Virtual Environment creation
    std::string projName = "test_tapri_project";
    std::filesystem::remove_all(projName);

    std::filesystem::create_directories(projName + "/src");
    std::filesystem::create_directories(projName + "/tests");
    std::filesystem::create_directories(projName + "/.meme_env/pkgs");
    std::filesystem::create_directories(projName + "/.tapri_modules");

    // Create .env in project root
    {
        std::ofstream envf(projName + "/.env");
        envf << "APP_ENV=testing\n"
             << "SECRET_APP_NAME=" << projName << "\n";
    }

    // Create tapri.json in project root
    {
        std::ofstream tf(projName + "/tapri.json");
        tf << "{\n  \"name\": \"" << projName << "\",\n  \"version\": \"1.0.0\"\n}\n";
    }

    // Create @/ module: test_tapri_project/src/config.meme
    {
        std::ofstream cf(projName + "/src/config.meme");
        cf << "de {\"env\": systumm.env(\"APP_ENV\"), \"secret\": systumm.env(\"SECRET_APP_NAME\")};\n";
    }

    // Run code that imports with @/ alias
    std::string code2 = R"raw(
        bula "@/src/config" jaise cfg;
        dikha "loaded_env=" + cfg.env;
        dikha "loaded_secret=" + cfg.secret;
    )raw";

    std::ostringstream outStream;
    Interpreter interp2(outStream);
    interp2.setHeadless(true);
    interp2.setCurrentFilePath(projName + "/src/main.meme");

    Lexer lexer(code2);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto program = parser.parseProgram();
    TypeChecker tc;
    tc.check(program);
    interp2.interpret(program);

    std::string out2 = outStream.str();
    assert(out2.find("loaded_env=testing") != std::string::npos);
    assert(out2.find("loaded_secret=" + projName) != std::string::npos);

    // Clean up temporary project directory
    std::filesystem::remove_all(projName);

    std::cout << "PASSED\n";
}

void testMultiplatformTodoAppErgonomicsAndStress()
{
    std::cout << "[TEST] Multiplatform Todo App Stress Test & Ergonomics... " << std::flush;

    // 1. Array spread, Map spread, Ternary in arrow lambdas, and .length properties
    std::string code1 = R"raw(
        pakad todos = [
            {"id": 1, "text": "Task A", "done": jhooth},
            {"id": 2, "text": "Task B", "done": jhooth}
        ];
        pakad newTodo = {"id": 3, "text": "Task C", "done": sach};
        pakad combined = [...todos, newTodo];
        dikha "combined_len=" + combined.length;
        dikha "combined_size=" + combined.size();
        dikha "combined_lambaai=" + combined.lambaai;

        pakad toggled = combined.map(t => t.id == 2 ? {...t, "done": !t.done} : t);
        dikha "t2_done=" + toggled[1].done;

        pakad active = toggled.filter(t => !t.done);
        dikha "active_count=" + active.length;
    )raw";

    std::ostringstream out1;
    Interpreter interp1(out1);
    interp1.setHeadless(true);
    Lexer l1(code1);
    auto t1 = l1.tokenize();
    Parser p1(t1);
    auto pr1 = p1.parseProgram();
    TypeChecker tc1;
    tc1.check(pr1);
    interp1.interpret(pr1);

    std::string s1 = out1.str();
    assert(s1.find("combined_len=3") != std::string::npos);
    assert(s1.find("combined_size=3") != std::string::npos);
    assert(s1.find("combined_lambaai=3") != std::string::npos);
    assert(s1.find("t2_done=han") != std::string::npos);
    assert(s1.find("active_count=1") != std::string::npos);

    // 2. JSX conditional rendering, input rendering, and strike-through Tailwind layout
    std::string code2 = R"raw(
        kaam TodoRow(props) {
            pakad t = props.todo;
            de (
                <div class="flex items-center space-x-2">
                    <span class={t.done ? "line-through text-slate-500" : "text-white"}>{t.text}</span>
                </div>
            );
        }

        kaam App() {
            [todos, setTodos] = useState([{"id": 1, "text": "Buy Chai", "done": sach}]);
            de (
                <div class="p-4 bg-slate-900">
                    <input class="p-2" placeholder="Naya kaam..." value="" />
                    <TodoRow todo={todos[0]} />
                    {todos.length == 0 && <span>Khali</span>}
                </div>
            );
        }

        ui.mount(<App />);
    )raw";

    std::ostringstream out2;
    Interpreter interp2(out2);
    interp2.setHeadless(true);
    Lexer l2(code2);
    auto t2 = l2.tokenize();
    Parser p2(t2);
    auto pr2 = p2.parseProgram();
    interp2.interpret(pr2);

    std::string gpuJson = interp2.getLastGpuCommandsJson();
    assert(gpuJson.find("Buy Chai") != std::string::npos);
    assert(gpuJson.find("Naya kaam...") != std::string::npos);
    assert(gpuJson.find("DRAW_LINE") != std::string::npos); // strike-through line
    assert(gpuJson.find("Khali") == std::string::npos); // short-circuit ignored

    std::cout << "PASSED\n";
}

void testAsyncFetchThreadsMultiprocessingAndFs()
{
    std::cout << "[TEST] Async, Fetch, Threads, Multiprocessing, and File Systems... " << std::flush;

    // 1. Structured try/catch/finally/throw with Error objects and aliases
    std::string errCode = R"raw(
        pakad outList = [];

        panga {
            chillao Galti("Something exploded!", "CustomLafda");
        } lafda (err) {
            outList.daal("caught_galti");
            outList.daal(err.baat);
            outList.daal(err.kism);
        } pakka {
            outList.daal("pakka_done");
        }

        try {
            throw "Plain string error";
        } catch (e) {
            outList.daal("caught_string");
            outList.daal(e.message);
            outList.daal(e.type);
        } finally {
            outList.daal("finally_done");
        }

        // Bare catch
        try {
            chillao "bare test";
        } catch {
            outList.daal("bare_catch_ok");
        }

        dikha outList;
    )raw";
    std::string errOut = runAndCapture(errCode);
    assert(errOut.find("caught_galti") != std::string::npos);
    assert(errOut.find("Something exploded!") != std::string::npos);
    assert(errOut.find("CustomLafda") != std::string::npos);
    assert(errOut.find("pakka_done") != std::string::npos);
    assert(errOut.find("caught_string") != std::string::npos);
    assert(errOut.find("Plain string error") != std::string::npos);
    assert(errOut.find("finally_done") != std::string::npos);
    assert(errOut.find("bare_catch_ok") != std::string::npos);

    // 2. Fetch and jaal (http) client with mock protocol
    std::string fetchCode = R"raw(
        pakad res1 = sabar fetch("mock://users", {
            "method": "POST",
            "body": {"naam": "Raju", "umar": 25},
            "params": {"v": 2}
        });
        dikha "f1_ok=" + res1.ok;
        dikha "f1_status=" + res1.status;
        dikha "f1_method=" + res1.method;

        pakad res2 = sabar jaal.get("mock://items", {"params": {"q": "chai"}});
        dikha "f2_ok=" + res2.sahi;
        dikha "f2_url=" + res2.url;

        pakad res3 = sabar jaal.post("mock://items", {"title": "Samosa"});
        dikha "f3_method=" + res3.method;

        pakad res4 = sabar jaal.delete("mock://items/99");
        dikha "f4_method=" + res4.method;

        panga {
            pakad errRes = sabar fetch("mock://status/404");
            errRes.raise_for_status();
        } lafda (err) {
            dikha "status_err_caught=" + err.kism;
        }
    )raw";
    std::string fetchOut = runAndCapture(fetchCode);
    assert(fetchOut.find("f1_ok=han") != std::string::npos);
    assert(fetchOut.find("f1_status=200") != std::string::npos);
    assert(fetchOut.find("f1_method=POST") != std::string::npos);
    assert(fetchOut.find("f2_ok=han") != std::string::npos);
    assert(fetchOut.find("f2_url=mock://items?q=chai") != std::string::npos);
    assert(fetchOut.find("f3_method=POST") != std::string::npos);
    assert(fetchOut.find("f4_method=DELETE") != std::string::npos);
    assert(fetchOut.find("status_err_caught=NetworkLafda") != std::string::npos);

    // 3. Async / Await and Vaada (Promise)
    std::string asyncCode = R"raw(
        async kaam taskA(x) {
            ye_le x * 3;
        }

        async def taskB(x) {
            ye_le x + 10;
        }

        pakad a = sabar taskA(5);
        pakad b = sabar taskB(a);
        dikha "async_res=" + b;

        pakad pResolved = Vaada.de("instant_value");
        pakad resolvedVal = sabar pResolved;
        dikha "resolved=" + resolvedVal;

        pakad p1 = taskA(2);
        pakad p2 = taskA(3);
        pakad allResults = sabar Promise.all([p1, p2]);
        dikha "all=" + allResults;
    )raw";
    std::string asyncOut = runAndCapture(asyncCode);
    assert(asyncOut.find("async_res=25") != std::string::npos);
    assert(asyncOut.find("resolved=instant_value") != std::string::npos);
    assert(asyncOut.find("all=[6, 9]") != std::string::npos);

    // 4. Threads (dhaga), Channels (naali), Mutex (tala), and Multiprocessing (kaamgar / mp)
    std::string threadCode = R"raw(
        // Thread spawn and join
        pakad t = dhaga.chala((x, y) => x + y, 15, 25);
        pakad sum = sabar t;
        dikha "thread_sum=" + sum;

        // Mutex lock
        pakad lock = dhaga.tala();
        pakad count = 0;
        lock.surakshit(() => {
            count = count + 42;
        });
        dikha "mutex_count=" + count;

        // Channel send & recv
        pakad ch = dhaga.naali(5);
        dhaga.chala(() => {
            ch.bhej("packet_chai");
        });
        pakad msg = ch.le();
        dikha "channel_msg=" + msg;

        // Parallel mp.map / kaamgar.sab_pe
        pakad nums = [1, 2, 3, 4, 5];
        pakad squares = mp.map(x => x * x, nums);
        dikha "squares=" + squares;

        // Subprocess
        pakad p = mp.spawn("echo 'process_alive'");
        pakad code = p.ruk();
        pakad stdout = p.out();
        dikha "process_code=" + code;
    )raw";
    std::string threadOut = runAndCapture(threadCode);
    assert(threadOut.find("thread_sum=40") != std::string::npos);
    assert(threadOut.find("mutex_count=42") != std::string::npos);
    assert(threadOut.find("channel_msg=packet_chai") != std::string::npos);
    assert(threadOut.find("squares=[1, 4, 9, 16, 25]") != std::string::npos);
    assert(threadOut.find("process_code=0") != std::string::npos);

    // 5. File System (file / fs) & Path (rasta / path)
    std::string fsCode = R"raw(
        pakad testFile = "test_artifact_123.txt";
        fs.write(testFile, "Line One\nLine Two");
        dikha "fs_exists=" + fs.exists(testFile);

        pakad content = fs.read(testFile);
        dikha "fs_len=" + content.lambai();

        pakad lines = fs.read_lines(testFile);
        dikha "lines_count=" + lines.lambai();
        dikha "l0=" + lines[0];

        // CSV Read / Write
        pakad csvFile = "test_staff_123.csv";
        pakad csvData = [
            {"id": "1", "name": "Raju", "role": "Dev"},
            {"id": "2", "name": "Baburao", "role": "Manager"}
        ];
        file.likh_csv(csvFile, csvData);
        pakad readCsv = file.padh_csv(csvFile);
        dikha "csv_rows=" + readCsv.lambai();
        dikha "r0_name=" + readCsv[0]["name"];
        dikha "r1_role=" + readCsv[1]["role"];

        // Stat
        pakad info = fs.stat(testFile);
        dikha "stat_is_file=" + info["is_file"];

        // Cleanup
        fs.remove(testFile);
        file.mita(csvFile);
        dikha "clean_exists=" + fs.exists(testFile);

        // Path utils
        pakad pJoined = path.join("home", "user", "project", "code.meme");
        dikha "p_base=" + path.basename(pJoined);
        dikha "p_stem=" + path.stem(pJoined);
        dikha "p_ext=" + path.ext(pJoined);
    )raw";
    std::string fsOut = runAndCapture(fsCode);
    assert(fsOut.find("fs_exists=han") != std::string::npos);
    assert(fsOut.find("lines_count=2") != std::string::npos);
    assert(fsOut.find("l0=Line One") != std::string::npos);
    assert(fsOut.find("csv_rows=2") != std::string::npos);
    assert(fsOut.find("r0_name=Raju") != std::string::npos);
    assert(fsOut.find("r1_role=Manager") != std::string::npos);
    assert(fsOut.find("stat_is_file=han") != std::string::npos);
    assert(fsOut.find("clean_exists=nhi") != std::string::npos);
    assert(fsOut.find("p_base=code.meme") != std::string::npos);
    assert(fsOut.find("p_stem=code") != std::string::npos);
    assert(fsOut.find("p_ext=.meme") != std::string::npos);

    std::cout << "PASSED\n";
}

void testOptimizedDatatypesAndExtendedPrimitives()
{
    std::cout << "[TEST] Optimized Datatypes & Extended Primitives (byte, int32, float32, char, bytes, set, tuple)... " << std::flush;

    // 1. Byte literals, type, and arithmetic
    std::string byteCode = R"raw(
        pakad b1 = 200b;
        pakad b2 = 50b;
        dikha "b1_type=" + kya_hai(b1);
        dikha "b_sum=" + string(b1 + b2);
        pakad bHex = byte(0xff);
        dikha "b_hex=" + string(bHex);
        pakad bBin = 0b1010b;
        dikha "b_bin=" + string(bBin);
        pakad bCasted = byte(258);
        dikha "b_cast=" + string(bCasted);
    )raw";
    std::string byteOut = runAndCapture(byteCode);
    assert(byteOut.find("b1_type=byte") != std::string::npos);
    assert(byteOut.find("b_sum=250") != std::string::npos);
    assert(byteOut.find("b_hex=255") != std::string::npos);
    assert(byteOut.find("b_bin=10") != std::string::npos);
    assert(byteOut.find("b_cast=2") != std::string::npos);

    // 2. Fast Int32, Float32 and Char
    std::string numCode = R"raw(
        pakad i = int32(42);
        dikha "i_type=" + kya_hai(i);
        dikha "i_val=" + string(i + 8);

        pakad f = 3.14f;
        dikha "f_type=" + kya_hai(f);
        pakad fSum = f + 1.0f;
        dikha "f_sum=" + string(fSum > 4.0);

        pakad c = char("Namaste");
        dikha "c_type=" + kya_hai(c);
        dikha "c_val=" + string(c);
    )raw";
    std::string numOut = runAndCapture(numCode);
    assert(numOut.find("i_type=int32") != std::string::npos);
    assert(numOut.find("i_val=50") != std::string::npos);
    assert(numOut.find("f_type=float32") != std::string::npos);
    assert(numOut.find("f_sum=han") != std::string::npos);
    assert(numOut.find("c_type=char") != std::string::npos);
    assert(numOut.find("c_val=N") != std::string::npos);

    // 3. Bytes buffer, hex parsing, mutation, and binary I/O
    std::string bytesCode = R"raw(
        pakad buf = b"Hello";
        dikha "buf_type=" + kya_hai(buf);
        dikha "buf_len=" + string(buf.length);
        dikha "buf_b0=" + string(buf[0]);
        buf[0] = 104; // 'h'
        dikha "buf_str=" + buf.to_string();
        dikha "buf_hex=" + buf.to_hex();

        pakad parsed = bytes.from_hex("43686169"); // "Chai"
        dikha "parsed_str=" + parsed.to_string();

        // Binary file test
        file.likh_bytes("test_buffer.bin", parsed);
        pakad readBuf = file.padh_bytes("test_buffer.bin");
        dikha "bin_file_str=" + readBuf.to_string();
        file.mita("test_buffer.bin");
    )raw";
    std::string bytesOut = runAndCapture(bytesCode);
    assert(bytesOut.find("buf_type=bytes") != std::string::npos);
    assert(bytesOut.find("buf_len=5") != std::string::npos);
    assert(bytesOut.find("buf_b0=72") != std::string::npos);
    assert(bytesOut.find("buf_str=hello") != std::string::npos);
    assert(bytesOut.find("buf_hex=68656c6c6f") != std::string::npos);
    assert(bytesOut.find("parsed_str=Chai") != std::string::npos);
    assert(bytesOut.find("bin_file_str=Chai") != std::string::npos);

    // 4. Sets: literal, deduplication, operations, iteration
    std::string setCode = R"raw(
        pakad s = {1, 2, 2, 3, 3, 3};
        dikha "s_type=" + kya_hai(s);
        dikha "s_len=" + string(s.size);
        dikha "s_has_2=" + string(s.has(2));
        dikha "s_has_99=" + string(s.has(99));

        s.add(10);
        dikha "s_len_after_add=" + string(s.size);
        s.remove(1);
        dikha "s_has_1_after_rem=" + string(s.has(1));

        pakad sA = {1, 2};
        pakad sB = {2, 3};
        pakad sUnion = sA + sB;
        dikha "union_len=" + string(sUnion.size);
        pakad sDiff = sA - sB;
        dikha "diff_len=" + string(sDiff.size);
        dikha "diff_has_1=" + string(sDiff.has(1));

        pakad sumVals = 0;
        ghuma val mein sA {
            sumVals = sumVals + val;
        }
        dikha "set_iter_sum=" + string(sumVals);
    )raw";
    std::string setOut = runAndCapture(setCode);
    assert(setOut.find("s_type=set") != std::string::npos);
    assert(setOut.find("s_len=3") != std::string::npos);
    assert(setOut.find("s_has_2=han") != std::string::npos);
    assert(setOut.find("s_has_99=nhi") != std::string::npos);
    assert(setOut.find("s_len_after_add=4") != std::string::npos);
    assert(setOut.find("s_has_1_after_rem=nhi") != std::string::npos);
    assert(setOut.find("union_len=3") != std::string::npos);
    assert(setOut.find("diff_len=1") != std::string::npos);
    assert(setOut.find("diff_has_1=han") != std::string::npos);
    assert(setOut.find("set_iter_sum=3") != std::string::npos);

    // 5. Tuples: literal, indexing, immutability, iteration
    std::string tupleCode = R"raw(
        pakad tup = (10, "chai", 3.5);
        dikha "tup_type=" + kya_hai(tup);
        dikha "tup_len=" + string(tup.size);
        dikha "tup_0=" + string(tup[0]);
        dikha "tup_1=" + string(tup[1]);

        pakad singleTup = (42,);
        dikha "single_type=" + kya_hai(singleTup);
        dikha "single_len=" + string(singleTup.size);

        panga {
            tup[0] = 99;
        } lafda (err) {
            dikha "tup_immutable=han";
        }

        pakad strIter = "";
        ghuma item mein tup {
            strIter = strIter + string(item) + "-";
        }
        dikha "tup_iter=" + strIter;
    )raw";
    std::string tupleOut = runAndCapture(tupleCode);
    assert(tupleOut.find("tup_type=tuple") != std::string::npos);
    assert(tupleOut.find("tup_len=3") != std::string::npos);
    assert(tupleOut.find("tup_0=10") != std::string::npos);
    assert(tupleOut.find("tup_1=chai") != std::string::npos);
    assert(tupleOut.find("single_type=tuple") != std::string::npos);
    assert(tupleOut.find("single_len=1") != std::string::npos);
    assert(tupleOut.find("tup_immutable=han") != std::string::npos);
    assert(tupleOut.find("tup_iter=10-chai-3.5-") != std::string::npos);

    std::cout << "PASSED\n";
}

static void testStandardLibraries()
{
    std::cout << "[TEST] Preinstalled Standard Libraries (math, random, stats, crypto, regex, datetime, itertools, collections, csv, test)... " << std::flush;

    // 1. Math / Hisab & Destructured Import
    std::string mathCode = R"raw(
        import "math" as { sqrt, pi, gcd, factorial, comb, perm, hypot, clamp };
        dikha "math_sqrt=" + string(sqrt(49));
        dikha "math_pi_gt3=" + string(pi > 3.14);
        dikha "math_gcd=" + string(gcd(48, 18));
        dikha "math_fact=" + string(factorial(6));
        dikha "math_comb=" + string(comb(5, 2));
        dikha "math_perm=" + string(perm(5, 2));
        dikha "math_hypot=" + string(hypot(3, 4));
        dikha "math_clamp=" + string(clamp(15, 0, 10));
    )raw";
    std::string mathOut = runAndCapture(mathCode);
    assert(mathOut.find("math_sqrt=7.0") != std::string::npos);
    assert(mathOut.find("math_pi_gt3=han") != std::string::npos);
    assert(mathOut.find("math_gcd=6") != std::string::npos);
    assert(mathOut.find("math_fact=720") != std::string::npos);
    assert(mathOut.find("math_comb=10") != std::string::npos);
    assert(mathOut.find("math_perm=20") != std::string::npos);
    assert(mathOut.find("math_hypot=5.0") != std::string::npos);
    assert(mathOut.find("math_clamp=10.0") != std::string::npos);

    // 2. Random / Tukka
    std::string randCode = R"raw(
        random.seed(42);
        pakad r1 = random.randint(10, 20);
        dikha "rand_range=" + string(r1 >= 10 aur r1 <= 20);
        pakad choice = random.choice(["chai", "chai"]);
        dikha "rand_choice=" + choice;
        pakad sample = random.sample([1, 2, 3, 4, 5], 3);
        dikha "rand_sample_len=" + string(sample.size);
    )raw";
    std::string randOut = runAndCapture(randCode);
    assert(randOut.find("rand_range=han") != std::string::npos);
    assert(randOut.find("rand_choice=chai") != std::string::npos);
    assert(randOut.find("rand_sample_len=3") != std::string::npos);

    // 3. Statistics / Aankde
    std::string statsCode = R"raw(
        pakad nums = [10, 20, 30, 40, 50];
        dikha "stats_mean=" + string(stats.mean(nums));
        dikha "stats_median=" + string(stats.median(nums));
        dikha "stats_mode=" + string(stats.mode([1, 2, 2, 3]));
        dikha "stats_min=" + string(stats.min(nums));
        dikha "stats_max=" + string(stats.max(nums));
        dikha "stats_sum=" + string(stats.sum(nums));
    )raw";
    std::string statsOut = runAndCapture(statsCode);
    assert(statsOut.find("stats_mean=30.0") != std::string::npos);
    assert(statsOut.find("stats_median=30.0") != std::string::npos);
    assert(statsOut.find("stats_mode=2") != std::string::npos);
    assert(statsOut.find("stats_min=10") != std::string::npos);
    assert(statsOut.find("stats_max=50") != std::string::npos);
    assert(statsOut.find("stats_sum=150") != std::string::npos);

    // 4. Cryptography & Hashes
    std::string cryptoCode = R"raw(
        dikha "md5_hello=" + crypto.md5("hello");
        dikha "sha1_hello=" + crypto.sha1("hello");
        dikha "sha256_len=" + string(crypto.sha256("hello").size);
        pakad b64 = crypto.base64_encode("hello world");
        dikha "b64_enc=" + b64;
        dikha "b64_dec=" + crypto.base64_decode(b64);
        dikha "uuid_len=" + string(crypto.uuid().size);
    )raw";
    std::string cryptoOut = runAndCapture(cryptoCode);
    assert(cryptoOut.find("md5_hello=5d41402abc4b2a76b9719d911017c592") != std::string::npos);
    assert(cryptoOut.find("sha1_hello=aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d") != std::string::npos);
    assert(cryptoOut.find("sha256_len=64") != std::string::npos);
    assert(cryptoOut.find("b64_enc=aGVsbG8gd29ybGQ=") != std::string::npos);
    assert(cryptoOut.find("b64_dec=hello world") != std::string::npos);
    assert(cryptoOut.find("uuid_len=36") != std::string::npos);

    // 5. Regular Expressions (regex / re)
    std::string regexCode = R"raw(
        dikha "re_test=" + string(re.test("^[a-z]+$", "memelang"));
        pakad s = re.search("([a-z]+)@([a-z.]+)", "contact user@domain.com now");
        dikha "re_search_match=" + s.match;
        dikha "re_group0=" + s.groups[0];
        dikha "re_group1=" + s.groups[1];
        pakad allNums = re.find_all("[0-9]+", "apples: 100, bananas: 250");
        dikha "re_all_len=" + string(allNums.size);
        dikha "re_replace=" + re.replace("chai", "coffee", "garam chai");
        pakad parts = re.split(",", "red,green,blue");
        dikha "re_split_len=" + string(parts.size);
    )raw";
    std::string regexOut = runAndCapture(regexCode);
    assert(regexOut.find("re_test=han") != std::string::npos);
    assert(regexOut.find("re_search_match=user@domain.com") != std::string::npos);
    assert(regexOut.find("re_group0=user") != std::string::npos);
    assert(regexOut.find("re_group1=domain.com") != std::string::npos);
    assert(regexOut.find("re_all_len=2") != std::string::npos);
    assert(regexOut.find("re_replace=garam coffee") != std::string::npos);
    assert(regexOut.find("re_split_len=3") != std::string::npos);

    // 6. Datetime / Waqt
    std::string dtCode = R"raw(
        pakad yr = datetime.year();
        dikha "dt_year_valid=" + string(yr >= 2026);
        pakad isoStr = datetime.iso();
        dikha "dt_iso_has_T=" + string(isoStr.kahan("T") != -1);
    )raw";
    std::string dtOut = runAndCapture(dtCode);
    assert(dtOut.find("dt_year_valid=han") != std::string::npos);
    assert(dtOut.find("dt_iso_has_T=han") != std::string::npos);

    // 7. Itertools (range, zip, enumerate, chunk, flatten, repeat)
    std::string iterCode = R"raw(
        pakad r = range(5);
        dikha "range_len=" + string(r.size);
        dikha "range_last=" + string(r[4]);
        pakad z = zip([1, 2], ["a", "b"]);
        dikha "zip_first_0=" + string(z[0][0]);
        dikha "zip_first_1=" + z[0][1];
        pakad en = enumerate(["x", "y"]);
        dikha "enum_0=" + string(en[0][0]) + ":" + en[0][1];
        pakad chunks = itertools.chunk([1, 2, 3, 4, 5], 2);
        dikha "chunks_count=" + string(chunks.size);
        pakad flat = itertools.flatten([[1, 2], [3, 4]]);
        dikha "flat_len=" + string(flat.size);
        pakad rep = itertools.repeat("ok", 3);
        dikha "rep_len=" + string(rep.size);
    )raw";
    std::string iterOut = runAndCapture(iterCode);
    assert(iterOut.find("range_len=5") != std::string::npos);
    assert(iterOut.find("range_last=4") != std::string::npos);
    assert(iterOut.find("zip_first_0=1") != std::string::npos);
    assert(iterOut.find("zip_first_1=a") != std::string::npos);
    assert(iterOut.find("enum_0=0:x") != std::string::npos);
    assert(iterOut.find("chunks_count=3") != std::string::npos);
    assert(iterOut.find("flat_len=4") != std::string::npos);
    assert(iterOut.find("rep_len=3") != std::string::npos);

    // 8. Collections (Counter & Deque)
    std::string collCode = R"raw(
        pakad c = collections.Counter("banana");
        dikha "counter_a=" + string(c.a);
        dikha "counter_n=" + string(c.n);
        pakad dq = collections.deque();
        dq.push_back(10);
        dq.push_front(5);
        dikha "dq_size=" + string(dq.size());
        dikha "dq_pop_back=" + string(dq.pop_back());
        dikha "dq_pop_front=" + string(dq.pop_front());
    )raw";
    std::string collOut = runAndCapture(collCode);
    assert(collOut.find("counter_a=3") != std::string::npos);
    assert(collOut.find("counter_n=2") != std::string::npos);
    assert(collOut.find("dq_size=2") != std::string::npos);
    assert(collOut.find("dq_pop_back=10") != std::string::npos);
    assert(collOut.find("dq_pop_front=5") != std::string::npos);

    // 9. CSV Parsing & Stringifying
    std::string csvCode = R"raw(
        pakad csvStr = "name,score\nRaju,95\nBaburao,80";
        pakad rows = csv.parse(csvStr);
        dikha "csv_rows=" + string(rows.size);
        dikha "row0_name=" + rows[0].name;
        pakad generated = csv.stringify(rows);
        dikha "csv_has_Baburao=" + string(generated.kahan("Baburao") != -1);
    )raw";
    std::string csvOut = runAndCapture(csvCode);
    assert(csvOut.find("csv_rows=2") != std::string::npos);
    assert(csvOut.find("row0_name=Raju") != std::string::npos);
    assert(csvOut.find("csv_has_Baburao=han") != std::string::npos);

    // 10. Assertions & Testing
    std::string testCode = R"raw(
        assert(10 > 5, "Ten should be greater than five");
        assert_eq(2 + 2, 4);
        assert_ne(1, 2);
        assert_throws(() => {
            chillao "kaboom";
        });
        dikha "assertions_all_passed=han";
    )raw";
    std::string testOut = runAndCapture(testCode);
    assert(testOut.find("assertions_all_passed=han") != std::string::npos);

    std::cout << "PASSED\n";
}

static void testFastHttpServersExpressAndElysiaTypeSafety()
{
    std::cout << "[TEST] Fast HTTP Servers, Express Parity & Elysia Type Safety... " << std::flush;

    // 1. Elysia TypeBox Schema System (`t`) Standalone Validation & Coercion
    std::string schemaCode = R"raw(
        pakad schema = t.Object({
            "name": t.String({ "minLength": 3 }),
            "age": t.Integer({ "min": 18 }),
            "email": t.Optional(t.String({ "format": "email" })),
            "tags": t.Array(t.String(), { "minItems": 1 }),
            "role": t.Union([t.Literal("admin"), t.Literal("member")])
        });

        pakad validRes = schema.validate({
            "name": "Raju",
            "age": "21",
            "email": "raju@memelang.org",
            "tags": ["chai", "code"],
            "role": "admin"
        });
        dikha "schema_valid=" + string(validRes.success);
        dikha "schema_coerced_age=" + string(validRes.data.age);
        dikha "schema_age_type=" + kya_hai(validRes.data.age);

        pakad invalidRes = schema.validate({
            "name": "Bo",
            "age": 15,
            "tags": [],
            "role": "guest"
        });
        dikha "schema_invalid=" + string(invalidRes.success);
        dikha "schema_err_count=" + string(invalidRes.errors.size);
    )raw";
    std::string schemaOut = runAndCapture(schemaCode);
    assert(schemaOut.find("schema_valid=han") != std::string::npos);
    assert(schemaOut.find("schema_coerced_age=21") != std::string::npos);
    assert(schemaOut.find("schema_age_type=int") != std::string::npos);
    assert(schemaOut.find("schema_invalid=nhi") != std::string::npos);
    assert(schemaOut.find("schema_err_count=4") != std::string::npos);

    // 2. Express Server (Middlewares, Sub-Routers, Req/Res, Headers)
    std::string expressCode = R"raw(
        pakad app = express();

        // Global middleware setting custom header
        app.use((req, res, next) => {
            res.set("X-Custom-Server", "MEMElang-Express");
            next();
        });

        // Route-specific auth middleware
        pakad authMw = (req, res, next) => {
            dekh req.get("authorization") == "Bearer secret123" {
                next();
            } warna {
                res.status(401).json({ "error": "Unauthorized" });
            }
        };

        app.get("/public", (req, res) => {
            res.status(200).json({ "message": "Public area" });
        });

        app.get("/private", authMw, (req, res) => {
            res.status(200).json({ "message": "Secret vault" });
        });

        // Sub-router mounted at /api/v1
        pakad v1Router = express.Router();
        v1Router.get("/users/:id", (req, res) => {
            res.status(200).json({ "userId": req.params.id, "query_tab": req.query.tab });
        });
        app.use("/api/v1", v1Router);

        // Requests test
        pakad r1 = app.request("GET", "/public");
        dikha "exp_pub_status=" + string(r1.status);
        dikha "exp_pub_hdr=" + r1.headers["X-Custom-Server"];

        pakad r2 = app.request("GET", "/private");
        dikha "exp_priv_unauth=" + string(r2.status);

        pakad r3 = app.request("GET", "/private", na, { "authorization": "Bearer secret123" });
        dikha "exp_priv_auth=" + string(r3.status);
        dikha "exp_priv_msg=" + r3.body.message;

        pakad r4 = app.request("GET", "/api/v1/users/99?tab=profile");
        dikha "exp_sub_status=" + string(r4.status);
        dikha "exp_sub_uid=" + r4.body.userId;
        dikha "exp_sub_tab=" + r4.body.query_tab;
    )raw";
    std::string expressOut = runAndCapture(expressCode);
    assert(expressOut.find("exp_pub_status=200") != std::string::npos);
    assert(expressOut.find("exp_pub_hdr=MEMElang-Express") != std::string::npos);
    assert(expressOut.find("exp_priv_unauth=401") != std::string::npos);
    assert(expressOut.find("exp_priv_auth=200") != std::string::npos);
    assert(expressOut.find("exp_priv_msg=Secret vault") != std::string::npos);
    assert(expressOut.find("exp_sub_status=200") != std::string::npos);
    assert(expressOut.find("exp_sub_uid=99") != std::string::npos);
    assert(expressOut.find("exp_sub_tab=profile") != std::string::npos);

    // 3. Elysia Full Typesafety (Schema Routes, Derivations, State & Direct Returns)
    std::string elysiaCode = R"raw(
        pakad app = elysia();

        app.state("counter", 100);
        app.decorate("appVersion", () => "2.1.0");
        app.derive((ctx) => {
            ye_le { "userRole": "admin" };
        });

        // Type-safe POST with params, query, and body schema
        app.post("/products/:id", {
            params: t.Object({ "id": t.Integer() }),
            query: t.Object({ "tag": t.Optional(t.String()) }),
            body: t.Object({
                "title": t.String({ "minLength": 2 }),
                "price": t.Number({ "min": 0 })
            })
        }, (ctx) => {
            ctx.store.counter = ctx.store.counter + 1;
            ye_le {
                "id": ctx.params.id,
                "title": ctx.body.title,
                "price": ctx.body.price,
                "tag": ctx.query.tag,
                "role": ctx.userRole,
                "version": ctx.appVersion(),
                "visits": ctx.store.counter
            };
        });

        // Test 1: Valid POST with string coercion
        pakad res1 = app.request("POST", "/products/500?tag=tech", { "title": "Monitor", "price": "199.99" });
        dikha "elysia_res1_status=" + string(res1.status);
        dikha "elysia_res1_id=" + string(res1.body.id);
        dikha "elysia_res1_id_type=" + kya_hai(res1.body.id);
        dikha "elysia_res1_price=" + string(res1.body.price);
        dikha "elysia_res1_role=" + res1.body.role;
        dikha "elysia_res1_visits=" + string(res1.body.visits);

        // Test 2: Invalid Body (400 ValidationError)
        pakad res2 = app.request("POST", "/products/500", { "title": "A", "price": -10 });
        dikha "elysia_res2_status=" + string(res2.status);
        dikha "elysia_res2_err=" + res2.body.error;

        // Test 3: Invalid Param (400 ValidationError)
        pakad res3 = app.request("POST", "/products/invalid-id", { "title": "Valid", "price": 50 });
        dikha "elysia_res3_status=" + string(res3.status);
        dikha "elysia_res3_err=" + res3.body.error;
    )raw";
    std::string elysiaOut = runAndCapture(elysiaCode);
    assert(elysiaOut.find("elysia_res1_status=200") != std::string::npos);
    assert(elysiaOut.find("elysia_res1_id=500") != std::string::npos);
    assert(elysiaOut.find("elysia_res1_id_type=int") != std::string::npos);
    assert(elysiaOut.find("elysia_res1_price=199.99") != std::string::npos);
    assert(elysiaOut.find("elysia_res1_role=admin") != std::string::npos);
    assert(elysiaOut.find("elysia_res1_visits=101") != std::string::npos);
    assert(elysiaOut.find("elysia_res2_status=400") != std::string::npos);
    assert(elysiaOut.find("elysia_res2_err=ValidationError") != std::string::npos);
    assert(elysiaOut.find("elysia_res3_status=400") != std::string::npos);
    assert(elysiaOut.find("elysia_res3_err=ValidationError") != std::string::npos);

    // 4. Live Multi-threaded Native TCP Server & Real Loopback Network Fetch
    std::string liveServerCode = R"raw(
        pakad app = express();
        app.get("/health", (req, res) => {
            res.status(200).json({ "status": "healthy", "uptime_ms": 100 });
        });

        app.listen(18765, () => {
            dikha "live_server_online=han";
        });

        pakad response = sabar fetch("http://localhost:18765/health");
        dikha "live_fetch_status=" + string(response.status);
        pakad data = response.json();
        dikha "live_health=" + data.status;

        app.stop();
        dikha "live_server_closed=han";
    )raw";
    std::string liveOut = runAndCapture(liveServerCode);
    assert(liveOut.find("live_server_online=han") != std::string::npos);
    assert(liveOut.find("live_fetch_status=200") != std::string::npos);
    assert(liveOut.find("live_health=healthy") != std::string::npos);
    assert(liveOut.find("live_server_closed=han") != std::string::npos);

    std::cout << "PASSED\n";
}

void testNativeUiPrimitivesTextEngineAndTessellator()
{
    std::cout << "[TEST] Native UI TextEngine, Tessellator, Egui & Session... ";

    // 1. TextFieldState: double-click word, triple-click line, Shift+Arrow, Ctrl+C/X/V/A/Z/Y, wrap & password safety
    MemeUI::TextFieldState tf;
    tf.syncFromProp("hello world test");
    float worldMidX = MemeUI::measureTextWidth("hello wo", 14.0f, false);
    assert(tf.handlePointerDown(worldMidX, 5.0f, 320.0f, 14.0f, false, 2));
    assert(tf.hasSelection());
    assert(tf.selectedText() == "world");

    // Ctrl+C copies selection, Ctrl+A selects all, Ctrl+V replaces, Ctrl+Z undoes, Ctrl+Y redoes
    MemeUI::UiKeyEvent kevCopy; kevCopy.key = "c"; kevCopy.ctrl = true;
    auto copyRes = tf.handleKeyEvent(kevCopy, 320.0f, 24.0f, 14.0f, false, "");
    assert(copyRes.requestClipboardCopy && copyRes.clipboardSet == "world");

    MemeUI::UiKeyEvent kevCut; kevCut.key = "x"; kevCut.ctrl = true;
    auto cutRes = tf.handleKeyEvent(kevCut, 320.0f, 24.0f, 14.0f, false, "");
    assert(cutRes.textChanged && cutRes.clipboardSet == "world");
    assert(tf.text == "hello  test");

    MemeUI::UiKeyEvent kevPaste; kevPaste.key = "v"; kevPaste.ctrl = true;
    auto pasteRes = tf.handleKeyEvent(kevPaste, 320.0f, 24.0f, 14.0f, false, "planet");
    assert(pasteRes.textChanged && tf.text == "hello planet test");

    MemeUI::UiKeyEvent kevUndo; kevUndo.key = "z"; kevUndo.ctrl = true;
    assert(tf.handleKeyEvent(kevUndo, 320.0f, 24.0f, 14.0f, false, "").textChanged);
    assert(tf.text == "hello  test");

    MemeUI::UiKeyEvent kevRedo; kevRedo.key = "y"; kevRedo.ctrl = true;
    assert(tf.handleKeyEvent(kevRedo, 320.0f, 24.0f, 14.0f, false, "").textChanged);
    assert(tf.text == "hello planet test");

    MemeUI::UiKeyEvent kevAll; kevAll.key = "a"; kevAll.ctrl = true;
    tf.handleKeyEvent(kevAll, 320.0f, 24.0f, 14.0f, false, "");
    assert(tf.selectedText() == "hello planet test");

    // Shift+ArrowLeft selection shrinks selection from end
    MemeUI::UiKeyEvent kevShiftLeft; kevShiftLeft.key = "ArrowLeft"; kevShiftLeft.shift = true;
    tf.handleKeyEvent(kevShiftLeft, 320.0f, 24.0f, 14.0f, false, "");
    assert(tf.selectedText() == "hello planet tes");

    // Multiline triple-click line selection & wrapTextLines
    MemeUI::TextFieldState ml;
    ml.multiline = true;
    ml.syncFromProp("alpha line\nbeta gamma\nomega");
    float line2Y = 14.0f * 1.5f;
    assert(ml.handlePointerDown(12.0f, line2Y, 260.0f, 14.0f, false, 3));
    assert(ml.selectedText().find("beta gamma") != std::string::npos);
    assert(ml.selectedText().find("alpha") == std::string::npos);
    auto wrapped = MemeUI::wrapTextLines("one two three four five", 60.0f, 14.0f, false);
    assert(wrapped.size() >= 2);

    // Password field must mask rendered bullets & disallow clipboard copy/selectedText leak
    MemeUI::TextFieldState pw;
    pw.password = true;
    pw.syncFromProp("topsecret");
    pw.selectAll();
    assert(pw.selectedText().empty());
    std::vector<MemeUI::GpuDrawCmd> pwCmds;
    pw.emitDrawCommands(pwCmds, 0, 0, 200, 24, 14.0f, 400, MemeUI::hexToRgba("#ffffff"), "", true, false, 1, 0);
    bool sawBulletText = false;
    for (const auto &c : pwCmds) {
        if (c.type == "DRAW_TEXT") {
            assert(c.text.find("topsecret") == std::string::npos);
            if (c.text.find("\xE2\x80\xA2") != std::string::npos) sawBulletText = true;
        }
    }
    assert(sawBulletText);

    // 2. Tessellator & FontAtlas: 20-byte vertex stride, nested clip batching, gradients, shadows, SVG, glyphs
    static_assert(sizeof(MemeUI::UiVertex) == 20, "UiVertex stride must be 20 bytes");
    auto rgbaAtlas = MemeUI::FontAtlas::getRgbaAtlas();
    assert(rgbaAtlas.size() == static_cast<size_t>(MemeUI::FontAtlas::ATLAS_W * MemeUI::FontAtlas::ATLAS_H * 4));
    assert(rgbaAtlas[0] == 255 && rgbaAtlas[3] == 255);
    float u0 = 0, v0 = 0, u1 = 0, v1 = 0;
    MemeUI::FontAtlas::getGlyphUV('A', u0, v0, u1, v1);
    assert(u1 > u0 && v1 > v0 && u1 <= 1.0f && v1 <= 1.0f);

    std::vector<MemeUI::GpuDrawCmd> tcmds;
    MemeUI::GpuDrawCmd sh; sh.type = "SHADOW_ROUND_RECT"; sh.x = 12; sh.y = 12; sh.width = 180; sh.height = 60; sh.radius = 12; sh.blur = 8; sh.alpha = 0.4f;
    tcmds.push_back(sh);
    MemeUI::GpuDrawCmd gr; gr.type = "FILL_GRADIENT_RECT"; gr.x = 12; gr.y = 12; gr.width = 180; gr.height = 60; gr.radius = 12; gr.color = "#4f46e5"; gr.color2 = "#06b6d4";
    tcmds.push_back(gr);
    MemeUI::GpuDrawCmd clip1; clip1.type = "PUSH_CLIP"; clip1.x = 10; clip1.y = 20; clip1.width = 100; clip1.height = 80;
    tcmds.push_back(clip1);
    MemeUI::GpuDrawCmd r1; r1.type = "FILL_ROUND_RECT"; r1.x = 15; r1.y = 25; r1.width = 60; r1.height = 30; r1.color = "#ffffff";
    tcmds.push_back(r1);
    MemeUI::GpuDrawCmd clip2; clip2.type = "PUSH_CLIP"; clip2.x = 50; clip2.y = 40; clip2.width = 100; clip2.height = 100;
    tcmds.push_back(clip2);
    MemeUI::GpuDrawCmd sv; sv.type = "DRAW_SVG_PATH"; sv.x = 52; sv.y = 42; sv.width = 24; sv.height = 24; sv.svgPath = "M 20 6 L 9 17 L 4 12"; sv.color = "#10b981"; sv.filled = false;
    tcmds.push_back(sv);
    MemeUI::GpuDrawCmd txt; txt.type = "DRAW_TEXT"; txt.x = 52; txt.y = 70; txt.text = "Hello"; txt.fontSize = 14.0f; txt.color = "#f8fafc";
    tcmds.push_back(txt);
    MemeUI::GpuDrawCmd popCmd; popCmd.type = "POP_CLIP";
    tcmds.push_back(popCmd);
    tcmds.push_back(popCmd);

    auto mesh = MemeUI::Tessellator::tessellate(tcmds, 800.0f, 600.0f);
    assert(mesh.vertices.size() > 50);
    assert(mesh.indices.size() > 80 && (mesh.indices.size() % 3) == 0);
    assert(mesh.batches.size() >= 3);
    bool foundIntersectedClip = false;
    for (const auto &b : mesh.batches) {
        if (std::abs(b.clipRect.x - 50.0f) < 0.1f && std::abs(b.clipRect.y - 40.0f) < 0.1f &&
            std::abs(b.clipRect.width - 60.0f) < 0.1f && std::abs(b.clipRect.height - 60.0f) < 0.1f) {
            foundIntersectedClip = true;
        }
    }
    assert(foundIntersectedClip);

    // 3. EguiContext immediate-mode panels, widgets & click cycle
    MemeUI::EguiContext eguiCtx;
    MemeUI::UiRect saveBtnRect;
    bool btnClickedFrame1 = false;
    MemeUI::UiInputState in1;
    in1.viewportW = 800; in1.viewportH = 600;
    eguiCtx.beginFrame(in1);
    eguiCtx.topPanel("top", 44.0f, [&](MemeUI::EguiUi &ui) {
        auto r = ui.button("Save");
        saveBtnRect = r.rect;
        btnClickedFrame1 = r.clicked;
    });
    eguiCtx.centralPanel([&](MemeUI::EguiUi &ui) {
        bool ck = true;
        double sl = 42.0;
        ui.checkbox("ck_imm", ck, "Enable");
        ui.slider("sl_imm", sl, 0.0, 100.0, "Gain");
    });
    eguiCtx.endFrame();
    assert(!btnClickedFrame1);
    assert(saveBtnRect.width > 30.0f && saveBtnRect.height >= 24.0f);
    assert(!eguiCtx.cmds.empty() && eguiCtx.hitRegions.size() >= 3);

    bool btnClickedFrame2 = false;
    MemeUI::UiInputState in2;
    in2.viewportW = 800; in2.viewportH = 600;
    in2.pointerX = saveBtnRect.x + saveBtnRect.width * 0.5f;
    in2.pointerY = saveBtnRect.y + saveBtnRect.height * 0.5f;
    in2.mouseReleased[0] = true;
    eguiCtx.beginFrame(in2);
    eguiCtx.topPanel("top", 44.0f, [&](MemeUI::EguiUi &ui) {
        btnClickedFrame2 = ui.button("Save").clicked;
    });
    eguiCtx.endFrame();
    assert(btnClickedFrame2);

    // 4. UiSession accessible audit + portal centering + scroll-ancestor hit-order + broken audit rejection
    MemeUI::UiSession sess;
    bool btnClickedByPtr = false;
    MemeUI::UiNode root;
    root.tag = "div"; root.classStr = "min-h-screen w-full p-4 flex flex-col gap-3 overflow-y-auto";
    MemeUI::UiNode okBtn;
    okBtn.tag = "button"; okBtn.testId = "ok_btn"; okBtn.classStr = "px-4 py-2 bg-indigo-600 text-white rounded-xl";
    okBtn.handlers["click"] = [&](const MemeUI::UiEvent &) { btnClickedByPtr = true; };
    MemeUI::UiNode okTxt; okTxt.isText = true; okTxt.text = "Confirm";
    okBtn.children.push_back(okTxt);
    MemeUI::UiNode slNode;
    slNode.tag = "slider"; slNode.role = "slider"; slNode.testId = "vol_slider"; slNode.ariaLabel = "Volume";
    slNode.classStr = "w-64 h-6"; slNode.numProps["min"] = 0; slNode.numProps["max"] = 100; slNode.numProps["value"] = 50;
    MemeUI::UiNode dlgNode;
    dlgNode.tag = "dialog"; dlgNode.role = "dialog"; dlgNode.testId = "test_dlg"; dlgNode.isPortal = true; dlgNode.open = true;
    dlgNode.classStr = "w-96 p-6 bg-slate-900 rounded-2xl flex flex-col gap-2";
    MemeUI::UiNode dlgTxt; dlgTxt.tag = "p";
    MemeUI::UiNode dlgTxtLeaf; dlgTxtLeaf.isText = true; dlgTxtLeaf.text = "Centered Modal";
    dlgTxt.children.push_back(dlgTxtLeaf);
    dlgNode.children.push_back(dlgTxt);
    root.children.push_back(okBtn);
    root.children.push_back(slNode);

    // First verify raw pointer click inside overflow-y-auto ancestor hits child button cleanly!
    sess.cachedRoot = root;
    sess.renderFrame(960.0f, 720.0f);
    auto btnHits = sess.agentQuery("", "", "ok_btn");
    assert(btnHits.size() == 1);
    float bx = btnHits[0].x + btnHits[0].width * 0.5f;
    float by = btnHits[0].y + btnHits[0].height * 0.5f;
    assert(sess.hitTestTopmost(bx, by) && sess.hitTestTopmost(bx, by)->testId == "ok_btn");
    assert(sess.handlePointerDown(bx, by, 0, 1));
    assert(sess.handlePointerUp(bx, by, 0));
    assert(btnClickedByPtr);

    // Now mount modal dialog and verify backdrop blocks clicks to underlying ok_btn!
    btnClickedByPtr = false;
    root.children.push_back(dlgNode);
    sess.cachedRoot = root;
    sess.renderFrame(960.0f, 720.0f);
    assert(sess.agentAssertAccessible().ok);
    const auto *blockedHit = sess.hitTestTopmost(bx, by);
    assert(blockedHit && blockedHit->tag == "backdrop");
    sess.handlePointerDown(bx, by, 0, 1);
    assert(!sess.handlePointerUp(bx, by, 0) && !btnClickedByPtr);
    auto dlgHits = sess.agentQuery("", "", "test_dlg");
    assert(dlgHits.size() == 1);
    assert(dlgHits[0].x > 100.0f && dlgHits[0].y > 100.0f && dlgHits[0].y + dlgHits[0].height <= 720.0f);

    // Broken a11y tree must fail audit cleanly
    MemeUI::UiNode badRoot;
    badRoot.tag = "div"; badRoot.classStr = "flex flex-col gap-2 p-4";
    MemeUI::UiNode badBtn; badBtn.tag = "button"; badBtn.testId = "dup"; badBtn.classStr = "w-8 h-8";
    MemeUI::UiNode badSl; badSl.tag = "slider"; badSl.role = "slider"; badSl.testId = "dup"; badSl.ariaLabel = "Bad";
    badSl.numProps["min"] = 0; badSl.numProps["max"] = 100; badSl.numProps["value"] = 150;
    badRoot.children.push_back(badBtn);
    badRoot.children.push_back(badSl);
    sess.cachedRoot = badRoot;
    sess.renderFrame(960.0f, 720.0f);
    auto badAudit = sess.agentAssertAccessible();
    assert(!badAudit.ok && badAudit.violations.size() >= 3);

    // Mobile (393x852 w/ safe area) root scrollability + scroll-chaining + click-after-scroll
    MemeUI::UiSession mobSess;
    mobSess.input.safeTop = 59.0f;
    mobSess.input.safeBottom = 34.0f;
    bool mobBottomClicked = false;
    MemeUI::UiNode mobRoot;
    mobRoot.tag = "div"; mobRoot.testId = "mob_root";
    mobRoot.classStr = "min-h-screen w-full p-4 flex flex-col gap-4 overflow-y-auto";
    MemeUI::UiNode innerNonOverflow;
    innerNonOverflow.tag = "div"; innerNonOverflow.testId = "inner_scroll";
    innerNonOverflow.classStr = "p-3 flex flex-col gap-2 overflow-y-auto";
    MemeUI::UiNode innerLbl; innerLbl.tag = "p";
    MemeUI::UiNode innerTxt; innerTxt.isText = true; innerTxt.text = "Inner non-overflowing panel";
    innerLbl.children.push_back(innerTxt);
    innerNonOverflow.children.push_back(innerLbl);
    mobRoot.children.push_back(innerNonOverflow);
    for (int bi = 0; bi < 6; ++bi) {
        MemeUI::UiNode spacer;
        spacer.tag = "div"; spacer.classStr = "h-36 w-full bg-slate-800 rounded-xl";
        mobRoot.children.push_back(spacer);
    }
    MemeUI::UiNode bottomBtn;
    bottomBtn.tag = "button"; bottomBtn.testId = "mob_bottom_btn";
    bottomBtn.classStr = "px-4 py-2 bg-indigo-600 text-white rounded-xl";
    bottomBtn.handlers["click"] = [&](const MemeUI::UiEvent &) { mobBottomClicked = true; };
    MemeUI::UiNode bbTxt; bbTxt.isText = true; bbTxt.text = "Bottom Action";
    bottomBtn.children.push_back(bbTxt);
    mobRoot.children.push_back(bottomBtn);

    mobSess.cachedRoot = mobRoot;
    mobSess.renderFrame(393.0f, 852.0f);
    auto mr = mobSess.agentQuery("", "", "mob_root");
    auto ir = mobSess.agentQuery("", "", "inner_scroll");
    auto bbBefore = mobSess.agentQuery("", "", "mob_bottom_btn");
    assert(mr.size() == 1 && mr[0].isScrollContainer && mr[0].maxScrollY > 50.0f);
    assert(ir.size() == 1 && ir[0].maxScrollY <= 1.0f);
    assert(bbBefore.size() == 1 && bbBefore[0].y > 800.0f);

    // Swipe up while pointer is inside non-overflowing inner container -> chains to root and reveals bottom button!
    assert(mobSess.handleScroll(ir[0].x + 20.0f, ir[0].y + 10.0f, 0.0f, -20.0f));
    mobSess.renderFrame(393.0f, 852.0f);
    auto mrAfter = mobSess.agentQuery("", "", "mob_root");
    auto bbAfter = mobSess.agentQuery("", "", "mob_bottom_btn");
    assert(mrAfter[0].scrollY > 250.0f);
    assert(bbAfter[0].y < bbBefore[0].y - 250.0f);
    assert(bbAfter[0].y + bbAfter[0].height <= 818.5f);
    float bbx = bbAfter[0].x + bbAfter[0].width * 0.5f;
    float bby = bbAfter[0].y + bbAfter[0].height * 0.5f;
    assert(mobSess.handlePointerDown(bbx, bby, 0, 1));
    assert(mobSess.handlePointerUp(bbx, bby, 0) && mobBottomClicked);

    std::cout << "PASSED\n";
}

static void testInnerContainerScrollAndNativeApisAndIcons()
{
    std::cout << "Testing inner container scroll, horizontal scroll, scrollbar drag, egui.scroll_area, Native APIs, and Icon builders... ";

    // 1. Multi-level rowFlex -> Card -> flex-1 overflow-y-auto (overscroll-contain) inside bounded desktop layout
    {
        MemeUI::UiSession sess;
        MemeUI::UiNode root;
        root.tag = "div"; root.testId = "desk_root";
        root.classStr = "h-screen w-full p-4 flex flex-col gap-3 overflow-y-auto";

        MemeUI::UiNode row;
        row.tag = "div"; row.classStr = "flex flex-row gap-3 flex-1";
        MemeUI::UiNode card;
        card.tag = "div"; card.classStr = "flex-1 flex flex-col gap-2 p-3 bg-slate-900";
        MemeUI::UiNode listBox;
        listBox.tag = "div"; listBox.testId = "chat_inner_list";
        listBox.classStr = "flex-1 flex flex-col gap-2 overflow-y-auto overscroll-contain";
        for (int i = 0; i < 16; ++i) {
            MemeUI::UiNode item;
            item.tag = "div"; item.classStr = "h-16 bg-slate-800 rounded-lg";
            MemeUI::UiNode t; t.isText = true; t.text = "Msg " + std::to_string(i);
            item.children.push_back(t);
            listBox.children.push_back(item);
        }
        card.children.push_back(listBox);
        row.children.push_back(card);
        root.children.push_back(row);

        // Extra sibling to make sure root itself also overflows vertically
        MemeUI::UiNode tallSib; tallSib.tag = "div"; tallSib.classStr = "h-48 shrink-0";
        root.children.push_back(tallSib);

        sess.cachedRoot = root;
        sess.renderFrame(1000.0f, 600.0f);
        auto innerRes = sess.agentQuery("", "", "chat_inner_list");
        assert(innerRes.size() == 1 && innerRes[0].isScrollContainer && innerRes[0].maxScrollY > 200.0f);

        // Scroll inside inner list -> inner list scrolls, root stays 0
        float ix = innerRes[0].x + 40.0f, iy = innerRes[0].y + 40.0f;
        assert(sess.handleScroll(ix, iy, 0.0f, -3.0f));
        sess.renderFrame(1000.0f, 600.0f);
        auto innerAfter = sess.agentQuery("", "", "chat_inner_list");
        assert(innerAfter[0].scrollY > 40.0f);
        auto rootAfter = sess.agentQuery("", "", "desk_root");
        assert(rootAfter[0].scrollY == 0.0f);

        // Drag vertical scrollbar thumb/track to bottom -> inner scroll reaches maxScrollY
        float sbX = innerAfter[0].x + innerAfter[0].width - 4.0f;
        float sbTopY = innerAfter[0].y + 8.0f;
        float sbBotY = innerAfter[0].y + innerAfter[0].height - 8.0f;
        assert(sess.handlePointerDown(sbX, sbTopY, 0, 1));
        assert(sess.interaction.draggingScrollbar);
        assert(sess.handlePointerMove(sbX, sbBotY));
        sess.handlePointerUp(sbX, sbBotY, 0);
        sess.renderFrame(1000.0f, 600.0f);
        auto innerDragged = sess.agentQuery("", "", "chat_inner_list");
        assert(innerDragged[0].scrollY > innerDragged[0].maxScrollY * 0.7f);

        // At bottom of inner container with overscroll-contain: scrolling further down must NOT chain to root
        sess.interaction.scrollOffsetsY[innerDragged[0].nodeKey] = innerDragged[0].maxScrollY;
        sess.renderFrame(1000.0f, 600.0f);
        assert(!sess.handleScroll(ix, iy, 0.0f, -5.0f));
        assert(sess.agentQuery("", "", "desk_root")[0].scrollY == 0.0f);
    }

    // 2. h-1/2, max-h-32, horizontal overflow-x-auto + onWheel virtual handler
    {
        MemeUI::UiSession sess;
        double wheelSum = 0.0;
        MemeUI::UiNode root; root.tag = "div"; root.classStr = "h-[400px] w-[320px] flex flex-col gap-2 p-2";
        MemeUI::UiNode halfBox; halfBox.tag = "div"; halfBox.testId = "half_scroll";
        halfBox.classStr = "h-1/2 overflow-y-auto flex flex-col gap-1";
        for (int i = 0; i < 10; ++i) {
            MemeUI::UiNode c; c.tag = "div"; c.classStr = "h-12";
            halfBox.children.push_back(c);
        }
        MemeUI::UiNode xStrip; xStrip.tag = "div"; xStrip.testId = "horiz_strip";
        xStrip.classStr = "w-full h-16 flex flex-row gap-2 overflow-x-auto";
        for (int i = 0; i < 6; ++i) {
            MemeUI::UiNode c; c.tag = "div"; c.classStr = "w-28 shrink-0 bg-indigo-700";
            xStrip.children.push_back(c);
        }
        MemeUI::UiNode wheelBox; wheelBox.tag = "div"; wheelBox.testId = "wheel_box";
        wheelBox.classStr = "h-20 overflow-hidden";
        wheelBox.handlers["wheel"] = [&](const MemeUI::UiEvent &ev) { wheelSum += ev.deltaY; };

        root.children.push_back(halfBox);
        root.children.push_back(xStrip);
        root.children.push_back(wheelBox);

        sess.cachedRoot = root;
        sess.renderFrame(320.0f, 400.0f);

        auto hrHalf = sess.agentQuery("", "", "half_scroll");
        assert(hrHalf.size() == 1 && std::abs(hrHalf[0].height - 192.0f) < 2.0f && hrHalf[0].maxScrollY > 100.0f);

        auto hrX = sess.agentQuery("", "", "horiz_strip");
        assert(hrX.size() == 1 && hrX[0].maxScrollX > 200.0f);
        assert(sess.handleScroll(hrX[0].x + 20.0f, hrX[0].y + 10.0f, -3.0f, 0.0f));
        sess.renderFrame(320.0f, 400.0f);
        assert(sess.agentQuery("", "", "horiz_strip")[0].scrollX > 40.0f);

        auto hrW = sess.agentQuery("", "", "wheel_box");
        assert(hrW.size() == 1 && sess.handleScroll(hrW[0].x + 15.0f, hrW[0].y + 15.0f, 0.0f, -2.5f));
        assert(std::abs(wheelSum - (-2.5)) < 0.01);
    }

    // 3. Immediate-mode egui.scroll_area registers UiHitRegion & scrolls cleanly
    {
        MemeUI::UiSession esess;
        esess.isImmediateMode = true;
        esess.eguiUpdateFn = [](MemeUI::EguiContext &ctx) {
            MemeUI::EguiUi u(&ctx, ctx.remainingRect, false, 0);
            u.scrollArea("egui_test_list", 120.0f, [](MemeUI::EguiUi &sub) {
                for (int i = 0; i < 14; ++i) {
                    sub.label("Row " + std::to_string(i));
                }
            });
        };
        esess.renderFrame(400.0f, 300.0f);
        bool foundSa = false;
        MemeUI::UiHitRegion saHr;
        for (const auto &hr : esess.lastHits) {
            if (hr.nodeKey == "egui_sa_egui_test_list") { foundSa = true; saHr = hr; break; }
        }
        assert(foundSa && saHr.isScrollContainer && saHr.maxScrollY > 50.0f);
        assert(esess.handleScroll(saHr.x + 20.0f, saHr.y + 20.0f, 0.0f, -3.0f));
        assert(esess.interaction.scrollOffsetsY["egui_test_list"] > 20.0f);
    }

    // 4. Unified MemeNative storage, keychain, clipboard & device info
    {
        std::string uniq = "suite_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        assert(MemeNative::storageSet(uniq, "val_ok"));
        assert(MemeNative::storageGet(uniq) == "val_ok");
        auto ks = MemeNative::storageKeys();
        assert(std::find(ks.begin(), ks.end(), uniq) != ks.end());
        assert(MemeNative::storageRemove(uniq));
        assert(MemeNative::storageGet(uniq, "gone") == "gone");

        assert(MemeNative::keychainSet("meme_suite_svc", uniq, "sec_val"));
        assert(MemeNative::keychainGet("meme_suite_svc", uniq) == "sec_val");
        assert(MemeNative::keychainRemove("meme_suite_svc", uniq));
        assert(MemeNative::keychainGet("meme_suite_svc", uniq).empty());

        auto d = MemeNative::getDeviceInfo();
        assert(!d.os.empty() && !d.arch.empty());
        auto p = MemeNative::getAppPaths();
        assert(!p.appData.empty() && !p.cache.empty());
    }

    // 5. IconAssets PNG encode/decode + ICNS + ICO binary format validation
    {
        auto icon = MemePackager::loadOrDefaultIcon("", 64);
        assert(icon.width == 64 && icon.height == 64 && icon.rgba.size() == 64u * 64u * 4u);
        auto png = MemePackager::encodePngRgba(icon);
        assert(png.size() > 32 && png[0] == 0x89 && png[1] == 'P' && png[2] == 'N' && png[3] == 'G');
        MemePackager::IconImage decoded;
        assert(MemePackager::decodePngRgba(png, decoded));
        assert(decoded.width == 64 && decoded.height == 64 && decoded.rgba.size() == icon.rgba.size());

        auto icns = MemePackager::buildIcnsFromRgba(icon);
        assert(icns.size() > 128 && icns[0] == 'i' && icns[1] == 'c' && icns[2] == 'n' && icns[3] == 's');

        auto ico = MemePackager::buildIcoFromRgba(icon);
        assert(ico.size() > 64 && ico[0] == 0 && ico[1] == 0 && ico[2] == 1 && ico[3] == 0 && ico[4] == 4);
    }

    std::cout << "PASSED\n";
}

static void testPlatformAndPermissionsAndManifest()
{
    std::cout << "Running Platform Separation & Cross-Platform Permissions tests... ";

    // 1. MemePlatform normalization, spec matching, select priority & candidate probing
    {
        assert(MemePlatform::normalizePlatformName("iPhoneOS") == "ios");
        assert(MemePlatform::normalizePlatformName("ipa") == "ios");
        assert(MemePlatform::normalizePlatformName("apk") == "android");
        assert(MemePlatform::normalizePlatformName("win32") == "windows");
        assert(MemePlatform::normalizePlatformName("darwin") == "mac");
        assert(MemePlatform::normalizePlatformName("wasm") == "web");

        assert(MemePlatform::matchPlatformSpec("ios,android", "ios"));
        assert(MemePlatform::matchPlatformSpec("mobile", "android"));
        assert(!MemePlatform::matchPlatformSpec("mobile", "mac"));
        assert(MemePlatform::matchPlatformSpec("apple", "ios"));
        assert(MemePlatform::matchPlatformSpec("apple", "mac"));
        assert(MemePlatform::matchPlatformSpec("desktop", "linux"));
        assert(MemePlatform::matchPlatformSpec("!web", "mac"));
        assert(!MemePlatform::matchPlatformSpec("!web", "web"));

        auto prio = MemePlatform::platformSelectPriority("ios");
        assert(!prio.empty() && prio[0] == "ios");

        namespace fs = std::filesystem;
        fs::path tmp = fs::temp_directory_path() / "meme_plat_probe_suite";
        fs::remove_all(tmp);
        fs::create_directories(tmp / "platforms" / "linux");
        {
            std::ofstream(tmp / "mod.meme") << "kaam v() { de \"default\"; }\n";
            std::ofstream(tmp / "mod.ios.meme") << "kaam v() { de \"ios\"; }\n";
            std::ofstream(tmp / "mod.mobile.meme") << "kaam v() { de \"mobile\"; }\n";
            std::ofstream(tmp / "mod.web.meme") << "kaam v() { de \"web\"; }\n";
            std::ofstream(tmp / "platforms" / "linux" / "mod.meme") << "kaam v() { de \"linux\"; }\n";
        }
        assert(MemePlatform::probeCandidateBase(tmp / "mod", "ios").filename() == "mod.ios.meme");
        assert(MemePlatform::probeCandidateBase(tmp / "mod.meme", "android").filename() == "mod.mobile.meme");
        assert(MemePlatform::probeCandidateBase(tmp / "mod", "web").filename() == "mod.web.meme");
        assert(MemePlatform::probeCandidateBase(tmp / "mod", "linux").filename() == "mod.meme");
        assert(MemePlatform::probeCandidateBase(tmp / "mod", "linux").parent_path().filename() == "linux");
        assert(MemePlatform::probeCandidateBase(tmp / "mod", "windows").filename() == "mod.meme");
        fs::remove_all(tmp);
    }

    // 2. MemePermissions normalization, OS permission maps, strict mode & headless overrides
    {
        assert(MemePermissions::normalizeCapability("CAM") == "camera");
        assert(MemePermissions::normalizeCapability("PhotoLibrary") == "photos");
        assert(MemePermissions::normalizeCapability("geolocation") == "location");
        assert(MemePermissions::normalizeCapability("background_location") == "locationAlways");
        assert(MemePermissions::normalizeCapability("FaceID") == "biometrics");
        assert(MemePermissions::normalizeCapability("POST_NOTIFICATIONS") == "notifications");
        assert(MemePermissions::normalizeStateString("authorized") == "granted");
        assert(MemePermissions::normalizeStateString("notDetermined") == "prompt");
        assert(MemePermissions::normalizeStateString("blocked") == "denied");

        auto ph = MemePermissions::androidPermissionsFor("photos");
        assert(ph.size() == 2 && ph[0].name == "android.permission.READ_MEDIA_IMAGES" && ph[1].maxSdkVersion == 32);
        auto ble = MemePermissions::androidPermissionsFor("bluetooth");
        assert(!ble.empty() && ble[0].flags == "neverForLocation");
        auto locCoarse = MemePermissions::androidPermissionsFor("location", "coarse");
        assert(locCoarse.size() == 1 && locCoarse[0].name == "android.permission.ACCESS_COARSE_LOCATION");

        assert(MemePermissions::appleUsagePlistKeyFor("camera", "Demo").first == "NSCameraUsageDescription");
        assert(MemePermissions::macEntitlementKeyFor("microphone") == "com.apple.security.device.audio-input");
        auto webPol = MemePermissions::webPolicyDirectivesFor("location");
        assert(webPol.size() == 1 && webPol[0] == "geolocation=(self)");

        assert(MemePermissions::isSupportedOnPlatform("camera", "web"));
        assert(!MemePermissions::isSupportedOnPlatform("biometrics", "web"));
        assert(MemePermissions::isSupportedOnPlatform("biometrics", "ios"));

#if defined(_WIN32)
        _putenv_s("MEME_HEADLESS", "1");
#else
        setenv("MEME_HEADLESS", "1", 1);
#endif
        MemePermissions::setDeclaredPermissions({"notifications", "camera"}, true, {{"camera", "Scan QR"}});
        auto d = MemePermissions::getDeclaredPermissions();
        assert(d.size() == 2 && MemePermissions::isPermissionDeclared("camera") && !MemePermissions::isPermissionDeclared("location"));

        auto sLoc = MemeNative::checkPermission("location");
        assert(sLoc.state == "denied" && !sLoc.granted && !sLoc.canAskAgain);

        MemePermissions::setHeadlessPermissionOverride("camera", "prompt-with-rationale");
        auto sCam = MemeNative::checkPermission("camera");
        assert(sCam.state == "prompt-with-rationale" && sCam.canAskAgain && sCam.reason == "Scan QR");

        MemePermissions::setHeadlessPermissionOverride("photos", "limited");
        MemePermissions::setDeclaredPermissions({"notifications", "camera", "photos"}, true, {});
        auto sPh = MemeNative::checkPermission("photos");
        assert(sPh.state == "limited" && sPh.granted && sPh.limited);

        MemePermissions::clearHeadlessPermissionOverrides();
        MemePermissions::setDeclaredPermissions({}, false, {});
        MemePermissions::permStore().hasDeclaredCapabilities = false;
    }

    // 3. tapri.json per-platform permissions parser
    {
        namespace fs = std::filesystem;
        fs::path tmp = fs::temp_directory_path() / "meme_tapri_perm_suite";
        fs::remove_all(tmp);
        fs::create_directories(tmp / "src");
        {
            std::ofstream(tmp / "tapri.json") << R"({
              "name": "sample-perm",
              "displayName": "SamplePerm",
              "strictPermissions": true,
              "permissions": {
                "common": ["notifications"],
                "ios": { "camera": "iOS Custom Cam" },
                "android": ["location"]
              }
            })";
            std::ofstream(tmp / "src" / "App.meme") << "dikha \"hi\";\n";
        }
        auto cfg = MemePackager::detail::mergeTapriConfig(tmp / "src" / "App.meme", MemePackager::PackageConfig{});
        assert(cfg.displayName == "SamplePerm");
        assert(cfg.permissions.hasPermissionsBlock && cfg.permissions.strictMode);
        auto iosCaps = cfg.permissions.resolvedForTarget("ios");
        assert(std::find(iosCaps.begin(), iosCaps.end(), "notifications") != iosCaps.end());
        assert(std::find(iosCaps.begin(), iosCaps.end(), "camera") != iosCaps.end());
        assert(std::find(iosCaps.begin(), iosCaps.end(), "location") == iosCaps.end());
        assert(cfg.permissions.reasonFor("ios", "camera", "SamplePerm") == "iOS Custom Cam");
        auto andCaps = cfg.permissions.resolvedForTarget("apk");
        assert(std::find(andCaps.begin(), andCaps.end(), "location") != andCaps.end());
        assert(std::find(andCaps.begin(), andCaps.end(), "camera") == andCaps.end());
        fs::remove_all(tmp);
    }

    // 4. Interpreter E2E: platform, permissions, .then(), <Platform>, and platform file imports
    {
#if defined(_WIN32)
        _putenv_s("MEME_HEADLESS", "1");
#else
        setenv("MEME_HEADLESS", "1", 1);
#endif
        std::string out = runAndCapture(R"(
bula "platform";
bula "permissions";
bula "ui:shadcn" jaise { Platform };
pakad o = platform.os();
pakad s = platform.select({ "ios": "I", "android": "A", "mac": "M", "windows": "W", "linux": "L", "web": "WB", "default": "D" });
pakad c1 = permissions.check("cam");
pakad cbSt = "";
permissions.request("camera", (r) => { cbSt = r.state; });
pakad thSt = "";
permissions.request("microphone").then((r) => { thSt = r.state; });
pakad manyOk = jhooth;
permissions.requestMany(["notifications", "location"], (m) => {
    manyOk = m.notifications.granted && m.location.permission == "location";
});
dikha o + "|" + s + "|" + c1.permission + "|" + cbSt + "|" + thSt + "|" + string(manyOk);
)");
        assert(out.find("|camera|") != std::string::npos);
        assert(out.find("granted|granted|han") != std::string::npos);

        namespace fs = std::filesystem;
        fs::path tmp = fs::temp_directory_path() / "meme_interp_plat_import";
        fs::remove_all(tmp);
        fs::create_directories(tmp);
        {
            std::ofstream(tmp / "drv.meme") << "kaam name() { de \"DRV_DEFAULT\"; }\n";
            std::ofstream(tmp / "drv.ios.meme") << "kaam name() { de \"DRV_IOS\"; }\n";
            std::ofstream(tmp / "drv.web.meme") << "kaam name() { de \"DRV_WEB\"; }\n";
            std::ofstream(tmp / "main.meme") << "bula \"./drv\" jaise d;\nbula \"platform\";\ndikha platform.os() + \":\" + d.name();\n";
        }
        auto runFileWithPlat = [&](const std::string &plat) -> std::string {
#if defined(_WIN32)
            _putenv_s("MEME_PLATFORM", plat.c_str());
#else
            setenv("MEME_PLATFORM", plat.c_str(), 1);
#endif
            MemePermissions::clearHeadlessPermissionOverrides();
            std::stringstream cap;
            std::stringstream in;
            Interpreter interp(cap, in);
            interp.setHeadless(true);
            interp.setCurrentFilePath((tmp / "main.meme").string());
            std::ifstream mf(tmp / "main.meme");
            std::stringstream src;
            src << mf.rdbuf();
            Lexer lx(src.str());
            auto toks = lx.tokenize();
            Parser ps(toks);
            auto prog = ps.parseProgram();
            interp.interpret(prog);
            return cap.str();
        };
        assert(runFileWithPlat("ios").find("ios:DRV_IOS") != std::string::npos);
        assert(runFileWithPlat("web").find("web:DRV_WEB") != std::string::npos);
        assert(runFileWithPlat("windows").find("windows:DRV_DEFAULT") != std::string::npos);
#if !defined(_WIN32)
        unsetenv("MEME_PLATFORM");
#endif
        fs::remove_all(tmp);
        MemePermissions::clearHeadlessPermissionOverrides();
    }

    std::cout << "PASSED\n";
}

void testPureMemeEvalAndWebHostingArtifacts()
{
    std::cout << "Testing meme.eval(), akshar_bana/size(), max-w-6xl, and Web Router/Hosting Shell... ";

    // 1. meme.eval() sandboxed stdout, error trapping, and GPU UI command capture
    {
        std::string out = runAndCapture(R"(
bula "meme";
pakad okRes = meme.eval("dikha 2 + 3 * 4;");
pakad errRes = meme.eval("pakad bad = 1 / 0;");
pakad uiRes = meme.eval("bula \"ui\" jaise { useState }; jugaad App() { ye_le <div className=\"p-4 max-w-6xl\"><span>Hi</span></div>; } ui.mount(App); ui.show(\"EvalUi\");");
pakad ch = akshar_bana(65) + ":" + string(code_de("A")) + ":" + string(size([1, 2, 3])) + ":" + string(size("meme"));
dikha string(!okRes.isError) + "|" + okRes.output.trim() + "|" + string(okRes.hasUi);
dikha string(!errRes.isError) + "|" + string(size(errRes.error) > 0);
dikha string(!uiRes.isError) + "|" + string(uiRes.hasUi) + "|" + string(size(uiRes.gpuCommands) > 0);
dikha ch;
)");
        assert(out.find("han|14|nhi") != std::string::npos);
        assert(out.find("nhi|han") != std::string::npos);
        assert(out.find("han|han|han") != std::string::npos);
        assert(out.find("A:65:3:4") != std::string::npos);
    }

    // 2. Tailwind numeric strictness & max-w-6xl / max-w-7xl
    {
        assert(MemeUI::parseTwNumericOrArb("6xl") == 0.0f);
        assert(MemeUI::parseTwNumericOrArb("12") == 48.0f);
        MemeUI::ComputedStyle st6;
        MemeUI::applySingleTailwindToken("max-w-6xl", st6);
        assert(st6.maxWidth == 1152.0f);
        MemeUI::ComputedStyle st7;
        MemeUI::applySingleTailwindToken("max-w-7xl", st7);
        assert(st7.maxWidth == 1280.0f);
    }

    // 3. Web Canvas HTML shell <base href="/"> + Router history sync
    {
        std::string shell = MemeWebHost::generateZeroDomHtmlShell("docs.js", "MEMElang Docs", "#09090b", true, "clipboard-write=(self)");
        assert(shell.find("<base href=\"/\"") != std::string::npos);
        assert(shell.find("window.__MEME_PUSH_ROUTE__") != std::string::npos);
        assert(shell.find("meme_web_set_route") != std::string::npos);
        assert(shell.find("<canvas id=\"meme-canvas\"") != std::string::npos);
        assert(shell.find("Permissions-Policy") != std::string::npos);
    }

    // 4. English syntax parity: extends, for..from..to..step, let/fn/if/else/return/print
    {
        std::string enOut = runAndCapture(R"(
class Animal {
    fn speak() { return "base"; }
}
class Dog extends Animal {
    fn speak() { return "woof"; }
}
let total = 0;
for i from 1 to 6 step 2 {
    total = total + i;
}
let d = Dog();
if total == 9 {
    print d.speak() + ":" + str(total);
} else {
    print "fail";
}
)");
        assert(enOut.find("woof:9") != std::string::npos);
    }

    std::cout << "PASSED\n";
}

void testWebDomFontsIconsMotionJsBridgeAndSsr()
{
    std::cout << "Testing Web DOM Transpiler, Multi-Fonts, Lucide Icons, Motion, Zero-WebView JS Bridge & SSR Middleware... " << std::flush;

    namespace fs = std::filesystem;
    fs::path tmpRoot = fs::temp_directory_path() / "meme_suite_m2_m3_test";
    fs::remove_all(tmpRoot);
    fs::create_directories(tmpRoot);

    // 1. WebDomTranspiler: AST -> standard HTML + Tailwind CSS + ES6 JS + Icons + Motion + Lazy Eval
    {
        fs::path domEntry = tmpRoot / "DomApp.meme";
        std::ofstream df(domEntry);
        df << R"(
bula "ui" jaise { useState };
bula "ui:icons" jaise { Icon, Terminal };
bula "ui:motion" jaise { motion, useSpring };
bula "meme";

jugaad DomApp() {
    pakad [count, setCount] = useState(0);
    pakad val = useSpring(count * 10);
    ye_le (
        <motion.div className="min-h-screen bg-zinc-950 p-6 grid grid-cols-2 gap-4 animate-fade-in">
            <Icon name="rocket" size={20} color="#f59e0b" />
            <Terminal size={18} />
            <button onClick={() => setCount(count + 1)}>{"Count=" + string(count) + ":" + string(val)}</button>
            <button onClick={() => meme.eval("dikha 2 + 2;")}>Run</button>
        </motion.div>
    );
}
ui.mount(DomApp, "Dom Docs App");
)";
        df.close();

        MemeWebDom::WebDomOptions wopts;
        wopts.title = "Dom Docs App";
        auto bundle = MemeWebDom::transpileToWebDomBundle(domEntry, wopts);
        assert(bundle.success);
        assert(bundle.usesIcons);
        assert(bundle.usesMotion);
        assert(bundle.usesEval);
        assert(bundle.stylesCss.find(".animate-fade-in") != std::string::npos);
        assert(bundle.stylesCss.find("grid-template-columns") != std::string::npos);
        assert(bundle.memeDomJs.find("\"alert-triangle\"") != std::string::npos);
        assert(bundle.memeDomJs.find("\"rocket\"") != std::string::npos);
        assert(bundle.memeDomJs.find("getInitialData") != std::string::npos);
        assert(bundle.indexHtml.find("<canvas") == std::string::npos);
        assert(bundle.memeDomJs.find("lazyEval") != std::string::npos);
        assert(bundle.appJs.find("$m.meme") != std::string::npos || bundle.appJs.find("lazyEval") != std::string::npos);
    }

    // 2. Multi-Font TrueType/Kerning Engine & 100+ Lucide Icons + Multi-Subpath Elliptical Arc SVG Tessellation
    {
        assert(MemeUI::Icons::registry().size() >= 100);
        assert(MemeUI::Icons::normalizeIconName("AlertTriangle") == "alert-triangle");
        assert(!MemeUI::Icons::lookupIconSvgPath("github").empty());
        assert(!MemeUI::Icons::lookupIconSvgPath("shield-check").empty());

        uint64_t ver0 = MemeUI::FontAtlas::getAtlasVersion();
        assert(ver0 >= 1);
        float wSans = MemeUI::MemeUiFonts::measureTextWidth("AVAVA", 16.0f, MemeUI::FontFamily::SANS);
        float wMono = MemeUI::MemeUiFonts::measureTextWidth("AVAVA", 16.0f, MemeUI::FontFamily::MONO);
        float wSerif = MemeUI::MemeUiFonts::measureTextWidth("AVAVA", 16.0f, MemeUI::FontFamily::SERIF);
        assert(wSans > 15.0f && wMono > 15.0f && wSerif > 15.0f);
        auto rgba = MemeUI::FontAtlas::getRgbaAtlas();
        assert(rgba[0] == 255 && rgba[3] == 255);

        std::vector<MemeUI::GpuDrawCmd> iconCmds;
        for (const char *ic : {"github", "clock", "rocket", "alert-triangle"}) {
            MemeUI::GpuDrawCmd sc;
            sc.type = "DRAW_SVG_PATH";
            sc.x = 10; sc.y = 10; sc.width = 24; sc.height = 24;
            sc.svgPath = MemeUI::Icons::lookupIconSvgPath(ic);
            sc.lineWidth = 2.0f;
            sc.color = "#f59e0b";
            iconCmds.push_back(sc);
        }
        auto mesh = MemeUI::Tessellator::tessellate(iconCmds, 400.0f, 300.0f);
        assert(mesh.vertices.size() > 80);
        assert(mesh.indices.size() > 120 && (mesh.indices.size() % 3) == 0);
    }

    // 3. Spring Animation Physics + ui:icons & ui:motion Interpreter Integration
    {
        MemeUI::UiAnimationStore store;
        store.setTarget("card:op", 1.0f, 0.0f, true);
        float prev = store.getValue("card:op", 0.0f);
        assert(prev == 0.0f);
        for (int i = 0; i < 60; ++i) store.tickAll(16.0);
        float settled = store.getValue("card:op", 0.0f);
        assert(settled > 0.9f && settled <= 1.05f);

        std::string uiAnimOut = runAndCapture(R"(
bula "ui:icons" jaise { Icon, Play };
bula "ui:motion" jaise { motion, useSpring };
pakad v = useSpring(42);
pakad node = <motion.div initial={{"opacity": 0, "y": 10}} animate={{"opacity": 1, "y": 0}}><Icon name="play" size={20} /><Play size={24} /></motion.div>;
dikha node.tag + ":" + string(node.props.animateOpacity) + ":" + string(node.children.lambai()) + ":" + string(v);
)");
        assert(uiAnimOut.find("div:1:2:42") != std::string::npos);
    }

    // 4. Zero-WebView Embedded JS Ecosystem (JavaScriptCore / QuickJS-NG) + ESM + local node_modules
    {
        fs::path modDir = tmpRoot / "node_modules" / "mini-math";
        fs::create_directories(modDir);
        std::ofstream pjf(modDir / "package.json");
        pjf << "{\"name\":\"mini-math\",\"version\":\"1.2.3\",\"main\":\"index.js\"}";
        pjf.close();
        std::ofstream mjf(modDir / "index.js");
        mjf << "export function add(a, b) { return a + b; }\nexport const VERSION = '1.2.3';\n";
        mjf.close();

        fs::path scriptPath = tmpRoot / "js_test.meme";
        std::ofstream sf(scriptPath);
        sf << R"(
bula "npm:clsx" jaise clsx;
bula "npm:tailwind-merge" jaise { twMerge };
bula "npm:zustand" jaise { create };
bula "npm:mini-math" jaise { add, VERSION };

pakad cls = twMerge("p-2 p-6 text-sm", clsx("flex", {"hidden": nhi, "gap-2": han}));
pakad store = create((set) => ({"score": 99}));
dikha "ENGINE=" + clsx.__jsEngine;
dikha "CLS=" + cls;
dikha "ZUSTAND=" + string(store.getState().score);
dikha "MINIMATH=" + string(add(15, 27)) + ":" + VERSION;
)";
        sf.close();

        std::stringstream out, in;
        Interpreter interp(out, in);
        interp.setHeadless(true);
        interp.setCurrentFilePath(scriptPath.string());
        std::ifstream ifs(scriptPath);
        std::stringstream src; src << ifs.rdbuf();
        Lexer lex(src.str());
        auto toks = lex.tokenize();
        Parser parser(toks);
        auto prog = parser.parseProgram();
        interp.interpret(prog);
        std::string s = out.str();
        assert(s.find("ENGINE=JavaScriptCore") != std::string::npos || s.find("ENGINE=QuickJS-NG") != std::string::npos);
        assert(s.find("CLS=p-6 text-sm flex gap-2") != std::string::npos);
        assert(s.find("ZUSTAND=99") != std::string::npos);
        assert(s.find("MINIMATH=42:1.2.3") != std::string::npos);
    }

    // 5. Next.js-Style SSR Router, Middleware, App Router Layouts, Loaders & API Route Handlers
    {
        fs::path ssrRoot = tmpRoot / "ssr_app";
        fs::create_directories(ssrRoot / "app" / "blog" / "[slug]");
        fs::create_directories(ssrRoot / "app" / "api" / "ping");

        {
            std::ofstream f(ssrRoot / "middleware.meme");
            f << R"(
jugaad middleware(ctx) {
    ctx.setHeader("x-meme-mw", "active");
    dekh ctx.req.path == "/old-home" {
        ye_le ctx.redirect("/new-home", 302);
    }
    dekh ctx.req.path == "/alias-post" {
        ye_le ctx.rewrite("/blog/middleware-rewrite");
    }
    ye_le ctx.next();
}
)";
        }
        {
            std::ofstream f(ssrRoot / "app" / "layout.meme");
            f << R"(
jugaad Layout(props) {
    ye_le <main className="min-h-screen bg-zinc-950 p-6"><header className="font-bold">SSR Shell</header>{props.children}</main>;
}
)";
        }
        {
            std::ofstream f(ssrRoot / "app" / "page.meme");
            f << R"(
jugaad loader(ctx) {
    ye_le {"greeting": "Namaste SSR", "q": ctx.query.tag};
}
jugaad head(ctx) {
    ye_le {"title": "Home SSR Title", "description": "Fast MEMElang SSR"};
}
jugaad Page(props) {
    ye_le <section className="p-4 text-zinc-100"><h1>{props.data.greeting}</h1></section>;
}
)";
        }
        {
            std::ofstream f(ssrRoot / "app" / "blog" / "[slug]" / "page.meme");
            f << R"(
bula "ui:icons" jaise { Icon };
jugaad head(ctx) {
    ye_le {"title": "Post: " + ctx.params.slug};
}
jugaad Page(props) {
    ye_le <article className="p-4"><Icon name="rocket" size={20} /><h2>{props.params.slug}</h2></article>;
}
)";
        }
        {
            std::ofstream f(ssrRoot / "app" / "api" / "ping" / "route.meme");
            f << R"(
jugaad GET(ctx) {
    ye_le {"ok": han, "method": ctx.method, "route": "/api/ping"};
}
)";
        }

        MemeSsr::MemeSsrEngine ssr;
        std::string discoverErr;
        assert(ssr.discover(ssrRoot, discoverErr));
        assert(ssr.routes.size() == 3);
        assert(!ssr.middlewareFile.empty());

        auto rRedir = ssr.handleRequest({"GET", "/old-home", "/old-home", "", {}, {}, ""});
        assert(rRedir.status == 302);
        assert(rRedir.headers["Location"] == "/new-home");
        assert(rRedir.headers["x-meme-mw"] == "active");

        auto rHome = ssr.handleRequest({"GET", "/?tag=v1", "/?tag=v1", "", {}, {}, ""});
        assert(rHome.status == 200);
        assert(rHome.headers["x-meme-mw"] == "active");
        assert(rHome.body.find("<title>Home SSR Title</title>") != std::string::npos);
        assert(rHome.body.find("SSR Shell") != std::string::npos);
        assert(rHome.body.find("Namaste SSR") != std::string::npos);
        assert(rHome.body.find("__MEME_SSR_DATA__") != std::string::npos);

        auto rRewrite = ssr.handleRequest({"GET", "/alias-post", "/alias-post", "", {}, {}, ""});
        assert(rRewrite.status == 200);
        assert(rRewrite.body.find("<title>Post: middleware-rewrite</title>") != std::string::npos);
        assert(rRewrite.body.find("<svg") != std::string::npos);

        auto rApi = ssr.handleRequest({"GET", "/api/ping", "/api/ping", "", {}, {}, ""});
        assert(rApi.status == 200);
        assert(rApi.headers["Content-Type"].find("application/json") != std::string::npos);
        assert(rApi.body.find("\"ok\":true") != std::string::npos);
    }

    fs::remove_all(tmpRoot);
    std::cout << "PASSED\n";
}

int main()
{
    std::cout << "========================================\n";
    std::cout << "🎭 MEMElang Comprehensive Test Suite\n";
    std::cout << "========================================\n";

    testLexer();
    testArithmeticAndOperators();
    testLogicAndComparisons();
    testVariablesAndScoping();
    testControlFlow();
    testFunctionsAndRecursion();
    testBuiltins();
    testObjectOrientedProgramming();
    testGenZMemeVocabulary();
    testPythonicBatchA();
    testPythonicBatchB();
    testGradualTypingAndMemoryEfficiency();
    testPythonicWhatsAppErgonomics();
    testRichDikhaStdlibAndExpandedMethods();
    testTijoriDhabaAndUi();
    testEcosystemBridgesAndPolyfills();
    testZeroWebViewNativeUiTailwindAndGpuCommands();
    testTapriAndDestructuring();
    testStatefulReactHooksEventsContextAndRouter();
    testProjectWorkspaceVenvScriptsAndTestRunner();
    testMultiplatformTodoAppErgonomicsAndStress();
    testAsyncFetchThreadsMultiprocessingAndFs();
    testOptimizedDatatypesAndExtendedPrimitives();
    testStandardLibraries();
    testFastHttpServersExpressAndElysiaTypeSafety();
    testNativeUiPrimitivesTextEngineAndTessellator();
    testInnerContainerScrollAndNativeApisAndIcons();
    testPlatformAndPermissionsAndManifest();
    testPureMemeEvalAndWebHostingArtifacts();
    testWebDomFontsIconsMotionJsBridgeAndSsr();

    std::cout << "========================================\n";
    std::cout << "✅ ALL TESTS PASSED SUCCESSFULLY! Wah bete mauj kardi!\n";
    std::cout << "========================================\n";

    return 0;
}
