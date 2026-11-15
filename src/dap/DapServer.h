#ifndef MEMELANG_DAPSERVER_H
#define MEMELANG_DAPSERVER_H

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>
#include "../interpreter/Interpreter.h"
#include "../lsp/JsonRpc.h"

namespace memelang::dap {

class DapServer {
private:
    std::istream &in;
    std::ostream &out;
    bool isRunning = true;
    int seqCounter = 1;

    std::unordered_map<std::string, std::unordered_set<int>> breakpoints; // file -> lines
    std::mutex mtx;
    std::condition_variable cv;
    bool isPaused = false;
    bool stepRequested = false;
    bool terminateRequested = false;
    std::string stopReason = "breakpoint";

    std::unique_ptr<Interpreter> interpreter;
    std::thread execThread;
    std::string currentProgram;
    bool stopOnEntry = false;
    std::atomic<bool> programFinished{false};
    int programExitCode = 0;

    void sendResponse(int requestSeq, const std::string &command, bool success, const lsp::JsonValue &body = lsp::JsonValue(), const std::string &errMsg = "");
    void sendEvent(const std::string &event, const lsp::JsonValue &body = lsp::JsonValue());

    void handleMessage(const std::string &jsonStr);
    void handleInitialize(int reqSeq, const lsp::JsonValue &args);
    void handleLaunch(int reqSeq, const lsp::JsonValue &args);
    void handleSetBreakpoints(int reqSeq, const lsp::JsonValue &args);
    void handleConfigurationDone(int reqSeq, const lsp::JsonValue &args);
    void handleThreads(int reqSeq, const lsp::JsonValue &args);
    void handleStackTrace(int reqSeq, const lsp::JsonValue &args);
    void handleScopes(int reqSeq, const lsp::JsonValue &args);
    void handleVariables(int reqSeq, const lsp::JsonValue &args);
    void handleContinue(int reqSeq, const lsp::JsonValue &args);
    void handleNext(int reqSeq, const lsp::JsonValue &args);
    void handleStepIn(int reqSeq, const lsp::JsonValue &args);
    void handleDisconnect(int reqSeq, const lsp::JsonValue &args);

public:
    DapServer(std::istream &inStream = std::cin, std::ostream &outStream = std::cout);
    ~DapServer();
    void run();
};

} // namespace memelang::dap

#endif // MEMELANG_DAPSERVER_H
