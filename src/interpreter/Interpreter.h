#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "Value.h"
#include "Environment.h"
#include "Callable.h"
#include "Function.h"
#include "Class.h"
#include "Operators.h"
#include "Signals.h"
#include "../parser/Parser.h"
#include "../ui/TailwindEngine.h"

class Interpreter
{
private:
    std::shared_ptr<Environment> globals;
    std::shared_ptr<Environment> environment;
    std::ostream &out;
    std::istream &in;
    std::function<std::string(const std::string &)> inputPromptCallback;

    int currentScopeDepth = 0;
    uint64_t nextBorrowId = 1;

    struct ActiveBorrowInfo
    {
        uint64_t borrowId;
        std::shared_ptr<Environment> env;
        std::string varName;
        bool isMutable;
        int scopeDepth;
    };
    std::vector<ActiveBorrowInfo> activeBorrows;

public:
    int currentLine = 1;
    std::string currentFilePath;
    std::string lastGpuCommandsJson;
    std::string lastHitRegionsJson = "[]";
    std::string lastTerminalUi;
    std::string lastHtml;
    std::unordered_map<std::string, Value> importedModules;
    std::vector<std::string> importStack;
    std::vector<std::vector<std::unique_ptr<Stmt>>> importedAsts;
    std::vector<std::vector<std::unique_ptr<Stmt>>> astStorage;

    // React Runtime & Fiber State
    Value rootUiComponent = Value(std::monostate{});
    Value rootCallable = Value(std::monostate{});
    std::unordered_map<std::string, Value> hookSlots;
    std::vector<std::string> componentPathStack;
    std::unordered_map<std::string, int> componentInstanceCounters;
    std::unordered_map<std::string, int> componentHookCursors;
    std::unordered_map<int, TailwindEngine::HitRegion> interactiveRegions;
    std::unordered_map<std::string, std::vector<Value>> contextStacks;
    std::string currentRouterPath = "/";
    std::vector<std::string> routerHistory;
    int nextContextId = 0;

    std::vector<std::string> commandLineArgs;
    bool isHeadless = false;
    bool isInteractiveCLI = false;

    Interpreter(std::ostream &out = std::cout, std::istream &in = std::cin);

    void setCommandLineArgs(const std::vector<std::string> &args);
    const std::vector<std::string> &getCommandLineArgs() const { return commandLineArgs; }
    void setHeadless(bool h) { isHeadless = h; }
    bool getHeadless() const { return isHeadless; }
    void setInteractiveCLI(bool i) { isInteractiveCLI = i; }
    bool getInteractiveCLI() const { return isInteractiveCLI; }

    void setInputPromptCallback(std::function<std::string(const std::string &)> cb)
    {
        inputPromptCallback = cb;
    }

    void setCurrentFilePath(const std::string &path) { currentFilePath = path; }
    const std::string &getCurrentFilePath() const { return currentFilePath; }

    const std::string &getLastGpuCommandsJson() const { return lastGpuCommandsJson; }
    void setLastGpuCommandsJson(const std::string &json) { lastGpuCommandsJson = json; }

    const std::string &getLastHitRegionsJson() const { return lastHitRegionsJson; }
    void setLastHitRegionsJson(const std::string &json) { lastHitRegionsJson = json; }

    const std::string &getLastTerminalUi() const { return lastTerminalUi; }
    void setLastTerminalUi(const std::string &t) { lastTerminalUi = t; }

    const std::string &getLastHtml() const { return lastHtml; }
    void setLastHtml(const std::string &h) { lastHtml = h; }

    void setRootUiComponent(const Value &c) { rootUiComponent = c; }
    const Value &getRootUiComponent() const { return rootUiComponent; }

    void pushComponentCall(const std::string &name);
    void popComponentCall();
    std::string getCurrentComponentPath() const;
    int getNextHookCursor(const std::string &path);

    void pushContext(const std::string &ctxId, const Value &val);
    void popContext(const std::string &ctxId);
    Value getContext(const std::string &ctxId, const Value &defaultVal);

    void setRouterPath(const std::string &path);
    const std::string &getRouterPath() const { return currentRouterPath; }

    void triggerUiRerender();
    bool dispatchUiEvent(int nodeId, const std::string &eventType, const std::string &value = "");

    Value importModule(const std::string &path, int line);

    void enterScope() { currentScopeDepth++; }
    int getCurrentScopeDepth() const { return currentScopeDepth; }
    void cleanupScope(std::shared_ptr<Environment> env, int scopeDepth);
    void dropInstance(std::shared_ptr<Instance> instance);
    Value cloneValue(const Value &val);
    Value moveVariable(const std::string &varName);

    void interpret(const std::vector<std::unique_ptr<Stmt>> &statements);
    void interpret(std::vector<std::unique_ptr<Stmt>> &&statements);
    Value evaluate(const Expr *expr);
    Value evaluateInEnv(const Expr *expr, std::shared_ptr<Environment> env);
    std::string stringify(const Value &val);
    void execute(const Stmt *stmt);
    void executeInEnv(const Stmt *stmt, std::shared_ptr<Environment> targetEnv);
    void executeBlock(const std::vector<std::unique_ptr<Stmt>> &statements, std::shared_ptr<Environment> newEnv);

    std::shared_ptr<Environment> getGlobals() { return globals; }
    std::shared_ptr<Environment> getEnvironment() { return environment; }
    std::ostream &getOutputStream() { return out; }
    std::istream &getInputStream() { return in; }
};

std::string stripAnsi(const std::string &str);
size_t visibleLength(const std::string &str);
std::string formatRichText(const std::string &input, bool useAnsi = true);
std::string renderUiScreen(const Value &screen, const std::string &title = "");

#endif
