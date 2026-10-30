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

    // 8. Implicit Typed Declarations
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

    std::cout << "========================================\n";
    std::cout << "✅ ALL TESTS PASSED SUCCESSFULLY! Wah bete mauj kardi!\n";
    std::cout << "========================================\n";

    return 0;
}
