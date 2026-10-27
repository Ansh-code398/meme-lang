#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <cstdlib>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define WASM_EXPORT EMSCRIPTEN_KEEPALIVE

EM_JS(char*, js_get_input, (const char* prompt_str), {
    let promptText = "";
    if (prompt_str) {
        promptText = UTF8ToString(prompt_str);
    }
    let val = "";
    if (typeof window !== 'undefined' && typeof window.memePromptHandler === 'function') {
        val = window.memePromptHandler(promptText);
    } else if (typeof window !== 'undefined' && typeof window.prompt === 'function') {
        val = window.prompt(promptText || "Input for MEMElang:") ?? "";
    }
    if (val === null || val === undefined) {
        val = "";
    }
    const lengthBytes = lengthBytesUTF8(val) + 1;
    const stringOnWasmHeap = _malloc(lengthBytes);
    stringToUTF8(val, stringOnWasmHeap, lengthBytes);
    return stringOnWasmHeap;
});

#else
#define WASM_EXPORT
#endif

#include "../lexer/Lexer.h"
#include "../parser/Parser.h"
#include "../types/TypeChecker.h"
#include "../interpreter/Interpreter.h"

static std::string output_buffer;
static std::string last_gpu_commands_buffer;
static std::string last_hit_regions_buffer;
static std::unique_ptr<Interpreter> g_wasm_interpreter;
static std::ostringstream g_wasm_out;
static std::istringstream g_wasm_in;

extern "C" {

WASM_EXPORT
const char* run_meme_code(const char* source_code, const char* input_text) {
    output_buffer.clear();
    last_gpu_commands_buffer.clear();
    last_hit_regions_buffer = "[]";

    if (!source_code || source_code[0] == '\0') {
        output_buffer = "";
        return output_buffer.c_str();
    }

    g_wasm_out.str("");
    g_wasm_out.clear();
    std::string in_str = (input_text && input_text[0] != '\0') ? input_text : "";
    g_wasm_in.str(in_str);
    g_wasm_in.clear();

    try {
        Lexer lexer(source_code);
        std::vector<Token> tokens = lexer.tokenize();

        Parser parser(tokens);
        std::vector<std::unique_ptr<Stmt>> program = parser.parseProgram();

        TypeChecker typeChecker;
        typeChecker.check(program);

        g_wasm_interpreter = std::make_unique<Interpreter>(g_wasm_out, g_wasm_in);

#ifdef __EMSCRIPTEN__
        g_wasm_interpreter->setInputPromptCallback([](const std::string &prompt) -> std::string {
            char* res = js_get_input(prompt.c_str());
            if (res) {
                std::string s(res);
                free(res);
                return s;
            }
            return "";
        });
#endif

        g_wasm_interpreter->interpret(program);
        last_gpu_commands_buffer = g_wasm_interpreter->getLastGpuCommandsJson();
        last_hit_regions_buffer = g_wasm_interpreter->getLastHitRegionsJson();

        output_buffer = g_wasm_out.str();
    } catch (const std::exception &e) {
        output_buffer = g_wasm_out.str();
        if (!output_buffer.empty() && output_buffer.back() != '\n') {
            output_buffer += "\n";
        }
        output_buffer += "\n❌ Gadbad ho gayi:\n";
        output_buffer += e.what();
        output_buffer += "\n";
    } catch (...) {
        output_buffer = g_wasm_out.str();
        if (!output_buffer.empty() && output_buffer.back() != '\n') {
            output_buffer += "\n";
        }
        output_buffer += "\n❌ Anjaan gadbad ho gayi!\n";
    }

    return output_buffer.c_str();
}

WASM_EXPORT
const char* get_meme_version() {
    return "MEMElang v1.0 [Ye Bik Gayi Hai Gormint Edition - WebAssembly]";
}

WASM_EXPORT
const char* get_last_gpu_commands() {
    return last_gpu_commands_buffer.c_str();
}

WASM_EXPORT
const char* get_last_hit_regions() {
    if (g_wasm_interpreter) {
        last_hit_regions_buffer = g_wasm_interpreter->getLastHitRegionsJson();
    }
    return last_hit_regions_buffer.c_str();
}

WASM_EXPORT
const char* wasm_dispatch_ui_event(int nodeId, const char* eventType, const char* value) {
    if (!g_wasm_interpreter) return "[]";
    g_wasm_interpreter->dispatchUiEvent(nodeId, eventType ? eventType : "click", value ? value : "");
    last_gpu_commands_buffer = g_wasm_interpreter->getLastGpuCommandsJson();
    last_hit_regions_buffer = g_wasm_interpreter->getLastHitRegionsJson();
    return last_gpu_commands_buffer.c_str();
}

}
