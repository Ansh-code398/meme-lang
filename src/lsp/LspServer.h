#ifndef MEMELANG_LSPSERVER_H
#define MEMELANG_LSPSERVER_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "../lexer/Lexer.h"
#include "../parser/Parser.h"
#include "../types/TypeChecker.h"
#include "JsonRpc.h"

namespace memelang::lsp {

struct DocumentState {
    std::string uri;
    std::string content;
    int version = 0;
    std::vector<std::unique_ptr<Stmt>> ast;
    std::vector<Diagnostic> diagnostics;
};

class LspServer {
private:
    std::unordered_map<std::string, DocumentState> documents;
    bool isRunning = true;
    std::istream &in;
    std::ostream &out;

    void analyzeDocument(const std::string &uri, const std::string &text, int version = 1);
    void publishDiagnostics(const std::string &uri, const std::vector<Diagnostic> &diags);

    void handleInitialize(const JsonValue &id);
    void handleDidOpen(const JsonValue &params);
    void handleDidChange(const JsonValue &params);
    void handleDidClose(const JsonValue &params);
    void handleHover(const JsonValue &id, const JsonValue &params);
    void handleCompletion(const JsonValue &id, const JsonValue &params);
    void handleDefinition(const JsonValue &id, const JsonValue &params);
    void handleShutdown(const JsonValue &id);

    std::string getHoverMarkdown(const std::string &tokenText);

public:
    LspServer(std::istream &inStream = std::cin, std::ostream &outStream = std::cout)
        : in(inStream), out(outStream) {}

    void run();
    void handleMessage(const std::string &jsonStr);
};

} // namespace memelang::lsp

#endif // MEMELANG_LSPSERVER_H
