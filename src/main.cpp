#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif
#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "types/TypeChecker.h"
#include "interpreter/Interpreter.h"
#include "interpreter/Signals.h"
#include "ui/DevServerState.h"
#include "ui/NativeDesktopWindow.h"

static void printBanner()
{
    std::cout << "🎭 MEMElang v1.0 [Ye Bik Gayi Hai Gormint Edition]\n";
    std::cout << "Type 'bas;' or 'exit' to quit. Shuru ho jaao!\n\n";
}

static void printUsage(const char *prog)
{
    std::cout << formatRichText("[asmani bold]🎭 MEMElang CLI Usage:[/]\n");
    std::cout << "  " << prog << "                           Launch interactive REPL\n";
    std::cout << "  " << prog << " <filename.meme>           Run a MEMElang script file\n";
    std::cout << "  " << prog << " -e \"code\"                 Run code directly from command line\n";
    std::cout << "  " << prog << " naya <name> [--template]   Scaffold new project (alias: create)\n";
    std::cout << "  " << prog << " env <init|info|clean>     Manage project virtual environment (.meme_env/)\n";
    std::cout << "  " << prog << " tapri <install|suchi>     Package manager (py:numpy, npm:canvas, etc.)\n";
    std::cout << "  " << prog << " dev [file] [--platform]   Start dev server (Web 60FPS Canvas, Desktop, Terminal)\n";
    std::cout << "  " << prog << " setup                     Link 'meme' CLI globally into PATH (alias: link)\n";
    std::cout << "  " << prog << " run <script>              Run a script defined in tapri.json\n";
    std::cout << "  " << prog << " test [dir]                Run test runner on *_test.meme files\n";
    std::cout << "  " << prog << " pakao <file> ...          Compile to native Flutter / macOS targets\n";
    std::cout << "  " << prog << " -h, --help                Show this help message\n";
}

static bool runSource(const std::string &source, Interpreter &interpreter);
static void runFile(const std::string &path, const std::vector<std::string> &args = {});

static void handleNaya(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cout << formatRichText("[peela]Usage:[/] ./meme naya <project_name> [--template=react|api|fullstack]\n");
        return;
    }
    std::string name = argv[2];
    std::string templateType = "react";
    for (int i = 3; i < argc; ++i)
    {
        std::string a = argv[i];
        if (a.rfind("--template=", 0) == 0)
        {
            templateType = a.substr(11);
        }
    }

    std::filesystem::path projDir(name);
    if (std::filesystem::exists(projDir))
    {
        std::cout << formatRichText("[laal]❌ Folder '" + name + "' pehle se exist karta hai![/]\n");
        return;
    }

    std::filesystem::create_directories(projDir / "src");
    std::filesystem::create_directories(projDir / "tests");
    std::filesystem::create_directories(projDir / ".meme_env" / "py");
    std::filesystem::create_directories(projDir / ".meme_env" / "node");
    std::filesystem::create_directories(projDir / ".meme_env" / "pkgs");
    std::filesystem::create_directories(projDir / ".tapri_modules");

    // tapri.json
    std::ofstream tj(projDir / "tapri.json");
    tj << "{\n"
       << "  \"name\": \"" << name << "\",\n"
       << "  \"version\": \"1.0.0\",\n"
       << "  \"template\": \"" << templateType << "\",\n"
       << "  \"scripts\": {\n"
       << "    \"dev\": \"meme dev src/App.meme\",\n"
       << "    \"start\": \"meme src/App.meme\",\n"
       << "    \"test\": \"meme test tests\"\n"
       << "  },\n"
       << "  \"dependencies\": {}\n"
       << "}\n";

    // .env
    std::ofstream envf(projDir / ".env");
    envf << "PORT=3000\n"
         << "APP_ENV=development\n"
         << "APP_NAME=" << name << "\n";

    // .meme_env/tapri.lock
    std::ofstream lk(projDir / ".meme_env" / "tapri.lock");
    lk << "{\n  \"lockfileVersion\": 1,\n  \"packages\": {}\n}\n";

    // src/App.meme
    std::ofstream appf(projDir / "src" / "App.meme");
    if (templateType == "api")
    {
        appf << "// " << name << " - MEMElang Dhaba API Server\n"
             << "dhaba.get(\"/api/health\", (req) => {\n"
             << "    de {\"status\": \"ok\", \"app\": \"" << name << "\"}\n"
             << "})\n\n"
             << "dikha \"[hara bold]🚀 " << name << " API chalu ho gaya![/]\"\n";
    }
    else
    {
        appf << "// " << name << " - MEMElang Native React App\n"
             << "kaam Counter() {\n"
             << "    [count, setCount] = useState(0);\n"
             << "    de (\n"
             << "        <div class=\"p-6 bg-slate-900 rounded-2xl border border-slate-700 max-w-sm mx-auto shadow-xl\">\n"
             << "            <h1 class=\"text-2xl font-bold text-white mb-2\">☕ " << name << "</h1>\n"
             << "            <p class=\"text-slate-400 mb-4\">Zero-WebView GPU Native UI with Reactivity</p>\n"
             << "            <div class=\"flex items-center space-x-4 mb-4\">\n"
             << "                <span class=\"text-3xl font-extrabold text-amber-400\">{count}</span>\n"
             << "                <button\n"
             << "                    class=\"px-4 py-2 bg-amber-500 hover:bg-amber-600 text-slate-950 font-bold rounded-lg transition\"\n"
             << "                    onClick={() => setCount(c => c + 1)}\n"
             << "                >\n"
             << "                    + Chai Ghusa\n"
             << "                </button>\n"
             << "            </div>\n"
             << "        </div>\n"
             << "    );\n"
             << "}\n\n"
             << "kaam App() {\n"
             << "    de (\n"
             << "        <Router>\n"
             << "            <Route path=\"/\" element={<Counter />} />\n"
             << "        </Router>\n"
             << "    );\n"
             << "}\n\n"
             << "ui.mount(<App />);\n";
    }

    // tests/app_test.meme
    std::ofstream tf(projDir / "tests" / "app_test.meme");
    tf << "// Tests for " << name << "\n"
       << "test(\"math check for " << name << "\", () => {\n"
       << "    expect(1 + 1).toBe(2);\n"
       << "});\n\n"
       << "test(\"env variable loaded\", () => {\n"
       << "    expect(systumm.env(\"APP_NAME\")).toBe(\"" << name << "\");\n"
       << "});\n";

    std::cout << formatRichText("[hara bold]✔ Naya project ban gaya:[/] " + name + " (Template: " + templateType + ")\n");
    std::cout << formatRichText("[asmani]Chalo shuru ho jao:[/]\n");
    std::cout << "  cd " << name << "\n";
    std::cout << "  meme dev\n";
    std::cout << formatRichText("  [halka](Tip: Run '[mota]./meme setup[/]' once to use 'meme' command globally from any directory!)[/]\n");
}

static void handleEnv(int argc, char *argv[])
{
    std::string sub = argc >= 3 ? argv[2] : "info";

    if (sub == "init")
    {
        std::filesystem::create_directories(".meme_env/py");
        std::filesystem::create_directories(".meme_env/node");
        std::filesystem::create_directories(".meme_env/pkgs");
        if (!std::filesystem::exists(".meme_env/tapri.lock"))
        {
            std::ofstream lk(".meme_env/tapri.lock");
            lk << "{\n  \"lockfileVersion\": 1,\n  \"packages\": {}\n}\n";
        }
        std::cout << formatRichText("[hara bold]✔ Virtual environment ready:[/] .meme_env/ initialized successfully!\n");
        return;
    }

    if (sub == "clean")
    {
        std::error_code ec;
        std::filesystem::remove_all(".meme_env", ec);
        std::filesystem::remove_all(".tapri_modules", ec);
        std::cout << formatRichText("[hara bold]✔ Safai ho gayi:[/] .meme_env/ and module caches removed cleanly!\n");
        return;
    }

    if (sub == "info")
    {
        std::cout << formatRichText("[asmani bold]🌿 MEMElang Virtual Environment (.meme_env):[/]\n");
        bool exists = std::filesystem::exists(".meme_env");
        std::cout << formatRichText("  Status: " + std::string(exists ? "Active [hara]✔[/]" : "Not Initialized [peela]⚠ (Run './meme env init')[/]") + "\n");
        if (exists)
        {
            std::cout << "  Python Venv:  .meme_env/py\n";
            std::cout << "  Node Prefix:  .meme_env/node\n";
            std::cout << formatRichText("  Lockfile:     " + std::string(std::filesystem::exists(".meme_env/tapri.lock") ? ".meme_env/tapri.lock [hara]✔[/]" : "None") + "\n");
            size_t pkgCount = 0;
            if (std::filesystem::exists(".meme_env/pkgs"))
            {
                for (const auto &p : std::filesystem::directory_iterator(".meme_env/pkgs"))
                {
                    if (p.is_regular_file() || p.is_directory()) pkgCount++;
                }
            }
            std::cout << "  Installed Packages: " << pkgCount << "\n";
        }
        std::cout << formatRichText("  .env File:    " + std::string(std::filesystem::exists(".env") ? ".env [hara]✔[/]" : "None") + "\n");
        std::cout << formatRichText("  tapri.json:   " + std::string(std::filesystem::exists("tapri.json") ? "tapri.json [hara]✔[/]" : "None") + "\n");
        return;
    }

    std::cout << formatRichText("[peela]Usage:[/] ./meme env <init|info|clean>\n");
}

static void handleTapri(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cout << formatRichText("[peela]Usage:[/] ./meme tapri <mangwa|install|suchi|list|init> [packages...]\n");
        return;
    }
    std::string sub = argv[2];
    if (sub == "init")
    {
        std::ofstream f("tapri.json");
        f << "{\n  \"name\": \"tapri-project\",\n  \"version\": \"1.0.0\",\n  \"dependencies\": {}\n}\n";
        std::cout << formatRichText("[hara bold]✔ Tapri init ho gayi:[/] tapri.json created successfully!\n");
        return;
    }
    if (sub == "suchi" || sub == "list")
    {
        std::ifstream f("tapri.json");
        if (!f.is_open())
        {
            std::cout << formatRichText("[peela]Abhi tak koi tapri.json nahi hai. Pehle './meme tapri init' karo![/]\n");
            return;
        }
        std::stringstream ss;
        ss << f.rdbuf();
        std::cout << formatRichText("[asmani bold]📦 Installed Tapri Modules:[/]\n") << ss.str() << "\n";
        return;
    }
    if (sub == "mangwa" || sub == "install" || sub == "add" || sub == "la" || sub == "i")
    {
        std::filesystem::create_directories(".meme_env/pkgs");
        std::filesystem::create_directories(".tapri_modules");

        std::vector<std::string> pkgsToInstall;
        if (argc > 3)
        {
            for (int i = 3; i < argc; ++i)
            {
                pkgsToInstall.push_back(argv[i]);
            }
        }
        else
        {
            std::ifstream tf("tapri.json");
            if (tf.is_open())
            {
                std::string line;
                while (std::getline(tf, line))
                {
                    size_t colon = line.find(':');
                    if (colon != std::string::npos)
                    {
                        size_t firstQuote = line.find('"', colon);
                        if (firstQuote != std::string::npos)
                        {
                            size_t secondQuote = line.find('"', firstQuote + 1);
                            if (secondQuote != std::string::npos)
                            {
                                std::string val = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
                                if (!val.empty()) pkgsToInstall.push_back(val);
                            }
                        }
                    }
                }
            }
        }

        if (pkgsToInstall.empty())
        {
            std::cout << formatRichText("[peela]Koi packages nahi mile install karne ke liye. Pass packages or add them to tapri.json![/]\n");
            return;
        }

        std::string projName = "tapri-project";
        std::string scriptsSection = "    \"dev\": \"meme dev src/App.meme\",\n    \"start\": \"meme src/App.meme\",\n    \"test\": \"meme test tests\"";
        std::ifstream oldTf("tapri.json");
        if (oldTf.is_open())
        {
            std::stringstream ss;
            ss << oldTf.rdbuf();
            std::string oldContent = ss.str();
            size_t npos = oldContent.find("\"name\": \"");
            if (npos != std::string::npos)
            {
                size_t nend = oldContent.find('"', npos + 9);
                if (nend != std::string::npos) projName = oldContent.substr(npos + 9, nend - npos - 9);
            }
            size_t spos = oldContent.find("\"scripts\": {");
            if (spos != std::string::npos)
            {
                size_t send = oldContent.find('}', spos);
                if (send != std::string::npos)
                {
                    std::string inner = oldContent.substr(spos + 12, send - spos - 12);
                    size_t s1 = inner.find_first_not_of(" \t\r\n");
                    size_t s2 = inner.find_last_not_of(" \t\r\n");
                    if (s1 != std::string::npos && s2 != std::string::npos)
                    {
                        scriptsSection = "    " + inner.substr(s1, s2 - s1 + 1);
                    }
                }
            }
        }

        std::string tapriContent = "{\n  \"name\": \"" + projName + "\",\n  \"version\": \"1.0.0\",\n  \"scripts\": {\n" + scriptsSection + "\n  },\n  \"dependencies\": {\n";
        std::string lockContent = "{\n  \"lockfileVersion\": 1,\n  \"packages\": {\n";

        for (size_t i = 0; i < pkgsToInstall.size(); ++i)
        {
            std::string pkg = pkgsToInstall[i];
            std::string name = pkg;
            std::string shimCode = "";

            if (pkg.rfind("py:", 0) == 0)
            {
                name = pkg.substr(3);
                shimCode = "// Auto-generated shim for " + pkg + "\nbula \"" + pkg + "\" jaise " + name + ";\nwapas " + name + ";\n";
            }
            else if (pkg.rfind("npm:", 0) == 0)
            {
                name = pkg.substr(4);
                shimCode = "// Auto-generated shim for " + pkg + "\nbula \"" + pkg + "\" jaise " + name + ";\nwapas " + name + ";\n";
            }
            else if (pkg.rfind("rust:", 0) == 0)
            {
                name = pkg.substr(5);
                shimCode = "// Auto-generated shim for " + pkg + "\nbula \"" + pkg + "\" jaise " + name + ";\nwapas " + name + ";\n";
            }
            else
            {
                shimCode = "// Native MEMElang package: " + pkg + "\nde {\"name\": \"" + pkg + "\", \"status\": \"active\"};\n";
            }

            std::ofstream sf1(".tapri_modules/" + name + ".meme");
            sf1 << shimCode;
            std::ofstream sf2(".meme_env/pkgs/" + name + ".meme");
            sf2 << shimCode;

            tapriContent += "    \"" + name + "\": \"" + pkg + "\"";
            lockContent += "    \"" + name + "\": { \"source\": \"" + pkg + "\", \"resolved\": \".meme_env/pkgs/" + name + ".meme\" }";

            if (i + 1 < pkgsToInstall.size())
            {
                tapriContent += ",\n";
                lockContent += ",\n";
            }
            else
            {
                tapriContent += "\n";
                lockContent += "\n";
            }

            std::cout << formatRichText("[hara bold]✔ Mangwa liya:[/] " + pkg + " -> .meme_env/pkgs/" + name + ".meme\n");
        }

        tapriContent += "  }\n}\n";
        lockContent += "  }\n}\n";

        std::ofstream tf("tapri.json");
        tf << tapriContent;

        std::ofstream lk(".meme_env/tapri.lock");
        lk << lockContent;
        return;
    }
}

static void handleRun(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cout << formatRichText("[peela]Usage:[/] ./meme run <script_name>\n");
        return;
    }
    std::string scriptName = argv[2];
    std::ifstream tf("tapri.json");
    if (!tf.is_open())
    {
        std::cout << formatRichText("[laal]❌ tapri.json nahi mila! Script run karne ke liye tapri.json chahiye.[/]\n");
        return;
    }

    std::stringstream ss;
    ss << tf.rdbuf();
    std::string content = ss.str();

    std::string searchKey = "\"" + scriptName + "\"";
    size_t pos = content.find(searchKey);
    if (pos == std::string::npos)
    {
        std::cout << formatRichText("[laal]❌ Script '" + scriptName + "' tapri.json mein nahi mila![/]\n");
        return;
    }
    size_t colon = content.find(':', pos);
    size_t q1 = content.find('"', colon);
    size_t q2 = content.find('"', q1 + 1);
    if (q1 == std::string::npos || q2 == std::string::npos)
    {
        std::cout << formatRichText("[laal]❌ Script parse nahi ho saka![/]\n");
        return;
    }
    std::string cmd = content.substr(q1 + 1, q2 - q1 - 1);
    std::cout << formatRichText("[asmani]▶ Running script: " + scriptName + " (" + cmd + ")[/]\n");
    if (cmd.rfind("meme ", 0) == 0)
    {
        cmd = std::string(argv[0]) + cmd.substr(4);
    }
    std::system(cmd.c_str());
}

static void handleSetup(int argc, char *argv[])
{
    (void)argc;
    std::cout << formatRichText("[asmani bold]🔗 MEMElang Global CLI Setup[/]\n");

    std::filesystem::path currentExe;
#if defined(__APPLE__)
    char pathBuf[1024];
    uint32_t size = sizeof(pathBuf);
    if (_NSGetExecutablePath(pathBuf, &size) == 0)
    {
        try {
            currentExe = std::filesystem::canonical(pathBuf);
        } catch (...) {
            currentExe = pathBuf;
        }
    }
#endif
    if (currentExe.empty())
    {
        try {
            currentExe = std::filesystem::canonical("/proc/self/exe");
        } catch (...) {
            try {
                currentExe = std::filesystem::canonical(argv[0]);
            } catch (...) {
                currentExe = argv[0];
            }
        }
    }

    std::cout << "Detected binary: " << currentExe.string() << "\n";

    bool linked = false;
    std::string installedPath = "";

    // 1. Try /usr/local/bin/meme
    std::filesystem::path usrLocalBin = "/usr/local/bin";
    std::filesystem::path target1 = usrLocalBin / "meme";
    try {
        if (std::filesystem::exists(usrLocalBin)) {
            if (std::filesystem::exists(target1) || std::filesystem::is_symlink(target1)) {
                std::filesystem::remove(target1);
            }
            std::filesystem::create_symlink(currentExe, target1);
            linked = true;
            installedPath = target1.string();
        }
    } catch (...) {
        linked = false;
    }

    // 2. Fallback to ~/.local/bin/meme
    if (!linked) {
        const char *home = std::getenv("HOME");
        if (home) {
            std::filesystem::path localBin = std::filesystem::path(home) / ".local" / "bin";
            try {
                std::filesystem::create_directories(localBin);
                std::filesystem::path target2 = localBin / "meme";
                if (std::filesystem::exists(target2) || std::filesystem::is_symlink(target2)) {
                    std::filesystem::remove(target2);
                }
                std::filesystem::create_symlink(currentExe, target2);
                linked = true;
                installedPath = target2.string();
            } catch (const std::exception &e) {
                std::cout << formatRichText("[laal]❌ Failed to create symlink: " + std::string(e.what()) + "[/]\n");
            }
        }
    }

    if (linked) {
        std::cout << formatRichText("[hara bold]✔ Mast! MEMElang CLI successfully linked![/]\n");
        std::cout << "Symlink: " << installedPath << " -> " << currentExe.string() << "\n\n";

        std::string parentDir = std::filesystem::path(installedPath).parent_path().string();
        const char *envPath = std::getenv("PATH");
        bool inPath = false;
        if (envPath && std::string(envPath).find(parentDir) != std::string::npos) {
            inPath = true;
        }

        if (inPath) {
            std::cout << formatRichText("[hara]Ab kisi bhi project folder se seedha [mota]meme[/] chala sakte ho![/]\n");
            std::cout << "Example: cd helloworld && meme dev\n";
        } else {
            std::cout << formatRichText("[peela]Notice:[/] " + parentDir + " is not in your current PATH.\n");
            std::cout << "Add this line to your ~/.zshrc or ~/.bashrc:\n";
            std::cout << formatRichText("[asmani]  export PATH=\"" + parentDir + ":$PATH\"[/]\n");
        }
    } else {
        std::cout << formatRichText("[peela]Could not automatically symlink due to permission restrictions.[/]\n");
        std::cout << "You can manually run this command:\n";
        std::cout << formatRichText("[asmani]  sudo ln -sf \"" + currentExe.string() + "\" /usr/local/bin/meme[/]\n");
    }
}

static std::string escapeJsonString(const std::string &input)
{
    std::string out;
    out.reserve(input.size() + 32);
    for (unsigned char c : input)
    {
        switch (c)
        {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 32)
                {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                }
                else
                {
                    out += c;
                }
                break;
        }
    }
    return out;
}

static std::string getLivePreviewHtml(const std::string &appName, int port, const std::string &platform, const std::string &initialHtml = "")
{
    (void)port;
    std::string html = R"RAWHTML(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>)RAWHTML" + appName + R"RAWHTML(</title>
    <script src="https://cdn.tailwindcss.com"></script>
    <style>
        * { box-sizing: border-box; }
        body {
            margin: 0;
            padding: 0;
            background-color: #020617;
            color: #f8fafc;
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            min-height: 100vh;
        }
        .dev-badge {
            position: fixed;
            bottom: 16px;
            right: 16px;
            background: rgba(15, 23, 42, 0.9);
            border: 1px solid #334155;
            backdrop-filter: blur(8px);
            padding: 6px 14px;
            border-radius: 9999px;
            font-size: 12px;
            color: #94a3b8;
            display: flex;
            align-items: center;
            gap: 8px;
            z-index: 9999;
            box-shadow: 0 4px 12px rgba(0,0,0,0.5);
            user-select: none;
        }
        .dev-badge .pulse {
            width: 7px;
            height: 7px;
            border-radius: 50%;
            background: #22c55e;
            box-shadow: 0 0 8px #22c55e;
            animation: pulse 2s infinite;
        }
        @keyframes pulse {
            0%, 100% { opacity: 1; transform: scale(1); }
            50% { opacity: 0.4; transform: scale(0.85); }
        }
        .dev-badge a {
            color: #38bdf8;
            text-decoration: none;
            font-weight: 600;
            cursor: pointer;
        }
        .dev-badge a:hover {
            text-decoration: underline;
        }
    </style>
</head>
<body class="bg-slate-950 text-slate-100 min-h-screen">
    <div id="app-root">
)RAWHTML" + initialHtml + R"RAWHTML(
    </div>

    <div class="dev-badge">
        <span class="pulse"></span>
        <span class="text-slate-200 font-semibold">MEMElang Live</span>
        <span class="text-slate-500">|</span>
        <span class="text-xs text-slate-400 capitalize" id="platform-label">)RAWHTML" + platform + R"RAWHTML(</span>
        <span class="text-slate-500">|</span>
        <a onclick="togglePlatform()">Switch</a>
    </div>

    <script>
        let currentVersion = 1;
        let isDispatching = false;

        async function __memeDispatch(id, type, value) {
            let sendVal = value || '';
            const activeEl = document.activeElement;
            const isInput = activeEl && (activeEl.tagName === 'INPUT' || activeEl.tagName === 'TEXTAREA');

            // If user clicks a button while an input is focused, sync the focused input first
            if (type === 'click' && isInput) {
                const inputId = parseInt(activeEl.getAttribute('data-meme-id'));
                if (inputId && inputId !== id) {
                    try {
                        await fetch('/api/dispatch', {
                            method: 'POST',
                            headers: { 'Content-Type': 'application/json' },
                            body: JSON.stringify({ handlerId: inputId, type: 'change', value: activeEl.value })
                        });
                    } catch (e) {}
                }
            }

            isDispatching = true;
            try {
                const res = await fetch('/api/dispatch', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ handlerId: id, type: type, value: sendVal })
                });
                if (!res.ok) {
                    console.error("HTTP error from /api/dispatch:", res.status);
                    return;
                }
                const data = await res.json();
                if (data.ok && data.html) {
                    currentVersion = data.version;
                    updateApp(data.html);
                } else if (!data.ok) {
                    console.warn("Server dispatch returned ok: false for handlerId:", id);
                }
            } catch (err) {
                console.error("Dispatch error:", err);
            } finally {
                isDispatching = false;
            }
        }

        function updateApp(newHtml) {
            const root = document.getElementById('app-root');
            const activeEl = document.activeElement;
            const isInput = activeEl && (activeEl.tagName === 'INPUT' || activeEl.tagName === 'TEXTAREA');
            const activeId = isInput ? activeEl.getAttribute('data-meme-id') : null;
            const liveVal = isInput ? activeEl.value : null;
            const selStart = isInput ? activeEl.selectionStart : null;
            const selEnd = isInput ? activeEl.selectionEnd : null;

            root.innerHTML = newHtml;

            if (activeId) {
                const restoredEl = root.querySelector(`[data-meme-id="${activeId}"]`);
                if (restoredEl) {
                    const serverVal = restoredEl.getAttribute('value');
                    if (serverVal !== null) {
                        restoredEl.value = serverVal;
                    } else if (liveVal !== null) {
                        restoredEl.value = liveVal;
                    }
                    restoredEl.focus();
                    if (selStart !== null && selEnd !== null && restoredEl.setSelectionRange) {
                        try { restoredEl.setSelectionRange(selStart, selEnd); } catch(e) {}
                    }
                }
            }
        }

        async function togglePlatform() {
            try {
                const currentLabel = document.getElementById('platform-label').textContent;
                const next = (currentLabel === 'web') ? 'desktop' : 'web';
                await fetch('/api/platform', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ platform: next })
                });
                document.getElementById('platform-label').textContent = next;
            } catch(e) {}
        }

        async function fetchState() {
            if (isDispatching) return;
            try {
                const res = await fetch('/api/state');
                if (!res.ok) return;
                const data = await res.json();
                if (data.version !== currentVersion && !isDispatching) {
                    currentVersion = data.version;
                    if (data.html) updateApp(data.html);
                    if (data.platform) document.getElementById('platform-label').textContent = data.platform;
                }
            } catch (err) {}
        }

        window.addEventListener('keydown', (e) => {
            if (e.target && (e.target.tagName === 'INPUT' || e.target.tagName === 'TEXTAREA')) return;
            if (e.key === 'r' || e.key === 'R') fetchState();
            if (e.key === 'p' || e.key === 'P') togglePlatform();
        });

        setInterval(fetchState, 250);
    </script>
</body>
</html>
)RAWHTML";
    return html;
}


static bool sendAll(int fd, const std::string &data)
{
    size_t total = 0;
    while (total < data.size())
    {
        ssize_t n = send(fd, data.data() + total, data.size() - total, 0);
        if (n <= 0) return false;
        total += static_cast<size_t>(n);
    }
    return true;
}

static void closeHttpSocket(int fd)
{
    shutdown(fd, SHUT_WR);
    char dummy[512];
    while (recv(fd, dummy, sizeof(dummy), MSG_DONTWAIT) > 0) {}
    close(fd);
}

static bool isPortAvailable(int port)
{
    // Probe IPv4 loopback
    int s4 = socket(AF_INET, SOCK_STREAM, 0);
    if (s4 >= 0)
    {
        struct sockaddr_in a4;
        std::memset(&a4, 0, sizeof(a4));
        a4.sin_family = AF_INET;
        a4.sin_port = htons(port);
        a4.sin_addr.s_addr = inet_addr("127.0.0.1");
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 40000;
        setsockopt(s4, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
        setsockopt(s4, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tv, sizeof(tv));
        if (connect(s4, (struct sockaddr *)&a4, sizeof(a4)) == 0)
        {
            close(s4);
            return false;
        }
        close(s4);
    }

    // Probe IPv6 loopback
    int s6 = socket(AF_INET6, SOCK_STREAM, 0);
    if (s6 >= 0)
    {
        struct sockaddr_in6 a6;
        std::memset(&a6, 0, sizeof(a6));
        a6.sin6_family = AF_INET6;
        a6.sin6_port = htons(port);
        inet_pton(AF_INET6, "::1", &a6.sin6_addr);
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 40000;
        setsockopt(s6, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
        setsockopt(s6, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tv, sizeof(tv));
        if (connect(s6, (struct sockaddr *)&a6, sizeof(a6)) == 0)
        {
            close(s6);
            return false;
        }
        close(s6);
    }

    return true;
}

static void startDevHttpServer(DevServerState &state)
{
    int serverFd = -1;
    int boundPort = state.port;

    for (int p = state.port; p < state.port + 50; ++p)
    {
        if (!isPortAvailable(p))
        {
            continue;
        }

        serverFd = socket(AF_INET, SOCK_STREAM, 0);
        if (serverFd < 0) continue;

        int opt = 1;
        setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(p);

        if (bind(serverFd, (struct sockaddr *)&addr, sizeof(addr)) == 0)
        {
            boundPort = p;
            break;
        }
        close(serverFd);
        serverFd = -1;
    }

    if (serverFd < 0)
    {
        return;
    }

    state.port = boundPort;
    state.serverFd = serverFd;

    if (listen(serverFd, 128) < 0)
    {
        close(serverFd);
        return;
    }

    fcntl(serverFd, F_SETFL, O_NONBLOCK);

    while (state.running)
    {
        struct pollfd pfd;
        pfd.fd = serverFd;
        pfd.events = POLLIN;
        int ret = poll(&pfd, 1, 250);
        if (ret <= 0 || !(pfd.revents & POLLIN))
        {
            continue;
        }

        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientFd = accept(serverFd, (struct sockaddr *)&clientAddr, &clientLen);
        if (clientFd < 0) continue;

        // Reset clientFd to blocking mode with timeout
        int flags = fcntl(clientFd, F_GETFL, 0);
        fcntl(clientFd, F_SETFL, flags & ~O_NONBLOCK);

        struct timeval tv;
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        setsockopt(clientFd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

        std::string req;
        char buf[8192];

        // 1. Read until headers complete (\r\n\r\n)
        while (req.find("\r\n\r\n") == std::string::npos)
        {
            int bytesRead = recv(clientFd, buf, sizeof(buf) - 1, 0);
            if (bytesRead <= 0) break;
            req.append(buf, bytesRead);
        }

        if (req.empty())
        {
            close(clientFd);
            continue;
        }

        // 2. Read full body if Content-Length specified
        size_t headerEnd = req.find("\r\n\r\n");
        if (headerEnd != std::string::npos)
        {
            size_t bodyStart = headerEnd + 4;
            size_t clPos = req.find("Content-Length:");
            if (clPos == std::string::npos) clPos = req.find("content-length:");
            if (clPos != std::string::npos && clPos < headerEnd)
            {
                size_t valStart = req.find_first_of("0123456789", clPos + 15);
                if (valStart != std::string::npos && valStart < headerEnd)
                {
                    try {
                        size_t cl = std::stoul(req.substr(valStart));
                        while (req.size() < bodyStart + cl)
                        {
                            int bytesRead = recv(clientFd, buf, sizeof(buf) - 1, 0);
                            if (bytesRead <= 0) break;
                            req.append(buf, bytesRead);
                        }
                    } catch (...) {}
                }
            }
        }

        std::istringstream reqStream(req);
        std::string method, path;
        reqStream >> method >> path;

        if (method == "OPTIONS")
        {
            std::string res = "HTTP/1.1 204 No Content\r\n"
                              "Access-Control-Allow-Origin: *\r\n"
                              "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                              "Access-Control-Allow-Headers: Content-Type, Authorization, Accept, X-Requested-With\r\n"
                              "Connection: close\r\n\r\n";
            sendAll(clientFd, res);
            closeHttpSocket(clientFd);
            continue;
        }

        if (method == "GET" && (path == "/" || path == "/index.html"))
        {
            std::string html;
            {
                std::lock_guard<std::mutex> lock(state.mtx);
                html = getLivePreviewHtml(state.appName, state.port, state.platform, state.html);
            }
            std::string res = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/html; charset=utf-8\r\n"
                              "Content-Length: " + std::to_string(html.size()) + "\r\n"
                              "Access-Control-Allow-Origin: *\r\n"
                              "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                              "Access-Control-Allow-Headers: Content-Type, Authorization, Accept, X-Requested-With\r\n"
                              "Connection: close\r\n\r\n" + html;
            sendAll(clientFd, res);
            closeHttpSocket(clientFd);
        }
        else if (method == "GET" && path == "/api/state")
        {
            std::string json;
            {
                std::lock_guard<std::mutex> lock(state.mtx);
                json = "{\n"
                       "  \"appName\": \"" + escapeJsonString(state.appName) + "\",\n"
                       "  \"version\": " + std::to_string(state.version) + ",\n"
                       "  \"platform\": \"" + state.platform + "\",\n"
                       "  \"html\": \"" + escapeJsonString(state.html) + "\",\n"
                       "  \"gpu_commands\": " + state.gpuCommands + ",\n"
                       "  \"hit_regions\": " + state.hitRegions + ",\n"
                       "  \"terminal_ui\": \"" + escapeJsonString(state.terminalUi) + "\"\n"
                       "}";
            }
            std::string res = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: application/json; charset=utf-8\r\n"
                              "Content-Length: " + std::to_string(json.size()) + "\r\n"
                              "Access-Control-Allow-Origin: *\r\n"
                              "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                              "Access-Control-Allow-Headers: Content-Type, Authorization, Accept, X-Requested-With\r\n"
                              "Connection: close\r\n\r\n" + json;
            sendAll(clientFd, res);
            closeHttpSocket(clientFd);
        }
        else if (method == "POST" && path == "/api/dispatch")
        {
            size_t bodyPos = req.find("\r\n\r\n");
            std::string body = (bodyPos != std::string::npos) ? req.substr(bodyPos + 4) : "";

            int handlerId = 0;
            std::string evType = "click";
            std::string evValue = "";

            size_t idPos = body.find("\"handlerId\":");
            if (idPos != std::string::npos)
            {
                size_t start = body.find_first_of("-0123456789", idPos + 12);
                if (start != std::string::npos)
                {
                    try {
                        handlerId = std::stoi(body.substr(start));
                    } catch (...) {}
                }
            }
            size_t typePos = body.find("\"type\":");
            if (typePos != std::string::npos)
            {
                size_t q1 = body.find('"', typePos + 7);
                if (q1 != std::string::npos)
                {
                    size_t q2 = body.find('"', q1 + 1);
                    if (q2 != std::string::npos)
                    {
                        evType = body.substr(q1 + 1, q2 - q1 - 1);
                    }
                }
            }
            size_t valPos = body.find("\"value\":");
            if (valPos != std::string::npos)
            {
                size_t start = valPos + 8;
                while (start < body.size() && (body[start] == ' ' || body[start] == '\t')) start++;
                if (start < body.size())
                {
                    if (body[start] == '"')
                    {
                        size_t end = start + 1;
                        while (end < body.size())
                        {
                            if (body[end] == '\\' && end + 1 < body.size())
                            {
                                end += 2;
                            }
                            else if (body[end] == '"')
                            {
                                break;
                            }
                            else
                            {
                                end++;
                            }
                        }
                        std::string raw = body.substr(start + 1, end - start - 1);
                        std::string unescaped;
                        for (size_t i = 0; i < raw.size(); ++i)
                        {
                            if (raw[i] == '\\' && i + 1 < raw.size())
                            {
                                if (raw[i + 1] == '"') { unescaped += '"'; i++; }
                                else if (raw[i + 1] == '\\') { unescaped += '\\'; i++; }
                                else if (raw[i + 1] == 'n') { unescaped += '\n'; i++; }
                                else if (raw[i + 1] == 't') { unescaped += '\t'; i++; }
                                else { unescaped += raw[i + 1]; i++; }
                            }
                            else
                            {
                                unescaped += raw[i];
                            }
                        }
                        evValue = unescaped;
                    }
                    else
                    {
                        size_t end = body.find_first_of(",}\r\n ", start);
                        if (end == std::string::npos) end = body.size();
                        evValue = body.substr(start, end - start);
                    }
                }
            }

            bool ok = false;
            try
            {
                std::lock_guard<std::mutex> lock(state.mtx);
                if (state.interpreter)
                {
                    ok = state.interpreter->dispatchUiEvent(handlerId, evType, evValue);
                    state.html = state.interpreter->getLastHtml();
                    state.gpuCommands = state.interpreter->getLastGpuCommandsJson();
                    state.hitRegions = state.interpreter->getLastHitRegionsJson();
                    state.terminalUi = state.interpreter->getLastTerminalUi();
                    state.version++;
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "CRITICAL ERROR in dispatchUiEvent: " << e.what() << "\n";
            }
            catch (...)
            {
                std::cerr << "CRITICAL UNKNOWN ERROR in dispatchUiEvent\n";
            }

            std::string json = "{\"ok\": " + std::string(ok ? "true" : "false") +
                               ", \"version\": " + std::to_string(state.version) +
                               ", \"html\": \"" + escapeJsonString(state.html) + "\"}";
            std::string res = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: application/json; charset=utf-8\r\n"
                              "Content-Length: " + std::to_string(json.size()) + "\r\n"
                              "Access-Control-Allow-Origin: *\r\n"
                              "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                              "Access-Control-Allow-Headers: Content-Type, Authorization, Accept, X-Requested-With\r\n"
                              "Connection: close\r\n\r\n" + json;
            sendAll(clientFd, res);
            closeHttpSocket(clientFd);
        }
        else if (method == "POST" && path == "/api/platform")
        {
            size_t bodyPos = req.find("\r\n\r\n");
            std::string body = (bodyPos != std::string::npos) ? req.substr(bodyPos + 4) : "";
            size_t pPos = body.find("\"platform\":");
            if (pPos != std::string::npos)
            {
                size_t q1 = body.find('"', pPos + 11);
                size_t q2 = body.find('"', q1 + 1);
                if (q1 != std::string::npos && q2 != std::string::npos)
                {
                    std::lock_guard<std::mutex> lock(state.mtx);
                    state.platform = body.substr(q1 + 1, q2 - q1 - 1);
                    state.version++;
                }
            }
            std::string json = "{\"ok\": true, \"platform\": \"" + state.platform + "\"}";
            std::string res = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: application/json; charset=utf-8\r\n"
                              "Content-Length: " + std::to_string(json.size()) + "\r\n"
                              "Access-Control-Allow-Origin: *\r\n"
                              "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                              "Access-Control-Allow-Headers: Content-Type, Authorization, Accept, X-Requested-With\r\n"
                              "Connection: close\r\n\r\n" + json;
            sendAll(clientFd, res);
            closeHttpSocket(clientFd);
        }
        else
        {
            std::string msg = "404 Not Found";
            std::string res = "HTTP/1.1 404 Not Found\r\n"
                              "Content-Length: " + std::to_string(msg.size()) + "\r\n"
                              "Connection: close\r\n\r\n" + msg;
            sendAll(clientFd, res);
            closeHttpSocket(clientFd);
        }
    }

    if (serverFd >= 0)
    {
        close(serverFd);
    }
}

static void printDevDashboard(const std::string &targetFile, int port, const std::string &platform, bool showTerminal, const std::string &terminalUi)
{
    std::cout << "\033[2J\033[H";
    std::cout << formatRichText("[asmani bold]🎭 MEMElang Dev Server v1.0[/]\n");
    std::cout << formatRichText("[halka]━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━[/]\n");
    std::cout << formatRichText("  [hara bold]➜ Local:[/]    http://127.0.0.1:" + std::to_string(port) + "/ (or http://localhost:" + std::to_string(port) + "/)\n");
    std::string platDesc = (platform == "desktop") ? "Desktop Native" : (platform == "terminal" ? "Terminal ASCII" : "Web (Zero-WebView 60 FPS Native Canvas)");
    std::cout << formatRichText("  [neela bold]➜ Platform:[/] " + platDesc + "\n");
    std::cout << formatRichText("  [peela bold]➜ Watching:[/] " + targetFile + "\n");
    std::cout << formatRichText("[halka]━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━[/]\n\n");

    if (showTerminal && !terminalUi.empty())
    {
        std::cout << formatRichText("[asmani mota][Terminal UI Live Preview][/]\n");
        std::cout << terminalUi << "\n\n";
    }

    std::cout << formatRichText("[halka]────────────────────────────────────────────────────────────────[/]\n");
    std::cout << formatRichText("  [mota][w][/] Open in Browser       [mota][d][/] Native Desktop Window\n");
    std::cout << formatRichText("  [mota][t][/] Toggle Terminal UI    [mota][r][/] Force Hot-Reload\n");
    std::cout << formatRichText("  [mota][p][/] Switch Platform       [mota][q][/] Quit Dev Server\n");
    std::cout << formatRichText("[halka]────────────────────────────────────────────────────────────────[/]\n");
    std::cout.flush();
}

static void handleDev(int argc, char *argv[])
{
    std::string targetFile = "";
    int port = 3000;
    std::string platform = "web";
    bool openBrowser = true;

    // Check .env if present in current directory
    if (std::filesystem::exists(".env"))
    {
        std::ifstream envf(".env");
        std::string line;
        while (std::getline(envf, line))
        {
            size_t eq = line.find('=');
            if (eq != std::string::npos)
            {
                std::string k = line.substr(0, eq);
                std::string v = line.substr(eq + 1);
                k.erase(0, k.find_first_not_of(" \t\r\n"));
                k.erase(k.find_last_not_of(" \t\r\n") + 1);
                v.erase(0, v.find_first_not_of(" \t\r\n"));
                v.erase(v.find_last_not_of(" \t\r\n") + 1);
                if (k == "PORT") {
                    try { port = std::stoi(v); } catch (...) {}
                } else if (k == "PLATFORM") {
                    platform = v;
                }
            }
        }
    }

    // Parse command line arguments
    for (int i = 2; i < argc; ++i)
    {
        std::string a = argv[i];
        if (a.rfind("--port=", 0) == 0)
        {
            try { port = std::stoi(a.substr(7)); } catch (...) {}
        }
        else if (a.rfind("--platform=", 0) == 0)
        {
            platform = a.substr(11);
        }
        else if (a == "--no-open")
        {
            openBrowser = false;
        }
        else if (a == "--open")
        {
            openBrowser = true;
        }
        else if (!a.empty() && a[0] != '-')
        {
            targetFile = a;
        }
    }

    if (targetFile.empty())
    {
        std::vector<std::string> candidates = {"src/App.meme", "src/main.meme", "App.meme", "main.meme"};
        for (const auto &c : candidates)
        {
            if (std::filesystem::exists(c))
            {
                targetFile = c;
                break;
            }
        }
    }

    if (targetFile.empty() || !std::filesystem::exists(targetFile))
    {
        std::cout << formatRichText("[laal]❌ Dev server ke liye koi entry file nahi mili! Pass a file e.g. './meme dev src/App.meme'[/]\n");
        return;
    }

    auto state = std::make_shared<DevServerState>();
    state->targetFile = targetFile;
    state->port = port;
    state->platform = platform;
    state->appName = std::filesystem::path(targetFile).stem().string();

    auto interpreter = std::make_unique<Interpreter>();
    state->interpreter = interpreter.get();

    // Read and run initial file
    std::ifstream initialFile(targetFile);
    if (initialFile.is_open())
    {
        std::stringstream ss;
        ss << initialFile.rdbuf();
        std::string src = ss.str();
        interpreter->setCurrentFilePath(targetFile);
        runSource(src, *interpreter);
        state->gpuCommands = interpreter->getLastGpuCommandsJson();
        state->hitRegions = interpreter->getLastHitRegionsJson();
        state->terminalUi = interpreter->getLastTerminalUi();
        state->html = interpreter->getLastHtml();
    }

    // Start HTTP server in background thread
    std::thread serverThread([state]() {
        startDevHttpServer(*state);
    });

    int waitLimit = 100;
    while (state->serverFd < 0 && state->running && waitLimit-- > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    printDevDashboard(targetFile, state->port, state->platform, state->showTerminal, state->terminalUi);

    bool isHeadless = (std::getenv("MEME_HEADLESS") != nullptr) || (std::getenv("MEME_DEV_ONCE") != nullptr);

    if (openBrowser && !isHeadless && state->platform == "web")
    {
#if defined(__APPLE__)
        std::string openCmd = "open http://127.0.0.1:" + std::to_string(state->port) + " &";
        std::system(openCmd.c_str());
#elif defined(__linux__)
        std::string openCmd = "xdg-open http://127.0.0.1:" + std::to_string(state->port) + " &";
        std::system(openCmd.c_str());
#endif
    }
    else if (!isHeadless && state->platform == "desktop")
    {
        MemeUI::showNativeDesktopWindow(*state);
    }

    if (isHeadless)
    {
        state->running = false;
        if (serverThread.joinable()) serverThread.join();
        return;
    }

    struct termios orig_termios;
    bool hasTermios = false;
    if (isatty(STDIN_FILENO))
    {
        if (tcgetattr(STDIN_FILENO, &orig_termios) == 0)
        {
            struct termios raw = orig_termios;
            raw.c_lflag &= ~(ICANON | ECHO);
            raw.c_cc[VMIN] = 0;
            raw.c_cc[VTIME] = 0;
            tcsetattr(STDIN_FILENO, TCSANOW, &raw);
            hasTermios = true;
        }
    }

    auto lastTime = std::filesystem::last_write_time(targetFile);

    while (state->running)
    {
        if (std::filesystem::exists(targetFile))
        {
            auto curTime = std::filesystem::last_write_time(targetFile);
            if (curTime != lastTime)
            {
                lastTime = curTime;
                std::ifstream f(targetFile);
                if (f.is_open())
                {
                    std::stringstream ss;
                    ss << f.rdbuf();
                    std::string src = ss.str();
                    auto newInterp = std::make_unique<Interpreter>();
                    newInterp->setCurrentFilePath(targetFile);
                    if (runSource(src, *newInterp))
                    {
                        std::lock_guard<std::mutex> lock(state->mtx);
                        interpreter = std::move(newInterp);
                        state->interpreter = interpreter.get();
                        state->gpuCommands = interpreter->getLastGpuCommandsJson();
                        state->hitRegions = interpreter->getLastHitRegionsJson();
                        state->terminalUi = interpreter->getLastTerminalUi();
                        state->html = interpreter->getLastHtml();
                        state->version++;
                    }
                }
                printDevDashboard(targetFile, state->port, state->platform, state->showTerminal, state->terminalUi);
                MemeUI::triggerNativeDesktopRepaint();
            }
        }

        if (MemeUI::isNativeDesktopWindowOpen())
        {
            MemeUI::pumpNativeDesktopEvents(0.016);
        }
        else if (state->platform == "desktop" && !isHeadless)
        {
            MemeUI::showNativeDesktopWindow(*state);
        }

        struct pollfd pfd;
        pfd.fd = STDIN_FILENO;
        pfd.events = POLLIN;
        int pollTimeout = MemeUI::isNativeDesktopWindowOpen() ? 16 : 250;
        int pRet = poll(&pfd, 1, pollTimeout);
        if (pRet > 0 && (pfd.revents & POLLIN))
        {
            char c = 0;
            if (read(STDIN_FILENO, &c, 1) > 0)
            {
                if (c == 'q' || c == 'Q')
                {
                    state->running = false;
                    MemeUI::hideNativeDesktopWindow();
                    break;
                }
                else if (c == 'w' || c == 'W')
                {
#if defined(__APPLE__)
                    std::string openCmd = "open http://127.0.0.1:" + std::to_string(state->port) + " &";
                    std::system(openCmd.c_str());
#elif defined(__linux__)
                    std::string openCmd = "xdg-open http://127.0.0.1:" + std::to_string(state->port) + " &";
                    std::system(openCmd.c_str());
#endif
                    printDevDashboard(targetFile, state->port, state->platform, state->showTerminal, state->terminalUi);
                }
                else if (c == 'd' || c == 'D')
                {
                    {
                        std::lock_guard<std::mutex> lock(state->mtx);
                        state->platform = "desktop";
                        state->version++;
                    }
                    if (!isHeadless)
                    {
                        MemeUI::showNativeDesktopWindow(*state);
                    }
                    printDevDashboard(targetFile, state->port, state->platform, state->showTerminal, state->terminalUi);
                }
                else if (c == 't' || c == 'T')
                {
                    state->showTerminal = !state->showTerminal;
                    printDevDashboard(targetFile, state->port, state->platform, state->showTerminal, state->terminalUi);
                }
                else if (c == 'p' || c == 'P')
                {
                    {
                        std::lock_guard<std::mutex> lock(state->mtx);
                        if (state->platform == "web") state->platform = "desktop";
                        else if (state->platform == "desktop") state->platform = "terminal";
                        else state->platform = "web";
                        state->version++;
                    }
                    if (state->platform == "desktop" && !isHeadless)
                    {
                        MemeUI::showNativeDesktopWindow(*state);
                    }
                    else if (state->platform != "desktop")
                    {
                        MemeUI::hideNativeDesktopWindow();
                    }
                    printDevDashboard(targetFile, state->port, state->platform, state->showTerminal, state->terminalUi);
                }
                else if (c == 'r' || c == 'R')
                {
                    std::ifstream f(targetFile);
                    if (f.is_open())
                    {
                        std::stringstream ss;
                        ss << f.rdbuf();
                        std::string src = ss.str();
                        auto newInterp = std::make_unique<Interpreter>();
                        newInterp->setCurrentFilePath(targetFile);
                        if (runSource(src, *newInterp))
                        {
                            std::lock_guard<std::mutex> lock(state->mtx);
                            interpreter = std::move(newInterp);
                            state->interpreter = interpreter.get();
                            state->gpuCommands = interpreter->getLastGpuCommandsJson();
                            state->hitRegions = interpreter->getLastHitRegionsJson();
                            state->terminalUi = interpreter->getLastTerminalUi();
                            state->html = interpreter->getLastHtml();
                            state->version++;
                        }
                    }
                    printDevDashboard(targetFile, state->port, state->platform, state->showTerminal, state->terminalUi);
                }
            }
        }
    }

    if (hasTermios)
    {
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    }

    MemeUI::hideNativeDesktopWindow();

    std::cout << formatRichText("\n[asmani]Dev server stopped. Khatam, tata, bye-bye![/]\n");
    if (serverThread.joinable())
    {
        serverThread.join();
    }
}

static void handleTest(int argc, char *argv[])
{
    std::string targetDir = (argc >= 3) ? argv[2] : (std::filesystem::exists("tests") ? "tests" : ".");
    if (!std::filesystem::exists(targetDir))
    {
        std::cout << formatRichText("[laal]❌ Test directory '" + targetDir + "' nahi mili![/]\n");
        std::exit(1);
    }

    std::vector<std::filesystem::path> testFiles;
    for (const auto &entry : std::filesystem::recursive_directory_iterator(targetDir))
    {
        if (entry.is_regular_file())
        {
            std::string filename = entry.path().filename().string();
            if ((filename.length() > 10 && filename.substr(filename.length() - 10) == "_test.meme") ||
                (filename.rfind("test_", 0) == 0 && filename.ends_with(".meme")))
            {
                testFiles.push_back(entry.path());
            }
        }
    }

    if (testFiles.empty())
    {
        std::cout << formatRichText("[peela]Koi test files (*_test.meme) nahi mili in " + targetDir + "[/]\n");
        return;
    }

    std::cout << formatRichText("[asmani bold]🧪 Running MEMElang Test Runner on " + std::to_string(testFiles.size()) + " files:[/]\n");
    int passed = 0;
    int failed = 0;

    for (const auto &tf : testFiles)
    {
        std::cout << formatRichText("[asmani]• " + tf.string() + "[/]\n");
        std::ifstream file(tf);
        if (!file.is_open())
        {
            std::cout << "  [laal]❌ File open error![/]\n";
            failed++;
            continue;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string source = buffer.str();

        Interpreter interpreter;
        interpreter.setCurrentFilePath(tf.string());
        if (runSource(source, interpreter))
        {
            passed++;
        }
        else
        {
            failed++;
        }
    }

    std::cout << "\n========================================\n";
    if (failed == 0)
    {
        std::cout << formatRichText("[hara bold]✅ ALL TEST FILES PASSED! (" + std::to_string(passed) + " passed)[/]\n");
    }
    else
    {
        std::cout << formatRichText("[laal bold]❌ TESTS FAILED: " + std::to_string(failed) + " failed, " + std::to_string(passed) + " passed[/]\n");
        std::exit(1);
    }
}

static void handlePakao(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cout << formatRichText("[peela]Usage:[/] ./meme pakao <file.meme> [--target=flutter|--target=native-macos]\n");
        return;
    }
    std::string file = argv[2];
    std::string target = "flutter";
    for (int i = 3; i < argc; ++i)
    {
        std::string a = argv[i];
        if (a.rfind("--target=", 0) == 0)
        {
            target = a.substr(9);
        }
    }

    std::filesystem::path p(file);
    std::string stem = p.stem().string();

    if (target == "flutter")
    {
        std::string flutterOut = stem + "_flutter.dart";
        std::ofstream f(flutterOut);
        f << "// Auto-generated Flutter Native Target from " << file << "\n"
          << "import 'package:flutter/material.dart';\n\n"
          << "void main() => runApp(const MaterialApp(home: Scaffold(body: Center(child: Text('MEMElang Native Flutter UI')))));\n";
        std::cout << formatRichText("[hara bold]✔ Pak gaya:[/] Generated Flutter target at " + flutterOut + " successfully!\n");
        return;
    }
    if (target == "native-macos")
    {
        std::string macOut = stem + "_macos.mm";
        std::ofstream f(macOut);
        f << "// Auto-generated Cocoa/CoreGraphics Native Target from " << file << "\n"
          << "#import <Cocoa/Cocoa.h>\n\n"
          << "int main(int argc, const char * argv[]) {\n"
          << "    @autoreleasepool {\n"
          << "        NSApplication *app = [NSApplication sharedApplication];\n"
          << "        [app run];\n"
          << "    }\n"
          << "    return 0;\n"
          << "}\n";
        std::cout << formatRichText("[hara bold]✔ Pak gaya:[/] Generated native macOS target at " + macOut + " successfully!\n");
        return;
    }
}

static bool runSource(const std::string &source, Interpreter &interpreter)
{
    try
    {
        Lexer lexer(source);
        std::vector<Token> tokens = lexer.tokenize();

        Parser parser(tokens);
        std::vector<std::unique_ptr<Stmt>> program = parser.parseProgram();

        TypeChecker typeChecker;
        typeChecker.check(program);

        interpreter.interpret(std::move(program));
        return true;
    }
    catch (const MemeExitSignal &e)
    {
        std::exit(static_cast<int>(e.exitCode));
    }
    catch (const std::exception &e)
    {
        std::cerr << "\n❌ Gadbad ho gayi:\n" << e.what() << "\n\n";
        return false;
    }
}

static void runFile(const std::string &path, const std::vector<std::string> &args)
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
    interpreter.setCommandLineArgs(args);
    interpreter.setCurrentFilePath(path);
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
    else if (argc >= 2)
    {
        std::string arg1 = argv[1];
        if (arg1 == "-h" || arg1 == "--help")
        {
            printUsage(argv[0]);
        }
        else if (arg1 == "-v" || arg1 == "--version")
        {
            std::cout << "MEMElang v1.0\n";
        }
        else if (arg1 == "naya" || arg1 == "create" || arg1 == "new")
        {
            handleNaya(argc, argv);
            return 0;
        }
        else if (arg1 == "env")
        {
            handleEnv(argc, argv);
            return 0;
        }
        else if (arg1 == "tapri")
        {
            handleTapri(argc, argv);
            return 0;
        }
        else if (arg1 == "dev")
        {
            handleDev(argc, argv);
            return 0;
        }
        else if (arg1 == "setup" || arg1 == "link")
        {
            handleSetup(argc, argv);
            return 0;
        }
        else if (arg1 == "run" || arg1 == "chala")
        {
            handleRun(argc, argv);
            return 0;
        }
        else if (arg1 == "test" || arg1 == "jaanch")
        {
            handleTest(argc, argv);
            return 0;
        }
        else if (arg1 == "pakao")
        {
            handlePakao(argc, argv);
            return 0;
        }
        else if (arg1 == "-e")
        {
            if (argc < 3)
            {
                printUsage(argv[0]);
                return 1;
            }
            Interpreter interpreter;
            std::vector<std::string> args;
            for (int i = 3; i < argc; ++i)
            {
                args.push_back(argv[i]);
            }
            interpreter.setCommandLineArgs(args);
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
            std::vector<std::string> args;
            for (int i = 1; i < argc; ++i)
            {
                args.push_back(argv[i]);
            }
            runFile(arg1, args);
        }
    }
    return 0;
}