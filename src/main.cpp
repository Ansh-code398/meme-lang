#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "interpreter/Interpreter.h"

static void printBanner()
{
    std::cout << "🎭 MEMElang v1.0 [Ye Bik Gayi Hai Gormint Edition]\n";
    std::cout << "Type 'bas;' or 'exit' to quit. Shuru ho jaao!\n\n";
}

static void printUsage(const char *prog)
{
    std::cout << "Usage:\n";
    std::cout << "  " << prog << "                   Launch interactive REPL\n";
    std::cout << "  " << prog << " <filename.meme>   Run a MEMElang script file\n";
    std::cout << "  " << prog << " -e \"code\"         Run code directly from command line\n";
    std::cout << "  " << prog << " -h, --help        Show this help message\n";
}

static bool runSource(const std::string &source, Interpreter &interpreter)
{
    try
    {
        Lexer lexer(source);
        std::vector<Token> tokens = lexer.tokenize();

        Parser parser(tokens);
        std::vector<std::unique_ptr<Stmt>> program = parser.parseProgram();

        interpreter.interpret(program);
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "\n❌ Gadbad ho gayi:\n" << e.what() << "\n\n";
        return false;
    }
}

static void runFile(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "❌ File '" << path << "' gayab hai! Sahi path daalo warna main isse out hoon.\n";
        exit(1);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    Interpreter interpreter;
    if (!runSource(source, interpreter))
    {
        exit(1);
    }
}

static void runRepl()
{
    printBanner();
    Interpreter interpreter;
    std::string line;

    while (true)
    {
        std::cout << "meme > ";
        if (!std::getline(std::cin, line))
        {
            std::cout << "\nKhatam, tata, bye-bye!\n";
            break;
        }

        // Trim whitespace
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos)
            continue;
        size_t end = line.find_last_not_of(" \t\r\n");
        std::string trimmed = line.substr(start, end - start + 1);

        if (trimmed == "exit" || trimmed == "bas;" || trimmed == "quit")
        {
            std::cout << "Khatam, tata, bye-bye!\n";
            break;
        }

        // If line doesn't end with ';' or '}', and it's not a block, try adding ';'
        if (trimmed.back() != ';' && trimmed.back() != '}')
        {
            trimmed += ";";
        }

        runSource(trimmed, interpreter);
    }
}

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        runRepl();
    }
    else if (argc == 2)
    {
        std::string arg = argv[1];
        if (arg == "-h" || arg == "--help")
        {
            printUsage(argv[0]);
        }
        else if (arg == "-v" || arg == "--version")
        {
            std::cout << "MEMElang v1.0\n";
        }
        else
        {
            runFile(arg);
        }
    }
    else if (argc == 3 && std::string(argv[1]) == "-e")
    {
        Interpreter interpreter;
        std::string code = argv[2];
        if (code.find_last_not_of(" \t\r\n") != std::string::npos &&
            code[code.find_last_not_of(" \t\r\n")] != ';' &&
            code[code.find_last_not_of(" \t\r\n")] != '}')
        {
            code += ";";
        }
        if (!runSource(code, interpreter))
        {
            return 1;
        }
    }
    else
    {
        printUsage(argv[0]);
        return 1;
    }

    return 0;
}