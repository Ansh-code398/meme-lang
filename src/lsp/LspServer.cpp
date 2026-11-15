#include "LspServer.h"
#include <iostream>
#include <sstream>

namespace memelang::lsp {

void LspServer::run() {
    std::string payload;
    while (isRunning && JsonRpcTransport::readMessage(in, payload)) {
        if (!payload.empty()) {
            handleMessage(payload);
        }
    }
}

void LspServer::handleMessage(const std::string &jsonStr) {
    JsonValue req = JsonValue::parse(jsonStr);
    std::string method = req["method"].asString();
    JsonValue id = req["id"];

    if (method == "initialize") {
        handleInitialize(id);
    } else if (method == "initialized") {
        // Notification, nothing to send back
    } else if (method == "shutdown") {
        handleShutdown(id);
    } else if (method == "exit") {
        isRunning = false;
    } else if (method == "textDocument/didOpen") {
        handleDidOpen(req["params"]);
    } else if (method == "textDocument/didChange") {
        handleDidChange(req["params"]);
    } else if (method == "textDocument/didClose") {
        handleDidClose(req["params"]);
    } else if (method == "textDocument/hover") {
        handleHover(id, req["params"]);
    } else if (method == "textDocument/completion") {
        handleCompletion(id, req["params"]);
    } else if (method == "textDocument/definition") {
        handleDefinition(id, req["params"]);
    } else if (!id.isNull()) {
        // Unknown request, return method not found
        JsonValue resp;
        resp["jsonrpc"] = "2.0";
        resp["id"] = id;
        resp["error"]["code"] = -32601;
        resp["error"]["message"] = "Method not found: " + method;
        JsonRpcTransport::writeMessage(out, resp.stringify());
    }
}

void LspServer::handleInitialize(const JsonValue &id) {
    JsonValue resp;
    resp["jsonrpc"] = "2.0";
    resp["id"] = id;

    JsonValue &res = resp["result"];
    JsonValue &cap = res["capabilities"];
    cap["textDocumentSync"] = 1; // Full sync
    cap["hoverProvider"] = true;

    JsonValue &comp = cap["completionProvider"];
    comp["triggerCharacters"].push(".");
    comp["triggerCharacters"].push(":");

    cap["definitionProvider"] = true;

    JsonValue &info = res["serverInfo"];
    info["name"] = "memelang-lsp";
    info["version"] = "1.0.0";

    JsonRpcTransport::writeMessage(out, resp.stringify());
}

void LspServer::handleShutdown(const JsonValue &id) {
    JsonValue resp;
    resp["jsonrpc"] = "2.0";
    resp["id"] = id;
    resp["result"] = JsonValue(); // null
    JsonRpcTransport::writeMessage(out, resp.stringify());
}

void LspServer::handleDidOpen(const JsonValue &params) {
    std::string uri = params["textDocument"]["uri"].asString();
    std::string text = params["textDocument"]["text"].asString();
    int version = static_cast<int>(params["textDocument"]["version"].asInt(1));
    analyzeDocument(uri, text, version);
}

void LspServer::handleDidChange(const JsonValue &params) {
    std::string uri = params["textDocument"]["uri"].asString();
    const JsonValue &changes = params["contentChanges"];
    std::string text = "";
    if (changes.isArray() && !changes.arrVal.empty()) {
        text = changes[0]["text"].asString();
    }
    int version = static_cast<int>(params["textDocument"]["version"].asInt(1));
    analyzeDocument(uri, text, version);
}

void LspServer::handleDidClose(const JsonValue &params) {
    std::string uri = params["textDocument"]["uri"].asString();
    documents.erase(uri);
    publishDiagnostics(uri, {});
}

void LspServer::analyzeDocument(const std::string &uri, const std::string &text, int version) {
    DocumentState state;
    state.uri = uri;
    state.content = text;
    state.version = version;

    try {
        Lexer lexer(text);
        std::vector<Token> tokens = lexer.tokenize();

        Parser parser(tokens);
        parser.enableDiagnosticMode();
        state.ast = parser.parseProgram();
        state.diagnostics = parser.getDiagnostics();

        if (state.diagnostics.empty()) {
            TypeChecker tc;
            tc.enableDiagnosticMode();
            tc.check(state.ast);
            for (const auto &d : tc.getDiagnostics()) {
                state.diagnostics.push_back(d);
            }
        }
    } catch (const std::exception &e) {
        Diagnostic diag;
        diag.range.startLine = 1;
        diag.range.startCol = 1;
        diag.range.endLine = 1;
        diag.range.endCol = 10;
        diag.message = e.what();
        diag.severity = Diagnostic::Error;
        state.diagnostics.push_back(diag);
    }

    publishDiagnostics(uri, state.diagnostics);
    documents[uri] = std::move(state);
}

void LspServer::publishDiagnostics(const std::string &uri, const std::vector<Diagnostic> &diags) {
    JsonValue notif;
    notif["jsonrpc"] = "2.0";
    notif["method"] = "textDocument/publishDiagnostics";

    JsonValue &params = notif["params"];
    params["uri"] = uri;
    JsonValue &arr = params["diagnostics"];
    arr.type = JsonType::Array;

    for (const auto &d : diags) {
        JsonValue diagItem;
        int sLine = std::max(0, d.range.startLine - 1);
        int sCol = std::max(0, d.range.startCol - 1);
        int eLine = std::max(0, d.range.endLine - 1);
        int eCol = std::max(0, d.range.endCol - 1);

        diagItem["range"]["start"]["line"] = sLine;
        diagItem["range"]["start"]["character"] = sCol;
        diagItem["range"]["end"]["line"] = eLine;
        diagItem["range"]["end"]["character"] = std::max(sCol + 1, eCol);
        diagItem["severity"] = static_cast<int>(d.severity);
        diagItem["message"] = d.message;
        diagItem["source"] = "memelang";

        arr.push(std::move(diagItem));
    }

    JsonRpcTransport::writeMessage(out, notif.stringify());
}

static std::string extractWordAt(const std::string &content, int lineIdx, int charIdx) {
    std::istringstream stream(content);
    std::string line;
    int curLine = 0;
    while (std::getline(stream, line)) {
        if (curLine == lineIdx) {
            if (charIdx < 0 || charIdx >= static_cast<int>(line.size())) {
                return "";
            }
            int start = charIdx;
            while (start > 0 && (std::isalnum(static_cast<unsigned char>(line[start - 1])) || line[start - 1] == '_')) {
                start--;
            }
            int end = charIdx;
            while (end < static_cast<int>(line.size()) && (std::isalnum(static_cast<unsigned char>(line[end])) || line[end] == '_')) {
                end++;
            }
            return line.substr(start, end - start);
        }
        curLine++;
    }
    return "";
}

std::string LspServer::getHoverMarkdown(const std::string &w) {
    static const std::unordered_map<std::string, std::pair<std::string, std::string>> HOVER_DOCS = {
        {"pakad", {"(keyword) pakad", "Declares a new variable with lexical scoping.\n\nExample: `pakad x = 10;`"}},
        {"le", {"(keyword) le", "Concise alias for variable declaration.\n\nExample: `le x = 10;`"}},
        {"let", {"(keyword) let", "Standard alias for variable declaration."}},
        {"var", {"(keyword) var", "Alias for variable declaration."}},
        {"jugaad", {"(keyword) jugaad", "Declares a function or method.\n\nExample:\n```meme\njugaad jod(a, b) {\n    ye_le a + b;\n}\n```"}},
        {"kaam", {"(keyword) kaam", "Function declaration alias.\n\nExample: `kaam jod(a, b) { ye_le a + b; }`"}},
        {"fn", {"(keyword) fn", "Concise function keyword."}},
        {"def", {"(keyword) def", "Pythonic function keyword."}},
        {"ye_le", {"(keyword) ye_le", "Returns a value from a function.\n\nExample: `ye_le 42;`"}},
        {"wapas", {"(keyword) wapas", "Return statement alias.\n\nExample: `wapas total;`"}},
        {"de", {"(keyword) de", "Return statement shorthand.\n\nExample: `de count;`"}},
        {"dekh", {"(keyword) dekh", "Conditional if statement.\n\nExample: `dekh x > 5 { ... }`"}},
        {"agar", {"(keyword) agar", "Conditional if statement alias.\n\nExample: `agar x > 5 { ... }`"}},
        {"warna", {"(keyword) warna", "Fallback else branch.\n\nExample: `dekh x { ... } warna { ... }`"}},
        {"jabtak", {"(keyword) jabtak", "While loop repeating while expression is true.\n\nExample: `jabtak i < 10 { ... }`"}},
        {"ghoom", {"(keyword) ghoom", "For loop over collections or numeric ranges.\n\nExample: `ghoom x in 0..10 { ... }`"}},
        {"khatam", {"(keyword) khatam", "Terminates loop execution (break) or drops resource.\n\nExample: `khatam;`"}},
        {"daayra", {"(keyword) daayra", "Continues to next loop iteration (continue) or lexical scope block.\n\nExample: `daayra;`"}},
        {"sancha", {"(keyword) sancha", "Defines a class blueprint.\n\nExample:\n```meme\nsancha Chai {\n    pakad temp;\n}\n```"}},
        {"biradari", {"(keyword) biradari", "Class declaration alias."}},
        {"vibe", {"(keyword) vibe", "Class declaration alias."}},
        {"class", {"(keyword) class", "Standard OOP class declaration keyword."}},
        {"dikha", {"(builtin) dikha(...args)", "Prints formatted arguments or expressions to standard output."}},
        {"bula", {"(keyword) bula", "Imports a standard or user module.\n\nExample: `bula \"json\";` or `bula \"file\" jaise fs;`"}},
        {"jaise", {"(keyword) jaise", "Alias specifier for module imports.\n\nExample: `bula \"file\" jaise fs;`"}},
        {"json", {"(module) json", "Zero-dependency RFC-compliant JSON engine.\n\nMethods:\n- `json.samajh(str)` / `json.parse(str)`\n- `json.likh(val)` / `json.stringify(val)`"}},
        {"csv", {"(module) csv", "Zero-dependency CSV processing engine.\n\nMethods:\n- `csv.padh(str)` / `csv.parse(str)`\n- `csv.likh(rows)` / `csv.stringify(rows)`"}},
        {"file", {"(module) file (alias: fs)", "File system I/O module.\n\nMethods:\n- `file.read(path)`\n- `file.write(path, data)`\n- `file.exists(path)`\n- `file.padh_json(path)`\n- `file.padh_csv(path)`"}},
        {"fs", {"(module) fs", "Alias for `file` system I/O module."}},
        {"env", {"(module) env", "Environment variables access.\n\nMethods:\n- `env.get(key, default)`\n- `env.set(key, val)`"}},
        {"sys", {"(module) sys", "System runtime utilities.\n\nProperties:\n- `sys.args`: Command line arguments list\n- `sys.exec(cmd)`: Execute shell command\n- `sys.exit(code)`: Terminate process"}}
    };

    auto it = HOVER_DOCS.find(w);
    if (it != HOVER_DOCS.end()) {
        return "**" + it->second.first + "**\n\n" + it->second.second;
    }
    return "";
}

void LspServer::handleHover(const JsonValue &id, const JsonValue &params) {
    std::string uri = params["textDocument"]["uri"].asString();
    int line = static_cast<int>(params["position"]["line"].asInt(0));
    int character = static_cast<int>(params["position"]["character"].asInt(0));

    std::string word;
    auto docIt = documents.find(uri);
    if (docIt != documents.end()) {
        word = extractWordAt(docIt->second.content, line, character);
    }

    std::string md = getHoverMarkdown(word);

    JsonValue resp;
    resp["jsonrpc"] = "2.0";
    resp["id"] = id;

    if (!md.empty()) {
        resp["result"]["contents"]["kind"] = "markdown";
        resp["result"]["contents"]["value"] = md;
    } else {
        resp["result"] = JsonValue(); // null
    }

    JsonRpcTransport::writeMessage(out, resp.stringify());
}

void LspServer::handleCompletion(const JsonValue &id, const JsonValue &/*params*/) {
    static const std::vector<std::pair<std::string, std::pair<int, std::string>>> COMPLETIONS = {
        {"pakad", {14, "Variable declaration"}},
        {"le", {14, "Variable declaration alias"}},
        {"jugaad", {3, "Function declaration"}},
        {"kaam", {3, "Function declaration alias"}},
        {"ye_le", {14, "Return statement"}},
        {"wapas", {14, "Return statement alias"}},
        {"dekh", {14, "If conditional statement"}},
        {"agar", {14, "If conditional alias"}},
        {"warna", {14, "Else conditional statement"}},
        {"jabtak", {14, "While loop"}},
        {"ghoom", {14, "For loop"}},
        {"khatam", {14, "Break loop or drop resource"}},
        {"daayra", {14, "Continue loop or scope block"}},
        {"sancha", {7, "Class declaration"}},
        {"biradari", {7, "Class declaration alias"}},
        {"dikha", {3, "Print expression"}},
        {"bula", {14, "Import module"}},
        {"jaise", {14, "Import alias"}},
        {"json", {9, "JSON module"}},
        {"csv", {9, "CSV module"}},
        {"file", {9, "File system module"}},
        {"fs", {9, "File system alias"}},
        {"env", {9, "Environment module"}},
        {"sys", {9, "System module"}},
        {"point_wala", {3, "Float conversion / formatting"}},
        {"bina_point", {3, "Integer conversion"}}
    };

    JsonValue resp;
    resp["jsonrpc"] = "2.0";
    resp["id"] = id;
    JsonValue &items = resp["result"];
    items.type = JsonType::Array;

    for (const auto &item : COMPLETIONS) {
        JsonValue c;
        c["label"] = item.first;
        c["kind"] = item.second.first;
        c["detail"] = item.second.second;
        items.push(std::move(c));
    }

    JsonRpcTransport::writeMessage(out, resp.stringify());
}

static bool findDefInStmt(const Stmt *stmt, const std::string &target, SourceRange &outRange) {
    if (!stmt) return false;
    if (const auto *v = dynamic_cast<const VarDeclStmt *>(stmt)) {
        if (v->name == target) {
            outRange = v->range;
            return true;
        }
    } else if (const auto *f = dynamic_cast<const FuncDeclStmt *>(stmt)) {
        if (f->name == target) {
            outRange = f->range;
            return true;
        }
    } else if (const auto *c = dynamic_cast<const ClassDeclStmt *>(stmt)) {
        if (c->name == target) {
            outRange = c->range;
            return true;
        }
    }
    return false;
}

void LspServer::handleDefinition(const JsonValue &id, const JsonValue &params) {
    std::string uri = params["textDocument"]["uri"].asString();
    int line = static_cast<int>(params["position"]["line"].asInt(0));
    int character = static_cast<int>(params["position"]["character"].asInt(0));

    std::string word;
    auto docIt = documents.find(uri);
    if (docIt != documents.end()) {
        word = extractWordAt(docIt->second.content, line, character);
    }

    JsonValue resp;
    resp["jsonrpc"] = "2.0";
    resp["id"] = id;

    bool found = false;
    SourceRange defRange;
    if (docIt != documents.end() && !word.empty()) {
        for (const auto &stmt : docIt->second.ast) {
            if (findDefInStmt(stmt.get(), word, defRange)) {
                found = true;
                break;
            }
        }
    }

    if (found) {
        int sLine = std::max(0, defRange.startLine - 1);
        int sCol = std::max(0, defRange.startCol - 1);
        int eLine = std::max(0, defRange.endLine - 1);
        int eCol = std::max(sCol + static_cast<int>(word.size()), defRange.endCol - 1);

        JsonValue &res = resp["result"];
        res["uri"] = uri;
        res["range"]["start"]["line"] = sLine;
        res["range"]["start"]["character"] = sCol;
        res["range"]["end"]["line"] = eLine;
        res["range"]["end"]["character"] = eCol;
    } else {
        resp["result"] = JsonValue(); // null
    }

    JsonRpcTransport::writeMessage(out, resp.stringify());
}

} // namespace memelang::lsp
