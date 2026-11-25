#pragma once

#include <string>
#include <mutex>
#include <atomic>
#include <functional>

class Interpreter;
namespace MemeUI { class UiSession; }

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
    MemeUI::UiSession *uiSession = nullptr;
    std::function<void()> onRenderFrame = nullptr;
    int viewportWidth = 1040;
    int viewportHeight = 720;
    std::atomic<bool> running{true};
    int serverFd = -1;
};
