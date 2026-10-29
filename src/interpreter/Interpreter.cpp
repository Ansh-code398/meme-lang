#include "Interpreter.h"
#include "../ui/TailwindEngine.h"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <cctype>
#include <cmath>
#include <random>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <future>
#include <filesystem>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
#include <unistd.h>
#include <termios.h>
#endif

thread_local std::unordered_set<const void *> activePrintPointers;

static double toDouble(const Value &val)
{
    Value v = val.deref();
    if (std::holds_alternative<long long>(v.data))
        return static_cast<double>(std::get<long long>(v.data));
    if (std::holds_alternative<double>(v.data))
        return std::get<double>(v.data);
    throw std::runtime_error("Number chahiye tha par mila: " + v.typeName());
}

static long long toLongLong(const Value &val)
{
    Value v = val.deref();
    if (std::holds_alternative<long long>(v.data))
        return std::get<long long>(v.data);
    if (std::holds_alternative<double>(v.data))
        return static_cast<long long>(std::get<double>(v.data));
    throw std::runtime_error("Number chahiye tha par mila: " + v.typeName());
}

std::string stripAnsi(const std::string &str)
{
    std::string result;
    size_t i = 0;
    while (i < str.size())
    {
        if (str[i] == '\033' && i + 1 < str.size() && str[i + 1] == '[')
        {
            size_t j = i + 2;
            while (j < str.size() && (std::isdigit(static_cast<unsigned char>(str[j])) || str[j] == ';'))
            {
                j++;
            }
            if (j < str.size() && str[j] == 'm')
            {
                i = j + 1;
                continue;
            }
        }
        result += str[i++];
    }
    return result;
}

static const std::unordered_map<std::string, std::string> RICH_TAGS = {
    {"laal", "\033[31m"},
    {"red", "\033[31m"},
    {"hara", "\033[32m"},
    {"green", "\033[32m"},
    {"peela", "\033[33m"},
    {"yellow", "\033[33m"},
    {"neela", "\033[34m"},
    {"blue", "\033[34m"},
    {"gulabi", "\033[35m"},
    {"pink", "\033[35m"},
    {"magenta", "\033[35m"},
    {"asmani", "\033[36m"},
    {"cyan", "\033[36m"},
    {"safed", "\033[37m"},
    {"white", "\033[37m"},
    {"kala", "\033[30m"},
    {"black", "\033[30m"},
    {"mota", "\033[1m"},
    {"bold", "\033[1m"},
    {"halka", "\033[2m"},
    {"dim", "\033[2m"},
    {"tedha", "\033[3m"},
    {"italic", "\033[3m"},
    {"line", "\033[4m"},
    {"underline", "\033[4m"},
    {"/", "\033[0m"},
    {"reset", "\033[0m"},
    {"saaf", "\033[0m"}
};

std::string formatRichText(const std::string &input, bool useAnsi)
{
    if (!useAnsi)
    {
        return stripAnsi(formatRichText(input, true));
    }

    std::string result;
    bool styled = false;
    size_t i = 0;
    while (i < input.size())
    {
        if (input[i] == '[')
        {
            size_t closePos = input.find(']', i + 1);
            if (closePos != std::string::npos)
            {
                std::string tagContent = input.substr(i + 1, closePos - i - 1);
                std::vector<std::string> words;
                std::stringstream ss(tagContent);
                std::string word;
                while (ss >> word)
                {
                    for (char &c : word) c = std::tolower(static_cast<unsigned char>(c));
                    words.push_back(word);
                }

                bool allValid = !words.empty();
                for (const auto &w : words)
                {
                    if (RICH_TAGS.find(w) == RICH_TAGS.end())
                    {
                        allValid = false;
                        break;
                    }
                }

                if (allValid)
                {
                    for (const auto &w : words)
                    {
                        result += RICH_TAGS.at(w);
                        if (w != "/" && w != "reset" && w != "saaf")
                        {
                            styled = true;
                        }
                    }
                    i = closePos + 1;
                    continue;
                }
            }
        }
        result += input[i++];
    }

    if (styled && (result.size() < 4 || result.substr(result.size() - 4) != "\033[0m"))
    {
        result += "\033[0m";
    }

    return result;
}

static int codepointWidth(uint32_t cp)
{
    if (cp == 0 || cp == 0x200B || cp == 0x200C || cp == 0x200D) return 0;
    if ((cp >= 0x0300 && cp <= 0x036F) || (cp >= 0x1AB0 && cp <= 0x1AFF) ||
        (cp >= 0x1DC0 && cp <= 0x1DFF) || (cp >= 0x20D0 && cp <= 0x20FF) ||
        (cp >= 0xFE00 && cp <= 0xFE0F) || (cp >= 0xFE20 && cp <= 0xFE2F) ||
        (cp >= 0xE0100 && cp <= 0xE01EF))
    {
        return 0; // Combining mark / variation selector
    }
    if (cp == 0x2614 || cp == 0x2615 || (cp >= 0x231A && cp <= 0x231B) ||
        (cp >= 0x23E9 && cp <= 0x23EC) || cp == 0x23F0 || cp == 0x23F3 ||
        (cp >= 0x25FD && cp <= 0x25FE) || (cp >= 0x2600 && cp <= 0x27BF) ||
        (cp >= 0x1100 && cp <= 0x115F) || (cp >= 0x2E80 && cp <= 0xA4CF) ||
        (cp >= 0xAC00 && cp <= 0xD7A3) || (cp >= 0xF900 && cp <= 0xFAFF) ||
        (cp >= 0xFE10 && cp <= 0xFE19) || (cp >= 0xFE30 && cp <= 0xFE6F) ||
        (cp >= 0xFF01 && cp <= 0xFF60) || (cp >= 0xFFE0 && cp <= 0xFFE6) ||
        (cp >= 0x1F000 && cp <= 0x1FAFF) || (cp >= 0x20000 && cp <= 0x3FFFF))
    {
        return 2;
    }
    return 1;
}

size_t visibleLength(const std::string &str)
{
    std::string stripped = stripAnsi(formatRichText(str, true));
    size_t width = 0;
    size_t i = 0;
    while (i < stripped.size())
    {
        unsigned char c = static_cast<unsigned char>(stripped[i]);
        uint32_t cp = c;
        size_t bytes = 1;
        if (c < 0x80)
        {
            bytes = 1;
            cp = c;
        }
        else if ((c & 0xE0) == 0xC0 && i + 1 < stripped.size())
        {
            bytes = 2;
            cp = ((c & 0x1F) << 6) | (static_cast<unsigned char>(stripped[i + 1]) & 0x3F);
        }
        else if ((c & 0xF0) == 0xE0 && i + 2 < stripped.size())
        {
            bytes = 3;
            cp = ((c & 0x0F) << 12) |
                 ((static_cast<unsigned char>(stripped[i + 1]) & 0x3F) << 6) |
                 (static_cast<unsigned char>(stripped[i + 2]) & 0x3F);
        }
        else if ((c & 0xF8) == 0xF0 && i + 3 < stripped.size())
        {
            bytes = 4;
            cp = ((c & 0x07) << 18) |
                 ((static_cast<unsigned char>(stripped[i + 1]) & 0x3F) << 12) |
                 ((static_cast<unsigned char>(stripped[i + 2]) & 0x3F) << 6) |
                 (static_cast<unsigned char>(stripped[i + 3]) & 0x3F);
        }
        width += codepointWidth(cp);
        i += bytes;
    }
    return width;
}

namespace MemeJson
{
    inline std::string stringify(const Value &val)
    {
        Value v = val.deref();
        return std::visit(overload{
            [](std::monostate) -> std::string { return "null"; },
            [](long long n) -> std::string { return std::to_string(n); },
            [](double d) -> std::string {
                std::string s = std::to_string(d);
                s.erase(s.find_last_not_of('0') + 1, std::string::npos);
                if (s.back() == '.') s += "0";
                return s;
            },
            [](bool b) -> std::string { return b ? "true" : "false"; },
            [](const std::string &str) -> std::string {
                std::string out = "\"";
                for (char c : str)
                {
                    if (c == '"') out += "\\\"";
                    else if (c == '\\') out += "\\\\";
                    else if (c == '\b') out += "\\b";
                    else if (c == '\f') out += "\\f";
                    else if (c == '\n') out += "\\n";
                    else if (c == '\r') out += "\\r";
                    else if (c == '\t') out += "\\t";
                    else out += c;
                }
                out += "\"";
                return out;
            },
            [](const std::shared_ptr<MemeList> &list) -> std::string {
                if (!list) return "[]";
                std::string out = "[";
                for (size_t i = 0; i < list->elements.size(); ++i)
                {
                    if (i > 0) out += ", ";
                    out += stringify(list->elements[i]);
                }
                out += "]";
                return out;
            },
            [](const std::shared_ptr<MemeMap> &map) -> std::string {
                if (!map) return "{}";
                std::string out = "{";
                for (size_t i = 0; i < map->keyOrder.size(); ++i)
                {
                    if (i > 0) out += ", ";
                    const auto &k = map->keyOrder[i];
                    out += "\"" + k + "\": " + stringify(map->entries.at(k));
                }
                out += "}";
                return out;
            },
            [](const auto &) -> std::string {
                return "null";
            }
        }, v.data);
    }

    class Parser
    {
        const std::string &src;
        size_t pos = 0;

        void skipWhitespace()
        {
            while (pos < src.size() && (src[pos] == ' ' || src[pos] == '\t' || src[pos] == '\r' || src[pos] == '\n'))
                pos++;
        }

        char peek()
        {
            skipWhitespace();
            return pos < src.size() ? src[pos] : '\0';
        }

        char get()
        {
            skipWhitespace();
            return pos < src.size() ? src[pos++] : '\0';
        }

    public:
        Parser(const std::string &src) : src(src) {}

        Value parseValue()
        {
            char c = peek();
            if (c == 'n') {
                if (pos + 4 <= src.size() && src.substr(pos, 4) == "null") { pos += 4; return Value(std::monostate{}); }
            }
            if (c == 't') {
                if (pos + 4 <= src.size() && src.substr(pos, 4) == "true") { pos += 4; return Value(true); }
            }
            if (c == 'f') {
                if (pos + 5 <= src.size() && src.substr(pos, 5) == "false") { pos += 5; return Value(false); }
            }
            if (c == '"') {
                return parseString();
            }
            if (c == '[') {
                return parseArray();
            }
            if (c == '{') {
                return parseObject();
            }
            if (c == '-' || (c >= '0' && c <= '9')) {
                return parseNumber();
            }
            throw std::runtime_error("JSON kharab hai! Unexpected character at position " + std::to_string(pos));
        }

        Value parseString()
        {
            get(); // consume '"'
            std::string s;
            while (pos < src.size())
            {
                char c = src[pos++];
                if (c == '"') return Value(s);
                if (c == '\\' && pos < src.size())
                {
                    char esc = src[pos++];
                    if (esc == '"') s += '"';
                    else if (esc == '\\') s += '\\';
                    else if (esc == '/') s += '/';
                    else if (esc == 'b') s += '\b';
                    else if (esc == 'f') s += '\f';
                    else if (esc == 'n') s += '\n';
                    else if (esc == 'r') s += '\r';
                    else if (esc == 't') s += '\t';
                    else s += esc;
                }
                else
                {
                    s += c;
                }
            }
            throw std::runtime_error("JSON string adhura reh gaya!");
        }

        Value parseArray()
        {
            get(); // consume '['
            auto list = std::make_shared<MemeList>();
            if (peek() == ']') { get(); return Value(list); }
            while (true)
            {
                list->elements.push_back(parseValue());
                char c = peek();
                if (c == ',') { get(); continue; }
                if (c == ']') { get(); break; }
                throw std::runtime_error("JSON array format galat hai! Expected ',' ya ']' mila: '" + std::string(1, c) + "'");
            }
            return Value(list);
        }

        Value parseObject()
        {
            get(); // consume '{'
            auto map = std::make_shared<MemeMap>();
            if (peek() == '}') { get(); return Value(map); }
            while (true)
            {
                if (peek() != '"')
                    throw std::runtime_error("JSON object mein key string honi chahiye!");
                Value keyVal = parseString();
                std::string k = std::get<std::string>(keyVal.data);
                if (get() != ':')
                    throw std::runtime_error("JSON key ke baad ':' lagao!");
                Value v = parseValue();
                map->set(k, std::move(v));
                char c = peek();
                if (c == ',') { get(); continue; }
                if (c == '}') { get(); break; }
                throw std::runtime_error("JSON object format galat hai! Expected ',' ya '}' mila: '" + std::string(1, c) + "'");
            }
            return Value(map);
        }

        Value parseNumber()
        {
            size_t start = pos;
            if (pos < src.size() && src[pos] == '-') pos++;
            bool isFloat = false;
            while (pos < src.size() && ((src[pos] >= '0' && src[pos] <= '9') || src[pos] == '.' || src[pos] == 'e' || src[pos] == 'E' || src[pos] == '+' || src[pos] == '-'))
            {
                if (src[pos] == '.' || src[pos] == 'e' || src[pos] == 'E') isFloat = true;
                pos++;
            }
            std::string numStr = src.substr(start, pos - start);
            try
            {
                if (isFloat)
                {
                    return Value(std::stod(numStr));
                }
                else
                {
                    return Value(static_cast<long long>(std::stoll(numStr)));
                }
            }
            catch (...)
            {
                throw std::runtime_error("JSON number samajh nahi aaya: '" + numStr + "'");
            }
        }
    };
}

std::string instanceToString(const std::shared_ptr<Instance> &instance)
{
    return instance ? instance->toString() : "<object>";
}

std::string listToString(const std::shared_ptr<MemeList> &list)
{
    if (!list)
        return "[]";

    const void *ptr = static_cast<const void *>(list.get());
    if (activePrintPointers.find(ptr) != activePrintPointers.end())
    {
        return "[...]";
    }

    activePrintPointers.insert(ptr);
    std::string result = "[";
    for (size_t i = 0; i < list->elements.size(); ++i)
    {
        if (i > 0)
            result += ", ";
        Value el = list->elements[i].deref();
        if (std::holds_alternative<std::string>(el.data))
        {
            result += "\"" + std::get<std::string>(el.data) + "\"";
        }
        else
        {
            result += el.toString();
        }
    }
    result += "]";
    activePrintPointers.erase(ptr);
    return result;
}

std::string mapToString(const std::shared_ptr<MemeMap> &map)
{
    if (!map)
        return "{}";

    if (map->has("__is_error__"))
    {
        if (map->has("baat")) return map->entries["baat"].deref().toString();
        if (map->has("message")) return map->entries["message"].deref().toString();
    }

    const void *ptr = static_cast<const void *>(map.get());
    if (activePrintPointers.find(ptr) != activePrintPointers.end())
    {
        return "{...}";
    }

    activePrintPointers.insert(ptr);
    std::string result = "{";
    for (size_t i = 0; i < map->keyOrder.size(); ++i)
    {
        if (i > 0)
            result += ", ";
        const std::string &k = map->keyOrder[i];
        result += "\"" + k + "\": ";
        auto it = map->entries.find(k);
        if (it != map->entries.end())
        {
            Value val = it->second.deref();
            if (std::holds_alternative<std::string>(val.data))
            {
                result += "\"" + std::get<std::string>(val.data) + "\"";
            }
            else
            {
                result += val.toString();
            }
        }
        else
        {
            result += "nalla";
        }
    }
    result += "}";
    activePrintPointers.erase(ptr);
    return result;
}

class NativeMethodCallable : public Callable
{
private:
    std::string name;
    int minArgs;
    int maxArgs;
    std::function<Value(Interpreter &, const std::vector<Value> &)> fn;

public:
    NativeMethodCallable(std::string name, int arity, std::function<Value(Interpreter &, const std::vector<Value> &)> fn)
        : name(std::move(name)), minArgs(arity), maxArgs(arity), fn(std::move(fn)) {}

    NativeMethodCallable(std::string name, int minArgs, int maxArgs, std::function<Value(Interpreter &, const std::vector<Value> &)> fn)
        : name(std::move(name)), minArgs(minArgs), maxArgs(maxArgs), fn(std::move(fn)) {}

    size_t arity() const override
    {
        return minArgs >= 0 ? static_cast<size_t>(minArgs) : 0;
    }

    size_t minArity() const override
    {
        return minArgs >= 0 ? static_cast<size_t>(minArgs) : 0;
    }

    size_t maxArity() const override
    {
        return maxArgs >= 0 ? static_cast<size_t>(maxArgs) : 0;
    }

    bool isVariadic() const override
    {
        return maxArgs < 0;
    }

    bool checkArity(size_t count) const override
    {
        if (minArgs >= 0 && static_cast<int>(count) < minArgs) return false;
        if (maxArgs >= 0 && static_cast<int>(count) > maxArgs) return false;
        return true;
    }

    Value call(Interpreter &interpreter, const std::vector<Value> &args) override
    {
        if (!checkArity(args.size()))
        {
            throw std::runtime_error(
                "Gaddari korbe! Method '" + name + "' ne " + std::to_string(minArgs) +
                " arguments maange the, aur tumne " + std::to_string(args.size()) + " chipka diye!");
        }
        return fn(interpreter, args);
    }

    std::string toString() const override
    {
        return "<builtin method " + name + ">";
    }
};

#include "../bridge/EcosystemBridge.h"
#include "../ui/ShadcnComponents.h"

static std::shared_ptr<MemeMap> createHisabModule()
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;
    m->set("pi", Value(3.14159265358979323846));
    m->set("e", Value(2.71828182845904523536));

    m->set("gol", Value(std::make_shared<NativeMethodCallable>("gol", 1, [](Interpreter &, const std::vector<Value> &args) {
        return Value(static_cast<long long>(std::round(toDouble(args[0]))));
    })));

    m->set("niche", Value(std::make_shared<NativeMethodCallable>("niche", 1, [](Interpreter &, const std::vector<Value> &args) {
        return Value(static_cast<long long>(std::floor(toDouble(args[0]))));
    })));

    m->set("upar", Value(std::make_shared<NativeMethodCallable>("upar", 1, [](Interpreter &, const std::vector<Value> &args) {
        return Value(static_cast<long long>(std::ceil(toDouble(args[0]))));
    })));

    m->set("jadd", Value(std::make_shared<NativeMethodCallable>("jadd", 1, [](Interpreter &, const std::vector<Value> &args) {
        double d = toDouble(args[0]);
        if (d < 0.0)
        {
            throw std::runtime_error("Negative number ka square root nikaaloge? Math fail ho gaya!");
        }
        return Value(std::sqrt(d));
    })));

    m->set("taqat", Value(std::make_shared<NativeMethodCallable>("taqat", 2, [](Interpreter &, const std::vector<Value> &args) {
        return Value(std::pow(toDouble(args[0]), toDouble(args[1])));
    })));

    m->set("halka", Value(std::make_shared<NativeMethodCallable>("halka", 2, [](Interpreter &, const std::vector<Value> &args) {
        double a = toDouble(args[0]);
        double b = toDouble(args[1]);
        if (std::holds_alternative<long long>(args[0].deref().data) && std::holds_alternative<long long>(args[1].deref().data))
        {
            return Value(std::min(std::get<long long>(args[0].deref().data), std::get<long long>(args[1].deref().data)));
        }
        return Value(std::min(a, b));
    })));

    m->set("bhaari", Value(std::make_shared<NativeMethodCallable>("bhaari", 2, [](Interpreter &, const std::vector<Value> &args) {
        double a = toDouble(args[0]);
        double b = toDouble(args[1]);
        if (std::holds_alternative<long long>(args[0].deref().data) && std::holds_alternative<long long>(args[1].deref().data))
        {
            return Value(std::max(std::get<long long>(args[0].deref().data), std::get<long long>(args[1].deref().data)));
        }
        return Value(std::max(a, b));
    })));

    m->set("positive", Value(std::make_shared<NativeMethodCallable>("positive", 1, [](Interpreter &, const std::vector<Value> &args) {
        Value v = args[0].deref();
        if (std::holds_alternative<long long>(v.data))
        {
            return Value(std::abs(std::get<long long>(v.data)));
        }
        return Value(std::abs(toDouble(v)));
    })));

    m->set("tukka", Value(std::make_shared<NativeMethodCallable>("tukka", 0, [](Interpreter &, const std::vector<Value> &) {
        static std::mt19937 gen(std::random_device{}());
        static std::uniform_real_distribution<double> dist(0.0, 1.0);
        return Value(dist(gen));
    })));

    m->set("tukka_num", Value(std::make_shared<NativeMethodCallable>("tukka_num", 2, [](Interpreter &, const std::vector<Value> &args) {
        long long mn = toLongLong(args[0]);
        long long mx = toLongLong(args[1]);
        if (mn > mx) std::swap(mn, mx);
        static std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<long long> dist(mn, mx);
        return Value(dist(gen));
    })));

    return m;
}

static std::shared_ptr<MemeMap> createWaqtModule()
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    m->set("ab", Value(std::make_shared<NativeMethodCallable>("ab", 0, [](Interpreter &, const std::vector<Value> &) {
        auto now = std::chrono::system_clock::now();
        double sec = std::chrono::duration<double>(now.time_since_epoch()).count();
        return Value(sec);
    })));

    auto sleepFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        double sec = toDouble(args[0]);
        if (sec > 0.0)
        {
#ifndef __EMSCRIPTEN__
            std::this_thread::sleep_for(std::chrono::duration<double>(sec));
#endif
        }
        return Value(std::monostate{});
    };

    m->set("so_ja", Value(std::make_shared<NativeMethodCallable>("so_ja", 1, sleepFn)));
    m->set("ruk", Value(std::make_shared<NativeMethodCallable>("ruk", 1, sleepFn)));

    return m;
}

static std::shared_ptr<MemeMap> createJsonModule()
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    auto parseFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string jsonStr = args[0].deref().toString();
        MemeJson::Parser parser(jsonStr);
        return parser.parseValue();
    };

    auto stringifyFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        return Value(MemeJson::stringify(args[0]));
    };

    m->set("samajh", Value(std::make_shared<NativeMethodCallable>("samajh", 1, parseFn)));
    m->set("padh", Value(std::make_shared<NativeMethodCallable>("padh", 1, parseFn)));
    m->set("likh", Value(std::make_shared<NativeMethodCallable>("likh", 1, stringifyFn)));
    m->set("bana", Value(std::make_shared<NativeMethodCallable>("bana", 1, stringifyFn)));

    return m;
}

static std::string categorizeErrorMessage(const std::string &msg)
{
    std::string lower = msg;
    for (char &c : lower) c = (char)std::tolower((unsigned char)c);
    if (lower.find("network") != std::string::npos || lower.find("http") != std::string::npos ||
        lower.find("jaal") != std::string::npos || lower.find("fetch") != std::string::npos ||
        lower.find("curl") != std::string::npos || lower.find("connection") != std::string::npos ||
        lower.find("url") != std::string::npos)
    {
        return "NetworkLafda";
    }
    if (lower.find("file") != std::string::npos || lower.find("path") != std::string::npos ||
        lower.find("directory") != std::string::npos || lower.find("folder") != std::string::npos ||
        lower.find("rasta") != std::string::npos || lower.find("kholo") != std::string::npos ||
        lower.find("gayab ho gayi") != std::string::npos)
    {
        return "FileLafda";
    }
    if (lower.find("zero") != std::string::npos || lower.find("divide") != std::string::npos ||
        lower.find("math") != std::string::npos || lower.find("hisab") != std::string::npos ||
        lower.find("ganit") != std::string::npos)
    {
        return "MathLafda";
    }
    if (lower.find("type") != std::string::npos || lower.find("pata_laga") != std::string::npos ||
        lower.find("aukaat") != std::string::npos || lower.find("gaddari") != std::string::npos)
    {
        return "TypeLafda";
    }
    return "RuntimeLafda";
}

static std::shared_ptr<MemeMap> createErrorMap(const std::string &message, const std::string &type, long long line = 0)
{
    auto m = std::make_shared<MemeMap>();
    m->set("__is_error__", Value(true));
    m->set("baat", Value(message));
    m->set("message", Value(message));
    m->set("kism", Value(type));
    m->set("type", Value(type));
    m->set("line", Value(line));
    return m;
}

static std::vector<std::string> parseCsvLine(const std::string &line)
{
    std::vector<std::string> fields;
    std::string field;
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); ++i)
    {
        char c = line[i];
        if (inQuotes)
        {
            if (c == '"')
            {
                if (i + 1 < line.size() && line[i + 1] == '"')
                {
                    field += '"';
                    i++;
                }
                else
                {
                    inQuotes = false;
                }
            }
            else
            {
                field += c;
            }
        }
        else
        {
            if (c == '"')
            {
                inQuotes = true;
            }
            else if (c == ',')
            {
                fields.push_back(field);
                field.clear();
            }
            else
            {
                field += c;
            }
        }
    }
    fields.push_back(field);
    return fields;
}

static std::string escapeCsvField(const std::string &f)
{
    if (f.find(',') != std::string::npos || f.find('"') != std::string::npos || f.find('\n') != std::string::npos || f.find('\r') != std::string::npos)
    {
        std::string res = "\"";
        for (char c : f)
        {
            if (c == '"') res += "\"\"";
            else res += c;
        }
        res += "\"";
        return res;
    }
    return f;
}

static bool wildcardMatch(const std::string &pattern, const std::string &str)
{
    if (pattern.empty()) return str.empty();
    if (pattern == "*") return true;
    size_t p = 0, s = 0, starIdx = std::string::npos, match = 0;
    while (s < str.size())
    {
        if (p < pattern.size() && (pattern[p] == '?' || pattern[p] == str[s]))
        {
            p++;
            s++;
        }
        else if (p < pattern.size() && pattern[p] == '*')
        {
            starIdx = p++;
            match = s;
        }
        else if (starIdx != std::string::npos)
        {
            p = starIdx + 1;
            match++;
            s = match;
        }
        else
        {
            return false;
        }
    }
    while (p < pattern.size() && pattern[p] == '*') p++;
    return p == pattern.size();
}

static std::shared_ptr<MemeMap> createFileModule()
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    // padh / read
    auto readFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string path = args[0].deref().toString();
        std::ifstream f(path);
        if (!f.is_open())
        {
            throw std::runtime_error("File gayab ho gayi! Path: '" + path + "'");
        }
        std::stringstream ss;
        ss << f.rdbuf();
        return Value(ss.str());
    };
    m->set("padh", Value(std::make_shared<NativeMethodCallable>("padh", 1, readFn)));
    m->set("read", m->entries["padh"]);

    // likh / write
    auto writeFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string path = args[0].deref().toString();
        std::string content = args[1].deref().toString();
        std::ofstream f(path, std::ios::trunc);
        if (!f.is_open())
        {
            throw std::runtime_error("File kholne mein lafda ho gaya! Path: '" + path + "'");
        }
        f << content;
        return Value(true);
    };
    m->set("likh", Value(std::make_shared<NativeMethodCallable>("likh", 2, writeFn)));
    m->set("write", m->entries["likh"]);

    // jod / append
    auto appendFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string path = args[0].deref().toString();
        std::string content = args[1].deref().toString();
        std::ofstream f(path, std::ios::app);
        if (!f.is_open())
        {
            throw std::runtime_error("File kholne mein lafda ho gaya! Path: '" + path + "'");
        }
        f << content;
        return Value(true);
    };
    m->set("jod", Value(std::make_shared<NativeMethodCallable>("jod", 2, appendFn)));
    m->set("append", m->entries["jod"]);

    // hai_kya / exists
    auto existsFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string path = args[0].deref().toString();
        std::error_code ec;
        return Value(std::filesystem::exists(path, ec));
    };
    m->set("hai_kya", Value(std::make_shared<NativeMethodCallable>("hai_kya", 1, existsFn)));
    m->set("exists", m->entries["hai_kya"]);

    // lines / read_lines
    auto linesFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string path = args[0].deref().toString();
        std::ifstream f(path);
        if (!f.is_open())
        {
            throw std::runtime_error("File gayab ho gayi! Path: '" + path + "'");
        }
        auto list = std::make_shared<MemeList>();
        std::string line;
        while (std::getline(f, line))
        {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            list->elements.push_back(Value(line));
        }
        return Value(list);
    };
    m->set("lines", Value(std::make_shared<NativeMethodCallable>("lines", 1, linesFn)));
    m->set("read_lines", m->entries["lines"]);

    // likh_lines / write_lines
    auto writeLinesFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string path = args[0].deref().toString();
        Value linesVal = args[1].deref();
        if (!linesVal.isList())
        {
            throw std::runtime_error("likh_lines ko list chahiye!");
        }
        std::ofstream f(path, std::ios::trunc);
        if (!f.is_open())
        {
            throw std::runtime_error("File kholne mein lafda ho gaya! Path: '" + path + "'");
        }
        for (const auto &elem : linesVal.getList()->elements)
        {
            f << elem.deref().toString() << "\n";
        }
        return Value(true);
    };
    m->set("likh_lines", Value(std::make_shared<NativeMethodCallable>("likh_lines", 2, writeLinesFn)));
    m->set("write_lines", m->entries["likh_lines"]);

    // padh_json / read_json
    auto readJsonFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string path = args[0].deref().toString();
        std::ifstream f(path);
        if (!f.is_open())
        {
            throw std::runtime_error("File gayab ho gayi! Path: '" + path + "'");
        }
        std::stringstream ss;
        ss << f.rdbuf();
        MemeJson::Parser parser(ss.str());
        return parser.parseValue();
    };
    m->set("padh_json", Value(std::make_shared<NativeMethodCallable>("padh_json", 1, readJsonFn)));
    m->set("read_json", m->entries["padh_json"]);

    // likh_json / write_json
    auto writeJsonFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string path = args[0].deref().toString();
        std::string jsonStr = MemeJson::stringify(args[1]);
        std::ofstream f(path, std::ios::trunc);
        if (!f.is_open())
        {
            throw std::runtime_error("File kholne mein lafda ho gaya! Path: '" + path + "'");
        }
        f << jsonStr;
        return Value(true);
    };
    m->set("likh_json", Value(std::make_shared<NativeMethodCallable>("likh_json", 2, 3, writeJsonFn)));
    m->set("write_json", m->entries["likh_json"]);

    // padh_csv / read_csv
    auto readCsvFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string path = args[0].deref().toString();
        std::ifstream f(path);
        if (!f.is_open())
        {
            throw std::runtime_error("File gayab ho gayi! Path: '" + path + "'");
        }
        auto list = std::make_shared<MemeList>();
        std::string headerLine;
        if (!std::getline(f, headerLine)) return Value(list);
        if (!headerLine.empty() && headerLine.back() == '\r') headerLine.pop_back();
        auto headers = parseCsvLine(headerLine);

        std::string line;
        while (std::getline(f, line))
        {
            if (line.empty()) continue;
            if (line.back() == '\r') line.pop_back();
            auto cells = parseCsvLine(line);
            auto rowMap = std::make_shared<MemeMap>();
            for (size_t i = 0; i < headers.size(); ++i)
            {
                std::string cellVal = (i < cells.size()) ? cells[i] : "";
                rowMap->set(headers[i], Value(cellVal));
            }
            list->elements.push_back(Value(rowMap));
        }
        return Value(list);
    };
    m->set("padh_csv", Value(std::make_shared<NativeMethodCallable>("padh_csv", 1, readCsvFn)));
    m->set("read_csv", m->entries["padh_csv"]);

    // likh_csv / write_csv
    auto writeCsvFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string path = args[0].deref().toString();
        Value rowsVal = args[1].deref();
        if (!rowsVal.isList())
        {
            throw std::runtime_error("likh_csv ko list chahiye!");
        }
        auto list = rowsVal.getList();
        std::ofstream f(path, std::ios::trunc);
        if (!f.is_open())
        {
            throw std::runtime_error("File kholne mein lafda ho gaya! Path: '" + path + "'");
        }
        if (list->elements.empty()) return Value(true);

        Value first = list->elements[0].deref();
        if (first.isMap())
        {
            auto map0 = first.getMap();
            std::vector<std::string> keys = map0->keyOrder;
            for (size_t i = 0; i < keys.size(); ++i)
            {
                if (i > 0) f << ",";
                f << escapeCsvField(keys[i]);
            }
            f << "\n";
            for (const auto &rowElem : list->elements)
            {
                Value rv = rowElem.deref();
                if (rv.isMap())
                {
                    auto rm = rv.getMap();
                    for (size_t i = 0; i < keys.size(); ++i)
                    {
                        if (i > 0) f << ",";
                        std::string cell = rm->has(keys[i]) ? rm->entries[keys[i]].deref().toString() : "";
                        f << escapeCsvField(cell);
                    }
                    f << "\n";
                }
            }
        }
        return Value(true);
    };
    m->set("likh_csv", Value(std::make_shared<NativeMethodCallable>("likh_csv", 2, writeCsvFn)));
    m->set("write_csv", m->entries["likh_csv"]);

    // nakal / copy
    auto copyFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string src = args[0].deref().toString();
        std::string dst = args[1].deref().toString();
        std::error_code ec;
        std::filesystem::copy(src, dst, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing, ec);
        if (ec)
        {
            throw std::runtime_error("Copy karne mein lafda: " + ec.message());
        }
        return Value(true);
    };
    m->set("nakal", Value(std::make_shared<NativeMethodCallable>("nakal", 2, copyFn)));
    m->set("copy", m->entries["nakal"]);

    // hila / move / rename
    auto moveFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string src = args[0].deref().toString();
        std::string dst = args[1].deref().toString();
        std::error_code ec;
        std::filesystem::rename(src, dst, ec);
        if (ec)
        {
            throw std::runtime_error("Move karne mein lafda: " + ec.message());
        }
        return Value(true);
    };
    m->set("hila", Value(std::make_shared<NativeMethodCallable>("hila", 2, moveFn)));
    m->set("move", m->entries["hila"]);
    m->set("rename", m->entries["hila"]);

    // mita / remove / delete
    auto removeFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string path = args[0].deref().toString();
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
        return Value(true);
    };
    m->set("mita", Value(std::make_shared<NativeMethodCallable>("mita", 1, removeFn)));
    m->set("remove", m->entries["mita"]);
    m->set("delete", m->entries["mita"]);

    // kundli / stat
    auto statFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string path = args[0].deref().toString();
        std::error_code ec;
        auto statMap = std::make_shared<MemeMap>();
        statMap->set("exists", Value(std::filesystem::exists(path, ec)));
        statMap->set("is_file", Value(std::filesystem::is_regular_file(path, ec)));
        statMap->set("is_dir", Value(std::filesystem::is_directory(path, ec)));
        auto sz = std::filesystem::is_regular_file(path, ec) ? std::filesystem::file_size(path, ec) : 0ULL;
        statMap->set("size", Value(static_cast<long long>(sz)));
        statMap->set("ext", Value(std::filesystem::path(path).extension().string()));
        auto ftime = std::filesystem::last_write_time(path, ec);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
        long long ts = std::chrono::duration_cast<std::chrono::seconds>(sctp.time_since_epoch()).count();
        statMap->set("modified", Value(ts));
        return Value(statMap);
    };
    m->set("kundli", Value(std::make_shared<NativeMethodCallable>("kundli", 1, statFn)));
    m->set("stat", m->entries["kundli"]);

    // folder_bana / mkdir
    auto mkdirFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string path = args[0].deref().toString();
        std::error_code ec;
        std::filesystem::create_directories(path, ec);
        return Value(true);
    };
    m->set("folder_bana", Value(std::make_shared<NativeMethodCallable>("folder_bana", 1, mkdirFn)));
    m->set("mkdir", m->entries["folder_bana"]);

    // dekho / listdir / readdir
    auto listdirFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string path = args.empty() ? "." : args[0].deref().toString();
        auto list = std::make_shared<MemeList>();
        std::error_code ec;
        for (const auto &entry : std::filesystem::directory_iterator(path, ec))
        {
            list->elements.push_back(Value(entry.path().filename().string()));
        }
        return Value(list);
    };
    m->set("dekho", Value(std::make_shared<NativeMethodCallable>("dekho", 0, 1, listdirFn)));
    m->set("listdir", m->entries["dekho"]);
    m->set("readdir", m->entries["dekho"]);

    // khoj / glob
    auto globFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string dir = args.empty() ? "." : args[0].deref().toString();
        std::string pat = args.size() > 1 ? args[1].deref().toString() : "*";
        auto list = std::make_shared<MemeList>();
        std::error_code ec;
        for (const auto &entry : std::filesystem::recursive_directory_iterator(dir, ec))
        {
            if (wildcardMatch(pat, entry.path().filename().string()))
            {
                list->elements.push_back(Value(entry.path().string()));
            }
        }
        return Value(list);
    };
    m->set("khoj", Value(std::make_shared<NativeMethodCallable>("khoj", 1, 2, globFn)));
    m->set("glob", m->entries["khoj"]);

    // temp_dir
    auto tempDirFn = [](Interpreter &, const std::vector<Value> &) -> Value {
        return Value(std::filesystem::temp_directory_path().string());
    };
    m->set("temp_dir", Value(std::make_shared<NativeMethodCallable>("temp_dir", 0, tempDirFn)));

    return m;
}

static std::shared_ptr<MemeMap> createPathModule()
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    // jod / join
    auto joinFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::filesystem::path p;
        for (const auto &a : args)
        {
            p /= a.deref().toString();
        }
        return Value(p.string());
    };
    m->set("jod", Value(std::make_shared<NativeMethodCallable>("jod", 1, -1, joinFn)));
    m->set("join", m->entries["jod"]);

    // naam / basename
    auto baseFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        return Value(std::filesystem::path(args[0].deref().toString()).filename().string());
    };
    m->set("naam", Value(std::make_shared<NativeMethodCallable>("naam", 1, baseFn)));
    m->set("basename", m->entries["naam"]);

    // bina_ext / stem
    auto stemFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        return Value(std::filesystem::path(args[0].deref().toString()).stem().string());
    };
    m->set("bina_ext", Value(std::make_shared<NativeMethodCallable>("bina_ext", 1, stemFn)));
    m->set("stem", m->entries["bina_ext"]);

    // folder / dirname
    auto dirFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        return Value(std::filesystem::path(args[0].deref().toString()).parent_path().string());
    };
    m->set("folder", Value(std::make_shared<NativeMethodCallable>("folder", 1, dirFn)));
    m->set("dirname", m->entries["folder"]);

    // ext / extname
    auto extFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        return Value(std::filesystem::path(args[0].deref().toString()).extension().string());
    };
    m->set("ext", Value(std::make_shared<NativeMethodCallable>("ext", 1, extFn)));
    m->set("extname", m->entries["ext"]);

    // asli / resolve
    auto resolveFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string p = args[0].deref().toString();
        std::error_code ec;
        auto canonical = std::filesystem::weakly_canonical(std::filesystem::absolute(p), ec);
        return Value(canonical.string());
    };
    m->set("asli", Value(std::make_shared<NativeMethodCallable>("asli", 1, resolveFn)));
    m->set("resolve", m->entries["asli"]);

    // pakka_rasta / is_abs
    auto isAbsFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        return Value(std::filesystem::path(args[0].deref().toString()).is_absolute());
    };
    m->set("pakka_rasta", Value(std::make_shared<NativeMethodCallable>("pakka_rasta", 1, isAbsFn)));
    m->set("is_abs", m->entries["pakka_rasta"]);

    return m;
}

// ==========================================
// PROMISE / VAADA RUNTIME
// ==========================================
struct PromiseState
{
    mutable std::mutex mtx;
    std::condition_variable cv;
    bool resolved = false;
    bool rejected = false;
    Value result;
    Value errorVal;
    std::vector<std::function<void(const Value &)>> thenCallbacks;
    std::vector<std::function<void(const Value &)>> catchCallbacks;

    void resolve(const Value &val)
    {
        std::vector<std::function<void(const Value &)>> cbs;
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (resolved || rejected) return;
            resolved = true;
            result = val;
            cbs = thenCallbacks;
        }
        cv.notify_all();
        for (auto &cb : cbs)
        {
            try { cb(val); } catch (...) {}
        }
    }

    void reject(const Value &err)
    {
        std::vector<std::function<void(const Value &)>> cbs;
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (resolved || rejected) return;
            rejected = true;
            errorVal = err;
            cbs = catchCallbacks;
        }
        cv.notify_all();
        for (auto &cb : cbs)
        {
            try { cb(err); } catch (...) {}
        }
    }

    Value waitAndGet()
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return resolved || rejected; });
        if (rejected)
        {
            throw MemeUserException(errorVal, 0);
        }
        return result;
    }

    bool isDone() const
    {
        std::lock_guard<std::mutex> lock(mtx);
        return resolved || rejected;
    }
};

static std::shared_ptr<MemeMap> createPromiseMap(const std::shared_ptr<PromiseState> &state, Interpreter *interp);

static std::shared_ptr<MemeMap> createPromiseMap(const std::shared_ptr<PromiseState> &state, Interpreter *interp)
{
    (void)interp;
    auto m = std::make_shared<MemeMap>();
    m->set("__is_promise__", Value(true));

    // .phir / .then
    m->set("phir", Value(std::make_shared<NativeMethodCallable>("phir", 1, [state](Interpreter &callerInterp, const std::vector<Value> &args) -> Value {
        Value fnVal = args[0];
        auto nextState = std::make_shared<PromiseState>();
        auto nextMap = createPromiseMap(nextState, &callerInterp);

        auto handleSuccess = [fnVal, nextState](const Value &val) {
            try
            {
                Interpreter childInterp;
                Value callbackRes;
                if (fnVal.isCallable())
                {
                    callbackRes = fnVal.getCallable()->call(childInterp, {val});
                }
                else
                {
                    callbackRes = val;
                }

                if (callbackRes.deref().isMap() && callbackRes.deref().getMap()->has("__is_promise__"))
                {
                    Value jodVal = callbackRes.deref().getMap()->entries["jod"];
                    if (jodVal.isCallable())
                    {
                        callbackRes = jodVal.getCallable()->call(childInterp, {});
                    }
                }
                nextState->resolve(callbackRes);
            }
            catch (const MemeUserException &e)
            {
                nextState->reject(e.value);
            }
            catch (const std::exception &e)
            {
                nextState->reject(Value(createErrorMap(e.what(), categorizeErrorMessage(e.what()), 0)));
            }
            catch (...)
            {
                nextState->reject(Value(createErrorMap("Promise callback failed", "RuntimeLafda", 0)));
            }
        };

        std::lock_guard<std::mutex> lock(state->mtx);
        if (state->resolved)
        {
            handleSuccess(state->result);
        }
        else if (state->rejected)
        {
            nextState->reject(state->errorVal);
        }
        else
        {
            state->thenCallbacks.push_back(handleSuccess);
            state->catchCallbacks.push_back([nextState](const Value &err) {
                nextState->reject(err);
            });
        }

        return Value(nextMap);
    })));
    m->set("then", m->entries["phir"]);

    // .pakad / .catch
    m->set("pakad", Value(std::make_shared<NativeMethodCallable>("pakad", 1, [state](Interpreter &callerInterp, const std::vector<Value> &args) -> Value {
        Value fnVal = args[0];
        auto nextState = std::make_shared<PromiseState>();
        auto nextMap = createPromiseMap(nextState, &callerInterp);

        auto handleCatch = [fnVal, nextState](const Value &err) {
            try
            {
                Interpreter childInterp;
                Value callbackRes;
                if (fnVal.isCallable())
                {
                    callbackRes = fnVal.getCallable()->call(childInterp, {err});
                }
                else
                {
                    callbackRes = err;
                }
                nextState->resolve(callbackRes);
            }
            catch (const MemeUserException &e)
            {
                nextState->reject(e.value);
            }
            catch (const std::exception &e)
            {
                nextState->reject(Value(createErrorMap(e.what(), categorizeErrorMessage(e.what()), 0)));
            }
            catch (...)
            {
                nextState->reject(Value(createErrorMap("Promise catch failed", "RuntimeLafda", 0)));
            }
        };

        std::lock_guard<std::mutex> lock(state->mtx);
        if (state->rejected)
        {
            handleCatch(state->errorVal);
        }
        else if (state->resolved)
        {
            nextState->resolve(state->result);
        }
        else
        {
            state->thenCallbacks.push_back([nextState](const Value &val) {
                nextState->resolve(val);
            });
            state->catchCallbacks.push_back(handleCatch);
        }

        return Value(nextMap);
    })));
    m->set("catch", m->entries["pakad"]);

    // .pura_hua / .is_resolved / .done
    m->set("pura_hua", Value(std::make_shared<NativeMethodCallable>("pura_hua", 0, [state](Interpreter &, const std::vector<Value> &) {
        return Value(state->isDone());
    })));
    m->set("is_resolved", m->entries["pura_hua"]);
    m->set("done", m->entries["pura_hua"]);

    // .jod / .wait / .await
    m->set("jod", Value(std::make_shared<NativeMethodCallable>("jod", 0, [state](Interpreter &, const std::vector<Value> &) {
        return state->waitAndGet();
    })));
    m->set("wait", m->entries["jod"]);
    m->set("await", m->entries["jod"]);

    return m;
}

static std::shared_ptr<MemeMap> createVaadaModule()
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    // Vaada.de(val) / Promise.resolve(val)
    auto resolveFn = [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        Value val = args.empty() ? Value(std::monostate{}) : args[0];
        auto state = std::make_shared<PromiseState>();
        state->resolve(val);
        return Value(createPromiseMap(state, &interp));
    };
    m->set("de", Value(std::make_shared<NativeMethodCallable>("de", 0, 1, resolveFn)));
    m->set("resolve", m->entries["de"]);

    // Vaada.radh(err) / Promise.reject(err)
    auto rejectFn = [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        Value err = args.empty() ? Value(createErrorMap("Vaada rejected", "UserLafda", 0)) : args[0];
        auto state = std::make_shared<PromiseState>();
        state->reject(err);
        return Value(createPromiseMap(state, &interp));
    };
    m->set("radh", Value(std::make_shared<NativeMethodCallable>("radh", 0, 1, rejectFn)));
    m->set("reject", m->entries["radh"]);

    // Vaada.ruk(ms) / Promise.sleep(ms)
    auto sleepFn = [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        long long ms = args.empty() ? 0 : toLongLong(args[0]);
        auto state = std::make_shared<PromiseState>();
        auto pMap = createPromiseMap(state, &interp);
#if defined(__EMSCRIPTEN__)
        state->resolve(Value(true));
#else
        std::thread([state, ms]() {
            if (ms > 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            }
            state->resolve(Value(true));
        }).detach();
#endif
        return Value(pMap);
    };
    m->set("ruk", Value(std::make_shared<NativeMethodCallable>("ruk", 1, sleepFn)));
    m->set("sleep", m->entries["ruk"]);

    // Vaada.sab(promisesList) / Promise.all(promisesList)
    auto allFn = [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        if (args.empty() || !args[0].deref().isList())
        {
            throw std::runtime_error("Vaada.sab ko promises ki list chahiye!");
        }
        auto inList = args[0].deref().getList();
        auto resultState = std::make_shared<PromiseState>();
        auto pMap = createPromiseMap(resultState, &interp);

        if (inList->elements.empty())
        {
            resultState->resolve(Value(std::make_shared<MemeList>()));
            return Value(pMap);
        }

        auto promises = inList->elements;
#if defined(__EMSCRIPTEN__)
        auto outList = std::make_shared<MemeList>();
        for (auto &pVal : promises)
        {
            Value d = pVal.deref();
            if (d.isMap() && d.getMap()->has("__is_promise__"))
            {
                outList->elements.push_back(d.getMap()->entries["jod"].getCallable()->call(interp, {}));
            }
            else
            {
                outList->elements.push_back(d);
            }
        }
        resultState->resolve(Value(outList));
#else
        std::thread([promises, resultState]() {
            try
            {
                auto outList = std::make_shared<MemeList>();
                Interpreter subInterp;
                for (auto &pVal : promises)
                {
                    Value d = pVal.deref();
                    if (d.isMap() && d.getMap()->has("__is_promise__"))
                    {
                        Value res = d.getMap()->entries["jod"].getCallable()->call(subInterp, {});
                        outList->elements.push_back(res);
                    }
                    else
                    {
                        outList->elements.push_back(d);
                    }
                }
                resultState->resolve(Value(outList));
            }
            catch (const MemeUserException &e)
            {
                resultState->reject(e.value);
            }
            catch (const std::exception &e)
            {
                resultState->reject(Value(createErrorMap(e.what(), categorizeErrorMessage(e.what()), 0)));
            }
        }).detach();
#endif
        return Value(pMap);
    };
    m->set("sab", Value(std::make_shared<NativeMethodCallable>("sab", 1, allFn)));
    m->set("all", m->entries["sab"]);

    // Vaada.race(promisesList) / Promise.race(promisesList)
    auto raceFn = [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        if (args.empty() || !args[0].deref().isList())
        {
            throw std::runtime_error("Vaada.race ko promises ki list chahiye!");
        }
        auto inList = args[0].deref().getList();
        auto resultState = std::make_shared<PromiseState>();
        auto pMap = createPromiseMap(resultState, &interp);

        if (inList->elements.empty())
        {
            resultState->resolve(Value(std::monostate{}));
            return Value(pMap);
        }

        auto promises = inList->elements;
#if defined(__EMSCRIPTEN__)
        Value d = promises[0].deref();
        if (d.isMap() && d.getMap()->has("__is_promise__"))
            resultState->resolve(d.getMap()->entries["jod"].getCallable()->call(interp, {}));
        else
            resultState->resolve(d);
#else
        for (auto &pVal : promises)
        {
            std::thread([pVal, resultState]() {
                try
                {
                    Value d = pVal.deref();
                    if (d.isMap() && d.getMap()->has("__is_promise__"))
                    {
                        Interpreter subInterp;
                        Value res = d.getMap()->entries["jod"].getCallable()->call(subInterp, {});
                        resultState->resolve(res);
                    }
                    else
                    {
                        resultState->resolve(d);
                    }
                }
                catch (const MemeUserException &e)
                {
                    resultState->reject(e.value);
                }
                catch (const std::exception &e)
                {
                    resultState->reject(Value(createErrorMap(e.what(), categorizeErrorMessage(e.what()), 0)));
                }
            }).detach();
        }
#endif
        return Value(pMap);
    };
    m->set("race", Value(std::make_shared<NativeMethodCallable>("race", 1, raceFn)));

    return m;
}

// ==========================================
// CHANNEL & MUTEX & THREAD RUNTIME
// ==========================================
struct ChannelState
{
    mutable std::mutex mtx;
    std::condition_variable cv_read;
    std::condition_variable cv_write;
    std::queue<Value> queue;
    size_t capacity = 0;
    bool closed = false;

    void send(const Value &val)
    {
        std::unique_lock<std::mutex> lock(mtx);
        if (closed)
        {
            throw std::runtime_error("Band naali (closed channel) mein maal nahi bhej sakte!");
        }
        if (capacity > 0)
        {
            cv_write.wait(lock, [this]() { return queue.size() < capacity || closed; });
            if (closed) throw std::runtime_error("Band naali (closed channel) mein maal nahi bhej sakte!");
        }
        queue.push(val);
        cv_read.notify_one();
    }

    Value recv()
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv_read.wait(lock, [this]() { return !queue.empty() || closed; });
        if (queue.empty() && closed)
        {
            return Value(std::monostate{});
        }
        Value val = queue.front();
        queue.pop();
        if (capacity > 0)
        {
            cv_write.notify_one();
        }
        return val;
    }

    bool isEmpty() const
    {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.empty();
    }

    size_t size() const
    {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.size();
    }

    void close()
    {
        {
            std::lock_guard<std::mutex> lock(mtx);
            closed = true;
        }
        cv_read.notify_all();
        cv_write.notify_all();
    }
};

static std::shared_ptr<MemeMap> createChannelMap(const std::shared_ptr<ChannelState> &ch)
{
    auto m = std::make_shared<MemeMap>();
    m->set("__is_channel__", Value(true));

    m->set("bhej", Value(std::make_shared<NativeMethodCallable>("bhej", 1, [ch](Interpreter &, const std::vector<Value> &args) {
        ch->send(args[0]);
        return Value(true);
    })));
    m->set("send", m->entries["bhej"]);

    m->set("le", Value(std::make_shared<NativeMethodCallable>("le", 0, [ch](Interpreter &, const std::vector<Value> &) {
        return ch->recv();
    })));
    m->set("recv", m->entries["le"]);
    m->set("receive", m->entries["le"]);

    m->set("khali_hai", Value(std::make_shared<NativeMethodCallable>("khali_hai", 0, [ch](Interpreter &, const std::vector<Value> &) {
        return Value(ch->isEmpty());
    })));
    m->set("is_empty", m->entries["khali_hai"]);

    m->set("lambai", Value(std::make_shared<NativeMethodCallable>("lambai", 0, [ch](Interpreter &, const std::vector<Value> &) {
        return Value(static_cast<long long>(ch->size()));
    })));
    m->set("size", m->entries["lambai"]);
    m->set("len", m->entries["lambai"]);

    m->set("band", Value(std::make_shared<NativeMethodCallable>("band", 0, [ch](Interpreter &, const std::vector<Value> &) {
        ch->close();
        return Value(true);
    })));
    m->set("close", m->entries["band"]);

    return m;
}

static std::shared_ptr<MemeMap> createMutexMap()
{
    auto m = std::make_shared<MemeMap>();
    m->set("__is_mutex__", Value(true));
    auto mtx = std::make_shared<std::recursive_mutex>();

    m->set("band", Value(std::make_shared<NativeMethodCallable>("band", 0, [mtx](Interpreter &, const std::vector<Value> &) {
        mtx->lock();
        return Value(true);
    })));
    m->set("lock", m->entries["band"]);

    m->set("khol", Value(std::make_shared<NativeMethodCallable>("khol", 0, [mtx](Interpreter &, const std::vector<Value> &) {
        mtx->unlock();
        return Value(true);
    })));
    m->set("unlock", m->entries["khol"]);

    m->set("surakshit", Value(std::make_shared<NativeMethodCallable>("surakshit", 1, [mtx](Interpreter &interp, const std::vector<Value> &args) -> Value {
        Value fnVal = args[0];
        std::lock_guard<std::recursive_mutex> lock(*mtx);
        if (fnVal.isCallable())
            return fnVal.getCallable()->call(interp, {});
        return Value(std::monostate{});
    })));
    m->set("with_lock", m->entries["surakshit"]);

    return m;
}

struct ThreadHandleState
{
    mutable std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    Value result;
    Value errorVal;
    bool hasError = false;
    std::string threadId;

    Value join()
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return done; });
        if (hasError)
        {
            throw MemeUserException(errorVal, 0);
        }
        return result;
    }

    bool isAlive() const
    {
        std::lock_guard<std::mutex> lock(mtx);
        return !done;
    }
};

static std::shared_ptr<MemeMap> createThreadMap(const std::shared_ptr<ThreadHandleState> &tState)
{
    auto m = std::make_shared<MemeMap>();
    m->set("__is_thread__", Value(true));
    m->set("id", Value(tState->threadId));

    m->set("chal_raha", Value(std::make_shared<NativeMethodCallable>("chal_raha", 0, [tState](Interpreter &, const std::vector<Value> &) {
        return Value(tState->isAlive());
    })));
    m->set("is_alive", m->entries["chal_raha"]);

    m->set("jod", Value(std::make_shared<NativeMethodCallable>("jod", 0, [tState](Interpreter &, const std::vector<Value> &) {
        return tState->join();
    })));
    m->set("join", m->entries["jod"]);

    return m;
}

static std::shared_ptr<MemeMap> createDhagaModule()
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    // dhaga.chala(fn, ...args) / thread.spawn(fn, ...args)
    auto chalaFn = [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        if (args.empty()) throw std::runtime_error("dhaga.chala ko function chahiye!");
        Value fnVal = args[0];
        std::vector<Value> fnArgs(args.begin() + 1, args.end());

        auto tState = std::make_shared<ThreadHandleState>();
        auto globalsPtr = interp.getGlobals();
        static std::atomic<uint64_t> threadCounter{1};
        tState->threadId = "dhaga-" + std::to_string(threadCounter++);

#if defined(__EMSCRIPTEN__)
        try {
            Value res;
            if (fnVal.isCallable()) res = fnVal.getCallable()->call(interp, fnArgs);
            tState->result = res;
        } catch (const MemeUserException &e) {
            tState->hasError = true; tState->errorVal = e.value;
        } catch (const std::exception &e) {
            tState->hasError = true; tState->errorVal = Value(createErrorMap(e.what(), categorizeErrorMessage(e.what()), 0));
        }
        tState->done = true;
#else
        std::thread([globalsPtr, fnVal, fnArgs, tState]() {
            try {
                Interpreter childInterp;
                childInterp.setGlobals(globalsPtr);
                Value res;
                if (fnVal.isCallable()) res = fnVal.getCallable()->call(childInterp, fnArgs);
                {
                    std::lock_guard<std::mutex> lock(tState->mtx);
                    tState->result = res;
                    tState->done = true;
                }
                tState->cv.notify_all();
            } catch (const MemeUserException &e) {
                {
                    std::lock_guard<std::mutex> lock(tState->mtx);
                    tState->hasError = true;
                    tState->errorVal = e.value;
                    tState->done = true;
                }
                tState->cv.notify_all();
            } catch (const std::exception &e) {
                {
                    std::lock_guard<std::mutex> lock(tState->mtx);
                    tState->hasError = true;
                    tState->errorVal = Value(createErrorMap(e.what(), categorizeErrorMessage(e.what()), 0));
                    tState->done = true;
                }
                tState->cv.notify_all();
            }
        }).detach();
#endif
        return Value(createThreadMap(tState));
    };
    m->set("chala", Value(std::make_shared<NativeMethodCallable>("chala", 1, -1, chalaFn)));
    m->set("spawn", m->entries["chala"]);

    // dhaga.sab(fnList) / thread.all(fnList)
    auto allFn = [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        if (args.empty() || !args[0].deref().isList())
        {
            throw std::runtime_error("dhaga.sab ko functions ki list chahiye!");
        }
        auto inList = args[0].deref().getList();
        auto outList = std::make_shared<MemeList>();
        std::vector<std::shared_ptr<ThreadHandleState>> handles;
        auto globalsPtr = interp.getGlobals();

        for (auto &fnVal : inList->elements)
        {
            auto tState = std::make_shared<ThreadHandleState>();
            static std::atomic<uint64_t> tc{1};
            tState->threadId = "dhaga-" + std::to_string(tc++);
            handles.push_back(tState);
#if defined(__EMSCRIPTEN__)
            if (fnVal.isCallable()) tState->result = fnVal.getCallable()->call(interp, {});
            tState->done = true;
#else
            std::thread([globalsPtr, fnVal, tState]() {
                try {
                    Interpreter childInterp;
                    childInterp.setGlobals(globalsPtr);
                    Value res;
                    if (fnVal.isCallable()) res = fnVal.getCallable()->call(childInterp, {});
                    {
                        std::lock_guard<std::mutex> lock(tState->mtx);
                        tState->result = res;
                        tState->done = true;
                    }
                    tState->cv.notify_all();
                } catch (const MemeUserException &e) {
                    {
                        std::lock_guard<std::mutex> lock(tState->mtx);
                        tState->hasError = true;
                        tState->errorVal = e.value;
                        tState->done = true;
                    }
                    tState->cv.notify_all();
                } catch (const std::exception &e) {
                    {
                        std::lock_guard<std::mutex> lock(tState->mtx);
                        tState->hasError = true;
                        tState->errorVal = Value(createErrorMap(e.what(), categorizeErrorMessage(e.what()), 0));
                        tState->done = true;
                    }
                    tState->cv.notify_all();
                }
            }).detach();
#endif
        }

        for (auto &h : handles)
        {
            outList->elements.push_back(h->join());
        }
        return Value(outList);
    };
    m->set("sab", Value(std::make_shared<NativeMethodCallable>("sab", 1, allFn)));
    m->set("all", m->entries["sab"]);

    // dhaga.tala() / thread.mutex()
    auto mutexFn = [](Interpreter &, const std::vector<Value> &) -> Value {
        return Value(createMutexMap());
    };
    m->set("tala", Value(std::make_shared<NativeMethodCallable>("tala", 0, mutexFn)));
    m->set("mutex", m->entries["tala"]);

    // dhaga.naali(capacity = 0) / thread.channel(capacity = 0)
    auto channelFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto ch = std::make_shared<ChannelState>();
        if (!args.empty())
        {
            ch->capacity = static_cast<size_t>(std::max(0LL, toLongLong(args[0])));
        }
        return Value(createChannelMap(ch));
    };
    m->set("naali", Value(std::make_shared<NativeMethodCallable>("naali", 0, 1, channelFn)));
    m->set("channel", m->entries["naali"]);

    // dhaga.cores() / thread.cores()
    auto coresFn = [](Interpreter &, const std::vector<Value> &) -> Value {
        return Value(static_cast<long long>(std::max(1u, std::thread::hardware_concurrency())));
    };
    m->set("cores", Value(std::make_shared<NativeMethodCallable>("cores", 0, coresFn)));

    // dhaga.so_ja(ms) / thread.sleep(ms)
    auto sleepFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        long long ms = args.empty() ? 0 : toLongLong(args[0]);
        if (ms > 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        }
        return Value(true);
    };
    m->set("so_ja", Value(std::make_shared<NativeMethodCallable>("so_ja", 1, sleepFn)));
    m->set("sleep", m->entries["so_ja"]);

    // dhaga.id()
    auto idFn = [](Interpreter &, const std::vector<Value> &) -> Value {
        std::stringstream ss;
        ss << std::this_thread::get_id();
        return Value(ss.str());
    };
    m->set("id", Value(std::make_shared<NativeMethodCallable>("id", 0, idFn)));

    return m;
}

// Subprocess struct
struct ProcessHandleState
{
    mutable std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    long long exitCode = -1;
    long long pid = 0;
    std::string stdoutData;

    long long wait()
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return done; });
        return exitCode;
    }
};

static std::shared_ptr<MemeMap> createSubprocessMap(const std::shared_ptr<ProcessHandleState> &pState)
{
    auto m = std::make_shared<MemeMap>();
    m->set("__is_process__", Value(true));
    m->set("pid", Value(pState->pid));

    m->set("ruk", Value(std::make_shared<NativeMethodCallable>("ruk", 0, [pState](Interpreter &, const std::vector<Value> &) {
        return Value(pState->wait());
    })));
    m->set("wait", m->entries["ruk"]);

    m->set("code", Value(std::make_shared<NativeMethodCallable>("code", 0, [pState](Interpreter &, const std::vector<Value> &) {
        pState->wait();
        return Value(pState->exitCode);
    })));

    m->set("out", Value(std::make_shared<NativeMethodCallable>("out", 0, [pState](Interpreter &, const std::vector<Value> &) {
        pState->wait();
        return Value(pState->stdoutData);
    })));
    m->set("stdout", m->entries["out"]);

    m->set("maar", Value(std::make_shared<NativeMethodCallable>("maar", 0, [pState](Interpreter &, const std::vector<Value> &) {
        pState->done = true;
        pState->cv.notify_all();
        return Value(true);
    })));
    m->set("kill", m->entries["maar"]);

    return m;
}

static std::shared_ptr<MemeMap> createKaamgarModule()
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    // kaamgar.sab_pe(fn, list, workers = 4) / mp.map(fn, list, workers = 4)
    auto mapFn = [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        if (args.size() < 2 || !args[1].deref().isList())
        {
            throw std::runtime_error("kaamgar.sab_pe ko function aur list chahiye!");
        }
        Value fnVal = args[0];
        auto inList = args[1].deref().getList();
        size_t n = inList->elements.size();
        auto outList = std::make_shared<MemeList>();
        if (n == 0) return Value(outList);

        size_t numWorkers = (args.size() > 2) ? static_cast<size_t>(std::max(1LL, toLongLong(args[2]))) : 4;
        numWorkers = std::min(numWorkers, n);

#if defined(__EMSCRIPTEN__)
        for (size_t i = 0; i < n; ++i)
        {
            if (fnVal.isCallable())
                outList->elements.push_back(fnVal.getCallable()->call(interp, {inList->elements[i]}));
            else
                outList->elements.push_back(inList->elements[i]);
        }
#else
        std::vector<Value> results(n);
        std::atomic<size_t> nextIdx{0};
        auto globalsPtr = interp.getGlobals();
        std::vector<std::thread> workers;

        for (size_t w = 0; w < numWorkers; ++w)
        {
            workers.emplace_back([globalsPtr, fnVal, inList, &results, &nextIdx, n]() {
                Interpreter child;
                child.setGlobals(globalsPtr);
                while (true)
                {
                    size_t i = nextIdx.fetch_add(1);
                    if (i >= n) break;
                    try
                    {
                        if (fnVal.isCallable())
                        {
                            results[i] = fnVal.getCallable()->call(child, {inList->elements[i]});
                        }
                        else
                        {
                            results[i] = inList->elements[i];
                        }
                    }
                    catch (const std::exception &e)
                    {
                        results[i] = Value(createErrorMap(e.what(), categorizeErrorMessage(e.what()), 0));
                    }
                    catch (...)
                    {
                        results[i] = Value(createErrorMap("Worker task failed", "RuntimeLafda", 0));
                    }
                }
            });
        }

        for (auto &w : workers)
        {
            w.join();
        }

        for (size_t i = 0; i < n; ++i)
        {
            outList->elements.push_back(results[i]);
        }
#endif
        return Value(outList);
    };
    m->set("sab_pe", Value(std::make_shared<NativeMethodCallable>("sab_pe", 2, 3, mapFn)));
    m->set("map", m->entries["sab_pe"]);

    // kaamgar.pool(workers = 4) / mp.pool(workers = 4)
    auto poolFn = [mapFn](Interpreter &, const std::vector<Value> &args) -> Value {
        long long workers = args.empty() ? 4 : toLongLong(args[0]);
        auto poolMap = std::make_shared<MemeMap>();
        poolMap->set("workers", Value(workers));

        poolMap->set("sab_pe", Value(std::make_shared<NativeMethodCallable>("sab_pe", 2, [mapFn, workers](Interpreter &interp, const std::vector<Value> &a) {
            std::vector<Value> fullArgs = {a[0], a[1], Value(workers)};
            return mapFn(interp, fullArgs);
        })));
        poolMap->set("map", poolMap->entries["sab_pe"]);

        poolMap->set("band", Value(std::make_shared<NativeMethodCallable>("band", 0, [](Interpreter &, const std::vector<Value> &) {
            return Value(true);
        })));
        poolMap->set("close", poolMap->entries["band"]);

        return Value(poolMap);
    };
    m->set("pool", Value(std::make_shared<NativeMethodCallable>("pool", 0, 1, poolFn)));

    // kaamgar.alag(cmd, args = []) / mp.spawn(cmd, args = [])
    auto spawnProcessFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        if (args.empty()) throw std::runtime_error("kaamgar.alag ko command string chahiye!");
        std::string cmd = args[0].deref().toString();
        if (args.size() > 1 && args[1].deref().isList())
        {
            for (const auto &a : args[1].deref().getList()->elements)
            {
                cmd += " \"" + a.deref().toString() + "\"";
            }
        }

        auto pState = std::make_shared<ProcessHandleState>();
        static std::atomic<long long> pidCounter{1000};
        pState->pid = pidCounter++;

#if defined(__EMSCRIPTEN__)
        pState->exitCode = -1;
        pState->stdoutData = "Subprocess not supported in WebAssembly environment";
        pState->done = true;
#else
        std::thread([cmd, pState]() {
            std::string out;
#if defined(_WIN32)
            FILE *pipe = _popen(cmd.c_str(), "r");
#else
            FILE *pipe = popen(cmd.c_str(), "r");
#endif
            if (pipe)
            {
                char buffer[1024];
                while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
                {
                    out += buffer;
                }
#if defined(_WIN32)
                int code = _pclose(pipe);
#else
                int status = pclose(pipe);
                int code = WIFEXITED(status) ? WEXITSTATUS(status) : status;
#endif
                std::lock_guard<std::mutex> lock(pState->mtx);
                pState->exitCode = code;
                pState->stdoutData = out;
                pState->done = true;
            }
            else
            {
                std::lock_guard<std::mutex> lock(pState->mtx);
                pState->exitCode = -1;
                pState->stdoutData = "";
                pState->done = true;
            }
            pState->cv.notify_all();
        }).detach();
#endif
        return Value(createSubprocessMap(pState));
    };
    m->set("alag", Value(std::make_shared<NativeMethodCallable>("alag", 1, 2, spawnProcessFn)));
    m->set("spawn", m->entries["alag"]);

    return m;
}

// ==========================================
// HTTP & FETCH CLIENT RUNTIME
// ==========================================
static std::shared_ptr<MemeMap> createResponseMap(int status, const std::string &body, const std::map<std::string, std::string> &headers, const std::string &url, const std::string &method, long long timeMs)
{
    auto m = std::make_shared<MemeMap>();
    m->set("status", Value(static_cast<long long>(status)));
    bool isOk = (status >= 200 && status < 300);
    m->set("ok", Value(isOk));
    m->set("sahi", Value(isOk));
    m->set("url", Value(url));
    m->set("method", Value(method));
    m->set("raw", Value(body));
    m->set("body", Value(body));
    m->set("time_ms", Value(timeMs));

    auto headersMap = std::make_shared<MemeMap>();
    for (const auto &[k, v] : headers)
    {
        headersMap->set(k, Value(v));
    }
    m->set("headers", Value(headersMap));

    // Try parsing as JSON for convenience
    try
    {
        MemeJson::Parser parser(body);
        Value parsed = parser.parseValue();
        m->set("data", parsed);
        m->set("maal", parsed);
    }
    catch (...)
    {
        m->set("data", Value(body));
        m->set("maal", Value(body));
    }

    // Methods
    m->set("json", Value(std::make_shared<NativeMethodCallable>("json", 0, [body](Interpreter &, const std::vector<Value> &) {
        MemeJson::Parser parser(body);
        return parser.parseValue();
    })));

    m->set("text", Value(std::make_shared<NativeMethodCallable>("text", 0, [body](Interpreter &, const std::vector<Value> &) {
        return Value(body);
    })));

    m->set("raise_for_status", Value(std::make_shared<NativeMethodCallable>("raise_for_status", 0, [status, body](Interpreter &, const std::vector<Value> &) {
        if (status < 200 || status >= 300)
        {
            throw std::runtime_error("HTTP Lafda! Status " + std::to_string(status) + ": " + body);
        }
        return Value(true);
    })));

    return m;
}

static std::shared_ptr<MemeMap> executeFetchSync(const std::string &url, const Value &optsVal)
{
    auto startTime = std::chrono::steady_clock::now();
    std::string method = "GET";
    std::map<std::string, std::string> reqHeaders;
    std::string bodyStr;
    long long timeoutMs = 10000;

    std::string finalUrl = url;

    if (optsVal.deref().isMap())
    {
        auto opts = optsVal.deref().getMap();
        if (opts->has("method")) method = opts->entries["method"].deref().toString();
        for (char &c : method) c = (char)std::toupper((unsigned char)c);

        if (opts->has("timeout")) timeoutMs = toLongLong(opts->entries["timeout"]);

        if (opts->has("headers") && opts->entries["headers"].deref().isMap())
        {
            auto hm = opts->entries["headers"].deref().getMap();
            for (const auto &k : hm->keyOrder)
            {
                reqHeaders[k] = hm->entries[k].deref().toString();
            }
        }

        if (opts->has("auth"))
        {
            reqHeaders["Authorization"] = "Bearer " + opts->entries["auth"].deref().toString();
        }
        else if (opts->has("token"))
        {
            reqHeaders["Authorization"] = "Bearer " + opts->entries["token"].deref().toString();
        }

        if (opts->has("params") && opts->entries["params"].deref().isMap())
        {
            auto pm = opts->entries["params"].deref().getMap();
            std::string q;
            for (size_t i = 0; i < pm->keyOrder.size(); ++i)
            {
                const auto &k = pm->keyOrder[i];
                std::string v = pm->entries[k].deref().toString();
                q += (i == 0 ? "" : "&") + k + "=" + v;
            }
            if (!q.empty())
            {
                finalUrl += (finalUrl.find('?') == std::string::npos ? "?" : "&") + q;
            }
        }

        if (opts->has("body"))
        {
            Value b = opts->entries["body"].deref();
            if (b.isMap() || b.isList())
            {
                bodyStr = MemeJson::stringify(b);
                if (reqHeaders.find("Content-Type") == reqHeaders.end())
                    reqHeaders["Content-Type"] = "application/json";
            }
            else
            {
                bodyStr = b.toString();
            }
        }
        else if (opts->has("json"))
        {
            bodyStr = MemeJson::stringify(opts->entries["json"]);
            if (reqHeaders.find("Content-Type") == reqHeaders.end())
                reqHeaders["Content-Type"] = "application/json";
        }
    }

    // Mock URL handling for instant 0ms offline testing
    if (finalUrl.rfind("mock://", 0) == 0)
    {
        std::string mockPath = finalUrl.substr(7);
        int status = 200;
        std::string respBody;
        std::map<std::string, std::string> respHeaders = {{"Content-Type", "application/json"}, {"X-Mock", "true"}};

        if (mockPath.rfind("status/", 0) == 0)
        {
            try {
                status = std::stoi(mockPath.substr(7));
            } catch (...) {
                status = 400;
            }
            respBody = "{\"status\": " + std::to_string(status) + ", \"error\": \"Mock status error\"}";
        }
        else if (mockPath == "echo" || mockPath.find("echo") != std::string::npos)
        {
            respBody = "{\"echo\": true, \"method\": \"" + method + "\", \"url\": \"" + finalUrl + "\", \"body\": \"" + bodyStr + "\"}";
        }
        else if (mockPath == "lafda" || mockPath == "error")
        {
            status = 500;
            respBody = "{\"error\": \"Mock Internal Server Error\"}";
        }
        else
        {
            status = 200;
            respBody = "{\"success\": true, \"url\": \"" + finalUrl + "\", \"method\": \"" + method + "\", \"message\": \"mock response\"}";
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
        return createResponseMap(status, respBody, respHeaders, finalUrl, method, std::max(1LL, elapsed));
    }

    // Real HTTP request using curl CLI
    int status = 200;
    std::string respBody;
    std::map<std::string, std::string> respHeaders;

#if defined(__EMSCRIPTEN__)
    status = 200;
    respBody = "{\"success\": true, \"url\": \"" + finalUrl + "\", \"browser\": true}";
#else
    std::string curlCmd = "curl -s -i -X " + method;
    double secTimeout = timeoutMs / 1000.0;
    curlCmd += " -m " + std::to_string(secTimeout > 0 ? secTimeout : 10.0);

    for (const auto &[k, v] : reqHeaders)
    {
        curlCmd += " -H \"" + k + ": " + v + "\"";
    }

    if (!bodyStr.empty())
    {
        std::string escBody;
        for (char c : bodyStr)
        {
            if (c == '"') escBody += "\\\"";
            else if (c == '\\') escBody += "\\\\";
            else escBody += c;
        }
        curlCmd += " --data-raw \"" + escBody + "\"";
    }
    curlCmd += " \"" + finalUrl + "\" 2>/dev/null";

    std::string rawOutput;
#if defined(_WIN32)
    FILE *pipe = _popen(curlCmd.c_str(), "r");
#else
    FILE *pipe = popen(curlCmd.c_str(), "r");
#endif
    if (!pipe)
    {
        throw std::runtime_error("curl chalaane mein network lafda ho gaya!");
    }
    char buf[1024];
    while (fgets(buf, sizeof(buf), pipe) != nullptr)
    {
        rawOutput += buf;
    }
#if defined(_WIN32)
    _pclose(pipe);
#else
    pclose(pipe);
#endif

    // Parse HTTP headers and body
    size_t headerEnd = rawOutput.find("\r\n\r\n");
    size_t delimLen = 4;
    if (headerEnd == std::string::npos)
    {
        headerEnd = rawOutput.find("\n\n");
        delimLen = 2;
    }

    if (headerEnd != std::string::npos)
    {
        std::string headerSection = rawOutput.substr(0, headerEnd);
        respBody = rawOutput.substr(headerEnd + delimLen);

        std::stringstream ss(headerSection);
        std::string line;
        if (std::getline(ss, line))
        {
            // E.g. HTTP/1.1 200 OK
            std::stringstream statusLine(line);
            std::string proto;
            int code = 200;
            statusLine >> proto >> code;
            if (code > 0) status = code;
        }
        while (std::getline(ss, line))
        {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            auto colon = line.find(':');
            if (colon != std::string::npos)
            {
                std::string k = line.substr(0, colon);
                std::string v = line.substr(colon + 1);
                while (!v.empty() && (v.front() == ' ' || v.front() == '\t')) v.erase(v.begin());
                respHeaders[k] = v;
            }
        }
    }
    else
    {
        respBody = rawOutput;
    }
#endif

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
    return createResponseMap(status, respBody, respHeaders, finalUrl, method, std::max(1LL, elapsed));
}

static Value executeFetchAsync(Interpreter &interp, const std::string &url, const Value &optsVal)
{
    auto pState = std::make_shared<PromiseState>();
    auto pMap = createPromiseMap(pState, &interp);

#if defined(__EMSCRIPTEN__)
    try {
        auto resp = executeFetchSync(url, optsVal);
        pState->resolve(Value(resp));
    } catch (const std::exception &e) {
        pState->reject(Value(createErrorMap(e.what(), "NetworkLafda", 0)));
    }
#else
    std::thread([url, optsVal, pState]() {
        try {
            auto resp = executeFetchSync(url, optsVal);
            pState->resolve(Value(resp));
        } catch (const std::exception &e) {
            pState->reject(Value(createErrorMap(e.what(), "NetworkLafda", 0)));
        } catch (...) {
            pState->reject(Value(createErrorMap("Fetch request failed", "NetworkLafda", 0)));
        }
    }).detach();
#endif

    return Value(pMap);
}

static std::shared_ptr<MemeMap> createJaalModule()
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    // jaal.get(url, opts = {})
    auto getFn = [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        std::string url = args[0].deref().toString();
        auto opts = std::make_shared<MemeMap>();
        opts->set("method", Value(std::string("GET")));
        if (args.size() > 1 && args[1].deref().isMap())
        {
            auto extra = args[1].deref().getMap();
            for (const auto &k : extra->keyOrder) opts->set(k, extra->entries[k]);
            opts->set("method", Value(std::string("GET")));
        }
        return executeFetchAsync(interp, url, Value(opts));
    };
    m->set("get", Value(std::make_shared<NativeMethodCallable>("get", 1, 2, getFn)));
    m->set("la", m->entries["get"]);

    // jaal.post(url, bodyOrOpts, opts = {})
    auto postFn = [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        std::string url = args[0].deref().toString();
        auto opts = std::make_shared<MemeMap>();
        opts->set("method", Value(std::string("POST")));
        if (args.size() > 1)
        {
            Value arg1 = args[1].deref();
            if (arg1.isMap() && (arg1.getMap()->has("headers") || arg1.getMap()->has("body")))
            {
                auto extra = arg1.getMap();
                for (const auto &k : extra->keyOrder) opts->set(k, extra->entries[k]);
            }
            else
            {
                opts->set("body", arg1);
            }
        }
        if (args.size() > 2 && args[2].deref().isMap())
        {
            auto extra = args[2].deref().getMap();
            for (const auto &k : extra->keyOrder) opts->set(k, extra->entries[k]);
        }
        opts->set("method", Value(std::string("POST")));
        return executeFetchAsync(interp, url, Value(opts));
    };
    m->set("post", Value(std::make_shared<NativeMethodCallable>("post", 1, 3, postFn)));
    m->set("bhej", m->entries["post"]);

    // jaal.put(url, bodyOrOpts, opts = {})
    auto putFn = [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        std::string url = args[0].deref().toString();
        auto opts = std::make_shared<MemeMap>();
        opts->set("method", Value(std::string("PUT")));
        if (args.size() > 1)
        {
            Value arg1 = args[1].deref();
            if (arg1.isMap() && (arg1.getMap()->has("headers") || arg1.getMap()->has("body")))
            {
                auto extra = arg1.getMap();
                for (const auto &k : extra->keyOrder) opts->set(k, extra->entries[k]);
            }
            else
            {
                opts->set("body", arg1);
            }
        }
        if (args.size() > 2 && args[2].deref().isMap())
        {
            auto extra = args[2].deref().getMap();
            for (const auto &k : extra->keyOrder) opts->set(k, extra->entries[k]);
        }
        opts->set("method", Value(std::string("PUT")));
        return executeFetchAsync(interp, url, Value(opts));
    };
    m->set("put", Value(std::make_shared<NativeMethodCallable>("put", 1, 3, putFn)));
    m->set("badal", m->entries["put"]);

    // jaal.patch(url, bodyOrOpts, opts = {})
    auto patchFn = [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        std::string url = args[0].deref().toString();
        auto opts = std::make_shared<MemeMap>();
        opts->set("method", Value(std::string("PATCH")));
        if (args.size() > 1)
        {
            Value arg1 = args[1].deref();
            if (arg1.isMap() && (arg1.getMap()->has("headers") || arg1.getMap()->has("body")))
            {
                auto extra = arg1.getMap();
                for (const auto &k : extra->keyOrder) opts->set(k, extra->entries[k]);
            }
            else
            {
                opts->set("body", arg1);
            }
        }
        if (args.size() > 2 && args[2].deref().isMap())
        {
            auto extra = args[2].deref().getMap();
            for (const auto &k : extra->keyOrder) opts->set(k, extra->entries[k]);
        }
        opts->set("method", Value(std::string("PATCH")));
        return executeFetchAsync(interp, url, Value(opts));
    };
    m->set("patch", Value(std::make_shared<NativeMethodCallable>("patch", 1, 3, patchFn)));
    m->set("sudhar", m->entries["patch"]);

    // jaal.delete(url, opts = {})
    auto deleteFn = [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        std::string url = args[0].deref().toString();
        auto opts = std::make_shared<MemeMap>();
        opts->set("method", Value(std::string("DELETE")));
        if (args.size() > 1 && args[1].deref().isMap())
        {
            auto extra = args[1].deref().getMap();
            for (const auto &k : extra->keyOrder) opts->set(k, extra->entries[k]);
            opts->set("method", Value(std::string("DELETE")));
        }
        return executeFetchAsync(interp, url, Value(opts));
    };
    m->set("delete", Value(std::make_shared<NativeMethodCallable>("delete", 1, 2, deleteFn)));
    m->set("hata", m->entries["delete"]);

    // jaal.head(url, opts = {})
    auto headFn = [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        std::string url = args[0].deref().toString();
        auto opts = std::make_shared<MemeMap>();
        opts->set("method", Value(std::string("HEAD")));
        if (args.size() > 1 && args[1].deref().isMap())
        {
            auto extra = args[1].deref().getMap();
            for (const auto &k : extra->keyOrder) opts->set(k, extra->entries[k]);
            opts->set("method", Value(std::string("HEAD")));
        }
        return executeFetchAsync(interp, url, Value(opts));
    };
    m->set("head", Value(std::make_shared<NativeMethodCallable>("head", 1, 2, headFn)));

    // jaal.options(url, opts = {})
    auto optionsFn = [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        std::string url = args[0].deref().toString();
        auto opts = std::make_shared<MemeMap>();
        opts->set("method", Value(std::string("OPTIONS")));
        if (args.size() > 1 && args[1].deref().isMap())
        {
            auto extra = args[1].deref().getMap();
            for (const auto &k : extra->keyOrder) opts->set(k, extra->entries[k]);
            opts->set("method", Value(std::string("OPTIONS")));
        }
        return executeFetchAsync(interp, url, Value(opts));
    };
    m->set("options", Value(std::make_shared<NativeMethodCallable>("options", 1, 2, optionsFn)));

    // jaal.download(url, destPath)
    auto downloadFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string url = args[0].deref().toString();
        std::string dest = args[1].deref().toString();
        auto res = executeFetchSync(url, Value(std::monostate{}));
        std::string body = res->entries["body"].deref().toString();
        std::ofstream f(dest, std::ios::binary | std::ios::trunc);
        if (!f.is_open())
        {
            throw std::runtime_error("Download file save karne mein lafda: " + dest);
        }
        f << body;
        return Value(true);
    };
    m->set("download", Value(std::make_shared<NativeMethodCallable>("download", 2, downloadFn)));

    // jaal.upload(url, filePath, opts = {})
    auto uploadFn = [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        std::string url = args[0].deref().toString();
        std::string filePath = args[1].deref().toString();
        std::ifstream f(filePath, std::ios::binary);
        if (!f.is_open())
        {
            throw std::runtime_error("Upload file gayab ho gayi: " + filePath);
        }
        std::stringstream ss;
        ss << f.rdbuf();
        auto opts = std::make_shared<MemeMap>();
        opts->set("method", Value(std::string("POST")));
        opts->set("body", Value(ss.str()));
        if (args.size() > 2 && args[2].deref().isMap())
        {
            auto extra = args[2].deref().getMap();
            for (const auto &k : extra->keyOrder) opts->set(k, extra->entries[k]);
        }
        opts->set("method", Value(std::string("POST")));
        return executeFetchAsync(interp, url, Value(opts));
    };
    m->set("upload", Value(std::make_shared<NativeMethodCallable>("upload", 2, 3, uploadFn)));

    return m;
}

static std::string repeatU8(const std::string &ch, size_t n)
{
    std::string res;
    for (size_t i = 0; i < n; ++i) res += ch;
    return res;
}

static std::string renderTable(const Value &data)
{
    Value d = data.deref();
    if (!d.isList())
    {
        throw std::runtime_error("Table (mez) banane ke liye list pass karo!");
    }
    auto list = d.getList();
    if (!list || list->elements.empty())
    {
        return "";
    }

    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> rows;

    Value firstElem = list->elements[0].deref();
    if (firstElem.isMap())
    {
        for (const auto &item : list->elements)
        {
            Value rowVal = item.deref();
            if (rowVal.isMap())
            {
                auto map = rowVal.getMap();
                for (const auto &k : map->keyOrder)
                {
                    if (std::find(headers.begin(), headers.end(), k) == headers.end())
                    {
                        headers.push_back(k);
                    }
                }
            }
        }

        for (const auto &item : list->elements)
        {
            Value rowVal = item.deref();
            std::vector<std::string> row;
            if (rowVal.isMap())
            {
                auto map = rowVal.getMap();
                for (const auto &h : headers)
                {
                    if (map->has(h))
                    {
                        row.push_back(map->entries[h].toString());
                    }
                    else
                    {
                        row.push_back("");
                    }
                }
            }
            rows.push_back(row);
        }
    }
    else if (firstElem.isList())
    {
        auto headerList = firstElem.getList();
        for (const auto &el : headerList->elements)
        {
            headers.push_back(el.toString());
        }
        for (size_t i = 1; i < list->elements.size(); ++i)
        {
            Value rowVal = list->elements[i].deref();
            std::vector<std::string> row;
            if (rowVal.isList())
            {
                auto rowList = rowVal.getList();
                for (const auto &el : rowList->elements)
                {
                    row.push_back(el.toString());
                }
            }
            while (row.size() < headers.size()) row.push_back("");
            rows.push_back(row);
        }
    }
    else
    {
        headers.push_back("Value");
        for (const auto &item : list->elements)
        {
            rows.push_back({item.toString()});
        }
    }

    if (headers.empty())
    {
        return "";
    }

    std::vector<size_t> colWidths(headers.size(), 0);
    for (size_t c = 0; c < headers.size(); ++c)
    {
        colWidths[c] = visibleLength(headers[c]);
    }
    for (const auto &r : rows)
    {
        for (size_t c = 0; c < headers.size() && c < r.size(); ++c)
        {
            colWidths[c] = std::max(colWidths[c], visibleLength(r[c]));
        }
    }

    std::stringstream ss;
    // Top border: ┌───┬───┐
    ss << "┌";
    for (size_t c = 0; c < headers.size(); ++c)
    {
        ss << repeatU8("─", colWidths[c] + 2);
        if (c + 1 < headers.size()) ss << "┬";
    }
    ss << "┐\n";

    // Header row
    ss << "│";
    for (size_t c = 0; c < headers.size(); ++c)
    {
        size_t pad = colWidths[c] - visibleLength(headers[c]);
        ss << " " << headers[c] << std::string(pad, ' ') << " │";
    }
    ss << "\n";

    // Header separator: ├───┼───┤
    ss << "├";
    for (size_t c = 0; c < headers.size(); ++c)
    {
        ss << repeatU8("─", colWidths[c] + 2);
        if (c + 1 < headers.size()) ss << "┼";
    }
    ss << "┤\n";

    // Data rows
    for (const auto &r : rows)
    {
        ss << "│";
        for (size_t c = 0; c < headers.size(); ++c)
        {
            std::string cell = c < r.size() ? r[c] : "";
            size_t vLen = visibleLength(cell);
            size_t pad = colWidths[c] >= vLen ? colWidths[c] - vLen : 0;
            ss << " " << cell << std::string(pad, ' ') << " │";
        }
        ss << "\n";
    }

    // Bottom border: └───┴───┘
    ss << "└";
    for (size_t c = 0; c < headers.size(); ++c)
    {
        ss << repeatU8("─", colWidths[c] + 2);
        if (c + 1 < headers.size()) ss << "┴";
    }
    ss << "┘";

    return ss.str();
}

static std::string renderBox(const std::string &text, const std::string &title = "")
{
    std::vector<std::string> lines;
    std::stringstream ss(text);
    std::string line;
    while (std::getline(ss, line))
    {
        lines.push_back(line);
    }
    if (lines.empty()) lines.push_back("");

    size_t maxLen = 0;
    for (const auto &l : lines)
    {
        maxLen = std::max(maxLen, visibleLength(l));
    }
    if (!title.empty())
    {
        maxLen = std::max(maxLen, visibleLength(title) + 2);
    }

    size_t innerWidth = maxLen + 2;
    std::stringstream out;
    if (title.empty())
    {
        out << "┌" << repeatU8("─", innerWidth) << "┐\n";
    }
    else
    {
        size_t titleLen = visibleLength(title);
        size_t rem = (innerWidth >= titleLen + 3) ? (innerWidth - titleLen - 3) : 0;
        out << "┌─ " << title << " " << repeatU8("─", rem) << "┐\n";
    }

    for (const auto &l : lines)
    {
        size_t vLen = visibleLength(l);
        size_t pad = maxLen >= vLen ? maxLen - vLen : 0;
        out << "│ " << l << std::string(pad, ' ') << " │\n";
    }

    out << "└" << repeatU8("─", maxLen + 2) << "┘";
    return out.str();
}

static std::shared_ptr<MemeMap> createDikhaModule(Interpreter &interpreter)
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    auto tableFn = [&interpreter](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string res = renderTable(args[0]);
        interpreter.getOutputStream() << res << "\n";
        return Value(res);
    };

    auto boxFn = [&interpreter](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string title = args.size() > 1 ? args[1].deref().toString() : "";
        std::string res = renderBox(args[0].deref().toString(), title);
        interpreter.getOutputStream() << res << "\n";
        return Value(res);
    };

    auto clearFn = [&interpreter](Interpreter &, const std::vector<Value> &) -> Value {
        interpreter.getOutputStream() << "\033[2J\033[H";
        return Value(true);
    };

    m->set("mez", Value(std::make_shared<NativeMethodCallable>("mez", 1, tableFn)));
    m->set("table", Value(std::make_shared<NativeMethodCallable>("table", 1, tableFn)));
    m->set("dabba", Value(std::make_shared<NativeMethodCallable>("dabba", 1, 2, boxFn)));
    m->set("box", Value(std::make_shared<NativeMethodCallable>("box", 1, 2, boxFn)));
    m->set("saaf", Value(std::make_shared<NativeMethodCallable>("saaf", 0, clearFn)));
    m->set("clear", Value(std::make_shared<NativeMethodCallable>("clear", 0, clearFn)));

    return m;
}

static std::string simulatedClipboard = "";

static std::shared_ptr<MemeMap> createSystummModule(Interpreter &interpreter)
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    auto argsList = std::make_shared<MemeList>();
    for (const auto &arg : interpreter.getCommandLineArgs())
    {
        argsList->elements.push_back(Value(arg));
    }
    m->set("args", Value(argsList));

    auto chalaVal = Value(std::make_shared<NativeMethodCallable>("chala", 1, [&interpreter](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string cmd = args[0].deref().toString();
#if defined(__EMSCRIPTEN__)
        return Value("[WASM: exec not supported]");
#else
        if (interpreter.getHeadless() || std::getenv("MEME_HEADLESS"))
        {
            return Value("[HEADLESS: executed " + cmd + "]");
        }
        FILE *pipe = popen(cmd.c_str(), "r");
        if (!pipe)
        {
            throw std::runtime_error("Systumm hang ho gaya! Command run nahi hui: " + cmd);
        }
        char buffer[256];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            result += buffer;
        }
        pclose(pipe);
        return Value(result);
#endif
    }));
    m->set("chala", chalaVal);
    m->set("exec", chalaVal);

    m->set("env", Value(std::make_shared<NativeMethodCallable>("env", 1, 2, [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string key = args[0].deref().toString();
        const char *val = std::getenv(key.c_str());
        if (val)
        {
            return Value(std::string(val));
        }
        return args.size() > 1 ? args[1] : Value(std::monostate{});
    })));

    auto setEnvVal = Value(std::make_shared<NativeMethodCallable>("set_env", 2, [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string key = args[0].deref().toString();
        std::string val = args[1].deref().toString();
#if !defined(__EMSCRIPTEN__)
#ifdef _WIN32
        _putenv_s(key.c_str(), val.c_str());
#else
        setenv(key.c_str(), val.c_str(), 1);
#endif
#endif
        return Value(true);
    }));
    m->set("set_env", setEnvVal);
    m->set("setEnv", setEnvVal);

    auto kahanVal = Value(std::make_shared<NativeMethodCallable>("kahan", 0, [](Interpreter &, const std::vector<Value> &) -> Value {
        return Value(std::filesystem::current_path().string());
    }));
    m->set("kahan", kahanVal);
    m->set("cwd", kahanVal);

    auto jaoVal = Value(std::make_shared<NativeMethodCallable>("jao", 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string path = args[0].deref().toString();
        std::error_code ec;
        std::filesystem::current_path(path, ec);
        if (ec)
        {
            throw std::runtime_error("Rasta bhatak gaye! Directory change nahi ho saki: " + path);
        }
        return Value(true);
    }));
    m->set("jao", jaoVal);
    m->set("chdir", jaoVal);

    auto kaunVal = Value(std::make_shared<NativeMethodCallable>("kaun", 0, [](Interpreter &, const std::vector<Value> &) -> Value {
#if defined(__EMSCRIPTEN__)
        return Value("wasm");
#elif defined(_WIN32)
        return Value("windows");
#elif defined(__APPLE__)
        return Value("mac");
#elif defined(__linux__)
        return Value("linux");
#else
        return Value("unknown");
#endif
    }));
    m->set("kaun", kaunVal);
    m->set("platform", kaunVal);

    auto nikalVal = Value(std::make_shared<NativeMethodCallable>("nikal", 0, 1, [&interpreter](Interpreter &, const std::vector<Value> &args) -> Value {
        long long code = 0;
        if (!args.empty())
        {
            code = toLongLong(args[0]);
        }
        if (interpreter.getHeadless() || std::getenv("MEME_HEADLESS"))
        {
            throw MemeExitSignal(code);
        }
        std::exit(static_cast<int>(code));
        return Value(std::monostate{});
    }));
    m->set("nikal", nikalVal);
    m->set("exit", nikalVal);

    m->set("copy", Value(std::make_shared<NativeMethodCallable>("copy", 1, [&interpreter](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string text = args[0].deref().toString();
        simulatedClipboard = text;
#if !defined(__EMSCRIPTEN__)
        if (!interpreter.getHeadless() && !std::getenv("MEME_HEADLESS"))
        {
#if defined(__APPLE__)
            FILE *p = popen("pbcopy", "w");
            if (p)
            {
                fwrite(text.data(), 1, text.size(), p);
                pclose(p);
            }
#elif defined(__linux__)
            FILE *p = popen("xclip -selection clipboard 2>/dev/null || wl-copy 2>/dev/null", "w");
            if (p)
            {
                fwrite(text.data(), 1, text.size(), p);
                pclose(p);
            }
#elif defined(_WIN32)
            FILE *p = _popen("clip", "w");
            if (p)
            {
                fwrite(text.data(), 1, text.size(), p);
                _pclose(p);
            }
#endif
        }
#endif
        return Value(true);
    })));

    m->set("paste", Value(std::make_shared<NativeMethodCallable>("paste", 0, [&interpreter](Interpreter &, const std::vector<Value> &) -> Value {
#if !defined(__EMSCRIPTEN__)
        if (!interpreter.getHeadless() && !std::getenv("MEME_HEADLESS"))
        {
#if defined(__APPLE__)
            FILE *p = popen("pbpaste", "r");
            if (p)
            {
                char buf[256];
                std::string res;
                while (fgets(buf, sizeof(buf), p)) res += buf;
                pclose(p);
                return Value(res);
            }
#elif defined(__linux__)
            FILE *p = popen("xclip -o -selection clipboard 2>/dev/null || wl-paste 2>/dev/null", "r");
            if (p)
            {
                char buf[256];
                std::string res;
                while (fgets(buf, sizeof(buf), p)) res += buf;
                pclose(p);
                return Value(res);
            }
#endif
        }
#endif
        return Value(simulatedClipboard);
    })));

    return m;
}

static std::shared_ptr<MemeMap> createCliModule(Interpreter &interpreter)
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    auto parseArgsFn = [&interpreter](Interpreter &, const std::vector<Value> &args) -> Value {
        std::vector<std::string> rawArgs;
        if (!args.empty() && args[0].deref().isList())
        {
            auto l = args[0].deref().getList();
            for (const auto &el : l->elements)
            {
                rawArgs.push_back(el.toString());
            }
        }
        else
        {
            rawArgs = interpreter.getCommandLineArgs();
        }

        auto resMap = std::make_shared<MemeMap>();
        auto positional = std::make_shared<MemeList>();

        for (const auto &arg : rawArgs)
        {
            if (arg.rfind("--", 0) == 0)
            {
                size_t eqPos = arg.find('=');
                if (eqPos != std::string::npos)
                {
                    std::string key = arg.substr(2, eqPos - 2);
                    std::string val = arg.substr(eqPos + 1);
                    resMap->set(key, Value(val));
                }
                else
                {
                    resMap->set(arg.substr(2), Value(true));
                }
            }
            else if (arg.rfind("-", 0) == 0 && arg.size() > 1)
            {
                resMap->set(arg.substr(1), Value(true));
            }
            else
            {
                positional->elements.push_back(Value(arg));
            }
        }
        resMap->set("_", Value(positional));
        return Value(resMap);
    };
    auto parseArgsVal = Value(std::make_shared<NativeMethodCallable>("args_samajh", 0, 1, parseArgsFn));
    m->set("args_samajh", parseArgsVal);
    m->set("parse_args", parseArgsVal);

    auto selectFn = [&interpreter](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string prompt = args[0].deref().toString();
        Value optVal = args[1].deref();
        if (!optVal.isList())
        {
            throw std::runtime_error("Chuno (select) mein options ki list chahiye!");
        }
        auto optList = optVal.getList();
        if (optList->elements.empty())
        {
            return Value(std::monostate{});
        }
        if (interpreter.getHeadless() || std::getenv("MEME_HEADLESS") || !interpreter.getInteractiveCLI())
        {
            return optList->elements[0];
        }

        interpreter.getOutputStream() << prompt << "\n";
        for (size_t i = 0; i < optList->elements.size(); ++i)
        {
            interpreter.getOutputStream() << "  " << (i + 1) << ") " << optList->elements[i].toString() << "\n";
        }
        interpreter.getOutputStream() << "Chuno [1-" << optList->elements.size() << "]: ";
        std::string input;
        std::getline(interpreter.getInputStream(), input);
        try
        {
            long long choice = std::stoll(input);
            if (choice >= 1 && choice <= static_cast<long long>(optList->elements.size()))
            {
                return optList->elements[choice - 1];
            }
        }
        catch (...) {}
        return optList->elements[0];
    };
    auto selectVal = Value(std::make_shared<NativeMethodCallable>("chuno", 2, selectFn));
    m->set("chuno", selectVal);
    m->set("select", selectVal);

    auto confirmFn = [&interpreter](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string prompt = args[0].deref().toString();
        if (interpreter.getHeadless() || std::getenv("MEME_HEADLESS") || !interpreter.getInteractiveCLI())
        {
            return Value(true);
        }
        interpreter.getOutputStream() << prompt << " [han/na] (y/n): ";
        std::string input;
        std::getline(interpreter.getInputStream(), input);
        if (!input.empty() && (input[0] == 'y' || input[0] == 'Y' || input[0] == 'h' || input[0] == 'H' || input[0] == 's' || input[0] == 'S'))
        {
            return Value(true);
        }
        return Value(false);
    };
    auto confirmVal = Value(std::make_shared<NativeMethodCallable>("puch_han_na", 1, confirmFn));
    m->set("puch_han_na", confirmVal);
    m->set("confirm", confirmVal);

    auto passwordFn = [&interpreter](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string prompt = args[0].deref().toString();
        if (interpreter.getHeadless() || std::getenv("MEME_HEADLESS") || !interpreter.getInteractiveCLI())
        {
            return Value("secret");
        }
        interpreter.getOutputStream() << prompt;
        std::string input;
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
        termios oldt, newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        std::getline(interpreter.getInputStream(), input);
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        interpreter.getOutputStream() << "\n";
#else
        std::getline(interpreter.getInputStream(), input);
#endif
        return Value(input);
    };
    auto passwordVal = Value(std::make_shared<NativeMethodCallable>("chhupa_puch", 1, passwordFn));
    m->set("chhupa_puch", passwordVal);
    m->set("password", passwordVal);

    auto progressFn = [&interpreter](Interpreter &, const std::vector<Value> &args) -> Value {
        long long cur = toLongLong(args[0]);
        long long tot = toLongLong(args[1]);
        std::string label = args.size() > 2 ? args[2].deref().toString() : "";
        if (tot <= 0) tot = 1;
        int pct = static_cast<int>((cur * 100) / tot);
        if (pct > 100) pct = 100;
        int barWidth = 30;
        int pos = (barWidth * pct) / 100;

        std::string bar = "[";
        for (int i = 0; i < barWidth; ++i)
        {
            if (i < pos) bar += "=";
            else if (i == pos) bar += ">";
            else bar += " ";
        }
        bar += "] " + std::to_string(pct) + "% " + label;

        interpreter.getOutputStream() << "\r" << bar << std::flush;
        if (cur >= tot)
        {
            interpreter.getOutputStream() << "\n";
        }
        return Value(true);
    };
    m->set("progress", Value(std::make_shared<NativeMethodCallable>("progress", 2, 3, progressFn)));

    return m;
}

static std::shared_ptr<MemeMap> createAppModule(Interpreter &interpreter)
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    auto alertFn = [&interpreter](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string title = args[0].deref().toString();
        std::string msg = args[1].deref().toString();
        if (interpreter.getHeadless() || std::getenv("MEME_HEADLESS"))
        {
            interpreter.getOutputStream() << "[App Alert: " << title << "] " << msg << "\n";
            return Value(true);
        }
#if defined(__APPLE__)
        std::string script = "osascript -e 'display dialog \"" + msg + "\" with title \"" + title + "\" buttons {\"OK\"} default button \"OK\"'";
        std::system(script.c_str());
#elif defined(__linux__)
        std::string script = "zenity --info --title=\"" + title + "\" --text=\"" + msg + "\" 2>/dev/null";
        std::system(script.c_str());
#elif defined(_WIN32)
        std::string script = "powershell -Command \"Add-Type -AssemblyName PresentationFramework;[System.Windows.MessageBox]::Show('" + msg + "', '" + title + "')\"";
        std::system(script.c_str());
#else
        interpreter.getOutputStream() << "[App Alert: " << title << "] " << msg << "\n";
#endif
        return Value(true);
    };
    auto alertVal = Value(std::make_shared<NativeMethodCallable>("alert", 2, alertFn));
    m->set("alert", alertVal);
    m->set("dikha_box", alertVal);

    auto puchBoxFn = [&interpreter](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string title = args[0].deref().toString();
        std::string prompt = args[1].deref().toString();
        std::string defVal = args.size() > 2 ? args[2].deref().toString() : "";
        if (interpreter.getHeadless() || std::getenv("MEME_HEADLESS"))
        {
            return Value(defVal);
        }
#if defined(__APPLE__)
        std::string script = "osascript -e 'display dialog \"" + prompt + "\" default answer \"" + defVal + "\" with title \"" + title + "\"'";
        FILE *p = popen(script.c_str(), "r");
        if (p)
        {
            char buf[256];
            std::string res;
            while (fgets(buf, sizeof(buf), p)) res += buf;
            pclose(p);
            size_t tag = res.find("text returned:");
            if (tag != std::string::npos)
            {
                std::string ans = res.substr(tag + 14);
                while (!ans.empty() && (ans.back() == '\n' || ans.back() == '\r')) ans.pop_back();
                return Value(ans);
            }
        }
#endif
        return Value(defVal);
    };
    auto puchBoxVal = Value(std::make_shared<NativeMethodCallable>("puch_box", 2, 3, puchBoxFn));
    m->set("puch_box", puchBoxVal);
    m->set("input_box", puchBoxVal);

    auto hanNaBoxFn = [&interpreter](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string title = args[0].deref().toString();
        std::string q = args[1].deref().toString();
        if (interpreter.getHeadless() || std::getenv("MEME_HEADLESS"))
        {
            return Value(true);
        }
#if defined(__APPLE__)
        std::string script = "osascript -e 'display dialog \"" + q + "\" with title \"" + title + "\" buttons {\"Na\", \"Han\"} default button \"Han\"'";
        FILE *p = popen(script.c_str(), "r");
        if (p)
        {
            char buf[256];
            std::string res;
            while (fgets(buf, sizeof(buf), p)) res += buf;
            pclose(p);
            return Value(res.find("button returned:Han") != std::string::npos);
        }
#endif
        return Value(true);
    };
    auto hanNaBoxVal = Value(std::make_shared<NativeMethodCallable>("han_na_box", 2, hanNaBoxFn));
    m->set("han_na_box", hanNaBoxVal);
    m->set("confirm_box", hanNaBoxVal);

    auto fileChunoFn = [&interpreter](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string title = args.size() > 0 ? args[0].deref().toString() : "Select File";
        if (interpreter.getHeadless() || std::getenv("MEME_HEADLESS"))
        {
            return Value("");
        }
#if defined(__APPLE__)
        std::string script = "osascript -e 'POSIX path of (choose file with prompt \"" + title + "\")' 2>/dev/null";
        FILE *p = popen(script.c_str(), "r");
        if (p)
        {
            char buf[512];
            std::string res;
            while (fgets(buf, sizeof(buf), p)) res += buf;
            pclose(p);
            while (!res.empty() && (res.back() == '\n' || res.back() == '\r')) res.pop_back();
            return Value(res);
        }
#endif
        return Value("");
    };
    auto fileChunoVal = Value(std::make_shared<NativeMethodCallable>("file_chuno", 0, 1, fileChunoFn));
    m->set("file_chuno", fileChunoVal);
    m->set("pick_file", fileChunoVal);

    auto notifyFn = [&interpreter](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string title = args[0].deref().toString();
        std::string msg = args[1].deref().toString();
        if (interpreter.getHeadless() || std::getenv("MEME_HEADLESS"))
        {
            interpreter.getOutputStream() << "[Notification: " << title << "] " << msg << "\n";
            return Value(true);
        }
#if defined(__APPLE__)
        std::string script = "osascript -e 'display notification \"" + msg + "\" with title \"" + title + "\"'";
        std::system(script.c_str());
#elif defined(__linux__)
        std::string script = "notify-send \"" + title + "\" \"" + msg + "\" 2>/dev/null";
        std::system(script.c_str());
#else
        interpreter.getOutputStream() << "[Notification: " << title << "] " << msg << "\n";
#endif
        return Value(true);
    };
    m->set("notify", Value(std::make_shared<NativeMethodCallable>("notify", 2, notifyFn)));

    auto openFn = [&interpreter](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string target = args[0].deref().toString();
        if (interpreter.getHeadless() || std::getenv("MEME_HEADLESS"))
        {
            interpreter.getOutputStream() << "[Open: " << target << "]\n";
            return Value(true);
        }
#if defined(__APPLE__)
        std::string cmd = "open \"" + target + "\"";
        std::system(cmd.c_str());
#elif defined(__linux__)
        std::string cmd = "xdg-open \"" + target + "\" 2>/dev/null";
        std::system(cmd.c_str());
#elif defined(_WIN32)
        std::string cmd = "start \"\" \"" + target + "\"";
        std::system(cmd.c_str());
#else
        interpreter.getOutputStream() << "[Open: " << target << "]\n";
#endif
        return Value(true);
    };
    auto openVal = Value(std::make_shared<NativeMethodCallable>("khol", 1, openFn));
    m->set("khol", openVal);
    m->set("open", openVal);

    return m;
}

struct TijoriStore {
    std::shared_ptr<MemeMap> dbData = std::make_shared<MemeMap>();
    std::string filePath = ".tijori.json";
    bool isLoaded = false;

    void ensureLoaded() {
        if (isLoaded) return;
        isLoaded = true;
        if (!filePath.empty() && std::filesystem::exists(filePath)) {
            try {
                std::ifstream f(filePath);
                if (f.is_open()) {
                    std::stringstream ss;
                    ss << f.rdbuf();
                    std::string content = ss.str();
                    if (!content.empty()) {
                        MemeJson::Parser p(content);
                        Value val = p.parseValue();
                        if (val.isMap()) {
                            dbData = val.getMap();
                        }
                    }
                }
            } catch (...) {}
        }
    }

    void persist() {
        if (filePath.empty()) return;
        try {
            std::ofstream f(filePath, std::ios::trunc);
            if (f.is_open()) {
                f << MemeJson::stringify(Value(dbData));
            }
        } catch (...) {}
    }
};

static std::shared_ptr<MemeMap> createTijoriModule(Interpreter &)
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    auto store = std::make_shared<TijoriStore>();
    store->ensureLoaded();

    auto rakhFn = [store](Interpreter &, const std::vector<Value> &args) -> Value {
        store->ensureLoaded();
        std::string key = args[0].deref().toString();
        Value val = args[1];
        store->dbData->set(key, val);
        store->persist();
        return Value(true);
    };
    auto rakhVal = Value(std::make_shared<NativeMethodCallable>("rakh", 2, rakhFn));
    m->set("rakh", rakhVal);
    m->set("set", rakhVal);

    auto nikalFn = [store](Interpreter &, const std::vector<Value> &args) -> Value {
        store->ensureLoaded();
        std::string key = args[0].deref().toString();
        if (store->dbData->has(key)) {
            return store->dbData->entries[key];
        }
        return args.size() > 1 ? args[1] : Value(std::monostate{});
    };
    auto nikalVal = Value(std::make_shared<NativeMethodCallable>("nikal", 1, 2, nikalFn));
    m->set("nikal", nikalVal);
    m->set("get", nikalVal);
    m->set("la", nikalVal);

    auto hasFn = [store](Interpreter &, const std::vector<Value> &args) -> Value {
        store->ensureLoaded();
        std::string key = args[0].deref().toString();
        return Value(store->dbData->has(key));
    };
    auto hasVal = Value(std::make_shared<NativeMethodCallable>("hai_kya", 1, hasFn));
    m->set("hai_kya", hasVal);
    m->set("has", hasVal);

    auto hataFn = [store](Interpreter &, const std::vector<Value> &args) -> Value {
        store->ensureLoaded();
        std::string key = args[0].deref().toString();
        bool rem = store->dbData->remove(key);
        if (rem) store->persist();
        return Value(rem);
    };
    auto hataVal = Value(std::make_shared<NativeMethodCallable>("hata", 1, hataFn));
    m->set("hata", hataVal);
    m->set("delete", hataVal);
    m->set("remove", hataVal);

    auto keysFn = [store](Interpreter &, const std::vector<Value> &) -> Value {
        store->ensureLoaded();
        auto l = std::make_shared<MemeList>();
        for (const auto &k : store->dbData->keyOrder) {
            l->elements.push_back(Value(k));
        }
        return Value(l);
    };
    auto keysVal = Value(std::make_shared<NativeMethodCallable>("chaabi", 0, keysFn));
    m->set("chaabi", keysVal);
    m->set("keys", keysVal);

    auto maalFn = [store](Interpreter &, const std::vector<Value> &) -> Value {
        store->ensureLoaded();
        auto l = std::make_shared<MemeList>();
        for (const auto &k : store->dbData->keyOrder) {
            l->elements.push_back(store->dbData->entries[k]);
        }
        return Value(l);
    };
    auto maalVal = Value(std::make_shared<NativeMethodCallable>("maal", 0, maalFn));
    m->set("maal", maalVal);
    m->set("values", maalVal);

    auto saafFn = [store](Interpreter &, const std::vector<Value> &) -> Value {
        store->dbData->entries.clear();
        store->dbData->keyOrder.clear();
        store->persist();
        return Value(true);
    };
    auto saafVal = Value(std::make_shared<NativeMethodCallable>("saaf", 0, saafFn));
    m->set("saaf", saafVal);
    m->set("clear", saafVal);

    auto sabFn = [store](Interpreter &, const std::vector<Value> &) -> Value {
        store->ensureLoaded();
        return Value(store->dbData);
    };
    auto sabVal = Value(std::make_shared<NativeMethodCallable>("sab", 0, sabFn));
    m->set("sab", sabVal);
    m->set("all", sabVal);

    auto fileFn = [store](Interpreter &, const std::vector<Value> &args) -> Value {
        store->filePath = args[0].deref().toString();
        store->isLoaded = false;
        store->ensureLoaded();
        return Value(true);
    };
    auto fileVal = Value(std::make_shared<NativeMethodCallable>("file", 1, fileFn));
    m->set("file", fileVal);
    m->set("kholo", fileVal);

    return m;
}

struct DhabaRoute {
    std::string method;
    std::string path;
    Value handler;
};

struct DhabaServer {
    std::vector<DhabaRoute> routes;
    std::vector<Value> middlewares;
    int port = 8080;
    bool isRunning = false;
};

static Value dispatchDhabaRequest(Interpreter &interpreter, const std::shared_ptr<DhabaServer> &server,
                                  const std::string &method, const std::string &rawPath,
                                  const Value &bodyVal = Value(std::monostate{}),
                                  const Value &headersVal = Value(std::monostate{}))
{
    std::string path = rawPath;
    auto queryMap = std::make_shared<MemeMap>();
    size_t qPos = path.find('?');
    if (qPos != std::string::npos)
    {
        std::string qStr = path.substr(qPos + 1);
        path = path.substr(0, qPos);
        std::stringstream ss(qStr);
        std::string pair;
        while (std::getline(ss, pair, '&'))
        {
            size_t eq = pair.find('=');
            if (eq != std::string::npos)
            {
                queryMap->set(pair.substr(0, eq), Value(pair.substr(eq + 1)));
            }
            else if (!pair.empty())
            {
                queryMap->set(pair, Value(true));
            }
        }
    }

    auto split = [](const std::string &s) {
        std::vector<std::string> parts;
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, '/')) {
            if (!item.empty()) parts.push_back(item);
        }
        return parts;
    };
    auto pathParts = split(path);

    for (const auto &r : server->routes)
    {
        if (r.method != method && r.method != "ALL" && r.method != "*")
            continue;

        auto paramsMap = std::make_shared<MemeMap>();
        bool matched = false;

        if (r.path == path || r.path == "*")
        {
            matched = true;
        }
        else
        {
            auto routeParts = split(r.path);
            if (routeParts.size() == pathParts.size())
            {
                matched = true;
                for (size_t i = 0; i < routeParts.size(); ++i)
                {
                    if (!routeParts[i].empty() && routeParts[i][0] == ':')
                    {
                        paramsMap->set(routeParts[i].substr(1), Value(pathParts[i]));
                    }
                    else if (routeParts[i] != pathParts[i])
                    {
                        matched = false;
                        break;
                    }
                }
            }
        }

        if (matched)
        {
            auto req = std::make_shared<MemeMap>();
            req->set("path", Value(path));
            req->set("method", Value(method));
            req->set("params", Value(paramsMap));
            req->set("query", Value(queryMap));
            req->set("body", bodyVal);
            req->set("headers", headersVal.isMap() ? headersVal : Value(std::make_shared<MemeMap>()));

            Value fnDeref = r.handler.deref();
            if (!std::holds_alternative<std::shared_ptr<Callable>>(fnDeref.data))
            {
                throw std::runtime_error("Dhaba route handler callable jugaad hona chahiye!");
            }
            auto callable = std::get<std::shared_ptr<Callable>>(fnDeref.data);
            Value handlerResult = callable->call(interpreter, { Value(req) });

            Value dRes = handlerResult.deref();
            if (dRes.isMap() && dRes.getMap()->has("status") && dRes.getMap()->has("body"))
            {
                return dRes;
            }

            auto res = std::make_shared<MemeMap>();
            res->set("status", Value(200LL));
            res->set("body", handlerResult);
            auto resHeaders = std::make_shared<MemeMap>();
            if (dRes.isMap() || dRes.isList())
            {
                resHeaders->set("Content-Type", Value("application/json"));
            }
            else
            {
                resHeaders->set("Content-Type", Value("text/plain"));
            }
            res->set("headers", Value(resHeaders));
            return Value(res);
        }
    }

    auto notFoundRes = std::make_shared<MemeMap>();
    notFoundRes->set("status", Value(404LL));
    notFoundRes->set("body", Value("Dhabe pe ye item nahi milta! 404 Route Not Found: " + method + " " + path));
    auto nfHeaders = std::make_shared<MemeMap>();
    nfHeaders->set("Content-Type", Value("text/plain"));
    notFoundRes->set("headers", Value(nfHeaders));
    return Value(notFoundRes);
}

static std::shared_ptr<MemeMap> createDhabaModule(Interpreter &interpreter)
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    auto server = std::make_shared<DhabaServer>();

    auto addRoute = [server](const std::string &method, const Value &pathVal, const Value &handlerVal) {
        DhabaRoute r;
        r.method = method;
        r.path = pathVal.deref().toString();
        r.handler = handlerVal;
        server->routes.push_back(r);
    };

    auto getFn = [addRoute](Interpreter &, const std::vector<Value> &args) -> Value {
        addRoute("GET", args[0], args[1]);
        return Value(true);
    };
    m->set("get", Value(std::make_shared<NativeMethodCallable>("get", 2, getFn)));

    auto postFn = [addRoute](Interpreter &, const std::vector<Value> &args) -> Value {
        addRoute("POST", args[0], args[1]);
        return Value(true);
    };
    m->set("post", Value(std::make_shared<NativeMethodCallable>("post", 2, postFn)));

    auto putFn = [addRoute](Interpreter &, const std::vector<Value> &args) -> Value {
        addRoute("PUT", args[0], args[1]);
        return Value(true);
    };
    m->set("put", Value(std::make_shared<NativeMethodCallable>("put", 2, putFn)));

    auto delFn = [addRoute](Interpreter &, const std::vector<Value> &args) -> Value {
        addRoute("DELETE", args[0], args[1]);
        return Value(true);
    };
    m->set("delete", Value(std::make_shared<NativeMethodCallable>("delete", 2, delFn)));

    auto routeFn = [addRoute](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string method = args[0].deref().toString();
        for (char &c : method) c = std::toupper(static_cast<unsigned char>(c));
        addRoute(method, args[1], args[2]);
        return Value(true);
    };
    m->set("route", Value(std::make_shared<NativeMethodCallable>("route", 3, routeFn)));

    auto reqFn = [server, &interpreter](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string method = args[0].deref().toString();
        for (char &c : method) c = std::toupper(static_cast<unsigned char>(c));
        std::string path = args[1].deref().toString();
        Value body = args.size() > 2 ? args[2] : Value(std::monostate{});
        Value headers = args.size() > 3 ? args[3] : Value(std::monostate{});
        return dispatchDhabaRequest(interpreter, server, method, path, body, headers);
    };
    m->set("request", Value(std::make_shared<NativeMethodCallable>("request", 2, 4, reqFn)));

    auto chalaFn = [server, &interpreter](Interpreter &, const std::vector<Value> &args) -> Value {
        int port = 8080;
        if (!args.empty()) {
            port = static_cast<int>(toLongLong(args[0]));
        }
        server->port = port;
        server->isRunning = true;
        interpreter.getOutputStream() << "[Dhaba Server] ☕ Garam orders taiyyar hain on http://localhost:" << port << "\n";
        if (args.size() > 1 && std::holds_alternative<std::shared_ptr<Callable>>(args[1].deref().data)) {
            auto cb = std::get<std::shared_ptr<Callable>>(args[1].deref().data);
            cb->call(interpreter, { Value(static_cast<long long>(port)) });
        }
        return Value(true);
    };
    auto chalaVal = Value(std::make_shared<NativeMethodCallable>("chala", 0, 2, chalaFn));
    m->set("chala", chalaVal);
    m->set("listen", chalaVal);

    auto saafFn = [server](Interpreter &, const std::vector<Value> &) -> Value {
        server->routes.clear();
        server->middlewares.clear();
        server->isRunning = false;
        return Value(true);
    };
    auto saafVal = Value(std::make_shared<NativeMethodCallable>("saaf", 0, saafFn));
    m->set("saaf", saafVal);
    m->set("reset", saafVal);

    return m;
}

static std::vector<std::string> renderUiTerminalNode(const Value &nodeVal)
{
    Value d = nodeVal.deref();
    if (d.isList())
    {
        std::vector<std::string> lines;
        for (const auto &el : d.getList()->elements)
        {
            auto sub = renderUiTerminalNode(el);
            lines.insert(lines.end(), sub.begin(), sub.end());
        }
        return lines;
    }
    if (!d.isMap())
    {
        return {d.toString()};
    }
    auto map = d.getMap();
    std::string type = map->has("type") ? map->entries["type"].deref().toString() : "";

    if (type == "likh")
    {
        std::string text = map->has("text") ? map->entries["text"].deref().toString() : "";
        std::string variant = map->has("variant") ? map->entries["variant"].deref().toString() : "body";
        if (variant == "heading" || variant == "title")
        {
            return {"[mota]" + text + "[/]"};
        }
        if (variant == "subheading")
        {
            return {"[asmani]" + text + "[/]"};
        }
        if (variant == "caption")
        {
            return {"[halka]" + text + "[/]"};
        }
        return {text};
    }
    if (type == "badge")
    {
        std::string label = map->has("label") ? map->entries["label"].deref().toString() : "";
        std::string color = map->has("color") ? map->entries["color"].deref().toString() : "hara";
        return {"[" + color + "][ " + label + " ][/]"};
    }
    if (type == "puch")
    {
        std::string ph = map->has("placeholder") ? map->entries["placeholder"].deref().toString() : "...";
        return {"[  " + ph + "  ]"};
    }
    if (type == "button")
    {
        std::string label = map->has("label") ? map->entries["label"].deref().toString() : "Button";
        return {"[ " + label + " ]"};
    }
    if (type == "faasla")
    {
        return {""};
    }
    if (type == "dabba")
    {
        std::string title = map->has("title") ? map->entries["title"].deref().toString() : "";
        std::vector<std::string> childLines;
        if (map->has("children"))
        {
            Value chVal = map->entries["children"].deref();
            if (chVal.isList())
            {
                for (const auto &c : chVal.getList()->elements)
                {
                    auto l = renderUiTerminalNode(c);
                    childLines.insert(childLines.end(), l.begin(), l.end());
                }
            }
            else
            {
                childLines = renderUiTerminalNode(chVal);
            }
        }

        size_t maxLen = 0;
        for (const auto &l : childLines)
        {
            maxLen = std::max(maxLen, visibleLength(l));
        }
        if (!title.empty())
        {
            maxLen = std::max(maxLen, visibleLength(title) + 2);
        }
        if (maxLen < 20) maxLen = 20;

        size_t innerWidth = maxLen + 2;
        std::vector<std::string> res;
        if (title.empty())
        {
            res.push_back("┌" + repeatU8("─", innerWidth) + "┐");
        }
        else
        {
            size_t titleLen = visibleLength(title);
            size_t rem = (innerWidth >= titleLen + 3) ? (innerWidth - titleLen - 3) : 0;
            res.push_back("┌─ " + title + " " + repeatU8("─", rem) + "┐");
        }

        for (const auto &l : childLines)
        {
            size_t vLen = visibleLength(l);
            size_t pad = maxLen >= vLen ? maxLen - vLen : 0;
            res.push_back("│ " + l + std::string(pad, ' ') + " │");
        }
        res.push_back("└" + repeatU8("─", innerWidth) + "┘");
        return res;
    }
    if (type == "katar")
    {
        std::vector<std::vector<std::string>> blocks;
        if (map->has("children") && map->entries["children"].deref().isList())
        {
            for (const auto &c : map->entries["children"].deref().getList()->elements)
            {
                blocks.push_back(renderUiTerminalNode(c));
            }
        }

        size_t maxLines = 0;
        for (const auto &b : blocks)
        {
            maxLines = std::max(maxLines, b.size());
        }

        std::vector<size_t> blockWidths;
        for (const auto &b : blocks)
        {
            size_t bw = 0;
            for (const auto &l : b) bw = std::max(bw, visibleLength(l));
            blockWidths.push_back(bw);
        }

        std::vector<std::string> rowLines;
        for (size_t y = 0; y < maxLines; ++y)
        {
            std::string line = "";
            for (size_t b = 0; b < blocks.size(); ++b)
            {
                if (b > 0) line += "   ";
                std::string cell = y < blocks[b].size() ? blocks[b][y] : "";
                size_t vLen = visibleLength(cell);
                size_t pad = blockWidths[b] >= vLen ? blockWidths[b] - vLen : 0;
                line += cell + std::string(pad, ' ');
            }
            rowLines.push_back(line);
        }
        return rowLines;
    }
    if (type == "khamba")
    {
        std::vector<std::string> colLines;
        if (map->has("children") && map->entries["children"].deref().isList())
        {
            const auto &children = map->entries["children"].deref().getList()->elements;
            for (size_t i = 0; i < children.size(); ++i)
            {
                if (i > 0) colLines.push_back("");
                auto cl = renderUiTerminalNode(children[i]);
                colLines.insert(colLines.end(), cl.begin(), cl.end());
            }
        }
        return colLines;
    }
    if (type == "jsx_element")
    {
        std::string tag = map->has("tag") ? map->entries["tag"].deref().toString() : "div";
        std::vector<std::string> childLines;
        if (map->has("children"))
        {
            Value chVal = map->entries["children"].deref();
            if (chVal.isList())
            {
                for (const auto &c : chVal.getList()->elements)
                {
                    auto l = renderUiTerminalNode(c);
                    childLines.insert(childLines.end(), l.begin(), l.end());
                }
            }
            else
            {
                childLines = renderUiTerminalNode(chVal);
            }
        }

        std::string text = "";
        for (size_t i = 0; i < childLines.size(); ++i) {
            if (i > 0) text += " ";
            text += childLines[i];
        }

        if (tag == "h1" || tag == "h2")
        {
            return {"[mota]" + text + "[/]"};
        }
        if (tag == "h3")
        {
            return {"[asmani mota]" + text + "[/]"};
        }
        if (tag == "button")
        {
            return {"[ " + text + " ]"};
        }
        if (tag == "span")
        {
            return {"[" + text + "]"};
        }
        if (tag == "input")
        {
            std::string text = "...";
            if (map->has("props") && map->entries["props"].deref().isMap()) {
                auto pm = map->entries["props"].deref().getMap();
                if (pm->has("value") && !pm->entries["value"].deref().toString().empty()) {
                    text = pm->entries["value"].deref().toString();
                } else if (pm->has("placeholder")) {
                    text = pm->entries["placeholder"].deref().toString();
                }
            }
            return {"[  " + text + "  ]"};
        }

        std::string cls = map->has("class") ? map->entries["class"].deref().toString() : "";
        if (cls.find("border") != std::string::npos || cls.find("rounded") != std::string::npos)
        {
            size_t maxLen = 0;
            for (const auto &l : childLines) maxLen = std::max(maxLen, visibleLength(l));
            if (maxLen < 20) maxLen = 20;
            size_t innerWidth = maxLen + 2;

            std::vector<std::string> res;
            res.push_back("┌" + repeatU8("─", innerWidth) + "┐");
            for (const auto &l : childLines)
            {
                size_t vLen = visibleLength(l);
                size_t pad = maxLen >= vLen ? maxLen - vLen : 0;
                res.push_back("│ " + l + std::string(pad, ' ') + " │");
            }
            res.push_back("└" + repeatU8("─", innerWidth) + "┘");
            return res;
        }

        return childLines.empty() ? std::vector<std::string>{text} : childLines;
    }

    return {d.toString()};
}

std::string renderUiScreen(const Value &screen, const std::string &title)
{
    auto innerLines = renderUiTerminalNode(screen);

    size_t maxLen = 0;
    for (const auto &l : innerLines)
    {
        maxLen = std::max(maxLen, visibleLength(l));
    }
    if (!title.empty())
    {
        maxLen = std::max(maxLen, visibleLength(title) + 2);
    }
    if (maxLen < 44) maxLen = 44;

    size_t innerWidth = maxLen + 4;
    std::stringstream ss;
    if (title.empty())
    {
        ss << "┌" << repeatU8("─", innerWidth) << "┐\n";
    }
    else
    {
        size_t titleLen = visibleLength(title);
        size_t rem = (innerWidth >= titleLen + 3) ? (innerWidth - titleLen - 3) : 0;
        ss << "┌─ " << title << " " << repeatU8("─", rem) << "┐\n";
    }

    ss << "│" << std::string(innerWidth, ' ') << "│\n";
    for (const auto &l : innerLines)
    {
        size_t vLen = visibleLength(l);
        size_t pad = maxLen >= vLen ? maxLen - vLen : 0;
        ss << "│  " << l << std::string(pad, ' ') << "  │\n";
    }
    ss << "│" << std::string(innerWidth, ' ') << "│\n";
    ss << "└" << repeatU8("─", innerWidth) << "┘";

    return formatRichText(ss.str());
}

static std::string escapeHtmlStr(const std::string &str)
{
    std::string out;
    for (char c : str)
    {
        if (c == '&') out += "&amp;";
        else if (c == '<') out += "&lt;";
        else if (c == '>') out += "&gt;";
        else if (c == '"') out += "&quot;";
        else if (c == '\'') out += "&#39;";
        else out += c;
    }
    return out;
}

static std::string renderUiHtmlTree(
    const Value &nodeVal,
    int &nodeIdCounter,
    std::unordered_map<int, TailwindEngine::HitRegion> &interactiveMap)
{
    Value d = nodeVal.deref();
    if (d.isNil() || (std::holds_alternative<bool>(d.data) && !std::get<bool>(d.data)))
    {
        return "";
    }
    if (d.isList())
    {
        std::string res;
        for (const auto &child : d.getList()->elements)
        {
            res += renderUiHtmlTree(child, nodeIdCounter, interactiveMap);
        }
        return res;
    }
    if (!d.isMap())
    {
        return escapeHtmlStr(d.toString());
    }

    auto map = d.getMap();
    std::string type = map->has("type") ? map->entries["type"].deref().toString() : "";
    std::string tag = map->has("tag") ? map->entries["tag"].deref().toString() : "";
    if (tag.empty())
    {
        tag = (type == "jsx_element" || type == "dabba" || type == "khamba" || type == "katar") ? "div" : "div";
    }
    if (tag == "fragment") tag = "div";

    std::shared_ptr<MemeMap> pm = nullptr;
    if (map->has("props") && map->entries["props"].deref().isMap())
    {
        pm = map->entries["props"].deref().getMap();
    }

    // Classes
    std::string cls = map->has("class") ? map->entries["class"].deref().toString() : "";
    if (cls.empty() && pm)
    {
        if (pm->has("class")) cls = pm->entries["class"].deref().toString();
        else if (pm->has("className")) cls = pm->entries["className"].deref().toString();
    }

    // Interactive event handlers
    Value clickHandler = Value(std::monostate{});
    Value changeHandler = Value(std::monostate{});
    if (pm)
    {
        if (pm->has("onClick")) clickHandler = pm->entries["onClick"];
        else if (pm->has("onclick")) clickHandler = pm->entries["onclick"];
        if (pm->has("onChange")) changeHandler = pm->entries["onChange"];
        else if (pm->has("onchange")) changeHandler = pm->entries["onchange"];
        else if (pm->has("onInput")) changeHandler = pm->entries["onInput"];
    }
    if (map->has("onClick")) clickHandler = map->entries["onClick"];
    else if (map->has("onclick")) clickHandler = map->entries["onclick"];
    if (map->has("onChange")) changeHandler = map->entries["onChange"];
    else if (map->has("onchange")) changeHandler = map->entries["onchange"];

    bool hasClick = (tag == "button" || tag == "a" || !clickHandler.deref().isNil());
    bool hasChange = (tag == "input" || tag == "textarea" || tag == "select" || !changeHandler.deref().isNil());

    int nodeId = 0;
    if (hasClick || hasChange)
    {
        nodeId = ++nodeIdCounter;
        TailwindEngine::HitRegion hr;
        hr.id = nodeId;
        hr.tag = tag;
        hr.hasClick = hasClick;
        hr.hasChange = hasChange;
        hr.onClick = clickHandler;
        hr.onChange = changeHandler;
        if (pm && pm->has("value")) hr.inputValue = pm->entries["value"].deref().toString();
        interactiveMap[nodeId] = hr;
    }

    std::string out = "<" + tag;
    if (!cls.empty())
    {
        out += " class=\"" + escapeHtmlStr(cls) + "\"";
    }

    if (nodeId > 0)
    {
        out += " data-meme-id=\"" + std::to_string(nodeId) + "\"";
        if (hasClick)
        {
            out += " onclick=\"window.__memeDispatch(" + std::to_string(nodeId) + ", 'click', '')\"";
        }
        if (hasChange)
        {
            out += " oninput=\"window.__memeDispatch(" + std::to_string(nodeId) + ", 'change', this.value)\"";
            out += " onkeydown=\"if(event.key==='Enter'){event.preventDefault();window.__memeDispatch(" + std::to_string(nodeId) + ", 'enter', this.value);}\"";
        }
    }

    // Pass through standard attributes from props
    if (pm)
    {
        for (const auto &k : pm->keyOrder)
        {
            if (k == "class" || k == "className" || k == "onClick" || k == "onclick" ||
                k == "onChange" || k == "onchange" || k == "onInput" || k == "children")
            {
                continue;
            }
            Value pv = pm->entries[k].deref();
            if (std::holds_alternative<bool>(pv.data))
            {
                if (std::get<bool>(pv.data)) out += " " + k;
            }
            else if (!pv.isNil())
            {
                out += " " + k + "=\"" + escapeHtmlStr(pv.toString()) + "\"";
            }
        }
    }

    // Void self-closing elements
    if (tag == "input" || tag == "img" || tag == "hr" || tag == "br")
    {
        out += " />";
        return out;
    }

    out += ">";

    // Children
    if (map->has("children"))
    {
        Value chVal = map->entries["children"].deref();
        if (chVal.isList())
        {
            for (const auto &c : chVal.getList()->elements)
            {
                out += renderUiHtmlTree(c, nodeIdCounter, interactiveMap);
            }
        }
        else if (!chVal.isNil())
        {
            out += renderUiHtmlTree(chVal, nodeIdCounter, interactiveMap);
        }
    }

    out += "</" + tag + ">";
    return out;
}

static std::string renderUiHtmlNode(const Value &nodeVal)
{
    Value d = nodeVal.deref();
    if (!d.isMap())
    {
        return "<p>" + d.toString() + "</p>";
    }
    auto map = d.getMap();
    std::string type = map->has("type") ? map->entries["type"].deref().toString() : "";

    if (type == "likh")
    {
        std::string text = map->has("text") ? map->entries["text"].deref().toString() : "";
        std::string variant = map->has("variant") ? map->entries["variant"].deref().toString() : "body";
        if (variant == "heading" || variant == "title")
        {
            return "<h1 class=\"text-2xl font-bold text-zinc-100 mb-2\">" + text + "</h1>";
        }
        if (variant == "subheading")
        {
            return "<h2 class=\"text-lg font-semibold text-emerald-400 mb-1\">" + text + "</h2>";
        }
        if (variant == "caption")
        {
            return "<p class=\"text-xs text-zinc-400\">" + text + "</p>";
        }
        return "<p class=\"text-zinc-200\">" + text + "</p>";
    }
    if (type == "badge")
    {
        std::string label = map->has("label") ? map->entries["label"].deref().toString() : "";
        std::string color = map->has("color") ? map->entries["color"].deref().toString() : "hara";
        std::string cls = "bg-emerald-500/20 text-emerald-400 border-emerald-500/30";
        if (color == "peela" || color == "yellow") cls = "bg-amber-500/20 text-amber-400 border-amber-500/30";
        else if (color == "laal" || color == "red") cls = "bg-rose-500/20 text-rose-400 border-rose-500/30";
        else if (color == "neela" || color == "blue") cls = "bg-sky-500/20 text-sky-400 border-sky-500/30";
        else if (color == "asmani" || color == "cyan") cls = "bg-cyan-500/20 text-cyan-400 border-cyan-500/30";
        return "<span class=\"inline-flex px-2.5 py-0.5 rounded-full text-xs font-semibold border " + cls + "\">" + label + "</span>";
    }
    if (type == "puch")
    {
        std::string ph = map->has("placeholder") ? map->entries["placeholder"].deref().toString() : "";
        return "<input type=\"text\" placeholder=\"" + ph + "\" class=\"w-full bg-zinc-800 border border-zinc-700 rounded-xl px-4 py-2.5 text-zinc-100 placeholder-zinc-500 focus:outline-none focus:border-emerald-500\" />";
    }
    if (type == "button")
    {
        std::string label = map->has("label") ? map->entries["label"].deref().toString() : "Button";
        return "<button class=\"w-full bg-emerald-600 hover:bg-emerald-500 text-white font-semibold py-2.5 rounded-xl shadow-lg transition duration-150\">" + label + "</button>";
    }
    if (type == "faasla")
    {
        return "<div class=\"h-4\"></div>";
    }
    if (type == "dabba")
    {
        std::string title = map->has("title") ? map->entries["title"].deref().toString() : "";
        std::string innerHtml = "";
        if (map->has("children"))
        {
            Value chVal = map->entries["children"].deref();
            if (chVal.isList())
            {
                for (const auto &c : chVal.getList()->elements)
                {
                    innerHtml += renderUiHtmlNode(c);
                }
            }
            else
            {
                innerHtml += renderUiHtmlNode(chVal);
            }
        }
        std::string titleHtml = title.empty() ? "" : ("<div class=\"text-xs font-semibold uppercase tracking-wider text-zinc-400 mb-1\">" + title + "</div>");
        return "<div class=\"border border-zinc-800 bg-zinc-900/90 rounded-xl p-4 shadow-sm flex flex-col gap-2\">" + titleHtml + innerHtml + "</div>";
    }
    if (type == "katar")
    {
        std::string innerHtml = "";
        if (map->has("children") && map->entries["children"].deref().isList())
        {
            for (const auto &c : map->entries["children"].deref().getList()->elements)
            {
                innerHtml += "<div class=\"flex-1 min-w-[200px]\">" + renderUiHtmlNode(c) + "</div>";
            }
        }
        return "<div class=\"flex flex-row flex-wrap gap-4 items-start\">" + innerHtml + "</div>";
    }
    if (type == "khamba")
    {
        std::string innerHtml = "";
        if (map->has("children") && map->entries["children"].deref().isList())
        {
            for (const auto &c : map->entries["children"].deref().getList()->elements)
            {
                innerHtml += renderUiHtmlNode(c);
            }
        }
        return "<div class=\"flex flex-col gap-4\">" + innerHtml + "</div>";
    }
    if (type == "jsx_element")
    {
        std::string tag = map->has("tag") ? map->entries["tag"].deref().toString() : "div";
        if (tag == "fragment") tag = "div";
        std::string cls = map->has("class") ? map->entries["class"].deref().toString() : "";
        std::string innerHtml = "";
        if (map->has("children"))
        {
            Value chVal = map->entries["children"].deref();
            if (chVal.isList())
            {
                for (const auto &c : chVal.getList()->elements)
                {
                    innerHtml += renderUiHtmlNode(c);
                }
            }
            else
            {
                innerHtml += renderUiHtmlNode(chVal);
            }
        }
        std::string classAttr = cls.empty() ? "" : (" class=\"" + cls + "\"");
        if (tag == "input" || tag == "img")
        {
            return "<" + tag + classAttr + " />";
        }
        return "<" + tag + classAttr + ">" + innerHtml + "</" + tag + ">";
    }

    return "<p>" + d.toString() + "</p>";
}

static std::string renderUiFullHtml(const Value &screen, const std::string &title = "MEMElang App")
{
    std::string body = renderUiHtmlNode(screen);
    return "<!DOCTYPE html>\n<html>\n<head>\n"
           "  <meta charset=\"utf-8\">\n"
           "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
           "  <title>" + title + "</title>\n"
           "  <script src=\"https://cdn.tailwindcss.com\"></script>\n"
           "</head>\n<body class=\"bg-zinc-950 text-zinc-100 min-h-screen p-6 flex justify-center items-start\">\n"
           "  <div class=\"max-w-2xl w-full bg-zinc-900 border border-zinc-800 rounded-2xl p-6 shadow-2xl flex flex-col gap-5\">\n"
           "    " + body + "\n"
           "  </div>\n</body>\n</html>\n";
}

static bool triggerUiClick(Interpreter &interpreter, const Value &nodeVal, const std::string &targetLabel = "")
{
    Value d = nodeVal.deref();
    if (!d.isMap()) return false;
    auto map = d.getMap();
    std::string type = map->has("type") ? map->entries["type"].deref().toString() : "";

    if (type == "button")
    {
        std::string label = map->has("label") ? map->entries["label"].deref().toString() : "";
        if (targetLabel.empty() || label == targetLabel)
        {
            if (map->has("onClick"))
            {
                Value cbDeref = map->entries["onClick"].deref();
                if (std::holds_alternative<std::shared_ptr<Callable>>(cbDeref.data))
                {
                    auto cb = std::get<std::shared_ptr<Callable>>(cbDeref.data);
                    cb->call(interpreter, {});
                    return true;
                }
            }
        }
    }
    if (type == "jsx_element")
    {
        std::string tag = map->has("tag") ? map->entries["tag"].deref().toString() : "";
        if (tag == "button")
        {
            if (map->has("props") && map->entries["props"].deref().isMap())
            {
                auto pm = map->entries["props"].deref().getMap();
                if (pm->has("onClick"))
                {
                    Value cbDeref = pm->entries["onClick"].deref();
                    if (std::holds_alternative<std::shared_ptr<Callable>>(cbDeref.data))
                    {
                        auto cb = std::get<std::shared_ptr<Callable>>(cbDeref.data);
                        cb->call(interpreter, {});
                        return true;
                    }
                }
            }
        }
    }

    if (map->has("children"))
    {
        Value chVal = map->entries["children"].deref();
        if (chVal.isList())
        {
            for (const auto &c : chVal.getList()->elements)
            {
                if (triggerUiClick(interpreter, c, targetLabel)) return true;
            }
        }
        else
        {
            if (triggerUiClick(interpreter, chVal, targetLabel)) return true;
        }
    }
    return false;
}

static std::shared_ptr<MemeMap> createUiModule(Interpreter &interpreter)
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    auto khambaFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto map = std::make_shared<MemeMap>();
        map->set("type", Value("khamba"));
        Value ch = args.size() > 0 ? args[0] : Value(std::make_shared<MemeList>());
        if (!ch.deref().isList()) {
            auto l = std::make_shared<MemeList>();
            l->elements.push_back(ch);
            ch = Value(l);
        }
        map->set("children", ch);
        return Value(map);
    };
    auto khambaVal = Value(std::make_shared<NativeMethodCallable>("khamba", 1, khambaFn));
    m->set("khamba", khambaVal);
    m->set("column", khambaVal);

    auto katarFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto map = std::make_shared<MemeMap>();
        map->set("type", Value("katar"));
        Value ch = args.size() > 0 ? args[0] : Value(std::make_shared<MemeList>());
        if (!ch.deref().isList()) {
            auto l = std::make_shared<MemeList>();
            l->elements.push_back(ch);
            ch = Value(l);
        }
        map->set("children", ch);
        return Value(map);
    };
    auto katarVal = Value(std::make_shared<NativeMethodCallable>("katar", 1, katarFn));
    m->set("katar", katarVal);
    m->set("row", katarVal);

    auto likhFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto map = std::make_shared<MemeMap>();
        map->set("type", Value("likh"));
        map->set("text", Value(args[0].deref().toString()));
        map->set("variant", Value(args.size() > 1 ? args[1].deref().toString() : "body"));
        return Value(map);
    };
    auto likhVal = Value(std::make_shared<NativeMethodCallable>("likh", 1, 2, likhFn));
    m->set("likh", likhVal);
    m->set("text", likhVal);

    auto dabbaFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto map = std::make_shared<MemeMap>();
        map->set("type", Value("dabba"));
        Value ch = args.size() > 0 ? args[0] : Value(std::make_shared<MemeList>());
        if (!ch.deref().isList()) {
            auto l = std::make_shared<MemeList>();
            l->elements.push_back(ch);
            ch = Value(l);
        }
        map->set("children", ch);
        map->set("title", Value(args.size() > 1 ? args[1].deref().toString() : ""));
        return Value(map);
    };
    auto dabbaVal = Value(std::make_shared<NativeMethodCallable>("dabba", 1, 2, dabbaFn));
    m->set("dabba", dabbaVal);
    m->set("box", dabbaVal);
    m->set("card", dabbaVal);

    auto badgeFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto map = std::make_shared<MemeMap>();
        map->set("type", Value("badge"));
        map->set("label", Value(args[0].deref().toString()));
        map->set("color", Value(args.size() > 1 ? args[1].deref().toString() : "hara"));
        return Value(map);
    };
    m->set("badge", Value(std::make_shared<NativeMethodCallable>("badge", 1, 2, badgeFn)));

    auto puchFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto map = std::make_shared<MemeMap>();
        map->set("type", Value("puch"));
        map->set("placeholder", Value(args[0].deref().toString()));
        map->set("value", Value(args.size() > 1 ? args[1].deref().toString() : ""));
        return Value(map);
    };
    auto puchVal = Value(std::make_shared<NativeMethodCallable>("puch", 1, 2, puchFn));
    m->set("puch", puchVal);
    m->set("input", puchVal);

    auto buttonFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto map = std::make_shared<MemeMap>();
        map->set("type", Value("button"));
        map->set("label", Value(args[0].deref().toString()));
        if (args.size() > 1) {
            map->set("onClick", args[1]);
        }
        return Value(map);
    };
    m->set("button", Value(std::make_shared<NativeMethodCallable>("button", 1, 2, buttonFn)));

    auto faaslaFn = [](Interpreter &, const std::vector<Value> &) -> Value {
        auto map = std::make_shared<MemeMap>();
        map->set("type", Value("faasla"));
        return Value(map);
    };
    auto faaslaVal = Value(std::make_shared<NativeMethodCallable>("faasla", 0, 1, faaslaFn));
    m->set("faasla", faaslaVal);
    m->set("divider", faaslaVal);

    auto dikhaFn = [&interpreter](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string title = args.size() > 1 ? args[1].deref().toString() : "";
        Value root = args.empty() ? Value(std::monostate{}) : args[0];
        interpreter.setRootUiComponent(root);
        interpreter.triggerUiRerender();
        std::string res = renderUiScreen(root, title);
        interpreter.getOutputStream() << res << "\n";
        return Value(res);
    };
    auto dikhaVal = Value(std::make_shared<NativeMethodCallable>("dikha", 1, 2, dikhaFn));
    m->set("dikha", dikhaVal);
    m->set("render", dikhaVal);
    m->set("mount", Value(std::make_shared<NativeMethodCallable>("mount", 1, [&interpreter](Interpreter &, const std::vector<Value> &args) -> Value {
        if (!args.empty()) {
            if (std::holds_alternative<std::shared_ptr<Callable>>(args[0].deref().data)) {
                interpreter.rootCallable = args[0];
            } else {
                interpreter.setRootUiComponent(args[0]);
                auto env = interpreter.getEnvironment();
                for (const auto &name : {"App", "Main", "Root", "ShellApp", "POSApp", "Counter"}) {
                    auto slot = env ? env->findSlot(name) : nullptr;
                    if (!slot && interpreter.getGlobals()) slot = interpreter.getGlobals()->findSlot(name);
                    if (slot && std::holds_alternative<std::shared_ptr<Callable>>(slot->value.deref().data)) {
                        interpreter.rootCallable = slot->value;
                        break;
                    }
                }
            }
            interpreter.triggerUiRerender();
            const char *plat = std::getenv("MEME_PLATFORM");
            bool isTerminalTarget = (plat != nullptr && std::string(plat) == "terminal");
            if (isTerminalTarget && !interpreter.getHeadless() && !std::getenv("MEME_HEADLESS") && !interpreter.getLastTerminalUi().empty()) {
                interpreter.getOutputStream() << interpreter.getLastTerminalUi() << "\n";
            }
        }
        return Value(interpreter.getLastGpuCommandsJson());
    })));

    auto dispatchFn = [&interpreter](Interpreter &, const std::vector<Value> &args) -> Value {
        int id = args.size() > 0 ? static_cast<int>(args[0].deref().asInt()) : 0;
        std::string evType = args.size() > 1 ? args[1].deref().toString() : "click";
        std::string val = args.size() > 2 ? args[2].deref().toString() : "";
        bool ok = interpreter.dispatchUiEvent(id, evType, val);
        return Value(ok);
    };
    m->set("dispatch", Value(std::make_shared<NativeMethodCallable>("dispatch", 2, 3, dispatchFn)));

    auto gpuCmdsFn = [&interpreter](Interpreter &, const std::vector<Value> &args) -> Value {
        Value tree = args.empty() ? Value(std::monostate{}) : args[0];
        auto cmds = TailwindEngine::computeNativeLayout(tree, 800.0f, 600.0f);
        interpreter.setLastGpuCommandsJson(TailwindEngine::gpuCommandsToJson(cmds));
        auto list = std::make_shared<MemeList>();
        for (const auto &c : cmds) {
            auto cmdMap = std::make_shared<MemeMap>();
            cmdMap->set("type", Value(c.type));
            cmdMap->set("x", Value(static_cast<double>(c.x)));
            cmdMap->set("y", Value(static_cast<double>(c.y)));
            cmdMap->set("width", Value(static_cast<double>(c.width)));
            cmdMap->set("height", Value(static_cast<double>(c.height)));
            cmdMap->set("radius", Value(static_cast<double>(c.radius)));
            cmdMap->set("color", Value(c.color));
            cmdMap->set("alpha", Value(static_cast<double>(c.alpha)));
            cmdMap->set("lineWidth", Value(static_cast<double>(c.lineWidth)));
            cmdMap->set("text", Value(c.text));
            cmdMap->set("fontSize", Value(static_cast<double>(c.fontSize)));
            cmdMap->set("fontWeight", Value(static_cast<long long>(c.fontWeight)));
            list->elements.push_back(Value(cmdMap));
        }
        return Value(list);
    };
    m->set("gpu_commands", Value(std::make_shared<NativeMethodCallable>("gpu_commands", 1, gpuCmdsFn)));

    auto htmlFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string title = args.size() > 1 ? args[1].deref().toString() : "MEMElang App";
        return Value(renderUiFullHtml(args[0], title));
    };
    m->set("html", Value(std::make_shared<NativeMethodCallable>("html", 1, 2, htmlFn)));

    auto clickFn = [&interpreter](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string label = args.size() > 1 ? args[1].deref().toString() : "";
        bool ok = triggerUiClick(interpreter, args[0], label);
        return Value(ok);
    };
    auto clickVal = Value(std::make_shared<NativeMethodCallable>("click", 1, 2, clickFn));
    m->set("click", clickVal);
    m->set("dabao", clickVal);

    return m;
}

static Value getListMethod(const std::shared_ptr<MemeList> &list, const std::string &name)
{
    if (name == "ghusa" || name == "dhakka" || name == "push" || name == "append" || name == "daal")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [list](Interpreter &, const std::vector<Value> &args) {
            list->elements.push_back(args[0]);
            return Value(static_cast<long long>(list->elements.size()));
        }));
    }
    if (name == "laat_maar" || name == "nikaal" || name == "pop" || name == "nikal")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [list](Interpreter &, const std::vector<Value> &) {
            if (list->elements.empty())
            {
                throw std::runtime_error("Khali gang mein se kya nikaaloge? Array pehle se khali hai!");
            }
            Value last = list->elements.back();
            list->elements.pop_back();
            return last;
        }));
    }
    if (name == "kaat" || name == "slice")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, 2, [list](Interpreter &, const std::vector<Value> &args) {
            long long sz = static_cast<long long>(list->elements.size());
            if (args.empty())
            {
                throw std::runtime_error("Gaddari korbe! 'kaat' method ko kam se kam 1 index argument chahiye!");
            }
            Value sVal = args[0].deref();
            long long start = std::holds_alternative<long long>(sVal.data) ? std::get<long long>(sVal.data) : static_cast<long long>(std::get<double>(sVal.data));
            if (start < 0) start += sz;
            if (start < 0) start = 0;
            if (start > sz) start = sz;

            long long end = sz;
            if (args.size() > 1)
            {
                Value eVal = args[1].deref();
                end = std::holds_alternative<long long>(eVal.data) ? std::get<long long>(eVal.data) : static_cast<long long>(std::get<double>(eVal.data));
                if (end < 0) end += sz;
                if (end < 0) end = 0;
                if (end > sz) end = sz;
            }

            auto subList = std::make_shared<MemeList>();
            if (start < end)
            {
                for (long long i = start; i < end; ++i)
                {
                    subList->elements.push_back(list->elements[i]);
                }
            }
            return Value(subList);
        }));
    }
    if (name == "hai_kya" || name == "has" || name == "contains")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [list](Interpreter &, const std::vector<Value> &args) {
            for (const auto &elem : list->elements)
            {
                if (elem == args[0])
                    return Value(true);
            }
            return Value(false);
        }));
    }
    if (name == "chipka" || name == "join")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [list](Interpreter &, const std::vector<Value> &args) {
            std::string sep = args[0].deref().toString();
            std::string joined = "";
            for (size_t i = 0; i < list->elements.size(); ++i)
            {
                if (i > 0) joined += sep;
                joined += list->elements[i].deref().toString();
            }
            return Value(joined);
        }));
    }
    if (name == "size" || name == "kitna_lamba" || name == "lambai" || name == "len" || name == "length")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [list](Interpreter &, const std::vector<Value> &) {
            return Value(static_cast<long long>(list->elements.size()));
        }));
    }
    if (name == "nakal" || name == "clone" || name == "copy")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [list](Interpreter &interp, const std::vector<Value> &) {
            return interp.cloneValue(Value(list));
        }));
    }
    if (name == "badal" || name == "map" || name == "sab_pe" || name == "badal_sab")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [list, name](Interpreter &interpreter, const std::vector<Value> &args) {
            Value fnDeref = args[0].deref();
            if (!std::holds_alternative<std::shared_ptr<Callable>>(fnDeref.data))
            {
                throw std::runtime_error("Doglapan! Method '" + name + "' ko callback jugaad chahiye!");
            }
            auto callable = std::get<std::shared_ptr<Callable>>(fnDeref.data);
            auto resultList = std::make_shared<MemeList>();
            auto elems = list->elements;
            resultList->elements.reserve(elems.size());
            for (size_t i = 0; i < elems.size(); ++i)
            {
                Value res;
                if (callable->checkArity(2))
                {
                    res = callable->call(interpreter, {elems[i], Value(static_cast<long long>(i))});
                }
                else
                {
                    res = callable->call(interpreter, {elems[i]});
                }
                resultList->elements.push_back(res);
            }
            return Value(resultList);
        }));
    }
    if (name == "chhan" || name == "chhaan" || name == "filter" || name == "filter_kar")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [list, name](Interpreter &interpreter, const std::vector<Value> &args) {
            Value fnDeref = args[0].deref();
            if (!std::holds_alternative<std::shared_ptr<Callable>>(fnDeref.data))
            {
                throw std::runtime_error("Doglapan! Method '" + name + "' ko callback jugaad chahiye!");
            }
            auto callable = std::get<std::shared_ptr<Callable>>(fnDeref.data);
            auto resultList = std::make_shared<MemeList>();
            auto elems = list->elements;
            resultList->elements.reserve(elems.size());
            for (size_t i = 0; i < elems.size(); ++i)
            {
                Value res;
                if (callable->checkArity(2))
                {
                    res = callable->call(interpreter, {elems[i], Value(static_cast<long long>(i))});
                }
                else
                {
                    res = callable->call(interpreter, {elems[i]});
                }
                if (res.isTruthy())
                {
                    resultList->elements.push_back(elems[i]);
                }
            }
            return Value(resultList);
        }));
    }
    if (name == "kul" || name == "jod" || name == "reduce" || name == "nichod" || name == "sameto")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, 2, [list, name](Interpreter &interpreter, const std::vector<Value> &args) {
            if (args.empty())
            {
                throw std::runtime_error("Ye sab doglapan hai! Method '" + name + "' ko kam se kam 1 argument chahiye!");
            }
            Value fnDeref = args[0].deref();
            if (!std::holds_alternative<std::shared_ptr<Callable>>(fnDeref.data))
            {
                throw std::runtime_error("Doglapan! Method '" + name + "' ko callback jugaad chahiye!");
            }
            auto callable = std::get<std::shared_ptr<Callable>>(fnDeref.data);

            if (list->elements.empty())
            {
                if (args.size() > 1)
                {
                    return args[1];
                }
                throw std::runtime_error("Khali dabba! Bina initial value ke khali list ko kaise nichodoge?");
            }

            Value acc;
            size_t startIdx = 0;
            if (args.size() > 1)
            {
                acc = args[1];
                startIdx = 0;
            }
            else
            {
                acc = list->elements[0];
                startIdx = 1;
            }

            auto elems = list->elements;
            for (size_t i = startIdx; i < elems.size(); ++i)
            {
                acc = callable->call(interpreter, {acc, elems[i]});
            }
            return acc;
        }));
    }
    if (name == "saja" || name == "sort" || name == "line_mein" || name == "chhaant")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, 1, [list](Interpreter &interpreter, const std::vector<Value> &args) {
            if (args.empty())
            {
                std::stable_sort(list->elements.begin(), list->elements.end(), [&interpreter](const Value &a, const Value &b) {
                    Value da = a.deref();
                    Value db = b.deref();
                    if (da.isInstance())
                    {
                        auto inst = da.getInstance();
                        if (inst && inst->getClass())
                        {
                            auto method = inst->getClass()->findMethod("__chota__");
                            if (method)
                            {
                                BoundMethod bound(inst, method);
                                return bound.call(interpreter, {db}).isTruthy();
                            }
                        }
                    }
                    return lessThan(a, b).isTruthy();
                });
            }
            else
            {
                Value fnDeref = args[0].deref();
                if (!std::holds_alternative<std::shared_ptr<Callable>>(fnDeref.data))
                {
                    throw std::runtime_error("Doglapan! Sort comparator callable hona chahiye!");
                }
                auto callable = std::get<std::shared_ptr<Callable>>(fnDeref.data);
                std::stable_sort(list->elements.begin(), list->elements.end(), [&](const Value &a, const Value &b) {
                    Value res = callable->call(interpreter, {a, b});
                    res = res.deref();
                    if (std::holds_alternative<bool>(res.data))
                    {
                        return std::get<bool>(res.data);
                    }
                    if (std::holds_alternative<long long>(res.data))
                    {
                        return std::get<long long>(res.data) < 0;
                    }
                    if (std::holds_alternative<double>(res.data))
                    {
                        return std::get<double>(res.data) < 0.0;
                    }
                    return res.isTruthy();
                });
            }
            return Value(list);
        }));
    }
    if (name == "har" || name == "har_ek" || name == "each" || name == "forEach")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [list, name](Interpreter &interpreter, const std::vector<Value> &args) {
            Value fnDeref = args[0].deref();
            if (!std::holds_alternative<std::shared_ptr<Callable>>(fnDeref.data))
            {
                throw std::runtime_error("Doglapan! Method '" + name + "' ko callback jugaad chahiye!");
            }
            auto callable = std::get<std::shared_ptr<Callable>>(fnDeref.data);
            auto elems = list->elements;
            for (size_t i = 0; i < elems.size(); ++i)
            {
                if (callable->checkArity(2))
                {
                    callable->call(interpreter, {elems[i], Value(static_cast<long long>(i))});
                }
                else
                {
                    callable->call(interpreter, {elems[i]});
                }
            }
            return Value(std::monostate{});
        }));
    }
    if (name == "dhund" || name == "dhund_pehla" || name == "find" || name == "pakad_ek")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [list, name](Interpreter &interpreter, const std::vector<Value> &args) {
            Value fnDeref = args[0].deref();
            if (!std::holds_alternative<std::shared_ptr<Callable>>(fnDeref.data))
            {
                throw std::runtime_error("Doglapan! Method '" + name + "' ko callback jugaad chahiye!");
            }
            auto callable = std::get<std::shared_ptr<Callable>>(fnDeref.data);
            auto elems = list->elements;
            for (size_t i = 0; i < elems.size(); ++i)
            {
                Value res;
                if (callable->checkArity(2))
                {
                    res = callable->call(interpreter, {elems[i], Value(static_cast<long long>(i))});
                }
                else
                {
                    res = callable->call(interpreter, {elems[i]});
                }
                if (res.isTruthy())
                {
                    return elems[i];
                }
            }
            return Value(std::monostate{});
        }));
    }
    if (name == "koi" || name == "koi_bhi" || name == "some" || name == "any")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [list, name](Interpreter &interpreter, const std::vector<Value> &args) {
            Value fnDeref = args[0].deref();
            if (!std::holds_alternative<std::shared_ptr<Callable>>(fnDeref.data))
            {
                throw std::runtime_error("Doglapan! Method '" + name + "' ko callback jugaad chahiye!");
            }
            auto callable = std::get<std::shared_ptr<Callable>>(fnDeref.data);
            auto elems = list->elements;
            for (size_t i = 0; i < elems.size(); ++i)
            {
                Value res;
                if (callable->checkArity(2))
                {
                    res = callable->call(interpreter, {elems[i], Value(static_cast<long long>(i))});
                }
                else
                {
                    res = callable->call(interpreter, {elems[i]});
                }
                if (res.isTruthy())
                {
                    return Value(true);
                }
            }
            return Value(false);
        }));
    }
    if (name == "sab" || name == "saare" || name == "every" || name == "all")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [list, name](Interpreter &interpreter, const std::vector<Value> &args) {
            Value fnDeref = args[0].deref();
            if (!std::holds_alternative<std::shared_ptr<Callable>>(fnDeref.data))
            {
                throw std::runtime_error("Doglapan! Method '" + name + "' ko callback jugaad chahiye!");
            }
            auto callable = std::get<std::shared_ptr<Callable>>(fnDeref.data);
            auto elems = list->elements;
            for (size_t i = 0; i < elems.size(); ++i)
            {
                Value res;
                if (callable->checkArity(2))
                {
                    res = callable->call(interpreter, {elems[i], Value(static_cast<long long>(i))});
                }
                else
                {
                    res = callable->call(interpreter, {elems[i]});
                }
                if (!res.isTruthy())
                {
                    return Value(false);
                }
            }
            return Value(true);
        }));
    }

    if (name == "ulta" || name == "reverse")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [list](Interpreter &, const std::vector<Value> &) {
            std::reverse(list->elements.begin(), list->elements.end());
            return Value(list);
        }));
    }
    if (name == "ek_kar" || name == "unique")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [list](Interpreter &, const std::vector<Value> &) {
            auto res = std::make_shared<MemeList>();
            for (const auto &elem : list->elements)
            {
                bool found = false;
                for (const auto &u : res->elements)
                {
                    if (u == elem)
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    res->elements.push_back(elem);
                }
            }
            return Value(res);
        }));
    }
    if (name == "seedha" || name == "flatten")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [list](Interpreter &, const std::vector<Value> &) {
            auto res = std::make_shared<MemeList>();
            std::function<void(const std::shared_ptr<MemeList>&)> flat = [&](const std::shared_ptr<MemeList> &l) {
                for (const auto &el : l->elements)
                {
                    Value d = el.deref();
                    if (d.isList())
                    {
                        flat(d.getList());
                    }
                    else
                    {
                        res->elements.push_back(el);
                    }
                }
            };
            flat(list);
            return Value(res);
        }));
    }
    if (name == "gino" || name == "count")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [list](Interpreter &, const std::vector<Value> &args) {
            long long cnt = 0;
            for (const auto &elem : list->elements)
            {
                if (elem == args[0])
                {
                    cnt++;
                }
            }
            return Value(cnt);
        }));
    }
    if (name == "kahan" || name == "index_of" || name == "indexOf")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [list](Interpreter &, const std::vector<Value> &args) {
            for (size_t i = 0; i < list->elements.size(); ++i)
            {
                if (list->elements[i] == args[0])
                {
                    return Value(static_cast<long long>(i));
                }
            }
            return Value(-1LL);
        }));
    }
    if (name == "tukde" || name == "chunk")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [list](Interpreter &, const std::vector<Value> &args) {
            long long n = toLongLong(args[0]);
            if (n <= 0)
            {
                throw std::runtime_error("Aukaat ke bahar! Chunk size 0 ya negative nahi ho sakta!");
            }
            auto res = std::make_shared<MemeList>();
            auto currentChunk = std::make_shared<MemeList>();
            for (size_t i = 0; i < list->elements.size(); ++i)
            {
                currentChunk->elements.push_back(list->elements[i]);
                if (static_cast<long long>(currentChunk->elements.size()) == n)
                {
                    res->elements.push_back(Value(currentChunk));
                    currentChunk = std::make_shared<MemeList>();
                }
            }
            if (!currentChunk->elements.empty())
            {
                res->elements.push_back(Value(currentChunk));
            }
            return Value(res);
        }));
    }
    if (name == "tukka" || name == "sample")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [list](Interpreter &, const std::vector<Value> &) {
            if (list->elements.empty())
            {
                return Value(std::monostate{});
            }
            size_t idx = static_cast<size_t>(rand()) % list->elements.size();
            return list->elements[idx];
        }));
    }
    if (name == "feko" || name == "shuffle")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [list](Interpreter &, const std::vector<Value> &) {
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(list->elements.begin(), list->elements.end(), g);
            return Value(list);
        }));
    }
    if (name == "pehla" || name == "first")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [list](Interpreter &, const std::vector<Value> &) {
            if (list->elements.empty())
            {
                return Value(std::monostate{});
            }
            return list->elements.front();
        }));
    }
    if (name == "aakhri" || name == "last")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [list](Interpreter &, const std::vector<Value> &) {
            if (list->elements.empty())
            {
                return Value(std::monostate{});
            }
            return list->elements.back();
        }));
    }
    if (name == "khali_hai" || name == "is_empty" || name == "isEmpty")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [list](Interpreter &, const std::vector<Value> &) {
            return Value(list->elements.empty());
        }));
    }

    throw std::runtime_error("Khopdi tod saale ka! 'list' ke paas '" + name + "' naam ka koi method nahi hai!");
}

static Value getStringMethod(const std::string &str, const std::string &name);

static Value getMapMethod(const std::shared_ptr<MemeMap> &map, const std::string &name)
{
    if (map->isModule && map->has(name))
    {
        return map->entries[name];
    }

    if (name == "chaabi" || name == "keys")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [map](Interpreter &, const std::vector<Value> &) {
            auto keyList = std::make_shared<MemeList>();
            for (const auto &k : map->keyOrder)
            {
                keyList->elements.push_back(Value(k));
            }
            return Value(keyList);
        }));
    }
    if (name == "maal" || name == "values")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [map](Interpreter &, const std::vector<Value> &) {
            auto valList = std::make_shared<MemeList>();
            for (const auto &k : map->keyOrder)
            {
                valList->elements.push_back(map->entries[k]);
            }
            return Value(valList);
        }));
    }
    if (name == "hai_kya" || name == "has" || name == "contains")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [map](Interpreter &, const std::vector<Value> &args) {
            std::string k = args[0].deref().toString();
            return Value(map->has(k));
        }));
    }
    if (name == "hata" || name == "delete" || name == "remove" || name == "pop")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [map](Interpreter &, const std::vector<Value> &args) {
            std::string k = args[0].deref().toString();
            return Value(map->remove(k));
        }));
    }
    if (name == "la" || name == "get")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, 2, [map](Interpreter &, const std::vector<Value> &args) {
            if (args.empty())
            {
                throw std::runtime_error("Gaddari korbe! 'la' method ko key argument chahiye!");
            }
            std::string k = args[0].deref().toString();
            if (map->has(k))
            {
                return map->entries[k];
            }
            return args.size() > 1 ? args[1] : Value(std::monostate{});
        }));
    }
    if (name == "size" || name == "kitna_lamba" || name == "lambai" || name == "len" || name == "length")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [map](Interpreter &, const std::vector<Value> &) {
            return Value(static_cast<long long>(map->entries.size()));
        }));
    }
    if (name == "nakal" || name == "clone" || name == "copy")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [map](Interpreter &interp, const std::vector<Value> &) {
            return interp.cloneValue(Value(map));
        }));
    }

    if (map->has(name))
    {
        return map->entries[name];
    }

    if (map->has("__is_error__"))
    {
        std::string s = map->has("baat") ? map->entries["baat"].deref().toString() : "";
        return getStringMethod(s, name);
    }

    throw std::runtime_error("Khopdi tod saale ka! 'khata' ke paas '" + name + "' naam ka koi method ya key nahi hai!");
}

static Value getStringMethod(const std::string &str, const std::string &name)
{
    if (name == "tod" || name == "tukde" || name == "split")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [str](Interpreter &, const std::vector<Value> &args) {
            std::string sep = args[0].deref().toString();
            auto resList = std::make_shared<MemeList>();
            if (sep.empty())
            {
                for (char ch : str)
                {
                    resList->elements.push_back(Value(std::string(1, ch)));
                }
            }
            else
            {
                size_t start = 0;
                size_t pos = 0;
                while ((pos = str.find(sep, start)) != std::string::npos)
                {
                    resList->elements.push_back(Value(str.substr(start, pos - start)));
                    start = pos + sep.length();
                }
                resList->elements.push_back(Value(str.substr(start)));
            }
            return Value(resList);
        }));
    }
    if (name == "chipka" || name == "join")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [str](Interpreter &, const std::vector<Value> &args) {
            Value arrVal = args[0].deref();
            if (!arrVal.isList())
            {
                throw std::runtime_error("String.chipka() mein list/array pass karna padega!");
            }
            auto arr = arrVal.getList();
            std::string joined = "";
            for (size_t i = 0; i < arr->elements.size(); ++i)
            {
                if (i > 0) joined += str;
                joined += arr->elements[i].deref().toString();
            }
            return Value(joined);
        }));
    }
    if (name == "badal" || name == "replace")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 2, [str](Interpreter &, const std::vector<Value> &args) {
            std::string old_str = args[0].deref().toString();
            std::string new_str = args[1].deref().toString();
            if (old_str.empty()) return Value(str);
            std::string res = str;
            size_t pos = 0;
            while ((pos = res.find(old_str, pos)) != std::string::npos)
            {
                res.replace(pos, old_str.length(), new_str);
                pos += new_str.length();
            }
            return Value(res);
        }));
    }
    if (name == "dhund" || name == "find" || name == "kahan" || name == "index_of" || name == "indexOf")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [str](Interpreter &, const std::vector<Value> &args) {
            std::string sub = args[0].deref().toString();
            size_t pos = str.find(sub);
            return pos == std::string::npos ? Value(-1LL) : Value(static_cast<long long>(pos));
        }));
    }
    if (name == "hai_kya" || name == "has" || name == "contains" || name == "andar")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [str](Interpreter &, const std::vector<Value> &args) {
            std::string sub = args[0].deref().toString();
            return Value(str.find(sub) != std::string::npos);
        }));
    }
    if (name == "kaat" || name == "slice" || name == "substring")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, 2, [str](Interpreter &, const std::vector<Value> &args) {
            long long sz = static_cast<long long>(str.size());
            if (args.empty())
            {
                throw std::runtime_error("Gaddari korbe! 'kaat' method ko kam se kam 1 index argument chahiye!");
            }
            Value sVal = args[0].deref();
            long long start = std::holds_alternative<long long>(sVal.data) ? std::get<long long>(sVal.data) : static_cast<long long>(std::get<double>(sVal.data));
            if (start < 0) start += sz;
            if (start < 0) start = 0;
            if (start > sz) start = sz;

            long long end = sz;
            if (args.size() > 1)
            {
                Value eVal = args[1].deref();
                end = std::holds_alternative<long long>(eVal.data) ? std::get<long long>(eVal.data) : static_cast<long long>(std::get<double>(eVal.data));
                if (end < 0) end += sz;
                if (end < 0) end = 0;
                if (end > sz) end = sz;
            }

            if (start >= end)
                return Value("");
            return Value(str.substr(start, end - start));
        }));
    }
    if (name == "chilla" || name == "upper" || name == "to_upper")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [str](Interpreter &, const std::vector<Value> &) {
            std::string res = str;
            for (char &c : res) c = std::toupper(static_cast<unsigned char>(c));
            return Value(res);
        }));
    }
    if (name == "dheere_bol" || name == "lower" || name == "to_lower")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [str](Interpreter &, const std::vector<Value> &) {
            std::string res = str;
            for (char &c : res) c = std::tolower(static_cast<unsigned char>(c));
            return Value(res);
        }));
    }
    if (name == "saaf" || name == "trim" || name == "strip")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [str](Interpreter &, const std::vector<Value> &) {
            size_t first = str.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) return Value("");
            size_t last = str.find_last_not_of(" \t\r\n");
            return Value(str.substr(first, (last - first + 1)));
        }));
    }
    if (name == "size" || name == "kitna_lamba" || name == "lambai" || name == "len" || name == "length")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [str](Interpreter &, const std::vector<Value> &) {
            return Value(static_cast<long long>(str.size()));
        }));
    }
    if (name == "nakal" || name == "clone" || name == "copy")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [str](Interpreter &, const std::vector<Value> &) {
            return Value(str);
        }));
    }

    if (name == "akshar" || name == "char_at" || name == "charAt")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [str](Interpreter &, const std::vector<Value> &args) {
            long long i = toLongLong(args[0]);
            long long sz = static_cast<long long>(str.size());
            if (i < 0) i += sz;
            if (i < 0 || i >= sz)
            {
                throw std::runtime_error("Aukaat ke bahar! Index " + std::to_string(i) + " string ki lambai se bahar hai!");
            }
            return Value(std::string(1, str[static_cast<size_t>(i)]));
        }));
    }
    if (name == "akshar_list" || name == "chars")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [str](Interpreter &, const std::vector<Value> &) {
            auto res = std::make_shared<MemeList>();
            for (char ch : str)
            {
                res->elements.push_back(Value(std::string(1, ch)));
            }
            return Value(res);
        }));
    }
    if (name == "shuru_hota" || name == "starts_with" || name == "startsWith")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [str](Interpreter &, const std::vector<Value> &args) {
            std::string sub = args[0].deref().toString();
            return Value(str.rfind(sub, 0) == 0);
        }));
    }
    if (name == "khatam_hota" || name == "ends_with" || name == "endsWith")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [str](Interpreter &, const std::vector<Value> &args) {
            std::string sub = args[0].deref().toString();
            if (sub.size() > str.size()) return Value(false);
            return Value(str.compare(str.size() - sub.size(), sub.size(), sub) == 0);
        }));
    }
    if (name == "ulta" || name == "reverse")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [str](Interpreter &, const std::vector<Value> &) {
            return Value(std::string(str.rbegin(), str.rend()));
        }));
    }
    if (name == "gino" || name == "count")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [str](Interpreter &, const std::vector<Value> &args) {
            std::string sub = args[0].deref().toString();
            if (sub.empty()) return Value(0LL);
            long long cnt = 0;
            size_t pos = 0;
            while ((pos = str.find(sub, pos)) != std::string::npos)
            {
                cnt++;
                pos += sub.size();
            }
            return Value(cnt);
        }));
    }
    if (name == "duhrao" || name == "repeat")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [str](Interpreter &, const std::vector<Value> &args) {
            long long n = toLongLong(args[0]);
            if (n < 0)
            {
                throw std::runtime_error("Aukaat ke bahar! Repeat count negative nahi ho sakta!");
            }
            std::string res;
            for (long long i = 0; i < n; ++i)
            {
                res += str;
            }
            return Value(res);
        }));
    }
    if (name == "aage_bhar" || name == "pad_start" || name == "padStart")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, 2, [str](Interpreter &, const std::vector<Value> &args) {
            long long targetLen = toLongLong(args[0]);
            std::string padChar = args.size() > 1 ? args[1].deref().toString() : " ";
            if (padChar.empty()) padChar = " ";
            std::string res = str;
            while (static_cast<long long>(res.size()) < targetLen)
            {
                res = padChar + res;
            }
            return Value(res);
        }));
    }
    if (name == "piche_bhar" || name == "pad_end" || name == "padEnd")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, 2, [str](Interpreter &, const std::vector<Value> &args) {
            long long targetLen = toLongLong(args[0]);
            std::string padChar = args.size() > 1 ? args[1].deref().toString() : " ";
            if (padChar.empty()) padChar = " ";
            std::string res = str;
            while (static_cast<long long>(res.size()) < targetLen)
            {
                res = res + padChar;
            }
            return Value(res);
        }));
    }
    if (name == "number_hai" || name == "is_digit" || name == "isDigit")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [str](Interpreter &, const std::vector<Value> &) {
            if (str.empty()) return Value(false);
            for (char ch : str)
            {
                if (!std::isdigit(static_cast<unsigned char>(ch))) return Value(false);
            }
            return Value(true);
        }));
    }
    if (name == "akshar_hai" || name == "is_alpha" || name == "isAlpha")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [str](Interpreter &, const std::vector<Value> &) {
            if (str.empty()) return Value(false);
            for (char ch : str)
            {
                if (!std::isalpha(static_cast<unsigned char>(ch))) return Value(false);
            }
            return Value(true);
        }));
    }
    if (name == "space_hai" || name == "is_space" || name == "isSpace")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [str](Interpreter &, const std::vector<Value> &) {
            if (str.empty()) return Value(false);
            for (char ch : str)
            {
                if (!std::isspace(static_cast<unsigned char>(ch))) return Value(false);
            }
            return Value(true);
        }));
    }
    if (name == "bada_hai" || name == "is_upper" || name == "isUpper")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [str](Interpreter &, const std::vector<Value> &) {
            if (str.empty()) return Value(false);
            bool hasAlpha = false;
            for (char ch : str)
            {
                if (std::isalpha(static_cast<unsigned char>(ch)))
                {
                    hasAlpha = true;
                    if (!std::isupper(static_cast<unsigned char>(ch))) return Value(false);
                }
            }
            return Value(hasAlpha);
        }));
    }
    if (name == "chota_hai" || name == "is_lower" || name == "isLower")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [str](Interpreter &, const std::vector<Value> &) {
            if (str.empty()) return Value(false);
            bool hasAlpha = false;
            for (char ch : str)
            {
                if (std::isalpha(static_cast<unsigned char>(ch)))
                {
                    hasAlpha = true;
                    if (!std::islower(static_cast<unsigned char>(ch))) return Value(false);
                }
            }
            return Value(hasAlpha);
        }));
    }

    auto wrapAnsi = [&str, &name](const std::string &code) -> Value {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [str, code](Interpreter &, const std::vector<Value> &) {
            return Value(code + str + "\033[0m");
        }));
    };

    if (name == "laal" || name == "red") return wrapAnsi("\033[31m");
    if (name == "hara" || name == "green") return wrapAnsi("\033[32m");
    if (name == "peela" || name == "yellow") return wrapAnsi("\033[33m");
    if (name == "neela" || name == "blue") return wrapAnsi("\033[34m");
    if (name == "gulabi" || name == "pink" || name == "magenta") return wrapAnsi("\033[35m");
    if (name == "asmani" || name == "cyan") return wrapAnsi("\033[36m");
    if (name == "safed" || name == "white") return wrapAnsi("\033[37m");
    if (name == "kala" || name == "black") return wrapAnsi("\033[30m");
    if (name == "mota" || name == "bold") return wrapAnsi("\033[1m");
    if (name == "halka" || name == "dim") return wrapAnsi("\033[2m");
    if (name == "tedha" || name == "italic") return wrapAnsi("\033[3m");
    if (name == "line" || name == "underline") return wrapAnsi("\033[4m");

    if (name == "rang_hata" || name == "strip_ansi" || name == "stripAnsi")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [str](Interpreter &, const std::vector<Value> &) {
            return Value(stripAnsi(str));
        }));
    }

    throw std::runtime_error("Khopdi tod saale ka! 'string' ke paas '" + name + "' naam ka koi method nahi hai!");
}

Value NakalBuiltin::call(Interpreter &interpreter, const std::vector<Value> &args)
{
    return interpreter.cloneValue(args[0]);
}

Value LambaiBuiltin::call(Interpreter &interpreter, const std::vector<Value> &args)
{
    Value val = args[0].deref();
    if (std::holds_alternative<std::string>(val.data))
    {
        return Value(static_cast<long long>(std::get<std::string>(val.data).size()));
    }
    if (val.isList())
    {
        return Value(static_cast<long long>(val.getList()->elements.size()));
    }
    if (val.isMap())
    {
        return Value(static_cast<long long>(val.getMap()->entries.size()));
    }
    if (val.isInstance())
    {
        auto inst = val.getInstance();
        if (inst && inst->getClass())
        {
            auto method = inst->getClass()->findMethod("__size__");
            if (!method) method = inst->getClass()->findMethod("__len__");
            if (!method) method = inst->getClass()->findMethod("__lambai__");
            if (method)
            {
                BoundMethod bound(inst, method);
                return bound.call(interpreter, {});
            }
        }
    }
    throw std::runtime_error("'size()' sirf string, list ya khata naapta hai, '" + val.typeName() + "' pe fita mat lagao!");
}

Value UserFunction::callSync(Interpreter &interpreter, const std::vector<Value> &args)
{
    interpreter.enterScope();
    int funcScopeDepth = interpreter.getCurrentScopeDepth();
    auto env = std::make_shared<Environment>(closure, funcScopeDepth);

    size_t argIdx = 0;
    for (size_t i = 0; i < declaration->paramList.size(); ++i)
    {
        const auto &param = declaration->paramList[i];
        if (param.isMapDestructure)
        {
            Value argVal = (argIdx < args.size()) ? args[argIdx++] : Value(std::make_shared<MemeMap>());
            std::shared_ptr<MemeMap> map;
            if (argVal.deref().isMap())
            {
                map = argVal.deref().getMap();
            }
            for (const auto &k : param.destructureKeys)
            {
                if (map && map->has(k.name))
                {
                    env->define(k.name, map->entries[k.name]);
                }
                else if (k.defaultValue)
                {
                    Value defVal = interpreter.evaluateInEnv(k.defaultValue.get(), env);
                    env->define(k.name, defVal);
                }
                else
                {
                    env->define(k.name, Value(std::monostate{}));
                }
            }
        }
        else if (param.isVariadic)
        {
            auto restList = std::make_shared<MemeList>();
            while (argIdx < args.size())
            {
                restList->elements.push_back(args[argIdx++]);
            }
            env->define(param.name, Value(restList));
        }
        else if (argIdx < args.size())
        {
            env->define(param.name, args[argIdx++]);
        }
        else
        {
            if (param.defaultValue)
            {
                Value defVal = interpreter.evaluateInEnv(param.defaultValue.get(), env);
                env->define(param.name, defVal);
            }
            else
            {
                env->define(param.name, Value(std::monostate{}));
            }
        }
    }

    try
    {
        if (auto *block = dynamic_cast<const BlockStmt *>(declaration->body.get()))
        {
            interpreter.executeBlock(block->statements, env);
        }
        else
        {
            interpreter.executeInEnv(declaration->body.get(), env);
        }
        interpreter.cleanupScope(env, funcScopeDepth);
    }
    catch (const ReturnSignal &sig)
    {
        interpreter.cleanupScope(env, funcScopeDepth);
        return sig.value;
    }
    catch (...)
    {
        interpreter.cleanupScope(env, funcScopeDepth);
        throw;
    }

    return Value(std::monostate{});
}

Value UserFunction::call(Interpreter &interpreter, const std::vector<Value> &args)
{
    if (isAsync())
    {
        return interpreter.executeAsyncFunction(this, args);
    }
    return callSync(interpreter, args);
}

Value Interpreter::executeAsyncFunction(UserFunction *fn, const std::vector<Value> &args)
{
    auto state = std::make_shared<PromiseState>();
    auto promiseMap = createPromiseMap(state, this);

#if defined(__EMSCRIPTEN__)
    try
    {
        Value res = fn->callSync(*this, args);
        state->resolve(res);
    }
    catch (const MemeUserException &e)
    {
        state->reject(e.value);
    }
    catch (const std::exception &e)
    {
        state->reject(Value(createErrorMap(e.what(), categorizeErrorMessage(e.what()), 0)));
    }
    catch (...)
    {
        state->reject(Value(createErrorMap("Async execution failed unexpectedly", "RuntimeLafda", 0)));
    }
#else
    auto globalsPtr = this->getGlobals();
    auto argsCopy = args;
    std::thread([globalsPtr, fn, argsCopy, state]() {
        try
        {
            Interpreter childInterp;
            childInterp.setGlobals(globalsPtr);
            Value res = fn->callSync(childInterp, argsCopy);
            state->resolve(res);
        }
        catch (const MemeUserException &e)
        {
            state->reject(e.value);
        }
        catch (const std::exception &e)
        {
            state->reject(Value(createErrorMap(e.what(), categorizeErrorMessage(e.what()), 0)));
        }
        catch (...)
        {
            state->reject(Value(createErrorMap("Async execution failed unexpectedly", "RuntimeLafda", 0)));
        }
    }).detach();
#endif

    return Value(promiseMap);
}

Interpreter::Interpreter(std::ostream &out, std::istream &in)
    : globals(std::make_shared<Environment>(nullptr, 0)), environment(globals), out(out), in(in)
{
    // Type checking
    auto typeFunc = Value(std::make_shared<PataLagaBuiltin>());
    globals->define("kya_hai", typeFunc);
    globals->define("aukaat", typeFunc);
    globals->define("pata_laga", typeFunc);

    // String length
    auto lenFunc = Value(std::make_shared<LambaiBuiltin>());
    globals->define("size", lenFunc);
    globals->define("kitna_lamba", lenFunc);
    globals->define("lambai", lenFunc);
    globals->define("len", lenFunc);
    globals->define("length", lenFunc);

    // Whole integer conversion
    auto intFunc = Value(std::make_shared<AnkBuiltin>());
    globals->define("bina_point", intFunc);
    globals->define("pura_num", intFunc);
    globals->define("ank", intFunc);

    // Decimal float conversion
    auto floatFunc = Value(std::make_shared<DashamlavBuiltin>());
    globals->define("point_wala", floatFunc);
    globals->define("float_bana", floatFunc);
    globals->define("dashamlav", floatFunc);

    // Explicit cloning (nakal / clone / copy)
    auto nakalFunc = Value(std::make_shared<NakalBuiltin>());
    globals->define("nakal", nakalFunc);
    globals->define("clone", nakalFunc);
    globals->define("copy", nakalFunc);

    // Standard Library Modules: hisab, waqt, json, file
    auto hisabMod = Value(createHisabModule());
    globals->define("hisab", hisabMod);
    importedModules["hisab"] = hisabMod;

    auto waqtMod = Value(createWaqtModule());
    globals->define("waqt", waqtMod);
    importedModules["waqt"] = waqtMod;

    // Top-level sleep
    auto sleepFunc = Value(std::make_shared<NativeMethodCallable>("so_ja", 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        double sec = toDouble(args[0]);
        if (sec > 0.0)
        {
#ifndef __EMSCRIPTEN__
            std::this_thread::sleep_for(std::chrono::duration<double>(sec));
#endif
        }
        return Value(std::monostate{});
    }));
    globals->define("so_ja", sleepFunc);
    globals->define("ruk", sleepFunc);

    auto jsonMod = Value(createJsonModule());
    globals->define("json", jsonMod);
    importedModules["json"] = jsonMod;

    auto fileMod = Value(createFileModule());
    globals->define("file", fileMod);
    globals->define("fs", fileMod);
    importedModules["file"] = fileMod;
    importedModules["fs"] = fileMod;

    auto pathMod = Value(createPathModule());
    globals->define("rasta", pathMod);
    globals->define("path", pathMod);
    importedModules["rasta"] = pathMod;
    importedModules["path"] = pathMod;

    auto vaadaMod = Value(createVaadaModule());
    globals->define("Vaada", vaadaMod);
    globals->define("vaada", vaadaMod);
    globals->define("Promise", vaadaMod);
    globals->define("promise", vaadaMod);
    importedModules["Vaada"] = vaadaMod;
    importedModules["vaada"] = vaadaMod;
    importedModules["Promise"] = vaadaMod;
    importedModules["promise"] = vaadaMod;

    auto dhagaMod = Value(createDhagaModule());
    globals->define("dhaga", dhagaMod);
    globals->define("thread", dhagaMod);
    globals->define("threads", dhagaMod);
    importedModules["dhaga"] = dhagaMod;
    importedModules["thread"] = dhagaMod;
    importedModules["threads"] = dhagaMod;

    auto mpMod = Value(createKaamgarModule());
    globals->define("kaamgar", mpMod);
    globals->define("mp", mpMod);
    globals->define("process", mpMod);
    globals->define("multiprocessing", mpMod);
    importedModules["kaamgar"] = mpMod;
    importedModules["mp"] = mpMod;
    importedModules["process"] = mpMod;
    importedModules["multiprocessing"] = mpMod;

    auto jaalMod = Value(createJaalModule());
    globals->define("jaal", jaalMod);
    globals->define("http", jaalMod);
    globals->define("HTTP", jaalMod);
    importedModules["jaal"] = jaalMod;
    importedModules["http"] = jaalMod;
    importedModules["HTTP"] = jaalMod;

    auto fetchFunc = Value(std::make_shared<NativeMethodCallable>("fetch", 1, 2, [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        std::string url = args[0].deref().toString();
        Value opts = (args.size() > 1) ? args[1] : Value(std::monostate{});
        return executeFetchAsync(interp, url, opts);
    }));
    globals->define("fetch", fetchFunc);
    globals->define("mangwa", fetchFunc);

    auto galtiFunc = Value(std::make_shared<NativeMethodCallable>("Galti", 1, 2, [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string msg = args[0].deref().toString();
        std::string type = (args.size() > 1) ? args[1].deref().toString() : "UserLafda";
        return Value(createErrorMap(msg, type, 0));
    }));
    globals->define("Galti", galtiFunc);
    globals->define("Error", galtiFunc);
    globals->define("galti", galtiFunc);
    globals->define("error", galtiFunc);

    // Reflection builtin: kundli
    auto kundliFunc = Value(std::make_shared<KundliBuiltin>());
    globals->define("kundli", kundliFunc);

    // Character conversion builtins: code_de / ord, akshar_bana / chr
    auto ordFunc = Value(std::make_shared<NativeMethodCallable>("ord", 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string s = args[0].deref().toString();
        if (s.empty())
        {
            throw std::runtime_error("Khali string ka code kaise nikaalein?");
        }
        return Value(static_cast<long long>(static_cast<unsigned char>(s[0])));
    }));
    globals->define("code_de", ordFunc);
    globals->define("ord", ordFunc);

    auto chrFunc = Value(std::make_shared<NativeMethodCallable>("chr", 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        long long code = toLongLong(args[0]);
        return Value(std::string(1, static_cast<char>(code)));
    }));
    globals->define("akshar_bana", chrFunc);
    globals->define("chr", chrFunc);

    // Global rich terminal widgets: table / mez, box / dabba, clear / saaf
    auto tableFunc = Value(std::make_shared<NativeMethodCallable>("table", 1, [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        std::string res = renderTable(args[0]);
        interp.getOutputStream() << res << "\n";
        return Value(res);
    }));
    globals->define("dikha_mez", tableFunc);
    globals->define("table", tableFunc);

    auto boxFunc = Value(std::make_shared<NativeMethodCallable>("box", 1, 2, [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        std::string title = args.size() > 1 ? args[1].deref().toString() : "";
        std::string res = renderBox(args[0].deref().toString(), title);
        interp.getOutputStream() << res << "\n";
        return Value(res);
    }));
    globals->define("dikha_dabba", boxFunc);
    globals->define("box", boxFunc);

    auto clearFunc = Value(std::make_shared<NativeMethodCallable>("clear", 0, [](Interpreter &interp, const std::vector<Value> &) -> Value {
        interp.getOutputStream() << "\033[2J\033[H";
        return Value(true);
    }));
    globals->define("saaf_kar", clearFunc);
    globals->define("clear", clearFunc);

    // Modules: dikha, systumm, cli, app
    auto dikhaMod = Value(createDikhaModule(*this));
    globals->define("dikha", dikhaMod);
    globals->define("bol", dikhaMod);
    globals->define("print", dikhaMod);
    importedModules["dikha"] = dikhaMod;
    importedModules["bol"] = dikhaMod;
    importedModules["print"] = dikhaMod;

    auto systummMod = Value(createSystummModule(*this));
    globals->define("systumm", systummMod);
    globals->define("sys", systummMod);
    globals->define("os", systummMod);
    importedModules["systumm"] = systummMod;
    importedModules["sys"] = systummMod;
    importedModules["os"] = systummMod;

    auto cliMod = Value(createCliModule(*this));
    globals->define("cli", cliMod);
    importedModules["cli"] = cliMod;

    auto appMod = Value(createAppModule(*this));
    globals->define("app", appMod);
    importedModules["app"] = appMod;

    auto tijoriMod = Value(createTijoriModule(*this));
    globals->define("tijori", tijoriMod);
    globals->define("db", tijoriMod);
    globals->define("vault", tijoriMod);
    importedModules["tijori"] = tijoriMod;
    importedModules["db"] = tijoriMod;
    importedModules["vault"] = tijoriMod;

    auto dhabaMod = Value(createDhabaModule(*this));
    globals->define("dhaba", dhabaMod);
    globals->define("server", dhabaMod);
    globals->define("api", dhabaMod);
    importedModules["dhaba"] = dhabaMod;
    importedModules["server"] = dhabaMod;
    importedModules["api"] = dhabaMod;

    auto uiMod = Value(createUiModule(*this));
    globals->define("ui", uiMod);
    globals->define("view", uiMod);
    globals->define("gui", uiMod);
    importedModules["ui"] = uiMod;
    importedModules["view"] = uiMod;
    importedModules["gui"] = uiMod;

    // React Hooks & Component System
    auto useStateFn = [this](Interpreter &interp, const std::vector<Value> &args) -> Value {
        std::string compPath = interp.getCurrentComponentPath();
        int cursor = interp.getNextHookCursor(compPath);
        std::string slotKey = compPath + "@state@" + std::to_string(cursor);

        if (interp.hookSlots.find(slotKey) == interp.hookSlots.end()) {
            Value initVal = args.empty() ? Value(std::monostate{}) : args[0];
            interp.hookSlots[slotKey] = initVal;
        }

        auto resList = std::make_shared<MemeList>();
        resList->elements.push_back(interp.hookSlots[slotKey]);

        auto setter = [this, slotKey](Interpreter &it, const std::vector<Value> &sArgs) -> Value {
            if (!sArgs.empty()) {
                Value nextVal = sArgs[0];
                if (std::holds_alternative<std::shared_ptr<Callable>>(nextVal.deref().data)) {
                    auto updater = std::get<std::shared_ptr<Callable>>(nextVal.deref().data);
                    nextVal = updater->call(it, { this->hookSlots[slotKey] });
                }
                this->hookSlots[slotKey] = nextVal;
                this->triggerUiRerender();
            }
            return this->hookSlots[slotKey];
        };
        resList->elements.push_back(Value(std::make_shared<NativeMethodCallable>("set_state", 1, setter)));
        return Value(resList);
    };
    auto useStateVal = Value(std::make_shared<NativeMethodCallable>("useState", 0, 1, useStateFn));
    globals->define("useState", useStateVal);
    globals->define("state_bana", useStateVal);

    auto useEffectFn = [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        if (!args.empty()) {
            Value fnVal = args[0].deref();
            if (std::holds_alternative<std::shared_ptr<Callable>>(fnVal.data)) {
                std::get<std::shared_ptr<Callable>>(fnVal.data)->call(interp, {});
            }
        }
        return Value(std::monostate{});
    };
    auto useEffectVal = Value(std::make_shared<NativeMethodCallable>("useEffect", 1, 2, useEffectFn));
    globals->define("useEffect", useEffectVal);

    auto useMemoFn = [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        std::string compPath = interp.getCurrentComponentPath();
        int cursor = interp.getNextHookCursor(compPath);
        std::string slotKey = compPath + "@memo@" + std::to_string(cursor);

        if (interp.hookSlots.find(slotKey) == interp.hookSlots.end()) {
            if (!args.empty()) {
                Value fnVal = args[0].deref();
                if (std::holds_alternative<std::shared_ptr<Callable>>(fnVal.data)) {
                    interp.hookSlots[slotKey] = std::get<std::shared_ptr<Callable>>(fnVal.data)->call(interp, {});
                } else {
                    interp.hookSlots[slotKey] = fnVal;
                }
            } else {
                interp.hookSlots[slotKey] = Value(std::monostate{});
            }
        }
        return interp.hookSlots[slotKey];
    };
    auto useMemoVal = Value(std::make_shared<NativeMethodCallable>("useMemo", 1, 2, useMemoFn));
    globals->define("useMemo", useMemoVal);

    auto useRefFn = [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        std::string compPath = interp.getCurrentComponentPath();
        int cursor = interp.getNextHookCursor(compPath);
        std::string slotKey = compPath + "@ref@" + std::to_string(cursor);

        if (interp.hookSlots.find(slotKey) == interp.hookSlots.end()) {
            auto m = std::make_shared<MemeMap>();
            Value initVal = args.empty() ? Value(std::monostate{}) : args[0];
            m->set("current", initVal);
            interp.hookSlots[slotKey] = Value(m);
        }
        return interp.hookSlots[slotKey];
    };
    auto useRefVal = Value(std::make_shared<NativeMethodCallable>("useRef", 0, 1, useRefFn));
    globals->define("useRef", useRefVal);

    // Context API
    auto createContextFn = [this](Interpreter &, const std::vector<Value> &args) -> Value {
        auto ctxMap = std::make_shared<MemeMap>();
        std::string ctxId = "ctx_" + std::to_string(++this->nextContextId);
        Value defaultVal = args.empty() ? Value(std::monostate{}) : args[0];
        ctxMap->set("__isContext", Value(true));
        ctxMap->set("__contextId", Value(ctxId));
        ctxMap->set("id", Value(ctxId));
        ctxMap->set("__defaultValue", defaultVal);

        auto providerFn = [](Interpreter &, const std::vector<Value> &pArgs) -> Value {
            if (!pArgs.empty() && pArgs[0].deref().isMap()) {
                auto props = pArgs[0].deref().getMap();
                if (props->has("children")) {
                    return props->get("children");
                }
            }
            return Value(std::make_shared<MemeList>());
        };
        ctxMap->set("Provider", Value(std::make_shared<NativeMethodCallable>("Provider", 1, providerFn)));
        return Value(ctxMap);
    };
    auto createContextVal = Value(std::make_shared<NativeMethodCallable>("createContext", 0, 1, createContextFn));
    globals->define("createContext", createContextVal);

    auto useContextFn = [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        if (args.empty()) return Value(std::monostate{});
        Value ctxVal = args[0].deref();
        if (ctxVal.isMap()) {
            auto ctxMap = ctxVal.getMap();
            std::string ctxId = ctxMap->has("__contextId") ? ctxMap->get("__contextId").toString()
                                : (ctxMap->has("id") ? ctxMap->get("id").toString() : "");
            Value defaultVal = ctxMap->has("__defaultValue") ? ctxMap->get("__defaultValue") : Value(std::monostate{});
            return interp.getContext(ctxId, defaultVal);
        }
        return Value(std::monostate{});
    };
    auto useContextVal = Value(std::make_shared<NativeMethodCallable>("useContext", 1, useContextFn));
    globals->define("useContext", useContextVal);

    // Router API
    auto useRouterFn = [](Interpreter &interp, const std::vector<Value> &) -> Value {
        auto rMap = std::make_shared<MemeMap>();
        rMap->set("path", Value(interp.getRouterPath()));
        rMap->set("push", Value(std::make_shared<NativeMethodCallable>("push", 1, [&interp, rMap](Interpreter &, const std::vector<Value> &rArgs) -> Value {
            if (!rArgs.empty()) {
                std::string newPath = rArgs[0].deref().toString();
                rMap->set("path", Value(newPath));
                interp.setRouterPath(newPath);
            }
            return Value(std::monostate{});
        })));
        rMap->set("back", Value(std::make_shared<NativeMethodCallable>("back", 0, [&interp, rMap](Interpreter &, const std::vector<Value> &) -> Value {
            if (!interp.routerHistory.empty()) {
                interp.routerHistory.pop_back();
                std::string prev = interp.routerHistory.empty() ? "/" : interp.routerHistory.back();
                rMap->set("path", Value(prev));
                interp.setRouterPath(prev);
            }
            return Value(std::monostate{});
        })));
        return Value(rMap);
    };
    auto useRouterVal = Value(std::make_shared<NativeMethodCallable>("useRouter", 0, useRouterFn));
    globals->define("useRouter", useRouterVal);

    auto routerCompFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        if (!args.empty() && args[0].deref().isMap()) {
            auto props = args[0].deref().getMap();
            if (props->has("children")) {
                return props->get("children");
            }
        }
        return Value(std::make_shared<MemeList>());
    };
    auto routerCompVal = Value(std::make_shared<NativeMethodCallable>("Router", 1, routerCompFn));
    globals->define("Router", routerCompVal);

    auto routeCompFn = [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        if (!args.empty() && args[0].deref().isMap()) {
            auto props = args[0].deref().getMap();
            std::string routePath = props->has("path") ? props->get("path").toString() : "/";
            if (interp.getRouterPath() == routePath || routePath == "*") {
                if (props->has("element")) return props->get("element");
                if (props->has("children")) return props->get("children");
            }
        }
        return Value(std::monostate{});
    };
    auto routeCompVal = Value(std::make_shared<NativeMethodCallable>("Route", 1, routeCompFn));
    globals->define("Route", routeCompVal);

    auto linkCompFn = [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        auto props = (!args.empty() && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        std::string to = props->has("to") ? props->get("to").toString() : "/";
        auto node = std::make_shared<MemeMap>();
        node->set("type", Value("jsx_element"));
        node->set("tag", Value("a"));
        auto navFn = [&interp, to](Interpreter &, const std::vector<Value> &) -> Value {
            interp.setRouterPath(to);
            return Value(std::monostate{});
        };
        props->set("onClick", Value(std::make_shared<NativeMethodCallable>("nav", 0, 1, navFn)));
        props->set("href", Value(to));
        node->set("props", Value(props));
        node->set("children", props->has("children") ? props->get("children") : Value(std::make_shared<MemeList>()));
        return Value(node);
    };
    auto linkCompVal = Value(std::make_shared<NativeMethodCallable>("Link", 1, linkCompFn));
    globals->define("Link", linkCompVal);

    // Testing matchers: test() & expect()
    auto expectFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        Value actual = args.empty() ? Value(std::monostate{}) : args[0];
        auto matchers = std::make_shared<MemeMap>();

        matchers->set("toBe", Value(std::make_shared<NativeMethodCallable>("toBe", 1, [actual](Interpreter &, const std::vector<Value> &mArgs) -> Value {
            Value expected = mArgs.empty() ? Value(std::monostate{}) : mArgs[0];
            if (actual.toString() != expected.toString()) {
                throw std::runtime_error("Test failure! Expected " + expected.toString() + " but got " + actual.toString());
            }
            return Value(true);
        })));

        matchers->set("toEqual", Value(std::make_shared<NativeMethodCallable>("toEqual", 1, [actual](Interpreter &, const std::vector<Value> &mArgs) -> Value {
            Value expected = mArgs.empty() ? Value(std::monostate{}) : mArgs[0];
            if (actual.toString() != expected.toString()) {
                throw std::runtime_error("Test failure! Expected " + expected.toString() + " but got " + actual.toString());
            }
            return Value(true);
        })));

        matchers->set("toBeTruthy", Value(std::make_shared<NativeMethodCallable>("toBeTruthy", 0, [actual](Interpreter &, const std::vector<Value> &) -> Value {
            if (!actual.isTruthy()) {
                throw std::runtime_error("Test failure! Expected truthy value but got " + actual.toString());
            }
            return Value(true);
        })));
        matchers->set("sach", matchers->get("toBeTruthy"));

        matchers->set("toBeFalsy", Value(std::make_shared<NativeMethodCallable>("toBeFalsy", 0, [actual](Interpreter &, const std::vector<Value> &) -> Value {
            if (actual.isTruthy()) {
                throw std::runtime_error("Test failure! Expected falsy value but got " + actual.toString());
            }
            return Value(true);
        })));
        matchers->set("jhooth", matchers->get("toBeFalsy"));

        matchers->set("toContain", Value(std::make_shared<NativeMethodCallable>("toContain", 1, [actual](Interpreter &, const std::vector<Value> &mArgs) -> Value {
            std::string sub = mArgs.empty() ? "" : mArgs[0].deref().toString();
            if (actual.toString().find(sub) == std::string::npos) {
                throw std::runtime_error("Test failure! Expected '" + actual.toString() + "' to contain '" + sub + "'");
            }
            return Value(true);
        })));

        matchers->set("toThrow", Value(std::make_shared<NativeMethodCallable>("toThrow", 0, [actual](Interpreter &it, const std::vector<Value> &) -> Value {
            if (!std::holds_alternative<std::shared_ptr<Callable>>(actual.deref().data)) {
                throw std::runtime_error("expect(...).toThrow() requires a function/callable argument!");
            }
            auto callable = std::get<std::shared_ptr<Callable>>(actual.deref().data);
            bool didThrow = false;
            try {
                callable->call(it, {});
            } catch (...) {
                didThrow = true;
            }
            if (!didThrow) {
                throw std::runtime_error("Test failure! Expected function to throw an error, but it succeeded!");
            }
            return Value(true);
        })));
        matchers->set("chillaye", matchers->get("toThrow"));

        return Value(matchers);
    };
    globals->define("expect", Value(std::make_shared<NativeMethodCallable>("expect", 1, expectFn)));
    globals->define("umeed", Value(std::make_shared<NativeMethodCallable>("umeed", 1, expectFn)));
    globals->define("check", Value(std::make_shared<NativeMethodCallable>("check", 1, expectFn)));

    auto testFn = [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        std::string name = args.empty() ? "anonymous test" : args[0].deref().toString();
        Value fnVal = args.size() > 1 ? args[1].deref() : Value(std::monostate{});
        if (std::holds_alternative<std::shared_ptr<Callable>>(fnVal.data)) {
            auto callable = std::get<std::shared_ptr<Callable>>(fnVal.data);
            try {
                callable->call(interp, {});
                interp.getOutputStream() << "  ✓ " << name << "\n";
                return Value(true);
            } catch (const std::exception &ex) {
                interp.getOutputStream() << "  ✗ " << name << ": " << ex.what() << "\n";
                throw;
            }
        }
        return Value(false);
    };
    globals->define("test", Value(std::make_shared<NativeMethodCallable>("test", 2, testFn)));
    globals->define("jaanch_kar", Value(std::make_shared<NativeMethodCallable>("jaanch_kar", 2, testFn)));
    globals->define("it", Value(std::make_shared<NativeMethodCallable>("it", 2, testFn)));

    // Also populate ui module with React & Router components
    if (std::holds_alternative<std::shared_ptr<MemeMap>>(uiMod.data)) {
        auto uim = std::get<std::shared_ptr<MemeMap>>(uiMod.data);
        uim->set("useState", useStateVal);
        uim->set("useEffect", useEffectVal);
        uim->set("useMemo", useMemoVal);
        uim->set("useRef", useRefVal);
        uim->set("createContext", createContextVal);
        uim->set("useContext", useContextVal);
        uim->set("useRouter", useRouterVal);
        uim->set("Router", routerCompVal);
        uim->set("Route", routeCompVal);
        uim->set("Link", linkCompVal);
    }

    // Bridge module
    auto bridgeMap = std::make_shared<MemeMap>();
    bridgeMap->isModule = true;
    bridgeMap->set("python", Value(std::make_shared<NativeMethodCallable>("python", 1, [](Interpreter &, const std::vector<Value> &args) {
        return EcosystemBridge::createPythonBridge(args.empty() ? "" : args[0].deref().toString());
    })));
    bridgeMap->set("npm", Value(std::make_shared<NativeMethodCallable>("npm", 1, [](Interpreter &, const std::vector<Value> &args) {
        return EcosystemBridge::createNpmBridge(args.empty() ? "" : args[0].deref().toString());
    })));
    bridgeMap->set("rust", Value(std::make_shared<NativeMethodCallable>("rust", 1, [](Interpreter &, const std::vector<Value> &args) {
        return EcosystemBridge::createRustBridge(args.empty() ? "" : args[0].deref().toString());
    })));
    globals->define("bridge", Value(bridgeMap));
}

void Interpreter::setCommandLineArgs(const std::vector<std::string> &args)
{
    commandLineArgs = args;
    if (globals && globals->findSlot("systumm"))
    {
        Value sysVal = globals->get("systumm");
        if (sysVal.isMap())
        {
            auto argsList = std::make_shared<MemeList>();
            for (const auto &arg : commandLineArgs)
            {
                argsList->elements.push_back(Value(arg));
            }
            sysVal.getMap()->set("args", Value(argsList));
        }
    }
}

std::string Interpreter::stringify(const Value &val)
{
    Value d = val.deref();
    if (d.isInstance())
    {
        auto inst = d.getInstance();
        if (inst && inst->getClass())
        {
            auto method = inst->getClass()->findMethod("__dikha__");
            if (!method) method = inst->getClass()->findMethod("__str__");
            if (!method) method = inst->getClass()->findMethod("__repr__");
            if (method)
            {
                BoundMethod bound(inst, method);
                Value res = bound.call(*this, {});
                return res.deref().toString();
            }
        }
    }
    return d.toString();
}

Value KundliBuiltin::call(Interpreter &, const std::vector<Value> &args)
{
    Value val = args[0].deref();
    if (val.isInstance())
    {
        auto inst = val.getInstance();
        auto map = std::make_shared<MemeMap>();
        for (const auto &pair : inst->getFields())
        {
            map->set(pair.first, pair.second);
        }
        return Value(map);
    }
    if (std::holds_alternative<std::shared_ptr<Callable>>(val.data))
    {
        auto callable = std::get<std::shared_ptr<Callable>>(val.data);
        if (auto *cls = dynamic_cast<ClassCallable *>(callable.get()))
        {
            auto list = std::make_shared<MemeList>();
            for (const auto &m : cls->getMethodNames())
            {
                list->elements.push_back(Value(m));
            }
            return Value(list);
        }
    }
    throw std::runtime_error("Kundli sirf biradari ya object instance ki nikali ja sakti hai! Mila: " + val.typeName());
}

void Interpreter::dropInstance(std::shared_ptr<Instance> instance)
{
    if (!instance || instance->isDropped)
        return;

    instance->isDropped = true;
    auto dropMethod = instance->getClass()->findMethod("alvida");
    if (!dropMethod)
    {
        dropMethod = instance->getClass()->findMethod("drop");
    }

    if (dropMethod)
    {
        BoundMethod bound(instance, dropMethod);
        bound.call(*this, {});
    }
}

void Interpreter::cleanupScope(std::shared_ptr<Environment> env, int scopeDepth)
{
    // 1. Release active borrows that were created at scopeDepth or deeper
    for (auto it = activeBorrows.begin(); it != activeBorrows.end();)
    {
        if (it->scopeDepth >= scopeDepth)
        {
            if (it->env)
            {
                VariableSlot *slot = it->env->findSlot(it->varName);
                if (slot)
                {
                    if (it->isMutable)
                    {
                        slot->hasExclusiveBorrow = false;
                    }
                    else
                    {
                        slot->sharedBorrows = std::max(0, slot->sharedBorrows - 1);
                    }
                }
            }
            it = activeBorrows.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // 2. Drop instances owned by this environment scope (if not moved)
    if (env)
    {
        for (auto &[name, slot] : env->getSlots())
        {
            if (!slot.isMoved && slot.value.isInstance())
            {
                dropInstance(slot.value.getInstance());
                slot.isMoved = true;
            }
        }
    }

    if (currentScopeDepth >= scopeDepth && currentScopeDepth > 0)
    {
        currentScopeDepth--;
    }
}

Value Interpreter::cloneValue(const Value &val)
{
    Value v = val.deref();
    if (v.isInstance())
    {
        auto inst = v.getInstance();
        return inst ? Value(inst->clone()) : v;
    }
    if (v.isList())
    {
        auto list = v.getList();
        auto newList = std::make_shared<MemeList>();
        if (list)
        {
            newList->elements.reserve(list->elements.size());
            for (const auto &elem : list->elements)
            {
                newList->elements.push_back(cloneValue(elem));
            }
        }
        return Value(newList);
    }
    if (v.isMap())
    {
        auto map = v.getMap();
        auto newMap = std::make_shared<MemeMap>();
        if (map)
        {
            for (const auto &k : map->keyOrder)
            {
                auto it = map->entries.find(k);
                if (it != map->entries.end())
                {
                    newMap->set(k, cloneValue(it->second));
                }
            }
        }
        return Value(newMap);
    }
    return v;
}

Value Interpreter::moveVariable(const std::string &varName)
{
    VariableSlot *slot = environment->findSlot(varName);
    if (!slot)
    {
        throw std::runtime_error(
            "Rasode mein kaun tha?! Ye '" + varName + "' bina bataye kahan se aaya? Pehle 'pakad " + varName + ";' se declare kar!");
    }

    if (slot->isMoved)
    {
        throw std::runtime_error(
            "Pehli fursat mein nikal! '" + varName + "' ki value move ho chuki hai! Ownership transfer ke baad use karna mana hai!");
    }

    if (slot->hasExclusiveBorrow)
    {
        throw std::runtime_error(
            "Beech mein mat bolo! '" + varName + "' abhi kisi aur ko udhaar diya hua hai!");
    }

    if (slot->sharedBorrows > 0)
    {
        if (slot->value.isMoveType())
        {
            throw std::runtime_error(
                "Khatam nahi hua abhi! '" + varName + "' udhaar pe hai, move nahi kar sakte!");
        }
    }

    // If it's a move type (string, instance, callable), transfer ownership!
    if (slot->value.isMoveType())
    {
        slot->isMoved = true;
        return slot->value;
    }

    // Primitive types and references are copied
    return slot->value;
}

static void autoLoadEnvFile(const std::filesystem::path &dir)
{
    std::filesystem::path envFile = dir / ".env";
    if (!std::filesystem::exists(envFile)) return;
    std::ifstream f(envFile);
    if (!f.is_open()) return;
    std::string line;
    while (std::getline(f, line)) {
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        if (line[start] == '#') continue;
        size_t eq = line.find('=', start);
        if (eq == std::string::npos) continue;
        std::string key = line.substr(start, eq - start);
        size_t keyEnd = key.find_last_not_of(" \t\r\n");
        if (keyEnd != std::string::npos) key = key.substr(0, keyEnd + 1);
        std::string val = line.substr(eq + 1);
        size_t valStart = val.find_first_not_of(" \t\r\n");
        if (valStart != std::string::npos) {
            val = val.substr(valStart);
            size_t valEnd = val.find_last_not_of(" \t\r\n");
            if (valEnd != std::string::npos) val = val.substr(0, valEnd + 1);
        } else {
            val = "";
        }
        if (val.length() >= 2 && ((val.front() == '"' && val.back() == '"') || (val.front() == '\'' && val.back() == '\''))) {
            val = val.substr(1, val.length() - 2);
        }
#ifndef _WIN32
        setenv(key.c_str(), val.c_str(), 0);
#else
        _putenv_s(key.c_str(), val.c_str());
#endif
    }
}

void Interpreter::interpret(std::vector<std::unique_ptr<Stmt>> &&statements)
{
    astStorage.push_back(std::move(statements));
    interpret(astStorage.back());
}

void Interpreter::interpret(const std::vector<std::unique_ptr<Stmt>> &statements)
{
    std::filesystem::path checkDir = !currentFilePath.empty() ? std::filesystem::absolute(currentFilePath).parent_path() : std::filesystem::current_path();
    while (!checkDir.empty()) {
        if (std::filesystem::exists(checkDir / ".env")) {
            autoLoadEnvFile(checkDir);
            break;
        }
        if (!checkDir.has_relative_path() || checkDir == checkDir.parent_path()) break;
        checkDir = checkDir.parent_path();
    }
    autoLoadEnvFile(std::filesystem::current_path());
    try
    {
        for (const auto &stmt : statements)
        {
            execute(stmt.get());
        }

        if (lastGpuCommandsJson.empty())
        {
            std::vector<std::string> candidates = {"appTree", "screen", "app", "uiTree", "mainScreen", "root", "card"};
            for (const auto &cand : candidates)
            {
                VariableSlot *slot = environment->findSlot(cand);
                if (!slot) slot = globals->findSlot(cand);
                if (slot && slot->value.deref().isMap())
                {
                    auto m = slot->value.deref().getMap();
                    if (m && (m->has("type") || m->has("props") || m->has("tag")))
                    {
                        auto cmds = TailwindEngine::computeNativeLayout(slot->value.deref(), 800.0f, 600.0f);
                        if (!cmds.empty())
                        {
                            lastGpuCommandsJson = TailwindEngine::gpuCommandsToJson(cmds);
                            break;
                        }
                    }
                }
            }
        }

        cleanupScope(globals, 0);
    }
    catch (const BreakSignal &)
    {
        throw std::runtime_error("Line " + std::to_string(currentLine) + ": Kisko rok rahe ho? Koi loop chal hi nahi raha toh 'bas_kar;' kyun likha?");
    }
    catch (const ContinueSignal &)
    {
        throw std::runtime_error("Line " + std::to_string(currentLine) + ": Kahan aage badhna hai? Loop ke bahar 'aage_badh;' kaam nahi karta!");
    }
    catch (const ReturnSignal &)
    {
        throw std::runtime_error("Line " + std::to_string(currentLine) + ": Kisko de rahe ho? Function ('jugaad') ke bahar 'ye_le;' nahi chalta!");
    }
    catch (const MemeUserException &e)
    {
        int line = (e.line > 0) ? e.line : currentLine;
        throw std::runtime_error("Line " + std::to_string(line) + ": Lafda ho gaya! Uncaught exception: " + e.value.toString());
    }
    catch (const std::runtime_error &e)
    {
        std::string msg = e.what();
        if (msg.rfind("Line ", 0) == 0)
        {
            throw;
        }
        throw std::runtime_error("Line " + std::to_string(currentLine) + ": " + msg);
    }
}

void Interpreter::executeBlock(const std::vector<std::unique_ptr<Stmt>> &statements, std::shared_ptr<Environment> newEnv)
{
    auto previous = this->environment;
    this->environment = newEnv;
    enterScope();
    int scopeDepth = currentScopeDepth;

    try
    {
        for (const auto &stmt : statements)
        {
            execute(stmt.get());
        }
        cleanupScope(newEnv, scopeDepth);
        this->environment = previous;
    }
    catch (const ReturnSignal &)
    {
        cleanupScope(newEnv, scopeDepth);
        this->environment = previous;
        throw;
    }
    catch (...)
    {
        cleanupScope(newEnv, scopeDepth);
        this->environment = previous;
        throw;
    }
}

Value Interpreter::evaluateInEnv(const Expr *expr, std::shared_ptr<Environment> targetEnv)
{
    auto prevEnv = this->environment;
    this->environment = targetEnv;
    try
    {
        Value val = evaluate(expr);
        this->environment = prevEnv;
        return val;
    }
    catch (...)
    {
        this->environment = prevEnv;
        throw;
    }
}

void Interpreter::executeInEnv(const Stmt *stmt, std::shared_ptr<Environment> targetEnv)
{
    auto prevEnv = this->environment;
    this->environment = targetEnv;
    try
    {
        execute(stmt);
        this->environment = prevEnv;
    }
    catch (...)
    {
        this->environment = prevEnv;
        throw;
    }
}

Value Interpreter::importModule(const std::string &path, int line)
{
    (void)line;

    // Workspace / Project root alias: @/...
    if (path.rfind("@/", 0) == 0)
    {
        std::string rel = path.substr(2);
        std::filesystem::path root = std::filesystem::current_path();
        if (!currentFilePath.empty()) {
            std::filesystem::path check = std::filesystem::path(currentFilePath).parent_path();
            while (!check.empty()) {
                if (std::filesystem::exists(check / "tapri.json") || std::filesystem::exists(check / ".meme_env")) {
                    root = check;
                    break;
                }
                if (!check.has_relative_path() || check == check.parent_path()) break;
                check = check.parent_path();
            }
        }
        autoLoadEnvFile(root);
        std::filesystem::path target = std::filesystem::absolute(root / rel);
        if (!std::filesystem::exists(target) && !target.has_extension()) {
            target += ".meme";
        }
        if (!std::filesystem::exists(target)) {
            std::filesystem::path srcTarget = std::filesystem::absolute(root / "src" / rel);
            if (!std::filesystem::exists(srcTarget) && !srcTarget.has_extension()) {
                srcTarget += ".meme";
            }
            if (std::filesystem::exists(srcTarget)) {
                target = srcTarget;
            }
        }
        return importModule(target.string(), line);
    }

    // Check built-in modules or exact-cached names first (e.g. "hisab", "json", "file", "waqt")
    auto bit = importedModules.find(path);
    if (bit != importedModules.end())
    {
        return bit->second;
    }

    // 1. Python Bridge (py:<pkg>)
    if (path.rfind("py:", 0) == 0)
    {
        Value mod = EcosystemBridge::createPythonBridge(path.substr(3));
        importedModules[path] = mod;
        return mod;
    }

    // 2. NPM / JS Bridge (npm:<pkg> / js:<pkg>)
    if (path.rfind("npm:", 0) == 0)
    {
        Value mod = EcosystemBridge::createNpmBridge(path.substr(4));
        importedModules[path] = mod;
        return mod;
    }
    if (path.rfind("js:", 0) == 0)
    {
        Value mod = EcosystemBridge::createNpmBridge(path.substr(3));
        importedModules[path] = mod;
        return mod;
    }

    // 3. Rust & Native FFI Bridge (rust:<target> / ffi:<path>)
    if (path.rfind("rust:", 0) == 0)
    {
        Value mod = EcosystemBridge::createRustBridge(path.substr(5));
        importedModules[path] = mod;
        return mod;
    }
    if (path.rfind("ffi:", 0) == 0)
    {
        Value mod = EcosystemBridge::createFfiBridge(path.substr(4));
        importedModules[path] = mod;
        return mod;
    }

    // 4. UI:shadcn & UI components (ui:shadcn)
    if (path == "ui:shadcn" || path.rfind("ui:", 0) == 0)
    {
        Value mod = Value(ShadcnComponents::createShadcnModule());
        importedModules[path] = mod;
        return mod;
    }

    std::filesystem::path modulePath(path);
    if (modulePath.is_relative() && !currentFilePath.empty())
    {
        std::filesystem::path baseDir = std::filesystem::path(currentFilePath).parent_path();
        modulePath = baseDir / modulePath;
    }

    std::error_code ec;
    std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(modulePath, ec);
    if (ec)
    {
        canonicalPath = modulePath.lexically_normal();
    }

    if (!std::filesystem::exists(canonicalPath))
    {
        if (!canonicalPath.has_extension())
        {
            auto withExt = canonicalPath;
            withExt += ".meme";
            if (std::filesystem::exists(withExt))
            {
                canonicalPath = withExt;
            }
        }
    }

    // Check .tapri_modules/<path>.meme or .tapri_modules/<path>/index.meme
    if (!std::filesystem::exists(canonicalPath))
    {
        std::filesystem::path tapriPath = std::filesystem::path(".tapri_modules") / path;
        if (std::filesystem::exists(tapriPath))
        {
            canonicalPath = tapriPath;
        }
        else if (std::filesystem::exists(tapriPath.string() + ".meme"))
        {
            canonicalPath = tapriPath.string() + ".meme";
        }
        else if (std::filesystem::exists(tapriPath / "index.meme"))
        {
            canonicalPath = tapriPath / "index.meme";
        }
    }

    // Check .meme_env/pkgs/<path>.meme or .meme_env/pkgs/<path>/index.meme
    if (!std::filesystem::exists(canonicalPath))
    {
        std::filesystem::path envPkgPath = std::filesystem::path(".meme_env") / "pkgs" / path;
        if (std::filesystem::exists(envPkgPath))
        {
            canonicalPath = envPkgPath;
        }
        else if (std::filesystem::exists(envPkgPath.string() + ".meme"))
        {
            canonicalPath = envPkgPath.string() + ".meme";
        }
        else if (std::filesystem::exists(envPkgPath / "index.meme"))
        {
            canonicalPath = envPkgPath / "index.meme";
        }
    }

    std::string canonicalStr = canonicalPath.string();

    // Circular import detection
    for (const auto &p : importStack)
    {
        if (p == canonicalStr)
        {
            throw std::runtime_error("Golmaal hai sab golmaal hai! Circular import pakda gaya: '" + path + "'");
        }
    }

    // Module cache
    auto it = importedModules.find(canonicalStr);
    if (it != importedModules.end())
    {
        return it->second;
    }

    std::ifstream file(canonicalPath);
    if (!file.is_open())
    {
        throw std::runtime_error("Bulao kisko rahe ho? File mili hi nahi: '" + path + "'");
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string fileContent = buffer.str();
    file.close();

    Lexer modLexer(fileContent);
    auto modTokens = modLexer.tokenize();
    Parser modParser(modTokens);
    auto modStmts = modParser.parseProgram();

    importedAsts.push_back(std::move(modStmts));
    const auto &stmtsToExecute = importedAsts.back();

    auto modEnv = std::make_shared<Environment>(globals, 0);
    auto prevEnv = this->environment;
    auto prevPath = this->currentFilePath;
    auto prevScope = this->currentScopeDepth;

    this->environment = modEnv;
    this->currentFilePath = canonicalStr;
    this->currentScopeDepth = 0;
    importStack.push_back(canonicalStr);

    Value explicitReturn(std::monostate{});
    bool hasExplicitReturn = false;

    try
    {
        for (const auto &s : stmtsToExecute)
        {
            execute(s.get());
        }
        cleanupScope(modEnv, 0);
    }
    catch (const ReturnSignal &ret)
    {
        explicitReturn = ret.value;
        hasExplicitReturn = true;
        cleanupScope(modEnv, 0);
    }
    catch (...)
    {
        importStack.pop_back();
        this->environment = prevEnv;
        this->currentFilePath = prevPath;
        this->currentScopeDepth = prevScope;
        throw;
    }

    importStack.pop_back();
    this->environment = prevEnv;
    this->currentFilePath = prevPath;
    this->currentScopeDepth = prevScope;

    if (hasExplicitReturn)
    {
        importedModules[canonicalStr] = explicitReturn;
        return explicitReturn;
    }

    auto moduleMap = std::make_shared<MemeMap>();
    moduleMap->isModule = true;
    for (const auto &[name, slot] : modEnv->getSlots())
    {
        moduleMap->set(name, slot.value);
    }
    Value modVal(moduleMap);
    importedModules[canonicalStr] = modVal;
    return modVal;
}

void Interpreter::execute(const Stmt *stmt)
{
    if (!stmt)
        return;

    if (stmt->line > 0)
    {
        currentLine = stmt->line;
    }

    try
    {

    // 1. Variable declaration: `pakad x = 5;`
    if (auto *v = dynamic_cast<const VarDeclStmt *>(stmt))
    {
        Value val;
        if (v->initializer)
        {
            if (auto *ident = dynamic_cast<const IdentifierExpr *>(v->initializer.get()))
            {
                val = moveVariable(ident->name);
            }
            else
            {
                val = evaluate(v->initializer.get());
            }
        }
        else
        {
            val = Value(std::monostate{});
        }
        environment->define(v->name, std::move(val));
        return;
    }

    // Array Destructure statement: `pakad [a, b, *baaki] = expr;`
    if (auto *ads = dynamic_cast<const ArrayDestructureStmt *>(stmt))
    {
        Value val = evaluate(ads->initializer.get()).deref();
        if (!val.isList())
        {
            throw std::runtime_error("Destructuring sirf Array / list pe ho sakti hai! Mila: " + val.typeName());
        }
        auto list = val.getList();

        size_t listIdx = 0;
        for (const auto &item : ads->items)
        {
            Value elemVal;
            if (item.isVariadic)
            {
                auto restList = std::make_shared<MemeList>();
                while (listIdx < list->elements.size())
                {
                    restList->elements.push_back(list->elements[listIdx++]);
                }
                elemVal = Value(restList);
            }
            else
            {
                elemVal = (listIdx < list->elements.size()) ? list->elements[listIdx++] : Value(std::monostate{});
            }

            VariableSlot *slot = environment->findSlot(item.name);
            if (slot)
            {
                environment->assign(item.name, elemVal);
            }
            else
            {
                environment->define(item.name, elemVal);
            }
        }
        return;
    }

    // Map Destructure statement: `pakad {naam, umar} = expr;`
    if (auto *mds = dynamic_cast<const MapDestructureStmt *>(stmt))
    {
        Value val = evaluate(mds->initializer.get()).deref();
        if (val.isMap())
        {
            auto map = val.getMap();
            for (const auto &key : mds->keys)
            {
                if (map && map->has(key))
                {
                    environment->define(key, map->entries[key]);
                }
                else
                {
                    environment->define(key, Value(std::monostate{}));
                }
            }
        }
        else if (val.isInstance())
        {
            auto inst = val.getInstance();
            for (const auto &key : mds->keys)
            {
                try
                {
                    environment->define(key, inst->get(key));
                }
                catch (...)
                {
                    environment->define(key, Value(std::monostate{}));
                }
            }
        }
        else
        {
            for (const auto &key : mds->keys)
            {
                environment->define(key, Value(std::monostate{}));
            }
        }
        return;
    }

    // 2. Expression statement: `x = 10;` or `x + 1;`
    if (auto *e = dynamic_cast<const ExprStmt *>(stmt))
    {
        evaluate(e->expression.get());
        return;
    }

    // 3. Print statement: `dikha a, b;`
    if (auto *p = dynamic_cast<const PrintStmt *>(stmt))
    {
        for (size_t i = 0; i < p->args.size(); ++i)
        {
            if (i > 0)
                out << " ";
            Value val = evaluate(p->args[i].get());
            out << formatRichText(stringify(val));
        }
        out << "\n";
        return;
    }

    // 4. Block statement: `{ ... }`
    if (auto *b = dynamic_cast<const BlockStmt *>(stmt))
    {
        executeBlock(b->statements, std::make_shared<Environment>(environment, currentScopeDepth + 1));
        return;
    }

    // 5. If statement: `dekh cond { ... } warna { ... }`
    if (auto *i = dynamic_cast<const IfStmt *>(stmt))
    {
        Value cond = evaluate(i->condition.get());
        if (cond.isTruthy())
        {
            execute(i->thenBranch.get());
        }
        else if (i->elseBranch)
        {
            execute(i->elseBranch.get());
        }
        return;
    }

    // 6. While statement: `jabtak cond { ... }`
    if (auto *w = dynamic_cast<const WhileStmt *>(stmt))
    {
        while (evaluate(w->condition.get()).isTruthy())
        {
            try
            {
                execute(w->body.get());
            }
            catch (const BreakSignal &)
            {
                break;
            }
            catch (const ContinueSignal &)
            {
                continue;
            }
        }
        return;
    }

    // 7. For/Range statement: `ghuma i se 1 se 10 [se step] { ... }`
    if (auto *g = dynamic_cast<const GhoomStmt *>(stmt))
    {
        Value startVal = evaluate(g->start.get());
        Value endVal = evaluate(g->end.get());

        long long start = 0;
        long long end = 0;

        startVal = startVal.deref();
        endVal = endVal.deref();

        if (std::holds_alternative<long long>(startVal.data))
        {
            start = std::get<long long>(startVal.data);
        }
        else if (std::holds_alternative<double>(startVal.data))
        {
            start = static_cast<long long>(std::get<double>(startVal.data));
        }
        else
        {
            throw std::runtime_error("Range ka start number hona chahiye, ye kya de diya: " + startVal.typeName());
        }

        if (std::holds_alternative<long long>(endVal.data))
        {
            end = std::get<long long>(endVal.data);
        }
        else if (std::holds_alternative<double>(endVal.data))
        {
            end = static_cast<long long>(std::get<double>(endVal.data));
        }
        else
        {
            throw std::runtime_error("Range ka end number hona chahiye, ye kya de diya: " + endVal.typeName());
        }

        long long step = 1;
        if (g->step)
        {
            Value stepVal = evaluate(g->step.get());
            stepVal = stepVal.deref();
            if (std::holds_alternative<long long>(stepVal.data))
            {
                step = std::get<long long>(stepVal.data);
            }
            else if (std::holds_alternative<double>(stepVal.data))
            {
                step = static_cast<long long>(std::get<double>(stepVal.data));
            }
            else
            {
                throw std::runtime_error("Range ka step number hona chahiye, ye kya de diya: " + stepVal.typeName());
            }

            if (step == 0)
            {
                throw std::runtime_error("Chakkar aa jayega! 'ghuma' range ka step 0 nahi ho sakta!");
            }
        }
        else
        {
            step = (start <= end) ? 1 : -1;
        }

        auto loopEnv = std::make_shared<Environment>(environment, currentScopeDepth + 1);

        for (long long i = start; (step > 0 ? (i <= end) : (i >= end)); i += step)
        {
            loopEnv->define(g->varName, Value(i));
            auto prev = environment;
            environment = loopEnv;
            try
            {
                execute(g->body.get());
            }
            catch (const BreakSignal &)
            {
                environment = prev;
                break;
            }
            catch (const ContinueSignal &)
            {
                environment = prev;
                continue;
            }
            environment = prev;
        }
        return;
    }

    // 7b. For-Each statement: `ghuma item mein collection { ... }`
    if (auto *gm = dynamic_cast<const GhumaMeinStmt *>(stmt))
    {
        Value colVal = evaluate(gm->collection.get()).deref();

        if (colVal.isList())
        {
            auto list = colVal.getList();
            size_t n = list ? list->elements.size() : 0;
            for (size_t i = 0; i < n; ++i)
            {
                auto loopEnv = std::make_shared<Environment>(environment, currentScopeDepth + 1);
                loopEnv->define(gm->varName, list->elements[i]);
                auto prev = environment;
                environment = loopEnv;
                try
                {
                    execute(gm->body.get());
                }
                catch (const BreakSignal &)
                {
                    environment = prev;
                    break;
                }
                catch (const ContinueSignal &)
                {
                    environment = prev;
                    continue;
                }
                environment = prev;
            }
            return;
        }

        if (colVal.isMap())
        {
            auto map = colVal.getMap();
            std::vector<std::string> keys = map ? map->keyOrder : std::vector<std::string>{};
            for (const auto &k : keys)
            {
                auto loopEnv = std::make_shared<Environment>(environment, currentScopeDepth + 1);
                loopEnv->define(gm->varName, Value(k));
                auto prev = environment;
                environment = loopEnv;
                try
                {
                    execute(gm->body.get());
                }
                catch (const BreakSignal &)
                {
                    environment = prev;
                    break;
                }
                catch (const ContinueSignal &)
                {
                    environment = prev;
                    continue;
                }
                environment = prev;
            }
            return;
        }

        if (std::holds_alternative<std::string>(colVal.data))
        {
            const std::string &str = std::get<std::string>(colVal.data);
            for (char ch : str)
            {
                auto loopEnv = std::make_shared<Environment>(environment, currentScopeDepth + 1);
                loopEnv->define(gm->varName, Value(std::string(1, ch)));
                auto prev = environment;
                environment = loopEnv;
                try
                {
                    execute(gm->body.get());
                }
                catch (const BreakSignal &)
                {
                    environment = prev;
                    break;
                }
                catch (const ContinueSignal &)
                {
                    environment = prev;
                    continue;
                }
                environment = prev;
            }
            return;
        }

        throw std::runtime_error("Ghumaane ke liye list, khata ya string chahiye! '" + colVal.typeName() + "' ko kaise ghumaoge?");
    }

    // 8. Return statement: `ye_le expr;` or `ye_le;`
    if (auto *r = dynamic_cast<const ReturnStmt *>(stmt))
    {
        Value val = Value(std::monostate{});
        if (r->value)
        {
            if (auto *ident = dynamic_cast<const IdentifierExpr *>(r->value.get()))
            {
                val = moveVariable(ident->name);
            }
            else
            {
                val = evaluate(r->value.get());
            }
        }

        // Prevent returning reference to local variable
        if (val.isReference())
        {
            const auto &ref = val.getReference();
            if (ref.scopeDepth >= currentScopeDepth)
            {
                throw std::runtime_error(
                    "Baburao ka chashma! Local variable '" + ref.varName +
                    "' ka udhaar wapas nahi bhej sakte, scope khatam hote hi mar jayega!");
            }
        }

        throw ReturnSignal(val);
    }

    // 9. Break statement: `bas_kar;`
    if (dynamic_cast<const BreakStmt *>(stmt))
    {
        throw BreakSignal();
    }

    // 10. Continue statement: `aage_badh;`
    if (dynamic_cast<const ContinueStmt *>(stmt))
    {
        throw ContinueSignal();
    }

    // 11. Function declaration: `jugaad fname(a, b) { ... }`
    if (auto *f = dynamic_cast<const FuncDeclStmt *>(stmt))
    {
        auto func = std::make_shared<UserFunction>(f, environment);
        environment->define(f->name, Value(func));
        return;
    }

    // 12. Class declaration: `biradari Name [nepo SuperName] { ... }`
    if (auto *c = dynamic_cast<const ClassDeclStmt *>(stmt))
    {
        std::shared_ptr<ClassCallable> superclass = nullptr;
        if (!c->superclass.empty())
        {
            Value superVal = environment->get(c->superclass);
            superVal = superVal.deref();
            if (!std::holds_alternative<std::shared_ptr<Callable>>(superVal.data))
            {
                throw std::runtime_error("Fake nepotism! '" + c->superclass + "' koi biradari (class) nahi hai, isko inherit kaise karoge?");
            }
            auto superCallable = std::get<std::shared_ptr<Callable>>(superVal.data);
            superclass = std::dynamic_pointer_cast<ClassCallable>(superCallable);
            if (!superclass)
            {
                throw std::runtime_error("Fake nepotism! '" + c->superclass + "' koi biradari (class) nahi hai!");
            }
        }

        std::unordered_map<std::string, std::shared_ptr<UserFunction>> methods;
        for (const auto &method : c->methods)
        {
            auto func = std::make_shared<UserFunction>(method.get(), environment);
            methods[method->name] = func;
        }

        auto klass = std::make_shared<ClassCallable>(c->name, superclass, std::move(methods));
        environment->define(c->name, Value(klass));
        return;
    }

    // 13. Throw statement: `chillao expr;`
    if (auto *th = dynamic_cast<const ThrowStmt *>(stmt))
    {
        Value val = evaluate(th->expression.get());
        int line = (th->line > 0) ? th->line : currentLine;
        throw MemeUserException(val, line);
    }

    // 14. Try-Catch-Finally statement: `panga { ... } lafda err { ... } [pakka { ... }]`
    if (auto *tc = dynamic_cast<const TryCatchStmt *>(stmt))
    {
        auto runFinally = [&]() {
            if (tc->finallyBlock)
            {
                execute(tc->finallyBlock.get());
            }
        };

        try
        {
            execute(tc->tryBlock.get());
        }
        catch (const ReturnSignal &)
        {
            runFinally();
            throw;
        }
        catch (const BreakSignal &)
        {
            runFinally();
            throw;
        }
        catch (const ContinueSignal &)
        {
            runFinally();
            throw;
        }
        catch (const MemeUserException &e)
        {
            if (!tc->catchBlock)
            {
                runFinally();
                throw;
            }
            auto catchEnv = std::make_shared<Environment>(environment, currentScopeDepth + 1);
            if (!tc->catchVar.empty())
            {
                Value errVal;
                Value d = e.value.deref();
                if (d.isMap() && d.getMap()->has("__is_error__"))
                {
                    errVal = d;
                }
                else if (d.isMap())
                {
                    auto m = d.getMap();
                    m->set("__is_error__", Value(true));
                    if (!m->has("baat")) m->set("baat", Value(m->has("message") ? m->entries["message"].deref().toString() : "Error"));
                    if (!m->has("message")) m->set("message", m->entries["baat"]);
                    if (!m->has("kism")) m->set("kism", Value(m->has("type") ? m->entries["type"].deref().toString() : "UserLafda"));
                    if (!m->has("type")) m->set("type", m->entries["kism"]);
                    if (!m->has("line")) m->set("line", Value(static_cast<long long>(e.line > 0 ? e.line : currentLine)));
                    errVal = Value(m);
                }
                else
                {
                    std::string msg = d.toString();
                    errVal = Value(createErrorMap(msg, "UserLafda", e.line > 0 ? e.line : currentLine));
                }
                catchEnv->define(tc->catchVar, errVal);
            }
            if (auto *cb = dynamic_cast<const BlockStmt *>(tc->catchBlock.get()))
            {
                try
                {
                    executeBlock(cb->statements, catchEnv);
                }
                catch (...)
                {
                    runFinally();
                    throw;
                }
            }
            else
            {
                auto prev = environment;
                environment = catchEnv;
                try
                {
                    execute(tc->catchBlock.get());
                    environment = prev;
                }
                catch (...)
                {
                    environment = prev;
                    runFinally();
                    throw;
                }
            }
        }
        catch (const std::exception &e)
        {
            if (!tc->catchBlock)
            {
                runFinally();
                throw;
            }
            auto catchEnv = std::make_shared<Environment>(environment, currentScopeDepth + 1);
            if (!tc->catchVar.empty())
            {
                std::string msg = e.what();
                std::string cat = categorizeErrorMessage(msg);
                catchEnv->define(tc->catchVar, Value(createErrorMap(msg, cat, currentLine)));
            }
            if (auto *cb = dynamic_cast<const BlockStmt *>(tc->catchBlock.get()))
            {
                try
                {
                    executeBlock(cb->statements, catchEnv);
                }
                catch (...)
                {
                    runFinally();
                    throw;
                }
            }
            else
            {
                auto prev = environment;
                environment = catchEnv;
                try
                {
                    execute(tc->catchBlock.get());
                    environment = prev;
                }
                catch (...)
                {
                    environment = prev;
                    runFinally();
                    throw;
                }
            }
        }

        runFinally();
        return;
    }

    // 15. Import statement: `bula "path" [jaise alias | jaise { a, b, c }];`
    if (auto *im = dynamic_cast<const ImportStmt *>(stmt))
    {
        Value modVal = importModule(im->path, im->line);
        if (!im->namedImports.empty())
        {
            for (const auto &name : im->namedImports)
            {
                Value val;
                if (modVal.isMap())
                {
                    auto map = modVal.getMap();
                    if (map->has(name))
                    {
                        val = map->entries[name];
                    }
                }
                else if (modVal.isInstance())
                {
                    val = modVal.getInstance()->get(name);
                }
                environment->define(name, val);
            }
        }
        else
        {
            std::string alias = im->alias;
            if (alias.empty())
            {
                std::filesystem::path p(im->path);
                alias = p.stem().string();
                if (alias.empty()) alias = "mod";
            }
            environment->define(alias, modVal);
        }
        return;
    }

    // 16. Enum declaration: `avtar Status { Pehla, Dusra [= val] }`
    if (auto *ed = dynamic_cast<const EnumDeclStmt *>(stmt))
    {
        auto enumMap = std::make_shared<MemeMap>();
        long long nextVal = 0;
        for (const auto &member : ed->members)
        {
            if (member.value)
            {
                Value val = evaluate(member.value.get()).deref();
                if (std::holds_alternative<long long>(val.data))
                {
                    nextVal = std::get<long long>(val.data) + 1;
                }
                enumMap->set(member.name, std::move(val));
            }
            else
            {
                enumMap->set(member.name, Value(nextVal++));
            }
        }
        environment->define(ed->name, Value(enumMap));
        return;
    }

    // 17. Pattern match: `jaanch expr { ... }`
    if (auto *j = dynamic_cast<const JaanchStmt *>(stmt))
    {
        Value targetVal = evaluate(j->target.get()).deref();
        bool matched = false;

        for (const auto &arm : j->arms)
        {
            Value patternVal = evaluate(arm.pattern.get()).deref();
            if (targetVal == patternVal)
            {
                execute(arm.body.get());
                matched = true;
                break;
            }
        }

        if (!matched && j->defaultArm)
        {
            execute(j->defaultArm.get());
        }
        return;
    }

    throw std::runtime_error("Ye kaisa anokha statement hai? Syntax samajh nahi aaya!");
    }
    catch (const BreakSignal &) { throw; }
    catch (const ContinueSignal &) { throw; }
    catch (const ReturnSignal &) { throw; }
    catch (const MemeUserException &) { throw; }
    catch (const std::runtime_error &e)
    {
        std::string msg = e.what();
        if (msg.rfind("Line ", 0) == 0)
        {
            throw;
        }
        int line = (stmt->line > 0) ? stmt->line : currentLine;
        throw std::runtime_error("Line " + std::to_string(line) + ": " + msg);
    }
}

Value Interpreter::evaluate(const Expr *expr)
{
    if (!expr)
        return Value(std::monostate{});

    if (expr->line > 0)
    {
        currentLine = expr->line;
    }

    // Literals
    if (auto *e = dynamic_cast<const IntExpr *>(expr))
        return Value(e->value);

    if (auto *e = dynamic_cast<const FloatExpr *>(expr))
        return Value(e->value);

    if (auto *e = dynamic_cast<const StringExpr *>(expr))
        return Value(e->value);

    // F-String expression: `f"Hello {name}"`
    if (auto *fs = dynamic_cast<const FStringExpr *>(expr))
    {
        std::string result = "";
        for (const auto &part : fs->parts)
        {
            Value val = evaluate(part.get());
            result += stringify(val);
        }
        return Value(result);
    }

    if (auto *e = dynamic_cast<const BoolExpr *>(expr))
        return Value(e->value);

    if (dynamic_cast<const NullExpr *>(expr))
        return Value(std::monostate{});

    // Lambda expression: `(a, b) => expr` or `jugaad(x) { ... }`
    if (auto *l = dynamic_cast<const LambdaExpr *>(expr))
    {
        auto function = std::make_shared<UserFunction>(l->declaration, environment);
        return Value(function);
    }

    // Identifier lookup
    if (auto *e = dynamic_cast<const IdentifierExpr *>(expr))
        return environment->get(e->name);

    // Assignment: `x = value`
    if (auto *e = dynamic_cast<const AssignExpr *>(expr))
    {
        VariableSlot *slot = environment->findSlot(e->name);
        Value val;
        if (auto *ident = dynamic_cast<const IdentifierExpr *>(e->value.get()))
        {
            val = moveVariable(ident->name);
        }
        else
        {
            val = evaluate(e->value.get());
        }

        if (!slot)
        {
            // Python-style auto variable creation
            environment->define(e->name, val);
            return val;
        }

        // Prevent assigning local reference to outer scope variable (lifetime escape)
        if (val.isReference())
        {
            const auto &ref = val.getReference();
            if (ref.scopeDepth > slot->depth)
            {
                throw std::runtime_error(
                    "Baburao ka chashma! Local variable '" + ref.varName +
                    "' ka udhaar bahar nahi bhej sakte, scope khatam hote hi mar jayega!");
            }
        }

        environment->assign(e->name, val);
        return val;
    }

    // Array Destructure Assignment: `[a, b, *rest] = value` or `[count, setCount] = useState(0)`
    if (auto *da = dynamic_cast<const DestructureAssignExpr *>(expr))
    {
        Value val = evaluate(da->value.get()).deref();
        if (!val.isList())
        {
            throw std::runtime_error("Destructuring sirf Array / list pe ho sakti hai! Mila: " + val.typeName());
        }
        auto list = val.getList();
        size_t listIdx = 0;
        size_t total = list ? list->elements.size() : 0;
        for (const auto &item : da->items)
        {
            Value elemVal;
            if (item.isVariadic)
            {
                auto restList = std::make_shared<MemeList>();
                while (listIdx < total)
                {
                    restList->elements.push_back(list->elements[listIdx++]);
                }
                elemVal = Value(restList);
            }
            else
            {
                elemVal = (listIdx < total) ? list->elements[listIdx++] : Value(std::monostate{});
            }

            VariableSlot *slot = environment->findSlot(item.name);
            if (slot)
            {
                environment->assign(item.name, elemVal);
            }
            else
            {
                environment->define(item.name, elemVal);
            }
        }
        return val;
    }

    // Borrow expression: `udhaar x` or `udhaar_badal x`
    if (auto *b = dynamic_cast<const BorrowExpr *>(expr))
    {
        auto *ident = dynamic_cast<const IdentifierExpr *>(b->operand.get());
        if (!ident)
        {
            throw std::runtime_error(
                "Ye sab doglapan hai! Sirf variable ko udhaar (borrow) kiya ja sakta hai!");
        }

        VariableSlot *slot = environment->findSlot(ident->name);
        if (!slot)
        {
            throw std::runtime_error(
                "Rasode mein kaun tha?! Ye '" + ident->name + "' bina bataye kahan se aaya? Pehle 'pakad " + ident->name + ";' se declare kar!");
        }

        if (slot->isMoved)
        {
            throw std::runtime_error(
                "Pehli fursat mein nikal! '" + ident->name + "' ki value move ho chuki hai! Ownership transfer ke baad use karna mana hai!");
        }

        if (b->isMutable)
        {
            if (slot->hasExclusiveBorrow)
            {
                throw std::runtime_error(
                    "Ek mawaali ek time pe ek hi jagah! '" + ident->name + "' already exclusively borrowed hai!");
            }
            if (slot->sharedBorrows > 0)
            {
                throw std::runtime_error(
                    "Ek mawaali ek time pe ek hi jagah! '" + ident->name + "' already udhaar (borrowed) pe hai, exclusive borrow nahi mil sakta!");
            }

            slot->hasExclusiveBorrow = true;
        }
        else
        {
            if (slot->hasExclusiveBorrow)
            {
                throw std::runtime_error(
                    "Shanti gadadhari Bheem! '" + ident->name + "' already mutably borrowed hai!");
            }

            slot->sharedBorrows++;
        }

        uint64_t bId = nextBorrowId++;
        activeBorrows.push_back({bId, environment, ident->name, b->isMutable, currentScopeDepth});

        Reference ref;
        ref.varName = ident->name;
        ref.env = environment;
        ref.isMutable = b->isMutable;
        ref.scopeDepth = currentScopeDepth;
        ref.borrowId = bId;

        return Value(ref);
    }

    // Nakal (clone) expression: `nakal(expr)` or `nakal expr`
    if (auto *n = dynamic_cast<const NakalExpr *>(expr))
    {
        Value val = evaluate(n->operand.get());
        return cloneValue(val);
    }

    // Await expression: `sabar expr` or `await expr`
    if (auto *a = dynamic_cast<const AwaitExpr *>(expr))
    {
        Value operand = evaluate(a->expression.get());
        Value d = operand.deref();
        if (d.isMap())
        {
            auto map = d.getMap();
            if (map->has("__is_promise__") || map->has("__is_thread__"))
            {
                if (map->has("jod"))
                {
                    Value jodVal = map->entries["jod"].deref();
                    if (jodVal.isCallable())
                    {
                        return jodVal.getCallable()->call(*this, {});
                    }
                }
            }
        }
        return operand;
    }

    // Unary expression: `-x`, `!x`
    if (auto *e = dynamic_cast<const UnaryExpr *>(expr))
    {
        Value operand = evaluate(e->operand.get());
        if (e->op.type == TokenType::MINUS)
            return negate(operand);
        if (e->op.type == TokenType::NOT)
            return logicalNot(operand);

        throw std::runtime_error("Ye operator '" + e->op.value + "' unary mein nahi chalta!");
    }

    // Binary expression
    if (auto *e = dynamic_cast<const BinaryExpr *>(expr))
    {
        // Short-circuiting logical AND: `aur`
        if (e->op.type == TokenType::AUR)
        {
            Value left = evaluate(e->left.get());
            if (!left.isTruthy())
                return left;
            return evaluate(e->right.get());
        }

        // Short-circuiting logical OR: `ya`
        if (e->op.type == TokenType::YA)
        {
            Value left = evaluate(e->left.get());
            if (left.isTruthy())
                return left;
            return evaluate(e->right.get());
        }

        Value left = evaluate(e->left.get());
        Value right = evaluate(e->right.get());

        Value dl = left.deref();
        Value dr = right.deref();

        if (e->op.type == TokenType::PLUS)
        {
            if (dl.isInstance())
            {
                auto method = dl.getInstance()->getClass()->findMethod("__jod__");
                if (!method) method = dl.getInstance()->getClass()->findMethod("__add__");
                if (method)
                {
                    BoundMethod bound(dl.getInstance(), method);
                    return bound.call(*this, {dr});
                }
            }
            if (std::holds_alternative<std::string>(dl.data) || std::holds_alternative<std::string>(dr.data))
            {
                return Value(stringify(dl) + stringify(dr));
            }
            return add(dl, dr);
        }
        if (e->op.type == TokenType::MINUS)
        {
            if (dl.isInstance())
            {
                auto method = dl.getInstance()->getClass()->findMethod("__ghata__");
                if (!method) method = dl.getInstance()->getClass()->findMethod("__sub__");
                if (method)
                {
                    BoundMethod bound(dl.getInstance(), method);
                    return bound.call(*this, {dr});
                }
            }
            return subtract(dl, dr);
        }
        if (e->op.type == TokenType::MULTIPLY)
        {
            if (dl.isInstance())
            {
                auto method = dl.getInstance()->getClass()->findMethod("__guna__");
                if (!method) method = dl.getInstance()->getClass()->findMethod("__mul__");
                if (method)
                {
                    BoundMethod bound(dl.getInstance(), method);
                    return bound.call(*this, {dr});
                }
            }
            return multiply(dl, dr);
        }
        if (e->op.type == TokenType::DIVIDE)
        {
            if (dl.isInstance())
            {
                auto method = dl.getInstance()->getClass()->findMethod("__bhaag__");
                if (!method) method = dl.getInstance()->getClass()->findMethod("__truediv__");
                if (!method) method = dl.getInstance()->getClass()->findMethod("__div__");
                if (method)
                {
                    BoundMethod bound(dl.getInstance(), method);
                    return bound.call(*this, {dr});
                }
            }
            return divide(dl, dr);
        }
        if (e->op.type == TokenType::MOD)
        {
            if (dl.isInstance())
            {
                auto method = dl.getInstance()->getClass()->findMethod("__bacha__");
                if (!method) method = dl.getInstance()->getClass()->findMethod("__mod__");
                if (method)
                {
                    BoundMethod bound(dl.getInstance(), method);
                    return bound.call(*this, {dr});
                }
            }
            return modulo(dl, dr);
        }
        if (e->op.type == TokenType::EQUAL)
        {
            if (dl.isInstance())
            {
                auto method = dl.getInstance()->getClass()->findMethod("__barabar__");
                if (!method) method = dl.getInstance()->getClass()->findMethod("__eq__");
                if (method)
                {
                    BoundMethod bound(dl.getInstance(), method);
                    return bound.call(*this, {dr});
                }
            }
            if (dr.isInstance())
            {
                auto method = dr.getInstance()->getClass()->findMethod("__barabar__");
                if (!method) method = dr.getInstance()->getClass()->findMethod("__eq__");
                if (method)
                {
                    BoundMethod bound(dr.getInstance(), method);
                    return bound.call(*this, {dl});
                }
            }
            return equal(dl, dr);
        }
        if (e->op.type == TokenType::NOT_EQUAL)
        {
            if (dl.isInstance())
            {
                auto method = dl.getInstance()->getClass()->findMethod("__barabar__");
                if (!method) method = dl.getInstance()->getClass()->findMethod("__eq__");
                if (method)
                {
                    BoundMethod bound(dl.getInstance(), method);
                    Value res = bound.call(*this, {dr});
                    return Value(!res.isTruthy());
                }
                auto neMethod = dl.getInstance()->getClass()->findMethod("__ne__");
                if (neMethod)
                {
                    BoundMethod bound(dl.getInstance(), neMethod);
                    return bound.call(*this, {dr});
                }
            }
            if (dr.isInstance())
            {
                auto method = dr.getInstance()->getClass()->findMethod("__barabar__");
                if (!method) method = dr.getInstance()->getClass()->findMethod("__eq__");
                if (method)
                {
                    BoundMethod bound(dr.getInstance(), method);
                    Value res = bound.call(*this, {dl});
                    return Value(!res.isTruthy());
                }
            }
            return notEqual(dl, dr);
        }
        if (e->op.type == TokenType::LESS_THAN)
        {
            if (dl.isInstance())
            {
                auto method = dl.getInstance()->getClass()->findMethod("__chota__");
                if (!method) method = dl.getInstance()->getClass()->findMethod("__lt__");
                if (method)
                {
                    BoundMethod bound(dl.getInstance(), method);
                    return bound.call(*this, {dr});
                }
            }
            return lessThan(dl, dr);
        }
        if (e->op.type == TokenType::GREATER_THAN)
        {
            if (dl.isInstance())
            {
                auto badaMethod = dl.getInstance()->getClass()->findMethod("__bada__");
                if (!badaMethod) badaMethod = dl.getInstance()->getClass()->findMethod("__gt__");
                if (badaMethod)
                {
                    BoundMethod bound(dl.getInstance(), badaMethod);
                    return bound.call(*this, {dr});
                }
            }
            if (dr.isInstance())
            {
                auto chotaMethod = dr.getInstance()->getClass()->findMethod("__chota__");
                if (!chotaMethod) chotaMethod = dr.getInstance()->getClass()->findMethod("__lt__");
                if (chotaMethod)
                {
                    BoundMethod bound(dr.getInstance(), chotaMethod);
                    return bound.call(*this, {dl});
                }
            }
            return greaterThan(dl, dr);
        }
        if (e->op.type == TokenType::LESS_THAN_EQUAL)
        {
            if (dl.isInstance())
            {
                auto leMethod = dl.getInstance()->getClass()->findMethod("__le__");
                if (leMethod)
                {
                    BoundMethod bound(dl.getInstance(), leMethod);
                    return bound.call(*this, {dr});
                }
            }
            if (dr.isInstance())
            {
                auto chotaMethod = dr.getInstance()->getClass()->findMethod("__chota__");
                if (!chotaMethod) chotaMethod = dr.getInstance()->getClass()->findMethod("__lt__");
                if (chotaMethod)
                {
                    BoundMethod bound(dr.getInstance(), chotaMethod);
                    Value res = bound.call(*this, {dl});
                    return Value(!res.isTruthy());
                }
            }
            return lessThanEqual(dl, dr);
        }
        if (e->op.type == TokenType::GREATER_THAN_EQUAL)
        {
            if (dl.isInstance())
            {
                auto geMethod = dl.getInstance()->getClass()->findMethod("__ge__");
                if (geMethod)
                {
                    BoundMethod bound(dl.getInstance(), geMethod);
                    return bound.call(*this, {dr});
                }
                auto chotaMethod = dl.getInstance()->getClass()->findMethod("__chota__");
                if (!chotaMethod) chotaMethod = dl.getInstance()->getClass()->findMethod("__lt__");
                if (chotaMethod)
                {
                    BoundMethod bound(dl.getInstance(), chotaMethod);
                    Value res = bound.call(*this, {dr});
                    return Value(!res.isTruthy());
                }
            }
            return greaterThanEqual(dl, dr);
        }

        throw std::runtime_error("Ye kaisa operator ghusa diya: " + e->op.value);
    }

    // Function call: `f(a, b)`
    if (auto *e = dynamic_cast<const CallExpr *>(expr))
    {
        Value callee = evaluate(e->callee.get());
        callee = callee.deref();

        std::shared_ptr<Callable> function = nullptr;
        if (std::holds_alternative<std::shared_ptr<Callable>>(callee.data))
        {
            function = std::get<std::shared_ptr<Callable>>(callee.data);
        }
        else if (std::holds_alternative<long long>(callee.data) && e->args.empty())
        {
            auto *getExpr = dynamic_cast<const GetExpr *>(e->callee.get());
            if (getExpr && (getExpr->name == "length" || getExpr->name == "size" || getExpr->name == "lambai" || getExpr->name == "lambaai" || getExpr->name == "count" || getExpr->name == "kitna_lamba"))
            {
                return callee;
            }
        }
        else if (callee.isInstance())
        {
            auto inst = callee.getInstance();
            if (inst && inst->getClass())
            {
                auto method = inst->getClass()->findMethod("__chala__");
                if (!method)
                {
                    method = inst->getClass()->findMethod("__call__");
                }
                if (method)
                {
                    function = std::make_shared<BoundMethod>(inst, method);
                }
            }
        }

        if (!function)
        {
            throw std::runtime_error("Beta, tumse na ho payega! '" + callee.toString() + "' koi jugaad (function) thodi hai jo call kar rahe ho!");
        }

        bool isUserFunc = (dynamic_cast<UserFunction *>(function.get()) != nullptr) ||
                          (dynamic_cast<BoundMethod *>(function.get()) != nullptr);

        std::vector<Value> arguments;
        for (const auto &arg : e->args)
        {
            if (auto *sp = dynamic_cast<const SpreadExpr *>(arg.get()))
            {
                Value val = evaluate(sp->expr.get());
                val = val.deref();
                if (!val.isList())
                {
                    throw std::runtime_error("Doglapan! Spread ('*') sirf list / array pe chal sakta hai! Mila: " + val.typeName());
                }
                auto list = val.getList();
                for (const auto &elem : list->elements)
                {
                    arguments.push_back(elem);
                }
                continue;
            }

            if (isUserFunc)
            {
                if (auto *ident = dynamic_cast<const IdentifierExpr *>(arg.get()))
                {
                    arguments.push_back(moveVariable(ident->name));
                    continue;
                }
            }
            arguments.push_back(evaluate(arg.get()));
        }

        if (!function->checkArity(arguments.size()))
        {
            int line = (e->line > 0) ? e->line : currentLine;
            throw std::runtime_error(
                "Line " + std::to_string(line) + ": Gaddari korbe! Jugaad ko kam se kam " +
                std::to_string(function->minArity()) + " arguments chahiye the, par tumne " +
                std::to_string(arguments.size()) + " pakda diye!");
        }

        return function->call(*this, arguments);
    }

    // Input: `bata()` or `bata("Prompt: ")`
    if (auto *e = dynamic_cast<const PoochExpr *>(expr))
    {
        std::string promptStr = "";
        if (e->prompt)
        {
            Value p = evaluate(e->prompt.get());
            promptStr = p.toString();
            out << promptStr;
            out.flush();
        }

        std::string line;
        if (std::getline(in, line))
        {
            return Value(line);
        }

        if (inputPromptCallback)
        {
            std::string inputVal = inputPromptCallback(promptStr);
            out << inputVal << "\n";
            out.flush();
            return Value(inputVal);
        }

        return Value("");
    }

    // Property get: `object.property`
    if (auto *g = dynamic_cast<const GetExpr *>(expr))
    {
        Value objVal = evaluate(g->object.get());
        objVal = objVal.deref();

        if (objVal.isInstance())
        {
            auto instance = objVal.getInstance();
            if (!instance)
            {
                throw std::runtime_error("Object toh pura 'nalla' (null) hai! Nalle se kya property maang rahe ho?");
            }
            if (g->name == "chala_de")
            {
                if (instance->hasField("chala_de") || (instance->getClass() && instance->getClass()->findMethod("chala_de")))
                {
                    return instance->get(g->name);
                }
                return Value(std::make_shared<NativeMethodCallable>("chala_de", 1, -1, [instance](Interpreter &interp, const std::vector<Value> &args) {
                    if (args.empty())
                    {
                        throw std::runtime_error("'chala_de' ko method ka naam chahiye!");
                    }
                    std::string methodName = args[0].deref().toString();
                    std::vector<Value> callArgs;
                    if (args.size() > 1 && args[1].deref().isList())
                    {
                        callArgs = args[1].deref().getList()->elements;
                    }
                    else
                    {
                        for (size_t i = 1; i < args.size(); ++i)
                        {
                            callArgs.push_back(args[i].deref());
                        }
                    }
                    auto targetMethod = instance->getClass() ? instance->getClass()->findMethod(methodName) : nullptr;
                    if (!targetMethod)
                    {
                        std::string clsName = instance->getClass() ? instance->getClass()->getName() : "unknown";
                        throw std::runtime_error("Khopdi tod saale ka! '" + clsName + "' biradari ke paas '" + methodName + "' naam ka koi method nahi mila!");
                    }
                    BoundMethod bound(instance, targetMethod);
                    return bound.call(interp, callArgs);
                }));
            }
            return instance->get(g->name);
        }

        if (objVal.isList())
        {
            if (g->name == "length" || g->name == "lambaai" || g->name == "lambai" || g->name == "size" || g->name == "count")
            {
                return Value(static_cast<long long>(objVal.getList()->elements.size()));
            }
            return getListMethod(objVal.getList(), g->name);
        }

        if (objVal.isMap())
        {
            auto mapPtr = objVal.getMap();
            if (mapPtr->has(g->name))
            {
                return mapPtr->entries[g->name];
            }
            if (mapPtr->has("__is_error__"))
            {
                if ((g->name == "message" || g->name == "baat") && (mapPtr->has("baat") || mapPtr->has("message")))
                    return mapPtr->has("baat") ? mapPtr->entries["baat"] : mapPtr->entries["message"];
                if ((g->name == "type" || g->name == "kism") && (mapPtr->has("kism") || mapPtr->has("type")))
                    return mapPtr->has("kism") ? mapPtr->entries["kism"] : mapPtr->entries["type"];
                if (g->name == "line" && mapPtr->has("line"))
                    return mapPtr->entries["line"];
            }
            if (g->name == "length" || g->name == "lambaai" || g->name == "lambai" || g->name == "size" || g->name == "count")
            {
                return Value(static_cast<long long>(mapPtr->entries.size()));
            }
            return getMapMethod(mapPtr, g->name);
        }

        if (std::holds_alternative<std::string>(objVal.data))
        {
            const std::string &strVal = std::get<std::string>(objVal.data);
            if (g->name == "length" || g->name == "lambaai" || g->name == "lambai" || g->name == "size" || g->name == "count")
            {
                return Value(static_cast<long long>(strVal.size()));
            }
            return getStringMethod(strVal, g->name);
        }

        throw std::runtime_error(
            "Aura -10000! '" + objVal.toString() + "' (" + objVal.typeName() +
            ") koi object, list ya khata thodi hai jo uski property nikaal rahe ho!");
    }

    // Ternary expression: `cond ? then : else`
    if (auto *t = dynamic_cast<const TernaryExpr *>(expr))
    {
        Value condVal = evaluate(t->condition.get()).deref();
        if (condVal.isTruthy())
        {
            return evaluate(t->thenBranch.get());
        }
        else
        {
            return evaluate(t->elseBranch.get());
        }
    }

    // Spread expression: `...x`
    if (auto *sp = dynamic_cast<const SpreadExpr *>(expr))
    {
        return evaluate(sp->expr.get());
    }

    // Array literal: `[a, b, c]`
    if (auto *al = dynamic_cast<const ArrayLiteralExpr *>(expr))
    {
        std::vector<Value> elems;
        for (const auto &elemExpr : al->elements)
        {
            if (auto *sp = dynamic_cast<const SpreadExpr *>(elemExpr.get()))
            {
                Value spVal = evaluate(sp->expr.get()).deref();
                if (spVal.isList())
                {
                    for (const auto &item : spVal.getList()->elements)
                    {
                        elems.push_back(item);
                    }
                }
                else if (std::holds_alternative<std::string>(spVal.data))
                {
                    for (char ch : std::get<std::string>(spVal.data))
                    {
                        elems.push_back(Value(std::string(1, ch)));
                    }
                }
                else
                {
                    elems.push_back(spVal);
                }
            }
            else
            {
                elems.push_back(evaluate(elemExpr.get()));
            }
        }
        return Value(std::make_shared<MemeList>(std::move(elems)));
    }

    // Map literal: `{ k1: v1, k2: v2 }`
    if (auto *ml = dynamic_cast<const MapLiteralExpr *>(expr))
    {
        auto map = std::make_shared<MemeMap>();
        for (const auto &entry : ml->entries)
        {
            if (auto *sp = dynamic_cast<const SpreadExpr *>(entry.first.get()))
            {
                Value spVal = evaluate(sp->expr.get()).deref();
                if (spVal.isMap())
                {
                    for (const auto &[k, v] : spVal.getMap()->entries)
                    {
                        map->set(k, v);
                    }
                }
            }
            else
            {
                std::string key = evaluate(entry.first.get()).deref().toString();
                Value val = evaluate(entry.second.get());
                map->set(key, std::move(val));
            }
        }
        return Value(map);
    }

    // JSX Element: `<tag prop="val">...</tag>` or `<Component ... />`
    if (auto *jsx = dynamic_cast<const JsxElementExpr *>(expr))
    {
        // Evaluate props into a MemeMap, supporting `{...spread}`
        auto propsMap = std::make_shared<MemeMap>();
        for (const auto &[key, valExpr] : jsx->props)
        {
            if (key == "...")
            {
                Value spVal = evaluate(valExpr.get()).deref();
                if (spVal.isMap())
                {
                    auto sm = spVal.getMap();
                    for (const auto &[k, v] : sm->entries)
                    {
                        propsMap->set(k, v);
                        if (k == "className" && !propsMap->has("class"))
                        {
                            propsMap->set("class", v);
                        }
                    }
                }
            }
            else
            {
                propsMap->set(key, evaluate(valExpr.get()));
                if (key == "className" && !propsMap->has("class"))
                {
                    propsMap->set("class", propsMap->entries["className"]);
                }
            }
        }

        // Check if this is a Context Provider: `<ThemeCtx.Provider value={...}>`
        bool isContextProvider = false;
        std::string ctxId = "";
        Value provideVal = Value(std::monostate{});
        size_t dotPos = jsx->tag.find('.');
        if (dotPos != std::string::npos && jsx->tag.substr(dotPos + 1) == "Provider")
        {
            std::string ctxObjName = jsx->tag.substr(0, dotPos);
            VariableSlot *slot = environment->findSlot(ctxObjName);
            if (!slot) slot = globals->findSlot(ctxObjName);
            if (slot && slot->value.deref().isMap())
            {
                auto cm = slot->value.deref().getMap();
                ctxId = cm->has("id") ? cm->entries["id"].toString() : ctxObjName;
                isContextProvider = true;
                if (propsMap->has("value")) provideVal = propsMap->entries["value"];
            }
        }

        if (isContextProvider)
        {
            pushContext(ctxId, provideVal);
        }

        // Evaluate children into a MemeList, ignoring true, false, and null
        auto childrenList = std::make_shared<MemeList>();
        auto shouldIgnoreChild = [](const Value &v) -> bool {
            Value d = v.deref();
            if (d.isNil()) return true;
            if (std::holds_alternative<bool>(d.data)) return true;
            return false;
        };

        for (const auto &childExpr : jsx->children)
        {
            Value childVal = evaluate(childExpr.get()).deref();
            if (childVal.isList())
            {
                // Flatten arrays like {prices.map(...)}
                auto list = childVal.getList();
                for (const auto &item : list->elements)
                {
                    if (!shouldIgnoreChild(item))
                    {
                        childrenList->elements.push_back(item);
                    }
                }
            }
            else if (!shouldIgnoreChild(childVal))
            {
                childrenList->elements.push_back(childVal);
            }
        }

        if (isContextProvider)
        {
            popContext(ctxId);
            auto node = std::make_shared<MemeMap>();
            node->set("type", Value("jsx_element"));
            node->set("tag", Value("fragment"));
            node->set("props", Value(propsMap));
            node->set("children", Value(childrenList));
            return Value(node);
        }

        // If Component (uppercase tag name or dotted name like MyComp.Sub)
        if (!jsx->isFragment && jsx->tag != "Fragment" && jsx->tag != "fragment" &&
            !jsx->tag.empty() && (std::isupper(static_cast<unsigned char>(jsx->tag[0])) || jsx->tag.find('.') != std::string::npos))
        {
            propsMap->set("children", Value(childrenList));
            Value compVal;
            if (dotPos != std::string::npos)
            {
                std::string baseName = jsx->tag.substr(0, dotPos);
                std::string member = jsx->tag.substr(dotPos + 1);
                VariableSlot *slot = environment->findSlot(baseName);
                if (!slot) slot = globals->findSlot(baseName);
                if (!slot) throw std::runtime_error("Component '<" + jsx->tag + ">' nahi mila!");
                Value baseVal = slot->value.deref();
                if (baseVal.isMap()) compVal = baseVal.getMap()->entries[member];
            }
            else
            {
                VariableSlot *slot = environment->findSlot(jsx->tag);
                if (!slot) slot = globals->findSlot(jsx->tag);
                if (!slot)
                {
                    // Check if any imported module in environment contains this component
                    for (const auto &[vname, vslot] : environment->getSlots())
                    {
                        Value sv = vslot.value.deref();
                        if (sv.isMap() && sv.getMap()->has(jsx->tag))
                        {
                            compVal = sv.getMap()->entries[jsx->tag];
                            break;
                        }
                    }
                    if (!std::holds_alternative<std::shared_ptr<Callable>>(compVal.data))
                    {
                        throw std::runtime_error("Component '<" + jsx->tag + ">' nahi mila! Pehle declare karo ya import karo.");
                    }
                }
                else
                {
                    compVal = slot->value.deref();
                }
            }

            if (compVal.isMap())
            {
                auto map = compVal.getMap();
                if (map->has(jsx->tag))
                {
                    compVal = map->entries[jsx->tag];
                }
                else if (map->has("default"))
                {
                    compVal = map->entries["default"];
                }
            }

            if (std::holds_alternative<std::shared_ptr<Callable>>(compVal.data))
            {
                auto callable = std::get<std::shared_ptr<Callable>>(compVal.data);
                pushComponentCall(jsx->tag);
                Value compRes;
                try {
                    compRes = callable->call(*this, {Value(propsMap)});
                    popComponentCall();
                } catch (...) {
                    popComponentCall();
                    throw;
                }
                return compRes;
            }
            throw std::runtime_error("'<" + jsx->tag + ">' koi component ya function nahi hai!");
        }

        // Lowercase tag or fragment (standard HTML element)
        auto node = std::make_shared<MemeMap>();
        node->set("type", Value("jsx_element"));
        node->set("tag", Value(jsx->tag.empty() || jsx->isFragment ? "fragment" : jsx->tag));
        node->set("props", Value(propsMap));
        node->set("children", Value(childrenList));
        if (propsMap->has("class"))
        {
            node->set("class", propsMap->entries["class"]);
        }
        else if (propsMap->has("className"))
        {
            node->set("class", propsMap->entries["className"]);
        }
        return Value(node);
    }

    // Index get: `target[index]`
    if (auto *ix = dynamic_cast<const IndexExpr *>(expr))
    {
        Value target = evaluate(ix->target.get()).deref();
        Value idxVal = evaluate(ix->index.get()).deref();

        if (target.isInstance())
        {
            auto inst = target.getInstance();
            if (inst && inst->getClass())
            {
                auto method = inst->getClass()->findMethod("__la__");
                if (!method) method = inst->getClass()->findMethod("__getitem__");
                if (method)
                {
                    BoundMethod bound(inst, method);
                    return bound.call(*this, {idxVal});
                }
            }
        }

        if (target.isList())
        {
            auto list = target.getList();
            long long sz = list ? static_cast<long long>(list->elements.size()) : 0;
            long long idx = 0;
            if (std::holds_alternative<long long>(idxVal.data))
                idx = std::get<long long>(idxVal.data);
            else if (std::holds_alternative<double>(idxVal.data))
                idx = static_cast<long long>(std::get<double>(idxVal.data));
            else
                throw std::runtime_error("Array index number hona chahiye, ye kya diya: " + idxVal.typeName());

            long long origIdx = idx;
            if (idx < 0)
                idx += sz;

            if (idx < 0 || idx >= sz)
            {
                throw std::runtime_error(
                    "Aukaat ke bahar! Index " + std::to_string(origIdx) +
                    " maang rahe ho par size sirf " + std::to_string(sz) + " hai!");
            }

            return list->elements[idx];
        }

        if (std::holds_alternative<std::string>(target.data))
        {
            const std::string &str = std::get<std::string>(target.data);
            long long sz = static_cast<long long>(str.size());
            long long idx = 0;
            if (std::holds_alternative<long long>(idxVal.data))
                idx = std::get<long long>(idxVal.data);
            else if (std::holds_alternative<double>(idxVal.data))
                idx = static_cast<long long>(std::get<double>(idxVal.data));
            else
                throw std::runtime_error("String index number hona chahiye, ye kya diya: " + idxVal.typeName());

            long long origIdx = idx;
            if (idx < 0)
                idx += sz;

            if (idx < 0 || idx >= sz)
            {
                throw std::runtime_error(
                    "Aukaat ke bahar! Index " + std::to_string(origIdx) +
                    " maang rahe ho par string length sirf " + std::to_string(sz) + " hai!");
            }

            return Value(std::string(1, str[idx]));
        }

        if (target.isMap())
        {
            auto map = target.getMap();
            std::string key = idxVal.toString();
            if (map && map->has(key))
            {
                return map->entries[key];
            }
            return Value(std::monostate{}); // returns nalla if key missing
        }

        throw std::runtime_error(
            "Ye kya bawasir hai? '" + target.typeName() + "' ke andar '[]' laga ke kya dhund rahe ho?");
    }

    // Index set: `target[index] = val`
    if (auto *ia = dynamic_cast<const IndexAssignExpr *>(expr))
    {
        Value target = evaluate(ia->target.get()).deref();
        Value idxVal = evaluate(ia->index.get()).deref();
        Value val = evaluate(ia->value.get());

        if (target.isInstance())
        {
            auto inst = target.getInstance();
            if (inst && inst->getClass())
            {
                auto method = inst->getClass()->findMethod("__daal__");
                if (!method)
                {
                    method = inst->getClass()->findMethod("__rakh__");
                }
                if (!method)
                {
                    method = inst->getClass()->findMethod("__setitem__");
                }
                if (method)
                {
                    BoundMethod bound(inst, method);
                    bound.call(*this, {idxVal, val});
                    return val;
                }
            }
        }

        if (target.isList())
        {
            auto list = target.getList();
            long long sz = list ? static_cast<long long>(list->elements.size()) : 0;
            long long idx = 0;
            if (std::holds_alternative<long long>(idxVal.data))
                idx = std::get<long long>(idxVal.data);
            else if (std::holds_alternative<double>(idxVal.data))
                idx = static_cast<long long>(std::get<double>(idxVal.data));
            else
                throw std::runtime_error("Array index number hona chahiye, ye kya diya: " + idxVal.typeName());

            long long origIdx = idx;
            if (idx < 0)
                idx += sz;

            if (idx < 0 || idx >= sz)
            {
                throw std::runtime_error(
                    "Aukaat ke bahar! Index " + std::to_string(origIdx) +
                    " pe assign karna chahte ho par size sirf " + std::to_string(sz) + " hai!");
            }

            list->elements[idx] = val;
            return val;
        }

        if (target.isMap())
        {
            auto map = target.getMap();
            std::string key = idxVal.toString();
            if (map)
            {
                map->set(key, val);
            }
            return val;
        }

        if (std::holds_alternative<std::string>(target.data))
        {
            throw std::runtime_error(
                "Ye sab doglapan hai! String ke andar index pe direct assign nahi kar sakte, '.badal()' use karo!");
        }

        throw std::runtime_error(
            "Ye kya bawasir hai? '" + target.typeName() + "' ke andar '[]' pe assign nahi ho sakta!");
    }

    // Property set: `object.property = value`
    if (auto *s = dynamic_cast<const SetExpr *>(expr))
    {
        Value objVal = evaluate(s->object.get());

        if (objVal.isReference())
        {
            const auto &ref = objVal.getReference();
            if (!ref.isMutable)
            {
                throw std::runtime_error(
                    "Doglapan! '" + ref.varName + "' abhi udhaar (borrowed) pe hai, isko modify nahi kar sakte!");
            }
            objVal = objVal.deref();
        }
        else if (auto *ident = dynamic_cast<const IdentifierExpr *>(s->object.get()))
        {
            VariableSlot *slot = environment->findSlot(ident->name);
            if (slot && (slot->sharedBorrows > 0 || slot->hasExclusiveBorrow))
            {
                throw std::runtime_error(
                    "Doglapan! '" + ident->name + "' abhi udhaar (borrowed) pe hai, isko modify nahi kar sakte!");
            }
        }

        if (!std::holds_alternative<std::shared_ptr<Instance>>(objVal.data))
        {
            throw std::runtime_error(
                "Zabardasti hai kya? '" + objVal.toString() + "' (" + objVal.typeName() +
                ") koi object nahi hai, uspe property set nahi kar sakte!");
        }
        auto instance = std::get<std::shared_ptr<Instance>>(objVal.data);
        if (!instance)
        {
            throw std::runtime_error("Object 'nalla' (null) hai! Hawa mein property set nahi hoti.");
        }

        Value val;
        if (auto *ident = dynamic_cast<const IdentifierExpr *>(s->value.get()))
        {
            val = moveVariable(ident->name);
        }
        else
        {
            val = evaluate(s->value.get());
        }

        instance->set(s->name, val);
        return val;
    }

    throw std::runtime_error("Ye expression samajh se pare hai!");
}

void Interpreter::pushComponentCall(const std::string &name)
{
    std::string parentPath = componentPathStack.empty() ? "" : componentPathStack.back() + "/";
    std::string keyPrefix = parentPath + name;
    int instanceIndex = componentInstanceCounters[keyPrefix]++;
    std::string currentPath = keyPrefix + "#" + std::to_string(instanceIndex);
    componentPathStack.push_back(currentPath);
    componentHookCursors[currentPath] = 0;
}

void Interpreter::popComponentCall()
{
    if (!componentPathStack.empty())
    {
        componentPathStack.pop_back();
    }
}

std::string Interpreter::getCurrentComponentPath() const
{
    if (componentPathStack.empty())
        return "App#0";
    return componentPathStack.back();
}

int Interpreter::getNextHookCursor(const std::string &path)
{
    return componentHookCursors[path]++;
}

void Interpreter::pushContext(const std::string &ctxId, const Value &val)
{
    contextStacks[ctxId].push_back(val);
}

void Interpreter::popContext(const std::string &ctxId)
{
    auto it = contextStacks.find(ctxId);
    if (it != contextStacks.end() && !it->second.empty())
    {
        it->second.pop_back();
    }
}

Value Interpreter::getContext(const std::string &ctxId, const Value &defaultVal)
{
    auto it = contextStacks.find(ctxId);
    if (it != contextStacks.end() && !it->second.empty())
    {
        return it->second.back();
    }
    return defaultVal;
}

void Interpreter::setRouterPath(const std::string &path)
{
    currentRouterPath = path;
    routerHistory.push_back(path);
    triggerUiRerender();
}

void Interpreter::triggerUiRerender()
{
    if (rootUiComponent.isNil() && rootCallable.isNil())
        return;

    componentInstanceCounters.clear();
    componentHookCursors.clear();
    componentPathStack.clear();

    Value renderedTree;
    if (!rootCallable.isNil() && std::holds_alternative<std::shared_ptr<Callable>>(rootCallable.deref().data))
    {
        auto callable = std::get<std::shared_ptr<Callable>>(rootCallable.deref().data);
        pushComponentCall("App");
        try
        {
            renderedTree = callable->call(*this, {});
            popComponentCall();
        }
        catch (const std::exception &e)
        {
            popComponentCall();
            std::cerr << "EXCEPTION in triggerUiRerender: " << e.what() << "\n";
            throw;
        }
        catch (...)
        {
            popComponentCall();
            std::cerr << "UNKNOWN EXCEPTION in triggerUiRerender\n";
            throw;
        }
    }
    else if (std::holds_alternative<std::shared_ptr<Callable>>(rootUiComponent.deref().data))
    {
        auto callable = std::get<std::shared_ptr<Callable>>(rootUiComponent.deref().data);
        pushComponentCall("App");
        try
        {
            renderedTree = callable->call(*this, {});
            popComponentCall();
        }
        catch (const std::exception &e)
        {
            popComponentCall();
            std::cerr << "EXCEPTION in triggerUiRerender: " << e.what() << "\n";
            throw;
        }
        catch (...)
        {
            popComponentCall();
            std::cerr << "UNKNOWN EXCEPTION in triggerUiRerender\n";
            throw;
        }
    }
    else
    {
        renderedTree = rootUiComponent;
    }

    rootUiComponent = renderedTree;

    std::vector<TailwindEngine::HitRegion> hitRegions;
    auto cmds = TailwindEngine::computeNativeLayout(renderedTree, 800.0f, 600.0f, &hitRegions);
    lastGpuCommandsJson = TailwindEngine::gpuCommandsToJson(cmds);
    lastHitRegionsJson = TailwindEngine::hitRegionsToJson(hitRegions);
    lastTerminalUi = renderUiScreen(renderedTree, "");
    interactiveRegions.clear();
    for (const auto &hr : hitRegions)
    {
        interactiveRegions[hr.id] = hr;
    }

    int htmlNodeId = 0;
    std::unordered_map<int, TailwindEngine::HitRegion> htmlHitRegions;
    lastHtml = renderUiHtmlTree(renderedTree, htmlNodeId, htmlHitRegions);
    for (const auto &p : htmlHitRegions)
    {
        interactiveRegions[p.first] = p.second;
    }
}

bool Interpreter::dispatchUiEvent(int nodeId, const std::string &eventType, const std::string &value)
{
    auto it = interactiveRegions.find(nodeId);
    if (it == interactiveRegions.end())
    {
        return false;
    }
    auto region = it->second;

    if (eventType == "click")
    {
        // If a value was sent along with click (e.g. text from an input), commit it to the closest input before this button
        if (!value.empty())
        {
            int targetInputId = 0;
            for (const auto &p : interactiveRegions)
            {
                if (p.second.hasChange && p.second.tag == "input" && (targetInputId == 0 || (p.first < nodeId && p.first > targetInputId)))
                {
                    targetInputId = p.first;
                }
            }
            if (targetInputId > 0)
            {
                auto &inRegion = interactiveRegions[targetInputId];
                Value chCb = inRegion.onChange.deref();
                if (std::holds_alternative<std::shared_ptr<Callable>>(chCb.data))
                {
                    auto callable = std::get<std::shared_ptr<Callable>>(chCb.data);
                    auto ev = std::make_shared<MemeMap>();
                    ev->set("type", Value("change"));
                    ev->set("targetId", Value(static_cast<long long>(targetInputId)));
                    auto target = std::make_shared<MemeMap>();
                    target->set("value", Value(value));
                    ev->set("target", Value(target));
                    ev->set("value", Value(value));
                    callable->call(*this, {Value(ev)});
                }
            }
        }

        Value cb = region.onClick.deref();
        if (std::holds_alternative<std::shared_ptr<Callable>>(cb.data))
        {
            auto callable = std::get<std::shared_ptr<Callable>>(cb.data);
            auto ev = std::make_shared<MemeMap>();
            ev->set("type", Value("click"));
            ev->set("targetId", Value(static_cast<long long>(nodeId)));
            callable->call(*this, {Value(ev)});
            triggerUiRerender();
            return true;
        }
    }
    else if (eventType == "change" || eventType == "input")
    {
        Value cb = region.onChange.deref();
        if (std::holds_alternative<std::shared_ptr<Callable>>(cb.data))
        {
            auto callable = std::get<std::shared_ptr<Callable>>(cb.data);
            auto ev = std::make_shared<MemeMap>();
            ev->set("type", Value(eventType));
            ev->set("targetId", Value(static_cast<long long>(nodeId)));
            auto target = std::make_shared<MemeMap>();
            target->set("value", Value(value));
            ev->set("target", Value(target));
            ev->set("value", Value(value));
            callable->call(*this, {Value(ev)});
            triggerUiRerender();
            return true;
        }
    }
    else if (eventType == "enter")
    {
        Value cb = region.onChange.deref();
        if (std::holds_alternative<std::shared_ptr<Callable>>(cb.data))
        {
            auto callable = std::get<std::shared_ptr<Callable>>(cb.data);
            auto ev = std::make_shared<MemeMap>();
            ev->set("type", Value("change"));
            ev->set("targetId", Value(static_cast<long long>(nodeId)));
            auto target = std::make_shared<MemeMap>();
            target->set("value", Value(value));
            ev->set("target", Value(target));
            ev->set("value", Value(value));
            callable->call(*this, {Value(ev)});
        }
        int targetBtnId = 0;
        for (const auto &p : interactiveRegions)
        {
            if (p.second.hasClick && p.second.tag == "button" && p.first > nodeId)
            {
                if (targetBtnId == 0 || p.first < targetBtnId)
                {
                    targetBtnId = p.first;
                }
            }
        }
        if (targetBtnId > 0)
        {
            auto &btnRegion = interactiveRegions[targetBtnId];
            Value btnCb = btnRegion.onClick.deref();
            if (std::holds_alternative<std::shared_ptr<Callable>>(btnCb.data))
            {
                auto callable = std::get<std::shared_ptr<Callable>>(btnCb.data);
                auto ev = std::make_shared<MemeMap>();
                ev->set("type", Value("click"));
                ev->set("targetId", Value(static_cast<long long>(targetBtnId)));
                callable->call(*this, {Value(ev)});
            }
        }
        triggerUiRerender();
        return true;
    }
    return false;
}
