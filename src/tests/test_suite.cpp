#include <iostream>
#include <sstream>
#include <cassert>
#include <string>
#include <vector>
#include "../lexer/Lexer.h"
#include "../parser/Parser.h"
#include "../interpreter/Interpreter.h"

// Test helper: runs code in a fresh interpreter and captures stdout
static std::string runAndCapture(const std::string &code)
{
    std::stringstream out;
    std::stringstream in;
    Interpreter interpreter(out, in);

    Lexer lexer(code);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto program = parser.parseProgram();

    interpreter.interpret(program);
    return out.str();
}

// Test helper: verifies that code throws an exception containing expected substring
static bool expectError(const std::string &code, const std::string &expectedSubstring)
{
    try
    {
        std::stringstream out;
        std::stringstream in;
        Interpreter interpreter(out, in);

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

    // Error on undeclared variable
    assert(expectError("dikha unknownVar;", "Rasode mein kaun tha"));
    assert(expectError("unknownVar = 5;", "Chuna laga diya"));

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
    assert(expectError("pakad a = 1", "Chhoti bachi ho kya"));
    assert(expectError("pakad s = \"hello;", "String khol ke chhod diya"));
    assert(expectError("(x) = 5;", "Ye sab doglapan hai"));
    assert(expectError("dikha 10 < 'abc';", "Seb ko santre se mat tolo"));
    assert(expectError("dikha -'abc';", "ke aage minus (-) kaise laga diya"));
    assert(expectError("pakad x = 5; x();", "Beta, tumse na ho payega"));

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

    std::cout << "========================================\n";
    std::cout << "✅ ALL TESTS PASSED SUCCESSFULLY! Wah bete mauj kardi!\n";
    std::cout << "========================================\n";

    return 0;
}
