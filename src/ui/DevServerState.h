#pragma once

#include <string>
#include <mutex>
#include <atomic>

class Interpreter;

struct DevServerState {
    std::mutex mtx;
    std::string appName = "MEMElang App";
    std::string targetFile = "";
    std::string platform = "web";
    int port = 3000;
    int version = 1;
    bool showTerminal = true;
    std::string gpuCommands = "[]";
    std::string hitRegions = "[]";
    std::string terminalUi = "";
    std::string html = "";
    Interpreter *interpreter = nullptr;
    std::atomic<bool> running{true};
    int serverFd = -1;
};
