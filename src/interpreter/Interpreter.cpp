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
#include <numeric>
#include <random>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <regex>
#include <deque>
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#endif
#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

thread_local std::unordered_set<const void *> activePrintPointers;

static double toDouble(const Value &val)
{
    Value v = val.deref();
    if (v.isNumeric())
        return v.asDouble();
    throw std::runtime_error("Number chahiye tha par mila: " + v.typeName());
}

static long long toLongLong(const Value &val)
{
    Value v = val.deref();
    if (v.isNumeric())
        return v.asInt();
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
    // Standard Colors
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
    {"gray", "\033[90m"},
    {"grey", "\033[90m"},
    {"dhundhla", "\033[90m"},
    {"orange", "\033[38;5;208m"},
    {"santre", "\033[38;5;208m"},
    {"purple", "\033[38;5;129m"},
    {"jamuni", "\033[38;5;129m"},

    // Bright Colors
    {"bright_red", "\033[91m"},
    {"bright_laal", "\033[91m"},
    {"chamkila_laal", "\033[91m"},
    {"bright_green", "\033[92m"},
    {"bright_hara", "\033[92m"},
    {"chamkila_hara", "\033[92m"},
    {"bright_yellow", "\033[93m"},
    {"bright_peela", "\033[93m"},
    {"chamkila_peela", "\033[93m"},
    {"bright_blue", "\033[94m"},
    {"bright_neela", "\033[94m"},
    {"chamkila_neela", "\033[94m"},
    {"bright_magenta", "\033[95m"},
    {"bright_pink", "\033[95m"},
    {"chamkila_gulabi", "\033[95m"},
    {"bright_cyan", "\033[96m"},
    {"bright_asmani", "\033[96m"},
    {"chamkila_asmani", "\033[96m"},
    {"bright_white", "\033[97m"},
    {"bright_safed", "\033[97m"},
    {"chamkila_safed", "\033[97m"},

    // Text Styles
    {"mota", "\033[1m"},
    {"bold", "\033[1m"},
    {"halka", "\033[2m"},
    {"dim", "\033[2m"},
    {"tedha", "\033[3m"},
    {"italic", "\033[3m"},
    {"line", "\033[4m"},
    {"underline", "\033[4m"},
    {"blink", "\033[5m"},
    {"chamak", "\033[5m"},
    {"ulta", "\033[7m"},
    {"reverse", "\033[7m"},
    {"invert", "\033[7m"},
    {"kata", "\033[9m"},
    {"strikethrough", "\033[9m"},
    {"strike", "\033[9m"},

    // Resets
    {"/", "\033[0m"},
    {"reset", "\033[0m"},
    {"saaf", "\033[0m"}
};

static bool parseHexColor(const std::string &str, int &r, int &g, int &b)
{
    if (str.empty() || str[0] != '#') return false;
    if (str.size() == 7)
    {
        for (size_t i = 1; i < 7; ++i)
        {
            if (!std::isxdigit(static_cast<unsigned char>(str[i]))) return false;
        }
        r = std::stoi(str.substr(1, 2), nullptr, 16);
        g = std::stoi(str.substr(3, 2), nullptr, 16);
        b = std::stoi(str.substr(5, 2), nullptr, 16);
        return true;
    }
    else if (str.size() == 4)
    {
        for (size_t i = 1; i < 4; ++i)
        {
            if (!std::isxdigit(static_cast<unsigned char>(str[i]))) return false;
        }
        r = std::stoi(std::string(2, str[1]), nullptr, 16);
        g = std::stoi(std::string(2, str[2]), nullptr, 16);
        b = std::stoi(std::string(2, str[3]), nullptr, 16);
        return true;
    }
    return false;
}

static bool parseRgbColor(const std::string &str, int &r, int &g, int &b)
{
    if (str.size() < 7) return false;
    if (str.rfind("rgb(", 0) != 0 || str.back() != ')') return false;
    std::string inner = str.substr(4, str.size() - 5);
    std::stringstream ss(inner);
    std::string p1, p2, p3;
    if (!std::getline(ss, p1, ',') || !std::getline(ss, p2, ',') || !std::getline(ss, p3, ',')) return false;
    try {
        r = std::stoi(p1);
        g = std::stoi(p2);
        b = std::stoi(p3);
    } catch (...) {
        return false;
    }
    return (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255);
}

static bool parse256Color(const std::string &str, int &code)
{
    if (str.empty()) return false;
    for (char c : str)
    {
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    }
    try {
        code = std::stoi(str);
    } catch (...) {
        return false;
    }
    return (code >= 0 && code <= 255);
}

static bool isBadgeTag(const std::string &w, std::string &outBadge)
{
    if (w == "success" || w == "theek")
    {
        outBadge = "\033[1;32m[SUCCESS]\033[0m";
        return true;
    }
    if (w == "error" || w == "galat" || w == "gadbad")
    {
        outBadge = "\033[1;31m[ERROR]\033[0m";
        return true;
    }
    if (w == "warn" || w == "warning" || w == "chetavni")
    {
        outBadge = "\033[1;33m[WARN]\033[0m";
        return true;
    }
    if (w == "info" || w == "suchna" || w == "jankari")
    {
        outBadge = "\033[1;36m[INFO]\033[0m";
        return true;
    }
    return false;
}

static bool resolveRichWord(const std::string &w, std::string &outAnsi, bool allowStatusAliases = true)
{
    auto it = RICH_TAGS.find(w);
    if (it != RICH_TAGS.end())
    {
        outAnsi = it->second;
        return true;
    }

    if (allowStatusAliases)
    {
        if (w == "success" || w == "theek") { outAnsi = "\033[1;32m"; return true; }
        if (w == "error" || w == "galat" || w == "gadbad") { outAnsi = "\033[1;31m"; return true; }
        if (w == "warn" || w == "warning" || w == "chetavni") { outAnsi = "\033[1;33m"; return true; }
        if (w == "info" || w == "suchna" || w == "jankari") { outAnsi = "\033[1;36m"; return true; }
    }

    // Hex colors: #RRGGBB, #RGB, fg:#..., bg:#...
    if (w.rfind("#", 0) == 0)
    {
        int r, g, b;
        if (parseHexColor(w, r, g, b))
        {
            outAnsi = "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
            return true;
        }
    }
    else if (w.rfind("fg:#", 0) == 0)
    {
        int r, g, b;
        if (parseHexColor(w.substr(3), r, g, b))
        {
            outAnsi = "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
            return true;
        }
    }
    else if (w.rfind("bg:#", 0) == 0)
    {
        int r, g, b;
        if (parseHexColor(w.substr(3), r, g, b))
        {
            outAnsi = "\033[48;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
            return true;
        }
    }

    // RGB function: rgb(r,g,b), fg:rgb(r,g,b), bg:rgb(r,g,b)
    if (w.rfind("rgb(", 0) == 0)
    {
        int r, g, b;
        if (parseRgbColor(w, r, g, b))
        {
            outAnsi = "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
            return true;
        }
    }
    else if (w.rfind("fg:rgb(", 0) == 0)
    {
        int r, g, b;
        if (parseRgbColor(w.substr(3), r, g, b))
        {
            outAnsi = "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
            return true;
        }
    }
    else if (w.rfind("bg:rgb(", 0) == 0)
    {
        int r, g, b;
        if (parseRgbColor(w.substr(3), r, g, b))
        {
            outAnsi = "\033[48;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
            return true;
        }
    }

    // 256 colors: color:N, fg:N, c:N, bg:N
    if (w.rfind("color:", 0) == 0)
    {
        int code;
        if (parse256Color(w.substr(6), code))
        {
            outAnsi = "\033[38;5;" + std::to_string(code) + "m";
            return true;
        }
    }
    else if (w.rfind("fg:", 0) == 0)
    {
        int code;
        if (parse256Color(w.substr(3), code))
        {
            outAnsi = "\033[38;5;" + std::to_string(code) + "m";
            return true;
        }
    }
    else if (w.rfind("c:", 0) == 0)
    {
        int code;
        if (parse256Color(w.substr(2), code))
        {
            outAnsi = "\033[38;5;" + std::to_string(code) + "m";
            return true;
        }
    }
    else if (w.rfind("bg:", 0) == 0)
    {
        std::string rest = w.substr(3);
        int code;
        if (parse256Color(rest, code))
        {
            outAnsi = "\033[48;5;" + std::to_string(code) + "m";
            return true;
        }
        static const std::unordered_map<std::string, std::string> BG_MAP = {
            {"laal", "\033[41m"}, {"red", "\033[41m"},
            {"hara", "\033[42m"}, {"green", "\033[42m"},
            {"peela", "\033[43m"}, {"yellow", "\033[43m"},
            {"neela", "\033[44m"}, {"blue", "\033[44m"},
            {"gulabi", "\033[45m"}, {"pink", "\033[45m"}, {"magenta", "\033[45m"},
            {"asmani", "\033[46m"}, {"cyan", "\033[46m"},
            {"safed", "\033[47m"}, {"white", "\033[47m"},
            {"kala", "\033[40m"}, {"black", "\033[40m"},
            {"gray", "\033[100m"}, {"grey", "\033[100m"}, {"dhundhla", "\033[100m"},
            {"orange", "\033[48;5;208m"}, {"santre", "\033[48;5;208m"},
            {"purple", "\033[48;5;129m"}, {"jamuni", "\033[48;5;129m"},
            {"bright_red", "\033[101m"}, {"bright_laal", "\033[101m"},
            {"bright_green", "\033[102m"}, {"bright_hara", "\033[102m"},
            {"bright_yellow", "\033[103m"}, {"bright_peela", "\033[103m"},
            {"bright_blue", "\033[104m"}, {"bright_neela", "\033[104m"},
            {"bright_magenta", "\033[105m"}, {"bright_pink", "\033[105m"},
            {"bright_cyan", "\033[106m"}, {"bright_asmani", "\033[106m"},
            {"bright_white", "\033[107m"}, {"bright_safed", "\033[107m"}
        };
        auto bit = BG_MAP.find(rest);
        if (bit != BG_MAP.end())
        {
            outAnsi = bit->second;
            return true;
        }
    }

    return false;
}

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
                
                // Normalize spaces inside parentheses, e.g. rgb(255, 0, 0)
                std::string normalizedTag;
                bool inParens = false;
                for (char c : tagContent)
                {
                    if (c == '(') inParens = true;
                    else if (c == ')') inParens = false;
                    if (inParens && std::isspace(static_cast<unsigned char>(c))) continue;
                    normalizedTag += c;
                }

                std::vector<std::string> words;
                std::stringstream ss(normalizedTag);
                std::string word;
                while (ss >> word)
                {
                    for (char &c : word) c = std::tolower(static_cast<unsigned char>(c));
                    words.push_back(word);
                }

                if (words.size() == 1)
                {
                    std::string badge;
                    if (isBadgeTag(words[0], badge))
                    {
                        result += badge;
                        i = closePos + 1;
                        continue;
                    }
                }

                bool allValid = !words.empty();
                std::string combinedAnsi;
                for (const auto &w : words)
                {
                    std::string wordAnsi;
                    if (!resolveRichWord(w, wordAnsi, false))
                    {
                        allValid = false;
                        break;
                    }
                    combinedAnsi += wordAnsi;
                }

                if (allValid)
                {
                    result += combinedAnsi;
                    for (const auto &w : words)
                    {
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

std::string resolveStyleAnsi(const std::string &rawStyle)
{
    std::string s = rawStyle;
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();

    if (s.empty()) return "";

    if (s.front() == '[' && s.back() == ']')
    {
        s = s.substr(1, s.size() - 2);
    }

    std::string normalized;
    bool inParens = false;
    for (char c : s)
    {
        if (c == '(') inParens = true;
        else if (c == ')') inParens = false;
        if (inParens && std::isspace(static_cast<unsigned char>(c))) continue;
        if (c == ',') { normalized += ' '; continue; }
        normalized += c;
    }

    std::stringstream ss(normalized);
    std::string word;
    std::string combined;
    while (ss >> word)
    {
        for (char &c : word) c = std::tolower(static_cast<unsigned char>(c));
        std::string ansi;
        if (resolveRichWord(word, ansi, true))
        {
            combined += ansi;
        }
    }
    return combined;
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
            [](int n) -> std::string { return std::to_string(n); },
            [](uint8_t u) -> std::string { return std::to_string(static_cast<int>(u)); },
            [](double d) -> std::string {
                std::string s = std::to_string(d);
                s.erase(s.find_last_not_of('0') + 1, std::string::npos);
                if (s.back() == '.') s += "0";
                return s;
            },
            [](float f) -> std::string {
                std::string s = std::to_string(f);
                s.erase(s.find_last_not_of('0') + 1, std::string::npos);
                if (s.back() == '.') s += "0";
                return s;
            },
            [](char c) -> std::string { return std::string("\"") + c + "\""; },
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
            [](const std::shared_ptr<MemeBytes> &bytes) -> std::string {
                if (!bytes) return "[]";
                std::string out = "[";
                for (size_t i = 0; i < bytes->data.size(); ++i)
                {
                    if (i > 0) out += ", ";
                    out += std::to_string(static_cast<int>(bytes->data[i]));
                }
                out += "]";
                return out;
            },
            [](const std::shared_ptr<MemeSet> &set) -> std::string {
                if (!set) return "[]";
                std::string out = "[";
                for (size_t i = 0; i < set->elements.size(); ++i)
                {
                    if (i > 0) out += ", ";
                    out += stringify(set->elements[i]);
                }
                out += "]";
                return out;
            },
            [](const std::shared_ptr<MemeTuple> &tup) -> std::string {
                if (!tup) return "[]";
                std::string out = "[";
                for (size_t i = 0; i < tup->elements.size(); ++i)
                {
                    if (i > 0) out += ", ";
                    out += stringify(tup->elements[i]);
                }
                out += "]";
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

std::string bytesToString(const std::shared_ptr<MemeBytes> &bytes)
{
    if (!bytes) return "b\"\"";
    std::string result = "b\"";
    for (uint8_t b : bytes->data)
    {
        if (b == '\\') result += "\\\\";
        else if (b == '"') result += "\\\"";
        else if (b == '\n') result += "\\n";
        else if (b == '\r') result += "\\r";
        else if (b == '\t') result += "\\t";
        else if (b >= 32 && b <= 126) result += static_cast<char>(b);
        else {
            char buf[8];
            snprintf(buf, sizeof(buf), "\\x%02x", b);
            result += buf;
        }
    }
    result += "\"";
    return result;
}

std::string setToString(const std::shared_ptr<MemeSet> &set)
{
    if (!set || set->elements.empty()) return "set()";
    const void *ptr = static_cast<const void *>(set.get());
    if (activePrintPointers.find(ptr) != activePrintPointers.end()) return "{...}";
    activePrintPointers.insert(ptr);
    std::string result = "{";
    for (size_t i = 0; i < set->elements.size(); ++i)
    {
        if (i > 0) result += ", ";
        Value el = set->elements[i].deref();
        if (std::holds_alternative<std::string>(el.data))
            result += "\"" + std::get<std::string>(el.data) + "\"";
        else
            result += el.toString();
    }
    result += "}";
    activePrintPointers.erase(ptr);
    return result;
}

std::string tupleToString(const std::shared_ptr<MemeTuple> &tuple)
{
    if (!tuple || tuple->elements.empty()) return "()";
    const void *ptr = static_cast<const void *>(tuple.get());
    if (activePrintPointers.find(ptr) != activePrintPointers.end()) return "(...)";
    activePrintPointers.insert(ptr);
    std::string result = "(";
    for (size_t i = 0; i < tuple->elements.size(); ++i)
    {
        if (i > 0) result += ", ";
        Value el = tuple->elements[i].deref();
        if (std::holds_alternative<std::string>(el.data))
            result += "\"" + std::get<std::string>(el.data) + "\"";
        else
            result += el.toString();
    }
    if (tuple->elements.size() == 1) result += ",";
    result += ")";
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

    // Constants
    m->set("pi", Value(3.14159265358979323846));
    m->set("e", Value(2.71828182845904523536));
    m->set("tau", Value(6.28318530717958647692));
    m->set("inf", Value(std::numeric_limits<double>::infinity()));
    m->set("nan", Value(std::numeric_limits<double>::quiet_NaN()));

    // Basic & Rounding
    m->set("abs", Value(std::make_shared<NativeMethodCallable>("abs", 1, [](Interpreter &, const std::vector<Value> &args) {
        Value v = args[0].deref();
        if (v.isInteger()) return Value(std::abs(v.asInt()));
        return Value(std::abs(v.asDouble()));
    })));
    m->set("positive", m->entries["abs"]);

    m->set("round", Value(std::make_shared<NativeMethodCallable>("round", 1, 2, [](Interpreter &, const std::vector<Value> &args) {
        double d = args[0].deref().asDouble();
        if (args.size() > 1) {
            int digits = static_cast<int>(args[1].deref().asInt());
            double factor = std::pow(10.0, digits);
            return Value(std::round(d * factor) / factor);
        }
        return Value(static_cast<long long>(std::round(d)));
    })));
    m->set("gol", m->entries["round"]);

    m->set("floor", Value(std::make_shared<NativeMethodCallable>("floor", 1, [](Interpreter &, const std::vector<Value> &args) {
        return Value(static_cast<long long>(std::floor(args[0].deref().asDouble())));
    })));
    m->set("niche", m->entries["floor"]);

    m->set("ceil", Value(std::make_shared<NativeMethodCallable>("ceil", 1, [](Interpreter &, const std::vector<Value> &args) {
        return Value(static_cast<long long>(std::ceil(args[0].deref().asDouble())));
    })));
    m->set("upar", m->entries["ceil"]);

    m->set("trunc", Value(std::make_shared<NativeMethodCallable>("trunc", 1, [](Interpreter &, const std::vector<Value> &args) {
        return Value(static_cast<long long>(std::trunc(args[0].deref().asDouble())));
    })));
    m->set("kaat", m->entries["trunc"]);

    // Powers & Roots
    m->set("sqrt", Value(std::make_shared<NativeMethodCallable>("sqrt", 1, [](Interpreter &, const std::vector<Value> &args) {
        double d = args[0].deref().asDouble();
        if (d < 0.0) throw std::runtime_error("Negative number ka square root nikaaloge? Math fail ho gaya!");
        return Value(std::sqrt(d));
    })));
    m->set("jadd", m->entries["sqrt"]);

    m->set("cbrt", Value(std::make_shared<NativeMethodCallable>("cbrt", 1, [](Interpreter &, const std::vector<Value> &args) {
        return Value(std::cbrt(args[0].deref().asDouble()));
    })));

    m->set("pow", Value(std::make_shared<NativeMethodCallable>("pow", 2, [](Interpreter &, const std::vector<Value> &args) {
        return Value(std::pow(args[0].deref().asDouble(), args[1].deref().asDouble()));
    })));
    m->set("taqat", m->entries["pow"]);

    m->set("exp", Value(std::make_shared<NativeMethodCallable>("exp", 1, [](Interpreter &, const std::vector<Value> &args) {
        return Value(std::exp(args[0].deref().asDouble()));
    })));

    m->set("log", Value(std::make_shared<NativeMethodCallable>("log", 1, 2, [](Interpreter &, const std::vector<Value> &args) {
        double x = args[0].deref().asDouble();
        if (x <= 0.0) throw std::runtime_error("log sirf positive numbers ka nikalta hai!");
        if (args.size() > 1) {
            double base = args[1].deref().asDouble();
            if (base <= 0.0 || base == 1.0) throw std::runtime_error("Invalid log base!");
            return Value(std::log(x) / std::log(base));
        }
        return Value(std::log(x));
    })));
    m->set("ln", m->entries["log"]);

    m->set("log2", Value(std::make_shared<NativeMethodCallable>("log2", 1, [](Interpreter &, const std::vector<Value> &args) {
        double x = args[0].deref().asDouble();
        if (x <= 0.0) throw std::runtime_error("log2 sirf positive numbers ka nikalta hai!");
        return Value(std::log2(x));
    })));

    m->set("log10", Value(std::make_shared<NativeMethodCallable>("log10", 1, [](Interpreter &, const std::vector<Value> &args) {
        double x = args[0].deref().asDouble();
        if (x <= 0.0) throw std::runtime_error("log10 sirf positive numbers ka nikalta hai!");
        return Value(std::log10(x));
    })));

    // Trigonometry
    m->set("sin", Value(std::make_shared<NativeMethodCallable>("sin", 1, [](Interpreter &, const std::vector<Value> &args) {
        return Value(std::sin(args[0].deref().asDouble()));
    })));
    m->set("cos", Value(std::make_shared<NativeMethodCallable>("cos", 1, [](Interpreter &, const std::vector<Value> &args) {
        return Value(std::cos(args[0].deref().asDouble()));
    })));
    m->set("tan", Value(std::make_shared<NativeMethodCallable>("tan", 1, [](Interpreter &, const std::vector<Value> &args) {
        return Value(std::tan(args[0].deref().asDouble()));
    })));
    m->set("asin", Value(std::make_shared<NativeMethodCallable>("asin", 1, [](Interpreter &, const std::vector<Value> &args) {
        return Value(std::asin(args[0].deref().asDouble()));
    })));
    m->set("acos", Value(std::make_shared<NativeMethodCallable>("acos", 1, [](Interpreter &, const std::vector<Value> &args) {
        return Value(std::acos(args[0].deref().asDouble()));
    })));
    m->set("atan", Value(std::make_shared<NativeMethodCallable>("atan", 1, [](Interpreter &, const std::vector<Value> &args) {
        return Value(std::atan(args[0].deref().asDouble()));
    })));
    m->set("atan2", Value(std::make_shared<NativeMethodCallable>("atan2", 2, [](Interpreter &, const std::vector<Value> &args) {
        return Value(std::atan2(args[0].deref().asDouble(), args[1].deref().asDouble()));
    })));

    // Hyperbolic
    m->set("sinh", Value(std::make_shared<NativeMethodCallable>("sinh", 1, [](Interpreter &, const std::vector<Value> &args) {
        return Value(std::sinh(args[0].deref().asDouble()));
    })));
    m->set("cosh", Value(std::make_shared<NativeMethodCallable>("cosh", 1, [](Interpreter &, const std::vector<Value> &args) {
        return Value(std::cosh(args[0].deref().asDouble()));
    })));
    m->set("tanh", Value(std::make_shared<NativeMethodCallable>("tanh", 1, [](Interpreter &, const std::vector<Value> &args) {
        return Value(std::tanh(args[0].deref().asDouble()));
    })));
    m->set("asinh", Value(std::make_shared<NativeMethodCallable>("asinh", 1, [](Interpreter &, const std::vector<Value> &args) {
        return Value(std::asinh(args[0].deref().asDouble()));
    })));
    m->set("acosh", Value(std::make_shared<NativeMethodCallable>("acosh", 1, [](Interpreter &, const std::vector<Value> &args) {
        return Value(std::acosh(args[0].deref().asDouble()));
    })));
    m->set("atanh", Value(std::make_shared<NativeMethodCallable>("atanh", 1, [](Interpreter &, const std::vector<Value> &args) {
        return Value(std::atanh(args[0].deref().asDouble()));
    })));

    // Angular conversions
    m->set("degrees", Value(std::make_shared<NativeMethodCallable>("degrees", 1, [](Interpreter &, const std::vector<Value> &args) {
        return Value(args[0].deref().asDouble() * (180.0 / 3.14159265358979323846));
    })));
    m->set("degree_bana", m->entries["degrees"]);

    m->set("radians", Value(std::make_shared<NativeMethodCallable>("radians", 1, [](Interpreter &, const std::vector<Value> &args) {
        return Value(args[0].deref().asDouble() * (3.14159265358979323846 / 180.0));
    })));
    m->set("radian_bana", m->entries["radians"]);

    // Geometry & distance
    m->set("hypot", Value(std::make_shared<NativeMethodCallable>("hypot", 2, [](Interpreter &, const std::vector<Value> &args) {
        return Value(std::hypot(args[0].deref().asDouble(), args[1].deref().asDouble()));
    })));
    m->set("dist", Value(std::make_shared<NativeMethodCallable>("dist", 2, [](Interpreter &, const std::vector<Value> &args) {
        Value p1 = args[0].deref();
        Value p2 = args[1].deref();
        std::vector<double> v1, v2;
        if (p1.isList()) { for (const auto &e : p1.getList()->elements) v1.push_back(e.deref().asDouble()); }
        else if (p1.isTuple()) { for (const auto &e : p1.getTuple()->elements) v1.push_back(e.deref().asDouble()); }
        if (p2.isList()) { for (const auto &e : p2.getList()->elements) v2.push_back(e.deref().asDouble()); }
        else if (p2.isTuple()) { for (const auto &e : p2.getTuple()->elements) v2.push_back(e.deref().asDouble()); }
        if (v1.size() != v2.size() || v1.empty()) throw std::runtime_error("dist requires two lists/tuples of equal dimensions!");
        double sum = 0.0;
        for (size_t i = 0; i < v1.size(); ++i) { double d = v1[i] - v2[i]; sum += d * d; }
        return Value(std::sqrt(sum));
    })));

    // Min / Max / Clamp
    m->set("min", Value(std::make_shared<NativeMethodCallable>("min", 2, [](Interpreter &, const std::vector<Value> &args) {
        Value a = args[0].deref();
        Value b = args[1].deref();
        if (a.isInteger() && b.isInteger()) return Value(std::min(a.asInt(), b.asInt()));
        return Value(std::min(a.asDouble(), b.asDouble()));
    })));
    m->set("halka", m->entries["min"]);

    m->set("max", Value(std::make_shared<NativeMethodCallable>("max", 2, [](Interpreter &, const std::vector<Value> &args) {
        Value a = args[0].deref();
        Value b = args[1].deref();
        if (a.isInteger() && b.isInteger()) return Value(std::max(a.asInt(), b.asInt()));
        return Value(std::max(a.asDouble(), b.asDouble()));
    })));
    m->set("bhaari", m->entries["max"]);

    m->set("clamp", Value(std::make_shared<NativeMethodCallable>("clamp", 3, [](Interpreter &, const std::vector<Value> &args) {
        double v = args[0].deref().asDouble();
        double mn = args[1].deref().asDouble();
        double mx = args[2].deref().asDouble();
        return Value(std::clamp(v, mn, mx));
    })));

    // Number Theory
    m->set("gcd", Value(std::make_shared<NativeMethodCallable>("gcd", 2, [](Interpreter &, const std::vector<Value> &args) {
        long long a = args[0].deref().asInt();
        long long b = args[1].deref().asInt();
        return Value(std::gcd(a, b));
    })));
    m->set("mahatam", m->entries["gcd"]);

    m->set("lcm", Value(std::make_shared<NativeMethodCallable>("lcm", 2, [](Interpreter &, const std::vector<Value> &args) {
        long long a = args[0].deref().asInt();
        long long b = args[1].deref().asInt();
        return Value(std::lcm(a, b));
    })));
    m->set("laghutam", m->entries["lcm"]);

    m->set("factorial", Value(std::make_shared<NativeMethodCallable>("factorial", 1, [](Interpreter &, const std::vector<Value> &args) {
        long long n = args[0].deref().asInt();
        if (n < 0) throw std::runtime_error("Negative number ka factorial nahi hota!");
        if (n > 20) throw std::runtime_error("Factorial overflow! (n <= 20)");
        long long res = 1;
        for (long long i = 2; i <= n; ++i) res *= i;
        return Value(res);
    })));
    m->set("kramgunan", m->entries["factorial"]);

    m->set("comb", Value(std::make_shared<NativeMethodCallable>("comb", 2, [](Interpreter &, const std::vector<Value> &args) {
        long long n = args[0].deref().asInt();
        long long k = args[1].deref().asInt();
        if (k < 0 || k > n) return Value(0LL);
        if (k == 0 || k == n) return Value(1LL);
        if (k > n / 2) k = n - k;
        long long res = 1;
        for (long long i = 1; i <= k; ++i) {
            res = res * (n - i + 1) / i;
        }
        return Value(res);
    })));

    m->set("perm", Value(std::make_shared<NativeMethodCallable>("perm", 2, [](Interpreter &, const std::vector<Value> &args) {
        long long n = args[0].deref().asInt();
        long long k = args[1].deref().asInt();
        if (k < 0 || k > n) return Value(0LL);
        long long res = 1;
        for (long long i = 0; i < k; ++i) res *= (n - i);
        return Value(res);
    })));

    m->set("is_nan", Value(std::make_shared<NativeMethodCallable>("is_nan", 1, [](Interpreter &, const std::vector<Value> &args) {
        return Value(std::isnan(args[0].deref().asDouble()));
    })));
    m->set("is_inf", Value(std::make_shared<NativeMethodCallable>("is_inf", 1, [](Interpreter &, const std::vector<Value> &args) {
        return Value(std::isinf(args[0].deref().asDouble()));
    })));
    m->set("is_finite", Value(std::make_shared<NativeMethodCallable>("is_finite", 1, [](Interpreter &, const std::vector<Value> &args) {
        return Value(std::isfinite(args[0].deref().asDouble()));
    })));

    // Backward compatibility: tukka and tukka_num
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

static std::shared_ptr<MemeMap> createRandomModule()
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    m->set("random", Value(std::make_shared<NativeMethodCallable>("random", 0, [](Interpreter &, const std::vector<Value> &) {
        static std::mt19937 gen(std::random_device{}());
        static std::uniform_real_distribution<double> dist(0.0, 1.0);
        return Value(dist(gen));
    })));
    m->set("tukka", m->entries["random"]);

    m->set("randint", Value(std::make_shared<NativeMethodCallable>("randint", 2, [](Interpreter &, const std::vector<Value> &args) {
        long long mn = args[0].deref().asInt();
        long long mx = args[1].deref().asInt();
        if (mn > mx) std::swap(mn, mx);
        static std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<long long> dist(mn, mx);
        return Value(dist(gen));
    })));
    m->set("tukka_num", m->entries["randint"]);

    m->set("uniform", Value(std::make_shared<NativeMethodCallable>("uniform", 2, [](Interpreter &, const std::vector<Value> &args) {
        double mn = args[0].deref().asDouble();
        double mx = args[1].deref().asDouble();
        if (mn > mx) std::swap(mn, mx);
        static std::mt19937 gen(std::random_device{}());
        std::uniform_real_distribution<double> dist(mn, mx);
        return Value(dist(gen));
    })));

    m->set("choice", Value(std::make_shared<NativeMethodCallable>("choice", 1, [](Interpreter &, const std::vector<Value> &args) {
        Value v = args[0].deref();
        static std::mt19937 gen(std::random_device{}());
        if (v.isList()) {
            auto l = v.getList();
            if (l->elements.empty()) throw std::runtime_error("choice() cannot pick from an empty list!");
            std::uniform_int_distribution<size_t> dist(0, l->elements.size() - 1);
            return l->elements[dist(gen)];
        }
        if (v.isTuple()) {
            auto t = v.getTuple();
            if (t->elements.empty()) throw std::runtime_error("choice() cannot pick from an empty tuple!");
            std::uniform_int_distribution<size_t> dist(0, t->elements.size() - 1);
            return t->elements[dist(gen)];
        }
        if (std::holds_alternative<std::string>(v.data)) {
            const auto &s = std::get<std::string>(v.data);
            if (s.empty()) throw std::runtime_error("choice() cannot pick from an empty string!");
            std::uniform_int_distribution<size_t> dist(0, s.size() - 1);
            return Value(std::string(1, s[dist(gen)]));
        }
        throw std::runtime_error("choice() requires a list, tuple, or string!");
    })));
    m->set("chun", m->entries["choice"]);

    m->set("shuffle", Value(std::make_shared<NativeMethodCallable>("shuffle", 1, [](Interpreter &, const std::vector<Value> &args) {
        Value v = args[0].deref();
        if (!v.isList()) throw std::runtime_error("shuffle() requires a list!");
        auto l = v.getList();
        static std::mt19937 gen(std::random_device{}());
        std::shuffle(l->elements.begin(), l->elements.end(), gen);
        return v;
    })));
    m->set("phent", m->entries["shuffle"]);

    m->set("sample", Value(std::make_shared<NativeMethodCallable>("sample", 2, [](Interpreter &, const std::vector<Value> &args) {
        Value v = args[0].deref();
        long long k = args[1].deref().asInt();
        if (k < 0) throw std::runtime_error("sample count cannot be negative!");
        std::vector<Value> pool;
        if (v.isList()) pool = v.getList()->elements;
        else if (v.isTuple()) pool = v.getTuple()->elements;
        else if (v.isSet()) pool = v.getSet()->elements;
        else throw std::runtime_error("sample() requires a list, tuple, or set!");
        if (static_cast<size_t>(k) > pool.size()) throw std::runtime_error("sample larger than population!");
        static std::mt19937 gen(std::random_device{}());
        std::shuffle(pool.begin(), pool.end(), gen);
        pool.resize(static_cast<size_t>(k));
        return Value(std::make_shared<MemeList>(std::move(pool)));
    })));
    m->set("namoona", m->entries["sample"]);

    m->set("seed", Value(std::make_shared<NativeMethodCallable>("seed", 1, [](Interpreter &, const std::vector<Value> &args) {
        long long s = args[0].deref().asInt();
        std::srand(static_cast<unsigned int>(s));
        return Value(std::monostate{});
    })));
    m->set("beej", m->entries["seed"]);

    return m;
}

static std::shared_ptr<MemeMap> createStatisticsModule()
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    auto getNumbers = [](const Value &v) -> std::vector<double> {
        std::vector<double> nums;
        Value d = v.deref();
        if (d.isList()) {
            for (const auto &e : d.getList()->elements) nums.push_back(e.deref().asDouble());
        } else if (d.isTuple()) {
            for (const auto &e : d.getTuple()->elements) nums.push_back(e.deref().asDouble());
        } else if (d.isSet()) {
            for (const auto &e : d.getSet()->elements) nums.push_back(e.deref().asDouble());
        } else {
            throw std::runtime_error("Statistics function requires a list, tuple, or set!");
        }
        if (nums.empty()) throw std::runtime_error("Cannot calculate statistics on empty data!");
        return nums;
    };

    m->set("mean", Value(std::make_shared<NativeMethodCallable>("mean", 1, [getNumbers](Interpreter &, const std::vector<Value> &args) {
        auto nums = getNumbers(args[0]);
        double sum = 0.0;
        for (double n : nums) sum += n;
        return Value(sum / nums.size());
    })));
    m->set("ausat", m->entries["mean"]);

    m->set("median", Value(std::make_shared<NativeMethodCallable>("median", 1, [getNumbers](Interpreter &, const std::vector<Value> &args) {
        auto nums = getNumbers(args[0]);
        std::sort(nums.begin(), nums.end());
        size_t n = nums.size();
        if (n % 2 == 1) return Value(nums[n / 2]);
        return Value((nums[n / 2 - 1] + nums[n / 2]) / 2.0);
    })));
    m->set("beech_ka", m->entries["median"]);

    m->set("mode", Value(std::make_shared<NativeMethodCallable>("mode", 1, [](Interpreter &, const std::vector<Value> &args) {
        Value d = args[0].deref();
        std::unordered_map<std::string, size_t> counts;
        std::unordered_map<std::string, Value> valMap;
        std::vector<Value> items;
        if (d.isList()) items = d.getList()->elements;
        else if (d.isTuple()) items = d.getTuple()->elements;
        else throw std::runtime_error("mode() requires a list or tuple!");
        if (items.empty()) throw std::runtime_error("mode() requires non-empty data!");
        size_t maxCount = 0;
        std::string modeKey;
        for (const auto &item : items) {
            std::string k = item.deref().toString();
            counts[k]++;
            valMap[k] = item;
            if (counts[k] > maxCount) {
                maxCount = counts[k];
                modeKey = k;
            }
        }
        return valMap[modeKey];
    })));

    m->set("variance", Value(std::make_shared<NativeMethodCallable>("variance", 1, [getNumbers](Interpreter &, const std::vector<Value> &args) {
        auto nums = getNumbers(args[0]);
        if (nums.size() < 2) throw std::runtime_error("Variance requires at least 2 data points!");
        double meanVal = 0.0;
        for (double n : nums) meanVal += n;
        meanVal /= nums.size();
        double sumSq = 0.0;
        for (double n : nums) { double diff = n - meanVal; sumSq += diff * diff; }
        return Value(sumSq / (nums.size() - 1));
    })));
    m->set("bhatkao", m->entries["variance"]);

    m->set("stdev", Value(std::make_shared<NativeMethodCallable>("stdev", 1, [getNumbers](Interpreter &, const std::vector<Value> &args) {
        auto nums = getNumbers(args[0]);
        if (nums.size() < 2) throw std::runtime_error("Standard deviation requires at least 2 data points!");
        double meanVal = 0.0;
        for (double n : nums) meanVal += n;
        meanVal /= nums.size();
        double sumSq = 0.0;
        for (double n : nums) { double diff = n - meanVal; sumSq += diff * diff; }
        return Value(std::sqrt(sumSq / (nums.size() - 1)));
    })));
    m->set("sthitik", m->entries["stdev"]);

    m->set("min", Value(std::make_shared<NativeMethodCallable>("min", 1, [getNumbers](Interpreter &, const std::vector<Value> &args) {
        auto nums = getNumbers(args[0]);
        double mVal = nums[0];
        for (double n : nums) if (n < mVal) mVal = n;
        return Value(mVal);
    })));
    m->set("chota", m->entries["min"]);

    m->set("max", Value(std::make_shared<NativeMethodCallable>("max", 1, [getNumbers](Interpreter &, const std::vector<Value> &args) {
        auto nums = getNumbers(args[0]);
        double mVal = nums[0];
        for (double n : nums) if (n > mVal) mVal = n;
        return Value(mVal);
    })));
    m->set("bada", m->entries["max"]);

    m->set("sum", Value(std::make_shared<NativeMethodCallable>("sum", 1, [getNumbers](Interpreter &, const std::vector<Value> &args) {
        auto nums = getNumbers(args[0]);
        double s = 0.0;
        for (double n : nums) s += n;
        return Value(s);
    })));
    m->set("jod", m->entries["sum"]);

    m->set("quantiles", Value(std::make_shared<NativeMethodCallable>("quantiles", 1, 2, [getNumbers](Interpreter &, const std::vector<Value> &args) {
        auto nums = getNumbers(args[0]);
        long long n = (args.size() > 1) ? args[1].deref().asInt() : 4;
        if (n < 2) throw std::runtime_error("n must be at least 2!");
        std::sort(nums.begin(), nums.end());
        auto list = std::make_shared<MemeList>();
        for (long long i = 1; i < n; ++i) {
            double idx = (static_cast<double>(i) * (nums.size() - 1)) / n;
            size_t low = static_cast<size_t>(std::floor(idx));
            size_t high = static_cast<size_t>(std::ceil(idx));
            double weight = idx - low;
            double val = nums[low] * (1.0 - weight) + nums[high] * weight;
            list->elements.push_back(Value(val));
        }
        return Value(list);
    })));

    return m;
}

static std::shared_ptr<MemeMap> createCryptoModule()
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    auto toStr = [](const Value &v) -> std::string {
        Value d = v.deref();
        if (d.isBytes()) {
            auto b = d.getBytes();
            return std::string(reinterpret_cast<const char*>(b->data.data()), b->data.size());
        }
        return d.toString();
    };

    m->set("sha256", Value(std::make_shared<NativeMethodCallable>("sha256", 1, [toStr](Interpreter &, const std::vector<Value> &args) {
        return Value(EcosystemBridge::Crypto::sha256(toStr(args[0])));
    })));

    m->set("hmac_sha256", Value(std::make_shared<NativeMethodCallable>("hmac_sha256", 2, [toStr](Interpreter &, const std::vector<Value> &args) {
        return Value(EcosystemBridge::Crypto::hmacSha256(toStr(args[0]), toStr(args[1])));
    })));
    m->set("hmac", m->entries["hmac_sha256"]);

    m->set("md5", Value(std::make_shared<NativeMethodCallable>("md5", 1, [toStr](Interpreter &, const std::vector<Value> &args) {
        return Value(EcosystemBridge::Crypto::md5(toStr(args[0])));
    })));

    m->set("sha1", Value(std::make_shared<NativeMethodCallable>("sha1", 1, [toStr](Interpreter &, const std::vector<Value> &args) {
        return Value(EcosystemBridge::Crypto::sha1(toStr(args[0])));
    })));

    m->set("base64_encode", Value(std::make_shared<NativeMethodCallable>("base64_encode", 1, [toStr](Interpreter &, const std::vector<Value> &args) {
        return Value(EcosystemBridge::Crypto::base64Encode(toStr(args[0])));
    })));
    m->set("b64encode", m->entries["base64_encode"]);

    m->set("base64_decode", Value(std::make_shared<NativeMethodCallable>("base64_decode", 1, [](Interpreter &, const std::vector<Value> &args) {
        return Value(EcosystemBridge::Crypto::base64Decode(args[0].deref().toString()));
    })));
    m->set("b64decode", m->entries["base64_decode"]);

    m->set("uuid", Value(std::make_shared<NativeMethodCallable>("uuid", 0, [](Interpreter &, const std::vector<Value> &) {
        return Value(EcosystemBridge::Crypto::uuid());
    })));
    m->set("uuid4", m->entries["uuid"]);

    return m;
}

static std::shared_ptr<MemeMap> createRegexModule()
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    m->set("test", Value(std::make_shared<NativeMethodCallable>("test", 2, [](Interpreter &, const std::vector<Value> &args) {
        std::string pattern = args[0].deref().toString();
        std::string text = args[1].deref().toString();
        try {
            std::regex re(pattern);
            return Value(std::regex_search(text, re));
        } catch (const std::exception &e) {
            throw std::runtime_error(std::string("Regex error: ") + e.what());
        }
    })));
    m->set("milta_hai", m->entries["test"]);

    m->set("search", Value(std::make_shared<NativeMethodCallable>("search", 2, [](Interpreter &, const std::vector<Value> &args) {
        std::string pattern = args[0].deref().toString();
        std::string text = args[1].deref().toString();
        try {
            std::regex re(pattern);
            std::smatch match;
            if (std::regex_search(text, match, re)) {
                auto res = std::make_shared<MemeMap>();
                res->set("match", Value(match.str()));
                res->set("start", Value(static_cast<long long>(match.position())));
                res->set("end", Value(static_cast<long long>(match.position() + match.length())));
                auto groups = std::make_shared<MemeList>();
                for (size_t i = 1; i < match.size(); ++i) {
                    groups->elements.push_back(Value(match[i].str()));
                }
                res->set("groups", Value(groups));
                return Value(res);
            }
            return Value(std::monostate{});
        } catch (const std::exception &e) {
            throw std::runtime_error(std::string("Regex error: ") + e.what());
        }
    })));
    m->set("dhund", m->entries["search"]);

    m->set("match", Value(std::make_shared<NativeMethodCallable>("match", 2, [](Interpreter &, const std::vector<Value> &args) {
        std::string pattern = args[0].deref().toString();
        std::string text = args[1].deref().toString();
        try {
            std::regex re(pattern);
            std::smatch match;
            if (std::regex_search(text, match, re) && match.position() == 0) {
                auto res = std::make_shared<MemeMap>();
                res->set("match", Value(match.str()));
                res->set("start", Value(0LL));
                res->set("end", Value(static_cast<long long>(match.length())));
                auto groups = std::make_shared<MemeList>();
                for (size_t i = 1; i < match.size(); ++i) {
                    groups->elements.push_back(Value(match[i].str()));
                }
                res->set("groups", Value(groups));
                return Value(res);
            }
            return Value(std::monostate{});
        } catch (const std::exception &e) {
            throw std::runtime_error(std::string("Regex error: ") + e.what());
        }
    })));

    m->set("find_all", Value(std::make_shared<NativeMethodCallable>("find_all", 2, [](Interpreter &, const std::vector<Value> &args) {
        std::string pattern = args[0].deref().toString();
        std::string text = args[1].deref().toString();
        try {
            std::regex re(pattern);
            auto list = std::make_shared<MemeList>();
            auto it = std::sregex_iterator(text.begin(), text.end(), re);
            auto end = std::sregex_iterator();
            for (; it != end; ++it) {
                list->elements.push_back(Value(it->str()));
            }
            return Value(list);
        } catch (const std::exception &e) {
            throw std::runtime_error(std::string("Regex error: ") + e.what());
        }
    })));
    m->set("sab_dhund", m->entries["find_all"]);

    m->set("replace", Value(std::make_shared<NativeMethodCallable>("replace", 3, [](Interpreter &, const std::vector<Value> &args) {
        std::string pattern = args[0].deref().toString();
        std::string repl = args[1].deref().toString();
        std::string text = args[2].deref().toString();
        try {
            std::regex re(pattern);
            return Value(std::regex_replace(text, re, repl));
        } catch (const std::exception &e) {
            throw std::runtime_error(std::string("Regex error: ") + e.what());
        }
    })));
    m->set("badal", m->entries["replace"]);

    m->set("split", Value(std::make_shared<NativeMethodCallable>("split", 2, [](Interpreter &, const std::vector<Value> &args) {
        std::string pattern = args[0].deref().toString();
        std::string text = args[1].deref().toString();
        try {
            std::regex re(pattern);
            auto list = std::make_shared<MemeList>();
            std::sregex_token_iterator it(text.begin(), text.end(), re, -1);
            std::sregex_token_iterator end;
            for (; it != end; ++it) {
                list->elements.push_back(Value(std::string(*it)));
            }
            return Value(list);
        } catch (const std::exception &e) {
            throw std::runtime_error(std::string("Regex error: ") + e.what());
        }
    })));
    m->set("baant", m->entries["split"]);

    return m;
}

static std::shared_ptr<MemeMap> createWaqtModule()
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    // now / ab (seconds as double)
    m->set("now", Value(std::make_shared<NativeMethodCallable>("now", 0, [](Interpreter &, const std::vector<Value> &) {
        auto now = std::chrono::system_clock::now();
        double sec = std::chrono::duration<double>(now.time_since_epoch()).count();
        return Value(sec);
    })));
    m->set("ab", m->entries["now"]);

    // now_ms (milliseconds as long long)
    m->set("now_ms", Value(std::make_shared<NativeMethodCallable>("now_ms", 0, [](Interpreter &, const std::vector<Value> &) {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        return Value(static_cast<long long>(ms));
    })));

    // iso([ts])
    m->set("iso", Value(std::make_shared<NativeMethodCallable>("iso", 0, 1, [](Interpreter &, const std::vector<Value> &args) {
        std::time_t t;
        if (args.empty()) {
            t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        } else {
            t = static_cast<std::time_t>(args[0].deref().asDouble());
        }
        std::tm tmBuf;
#if defined(_WIN32)
        gmtime_s(&tmBuf, &t);
#else
        gmtime_r(&t, &tmBuf);
#endif
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tmBuf);
        return Value(std::string(buf));
    })));

    // format([ts], fmt) / strftime
    m->set("format", Value(std::make_shared<NativeMethodCallable>("format", 1, 2, [](Interpreter &, const std::vector<Value> &args) {
        std::time_t t;
        std::string fmt;
        if (args.size() == 1) {
            t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            fmt = args[0].deref().toString();
        } else {
            t = static_cast<std::time_t>(args[0].deref().asDouble());
            fmt = args[1].deref().toString();
        }
        std::tm tmBuf;
#if defined(_WIN32)
        localtime_s(&tmBuf, &t);
#else
        localtime_r(&t, &tmBuf);
#endif
        char buf[128];
        std::strftime(buf, sizeof(buf), fmt.c_str(), &tmBuf);
        return Value(std::string(buf));
    })));
    m->set("strftime", m->entries["format"]);

    // parse(str, fmt) / strptime
    m->set("parse", Value(std::make_shared<NativeMethodCallable>("parse", 2, [](Interpreter &, const std::vector<Value> &args) {
        std::string dateStr = args[0].deref().toString();
        std::string fmt = args[1].deref().toString();
        std::tm tmBuf = {};
        std::istringstream ss(dateStr);
        ss >> std::get_time(&tmBuf, fmt.c_str());
        if (ss.fail()) throw std::runtime_error("Failed to parse date string: '" + dateStr + "' with format '" + fmt + "'");
        std::time_t t = std::mktime(&tmBuf);
        return Value(static_cast<double>(t));
    })));
    m->set("strptime", m->entries["parse"]);

    // date parts: year, month, day, hour, minute, second, weekday
    auto getTm = [](const std::vector<Value> &args) -> std::tm {
        std::time_t t;
        if (args.empty()) t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        else t = static_cast<std::time_t>(args[0].deref().asDouble());
        std::tm tmBuf;
#if defined(_WIN32)
        localtime_s(&tmBuf, &t);
#else
        localtime_r(&t, &tmBuf);
#endif
        return tmBuf;
    };

    m->set("year", Value(std::make_shared<NativeMethodCallable>("year", 0, 1, [getTm](Interpreter &, const std::vector<Value> &args) {
        return Value(static_cast<long long>(getTm(args).tm_year + 1900));
    })));
    m->set("month", Value(std::make_shared<NativeMethodCallable>("month", 0, 1, [getTm](Interpreter &, const std::vector<Value> &args) {
        return Value(static_cast<long long>(getTm(args).tm_mon + 1));
    })));
    m->set("day", Value(std::make_shared<NativeMethodCallable>("day", 0, 1, [getTm](Interpreter &, const std::vector<Value> &args) {
        return Value(static_cast<long long>(getTm(args).tm_mday));
    })));
    m->set("hour", Value(std::make_shared<NativeMethodCallable>("hour", 0, 1, [getTm](Interpreter &, const std::vector<Value> &args) {
        return Value(static_cast<long long>(getTm(args).tm_hour));
    })));
    m->set("minute", Value(std::make_shared<NativeMethodCallable>("minute", 0, 1, [getTm](Interpreter &, const std::vector<Value> &args) {
        return Value(static_cast<long long>(getTm(args).tm_min));
    })));
    m->set("second", Value(std::make_shared<NativeMethodCallable>("second", 0, 1, [getTm](Interpreter &, const std::vector<Value> &args) {
        return Value(static_cast<long long>(getTm(args).tm_sec));
    })));
    m->set("weekday", Value(std::make_shared<NativeMethodCallable>("weekday", 0, 1, [getTm](Interpreter &, const std::vector<Value> &args) {
        return Value(static_cast<long long>(getTm(args).tm_wday));
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
    m->set("sleep", Value(std::make_shared<NativeMethodCallable>("sleep", 1, sleepFn)));
    m->set("so_ja", m->entries["sleep"]);
    m->set("ruk", m->entries["sleep"]);

    return m;
}

static std::shared_ptr<MemeMap> createItertoolsModule()
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    auto rangeFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        if (args.empty()) throw std::runtime_error("range() requires at least 1 argument!");
        long long start = 0;
        long long stop = 0;
        long long step = 1;
        if (args.size() == 1) {
            stop = args[0].deref().asInt();
        } else if (args.size() == 2) {
            start = args[0].deref().asInt();
            stop = args[1].deref().asInt();
        } else {
            start = args[0].deref().asInt();
            stop = args[1].deref().asInt();
            step = args[2].deref().asInt();
            if (step == 0) throw std::runtime_error("range() step cannot be zero!");
        }
        auto list = std::make_shared<MemeList>();
        if (step > 0) {
            for (long long i = start; i < stop; i += step) {
                list->elements.push_back(Value(i));
            }
        } else {
            for (long long i = start; i > stop; i += step) {
                list->elements.push_back(Value(i));
            }
        }
        return Value(list);
    };
    m->set("range", Value(std::make_shared<NativeMethodCallable>("range", 1, 3, rangeFn)));
    m->set("ginti", m->entries["range"]);

    auto zipFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        if (args.empty()) return Value(std::make_shared<MemeList>());
        std::vector<std::vector<Value>> sequences;
        size_t minLen = std::numeric_limits<size_t>::max();
        for (const auto &arg : args) {
            Value d = arg.deref();
            std::vector<Value> items;
            if (d.isList()) items = d.getList()->elements;
            else if (d.isTuple()) items = d.getTuple()->elements;
            else if (d.isSet()) items = d.getSet()->elements;
            else if (std::holds_alternative<std::string>(d.data)) {
                for (char c : std::get<std::string>(d.data)) items.push_back(Value(std::string(1, c)));
            } else {
                throw std::runtime_error("zip() requires sequences (list, tuple, string, or set)!");
            }
            if (items.size() < minLen) minLen = items.size();
            sequences.push_back(std::move(items));
        }
        auto result = std::make_shared<MemeList>();
        for (size_t i = 0; i < minLen; ++i) {
            std::vector<Value> tupleItems;
            for (const auto &seq : sequences) {
                tupleItems.push_back(seq[i]);
            }
            result->elements.push_back(Value(std::make_shared<MemeTuple>(std::move(tupleItems))));
        }
        return Value(result);
    };
    m->set("zip", Value(std::make_shared<NativeMethodCallable>("zip", 1, -1, zipFn)));
    m->set("jodi_bana", m->entries["zip"]);

    auto enumFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        Value d = args[0].deref();
        long long start = (args.size() > 1) ? args[1].deref().asInt() : 0;
        std::vector<Value> items;
        if (d.isList()) items = d.getList()->elements;
        else if (d.isTuple()) items = d.getTuple()->elements;
        else if (std::holds_alternative<std::string>(d.data)) {
            for (char c : std::get<std::string>(d.data)) items.push_back(Value(std::string(1, c)));
        } else {
            throw std::runtime_error("enumerate() requires a list, tuple, or string!");
        }
        auto result = std::make_shared<MemeList>();
        for (size_t i = 0; i < items.size(); ++i) {
            std::vector<Value> tupleItems = { Value(start + static_cast<long long>(i)), items[i] };
            result->elements.push_back(Value(std::make_shared<MemeTuple>(std::move(tupleItems))));
        }
        return Value(result);
    };
    m->set("enumerate", Value(std::make_shared<NativeMethodCallable>("enumerate", 1, 2, enumFn)));
    m->set("ginti_kar", m->entries["enumerate"]);

    m->set("chunk", Value(std::make_shared<NativeMethodCallable>("chunk", 2, [](Interpreter &, const std::vector<Value> &args) {
        Value d = args[0].deref();
        long long sz = args[1].deref().asInt();
        if (sz <= 0) throw std::runtime_error("chunk size must be positive!");
        std::vector<Value> items;
        if (d.isList()) items = d.getList()->elements;
        else if (d.isTuple()) items = d.getTuple()->elements;
        else throw std::runtime_error("chunk() requires a list or tuple!");
        auto result = std::make_shared<MemeList>();
        for (size_t i = 0; i < items.size(); i += sz) {
            size_t endIdx = std::min(items.size(), i + static_cast<size_t>(sz));
            std::vector<Value> chunkItems(items.begin() + i, items.begin() + endIdx);
            result->elements.push_back(Value(std::make_shared<MemeList>(std::move(chunkItems))));
        }
        return Value(result);
    })));
    m->set("tukde", m->entries["chunk"]);

    m->set("flatten", Value(std::make_shared<NativeMethodCallable>("flatten", 1, [](Interpreter &, const std::vector<Value> &args) {
        Value d = args[0].deref();
        if (!d.isList()) throw std::runtime_error("flatten() requires a list!");
        auto out = std::make_shared<MemeList>();
        std::function<void(const Value&)> flat = [&](const Value &v) {
            Value derefVal = v.deref();
            if (derefVal.isList()) {
                for (const auto &e : derefVal.getList()->elements) flat(e);
            } else {
                out->elements.push_back(derefVal);
            }
        };
        flat(d);
        return Value(out);
    })));
    m->set("seedha_kar", m->entries["flatten"]);

    m->set("repeat", Value(std::make_shared<NativeMethodCallable>("repeat", 2, [](Interpreter &, const std::vector<Value> &args) {
        Value elem = args[0].deref();
        long long times = args[1].deref().asInt();
        if (times < 0) times = 0;
        auto list = std::make_shared<MemeList>();
        list->elements.resize(static_cast<size_t>(times), elem);
        return Value(list);
    })));

    return m;
}

static std::shared_ptr<MemeMap> createCollectionsModule()
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    // Counter(seq) -> returns map of counts
    m->set("Counter", Value(std::make_shared<NativeMethodCallable>("Counter", 1, [](Interpreter &, const std::vector<Value> &args) {
        Value d = args[0].deref();
        auto map = std::make_shared<MemeMap>();
        if (d.isList()) {
            for (const auto &item : d.getList()->elements) {
                std::string k = item.deref().toString();
                long long cur = map->has(k) ? map->entries[k].deref().asInt() : 0LL;
                map->set(k, Value(cur + 1LL));
            }
        } else if (d.isTuple()) {
            for (const auto &item : d.getTuple()->elements) {
                std::string k = item.deref().toString();
                long long cur = map->has(k) ? map->entries[k].deref().asInt() : 0LL;
                map->set(k, Value(cur + 1LL));
            }
        } else if (std::holds_alternative<std::string>(d.data)) {
            for (char c : std::get<std::string>(d.data)) {
                std::string k(1, c);
                long long cur = map->has(k) ? map->entries[k].deref().asInt() : 0LL;
                map->set(k, Value(cur + 1LL));
            }
        } else {
            throw std::runtime_error("Counter() requires a list, tuple, or string!");
        }
        return Value(map);
    })));
    m->set("ginti", m->entries["Counter"]);

    // deque([init]) -> double-ended queue object
    m->set("deque", Value(std::make_shared<NativeMethodCallable>("deque", 0, 1, [](Interpreter &, const std::vector<Value> &args) {
        auto dq = std::make_shared<std::deque<Value>>();
        if (!args.empty()) {
            Value d = args[0].deref();
            if (d.isList()) {
                for (const auto &e : d.getList()->elements) dq->push_back(e);
            }
        }
        auto dqMap = std::make_shared<MemeMap>();
        dqMap->set("push_back", Value(std::make_shared<NativeMethodCallable>("push_back", 1, [dq](Interpreter &, const std::vector<Value> &a) {
            dq->push_back(a[0].deref());
            return Value(std::monostate{});
        })));
        dqMap->set("push_right", dqMap->entries["push_back"]);
        dqMap->set("push_front", Value(std::make_shared<NativeMethodCallable>("push_front", 1, [dq](Interpreter &, const std::vector<Value> &a) {
            dq->push_front(a[0].deref());
            return Value(std::monostate{});
        })));
        dqMap->set("push_left", dqMap->entries["push_front"]);

        dqMap->set("pop_back", Value(std::make_shared<NativeMethodCallable>("pop_back", 0, [dq](Interpreter &, const std::vector<Value> &) {
            if (dq->empty()) throw std::runtime_error("pop_back() on empty deque!");
            Value v = dq->back();
            dq->pop_back();
            return v;
        })));
        dqMap->set("pop_right", dqMap->entries["pop_back"]);

        dqMap->set("pop_front", Value(std::make_shared<NativeMethodCallable>("pop_front", 0, [dq](Interpreter &, const std::vector<Value> &) {
            if (dq->empty()) throw std::runtime_error("pop_front() on empty deque!");
            Value v = dq->front();
            dq->pop_front();
            return v;
        })));
        dqMap->set("pop_left", dqMap->entries["pop_front"]);

        dqMap->set("size", Value(std::make_shared<NativeMethodCallable>("size", 0, [dq](Interpreter &, const std::vector<Value> &) {
            return Value(static_cast<long long>(dq->size()));
        })));
        dqMap->set("length", dqMap->entries["size"]);

        dqMap->set("to_list", Value(std::make_shared<NativeMethodCallable>("to_list", 0, [dq](Interpreter &, const std::vector<Value> &) {
            auto l = std::make_shared<MemeList>();
            for (const auto &item : *dq) l->elements.push_back(item);
            return Value(l);
        })));

        return Value(dqMap);
    })));

    return m;
}

static std::shared_ptr<MemeMap> createTestModule()
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    m->set("assert", Value(std::make_shared<NativeMethodCallable>("assert", 1, 2, [](Interpreter &, const std::vector<Value> &args) {
        Value cond = args[0].deref();
        if (!cond.isTruthy()) {
            std::string msg = (args.size() > 1) ? args[1].deref().toString() : "Assertion failed!";
            throw std::runtime_error(msg);
        }
        return Value(true);
    })));

    m->set("assert_eq", Value(std::make_shared<NativeMethodCallable>("assert_eq", 2, 3, [](Interpreter &, const std::vector<Value> &args) {
        Value a = args[0].deref();
        Value b = args[1].deref();
        if (!(a == b)) {
            std::string msg = (args.size() > 2) ? args[2].deref().toString() : "Assertion error";
            throw std::runtime_error(msg + ": Expected '" + b.toString() + "' but got '" + a.toString() + "'");
        }
        return Value(true);
    })));

    m->set("assert_ne", Value(std::make_shared<NativeMethodCallable>("assert_ne", 2, 3, [](Interpreter &, const std::vector<Value> &args) {
        Value a = args[0].deref();
        Value b = args[1].deref();
        if (a == b) {
            std::string msg = (args.size() > 2) ? args[2].deref().toString() : "Assertion error";
            throw std::runtime_error(msg + ": Values should not be equal: '" + a.toString() + "'");
        }
        return Value(true);
    })));

    m->set("assert_throws", Value(std::make_shared<NativeMethodCallable>("assert_throws", 1, 2, [](Interpreter &interp, const std::vector<Value> &args) {
        Value fnVal = args[0].deref();
        if (!std::holds_alternative<std::shared_ptr<Callable>>(fnVal.data)) {
            throw std::runtime_error("assert_throws requires a callable function!");
        }
        auto callable = std::get<std::shared_ptr<Callable>>(fnVal.data);
        bool threw = false;
        try {
            callable->call(interp, {});
        } catch (...) {
            threw = true;
        }
        if (!threw) {
            std::string msg = (args.size() > 1) ? args[1].deref().toString() : "Expected function to throw error, but it succeeded!";
            throw std::runtime_error(msg);
        }
        return Value(true);
    })));

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

    // padh_bytes / read_bytes
    auto readBytesFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string path = args[0].deref().toString();
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f.is_open())
        {
            throw std::runtime_error("File gayab ho gayi! Path: '" + path + "'");
        }
        std::streamsize size = f.tellg();
        f.seekg(0, std::ios::beg);
        auto bytes = std::make_shared<MemeBytes>();
        if (size > 0)
        {
            bytes->data.resize(static_cast<size_t>(size));
            if (!f.read(reinterpret_cast<char*>(bytes->data.data()), size))
            {
                throw std::runtime_error("Binary file padhne mein lafda! Path: '" + path + "'");
            }
        }
        return Value(bytes);
    };
    m->set("padh_bytes", Value(std::make_shared<NativeMethodCallable>("padh_bytes", 1, readBytesFn)));
    m->set("read_bytes", m->entries["padh_bytes"]);

    // likh_bytes / write_bytes
    auto writeBytesFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string path = args[0].deref().toString();
        Value dataVal = args[1].deref();
        std::ofstream f(path, std::ios::binary | std::ios::trunc);
        if (!f.is_open())
        {
            throw std::runtime_error("File kholne mein lafda ho gaya! Path: '" + path + "'");
        }
        if (dataVal.isBytes())
        {
            auto b = dataVal.getBytes();
            if (!b->data.empty())
            {
                f.write(reinterpret_cast<const char*>(b->data.data()), b->data.size());
            }
        }
        else if (dataVal.isList())
        {
            auto l = dataVal.getList();
            std::vector<uint8_t> buf;
            buf.reserve(l->elements.size());
            for (const auto &elem : l->elements)
            {
                buf.push_back(static_cast<uint8_t>(elem.deref().asInt()));
            }
            if (!buf.empty())
            {
                f.write(reinterpret_cast<const char*>(buf.data()), buf.size());
            }
        }
        else if (std::holds_alternative<std::string>(dataVal.data))
        {
            const std::string &s = std::get<std::string>(dataVal.data);
            if (!s.empty())
            {
                f.write(s.data(), s.size());
            }
        }
        else
        {
            throw std::runtime_error("likh_bytes ko bytes ya list chahiye!");
        }
        return Value(true);
    };
    m->set("likh_bytes", Value(std::make_shared<NativeMethodCallable>("likh_bytes", 2, writeBytesFn)));
    m->set("write_bytes", m->entries["likh_bytes"]);

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

static std::shared_ptr<MemeMap> createCsvModule()
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    // parse(text, has_header = true)
    m->set("parse", Value(std::make_shared<NativeMethodCallable>("parse", 1, 2, [](Interpreter &, const std::vector<Value> &args) {
        std::string text = args[0].deref().toString();
        bool hasHeader = (args.size() > 1) ? args[1].deref().isTruthy() : true;
        std::stringstream ss(text);
        std::string line;
        auto list = std::make_shared<MemeList>();
        std::vector<std::string> headers;
        bool isFirst = true;

        while (std::getline(ss, line))
        {
            if (line.empty()) continue;
            auto row = parseCsvLine(line);
            if (hasHeader && isFirst)
            {
                headers = row;
                isFirst = false;
                continue;
            }
            if (hasHeader)
            {
                auto rowMap = std::make_shared<MemeMap>();
                for (size_t i = 0; i < headers.size() && i < row.size(); ++i)
                {
                    rowMap->set(headers[i], Value(row[i]));
                }
                list->elements.push_back(Value(rowMap));
            }
            else
            {
                auto rowList = std::make_shared<MemeList>();
                for (const auto &cell : row) rowList->elements.push_back(Value(cell));
                list->elements.push_back(Value(rowList));
            }
        }
        return Value(list);
    })));
    m->set("padh", m->entries["parse"]);

    // stringify(rows, [headers])
    m->set("stringify", Value(std::make_shared<NativeMethodCallable>("stringify", 1, 2, [](Interpreter &, const std::vector<Value> &args) {
        Value rowsVal = args[0].deref();
        if (!rowsVal.isList()) throw std::runtime_error("csv.stringify ko list chahiye!");
        auto list = rowsVal.getList();
        std::ostringstream ss;
        if (list->elements.empty()) return Value("");

        std::vector<std::string> headers;
        if (args.size() > 1 && args[1].deref().isList()) {
            for (const auto &h : args[1].deref().getList()->elements) headers.push_back(h.deref().toString());
        }

        Value first = list->elements[0].deref();
        if (first.isMap()) {
            auto map0 = first.getMap();
            if (headers.empty()) headers = map0->keyOrder;
            for (size_t i = 0; i < headers.size(); ++i) {
                if (i > 0) ss << ",";
                ss << escapeCsvField(headers[i]);
            }
            ss << "\n";
            for (const auto &rowElem : list->elements) {
                Value rv = rowElem.deref();
                if (rv.isMap()) {
                    auto rm = rv.getMap();
                    for (size_t i = 0; i < headers.size(); ++i) {
                        if (i > 0) ss << ",";
                        std::string cell = rm->has(headers[i]) ? rm->entries[headers[i]].deref().toString() : "";
                        ss << escapeCsvField(cell);
                    }
                    ss << "\n";
                }
            }
        } else if (first.isList()) {
            if (!headers.empty()) {
                for (size_t i = 0; i < headers.size(); ++i) {
                    if (i > 0) ss << ",";
                    ss << escapeCsvField(headers[i]);
                }
                ss << "\n";
            }
            for (const auto &rowElem : list->elements) {
                Value rv = rowElem.deref();
                if (rv.isList()) {
                    auto rl = rv.getList();
                    for (size_t i = 0; i < rl->elements.size(); ++i) {
                        if (i > 0) ss << ",";
                        ss << escapeCsvField(rl->elements[i].deref().toString());
                    }
                    ss << "\n";
                }
            }
        }
        return Value(ss.str());
    })));
    m->set("likh", m->entries["stringify"]);

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

    auto formatArgs = [&interpreter](const std::vector<Value> &args, const std::string &sep = " ", bool rich = true) -> std::string {
        std::string out;
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) out += sep;
            std::string s = interpreter.stringify(args[i]);
            out += rich ? formatRichText(s) : s;
        }
        return out;
    };

    auto makeColorFn = [&interpreter, formatArgs](const std::string &ansiCode) {
        return [&interpreter, formatArgs, ansiCode](Interpreter &, const std::vector<Value> &args) -> Value {
            std::string text = formatArgs(args, " ", true);
            std::string res = ansiCode + text + "\033[0m";
            interpreter.getOutputStream() << res << "\n";
            return Value(res);
        };
    };

    auto makeBadgeFn = [&interpreter, formatArgs](const std::string &badge) {
        return [&interpreter, formatArgs, badge](Interpreter &, const std::vector<Value> &args) -> Value {
            std::string text = formatArgs(args, " ", true);
            std::string res = badge + (text.empty() ? "" : (" " + text));
            interpreter.getOutputStream() << res << "\n";
            return Value(res);
        };
    };

    auto lineFn = [&interpreter](Interpreter &, const std::vector<Value> &args) -> Value {
        int length = 50;
        std::string ch = "-";
        if (!args.empty()) {
            Value a0 = args[0].deref();
            if (a0.isNumeric()) length = std::max(0, a0.asInt32());
            else ch = a0.toString();
        }
        if (args.size() > 1) {
            ch = args[1].deref().toString();
        }
        if (ch.empty()) ch = "-";
        std::string res;
        while (static_cast<int>(res.size()) < length) {
            res += ch;
        }
        if (static_cast<int>(res.size()) > length) {
            res = res.substr(0, length);
        }
        interpreter.getOutputStream() << res << "\n";
        return Value(res);
    };

    auto rawFn = [&interpreter, formatArgs](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string res = formatArgs(args, " ", false);
        interpreter.getOutputStream() << res << "\n";
        return Value(res);
    };

    auto binaLineFn = [&interpreter, formatArgs](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string res = formatArgs(args, " ", true);
        interpreter.getOutputStream() << res;
        return Value(res);
    };

    m->set("mez", Value(std::make_shared<NativeMethodCallable>("mez", 1, tableFn)));
    m->set("table", Value(std::make_shared<NativeMethodCallable>("table", 1, tableFn)));
    m->set("dabba", Value(std::make_shared<NativeMethodCallable>("dabba", 1, 2, boxFn)));
    m->set("box", Value(std::make_shared<NativeMethodCallable>("box", 1, 2, boxFn)));
    m->set("saaf", Value(std::make_shared<NativeMethodCallable>("saaf", 0, clearFn)));
    m->set("clear", Value(std::make_shared<NativeMethodCallable>("clear", 0, clearFn)));

    // Color helpers
    m->set("laal", Value(std::make_shared<NativeMethodCallable>("laal", 0, -1, makeColorFn("\033[31m"))));
    m->set("red", Value(std::make_shared<NativeMethodCallable>("red", 0, -1, makeColorFn("\033[31m"))));
    m->set("hara", Value(std::make_shared<NativeMethodCallable>("hara", 0, -1, makeColorFn("\033[32m"))));
    m->set("green", Value(std::make_shared<NativeMethodCallable>("green", 0, -1, makeColorFn("\033[32m"))));
    m->set("peela", Value(std::make_shared<NativeMethodCallable>("peela", 0, -1, makeColorFn("\033[33m"))));
    m->set("yellow", Value(std::make_shared<NativeMethodCallable>("yellow", 0, -1, makeColorFn("\033[33m"))));
    m->set("neela", Value(std::make_shared<NativeMethodCallable>("neela", 0, -1, makeColorFn("\033[34m"))));
    m->set("blue", Value(std::make_shared<NativeMethodCallable>("blue", 0, -1, makeColorFn("\033[34m"))));
    m->set("gulabi", Value(std::make_shared<NativeMethodCallable>("gulabi", 0, -1, makeColorFn("\033[35m"))));
    m->set("pink", Value(std::make_shared<NativeMethodCallable>("pink", 0, -1, makeColorFn("\033[35m"))));
    m->set("magenta", Value(std::make_shared<NativeMethodCallable>("magenta", 0, -1, makeColorFn("\033[35m"))));
    m->set("asmani", Value(std::make_shared<NativeMethodCallable>("asmani", 0, -1, makeColorFn("\033[36m"))));
    m->set("cyan", Value(std::make_shared<NativeMethodCallable>("cyan", 0, -1, makeColorFn("\033[36m"))));
    m->set("safed", Value(std::make_shared<NativeMethodCallable>("safed", 0, -1, makeColorFn("\033[37m"))));
    m->set("white", Value(std::make_shared<NativeMethodCallable>("white", 0, -1, makeColorFn("\033[37m"))));
    m->set("kala", Value(std::make_shared<NativeMethodCallable>("kala", 0, -1, makeColorFn("\033[30m"))));
    m->set("black", Value(std::make_shared<NativeMethodCallable>("black", 0, -1, makeColorFn("\033[30m"))));
    m->set("gray", Value(std::make_shared<NativeMethodCallable>("gray", 0, -1, makeColorFn("\033[90m"))));
    m->set("grey", Value(std::make_shared<NativeMethodCallable>("grey", 0, -1, makeColorFn("\033[90m"))));
    m->set("orange", Value(std::make_shared<NativeMethodCallable>("orange", 0, -1, makeColorFn("\033[38;5;208m"))));
    m->set("purple", Value(std::make_shared<NativeMethodCallable>("purple", 0, -1, makeColorFn("\033[38;5;129m"))));
    m->set("jamuni", Value(std::make_shared<NativeMethodCallable>("jamuni", 0, -1, makeColorFn("\033[38;5;129m"))));

    // Status badges
    m->set("success", Value(std::make_shared<NativeMethodCallable>("success", 0, -1, makeBadgeFn("\033[1;32m[SUCCESS]\033[0m"))));
    m->set("theek", Value(std::make_shared<NativeMethodCallable>("theek", 0, -1, makeBadgeFn("\033[1;32m[SUCCESS]\033[0m"))));
    m->set("error", Value(std::make_shared<NativeMethodCallable>("error", 0, -1, makeBadgeFn("\033[1;31m[ERROR]\033[0m"))));
    m->set("galat", Value(std::make_shared<NativeMethodCallable>("galat", 0, -1, makeBadgeFn("\033[1;31m[ERROR]\033[0m"))));
    m->set("gadbad", Value(std::make_shared<NativeMethodCallable>("gadbad", 0, -1, makeBadgeFn("\033[1;31m[ERROR]\033[0m"))));
    m->set("warn", Value(std::make_shared<NativeMethodCallable>("warn", 0, -1, makeBadgeFn("\033[1;33m[WARN]\033[0m"))));
    m->set("warning", Value(std::make_shared<NativeMethodCallable>("warning", 0, -1, makeBadgeFn("\033[1;33m[WARN]\033[0m"))));
    m->set("chetavni", Value(std::make_shared<NativeMethodCallable>("chetavni", 0, -1, makeBadgeFn("\033[1;33m[WARN]\033[0m"))));
    m->set("info", Value(std::make_shared<NativeMethodCallable>("info", 0, -1, makeBadgeFn("\033[1;36m[INFO]\033[0m"))));
    m->set("suchna", Value(std::make_shared<NativeMethodCallable>("suchna", 0, -1, makeBadgeFn("\033[1;36m[INFO]\033[0m"))));
    m->set("jankari", Value(std::make_shared<NativeMethodCallable>("jankari", 0, -1, makeBadgeFn("\033[1;36m[INFO]\033[0m"))));

    // Utilities
    m->set("rekha", Value(std::make_shared<NativeMethodCallable>("rekha", 0, 2, lineFn)));
    m->set("line", Value(std::make_shared<NativeMethodCallable>("line", 0, 2, lineFn)));
    m->set("raw", Value(std::make_shared<NativeMethodCallable>("raw", 0, -1, rawFn)));
    m->set("bina_line", Value(std::make_shared<NativeMethodCallable>("bina_line", 0, -1, binaLineFn)));
    m->set("no_newline", Value(std::make_shared<NativeMethodCallable>("no_newline", 0, -1, binaLineFn)));

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

static std::string urlDecode(const std::string &src)
{
    std::string ret;
    for (size_t i = 0; i < src.length(); i++)
    {
        if (src[i] == '%')
        {
            if (i + 2 < src.length())
            {
                int val = 0;
                if (sscanf(src.substr(i + 1, 2).c_str(), "%x", &val) == 1)
                {
                    ret += static_cast<char>(val);
                    i += 2;
                    continue;
                }
            }
            ret += '%';
        }
        else if (src[i] == '+')
        {
            ret += ' ';
        }
        else
        {
            ret += src[i];
        }
    }
    return ret;
}

static Value parseUrlEncoded(const std::string &body)
{
    auto map = std::make_shared<MemeMap>();
    std::stringstream ss(body);
    std::string pair;
    while (std::getline(ss, pair, '&'))
    {
        if (pair.empty()) continue;
        size_t eq = pair.find('=');
        if (eq != std::string::npos)
        {
            std::string k = urlDecode(pair.substr(0, eq));
            std::string v = urlDecode(pair.substr(eq + 1));
            map->set(k, Value(v));
        }
        else
        {
            map->set(urlDecode(pair), Value(true));
        }
    }
    return Value(map);
}

static std::shared_ptr<MemeMap> parseCookieHeader(const std::string &cookieHeader)
{
    auto map = std::make_shared<MemeMap>();
    std::stringstream ss(cookieHeader);
    std::string item;
    while (std::getline(ss, item, ';'))
    {
        while (!item.empty() && (item.front() == ' ' || item.front() == '\t')) item.erase(item.begin());
        while (!item.empty() && (item.back() == ' ' || item.back() == '\t')) item.pop_back();
        if (item.empty()) continue;
        size_t eq = item.find('=');
        if (eq != std::string::npos)
        {
            map->set(item.substr(0, eq), Value(item.substr(eq + 1)));
        }
        else
        {
            map->set(item, Value(true));
        }
    }
    return map;
}

static std::string getMimeType(const std::string &path)
{
    size_t dot = path.rfind('.');
    if (dot == std::string::npos) return "application/octet-stream";
    std::string ext = path.substr(dot);
    for (char &c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (ext == ".html" || ext == ".htm") return "text/html; charset=utf-8";
    if (ext == ".css") return "text/css; charset=utf-8";
    if (ext == ".js" || ext == ".mjs") return "application/javascript; charset=utf-8";
    if (ext == ".json") return "application/json; charset=utf-8";
    if (ext == ".png") return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".gif") return "image/gif";
    if (ext == ".svg") return "image/svg+xml";
    if (ext == ".ico") return "image/x-icon";
    if (ext == ".webp") return "image/webp";
    if (ext == ".txt") return "text/plain; charset=utf-8";
    if (ext == ".pdf") return "application/pdf";
    if (ext == ".wasm") return "application/wasm";
    return "application/octet-stream";
}

// ---------------------------------------------------------------------------
// Elysia / TypeBox-inspired Schema System (`t` / `schema` / `nirmaan`)
// ---------------------------------------------------------------------------

struct MemeSchema {
    std::string kind; // "string", "number", "integer", "boolean", "array", "object", "optional", "union", "literal", "record", "any"
    std::shared_ptr<MemeSchema> inner;
    std::vector<std::shared_ptr<MemeSchema>> unionSchemas;
    std::map<std::string, std::shared_ptr<MemeSchema>> properties;
    Value literalVal;

    bool hasMin = false;
    double minVal = 0.0;
    bool hasMax = false;
    double maxVal = 0.0;

    bool hasMinLength = false;
    size_t minLength = 0;
    bool hasMaxLength = false;
    size_t maxLength = 0;

    std::string pattern;
    std::string format;
    bool additionalProperties = true;

    struct Result {
        bool success = true;
        Value data;
        std::vector<std::string> errors;
    };

    Result validate(const Value &input, const std::string &path = "") const
    {
        Value d = input.deref();

        if (kind == "optional")
        {
            if (d.isNull() || std::holds_alternative<std::monostate>(d.data))
            {
                return { true, Value(std::monostate{}), {} };
            }
            return inner ? inner->validate(d, path) : Result{ true, d, {} };
        }

        if (kind == "any")
        {
            return { true, d, {} };
        }

        if (kind == "literal")
        {
            if (d == literalVal)
            {
                return { true, d, {} };
            }
            std::string p = path.empty() ? "value" : path;
            return { false, d, { p + ": Expected literal '" + literalVal.toString() + "', got '" + d.toString() + "'" } };
        }

        if (kind == "union")
        {
            for (const auto &s : unionSchemas)
            {
                auto r = s->validate(d, path);
                if (r.success) return r;
            }
            std::string p = path.empty() ? "value" : path;
            return { false, d, { p + ": Value does not match any union variant" } };
        }

        if (kind == "string")
        {
            std::string str;
            if (std::holds_alternative<std::string>(d.data))
            {
                str = std::get<std::string>(d.data);
            }
            else
            {
                std::string p = path.empty() ? "value" : path;
                return { false, d, { p + ": Expected string, got " + d.typeName() } };
            }
            std::string p = path.empty() ? "string" : path;
            if (hasMinLength && str.size() < minLength)
            {
                return { false, d, { p + ": Length " + std::to_string(str.size()) + " is less than minLength " + std::to_string(minLength) } };
            }
            if (hasMaxLength && str.size() > maxLength)
            {
                return { false, d, { p + ": Length " + std::to_string(str.size()) + " is greater than maxLength " + std::to_string(maxLength) } };
            }
            if (!pattern.empty())
            {
                try {
                    std::regex re(pattern);
                    if (!std::regex_search(str, re))
                    {
                        return { false, d, { p + ": Value does not match regex pattern " + pattern } };
                    }
                } catch (...) {}
            }
            if (format == "email")
            {
                size_t at = str.find('@');
                size_t dot = str.rfind('.');
                if (at == std::string::npos || dot == std::string::npos || dot < at)
                {
                    return { false, d, { p + ": Expected valid email address" } };
                }
            }
            else if (format == "uuid")
            {
                if (str.size() != 36)
                {
                    return { false, d, { p + ": Expected 36-character UUID" } };
                }
            }
            else if (format == "url")
            {
                if (str.rfind("http://", 0) != 0 && str.rfind("https://", 0) != 0)
                {
                    return { false, d, { p + ": Expected valid URL (http:// or https://)" } };
                }
            }
            return { true, Value(str), {} };
        }

        if (kind == "number")
        {
            double num = 0.0;
            if (d.isNumeric())
            {
                num = d.asDouble();
            }
            else if (std::holds_alternative<std::string>(d.data))
            {
                try {
                    num = std::stod(std::get<std::string>(d.data));
                } catch (...) {
                    std::string p = path.empty() ? "value" : path;
                    return { false, d, { p + ": Cannot parse string as number: '" + std::get<std::string>(d.data) + "'" } };
                }
            }
            else
            {
                std::string p = path.empty() ? "value" : path;
                return { false, d, { p + ": Expected number, got " + d.typeName() } };
            }
            std::string p = path.empty() ? "number" : path;
            if (hasMin && num < minVal)
            {
                return { false, d, { p + ": Value " + std::to_string(num) + " is less than minimum " + std::to_string(minVal) } };
            }
            if (hasMax && num > maxVal)
            {
                return { false, d, { p + ": Value " + std::to_string(num) + " is greater than maximum " + std::to_string(maxVal) } };
            }
            return { true, Value(num), {} };
        }

        if (kind == "integer")
        {
            long long iVal = 0;
            if (d.isInteger())
            {
                iVal = d.asInt();
            }
            else if (d.isNumeric())
            {
                double fVal = d.asDouble();
                if (std::floor(fVal) != fVal)
                {
                    std::string p = path.empty() ? "value" : path;
                    return { false, d, { p + ": Expected integer, got floating point " + std::to_string(fVal) } };
                }
                iVal = static_cast<long long>(fVal);
            }
            else if (std::holds_alternative<std::string>(d.data))
            {
                try {
                    iVal = std::stoll(std::get<std::string>(d.data));
                } catch (...) {
                    std::string p = path.empty() ? "value" : path;
                    return { false, d, { p + ": Cannot parse string as integer: '" + std::get<std::string>(d.data) + "'" } };
                }
            }
            else
            {
                std::string p = path.empty() ? "value" : path;
                return { false, d, { p + ": Expected integer, got " + d.typeName() } };
            }
            std::string p = path.empty() ? "integer" : path;
            if (hasMin && iVal < static_cast<long long>(minVal))
            {
                return { false, d, { p + ": Value " + std::to_string(iVal) + " is less than minimum " + std::to_string(static_cast<long long>(minVal)) } };
            }
            if (hasMax && iVal > static_cast<long long>(maxVal))
            {
                return { false, d, { p + ": Value " + std::to_string(iVal) + " is greater than maximum " + std::to_string(static_cast<long long>(maxVal)) } };
            }
            return { true, Value(iVal), {} };
        }

        if (kind == "boolean")
        {
            if (std::holds_alternative<bool>(d.data))
            {
                return { true, d, {} };
            }
            if (std::holds_alternative<std::string>(d.data))
            {
                std::string s = std::get<std::string>(d.data);
                for (char &c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                if (s == "true" || s == "1" || s == "han" || s == "sach") return { true, Value(true), {} };
                if (s == "false" || s == "0" || s == "nhi" || s == "jhooth") return { true, Value(false), {} };
            }
            std::string p = path.empty() ? "value" : path;
            return { false, d, { p + ": Expected boolean, got " + d.typeName() } };
        }

        if (kind == "array")
        {
            if (!d.isList())
            {
                std::string p = path.empty() ? "value" : path;
                return { false, d, { p + ": Expected list, got " + d.typeName() } };
            }
            auto list = d.getList();
            std::string p = path.empty() ? "array" : path;
            if (hasMin && list->elements.size() < static_cast<size_t>(minVal))
            {
                return { false, d, { p + ": Array size " + std::to_string(list->elements.size()) + " is less than minItems " + std::to_string(static_cast<size_t>(minVal)) } };
            }
            if (hasMax && list->elements.size() > static_cast<size_t>(maxVal))
            {
                return { false, d, { p + ": Array size " + std::to_string(list->elements.size()) + " is greater than maxItems " + std::to_string(static_cast<size_t>(maxVal)) } };
            }
            auto outList = std::make_shared<MemeList>();
            std::vector<std::string> errs;
            for (size_t i = 0; i < list->elements.size(); ++i)
            {
                std::string elemPath = path + "[" + std::to_string(i) + "]";
                if (inner)
                {
                    auto itemRes = inner->validate(list->elements[i], elemPath);
                    if (!itemRes.success)
                    {
                        errs.insert(errs.end(), itemRes.errors.begin(), itemRes.errors.end());
                    }
                    else
                    {
                        outList->elements.push_back(itemRes.data);
                    }
                }
                else
                {
                    outList->elements.push_back(list->elements[i]);
                }
            }
            if (!errs.empty())
            {
                return { false, d, errs };
            }
            return { true, Value(outList), {} };
        }

        if (kind == "object")
        {
            if (!d.isMap())
            {
                std::string p = path.empty() ? "value" : path;
                return { false, d, { p + ": Expected object, got " + d.typeName() } };
            }
            auto map = d.getMap();
            auto outMap = std::make_shared<MemeMap>();
            std::vector<std::string> errs;

            for (const auto &[propKey, propSchema] : properties)
            {
                std::string propPath = path.empty() ? propKey : path + "." + propKey;
                if (!map->has(propKey))
                {
                    if (propSchema->kind == "optional")
                    {
                        continue;
                    }
                    errs.push_back(propPath + ": Missing required property");
                    continue;
                }
                auto propRes = propSchema->validate(map->entries[propKey], propPath);
                if (!propRes.success)
                {
                    errs.insert(errs.end(), propRes.errors.begin(), propRes.errors.end());
                }
                else
                {
                    outMap->set(propKey, propRes.data);
                }
            }

            if (additionalProperties)
            {
                for (const auto &k : map->keyOrder)
                {
                    if (properties.find(k) == properties.end())
                    {
                        outMap->set(k, map->entries[k]);
                    }
                }
            }

            if (!errs.empty())
            {
                return { false, d, errs };
            }
            return { true, Value(outMap), {} };
        }

        if (kind == "record")
        {
            if (!d.isMap())
            {
                std::string p = path.empty() ? "value" : path;
                return { false, d, { p + ": Expected object record, got " + d.typeName() } };
            }
            auto map = d.getMap();
            auto outMap = std::make_shared<MemeMap>();
            std::vector<std::string> errs;
            for (const auto &k : map->keyOrder)
            {
                std::string propPath = path.empty() ? k : path + "." + k;
                if (inner)
                {
                    auto itemRes = inner->validate(map->entries[k], propPath);
                    if (!itemRes.success)
                    {
                        errs.insert(errs.end(), itemRes.errors.begin(), itemRes.errors.end());
                    }
                    else
                    {
                        outMap->set(k, itemRes.data);
                    }
                }
                else
                {
                    outMap->set(k, map->entries[k]);
                }
            }
            if (!errs.empty()) return { false, d, errs };
            return { true, Value(outMap), {} };
        }

        return { true, d, {} };
    }
};

static std::shared_ptr<MemeSchema> extractSchema(const Value &val);

static std::shared_ptr<MemeMap> wrapSchema(const std::shared_ptr<MemeSchema> &s)
{
    auto m = std::make_shared<MemeMap>();
    m->set("__is_schema__", Value(true));
    m->set("kind", Value(s->kind));

    // validate(val) -> { success: bool, data: coerced, errors: list }
    m->set("validate", Value(std::make_shared<NativeMethodCallable>("validate", 1, [s](Interpreter &, const std::vector<Value> &args) -> Value {
        auto res = s->validate(args[0]);
        auto resMap = std::make_shared<MemeMap>();
        resMap->set("success", Value(res.success));
        resMap->set("data", res.data);
        auto errList = std::make_shared<MemeList>();
        for (const auto &err : res.errors) errList->elements.push_back(Value(err));
        resMap->set("errors", Value(errList));
        return Value(resMap);
    })));

    // check(val) -> bool
    m->set("check", Value(std::make_shared<NativeMethodCallable>("check", 1, [s](Interpreter &, const std::vector<Value> &args) -> Value {
        return Value(s->validate(args[0]).success);
    })));

    // parse(val) -> val or throw
    m->set("parse", Value(std::make_shared<NativeMethodCallable>("parse", 1, [s](Interpreter &, const std::vector<Value> &args) -> Value {
        auto res = s->validate(args[0]);
        if (!res.success) {
            std::string combined = "Schema Validation Failed: ";
            for (size_t i = 0; i < res.errors.size(); ++i) {
                if (i > 0) combined += "; ";
                combined += res.errors[i];
            }
            throw std::runtime_error(combined);
        }
        return res.data;
    })));

    return m;
}

static std::shared_ptr<MemeSchema> extractSchema(const Value &val)
{
    Value d = val.deref();
    if (!d.isMap()) return nullptr;
    auto map = d.getMap();
    if (!map->has("__is_schema__")) return nullptr;

    auto s = std::make_shared<MemeSchema>();
    s->kind = map->has("kind") ? map->entries["kind"].deref().toString() : "any";

    // Recover inner schemas or properties from map if preserved
    if (map->has("__inner_schema__")) {
        s->inner = extractSchema(map->entries["__inner_schema__"]);
    }
    if (map->has("__literal_val__")) {
        s->literalVal = map->entries["__literal_val__"];
    }
    if (map->has("__properties__") && map->entries["__properties__"].deref().isMap()) {
        auto pm = map->entries["__properties__"].deref().getMap();
        for (const auto &k : pm->keyOrder) {
            auto sub = extractSchema(pm->entries[k]);
            if (sub) s->properties[k] = sub;
        }
    }
    if (map->has("__unions__") && map->entries["__unions__"].deref().isList()) {
        for (const auto &u : map->entries["__unions__"].deref().getList()->elements) {
            auto sub = extractSchema(u);
            if (sub) s->unionSchemas.push_back(sub);
        }
    }
    if (map->has("hasMin") && map->entries["hasMin"].deref().isTruthy()) {
        s->hasMin = true;
        s->minVal = map->entries["minVal"].deref().asDouble();
    }
    if (map->has("hasMax") && map->entries["hasMax"].deref().isTruthy()) {
        s->hasMax = true;
        s->maxVal = map->entries["maxVal"].deref().asDouble();
    }
    if (map->has("hasMinLength") && map->entries["hasMinLength"].deref().isTruthy()) {
        s->hasMinLength = true;
        s->minLength = static_cast<size_t>(map->entries["minLength"].deref().asInt());
    }
    if (map->has("hasMaxLength") && map->entries["hasMaxLength"].deref().isTruthy()) {
        s->hasMaxLength = true;
        s->maxLength = static_cast<size_t>(map->entries["maxLength"].deref().asInt());
    }
    if (map->has("pattern")) s->pattern = map->entries["pattern"].deref().toString();
    if (map->has("format")) s->format = map->entries["format"].deref().toString();
    if (map->has("additionalProperties")) s->additionalProperties = map->entries["additionalProperties"].deref().isTruthy();

    return s;
}

static std::shared_ptr<MemeMap> createSchemaModule()
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    // String(options = {})
    m->set("String", Value(std::make_shared<NativeMethodCallable>("String", 0, 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto s = std::make_shared<MemeSchema>();
        s->kind = "string";
        auto wrapped = wrapSchema(s);
        if (!args.empty() && args[0].deref().isMap()) {
            auto opts = args[0].deref().getMap();
            if (opts->has("minLength")) { s->hasMinLength = true; s->minLength = static_cast<size_t>(opts->entries["minLength"].deref().asInt()); wrapped->set("hasMinLength", Value(true)); wrapped->set("minLength", Value(static_cast<long long>(s->minLength))); }
            if (opts->has("maxLength")) { s->hasMaxLength = true; s->maxLength = static_cast<size_t>(opts->entries["maxLength"].deref().asInt()); wrapped->set("hasMaxLength", Value(true)); wrapped->set("maxLength", Value(static_cast<long long>(s->maxLength))); }
            if (opts->has("pattern")) { s->pattern = opts->entries["pattern"].deref().toString(); wrapped->set("pattern", Value(s->pattern)); }
            if (opts->has("format")) { s->format = opts->entries["format"].deref().toString(); wrapped->set("format", Value(s->format)); }
        }
        return Value(wrapped);
    })));
    m->set("shabd", m->entries["String"]);
    m->set("bol", m->entries["String"]);

    // Number(options = {})
    m->set("Number", Value(std::make_shared<NativeMethodCallable>("Number", 0, 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto s = std::make_shared<MemeSchema>();
        s->kind = "number";
        auto wrapped = wrapSchema(s);
        if (!args.empty() && args[0].deref().isMap()) {
            auto opts = args[0].deref().getMap();
            if (opts->has("minimum")) { s->hasMin = true; s->minVal = opts->entries["minimum"].deref().asDouble(); wrapped->set("hasMin", Value(true)); wrapped->set("minVal", Value(s->minVal)); }
            else if (opts->has("min")) { s->hasMin = true; s->minVal = opts->entries["min"].deref().asDouble(); wrapped->set("hasMin", Value(true)); wrapped->set("minVal", Value(s->minVal)); }
            if (opts->has("maximum")) { s->hasMax = true; s->maxVal = opts->entries["maximum"].deref().asDouble(); wrapped->set("hasMax", Value(true)); wrapped->set("maxVal", Value(s->maxVal)); }
            else if (opts->has("max")) { s->hasMax = true; s->maxVal = opts->entries["max"].deref().asDouble(); wrapped->set("hasMax", Value(true)); wrapped->set("maxVal", Value(s->maxVal)); }
        }
        return Value(wrapped);
    })));
    m->set("ank", m->entries["Number"]);
    m->set("sankhya", m->entries["Number"]);

    // Integer(options = {}) / Int
    m->set("Integer", Value(std::make_shared<NativeMethodCallable>("Integer", 0, 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto s = std::make_shared<MemeSchema>();
        s->kind = "integer";
        auto wrapped = wrapSchema(s);
        if (!args.empty() && args[0].deref().isMap()) {
            auto opts = args[0].deref().getMap();
            if (opts->has("minimum")) { s->hasMin = true; s->minVal = opts->entries["minimum"].deref().asDouble(); wrapped->set("hasMin", Value(true)); wrapped->set("minVal", Value(s->minVal)); }
            else if (opts->has("min")) { s->hasMin = true; s->minVal = opts->entries["min"].deref().asDouble(); wrapped->set("hasMin", Value(true)); wrapped->set("minVal", Value(s->minVal)); }
            if (opts->has("maximum")) { s->hasMax = true; s->maxVal = opts->entries["maximum"].deref().asDouble(); wrapped->set("hasMax", Value(true)); wrapped->set("maxVal", Value(s->maxVal)); }
            else if (opts->has("max")) { s->hasMax = true; s->maxVal = opts->entries["max"].deref().asDouble(); wrapped->set("hasMax", Value(true)); wrapped->set("maxVal", Value(s->maxVal)); }
        }
        return Value(wrapped);
    })));
    m->set("Int", m->entries["Integer"]);
    m->set("chota_ank", m->entries["Integer"]);

    // Boolean()
    m->set("Boolean", Value(std::make_shared<NativeMethodCallable>("Boolean", 0, 0, [](Interpreter &, const std::vector<Value> &) -> Value {
        auto s = std::make_shared<MemeSchema>();
        s->kind = "boolean";
        return Value(wrapSchema(s));
    })));
    m->set("sach", m->entries["Boolean"]);
    m->set("faisla", m->entries["Boolean"]);

    // Array(itemSchema, options = {})
    m->set("Array", Value(std::make_shared<NativeMethodCallable>("Array", 1, 2, [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto s = std::make_shared<MemeSchema>();
        s->kind = "array";
        s->inner = extractSchema(args[0]);
        auto wrapped = wrapSchema(s);
        wrapped->set("__inner_schema__", args[0]);
        if (args.size() > 1 && args[1].deref().isMap()) {
            auto opts = args[1].deref().getMap();
            if (opts->has("minItems")) { s->hasMin = true; s->minVal = opts->entries["minItems"].deref().asDouble(); wrapped->set("hasMin", Value(true)); wrapped->set("minVal", Value(s->minVal)); }
            if (opts->has("maxItems")) { s->hasMax = true; s->maxVal = opts->entries["maxItems"].deref().asDouble(); wrapped->set("hasMax", Value(true)); wrapped->set("maxVal", Value(s->maxVal)); }
        }
        return Value(wrapped);
    })));
    m->set("suchi", m->entries["Array"]);
    m->set("list", m->entries["Array"]);

    // Object(propsMap, options = {})
    m->set("Object", Value(std::make_shared<NativeMethodCallable>("Object", 1, 2, [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto s = std::make_shared<MemeSchema>();
        s->kind = "object";
        if (args[0].deref().isMap()) {
            auto pm = args[0].deref().getMap();
            for (const auto &k : pm->keyOrder) {
                auto sub = extractSchema(pm->entries[k]);
                if (sub) s->properties[k] = sub;
            }
        }
        auto wrapped = wrapSchema(s);
        wrapped->set("__properties__", args[0]);
        if (args.size() > 1 && args[1].deref().isMap()) {
            auto opts = args[1].deref().getMap();
            if (opts->has("additionalProperties")) {
                s->additionalProperties = opts->entries["additionalProperties"].deref().isTruthy();
                wrapped->set("additionalProperties", Value(s->additionalProperties));
            }
        }
        return Value(wrapped);
    })));
    m->set("vastu", m->entries["Object"]);
    m->set("dabba", m->entries["Object"]);

    // Optional(innerSchema)
    m->set("Optional", Value(std::make_shared<NativeMethodCallable>("Optional", 1, 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto s = std::make_shared<MemeSchema>();
        s->kind = "optional";
        s->inner = extractSchema(args[0]);
        auto wrapped = wrapSchema(s);
        wrapped->set("__inner_schema__", args[0]);
        return Value(wrapped);
    })));
    m->set("marzi", m->entries["Optional"]);
    m->set("aicchik", m->entries["Optional"]);

    // Union(schemasList)
    m->set("Union", Value(std::make_shared<NativeMethodCallable>("Union", 1, 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto s = std::make_shared<MemeSchema>();
        s->kind = "union";
        if (args[0].deref().isList()) {
            for (const auto &u : args[0].deref().getList()->elements) {
                auto sub = extractSchema(u);
                if (sub) s->unionSchemas.push_back(sub);
            }
        }
        auto wrapped = wrapSchema(s);
        wrapped->set("__unions__", args[0]);
        return Value(wrapped);
    })));
    m->set("jod", m->entries["Union"]);

    // Literal(value)
    m->set("Literal", Value(std::make_shared<NativeMethodCallable>("Literal", 1, 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto s = std::make_shared<MemeSchema>();
        s->kind = "literal";
        s->literalVal = args[0].deref();
        auto wrapped = wrapSchema(s);
        wrapped->set("__literal_val__", s->literalVal);
        return Value(wrapped);
    })));
    m->set("asli", m->entries["Literal"]);

    // Record(valSchema)
    m->set("Record", Value(std::make_shared<NativeMethodCallable>("Record", 1, 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto s = std::make_shared<MemeSchema>();
        s->kind = "record";
        s->inner = extractSchema(args[0]);
        auto wrapped = wrapSchema(s);
        wrapped->set("__inner_schema__", args[0]);
        return Value(wrapped);
    })));

    // Any()
    m->set("Any", Value(std::make_shared<NativeMethodCallable>("Any", 0, 0, [](Interpreter &, const std::vector<Value> &) -> Value {
        auto s = std::make_shared<MemeSchema>();
        s->kind = "any";
        return Value(wrapSchema(s));
    })));

    return m;
}

// ---------------------------------------------------------------------------
// HTTP Server Architecture (Fast Multi-threaded Express + Elysia Engine)
// ---------------------------------------------------------------------------

struct HttpRoute {
    std::string method;
    std::string path;
    std::vector<std::string> pathParts;
    bool isWildcard = false;

    std::shared_ptr<MemeSchema> bodySchema;
    std::shared_ptr<MemeSchema> querySchema;
    std::shared_ptr<MemeSchema> paramsSchema;
    std::shared_ptr<MemeSchema> headersSchema;
    std::shared_ptr<MemeSchema> responseSchema;

    std::vector<Value> middlewares;
    Value handler;
};

class HttpServerState;

struct HttpMiddleware {
    std::string pathPrefix = "";
    Value handler;
    bool isRouter = false;
    std::shared_ptr<HttpServerState> routerState = nullptr;
};

struct ClientConnection {
    int cfd = -1;
    sockaddr_in csin{};
};

class HttpServerState : public std::enable_shared_from_this<HttpServerState> {
public:
    std::vector<HttpRoute> routes;
    std::vector<HttpMiddleware> middlewares;

    std::vector<Value> onRequestHooks;
    std::vector<Value> onBeforeHandleHooks;
    std::vector<Value> onAfterHandleHooks;
    std::vector<Value> onErrorHooks;
    std::vector<Value> derivations;

    std::shared_ptr<MemeMap> decorators;
    std::shared_ptr<MemeMap> stateMap;

    int port = 8080;
    std::atomic<bool> isRunning{false};
    int serverFd = -1;
    std::thread acceptThread;
    std::vector<std::thread> workerThreads;
    std::queue<ClientConnection> clientQueue;
    std::mutex queueMutex;
    std::condition_variable queueCv;
    std::shared_ptr<Environment> globalsPtr;

    HttpServerState() {
        decorators = std::make_shared<MemeMap>();
        stateMap = std::make_shared<MemeMap>();
    }

    ~HttpServerState() {
        stop();
    }

    void stop() {
        if (!isRunning) return;
        isRunning = false;
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
        if (serverFd >= 0) {
            ::shutdown(serverFd, SHUT_RDWR);
            ::close(serverFd);
            serverFd = -1;
        }
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            while (!clientQueue.empty()) {
                ::close(clientQueue.front().cfd);
                clientQueue.pop();
            }
        }
        queueCv.notify_all();
        for (auto &t : workerThreads) {
            if (t.joinable()) t.join();
        }
        workerThreads.clear();
        if (acceptThread.joinable()) {
            acceptThread.join();
        }
#endif
    }

    static std::vector<std::string> splitPath(const std::string &p) {
        std::vector<std::string> parts;
        std::stringstream ss(p);
        std::string item;
        while (std::getline(ss, item, '/')) {
            if (!item.empty()) parts.push_back(item);
        }
        return parts;
    }

    Value dispatchRequest(Interpreter &interpreter,
                          const std::string &rawMethod,
                          const std::string &rawPath,
                          const Value &bodyVal = Value(std::monostate{}),
                          const Value &headersVal = Value(std::monostate{}),
                          const std::string &clientIp = "127.0.0.1")
    {
        std::string method = rawMethod;
        for (char &c : method) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

        std::string path = rawPath;
        auto queryMap = std::make_shared<MemeMap>();
        size_t qPos = path.find('?');
        if (qPos != std::string::npos) {
            std::string qStr = path.substr(qPos + 1);
            path = path.substr(0, qPos);
            std::stringstream ss(qStr);
            std::string pair;
            while (std::getline(ss, pair, '&')) {
                if (pair.empty()) continue;
                size_t eq = pair.find('=');
                if (eq != std::string::npos) {
                    queryMap->set(urlDecode(pair.substr(0, eq)), Value(urlDecode(pair.substr(eq + 1))));
                } else {
                    queryMap->set(urlDecode(pair), Value(true));
                }
            }
        }

        auto headersMap = std::make_shared<MemeMap>();
        if (headersVal.isMap()) {
            auto hm = headersVal.getMap();
            for (const auto &k : hm->keyOrder) {
                std::string lk = k;
                for (char &c : lk) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                headersMap->set(lk, hm->entries[k]);
            }
        }

        std::string cookieHdr = headersMap->has("cookie") ? headersMap->entries["cookie"].deref().toString() : "";
        auto cookiesMap = parseCookieHeader(cookieHdr);

        // Mutable Request / Response objects
        auto req = std::make_shared<MemeMap>();
        req->set("path", Value(path));
        req->set("method", Value(method));
        req->set("url", Value(rawPath));
        auto paramsMap = std::make_shared<MemeMap>();
        req->set("params", Value(paramsMap));
        req->set("query", Value(queryMap));
        req->set("body", bodyVal);
        req->set("headers", Value(headersMap));
        req->set("cookies", Value(cookiesMap));
        req->set("ip", Value(clientIp));

        req->set("get", Value(std::make_shared<NativeMethodCallable>("get", 1, [headersMap](Interpreter &, const std::vector<Value> &a) -> Value {
            std::string hk = a[0].deref().toString();
            for (char &c : hk) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return headersMap->has(hk) ? headersMap->entries[hk] : Value(std::monostate{});
        })));
        req->set("header", req->entries["get"]);

        auto res = std::make_shared<MemeMap>();
        auto resHeaders = std::make_shared<MemeMap>();
        auto statusCodePtr = std::make_shared<long long>(200LL);
        auto isSentPtr = std::make_shared<bool>(false);
        auto resBodyPtr = std::make_shared<Value>(std::monostate{});

        res->set("status", Value(std::make_shared<NativeMethodCallable>("status", 1, [statusCodePtr, res](Interpreter &, const std::vector<Value> &a) -> Value {
            *statusCodePtr = a[0].deref().asInt();
            return Value(res);
        })));

        auto sendFn = [isSentPtr, resBodyPtr, resHeaders, res](const Value &body) {
            *resBodyPtr = body;
            *isSentPtr = true;
            if (!resHeaders->has("Content-Type") && !resHeaders->has("content-type")) {
                Value d = body.deref();
                if (d.isMap() || d.isList()) {
                    resHeaders->set("Content-Type", Value("application/json; charset=utf-8"));
                } else {
                    std::string s = d.toString();
                    if (!s.empty() && s[0] == '<') resHeaders->set("Content-Type", Value("text/html; charset=utf-8"));
                    else resHeaders->set("Content-Type", Value("text/plain; charset=utf-8"));
                }
            }
            return Value(res);
        };

        res->set("send", Value(std::make_shared<NativeMethodCallable>("send", 1, [sendFn](Interpreter &, const std::vector<Value> &a) -> Value {
            return sendFn(a[0]);
        })));

        res->set("json", Value(std::make_shared<NativeMethodCallable>("json", 1, [sendFn, resHeaders](Interpreter &, const std::vector<Value> &a) -> Value {
            resHeaders->set("Content-Type", Value("application/json; charset=utf-8"));
            return sendFn(a[0]);
        })));

        res->set("html", Value(std::make_shared<NativeMethodCallable>("html", 1, [sendFn, resHeaders](Interpreter &, const std::vector<Value> &a) -> Value {
            resHeaders->set("Content-Type", Value("text/html; charset=utf-8"));
            return sendFn(a[0]);
        })));

        res->set("text", Value(std::make_shared<NativeMethodCallable>("text", 1, [sendFn, resHeaders](Interpreter &, const std::vector<Value> &a) -> Value {
            resHeaders->set("Content-Type", Value("text/plain; charset=utf-8"));
            return sendFn(a[0]);
        })));

        res->set("redirect", Value(std::make_shared<NativeMethodCallable>("redirect", 1, 2, [statusCodePtr, resHeaders, isSentPtr, resBodyPtr, res](Interpreter &, const std::vector<Value> &a) -> Value {
            std::string url = a[0].deref().toString();
            long long code = a.size() > 1 ? a[1].deref().asInt() : 302LL;
            *statusCodePtr = code;
            resHeaders->set("Location", Value(url));
            *resBodyPtr = Value("");
            *isSentPtr = true;
            return Value(res);
        })));

        res->set("set", Value(std::make_shared<NativeMethodCallable>("set", 2, [resHeaders, res](Interpreter &, const std::vector<Value> &a) -> Value {
            resHeaders->set(a[0].deref().toString(), a[1]);
            return Value(res);
        })));
        res->set("header", res->entries["set"]);

        res->set("cookie", Value(std::make_shared<NativeMethodCallable>("cookie", 2, 3, [resHeaders, res](Interpreter &, const std::vector<Value> &a) -> Value {
            std::string name = a[0].deref().toString();
            std::string val = a[1].deref().toString();
            std::string cookieStr = name + "=" + val + "; Path=/";
            if (a.size() > 2 && a[2].deref().isMap()) {
                auto opts = a[2].deref().getMap();
                if (opts->has("path")) cookieStr += "; Path=" + opts->entries["path"].deref().toString();
                if (opts->has("httpOnly") && opts->entries["httpOnly"].deref().isTruthy()) cookieStr += "; HttpOnly";
                if (opts->has("secure") && opts->entries["secure"].deref().isTruthy()) cookieStr += "; Secure";
                if (opts->has("sameSite")) cookieStr += "; SameSite=" + opts->entries["sameSite"].deref().toString();
            }
            resHeaders->set("Set-Cookie", Value(cookieStr));
            return Value(res);
        })));

        res->set("clearCookie", Value(std::make_shared<NativeMethodCallable>("clearCookie", 1, [resHeaders, res](Interpreter &, const std::vector<Value> &a) -> Value {
            std::string name = a[0].deref().toString();
            resHeaders->set("Set-Cookie", Value(name + "=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT"));
            return Value(res);
        })));

        res->set("sendFile", Value(std::make_shared<NativeMethodCallable>("sendFile", 1, [isSentPtr, resBodyPtr, resHeaders, statusCodePtr, res](Interpreter &, const std::vector<Value> &a) -> Value {
            std::string filePath = a[0].deref().toString();
            std::ifstream f(filePath, std::ios::binary);
            if (f.is_open()) {
                std::stringstream ss;
                ss << f.rdbuf();
                *resBodyPtr = Value(ss.str());
                resHeaders->set("Content-Type", Value(getMimeType(filePath)));
                *isSentPtr = true;
            } else {
                *statusCodePtr = 404LL;
                *resBodyPtr = Value("File not found: " + filePath);
                resHeaders->set("Content-Type", Value("text/plain; charset=utf-8"));
                *isSentPtr = true;
            }
            return Value(res);
        })));

        // Elysia context (ctx)
        auto ctx = std::make_shared<MemeMap>();
        ctx->set("req", Value(req));
        ctx->set("res", Value(res));
        ctx->set("path", Value(path));
        ctx->set("method", Value(method));
        ctx->set("url", Value(rawPath));
        ctx->set("params", Value(paramsMap));
        ctx->set("query", Value(queryMap));
        ctx->set("body", bodyVal);
        ctx->set("headers", Value(headersMap));
        ctx->set("cookies", Value(cookiesMap));
        ctx->set("ip", Value(clientIp));
        ctx->set("store", Value(stateMap));

        auto setObj = std::make_shared<MemeMap>();
        setObj->set("status", Value(*statusCodePtr));
        setObj->set("headers", Value(resHeaders));
        setObj->set("redirect", Value(""));
        ctx->set("set", Value(setObj));

        ctx->set("error", Value(std::make_shared<NativeMethodCallable>("error", 2, [](Interpreter &, const std::vector<Value> &a) -> Value {
            auto errMap = std::make_shared<MemeMap>();
            errMap->set("status", a[0]);
            errMap->set("error", a[1]);
            return Value(errMap);
        })));

        // Copy decorators into ctx
        for (const auto &k : decorators->keyOrder) {
            ctx->set(k, decorators->entries[k]);
        }

        // Helper to finalize response
        auto packageResponse = [&](const Value &b) -> Value {
            auto out = std::make_shared<MemeMap>();
            long long finalStatus = *statusCodePtr;
            if (setObj->has("status") && setObj->entries["status"].deref().isInteger()) {
                long long s = setObj->entries["status"].deref().asInt();
                if (s != 200LL) finalStatus = s;
            }
            if (b.deref().isMap() && b.deref().getMap()->has("status") && b.deref().getMap()->entries["status"].deref().isInteger()) {
                long long s = b.deref().getMap()->entries["status"].deref().asInt();
                if (finalStatus == 200LL && s != 200LL) finalStatus = s;
            }
            if (setObj->has("redirect") && !setObj->entries["redirect"].deref().toString().empty()) {
                finalStatus = 302LL;
                resHeaders->set("Location", setObj->entries["redirect"]);
            }
            out->set("status", Value(finalStatus));
            out->set("headers", Value(resHeaders));
            out->set("body", b);
            out->set("ok", Value(finalStatus >= 200LL && finalStatus < 300LL));
            return Value(out);
        };

        try {
            // 1. onRequest Hooks
            for (const auto &hookVal : onRequestHooks) {
                if (hookVal.isCallable()) {
                    Value r = hookVal.getCallable()->call(interpreter, { Value(ctx) });
                    if (*isSentPtr) return packageResponse(*resBodyPtr);
                    if (!r.isNull() && !std::holds_alternative<std::monostate>(r.deref().data)) {
                        return packageResponse(r);
                    }
                }
            }

            // 2. Derivations (app.derive)
            for (const auto &dVal : derivations) {
                if (dVal.isCallable()) {
                    Value derived = dVal.getCallable()->call(interpreter, { Value(ctx) });
                    if (derived.isMap()) {
                        auto dm = derived.getMap();
                        for (const auto &k : dm->keyOrder) ctx->set(k, dm->entries[k]);
                    }
                }
            }

            // 3. Global & Prefix Middlewares
            for (const auto &mw : middlewares) {
                if (mw.pathPrefix.empty() || path.rfind(mw.pathPrefix, 0) == 0) {
                    if (mw.isRouter && mw.handler.isMap() && mw.handler.getMap()->has("request")) {
                        std::string subPath = path.substr(mw.pathPrefix.size());
                        if (subPath.empty()) subPath = "/";
                        else if (subPath.front() != '/') subPath = "/" + subPath;
                        if (qPos != std::string::npos) subPath += rawPath.substr(qPos);
                        auto rCallable = mw.handler.getMap()->entries["request"].getCallable();
                        Value subRes = rCallable->call(interpreter, { Value(method), Value(subPath), req->entries["body"], Value(headersMap) });
                        if (subRes.isMap() && subRes.getMap()->has("status") && subRes.getMap()->entries["status"].deref().asInt() != 404LL) {
                            return subRes;
                        }
                    } else if (mw.handler.isCallable()) {
                        bool nextCalled = false;
                        auto nextCallable = std::make_shared<NativeMethodCallable>("next", 0, [&nextCalled](Interpreter &, const std::vector<Value> &) -> Value {
                            nextCalled = true;
                            return Value(true);
                        });
                        auto mwCallable = mw.handler.getCallable();
                        Value r;
                        if (mwCallable->maxArity() <= 1) {
                            r = mwCallable->call(interpreter, { Value(ctx) });
                        } else {
                            r = mwCallable->call(interpreter, { Value(req), Value(res), Value(nextCallable) });
                        }
                        if (*isSentPtr) return packageResponse(*resBodyPtr);
                        if (!nextCalled && !r.isNull() && !std::holds_alternative<std::monostate>(r.deref().data)) {
                            return packageResponse(r);
                        }
                    }
                }
            }

            // 4. Route Matching
            auto reqParts = splitPath(path);
            const HttpRoute *matchedRoute = nullptr;

            for (const auto &r : routes) {
                if (r.method != method && r.method != "ALL" && r.method != "*") continue;

                if (r.path == path || r.path == "*") {
                    matchedRoute = &r;
                    break;
                }
                if (r.isWildcard && r.pathParts.size() <= reqParts.size()) {
                    bool match = true;
                    for (size_t i = 0; i < r.pathParts.size() - 1; ++i) {
                        if (r.pathParts[i] != reqParts[i]) { match = false; break; }
                    }
                    if (match) {
                        matchedRoute = &r;
                        std::string wild;
                        for (size_t i = r.pathParts.size() - 1; i < reqParts.size(); ++i) {
                            if (!wild.empty()) wild += "/";
                            wild += reqParts[i];
                        }
                        paramsMap->set("*", Value(wild));
                        break;
                    }
                }
                if (r.pathParts.size() == reqParts.size()) {
                    bool match = true;
                    auto tempParams = std::make_shared<MemeMap>();
                    for (size_t i = 0; i < r.pathParts.size(); ++i) {
                        if (!r.pathParts[i].empty() && r.pathParts[i][0] == ':') {
                            tempParams->set(r.pathParts[i].substr(1), Value(reqParts[i]));
                        } else if (r.pathParts[i] != reqParts[i]) {
                            match = false;
                            break;
                        }
                    }
                    if (match) {
                        for (const auto &k : tempParams->keyOrder) paramsMap->set(k, tempParams->entries[k]);
                        matchedRoute = &r;
                        break;
                    }
                }
            }

            if (!matchedRoute) {
                *statusCodePtr = 404LL;
                return packageResponse(Value("Dhabe pe ye item nahi milta! 404 Route Not Found: " + method + " " + path));
            }

            // 5. Schema Validation (Elysia-style complete typesafety)
            if (matchedRoute->paramsSchema) {
                auto vr = matchedRoute->paramsSchema->validate(Value(paramsMap), "params");
                if (!vr.success) {
                    *statusCodePtr = 400LL;
                    auto errMap = std::make_shared<MemeMap>();
                    errMap->set("status", Value(400LL));
                    errMap->set("error", Value("ValidationError"));
                    errMap->set("message", Value("Invalid route parameters"));
                    auto errs = std::make_shared<MemeList>();
                    for (const auto &e : vr.errors) errs->elements.push_back(Value(e));
                    errMap->set("details", Value(errs));
                    return packageResponse(Value(errMap));
                }
                if (vr.data.isMap()) {
                    for (const auto &k : vr.data.getMap()->keyOrder) paramsMap->set(k, vr.data.getMap()->entries[k]);
                }
            }

            if (matchedRoute->querySchema) {
                auto vr = matchedRoute->querySchema->validate(Value(queryMap), "query");
                if (!vr.success) {
                    *statusCodePtr = 400LL;
                    auto errMap = std::make_shared<MemeMap>();
                    errMap->set("status", Value(400LL));
                    errMap->set("error", Value("ValidationError"));
                    errMap->set("message", Value("Invalid query string parameters"));
                    auto errs = std::make_shared<MemeList>();
                    for (const auto &e : vr.errors) errs->elements.push_back(Value(e));
                    errMap->set("details", Value(errs));
                    return packageResponse(Value(errMap));
                }
                if (vr.data.isMap()) {
                    for (const auto &k : vr.data.getMap()->keyOrder) queryMap->set(k, vr.data.getMap()->entries[k]);
                }
            }

            if (matchedRoute->bodySchema) {
                auto vr = matchedRoute->bodySchema->validate(req->entries["body"], "body");
                if (!vr.success) {
                    *statusCodePtr = 400LL;
                    auto errMap = std::make_shared<MemeMap>();
                    errMap->set("status", Value(400LL));
                    errMap->set("error", Value("ValidationError"));
                    errMap->set("message", Value("Invalid request body payload"));
                    auto errs = std::make_shared<MemeList>();
                    for (const auto &e : vr.errors) errs->elements.push_back(Value(e));
                    errMap->set("details", Value(errs));
                    return packageResponse(Value(errMap));
                }
                req->set("body", vr.data);
                ctx->set("body", vr.data);
            }

            if (matchedRoute->headersSchema) {
                auto vr = matchedRoute->headersSchema->validate(Value(headersMap), "headers");
                if (!vr.success) {
                    *statusCodePtr = 400LL;
                    auto errMap = std::make_shared<MemeMap>();
                    errMap->set("status", Value(400LL));
                    errMap->set("error", Value("ValidationError"));
                    errMap->set("message", Value("Invalid request headers"));
                    auto errs = std::make_shared<MemeList>();
                    for (const auto &e : vr.errors) errs->elements.push_back(Value(e));
                    errMap->set("details", Value(errs));
                    return packageResponse(Value(errMap));
                }
            }

            // 6. onBeforeHandle Hooks
            for (const auto &hookVal : onBeforeHandleHooks) {
                if (hookVal.isCallable()) {
                    Value r = hookVal.getCallable()->call(interpreter, { Value(ctx) });
                    if (*isSentPtr) return packageResponse(*resBodyPtr);
                    if (!r.isNull() && !std::holds_alternative<std::monostate>(r.deref().data)) {
                        return packageResponse(r);
                    }
                }
            }

            // 7. Route-specific Middlewares
            for (const auto &rmwVal : matchedRoute->middlewares) {
                if (rmwVal.isCallable()) {
                    bool nextCalled = false;
                    auto nextCallable = std::make_shared<NativeMethodCallable>("next", 0, [&nextCalled](Interpreter &, const std::vector<Value> &) -> Value {
                        nextCalled = true;
                        return Value(true);
                    });
                    auto callable = rmwVal.getCallable();
                    Value r;
                    if (callable->maxArity() <= 1) {
                        r = callable->call(interpreter, { Value(ctx) });
                    } else {
                        r = callable->call(interpreter, { Value(req), Value(res), Value(nextCallable) });
                    }
                    if (*isSentPtr) return packageResponse(*resBodyPtr);
                    if (!nextCalled && !r.isNull() && !std::holds_alternative<std::monostate>(r.deref().data)) {
                        return packageResponse(r);
                    }
                }
            }

            // 8. Invoke Handler
            Value handlerResult = Value(std::monostate{});
            if (matchedRoute->handler.isCallable()) {
                auto h = matchedRoute->handler.getCallable();
                if (h->maxArity() <= 1) {
                    handlerResult = h->call(interpreter, { Value(ctx) });
                } else {
                    bool dummyNext = false;
                    auto nextCallable = std::make_shared<NativeMethodCallable>("next", 0, [&dummyNext](Interpreter &, const std::vector<Value> &) -> Value {
                        dummyNext = true; return Value(true);
                    });
                    handlerResult = h->call(interpreter, { Value(req), Value(res), Value(nextCallable) });
                }
            }

            Value finalBody = *isSentPtr ? *resBodyPtr : handlerResult;

            // 9. Response Schema Validation
            if (matchedRoute->responseSchema) {
                auto vr = matchedRoute->responseSchema->validate(finalBody, "response");
                if (vr.success) finalBody = vr.data;
            }

            // 10. onAfterHandle Hooks
            for (const auto &hookVal : onAfterHandleHooks) {
                if (hookVal.isCallable()) {
                    hookVal.getCallable()->call(interpreter, { Value(ctx), finalBody });
                }
            }

            return packageResponse(finalBody);
        } catch (const std::exception &e) {
            // onError Hooks
            for (const auto &errHookVal : onErrorHooks) {
                if (errHookVal.isCallable()) {
                    Value r = errHookVal.getCallable()->call(interpreter, { Value(e.what()), Value(ctx) });
                    if (*isSentPtr) return packageResponse(*resBodyPtr);
                    if (!r.isNull() && !std::holds_alternative<std::monostate>(r.deref().data)) {
                        return packageResponse(r);
                    }
                }
            }
            *statusCodePtr = 500LL;
            auto errMap = std::make_shared<MemeMap>();
            errMap->set("status", Value(500LL));
            errMap->set("error", Value("InternalServerError"));
            errMap->set("message", Value(e.what()));
            return packageResponse(Value(errMap));
        }
    }
};

static std::string formatHttpResponse(const Value &resVal, bool keepAlive = false)
{
    int status = 200;
    std::string statusText = "OK";
    std::string body;
    std::map<std::string, std::string> headers;

    Value d = resVal.deref();
    if (d.isMap()) {
        auto m = d.getMap();
        if (m->has("status")) status = static_cast<int>(m->entries["status"].deref().asInt());
        if (m->has("body")) {
            Value bVal = m->entries["body"].deref();
            if (bVal.isMap() || bVal.isList()) {
                body = MemeJson::stringify(bVal);
                if (headers.find("Content-Type") == headers.end()) {
                    headers["Content-Type"] = "application/json; charset=utf-8";
                }
            } else {
                body = bVal.toString();
            }
        }
        if (m->has("headers") && m->entries["headers"].deref().isMap()) {
            auto hm = m->entries["headers"].deref().getMap();
            for (const auto &k : hm->keyOrder) {
                headers[k] = hm->entries[k].deref().toString();
            }
        }
    } else if (d.isList()) {
        body = MemeJson::stringify(d);
        headers["Content-Type"] = "application/json; charset=utf-8";
    } else {
        body = d.toString();
        headers["Content-Type"] = "text/plain; charset=utf-8";
    }

    switch (status) {
        case 200: statusText = "OK"; break;
        case 201: statusText = "Created"; break;
        case 204: statusText = "No Content"; break;
        case 301: statusText = "Moved Permanently"; break;
        case 302: statusText = "Found"; break;
        case 304: statusText = "Not Modified"; break;
        case 400: statusText = "Bad Request"; break;
        case 401: statusText = "Unauthorized"; break;
        case 403: statusText = "Forbidden"; break;
        case 404: statusText = "Not Found"; break;
        case 405: statusText = "Method Not Allowed"; break;
        case 409: statusText = "Conflict"; break;
        case 422: statusText = "Unprocessable Entity"; break;
        case 500: statusText = "Internal Server Error"; break;
        default: statusText = "Status " + std::to_string(status); break;
    }

    if (headers.find("Content-Type") == headers.end() && headers.find("content-type") == headers.end()) {
        headers["Content-Type"] = (!body.empty() && body[0] == '<') ? "text/html; charset=utf-8" : "text/plain; charset=utf-8";
    }
    headers["Content-Length"] = std::to_string(body.size());
    if (headers.find("Connection") == headers.end() && headers.find("connection") == headers.end()) {
        headers["Connection"] = keepAlive ? "keep-alive" : "close";
    }
    if (headers.find("Server") == headers.end() && headers.find("server") == headers.end()) {
        headers["Server"] = "MEMElang-Dhaba/2.0";
    }

    std::string out;
    out.reserve(256 + body.size());
    out.append("HTTP/1.1 ").append(std::to_string(status)).append(" ").append(statusText).append("\r\n");
    for (const auto &[k, v] : headers) {
        out.append(k).append(": ").append(v).append("\r\n");
    }
    out.append("\r\n");
    out.append(body);
    return out;
}

static std::shared_ptr<MemeMap> createHttpServerMap(const std::shared_ptr<HttpServerState> &server, Interpreter &interpreter)
{
    auto m = std::make_shared<MemeMap>();
    m->set("__is_server__", Value(true));

    auto addRoute = [server](const std::string &method, const std::vector<Value> &args) {
        HttpRoute r;
        r.method = method;
        r.path = args[0].deref().toString();
        r.pathParts = HttpServerState::splitPath(r.path);
        r.isWildcard = (!r.pathParts.empty() && r.pathParts.back() == "*");

        size_t idx = 1;
        if (idx < args.size() && args[idx].deref().isMap() &&
            (args[idx].deref().getMap()->has("body") || args[idx].deref().getMap()->has("query") ||
             args[idx].deref().getMap()->has("params") || args[idx].deref().getMap()->has("headers") ||
             args[idx].deref().getMap()->has("response") || args[idx].deref().getMap()->has("__is_schema__")))
        {
            auto sm = args[idx].deref().getMap();
            if (sm->has("body")) r.bodySchema = extractSchema(sm->entries["body"]);
            if (sm->has("query")) r.querySchema = extractSchema(sm->entries["query"]);
            if (sm->has("params")) r.paramsSchema = extractSchema(sm->entries["params"]);
            if (sm->has("headers")) r.headersSchema = extractSchema(sm->entries["headers"]);
            if (sm->has("response")) r.responseSchema = extractSchema(sm->entries["response"]);
            idx++;
        }

        while (idx + 1 < args.size()) {
            r.middlewares.push_back(args[idx]);
            idx++;
        }
        if (idx < args.size()) {
            r.handler = args[idx];
        }
        server->routes.push_back(r);
    };

    auto makeMethodFn = [addRoute, m](const std::string &mName) {
        return Value(std::make_shared<NativeMethodCallable>(mName, 2, -1, [addRoute, mName, m](Interpreter &, const std::vector<Value> &a) -> Value {
            addRoute(mName, a);
            return Value(m);
        }));
    };

    m->set("get", makeMethodFn("GET"));
    m->set("le", m->entries["get"]);
    m->set("post", makeMethodFn("POST"));
    m->set("bhej", m->entries["post"]);
    m->set("put", makeMethodFn("PUT"));
    m->set("rakh", m->entries["put"]);
    m->set("patch", makeMethodFn("PATCH"));
    m->set("sudhar", m->entries["patch"]);
    m->set("delete", makeMethodFn("DELETE"));
    m->set("mita", m->entries["delete"]);
    m->set("head", makeMethodFn("HEAD"));
    m->set("options", makeMethodFn("OPTIONS"));
    m->set("all", makeMethodFn("ALL"));

    auto routeFn = [addRoute, m](Interpreter &, const std::vector<Value> &a) -> Value {
        std::string method = a[0].deref().toString();
        for (char &c : method) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        std::vector<Value> rest(a.begin() + 1, a.end());
        addRoute(method, rest);
        return Value(m);
    };
    m->set("route", Value(std::make_shared<NativeMethodCallable>("route", 3, -1, routeFn)));

    // app.use(...) / app.istemal(...)
    auto useFn = [server, m](Interpreter &, const std::vector<Value> &args) -> Value {
        if (args.empty()) return Value(m);
        HttpMiddleware mw;
        size_t idx = 0;
        if (args.size() > 1 && std::holds_alternative<std::string>(args[0].deref().data)) {
            mw.pathPrefix = args[0].deref().toString();
            idx = 1;
        }
        Value target = args[idx].deref();
        if (target.isMap() && target.getMap()->has("__is_server__")) {
            mw.isRouter = true;
            // Retrieve server state through request method closure or attach
            if (target.getMap()->has("__server_state__")) {
                // router mounted
            }
        }
        mw.handler = target;
        server->middlewares.push_back(mw);
        return Value(m);
    };
    m->set("use", Value(std::make_shared<NativeMethodCallable>("use", 1, 2, useFn)));
    m->set("istemal", m->entries["use"]);

    // app.request(method, path, body = null, headers = null) / app.handle(req)
    auto reqFn = [server, &interpreter](Interpreter &, const std::vector<Value> &args) -> Value {
        if (args.size() == 1 && args[0].deref().isMap()) {
            auto rm = args[0].deref().getMap();
            std::string method = rm->has("method") ? rm->entries["method"].deref().toString() : "GET";
            std::string path = rm->has("url") ? rm->entries["url"].deref().toString() : (rm->has("path") ? rm->entries["path"].deref().toString() : "/");
            Value body = rm->has("body") ? rm->entries["body"] : Value(std::monostate{});
            Value hdrs = rm->has("headers") ? rm->entries["headers"] : Value(std::monostate{});
            return server->dispatchRequest(interpreter, method, path, body, hdrs, "127.0.0.1");
        }
        std::string method = args[0].deref().toString();
        std::string path = args[1].deref().toString();
        Value body = args.size() > 2 ? args[2] : Value(std::monostate{});
        Value headers = args.size() > 3 ? args[3] : Value(std::monostate{});
        return server->dispatchRequest(interpreter, method, path, body, headers, "127.0.0.1");
    };
    m->set("request", Value(std::make_shared<NativeMethodCallable>("request", 1, 4, reqFn)));
    m->set("handle", m->entries["request"]);

    // app.listen(port = 8080, [host / callback], [callback])
    auto listenFn = [server, &interpreter, m](Interpreter &, const std::vector<Value> &args) -> Value {
        int port = 8080;
        std::string host = "0.0.0.0";
        Value cbVal = Value(std::monostate{});

        if (!args.empty()) port = static_cast<int>(args[0].deref().asInt());
        if (args.size() > 1) {
            if (std::holds_alternative<std::string>(args[1].deref().data)) host = args[1].deref().toString();
            else if (args[1].isCallable()) cbVal = args[1];
        }
        if (args.size() > 2 && args[2].isCallable()) cbVal = args[2];

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
        if (server->isRunning) server->stop();
        server->globalsPtr = interpreter.getGlobals();
        int sfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sfd < 0) throw std::runtime_error("Could not create TCP socket!");
        int opt = 1;
        setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#ifdef SO_REUSEPORT
        setsockopt(sfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
#endif
        int flag = 1;
        setsockopt(sfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

        sockaddr_in sin;
        std::memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_port = htons(port);
        if (host.empty() || host == "0.0.0.0") sin.sin_addr.s_addr = INADDR_ANY;
        else inet_pton(AF_INET, host.c_str(), &sin.sin_addr);

        if (bind(sfd, (sockaddr*)&sin, sizeof(sin)) != 0) {
            close(sfd);
            throw std::runtime_error("Port " + std::to_string(port) + " bind nahi ho paya!");
        }

        socklen_t slen = sizeof(sin);
        getsockname(sfd, (sockaddr*)&sin, &slen);
        server->port = ntohs(sin.sin_port);
        ::listen(sfd, 1024);
        server->serverFd = sfd;
        server->isRunning = true;

        auto sRef = server;

        size_t hw = std::thread::hardware_concurrency();
        size_t numWorkers = hw > 0 ? (hw * 2) : 8;
        if (numWorkers < 4) numWorkers = 4;
        if (numWorkers > 32) numWorkers = 32;

        for (size_t wi = 0; wi < numWorkers; ++wi) {
            server->workerThreads.emplace_back([sRef]() {
                Interpreter workerInterp;
                workerInterp.setGlobals(sRef->globalsPtr);

                while (sRef->isRunning) {
                    ClientConnection conn;
                    {
                        std::unique_lock<std::mutex> lock(sRef->queueMutex);
                        sRef->queueCv.wait(lock, [sRef]() {
                            return !sRef->isRunning || !sRef->clientQueue.empty();
                        });
                        if (!sRef->isRunning && sRef->clientQueue.empty()) break;
                        conn = sRef->clientQueue.front();
                        sRef->clientQueue.pop();
                    }

                    int cfd = conn.cfd;
                    if (cfd < 0) continue;

                    int flag = 1;
                    setsockopt(cfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

                    struct timeval tv;
                    tv.tv_sec = 5;
                    tv.tv_usec = 0;
                    setsockopt(cfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

                    char ipStr[INET_ADDRSTRLEN] = "127.0.0.1";
                    inet_ntop(AF_INET, &conn.csin.sin_addr, ipStr, sizeof(ipStr));
                    std::string clientIp(ipStr);

                    std::string raw;
                    char buf[8192];
                    bool keepAlive = true;

                    while (sRef->isRunning && keepAlive) {
                        size_t hend = raw.find("\r\n\r\n");
                        while (hend == std::string::npos) {
                            ssize_t n = recv(cfd, buf, sizeof(buf), 0);
                            if (n <= 0) {
                                keepAlive = false;
                                break;
                            }
                            raw.append(buf, n);
                            hend = raw.find("\r\n\r\n");
                        }
                        if (!keepAlive || hend == std::string::npos) break;

                        size_t bodyLenNeeded = 0;
                        size_t clPos = raw.find("Content-Length: ");
                        if (clPos == std::string::npos) clPos = raw.find("content-length: ");
                        if (clPos != std::string::npos && clPos < hend) {
                            size_t clEnd = raw.find("\r\n", clPos);
                            if (clEnd != std::string::npos) {
                                try {
                                    bodyLenNeeded = static_cast<size_t>(std::stoll(raw.substr(clPos + 16, clEnd - (clPos + 16))));
                                } catch (...) {}
                            }
                        }

                        while (raw.size() < hend + 4 + bodyLenNeeded) {
                            ssize_t n = recv(cfd, buf, sizeof(buf), 0);
                            if (n <= 0) {
                                keepAlive = false;
                                break;
                            }
                            raw.append(buf, n);
                        }
                        if (!keepAlive) break;

                        size_t totalReqLen = hend + 4 + bodyLenNeeded;
                        std::string singleReq = raw.substr(0, totalReqLen);
                        raw.erase(0, totalReqLen);

                        // Parse Request Line
                        size_t firstLineEnd = singleReq.find("\r\n");
                        if (firstLineEnd == std::string::npos) break;
                        std::string reqLine = singleReq.substr(0, firstLineEnd);

                        std::string m = "GET", p = "/", ver = "HTTP/1.1";
                        size_t s1 = reqLine.find(' ');
                        if (s1 != std::string::npos) {
                            m = reqLine.substr(0, s1);
                            size_t s2 = reqLine.find(' ', s1 + 1);
                            if (s2 != std::string::npos) {
                                p = reqLine.substr(s1 + 1, s2 - (s1 + 1));
                                ver = reqLine.substr(s2 + 1);
                            } else {
                                p = reqLine.substr(s1 + 1);
                            }
                        }

                        // Protocol defaults
                        if (ver.find("HTTP/1.0") != std::string::npos) {
                            keepAlive = false;
                        } else {
                            keepAlive = true;
                        }

                        // Parse Headers
                        auto hMap = std::make_shared<MemeMap>();
                        size_t hStart = firstLineEnd + 2;
                        while (hStart < hend) {
                            size_t hEnd = singleReq.find("\r\n", hStart);
                            if (hEnd == std::string::npos || hEnd == hStart) break;
                            std::string hl = singleReq.substr(hStart, hEnd - hStart);
                            hStart = hEnd + 2;

                            size_t colon = hl.find(':');
                            if (colon != std::string::npos) {
                                std::string hk = hl.substr(0, colon);
                                for (char &ch : hk) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                                std::string hv = hl.substr(colon + 1);
                                while (!hv.empty() && hv.front() == ' ') hv.erase(hv.begin());
                                while (!hv.empty() && (hv.back() == ' ' || hv.back() == '\r')) hv.pop_back();

                                if (hk == "connection") {
                                    std::string lhv = hv;
                                    for (char &ch : lhv) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                                    if (lhv.find("close") != std::string::npos) {
                                        keepAlive = false;
                                    } else if (lhv.find("keep-alive") != std::string::npos) {
                                        keepAlive = true;
                                    }
                                }
                                hMap->set(hk, Value(hv));
                            }
                        }

                        // Parse Body
                        std::string bstr = (hend + 4 < totalReqLen) ? singleReq.substr(hend + 4, bodyLenNeeded) : "";
                        Value bval = Value(bstr);
                        if (hMap->has("content-type")) {
                            std::string ct = hMap->entries["content-type"].deref().toString();
                            if (ct.find("application/json") != std::string::npos && !bstr.empty()) {
                                try { MemeJson::Parser jp(bstr); bval = jp.parseValue(); } catch (...) {}
                            } else if (ct.find("application/x-www-form-urlencoded") != std::string::npos && !bstr.empty()) {
                                bval = parseUrlEncoded(bstr);
                            }
                        }

                        workerInterp.resetState();
                        Value resVal = sRef->dispatchRequest(workerInterp, m, p, bval, Value(hMap), clientIp);
                        std::string wire = formatHttpResponse(resVal, keepAlive);
                        ssize_t sent = 0;
                        while (sent < static_cast<ssize_t>(wire.size())) {
                            ssize_t s = send(cfd, wire.data() + sent, wire.size() - sent, 0);
                            if (s <= 0) {
                                keepAlive = false;
                                break;
                            }
                            sent += s;
                        }
                    }
                    close(cfd);
                }
            });
        }

        server->acceptThread = std::thread([sRef]() {
            while (sRef->isRunning) {
                sockaddr_in csin;
                socklen_t cslen = sizeof(csin);
                int cfd = accept(sRef->serverFd, (sockaddr*)&csin, &cslen);
                if (cfd < 0) {
                    if (!sRef->isRunning) break;
                    continue;
                }
                {
                    std::lock_guard<std::mutex> lock(sRef->queueMutex);
                    sRef->clientQueue.push(ClientConnection{cfd, csin});
                }
                sRef->queueCv.notify_one();
            }
        });
#else
        server->port = port;
        server->isRunning = true;
#endif
        interpreter.getOutputStream() << "[Dhaba / Express / Elysia Server] 🚀 Online on http://localhost:" << server->port << "\n";
        if (cbVal.isCallable()) {
            cbVal.getCallable()->call(interpreter, { Value(static_cast<long long>(server->port)) });
        }
        return Value(m);
    };
    m->set("listen", Value(std::make_shared<NativeMethodCallable>("listen", 0, 3, listenFn)));
    m->set("chala", m->entries["listen"]);
    m->set("sun", m->entries["listen"]);

    auto stopFn = [server, m](Interpreter &, const std::vector<Value> &) -> Value {
        server->stop();
        return Value(m);
    };
    m->set("stop", Value(std::make_shared<NativeMethodCallable>("stop", 0, 0, stopFn)));
    m->set("rok", m->entries["stop"]);
    m->set("band_kar", m->entries["stop"]);

    auto resetFn = [server, m](Interpreter &, const std::vector<Value> &) -> Value {
        server->stop();
        server->routes.clear();
        server->middlewares.clear();
        server->onRequestHooks.clear();
        server->onBeforeHandleHooks.clear();
        server->onAfterHandleHooks.clear();
        server->onErrorHooks.clear();
        server->derivations.clear();
        server->decorators->entries.clear();
        server->decorators->keyOrder.clear();
        server->stateMap->entries.clear();
        server->stateMap->keyOrder.clear();
        return Value(m);
    };
    m->set("reset", Value(std::make_shared<NativeMethodCallable>("reset", 0, 0, resetFn)));
    m->set("saaf", m->entries["reset"]);

    // Elysia hooks & decorators
    m->set("onRequest", Value(std::make_shared<NativeMethodCallable>("onRequest", 1, [server, m](Interpreter &, const std::vector<Value> &a) -> Value {
        server->onRequestHooks.push_back(a[0]);
        return Value(m);
    })));
    m->set("onBeforeHandle", Value(std::make_shared<NativeMethodCallable>("onBeforeHandle", 1, [server, m](Interpreter &, const std::vector<Value> &a) -> Value {
        server->onBeforeHandleHooks.push_back(a[0]);
        return Value(m);
    })));
    m->set("onAfterHandle", Value(std::make_shared<NativeMethodCallable>("onAfterHandle", 1, [server, m](Interpreter &, const std::vector<Value> &a) -> Value {
        server->onAfterHandleHooks.push_back(a[0]);
        return Value(m);
    })));
    m->set("onError", Value(std::make_shared<NativeMethodCallable>("onError", 1, [server, m](Interpreter &, const std::vector<Value> &a) -> Value {
        server->onErrorHooks.push_back(a[0]);
        return Value(m);
    })));

    m->set("derive", Value(std::make_shared<NativeMethodCallable>("derive", 1, [server, m](Interpreter &, const std::vector<Value> &a) -> Value {
        server->derivations.push_back(a[0]);
        return Value(m);
    })));
    m->set("decorate", Value(std::make_shared<NativeMethodCallable>("decorate", 2, [server, m](Interpreter &, const std::vector<Value> &a) -> Value {
        std::string k = a[0].deref().toString();
        server->decorators->set(k, a[1]);
        m->set(k, a[1]);
        return Value(m);
    })));
    m->set("state", Value(std::make_shared<NativeMethodCallable>("state", 2, [server, m](Interpreter &, const std::vector<Value> &a) -> Value {
        std::string k = a[0].deref().toString();
        server->stateMap->set(k, a[1]);
        return Value(m);
    })));

    return m;
}

static std::shared_ptr<MemeMap> createDhabaModule(Interpreter &interpreter)
{
    auto serverState = std::make_shared<HttpServerState>();
    auto m = createHttpServerMap(serverState, interpreter);
    m->isModule = true;

    // Schema builder: t
    auto schemaMod = Value(createSchemaModule());
    m->set("t", schemaMod);
    m->set("schema", schemaMod);
    m->set("nirmaan", schemaMod);

    // Factory methods
    auto banaoFn = [&interpreter](Interpreter &, const std::vector<Value> &) -> Value {
        auto newServer = std::make_shared<HttpServerState>();
        return Value(createHttpServerMap(newServer, interpreter));
    };
    auto banaoVal = Value(std::make_shared<NativeMethodCallable>("banao", 0, 0, banaoFn));
    m->set("banao", banaoVal);
    m->set("create", banaoVal);
    m->set("__call__", banaoVal);
    m->set("__chala__", banaoVal);

    // Sub-router factory
    auto routerFn = [&interpreter](Interpreter &, const std::vector<Value> &) -> Value {
        auto newRouter = std::make_shared<HttpServerState>();
        auto rMap = createHttpServerMap(newRouter, interpreter);
        rMap->set("__is_router__", Value(true));
        return Value(rMap);
    };
    auto routerVal = Value(std::make_shared<NativeMethodCallable>("Router", 0, 0, routerFn));
    m->set("router", routerVal);
    m->set("Router", routerVal);

    // Built-in Express Middlewares
    // json()
    auto jsonMwFn = [](Interpreter &, const std::vector<Value> &) -> Value {
        auto mw = std::make_shared<NativeMethodCallable>("express.json", 3, [](Interpreter &interp, const std::vector<Value> &args) -> Value {
            Value reqVal = args[0];
            Value nextVal = args[2];
            if (reqVal.isMap()) {
                auto reqMap = reqVal.getMap();
                if (reqMap->has("headers") && reqMap->entries["headers"].deref().isMap()) {
                    auto hdrs = reqMap->entries["headers"].deref().getMap();
                    if (hdrs->has("content-type")) {
                        std::string ct = hdrs->entries["content-type"].deref().toString();
                        if (ct.find("application/json") != std::string::npos) {
                            if (reqMap->has("body") && std::holds_alternative<std::string>(reqMap->entries["body"].deref().data)) {
                                std::string bstr = std::get<std::string>(reqMap->entries["body"].deref().data);
                                if (!bstr.empty()) {
                                    try { MemeJson::Parser p(bstr); reqMap->set("body", p.parseValue()); } catch (...) {}
                                }
                            }
                        }
                    }
                }
            }
            if (nextVal.isCallable()) return nextVal.getCallable()->call(interp, {});
            return Value(true);
        });
        return Value(mw);
    };
    m->set("json", Value(std::make_shared<NativeMethodCallable>("json", 0, 0, jsonMwFn)));

    // urlencoded()
    auto urlencodedMwFn = [](Interpreter &, const std::vector<Value> &) -> Value {
        auto mw = std::make_shared<NativeMethodCallable>("express.urlencoded", 3, [](Interpreter &interp, const std::vector<Value> &args) -> Value {
            Value reqVal = args[0];
            Value nextVal = args[2];
            if (reqVal.isMap()) {
                auto reqMap = reqVal.getMap();
                if (reqMap->has("headers") && reqMap->entries["headers"].deref().isMap()) {
                    auto hdrs = reqMap->entries["headers"].deref().getMap();
                    if (hdrs->has("content-type")) {
                        std::string ct = hdrs->entries["content-type"].deref().toString();
                        if (ct.find("application/x-www-form-urlencoded") != std::string::npos) {
                            if (reqMap->has("body") && std::holds_alternative<std::string>(reqMap->entries["body"].deref().data)) {
                                reqMap->set("body", parseUrlEncoded(std::get<std::string>(reqMap->entries["body"].deref().data)));
                            }
                        }
                    }
                }
            }
            if (nextVal.isCallable()) return nextVal.getCallable()->call(interp, {});
            return Value(true);
        });
        return Value(mw);
    };
    m->set("urlencoded", Value(std::make_shared<NativeMethodCallable>("urlencoded", 0, 0, urlencodedMwFn)));

    // cors()
    auto corsMwFn = [](Interpreter &, const std::vector<Value> &) -> Value {
        auto mw = std::make_shared<NativeMethodCallable>("express.cors", 3, [](Interpreter &interp, const std::vector<Value> &args) -> Value {
            Value reqVal = args[0];
            Value resVal = args[1];
            Value nextVal = args[2];
            if (resVal.isMap()) {
                auto resMap = resVal.getMap();
                if (resMap->has("set") && resMap->entries["set"].isCallable()) {
                    resMap->entries["set"].getCallable()->call(interp, { Value("Access-Control-Allow-Origin"), Value("*") });
                    resMap->entries["set"].getCallable()->call(interp, { Value("Access-Control-Allow-Methods"), Value("GET, POST, PUT, PATCH, DELETE, OPTIONS, HEAD") });
                    resMap->entries["set"].getCallable()->call(interp, { Value("Access-Control-Allow-Headers"), Value("*") });
                }
            }
            if (reqVal.isMap() && reqVal.getMap()->has("method") && reqVal.getMap()->entries["method"].deref().toString() == "OPTIONS") {
                if (resVal.isMap() && resVal.getMap()->has("status") && resVal.getMap()->has("send")) {
                    resVal.getMap()->entries["status"].getCallable()->call(interp, { Value(204LL) });
                    return resVal.getMap()->entries["send"].getCallable()->call(interp, { Value("") });
                }
            }
            if (nextVal.isCallable()) return nextVal.getCallable()->call(interp, {});
            return Value(true);
        });
        return Value(mw);
    };
    m->set("cors", Value(std::make_shared<NativeMethodCallable>("cors", 0, 1, corsMwFn)));

    // static(dirPath)
    auto staticMwFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string dir = args[0].deref().toString();
        auto mw = std::make_shared<NativeMethodCallable>("express.static", 3, [dir](Interpreter &interp, const std::vector<Value> &a) -> Value {
            Value reqVal = a[0];
            Value resVal = a[1];
            Value nextVal = a[2];
            if (reqVal.isMap() && reqVal.getMap()->has("path")) {
                std::string p = reqVal.getMap()->entries["path"].deref().toString();
                std::string fullPath = dir + (p.front() == '/' ? p : "/" + p);
                if (std::filesystem::exists(fullPath) && std::filesystem::is_regular_file(fullPath)) {
                    if (resVal.isMap() && resVal.getMap()->has("sendFile")) {
                        return resVal.getMap()->entries["sendFile"].getCallable()->call(interp, { Value(fullPath) });
                    }
                }
            }
            if (nextVal.isCallable()) return nextVal.getCallable()->call(interp, {});
            return Value(true);
        });
        return Value(mw);
    };
    m->set("static", Value(std::make_shared<NativeMethodCallable>("static", 1, 1, staticMwFn)));

    // logger()
    auto loggerMwFn = [&interpreter](Interpreter &, const std::vector<Value> &) -> Value {
        auto mw = std::make_shared<NativeMethodCallable>("express.logger", 3, [&interpreter](Interpreter &interp, const std::vector<Value> &args) -> Value {
            Value reqVal = args[0];
            Value nextVal = args[2];
            if (reqVal.isMap()) {
                std::string m = reqVal.getMap()->has("method") ? reqVal.getMap()->entries["method"].deref().toString() : "GET";
                std::string p = reqVal.getMap()->has("path") ? reqVal.getMap()->entries["path"].deref().toString() : "/";
                interpreter.getOutputStream() << "[HTTP] " << m << " " << p << "\n";
            }
            if (nextVal.isCallable()) return nextVal.getCallable()->call(interp, {});
            return Value(true);
        });
        return Value(mw);
    };
    m->set("logger", Value(std::make_shared<NativeMethodCallable>("logger", 0, 0, loggerMwFn)));

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
    if (name == "ghusa" || name == "dhakka" || name == "push" || name == "append" || name == "daal" || name == "jod")
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
    if (name == "kul" || name == "reduce" || name == "nichod" || name == "sameto")
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
    if (name == "jodo" || name == "push" || name == "append" || name == "daal")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [list](Interpreter &, const std::vector<Value> &args) {
            list->elements.push_back(args[0]);
            return Value(list);
        }));
    }
    if (name == "hatao" || name == "pop" || name == "nikal")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [list](Interpreter &, const std::vector<Value> &) {
            if (list->elements.empty()) return Value(std::monostate{});
            Value last = list->elements.back();
            list->elements.pop_back();
            return last;
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

static Value getBytesMethod(const std::shared_ptr<MemeBytes> &bytes, const std::string &name)
{
    if (name == "length" || name == "size" || name == "lambai" || name == "lambaai" || name == "count")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [bytes](Interpreter &, const std::vector<Value> &) {
            return Value(static_cast<long long>(bytes ? bytes->data.size() : 0));
        }));
    }
    if (name == "to_string" || name == "string" || name == "decode" || name == "padh")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [bytes](Interpreter &, const std::vector<Value> &) {
            if (!bytes || bytes->data.empty()) return Value("");
            return Value(std::string(reinterpret_cast<const char*>(bytes->data.data()), bytes->data.size()));
        }));
    }
    if (name == "to_hex" || name == "hex")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [bytes](Interpreter &, const std::vector<Value> &) {
            if (!bytes || bytes->data.empty()) return Value("");
            std::string out;
            out.reserve(bytes->data.size() * 2);
            char hexChars[] = "0123456789abcdef";
            for (uint8_t b : bytes->data)
            {
                out.push_back(hexChars[(b >> 4) & 0x0F]);
                out.push_back(hexChars[b & 0x0F]);
            }
            return Value(out);
        }));
    }
    if (name == "kaat" || name == "slice" || name == "substring")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, 2, [bytes](Interpreter &, const std::vector<Value> &args) {
            auto out = std::make_shared<MemeBytes>();
            if (!bytes || bytes->data.empty()) return Value(out);
            long long sz = static_cast<long long>(bytes->data.size());
            long long start = args[0].deref().asInt();
            if (start < 0) start += sz;
            if (start < 0) start = 0;
            if (start > sz) start = sz;

            long long end = sz;
            if (args.size() > 1)
            {
                end = args[1].deref().asInt();
                if (end < 0) end += sz;
                if (end < 0) end = 0;
                if (end > sz) end = sz;
            }
            if (start < end)
            {
                out->data.assign(bytes->data.begin() + start, bytes->data.begin() + end);
            }
            return Value(out);
        }));
    }
    if (name == "jod" || name == "push" || name == "append")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [bytes](Interpreter &, const std::vector<Value> &args) {
            if (!bytes) return Value(std::monostate{});
            Value v = args[0].deref();
            if (v.isBytes())
            {
                auto b = v.getBytes();
                bytes->data.insert(bytes->data.end(), b->data.begin(), b->data.end());
            }
            else if (v.isNumeric())
            {
                bytes->data.push_back(static_cast<uint8_t>(v.asInt()));
            }
            else if (std::holds_alternative<std::string>(v.data))
            {
                const std::string &s = std::get<std::string>(v.data);
                bytes->data.insert(bytes->data.end(), s.begin(), s.end());
            }
            return Value(bytes);
        }));
    }
    if (name == "saaf" || name == "clear")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [bytes](Interpreter &, const std::vector<Value> &) {
            if (bytes) bytes->data.clear();
            return Value(std::monostate{});
        }));
    }
    if (name == "hai_kya" || name == "has" || name == "contains")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [bytes](Interpreter &, const std::vector<Value> &args) {
            if (!bytes) return Value(false);
            Value v = args[0].deref();
            if (v.isNumeric())
            {
                uint8_t target = static_cast<uint8_t>(v.asInt());
                for (uint8_t b : bytes->data)
                {
                    if (b == target) return Value(true);
                }
            }
            return Value(false);
        }));
    }
    throw std::runtime_error("Khopdi tod saale ka! 'bytes' ke paas '" + name + "' naam ka koi method nahi hai!");
}

static Value getSetMethod(const std::shared_ptr<MemeSet> &set, const std::string &name)
{
    if (name == "length" || name == "size" || name == "lambai" || name == "lambaai" || name == "count")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [set](Interpreter &, const std::vector<Value> &) {
            return Value(static_cast<long long>(set ? set->elements.size() : 0));
        }));
    }
    if (name == "jod" || name == "add" || name == "insert")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [set](Interpreter &, const std::vector<Value> &args) {
            if (set) set->add(args[0].deref());
            return Value(set);
        }));
    }
    if (name == "hata" || name == "remove" || name == "delete")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [set](Interpreter &, const std::vector<Value> &args) {
            bool rem = set ? set->remove(args[0].deref()) : false;
            return Value(rem);
        }));
    }
    if (name == "hai_kya" || name == "has" || name == "contains" || name == "andar")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [set](Interpreter &, const std::vector<Value> &args) {
            bool found = set ? set->contains(args[0].deref()) : false;
            return Value(found);
        }));
    }
    if (name == "saaf" || name == "clear")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [set](Interpreter &, const std::vector<Value> &) {
            if (set) set->clear();
            return Value(std::monostate{});
        }));
    }
    if (name == "to_list" || name == "list" || name == "elements")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [set](Interpreter &, const std::vector<Value> &) {
            auto l = std::make_shared<MemeList>();
            if (set) l->elements = set->elements;
            return Value(l);
        }));
    }
    if (name == "milap" || name == "union")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [set](Interpreter &, const std::vector<Value> &args) {
            auto out = std::make_shared<MemeSet>();
            if (set)
            {
                for (const auto &el : set->elements) out->add(el);
            }
            Value v = args[0].deref();
            if (v.isSet())
            {
                for (const auto &el : v.getSet()->elements) out->add(el);
            }
            else if (v.isList())
            {
                for (const auto &el : v.getList()->elements) out->add(el);
            }
            return Value(out);
        }));
    }
    if (name == "antar" || name == "difference")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [set](Interpreter &, const std::vector<Value> &args) {
            auto out = std::make_shared<MemeSet>();
            if (!set) return Value(out);
            Value v = args[0].deref();
            if (v.isSet())
            {
                auto other = v.getSet();
                for (const auto &el : set->elements)
                {
                    if (!other->contains(el)) out->add(el);
                }
            }
            else
            {
                for (const auto &el : set->elements) out->add(el);
            }
            return Value(out);
        }));
    }
    if (name == "sajha" || name == "intersection")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [set](Interpreter &, const std::vector<Value> &args) {
            auto out = std::make_shared<MemeSet>();
            if (!set) return Value(out);
            Value v = args[0].deref();
            if (v.isSet())
            {
                auto other = v.getSet();
                for (const auto &el : set->elements)
                {
                    if (other->contains(el)) out->add(el);
                }
            }
            return Value(out);
        }));
    }
    throw std::runtime_error("Khopdi tod saale ka! 'set' ke paas '" + name + "' naam ka koi method nahi hai!");
}

static Value getTupleMethod(const std::shared_ptr<MemeTuple> &tuple, const std::string &name)
{
    if (name == "length" || name == "size" || name == "lambai" || name == "lambaai" || name == "count")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [tuple](Interpreter &, const std::vector<Value> &) {
            return Value(static_cast<long long>(tuple ? tuple->elements.size() : 0));
        }));
    }
    if (name == "dhund" || name == "kahan" || name == "find" || name == "index_of" || name == "indexOf")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [tuple](Interpreter &, const std::vector<Value> &args) {
            if (!tuple) return Value(-1LL);
            Value target = args[0].deref();
            for (size_t i = 0; i < tuple->elements.size(); ++i)
            {
                if (tuple->elements[i] == target) return Value(static_cast<long long>(i));
            }
            return Value(-1LL);
        }));
    }
    if (name == "hai_kya" || name == "has" || name == "contains" || name == "andar")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, [tuple](Interpreter &, const std::vector<Value> &args) {
            if (!tuple) return Value(false);
            Value target = args[0].deref();
            for (size_t i = 0; i < tuple->elements.size(); ++i)
            {
                if (tuple->elements[i] == target) return Value(true);
            }
            return Value(false);
        }));
    }
    if (name == "to_list" || name == "list")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 0, [tuple](Interpreter &, const std::vector<Value> &) {
            auto l = std::make_shared<MemeList>();
            if (tuple) l->elements = tuple->elements;
            return Value(l);
        }));
    }
    if (name == "kaat" || name == "slice")
    {
        return Value(std::make_shared<NativeMethodCallable>(name, 1, 2, [tuple](Interpreter &, const std::vector<Value> &args) {
            auto out = std::make_shared<MemeTuple>();
            if (!tuple || tuple->elements.empty()) return Value(out);
            long long sz = static_cast<long long>(tuple->elements.size());
            long long start = args[0].deref().asInt();
            if (start < 0) start += sz;
            if (start < 0) start = 0;
            if (start > sz) start = sz;

            long long end = sz;
            if (args.size() > 1)
            {
                end = args[1].deref().asInt();
                if (end < 0) end += sz;
                if (end < 0) end = 0;
                if (end > sz) end = sz;
            }
            if (start < end)
            {
                out->elements.assign(tuple->elements.begin() + start, tuple->elements.begin() + end);
            }
            return Value(out);
        }));
    }
    throw std::runtime_error("Khopdi tod saale ka! 'tuple' ke paas '" + name + "' naam ka koi method nahi hai!");
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
    if (val.isBytes())
    {
        return Value(static_cast<long long>(val.getBytes()->data.size()));
    }
    if (val.isSet())
    {
        return Value(static_cast<long long>(val.getSet()->elements.size()));
    }
    if (val.isTuple())
    {
        return Value(static_cast<long long>(val.getTuple()->elements.size()));
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
    throw std::runtime_error("'size()' sirf string, list, khata, bytes, set ya tuple naapta hai, '" + val.typeName() + "' pe fita mat lagao!");
}

struct CallArgSource {
    std::shared_ptr<Environment> env = nullptr;
    std::string varName = "";
    bool isMoved = false;
};
static thread_local std::vector<CallArgSource> currentArgSources;

Value UserFunction::callSync(Interpreter &interpreter, const std::vector<Value> &args)
{
    auto argSources = currentArgSources;
    currentArgSources.clear();

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
            env->define(param.name, args[argIdx]);
            VariableSlot *slot = env->findSlot(param.name);
            if (slot)
            {
                slot->isParameter = true;
                if (argIdx < argSources.size())
                {
                    if (argSources[argIdx].isMoved)
                    {
                        slot->isOwned = true;
                    }
                    if (argSources[argIdx].env)
                    {
                        slot->sourceEnv = argSources[argIdx].env;
                        slot->sourceVarName = argSources[argIdx].varName;
                    }
                }
            }
            argIdx++;
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
            VariableSlot *slot = env->findSlot(param.name);
            if (slot)
            {
                slot->isParameter = true;
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

class BytesConstructorCallable : public Callable
{
public:
    size_t arity() const override { return 0; }
    size_t minArity() const override { return 0; }
    size_t maxArity() const override { return 1; }
    bool checkArity(size_t) const override { return true; }
    std::string toString() const override { return "<builtin bytes>"; }
    Value call(Interpreter &, const std::vector<Value> &args) override
    {
        auto b = std::make_shared<MemeBytes>();
        if (args.empty()) return Value(b);
        Value v = args[0].deref();
        if (v.isBytes())
        {
            b->data = v.getBytes()->data;
        }
        else if (v.isNumeric())
        {
            long long count = v.asInt();
            if (count > 0) b->data.resize(static_cast<size_t>(count), 0);
        }
        else if (std::holds_alternative<std::string>(v.data))
        {
            const std::string &s = std::get<std::string>(v.data);
            b->data.assign(s.begin(), s.end());
        }
        else if (v.isList())
        {
            auto l = v.getList();
            b->data.reserve(l->elements.size());
            for (const auto &el : l->elements)
            {
                b->data.push_back(static_cast<uint8_t>(el.deref().asInt()));
            }
        }
        return Value(b);
    }
};

class SetConstructorCallable : public Callable
{
public:
    size_t arity() const override { return 0; }
    size_t minArity() const override { return 0; }
    size_t maxArity() const override { return 1; }
    bool checkArity(size_t) const override { return true; }
    std::string toString() const override { return "<builtin set>"; }
    Value call(Interpreter &, const std::vector<Value> &args) override
    {
        auto s = std::make_shared<MemeSet>();
        if (args.empty()) return Value(s);
        Value v = args[0].deref();
        if (v.isList())
        {
            for (const auto &el : v.getList()->elements) s->add(el.deref());
        }
        else if (v.isSet())
        {
            for (const auto &el : v.getSet()->elements) s->add(el);
        }
        else if (v.isTuple())
        {
            for (const auto &el : v.getTuple()->elements) s->add(el.deref());
        }
        else if (std::holds_alternative<std::string>(v.data))
        {
            for (char ch : std::get<std::string>(v.data)) s->add(Value(std::string(1, ch)));
        }
        else if (v.isBytes())
        {
            for (uint8_t b : v.getBytes()->data) s->add(Value(b));
        }
        else
        {
            s->add(v);
        }
        return Value(s);
    }
};

class TupleConstructorCallable : public Callable
{
public:
    size_t arity() const override { return 0; }
    size_t minArity() const override { return 0; }
    size_t maxArity() const override { return 1; }
    bool checkArity(size_t) const override { return true; }
    std::string toString() const override { return "<builtin tuple>"; }
    Value call(Interpreter &, const std::vector<Value> &args) override
    {
        auto t = std::make_shared<MemeTuple>();
        if (args.empty()) return Value(t);
        Value v = args[0].deref();
        if (v.isList())
        {
            t->elements = v.getList()->elements;
        }
        else if (v.isTuple())
        {
            t->elements = v.getTuple()->elements;
        }
        else if (v.isSet())
        {
            t->elements = v.getSet()->elements;
        }
        else if (std::holds_alternative<std::string>(v.data))
        {
            for (char ch : std::get<std::string>(v.data)) t->elements.push_back(Value(std::string(1, ch)));
        }
        else if (v.isBytes())
        {
            for (uint8_t b : v.getBytes()->data) t->elements.push_back(Value(b));
        }
        else
        {
            t->elements.push_back(v);
        }
        return Value(t);
    }
};

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
    globals->define("int", intFunc);
    globals->define("long", intFunc);
    globals->define("bada_ank", intFunc);
    globals->define("i64", intFunc);

    // Decimal float conversion
    auto floatFunc = Value(std::make_shared<DashamlavBuiltin>());
    globals->define("point_wala", floatFunc);
    globals->define("float_bana", floatFunc);
    globals->define("dashamlav", floatFunc);
    globals->define("double", floatFunc);
    globals->define("f64", floatFunc);
    globals->define("point", floatFunc);

    // Byte (uint8) constructor
    auto byteFunc = Value(std::make_shared<NativeMethodCallable>("byte", 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        Value val = args[0].deref();
        if (val.isNumeric()) return Value(static_cast<uint8_t>(val.asInt()));
        if (std::holds_alternative<std::string>(val.data))
        {
            const std::string &s = std::get<std::string>(val.data);
            try {
                if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0)
                    return Value(static_cast<uint8_t>(std::stoul(s, nullptr, 16)));
                if (s.rfind("0b", 0) == 0 || s.rfind("0B", 0) == 0)
                    return Value(static_cast<uint8_t>(std::stoul(s.substr(2), nullptr, 2)));
                return Value(static_cast<uint8_t>(std::stoul(s)));
            } catch (...) {
                throw std::runtime_error("Utha le re baba! '" + s + "' ko byte mein kaise badlun?");
            }
        }
        throw std::runtime_error("Utha le re baba! Is type ko byte mein convert nahi kar sakte!");
    }));
    globals->define("byte", byteFunc);
    globals->define("chota_ank", byteFunc);
    globals->define("u8", byteFunc);

    // Int32 constructor
    auto int32Func = Value(std::make_shared<NativeMethodCallable>("int32", 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        Value val = args[0].deref();
        if (val.isNumeric()) return Value(val.asInt32());
        if (std::holds_alternative<std::string>(val.data))
        {
            const std::string &s = std::get<std::string>(val.data);
            try {
                if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0)
                    return Value(static_cast<int>(std::stol(s, nullptr, 16)));
                if (s.rfind("0b", 0) == 0 || s.rfind("0B", 0) == 0)
                    return Value(static_cast<int>(std::stol(s.substr(2), nullptr, 2)));
                return Value(static_cast<int>(std::stol(s)));
            } catch (...) {
                throw std::runtime_error("Utha le re baba! '" + s + "' ko int32 mein kaise badlun?");
            }
        }
        throw std::runtime_error("Utha le re baba! Is type ko int32 mein convert nahi kar sakte!");
    }));
    globals->define("int32", int32Func);
    globals->define("ank32", int32Func);
    globals->define("i32", int32Func);

    // Float32 constructor
    auto float32Func = Value(std::make_shared<NativeMethodCallable>("float32", 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        Value val = args[0].deref();
        if (val.isNumeric()) return Value(val.asFloat());
        if (std::holds_alternative<std::string>(val.data))
        {
            try {
                return Value(std::stof(std::get<std::string>(val.data)));
            } catch (...) {
                throw std::runtime_error("Daya, kuch toh gadbad hai! '" + std::get<std::string>(val.data) + "' ko float32 mein convert nahi kar sakte!");
            }
        }
        throw std::runtime_error("Daya, kuch toh gadbad hai! Is type ko float32 mein convert nahi kar sakte!");
    }));
    globals->define("float32", float32Func);
    globals->define("f32", float32Func);
    globals->define("float", float32Func);

    // Char constructor
    auto charFunc = Value(std::make_shared<NativeMethodCallable>("char", 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        Value val = args[0].deref();
        if (val.isChar()) return val;
        if (val.isNumeric()) return Value(static_cast<char>(val.asInt()));
        if (std::holds_alternative<std::string>(val.data))
        {
            const std::string &s = std::get<std::string>(val.data);
            return Value(s.empty() ? '\0' : s[0]);
        }
        throw std::runtime_error("Utha le re baba! Is type ko char mein convert nahi kar sakte!");
    }));
    globals->define("char", charFunc);
    globals->define("akshar", charFunc);

    // String constructor
    auto strFunc = Value(std::make_shared<NativeMethodCallable>("string", 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        return Value(args[0].deref().toString());
    }));
    globals->define("string", strFunc);
    globals->define("str", strFunc);
    globals->define("shabd", strFunc);
    globals->define("bol", strFunc);

    // Bytes constructor
    auto bytesFunc = Value(std::make_shared<BytesConstructorCallable>());
    globals->define("bytes", bytesFunc);
    globals->define("kaccha_data", bytesFunc);
    globals->define("bytearray", bytesFunc);

    // Set constructor
    auto setFunc = Value(std::make_shared<SetConstructorCallable>());
    globals->define("set", setFunc);
    globals->define("jhund", setFunc);
    globals->define("mandali", setFunc);

    // Tuple constructor
    auto tupleFunc = Value(std::make_shared<TupleConstructorCallable>());
    globals->define("tuple", tupleFunc);
    globals->define("jodi", tupleFunc);

    // Explicit cloning (nakal / clone / copy)
    auto nakalFunc = Value(std::make_shared<NakalBuiltin>());
    globals->define("nakal", nakalFunc);
    globals->define("clone", nakalFunc);
    globals->define("copy", nakalFunc);

    // Standard Library Modules: math, datetime, random, stats, crypto, regex, itertools, collections, csv, test
    auto hisabMod = Value(createHisabModule());
    globals->define("hisab", hisabMod);
    globals->define("math", hisabMod);
    importedModules["hisab"] = hisabMod;
    importedModules["math"] = hisabMod;

    auto waqtMod = Value(createWaqtModule());
    globals->define("waqt", waqtMod);
    globals->define("datetime", waqtMod);
    globals->define("time", waqtMod);
    globals->define("samay", waqtMod);
    importedModules["waqt"] = waqtMod;
    importedModules["datetime"] = waqtMod;
    importedModules["time"] = waqtMod;
    importedModules["samay"] = waqtMod;

    auto randomMod = Value(createRandomModule());
    globals->define("random", randomMod);
    globals->define("tukka", randomMod);
    importedModules["random"] = randomMod;
    importedModules["tukka"] = randomMod;

    auto statsMod = Value(createStatisticsModule());
    globals->define("statistics", statsMod);
    globals->define("stats", statsMod);
    globals->define("aankde", statsMod);
    importedModules["statistics"] = statsMod;
    importedModules["stats"] = statsMod;
    importedModules["aankde"] = statsMod;

    auto cryptoMod = Value(createCryptoModule());
    globals->define("crypto", cryptoMod);
    globals->define("hashlib", cryptoMod);
    globals->define("gupt", cryptoMod);
    importedModules["crypto"] = cryptoMod;
    importedModules["hashlib"] = cryptoMod;
    importedModules["gupt"] = cryptoMod;

    auto reMod = Value(createRegexModule());
    globals->define("regex", reMod);
    globals->define("re", reMod);
    globals->define("dhundo", reMod);
    importedModules["regex"] = reMod;
    importedModules["re"] = reMod;
    importedModules["dhundo"] = reMod;

    auto iterMod = Value(createItertoolsModule());
    globals->define("itertools", iterMod);
    globals->define("jugaad_tools", iterMod);
    importedModules["itertools"] = iterMod;
    importedModules["jugaad_tools"] = iterMod;
    globals->define("range", iterMod.getMap()->entries["range"]);
    globals->define("zip", iterMod.getMap()->entries["zip"]);
    globals->define("enumerate", iterMod.getMap()->entries["enumerate"]);

    auto collMod = Value(createCollectionsModule());
    globals->define("collections", collMod);
    globals->define("mandali", collMod);
    importedModules["collections"] = collMod;
    importedModules["mandali"] = collMod;

    auto csvMod = Value(createCsvModule());
    globals->define("csv", csvMod);
    importedModules["csv"] = csvMod;

    auto testMod = Value(createTestModule());
    globals->define("test", testMod);
    globals->define("pariksha", testMod);
    importedModules["test"] = testMod;
    importedModules["pariksha"] = testMod;
    importedModules["assert"] = testMod;
    globals->define("assert", testMod.getMap()->entries["assert"]);
    globals->define("assert_eq", testMod.getMap()->entries["assert_eq"]);
    globals->define("assert_ne", testMod.getMap()->entries["assert_ne"]);
    globals->define("assert_throws", testMod.getMap()->entries["assert_throws"]);

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
    globals->define("express", dhabaMod);
    globals->define("elysia", dhabaMod);
    globals->define("Elysia", dhabaMod);

    Value schemaVal = dhabaMod.getMap()->entries["t"];
    globals->define("t", schemaVal);
    globals->define("schema", schemaVal);
    globals->define("nirmaan", schemaVal);

    importedModules["dhaba"] = dhabaMod;
    importedModules["server"] = dhabaMod;
    importedModules["api"] = dhabaMod;
    importedModules["express"] = dhabaMod;
    importedModules["elysia"] = dhabaMod;
    importedModules["Elysia"] = dhabaMod;
    importedModules["t"] = schemaVal;
    importedModules["schema"] = schemaVal;

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
        dropMethod = instance->getClass()->findMethod("khatam");
    }
    if (!dropMethod)
    {
        dropMethod = instance->getClass()->findMethod("visarjan");
    }
    if (!dropMethod)
    {
        dropMethod = instance->getClass()->findMethod("drop");
    }
    if (!dropMethod)
    {
        dropMethod = instance->getClass()->findMethod("dispose");
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
    if (!activeBorrows.empty())
    {
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
    }

    // 2. Drop instances owned by this environment scope (if not moved)
    if (env)
    {
        for (auto &[name, slot] : env->getSlots())
        {
            if (name == "khud" || name == "this" || name == "apun" || name == "apna" || name == "self")
                continue;
            if (slot.isMoved)
                continue;
            if (!slot.value.isInstance())
                continue;

            bool shouldDrop = false;
            if (slot.isOwned)
            {
                shouldDrop = true;
            }
            else if (!slot.isParameter && slot.sourceEnv == nullptr)
            {
                shouldDrop = true;
            }

            if (shouldDrop)
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
    if (v.isBytes())
    {
        auto bytes = v.getBytes();
        return Value(std::make_shared<MemeBytes>(bytes ? bytes->data : std::vector<uint8_t>{}));
    }
    if (v.isSet())
    {
        auto set = v.getSet();
        auto newSet = std::make_shared<MemeSet>();
        if (set)
        {
            for (const auto &elem : set->elements)
            {
                newSet->add(cloneValue(elem));
            }
        }
        return Value(newSet);
    }
    if (v.isTuple())
    {
        auto tup = v.getTuple();
        auto newTup = std::make_shared<MemeTuple>();
        if (tup)
        {
            newTup->elements.reserve(tup->elements.size());
            for (const auto &elem : tup->elements)
            {
                newTup->elements.push_back(cloneValue(elem));
            }
        }
        return Value(newTup);
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

    // Check lib/<path> or lib/<path>.meme or lib/<path>/index.meme
    if (!std::filesystem::exists(canonicalPath))
    {
        std::filesystem::path libPath = (path.rfind("lib/", 0) == 0)
            ? std::filesystem::path(path)
            : (std::filesystem::path("lib") / path);
        if (std::filesystem::exists(libPath))
        {
            canonicalPath = libPath;
        }
        else if (std::filesystem::exists(libPath.string() + ".meme"))
        {
            canonicalPath = libPath.string() + ".meme";
        }
        else if (std::filesystem::exists(libPath / "index.meme"))
        {
            canonicalPath = libPath / "index.meme";
        }
    }

    // Check directly from working directory
    if (!std::filesystem::exists(canonicalPath))
    {
        std::filesystem::path cwdPath(path);
        if (std::filesystem::exists(cwdPath))
        {
            canonicalPath = cwdPath;
        }
        else if (std::filesystem::exists(cwdPath.string() + ".meme"))
        {
            canonicalPath = cwdPath.string() + ".meme";
        }
        else if (std::filesystem::exists(cwdPath / "index.meme"))
        {
            canonicalPath = cwdPath / "index.meme";
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
            val = evaluate(v->initializer.get());
        }
        else
        {
            val = Value(std::monostate{});
        }
        environment->define(v->name, std::move(val), v->isOwned, v->isMut, v->isConst,
                            v->typeAnnotation ? v->typeAnnotation->toString() : "");
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

    // 3. Print statement: `dikha a, b;` with custom options
    if (auto *p = dynamic_cast<const PrintStmt *>(stmt))
    {
        std::string sep = " ";
        if (p->sepExpr)
        {
            sep = evaluate(p->sepExpr.get()).deref().toString();
        }

        std::string endChar = p->noNewline ? "" : "\n";
        if (p->endExpr)
        {
            endChar = evaluate(p->endExpr.get()).deref().toString();
        }

        std::string stylePrefix;
        std::string styleReset;
        if (p->styleExpr)
        {
            std::string styleStr = evaluate(p->styleExpr.get()).deref().toString();
            stylePrefix = resolveStyleAnsi(styleStr);
            if (!stylePrefix.empty())
            {
                styleReset = "\033[0m";
            }
        }

        if (!stylePrefix.empty())
        {
            out << stylePrefix;
        }

        for (size_t i = 0; i < p->args.size(); ++i)
        {
            if (i > 0)
                out << sep;
            Value val = evaluate(p->args[i].get());
            out << formatRichText(stringify(val));
        }

        if (!styleReset.empty())
        {
            out << styleReset;
        }
        out << endChar;
        return;
    }

    // Drop statement: `khatam expr;`, `drop expr;`, `visarjan expr;`, `alvida expr;`, `dispose expr;`
    if (auto *ds = dynamic_cast<const DropStmt *>(stmt))
    {
        if (auto *ident = dynamic_cast<const IdentifierExpr *>(ds->expr.get()))
        {
            VariableSlot *slot = environment->findSlot(ident->name);
            if (!slot)
            {
                throw std::runtime_error(
                    "Rasode mein kaun tha?! Ye '" + ident->name + "' bina bataye kahan se aaya? Pehle 'pakad " + ident->name + ";' se declare kar!");
            }
            if (slot->isMoved)
            {
                throw std::runtime_error(
                    "Pehli fursat mein nikal! Variable '" + ident->name + "' ka maal move ho chuka hai! Ownership transfer ke baad use karna mana hai!");
            }
            if (slot->hasExclusiveBorrow || slot->sharedBorrows > 0)
            {
                throw std::runtime_error(
                    "Khatam nahi hua abhi! '" + ident->name + "' udhaar pe hai, drop nahi kar sakte!");
            }
            if (slot->value.isInstance())
            {
                dropInstance(slot->value.getInstance());
            }
            slot->isMoved = true;
            return;
        }

        if (auto *deref = dynamic_cast<const DerefExpr *>(ds->expr.get()))
        {
            Value ptrVal = evaluate(deref->operand.get()).deref();
            if (!ptrVal.isPointer() || ptrVal.getPointer()->isNull())
            {
                throw std::runtime_error("Chuna laga diya! Null pointer ko drop nahi kar sakte!");
            }
            auto ptr = ptrVal.getPointer();
            if (ptr->env && !ptr->varName.empty())
            {
                VariableSlot *slot = ptr->env->findSlot(ptr->varName);
                if (slot)
                {
                    if (slot->isMoved)
                    {
                        throw std::runtime_error("Pehli fursat mein nikal! Variable '" + ptr->varName + "' ka maal move ho chuka hai!");
                    }
                    if (slot->value.isInstance())
                    {
                        dropInstance(slot->value.getInstance());
                    }
                    slot->isMoved = true;
                    return;
                }
            }
            if (ptr->instance)
            {
                dropInstance(ptr->instance);
                return;
            }
        }

        Value val = evaluate(ds->expr.get());
        if (val.isInstance())
        {
            dropInstance(val.getInstance());
        }
        return;
    }

    // Scope Block statement: `daayra { ... }` or `scope { ... }`
    if (auto *sb = dynamic_cast<const ScopeBlockStmt *>(stmt))
    {
        executeBlock(sb->statements, std::make_shared<Environment>(environment, currentScopeDepth + 1));
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
        if (auto *b = dynamic_cast<const BlockStmt *>(w->body.get()))
        {
            auto loopEnv = std::make_shared<Environment>(environment, currentScopeDepth + 1);
            enterScope();
            int scopeDepth = currentScopeDepth;
            auto prev = environment;
            environment = loopEnv;
            try
            {
                while (evaluate(w->condition.get()).isTruthy())
                {
                    try
                    {
                        for (const auto &s : b->statements)
                        {
                            execute(s.get());
                        }
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
                cleanupScope(loopEnv, scopeDepth);
                environment = prev;
            }
            catch (...)
            {
                cleanupScope(loopEnv, scopeDepth);
                environment = prev;
                throw;
            }
            return;
        }

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

        if (colVal.isBytes())
        {
            auto bytes = colVal.getBytes();
            for (uint8_t b : bytes->data)
            {
                auto loopEnv = std::make_shared<Environment>(environment, currentScopeDepth + 1);
                loopEnv->define(gm->varName, Value(b));
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

        if (colVal.isSet())
        {
            auto set = colVal.getSet();
            for (const auto &elem : set->elements)
            {
                auto loopEnv = std::make_shared<Environment>(environment, currentScopeDepth + 1);
                loopEnv->define(gm->varName, elem);
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

        if (colVal.isTuple())
        {
            auto tup = colVal.getTuple();
            for (const auto &elem : tup->elements)
            {
                auto loopEnv = std::make_shared<Environment>(environment, currentScopeDepth + 1);
                loopEnv->define(gm->varName, elem);
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

        throw std::runtime_error("Ghumaane ke liye list, khata, string, bytes, set ya tuple chahiye! '" + colVal.typeName() + "' ko kaise ghumaoge?");
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
        if (f->isOverload)
        {
            environment->defineOverload(f->name, func);
        }
        else
        {
            auto *slot = environment->findSlot(f->name);
            if (slot && slot->value.isCallable() &&
                std::dynamic_pointer_cast<OverloadSet>(slot->value.getCallable()))
            {
                environment->defineOverload(f->name, func);
            }
            else
            {
                environment->define(f->name, Value(func));
            }
        }
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

        std::unordered_map<std::string, std::shared_ptr<Callable>> methods;
        for (const auto &method : c->methods)
        {
            auto func = std::make_shared<UserFunction>(method.get(), environment);
            auto it = methods.find(method->name);
            if (it != methods.end())
            {
                if (auto os = std::dynamic_pointer_cast<OverloadSet>(it->second))
                {
                    os->addCandidate(func);
                }
                else if (auto prevFunc = std::dynamic_pointer_cast<UserFunction>(it->second))
                {
                    auto os = std::make_shared<OverloadSet>(method->name);
                    os->addCandidate(prevFunc);
                    os->addCandidate(func);
                    methods[method->name] = os;
                }
                else
                {
                    methods[method->name] = func;
                }
            }
            else
            {
                if (method->isOverload)
                {
                    auto os = std::make_shared<OverloadSet>(method->name);
                    os->addCandidate(func);
                    methods[method->name] = os;
                }
                else
                {
                    methods[method->name] = func;
                }
            }
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

static Value dereferencePointerHelper(const Value &inputVal)
{
    Value ptrVal = inputVal;
    if (ptrVal.isNil() || ptrVal.isNull())
    {
        throw std::runtime_error("Chuna laga diya! Null pointer dereference: nalla pointer ko dereference nahi kar sakte!");
    }

    if (ptrVal.isReference())
    {
        ptrVal = ptrVal.deref();
    }

    if (!ptrVal.isPointer())
    {
        throw std::runtime_error(
            "Aukaat ke bahar! Dereference '*' sirf pointer type par chal sakta hai, mila '" +
            ptrVal.typeName() + "'!");
    }

    auto ptr = ptrVal.asPointer();
    if (!ptr || ptr->isNull())
    {
        throw std::runtime_error("Chuna laga diya! Null pointer dereference: nalla pointer ko dereference nahi kar sakte!");
    }

    // Read 1: Custom getter
    if (ptr->customGetter)
    {
        return ptr->customGetter();
    }

    // Read 2: Struct / Class instance field
    if (ptr->instance && !ptr->fieldName.empty())
    {
        return ptr->instance->get(ptr->fieldName);
    }

    // Read 3: Environment variable slot
    if (ptr->env && !ptr->varName.empty())
    {
        VariableSlot *slot = ptr->env->findSlot(ptr->varName);
        if (!slot)
        {
            throw std::runtime_error(
                "Chuna laga diya! Pointer jis variable ko point kar raha tha ('" +
                ptr->varName + "'), wo ab exist nahi karta!");
        }
        if (slot->isMoved)
        {
            throw std::runtime_error(
                "Pehli fursat mein nikal! '" + ptr->varName +
                "' ki value move ho chuki hai! Ownership transfer ke baad use karna mana hai!");
        }
        return slot->value;
    }

    // Read 4: Raw address
    if (ptr->rawAddress)
    {
        return *ptr->rawAddress;
    }

    throw std::runtime_error("Chuna laga diya! Null pointer dereference: nalla pointer ko dereference nahi kar sakte!");
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

    // Identifier lookup
    if (auto *e = dynamic_cast<const IdentifierExpr *>(expr))
        return environment->get(e->name);

    // Assignment: `x = value`
    if (auto *e = dynamic_cast<const AssignExpr *>(expr))
    {
        VariableSlot *slot = environment->findSlot(e->name);
        Value val = evaluate(e->value.get());

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

        environment->assignToSlot(slot, val, e->name);
        return val;
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

        // Fast path for raw 64-bit integers: zero dynamic deref, zero instance lookups, direct ALU ops
        if (std::holds_alternative<long long>(left.data) && std::holds_alternative<long long>(right.data))
        {
            long long lval = std::get<long long>(left.data);
            long long rval = std::get<long long>(right.data);
            switch (e->op.type)
            {
            case TokenType::PLUS: {
                long long res = 0;
                if (__builtin_add_overflow(lval, rval, &res)) {
                    throw std::runtime_error("25 din mein paisa double karte-karte integer overflow ho gaya! Aukaat ke bahar chala gaya jod.");
                }
                return Value(res);
            }
            case TokenType::MINUS: {
                long long res = 0;
                if (__builtin_sub_overflow(lval, rval, &res)) {
                    throw std::runtime_error("Ghatate-ghatate patal lok pahunch gaye! Integer underflow/overflow ho gaya.");
                }
                return Value(res);
            }
            case TokenType::MULTIPLY: {
                long long res = 0;
                if (__builtin_mul_overflow(lval, rval, &res)) {
                    throw std::runtime_error("Hisab-kitab aukaat ke bahar! Guna (*) karte-karte integer overflow ho gaya.");
                }
                return Value(res);
            }
            case TokenType::DIVIDE: {
                if (rval == 0) {
                    throw std::runtime_error("Arre re re! 0 se divide karoge toh Brahmaand phat jayega! Zero division mana hai!");
                }
                if (lval == std::numeric_limits<long long>::min() && rval == -1) {
                    throw std::runtime_error("Integer overflow ho gaya! Is hisaab ko sambhalne ki aukaat nahi hai.");
                }
                return Value(lval / rval);
            }
            case TokenType::MOD: {
                if (rval == 0) {
                    throw std::runtime_error("0 se modulo (%) karne chale ho? Maths teacher ko heart attack aa jayega!");
                }
                return Value(lval % rval);
            }
            case TokenType::LESS_THAN:
                return Value(lval < rval);
            case TokenType::LESS_THAN_EQUAL:
                return Value(lval <= rval);
            case TokenType::GREATER_THAN:
                return Value(lval > rval);
            case TokenType::GREATER_THAN_EQUAL:
                return Value(lval >= rval);
            case TokenType::EQUAL:
                return Value(lval == rval);
            case TokenType::NOT_EQUAL:
                return Value(lval != rval);
            default:
                break;
            }
        }

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
                auto neMethod = dl.getInstance()->getClass()->findMethod("__alag__");
                if (!neMethod) neMethod = dl.getInstance()->getClass()->findMethod("__ne__");
                if (neMethod)
                {
                    BoundMethod bound(dl.getInstance(), neMethod);
                    return bound.call(*this, {dr});
                }
                auto method = dl.getInstance()->getClass()->findMethod("__barabar__");
                if (!method) method = dl.getInstance()->getClass()->findMethod("__eq__");
                if (method)
                {
                    BoundMethod bound(dl.getInstance(), method);
                    Value res = bound.call(*this, {dr});
                    return Value(!res.isTruthy());
                }
            }
            if (dr.isInstance())
            {
                auto neMethod = dr.getInstance()->getClass()->findMethod("__alag__");
                if (!neMethod) neMethod = dr.getInstance()->getClass()->findMethod("__ne__");
                if (neMethod)
                {
                    BoundMethod bound(dr.getInstance(), neMethod);
                    return bound.call(*this, {dl});
                }
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
            if (dr.isInstance())
            {
                auto method = dr.getInstance()->getClass()->findMethod("__bada__");
                if (!method) method = dr.getInstance()->getClass()->findMethod("__gt__");
                if (method)
                {
                    BoundMethod bound(dr.getInstance(), method);
                    return bound.call(*this, {dl});
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
                auto leMethod = dl.getInstance()->getClass()->findMethod("__chota_barabar__");
                if (!leMethod) leMethod = dl.getInstance()->getClass()->findMethod("__le__");
                if (leMethod)
                {
                    BoundMethod bound(dl.getInstance(), leMethod);
                    return bound.call(*this, {dr});
                }
            }
            if (dr.isInstance())
            {
                auto geMethod = dr.getInstance()->getClass()->findMethod("__bada_barabar__");
                if (!geMethod) geMethod = dr.getInstance()->getClass()->findMethod("__ge__");
                if (geMethod)
                {
                    BoundMethod bound(dr.getInstance(), geMethod);
                    return bound.call(*this, {dl});
                }
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
                auto geMethod = dl.getInstance()->getClass()->findMethod("__bada_barabar__");
                if (!geMethod) geMethod = dl.getInstance()->getClass()->findMethod("__ge__");
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
            if (dr.isInstance())
            {
                auto leMethod = dr.getInstance()->getClass()->findMethod("__chota_barabar__");
                if (!leMethod) leMethod = dr.getInstance()->getClass()->findMethod("__le__");
                if (leMethod)
                {
                    BoundMethod bound(dr.getInstance(), leMethod);
                    return bound.call(*this, {dl});
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
        else if (callee.isMap())
        {
            auto map = callee.getMap();
            if (map->has("__call__") || map->has("__chala__"))
            {
                std::string callKey = map->has("__call__") ? "__call__" : "__chala__";
                Value mVal = map->entries[callKey].deref();
                if (mVal.isCallable())
                {
                    function = mVal.getCallable();
                }
            }
        }

        if (!function)
        {
            throw std::runtime_error("Beta, tumse na ho payega! '" + callee.toString() + "' koi jugaad (function) thodi hai jo call kar rahe ho!");
        }

        std::vector<CallArgSource> argSources;
        std::vector<Value> arguments;
        for (const auto &arg : e->args)
        {
            CallArgSource src;
            if (auto *mv = dynamic_cast<const MoveExpr *>(arg.get()))
            {
                src.isMoved = true;
                if (auto *ident = dynamic_cast<const IdentifierExpr *>(mv->operand.get()))
                {
                    src.env = environment->findOwnerEnv(ident->name);
                    src.varName = ident->name;
                }
            }
            else if (auto *ident = dynamic_cast<const IdentifierExpr *>(arg.get()))
            {
                src.env = environment->findOwnerEnv(ident->name);
                src.varName = ident->name;
            }
            argSources.push_back(src);

            if (auto *sp = dynamic_cast<const SpreadExpr *>(arg.get()))
            {
                Value val = evaluate(sp->expr.get());
                val = val.deref();
                if (val.isPointer())
                {
                    arguments.push_back(dereferencePointerHelper(val));
                    continue;
                }
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

            arguments.push_back(evaluate(arg.get()));
        }

        if (dynamic_cast<OverloadSet *>(function.get()) == nullptr && !function->checkArity(arguments.size()))
        {
            int line = (e->line > 0) ? e->line : currentLine;
            throw std::runtime_error(
                "Line " + std::to_string(line) + ": Gaddari korbe! Jugaad ko kam se kam " +
                std::to_string(function->minArity()) + " arguments chahiye the, par tumne " +
                std::to_string(arguments.size()) + " pakda diye!");
        }

        currentArgSources = std::move(argSources);
        return function->call(*this, arguments);
    }


    if (auto *e = dynamic_cast<const FloatExpr *>(expr))
    {
        if (e->isFloat32) return Value(static_cast<float>(e->value));
        return Value(e->value);
    }

    if (auto *e = dynamic_cast<const ByteExpr *>(expr))
        return Value(e->value);

    if (auto *e = dynamic_cast<const BytesExpr *>(expr))
        return Value(std::make_shared<MemeBytes>(e->value));

    if (auto *e = dynamic_cast<const StringExpr *>(expr))
        return Value(e->value);

    // F-String expression: `f"Hello {name}"`
    if (auto *fs = dynamic_cast<const FStringExpr *>(expr))
    {
        std::string result = "";
        for (const auto &part : fs->parts)
        {
            Value val = evaluate(part.get());
            Value d = val.deref();
            if (std::holds_alternative<bool>(d.data))
            {
                result += std::get<bool>(d.data) ? "true" : "false";
            }
            else
            {
                result += stringify(val);
            }
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
            if (slot->isConst)
            {
                throw std::runtime_error(
                    "Shanti gadadhari Bheem! Constant / atal variable '" + ident->name + "' ka badal (mutable) udhaar nahi le sakte!");
            }

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

    // Move expression: `rawana x` or `move x`
    if (auto *m = dynamic_cast<const MoveExpr *>(expr))
    {
        if (auto *ident = dynamic_cast<const IdentifierExpr *>(m->operand.get()))
        {
            VariableSlot *slot = environment->findSlot(ident->name);
            if (!slot)
            {
                throw std::runtime_error(
                    "Rasode mein kaun tha?! Ye '" + ident->name + "' bina bataye kahan se aaya? Pehle 'pakad " + ident->name + ";' se declare kar!");
            }
            if (slot->isMoved)
            {
                throw std::runtime_error(
                    "Pehli fursat mein nikal! Variable '" + ident->name + "' ka maal move ho chuka hai! Ownership transfer ke baad use karna mana hai!");
            }
            if (slot->hasExclusiveBorrow || slot->sharedBorrows > 0)
            {
                throw std::runtime_error(
                    "Khatam nahi hua abhi! '" + ident->name + "' udhaar pe hai, move nahi kar sakte!");
            }
            slot->isMoved = true;
            return slot->value;
        }
        return evaluate(m->operand.get());
    }

    // Address-of expression: `&expr` or `patta expr`
    if (auto *addrOf = dynamic_cast<const AddressOfExpr *>(expr))
    {
        if (!addrOf->operand)
        {
            throw std::runtime_error("Doglapan! AddressOf ka operand gayab hai!");
        }

        // Case 1: Variable identifier `&x`
        if (auto *ident = dynamic_cast<const IdentifierExpr *>(addrOf->operand.get()))
        {
            auto ownerEnv = environment->findOwnerEnv(ident->name);
            VariableSlot *slot = environment->findSlot(ident->name);
            if (!slot || !ownerEnv)
            {
                throw std::runtime_error(
                    "Rasode mein kaun tha?! Ye '" + ident->name +
                    "' bina bataye kahan se aaya? Pehle 'pakad " + ident->name + ";' se declare kar!");
            }

            if (slot->isMoved)
            {
                throw std::runtime_error(
                    "Pehli fursat mein nikal! '" + ident->name +
                    "' ki value move ho chuki hai! Ownership transfer ke baad use karna mana hai!");
            }

            if (slot->sourceEnv && !slot->sourceVarName.empty())
            {
                auto srcOwner = slot->sourceEnv->findOwnerEnv(slot->sourceVarName);
                auto srcSlot = slot->sourceEnv->findSlot(slot->sourceVarName);
                if (srcOwner && srcSlot)
                {
                    bool isMut = !srcSlot->isConst;
                    Pointer ptr(srcOwner, slot->sourceVarName, isMut, &srcSlot->value);
                    return Value(ptr);
                }
            }

            bool isMut = !slot->isConst;
            Pointer ptr(ownerEnv, ident->name, isMut, &slot->value);
            return Value(ptr);
        }

        // Case 2: Object field access `&obj.field`
        if (auto *g = dynamic_cast<const GetExpr *>(addrOf->operand.get()))
        {
            Value objVal = evaluate(g->object.get()).deref();
            if (!objVal.isInstance())
            {
                throw std::runtime_error(
                    "Doglapan! Struct field ka pointer lene ke liye instance hona chahiye, mila: " + objVal.typeName());
            }

            auto inst = objVal.getInstance();
            if (!inst)
            {
                throw std::runtime_error("Chuna laga diya! Null instance ke field ka address nahi le sakte!");
            }

            uintptr_t fieldAddr = reinterpret_cast<uintptr_t>(inst.get()) ^
                                  (static_cast<uintptr_t>(std::hash<std::string>{}(g->name)) << 3);
            Pointer ptr(inst, g->name, true);
            ptr.address = fieldAddr;
            return Value(ptr);
        }

        // Case 3: Array / Map indexing `&arr[i]`
        if (auto *ix = dynamic_cast<const IndexExpr *>(addrOf->operand.get()))
        {
            Value target = evaluate(ix->target.get()).deref();
            Value idxVal = evaluate(ix->index.get()).deref();

            if (target.isList())
            {
                auto list = target.getList();
                if (!idxVal.isNumeric())
                {
                    throw std::runtime_error("Array index number hona chahiye!");
                }
                long long idx = idxVal.asInt();
                long long sz = list ? static_cast<long long>(list->elements.size()) : 0;
                if (idx < 0) idx += sz;
                if (idx < 0 || idx >= sz)
                {
                    throw std::runtime_error("Aukaat ke bahar! Index out of bounds in address-of!");
                }

                Pointer ptr;
                ptr.rawAddress = &list->elements[idx];
                ptr.address = reinterpret_cast<uintptr_t>(&list->elements[idx]);
                ptr.isMut = true;
                ptr.customGetter = [list, idx]() -> Value {
                    return list->elements[idx];
                };
                ptr.customSetter = [list, idx](Value newVal) {
                    list->elements[idx] = std::move(newVal);
                };
                return Value(ptr);
            }

            if (target.isMap())
            {
                auto map = target.getMap();
                std::string key = idxVal.toString();
                Pointer ptr;
                ptr.address = reinterpret_cast<uintptr_t>(map.get()) ^
                              (static_cast<uintptr_t>(std::hash<std::string>{}(key)) << 3);
                ptr.isMut = true;
                ptr.customGetter = [map, key]() -> Value {
                    return map->get(key);
                };
                ptr.customSetter = [map, key](Value newVal) {
                    map->set(key, std::move(newVal));
                };
                return Value(ptr);
            }

            throw std::runtime_error("Doglapan! Indexing target ka address nahi liya ja sakta!");
        }

        // Case 4: Dereference address-of: `&*p` cancels to p
        if (auto *d = dynamic_cast<const DerefExpr *>(addrOf->operand.get()))
        {
            return evaluate(d->operand.get());
        }

        // Case 5: Non-lvalue rvalue temporary
        throw std::runtime_error(
            "Doglapan! Sirf variable ya memory location ka patta liya ja sakta hai, temporary rvalue ka nahi!");
    }

    // Dereference expression: `*expr` or `asli expr`
    if (auto *deref = dynamic_cast<const DerefExpr *>(expr))
    {
        if (!deref->operand)
        {
            throw std::runtime_error("Chuna laga diya! Dereference ka operand gayab hai!");
        }

        Value ptrVal = evaluate(deref->operand.get());
        return dereferencePointerHelper(ptrVal);
    }

    // Dereference assignment: `*ptr = val` or `asli ptr = val`
    if (auto *derefAssign = dynamic_cast<const DerefAssignExpr *>(expr))
    {
        if (!derefAssign->pointer || !derefAssign->value)
        {
            throw std::runtime_error("Aukaat ke bahar! Deref assignment incomplete hai!");
        }

        Value ptrVal = evaluate(derefAssign->pointer.get());

        // Null pointer check
        if (ptrVal.isNil() || ptrVal.isNull())
        {
            throw std::runtime_error("Chuna laga diya! Null pointer dereference: nalla pointer pe assign nahi kar sakte!");
        }

        if (ptrVal.isReference())
        {
            ptrVal = ptrVal.deref();
        }

        if (!ptrVal.isPointer())
        {
            throw std::runtime_error(
                "Aukaat ke bahar! Deref assignment sirf pointer pe chal sakti hai, mila '" +
                ptrVal.typeName() + "'!");
        }

        auto ptr = ptrVal.asPointer();
        if (!ptr || ptr->isNull())
        {
            throw std::runtime_error("Chuna laga diya! Null pointer dereference: nalla pointer pe assign nahi kar sakte!");
        }

        if (!ptr->isMut)
        {
            throw std::runtime_error("Doglapan! Constant pointer (atal/const) ke target ko modify nahi kar sakte!");
        }

        // Evaluate RHS value
        Value newVal;
        if (auto *ident = dynamic_cast<const IdentifierExpr *>(derefAssign->value.get()))
        {
            newVal = moveVariable(ident->name);
        }
        else
        {
            newVal = evaluate(derefAssign->value.get());
        }

        // Write 1: Custom setter
        if (ptr->customSetter)
        {
            ptr->customSetter(newVal);
            return newVal;
        }

        // Write 2: Struct / Class instance field
        if (ptr->instance && !ptr->fieldName.empty())
        {
            ptr->instance->set(ptr->fieldName, newVal);
            return newVal;
        }

        // Write 3: Environment variable slot
        if (ptr->env && !ptr->varName.empty())
        {
            VariableSlot *slot = ptr->env->findSlot(ptr->varName);
            if (!slot)
            {
                throw std::runtime_error(
                    "Chuna laga diya! Pointer jis variable ko point kar raha tha ('" +
                    ptr->varName + "'), wo ab exist nahi karta!");
            }
            if (slot->isConst)
            {
                throw std::runtime_error(
                    "Shanti gadadhari Bheem! Constant / atal variable '" + ptr->varName + "' ko modify nahi kar sakte!");
            }
            slot->value = newVal;
            slot->isMoved = false;
            return newVal;
        }

        // Write 4: Raw address
        if (ptr->rawAddress)
        {
            *ptr->rawAddress = newVal;
            return newVal;
        }

        throw std::runtime_error("Chuna laga diya! Invalid pointer assignment!");
    }

    // Unary expression: `-x`, `!x`
    if (auto *e = dynamic_cast<const UnaryExpr *>(expr))
    {
        Value operand = evaluate(e->operand.get());
        Value dop = operand.deref();
        if (dop.isInstance())
        {
            auto inst = dop.getInstance();
            if (e->op.type == TokenType::MINUS)
            {
                auto method = inst->getClass()->findMethod("__ulta__");
                if (!method) method = inst->getClass()->findMethod("__neg__");
                if (method)
                {
                    BoundMethod bound(inst, method);
                    return bound.call(*this, {});
                }
            }
            if (e->op.type == TokenType::NOT)
            {
                auto method = inst->getClass()->findMethod("__ulta__");
                if (!method) method = inst->getClass()->findMethod("__not__");
                if (!method) method = inst->getClass()->findMethod("__invert__");
                if (method)
                {
                    BoundMethod bound(inst, method);
                    return bound.call(*this, {});
                }
            }
        }
        if (e->op.type == TokenType::MINUS)
            return negate(operand);
        if (e->op.type == TokenType::NOT)
            return logicalNot(operand);

        throw std::runtime_error("Ye operator '" + e->op.value + "' unary mein nahi chalta!");
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

        if (objVal.isBytes())
        {
            if (g->name == "length" || g->name == "lambaai" || g->name == "lambai" || g->name == "size" || g->name == "count")
            {
                return Value(static_cast<long long>(objVal.getBytes()->data.size()));
            }
            return getBytesMethod(objVal.getBytes(), g->name);
        }

        if (objVal.isSet())
        {
            if (g->name == "length" || g->name == "lambaai" || g->name == "lambai" || g->name == "size" || g->name == "count")
            {
                return Value(static_cast<long long>(objVal.getSet()->elements.size()));
            }
            return getSetMethod(objVal.getSet(), g->name);
        }

        if (objVal.isTuple())
        {
            if (g->name == "length" || g->name == "lambaai" || g->name == "lambai" || g->name == "size" || g->name == "count")
            {
                return Value(static_cast<long long>(objVal.getTuple()->elements.size()));
            }
            return getTupleMethod(objVal.getTuple(), g->name);
        }

        if (std::holds_alternative<std::shared_ptr<Callable>>(objVal.data))
        {
            auto callable = std::get<std::shared_ptr<Callable>>(objVal.data);
            if (callable && callable->toString() == "<builtin bytes>" && (g->name == "from_hex" || g->name == "hex_se"))
            {
                return Value(std::make_shared<NativeMethodCallable>("from_hex", 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
                    std::string hexStr = args[0].deref().toString();
                    auto b = std::make_shared<MemeBytes>();
                    std::string clean;
                    for (char c : hexStr) {
                        if (std::isxdigit(static_cast<unsigned char>(c))) clean += c;
                    }
                    if (clean.size() % 2 != 0) clean = "0" + clean;
                    for (size_t i = 0; i < clean.size(); i += 2) {
                        std::string byteStr = clean.substr(i, 2);
                        uint8_t byteVal = static_cast<uint8_t>(std::strtoul(byteStr.c_str(), nullptr, 16));
                        b->data.push_back(byteVal);
                    }
                    return Value(b);
                }));
            }
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
        Value val = evaluate(sp->expr.get());
        if (val.isPointer())
        {
            return dereferencePointerHelper(val);
        }
        return val;
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

    // Tuple literal: `(a, b, c)` or `(a,)`
    if (auto *tl = dynamic_cast<const TupleExpr *>(expr))
    {
        std::vector<Value> elems;
        elems.reserve(tl->elements.size());
        for (const auto &elemExpr : tl->elements)
        {
            elems.push_back(evaluate(elemExpr.get()));
        }
        return Value(std::make_shared<MemeTuple>(std::move(elems)));
    }

    // Set literal: `{ a, b, c }`
    if (auto *sl = dynamic_cast<const SetLiteralExpr *>(expr))
    {
        auto set = std::make_shared<MemeSet>();
        for (const auto &elemExpr : sl->elements)
        {
            set->add(evaluate(elemExpr.get()));
        }
        return Value(set);
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
                if (!method) method = inst->getClass()->findMethod("__khol__");
                if (!method) method = inst->getClass()->findMethod("__padh__");
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

        if (target.isBytes())
        {
            auto bytes = target.getBytes();
            long long sz = bytes ? static_cast<long long>(bytes->data.size()) : 0;
            if (!idxVal.isNumeric())
                throw std::runtime_error("Bytes index number hona chahiye, ye kya diya: " + idxVal.typeName());
            long long idx = idxVal.asInt();
            long long origIdx = idx;
            if (idx < 0) idx += sz;
            if (idx < 0 || idx >= sz)
            {
                throw std::runtime_error(
                    "Aukaat ke bahar! Index " + std::to_string(origIdx) +
                    " maang rahe ho par bytes length sirf " + std::to_string(sz) + " hai!");
            }
            return Value(bytes->data[idx]);
        }

        if (target.isTuple())
        {
            auto tup = target.getTuple();
            long long sz = tup ? static_cast<long long>(tup->elements.size()) : 0;
            if (!idxVal.isNumeric())
                throw std::runtime_error("Tuple index number hona chahiye, ye kya diya: " + idxVal.typeName());
            long long idx = idxVal.asInt();
            long long origIdx = idx;
            if (idx < 0) idx += sz;
            if (idx < 0 || idx >= sz)
            {
                throw std::runtime_error(
                    "Aukaat ke bahar! Index " + std::to_string(origIdx) +
                    " maang rahe ho par tuple length sirf " + std::to_string(sz) + " hai!");
            }
            return tup->elements[idx];
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
                    method = inst->getClass()->findMethod("__khol__");
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

        if (target.isBytes())
        {
            auto bytes = target.getBytes();
            long long sz = bytes ? static_cast<long long>(bytes->data.size()) : 0;
            if (!idxVal.isNumeric())
                throw std::runtime_error("Bytes index number hona chahiye, ye kya diya: " + idxVal.typeName());
            long long idx = idxVal.asInt();
            long long origIdx = idx;
            if (idx < 0) idx += sz;
            if (idx < 0 || idx >= sz)
            {
                throw std::runtime_error(
                    "Aukaat ke bahar! Index " + std::to_string(origIdx) +
                    " pe assign karna chahte ho par bytes length sirf " + std::to_string(sz) + " hai!");
            }
            if (!val.isNumeric())
                throw std::runtime_error("Byte value number honi chahiye (0-255)!");
            long long byteNum = val.asInt();
            if (byteNum < 0 || byteNum > 255)
                throw std::runtime_error("Byte value 0 se 255 ke beech honi chahiye!");
            bytes->data[idx] = static_cast<uint8_t>(byteNum);
            return val;
        }

        if (target.isTuple())
        {
            throw std::runtime_error(
                "Gaddari korbe! Tuple immutable (aparivartansheel) hota hai, uski value badli nahi ja sakti!");
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

        if (objVal.isMap())
        {
            Value val;
            if (auto *ident = dynamic_cast<const IdentifierExpr *>(s->value.get()))
            {
                val = moveVariable(ident->name);
            }
            else
            {
                val = evaluate(s->value.get());
            }
            objVal.getMap()->set(s->name, val);
            return val;
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
