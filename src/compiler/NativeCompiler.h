#ifndef NATIVE_COMPILER_H
#define NATIVE_COMPILER_H

#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include "../parser/Parser.h"

struct CompileOptions {
    std::string outputPath = "";
    bool runAfterCompile = false;
    std::vector<std::string> runArgs = {};
    bool optimize = true;          // -O3
    bool debug = false;            // -g
    bool keepCppSource = false;
    bool verbose = false;
};

struct CompileResult {
    bool success = false;
    std::string binaryPath = "";
    std::string generatedCppPath = "";
    std::string generatedCppCode = "";
    std::string errorMessage = "";
    int exitCode = 0;
    double compileTimeMs = 0.0;
};

class NativeCompiler {
public:
    static std::string generateCpp(const std::vector<std::unique_ptr<Stmt>> &program,
                                   const std::string &sourceName = "meme_program");

    static CompileResult compile(const std::vector<std::unique_ptr<Stmt>> &program,
                                 const CompileOptions &options,
                                 const std::string &sourceName = "meme_program");

    static CompileResult compileFile(const std::string &sourcePath,
                                     const CompileOptions &options);

    static int runNative(const std::string &sourcePath,
                         const std::vector<std::string> &args = {},
                         bool verbose = false);
};

#endif // NATIVE_COMPILER_H
