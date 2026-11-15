#include "DapServer.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include "../lexer/Lexer.h"
#include "../parser/Parser.h"
#include "../types/TypeChecker.h"

namespace memelang::dap {

DapServer::DapServer(std::istream &inStream, std::ostream &outStream)
    : in(inStream), out(outStream) {}

DapServer::~DapServer() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        terminateRequested = true;
        isPaused = false;
        isRunning = false;
    }
    cv.notify_all();
    if (execThread.joinable()) {
        execThread.join();
    }
}

void DapServer::sendResponse(int requestSeq, const std::string &command, bool success, const lsp::JsonValue &body, const std::string &errMsg) {
    lsp::JsonValue resp;
    resp["seq"] = seqCounter++;
    resp["type"] = "response";
    resp["request_seq"] = requestSeq;
    resp["command"] = command;
    resp["success"] = success;
    if (!success && !errMsg.empty()) {
        resp["message"] = errMsg;
    }
    if (!body.isNull()) {
        resp["body"] = body;
    }
    lsp::JsonRpcTransport::writeMessage(out, resp.stringify());
}

void DapServer::sendEvent(const std::string &event, const lsp::JsonValue &body) {
    lsp::JsonValue ev;
    ev["seq"] = seqCounter++;
    ev["type"] = "event";
    ev["event"] = event;
    if (!body.isNull()) {
        ev["body"] = body;
    }
    lsp::JsonRpcTransport::writeMessage(out, ev.stringify());
}

void DapServer::run() {
    std::string payload;
    while (isRunning && lsp::JsonRpcTransport::readMessage(in, payload)) {
        if (!payload.empty()) {
            handleMessage(payload);
        }
    }
}

void DapServer::handleMessage(const std::string &jsonStr) {
    lsp::JsonValue req = lsp::JsonValue::parse(jsonStr);
    std::string type = req["type"].asString();
    if (type != "request") return;

    std::string command = req["command"].asString();
    int reqSeq = static_cast<int>(req["seq"].asInt());
    const lsp::JsonValue &args = req["arguments"];

    if (command == "initialize") {
        handleInitialize(reqSeq, args);
    } else if (command == "launch") {
        handleLaunch(reqSeq, args);
    } else if (command == "setBreakpoints") {
        handleSetBreakpoints(reqSeq, args);
    } else if (command == "configurationDone") {
        handleConfigurationDone(reqSeq, args);
    } else if (command == "threads") {
        handleThreads(reqSeq, args);
    } else if (command == "stackTrace") {
        handleStackTrace(reqSeq, args);
    } else if (command == "scopes") {
        handleScopes(reqSeq, args);
    } else if (command == "variables") {
        handleVariables(reqSeq, args);
    } else if (command == "continue") {
        handleContinue(reqSeq, args);
    } else if (command == "next") {
        handleNext(reqSeq, args);
    } else if (command == "stepIn") {
        handleStepIn(reqSeq, args);
    } else if (command == "disconnect") {
        handleDisconnect(reqSeq, args);
    } else {
        sendResponse(reqSeq, command, true);
    }
}

void DapServer::handleInitialize(int reqSeq, const lsp::JsonValue &) {
    lsp::JsonValue body;
    body["supportsConfigurationDoneRequest"] = true;
    body["supportsFunctionBreakpoints"] = false;
    body["supportsConditionalBreakpoints"] = false;
    body["supportsEvaluateForHovers"] = true;
    body["supportsStepBack"] = false;
    sendResponse(reqSeq, "initialize", true, body);
    sendEvent("initialized");
}

void DapServer::handleLaunch(int reqSeq, const lsp::JsonValue &args) {
    currentProgram = args["program"].asString();
    stopOnEntry = args["stopOnEntry"].asBool(false);

    if (currentProgram.empty() || !std::filesystem::exists(currentProgram)) {
        sendResponse(reqSeq, "launch", false, lsp::JsonValue(), "File not found: " + currentProgram);
        return;
    }

    sendResponse(reqSeq, "launch", true);
}

void DapServer::handleSetBreakpoints(int reqSeq, const lsp::JsonValue &args) {
    std::string path = args["source"]["path"].asString();
    std::unordered_set<int> lines;

    lsp::JsonValue body;
    body["breakpoints"].type = lsp::JsonType::Array;

    if (args["breakpoints"].isArray()) {
        for (const auto &bp : args["breakpoints"].arrVal) {
            int line = static_cast<int>(bp["line"].asInt());
            lines.insert(line);
            lsp::JsonValue resBp;
            resBp["verified"] = true;
            resBp["line"] = line;
            body["breakpoints"].arrVal.push_back(resBp);
        }
    } else if (args["lines"].isArray()) {
        for (const auto &l : args["lines"].arrVal) {
            int line = static_cast<int>(l.asInt());
            lines.insert(line);
            lsp::JsonValue resBp;
            resBp["verified"] = true;
            resBp["line"] = line;
            body["breakpoints"].arrVal.push_back(resBp);
        }
    }

    breakpoints[path] = lines;
    std::string absPath = std::filesystem::absolute(path).string();
    breakpoints[absPath] = lines;

    sendResponse(reqSeq, "setBreakpoints", true, body);
}

void DapServer::handleConfigurationDone(int reqSeq, const lsp::JsonValue &) {
    sendResponse(reqSeq, "configurationDone", true);

    interpreter = std::make_unique<Interpreter>();
    interpreter->setCurrentFilePath(currentProgram);

    interpreter->setStepHook([this](Interpreter &interp, int line, const Stmt *) {
        std::string path = interp.getCurrentFilePath();
        bool hit = false;
        if (stopOnEntry) {
            hit = true;
            stopOnEntry = false;
            stopReason = "entry";
        } else if (stepRequested) {
            hit = true;
            stepRequested = false;
            stopReason = "step";
        } else {
            auto it = breakpoints.find(path);
            if (it != breakpoints.end() && it->second.count(line)) {
                hit = true;
                stopReason = "breakpoint";
            } else {
                std::string fname = std::filesystem::path(path).filename().string();
                for (const auto &[bpPath, lines] : breakpoints) {
                    if (std::filesystem::path(bpPath).filename().string() == fname && lines.count(line)) {
                        hit = true;
                        stopReason = "breakpoint";
                        break;
                    }
                }
            }
        }

        if (hit) {
            std::unique_lock<std::mutex> lock(mtx);
            isPaused = true;
            lsp::JsonValue evBody;
            evBody["reason"] = stopReason;
            evBody["threadId"] = 1;
            sendEvent("stopped", evBody);

            cv.wait(lock, [this]() { return !isPaused || terminateRequested; });
            if (terminateRequested) {
                throw std::runtime_error("DAP Debugger Terminated");
            }
        }
    });

    execThread = std::thread([this]() {
        try {
            std::ifstream file(currentProgram);
            std::stringstream ss;
            ss << file.rdbuf();
            std::string source = ss.str();

            Lexer lexer(source);
            std::vector<Token> tokens = lexer.tokenize();

            Parser parser(tokens);
            std::vector<std::unique_ptr<Stmt>> program = parser.parseProgram();

            TypeChecker typeChecker;
            typeChecker.check(program);

            interpreter->interpret(program);
            programExitCode = 0;
        } catch (const std::exception &e) {
            programExitCode = 1;
        }
        programFinished = true;

        lsp::JsonValue exitBody;
        exitBody["exitCode"] = programExitCode;
        sendEvent("exited", exitBody);
        sendEvent("terminated");
    });
}

void DapServer::handleThreads(int reqSeq, const lsp::JsonValue &) {
    lsp::JsonValue body;
    body["threads"].type = lsp::JsonType::Array;
    lsp::JsonValue mainThread;
    mainThread["id"] = 1;
    mainThread["name"] = "main";
    body["threads"].arrVal.push_back(mainThread);
    sendResponse(reqSeq, "threads", true, body);
}

void DapServer::handleStackTrace(int reqSeq, const lsp::JsonValue &) {
    lsp::JsonValue body;
    body["stackFrames"].type = lsp::JsonType::Array;

    int currentLine = interpreter ? interpreter->getCurrentLine() : 1;
    std::string currentFile = interpreter ? interpreter->getCurrentFilePath() : currentProgram;

    lsp::JsonValue topFrame;
    topFrame["id"] = 1;
    topFrame["name"] = "<main>";
    topFrame["line"] = currentLine;
    topFrame["column"] = 1;
    topFrame["source"]["name"] = std::filesystem::path(currentFile).filename().string();
    topFrame["source"]["path"] = currentFile;

    if (interpreter && !interpreter->getCallStack().empty()) {
        const auto &stack = interpreter->getCallStack();
        topFrame["name"] = stack.back().functionName;
        body["stackFrames"].arrVal.push_back(topFrame);
        for (int i = static_cast<int>(stack.size()) - 2; i >= 0; --i) {
            lsp::JsonValue frame;
            frame["id"] = 2 + (static_cast<int>(stack.size()) - 2 - i);
            frame["name"] = stack[i].functionName;
            frame["line"] = stack[i].line;
            frame["column"] = 1;
            frame["source"]["name"] = std::filesystem::path(stack[i].filePath).filename().string();
            frame["source"]["path"] = stack[i].filePath;
            body["stackFrames"].arrVal.push_back(frame);
        }
    } else {
        body["stackFrames"].arrVal.push_back(topFrame);
    }

    body["totalFrames"] = static_cast<int>(body["stackFrames"].arrVal.size());
    sendResponse(reqSeq, "stackTrace", true, body);
}

void DapServer::handleScopes(int reqSeq, const lsp::JsonValue &) {
    lsp::JsonValue body;
    body["scopes"].type = lsp::JsonType::Array;

    lsp::JsonValue localScope;
    localScope["name"] = "Locals";
    localScope["variablesReference"] = 1;
    localScope["expensive"] = false;
    body["scopes"].arrVal.push_back(localScope);

    sendResponse(reqSeq, "scopes", true, body);
}

void DapServer::handleVariables(int reqSeq, const lsp::JsonValue &) {
    lsp::JsonValue body;
    body["variables"].type = lsp::JsonType::Array;

    if (interpreter) {
        auto env = interpreter->getEnvironment();
        if (env) {
            const auto &slots = env->getSlots();
            for (const auto &[name, slot] : slots) {
                lsp::JsonValue var;
                var["name"] = name;
                var["value"] = slot.value.toString();
                var["type"] = slot.value.typeName();
                var["variablesReference"] = 0;
                body["variables"].arrVal.push_back(var);
            }
        }
    }

    sendResponse(reqSeq, "variables", true, body);
}

void DapServer::handleContinue(int reqSeq, const lsp::JsonValue &) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        isPaused = false;
    }
    cv.notify_all();
    sendResponse(reqSeq, "continue", true);
}

void DapServer::handleNext(int reqSeq, const lsp::JsonValue &) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        stepRequested = true;
        isPaused = false;
    }
    cv.notify_all();
    sendResponse(reqSeq, "next", true);
}

void DapServer::handleStepIn(int reqSeq, const lsp::JsonValue &) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        stepRequested = true;
        isPaused = false;
    }
    cv.notify_all();
    sendResponse(reqSeq, "stepIn", true);
}

void DapServer::handleDisconnect(int reqSeq, const lsp::JsonValue &) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        terminateRequested = true;
        isPaused = false;
        isRunning = false;
    }
    cv.notify_all();
    sendResponse(reqSeq, "disconnect", true);
}

} // namespace memelang::dap
