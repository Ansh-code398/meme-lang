#pragma once

#include "../lexer/Lexer.h"
#include "../parser/Parser.h"
#include "../platform/MemePlatform.h"
#include "../platform/MemePermissions.h"
#include "../ui/MemeUiTypes.h"
#include "../ui/MemeIconsRegistry.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace MemeWebDom {

struct FontFaceConfig {
    std::string family;
    std::string src;
    std::string weight = "400";
    std::string style = "normal";
};

struct WebDomOptions {
    std::string title = "MEMElang App";
    std::string themeColor = "#09090b";
    std::string bgColor = "#09090b";
    std::string permissionsPolicy;
    bool strictPermissions = false;
    std::vector<std::string> webPermissions;
    std::vector<FontFaceConfig> fonts;
    bool includePwaMeta = true;
};

struct WebDomBundle {
    bool success = false;
    std::string error;
    std::string indexHtml;
    std::string stylesCss;
    std::string memeDomJs;
    std::string appJs;
    bool usesEval = false;
    bool usesIcons = false;
    bool usesMotion = false;
    std::vector<std::string> sourceFiles;
};

namespace detail {

inline std::string jsEscapeString(const std::string &s) {
    std::string out = "\"";
    for (unsigned char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    out += "\"";
    return out;
}

inline std::string htmlEscape(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '&') out += "&amp;";
        else if (c == '<') out += "&lt;";
        else if (c == '>') out += "&gt;";
        else if (c == '"') out += "&quot;";
        else out += c;
    }
    return out;
}

inline std::string cssEscapeIdent(const std::string &cls) {
    std::string out;
    out.reserve(cls.size() + 8);
    for (size_t i = 0; i < cls.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(cls[i]);
        if (i == 0 && std::isdigit(c)) {
            char buf[8];
            snprintf(buf, sizeof(buf), "\\3%c ", c);
            out += buf;
        } else if (std::isalnum(c) || c == '-' || c == '_') {
            out += static_cast<char>(c);
        } else {
            out += '\\';
            out += static_cast<char>(c);
        }
    }
    return out;
}

inline bool isBuiltinIdentifier(const std::string &id) {
    static const std::unordered_set<std::string> builtins = {
        "size", "len", "string", "str", "int", "bina_point", "float", "bool",
        "akshar_bana", "chr", "code_de", "ord", "type", "keys", "values", "range",
        "abs", "min", "max", "round", "floor", "ceil", "sqrt", "pow",
        "ui", "native", "platform", "permissions", "meme", "tijori", "json", "math"
    };
    return builtins.count(id) > 0;
}

inline std::string mapBuiltinGlobal(const std::string &id) {
    if (id == "size" || id == "len") return "$m.size";
    if (id == "string" || id == "str") return "$m.str";
    if (id == "int" || id == "bina_point") return "$m.int";
    if (id == "float") return "$m.float";
    if (id == "bool") return "Boolean";
    if (id == "akshar_bana" || id == "chr") return "$m.chr";
    if (id == "code_de" || id == "ord") return "$m.ord";
    if (id == "type") return "$m.type";
    if (id == "keys") return "$m.keys";
    if (id == "values") return "$m.values";
    if (id == "range") return "$m.range";
    if (id == "abs" || id == "min" || id == "max" || id == "round" ||
        id == "floor" || id == "ceil" || id == "sqrt" || id == "pow") {
        return "Math." + id;
    }
    if (id == "ui" || id == "native" || id == "platform" ||
        id == "permissions" || id == "meme" || id == "tijori" || id == "json") {
        return "$m." + id;
    }
    if (id == "math") return "Math";
    return "";
}

inline std::string safeJsIdent(const std::string &raw) {
    if (raw == "this" || raw == "khud" || raw == "apun") return "this";
    static const std::unordered_set<std::string> jsReserved = {
        "break", "case", "catch", "class", "const", "continue", "debugger", "default",
        "delete", "do", "else", "enum", "export", "extends", "false", "finally",
        "for", "function", "if", "import", "in", "instanceof", "new", "null",
        "return", "super", "switch", "throw", "true", "try", "typeof", "var",
        "void", "while", "with", "yield", "let", "static", "implements", "interface",
        "package", "private", "protected", "public", "await", "arguments", "eval"
    };
    if (jsReserved.count(raw)) return "_m_" + raw;
    return raw;
}

inline std::string mapJsExprIdent(const std::string &raw) {
    if (raw == "this" || raw == "khud" || raw == "apun") return "this";
    std::string mapped = mapBuiltinGlobal(raw);
    if (!mapped.empty()) return mapped;
    return safeJsIdent(raw);
}

inline std::string toCssColorRgba(const std::string &token, bool &ok) {
    ok = false;
    MemeUI::ColorRgba rgba;
    if (MemeUI::resolveColorToken(token, rgba)) {
        ok = true;
        int r = static_cast<int>(std::round(rgba.r * 255.0f));
        int g = static_cast<int>(std::round(rgba.g * 255.0f));
        int b = static_cast<int>(std::round(rgba.b * 255.0f));
        if (rgba.a >= 0.999f) {
            return "rgb(" + std::to_string(r) + " " + std::to_string(g) + " " + std::to_string(b) + ")";
        }
        std::ostringstream ss;
        ss.precision(3);
        ss << "rgb(" << r << " " << g << " " << b << " / " << rgba.a << ")";
        return ss.str();
    }
    return "";
}

inline std::string resolveSpacing(const std::string &v, bool &ok) {
    ok = false;
    if (!v.empty() && v.front() == '[' && v.back() == ']') {
        ok = true;
        return v.substr(1, v.size() - 2);
    }
    static const std::unordered_map<std::string, std::string> scale = {
        {"0", "0px"}, {"px", "1px"}, {"0.5", "0.125rem"}, {"1", "0.25rem"},
        {"1.5", "0.375rem"}, {"2", "0.5rem"}, {"2.5", "0.625rem"}, {"3", "0.75rem"},
        {"3.5", "0.875rem"}, {"4", "1rem"}, {"5", "1.25rem"}, {"6", "1.5rem"},
        {"7", "1.75rem"}, {"8", "2rem"}, {"9", "2.25rem"}, {"10", "2.5rem"},
        {"11", "2.75rem"}, {"12", "3rem"}, {"14", "3.5rem"}, {"16", "4rem"},
        {"20", "5rem"}, {"24", "6rem"}, {"28", "7rem"}, {"32", "8rem"},
        {"36", "9rem"}, {"40", "10rem"}, {"44", "11rem"}, {"48", "12rem"},
        {"52", "13rem"}, {"56", "14rem"}, {"60", "15rem"}, {"64", "16rem"},
        {"72", "18rem"}, {"80", "20rem"}, {"96", "24rem"},
        {"auto", "auto"}, {"full", "100%"}, {"1/2", "50%"}, {"1/3", "33.333333%"},
        {"2/3", "66.666667%"}, {"1/4", "25%"}, {"3/4", "75%"}, {"fit", "fit-content"},
        {"min", "min-content"}, {"max", "max-content"}
    };
    auto it = scale.find(v);
    if (it != scale.end()) {
        ok = true;
        return it->second;
    }
    return "";
}

inline std::string emitSingleTailwindDecl(const std::string &cls, bool &ok) {
    ok = true;
    // Display & Flex & Grid
    if (cls == "flex") return "display:flex;";
    if (cls == "inline-flex") return "display:inline-flex;";
    if (cls == "block") return "display:block;";
    if (cls == "inline-block") return "display:inline-block;";
    if (cls == "inline") return "display:inline;";
    if (cls == "hidden") return "display:none;";
    if (cls == "grid") return "display:grid;";
    if (cls == "flex-row") return "flex-direction:row;";
    if (cls == "flex-col") return "flex-direction:column;";
    if (cls == "flex-row-reverse") return "flex-direction:row-reverse;";
    if (cls == "flex-col-reverse") return "flex-direction:column-reverse;";
    if (cls == "flex-wrap") return "flex-wrap:wrap;";
    if (cls == "flex-nowrap") return "flex-wrap:nowrap;";
    if (cls == "flex-1") return "flex:1 1 0%;min-width:0;";
    if (cls == "flex-auto") return "flex:1 1 auto;";
    if (cls == "flex-none") return "flex:none;";
    if (cls == "shrink-0") return "flex-shrink:0;";
    if (cls == "shrink") return "flex-shrink:1;";
    if (cls == "grow") return "flex-grow:1;";
    if (cls == "grow-0") return "flex-grow:0;";
    if (cls == "items-start") return "align-items:flex-start;";
    if (cls == "items-center") return "align-items:center;";
    if (cls == "items-end") return "align-items:flex-end;";
    if (cls == "items-stretch") return "align-items:stretch;";
    if (cls == "items-baseline") return "align-items:baseline;";
    if (cls == "justify-start") return "justify-content:flex-start;";
    if (cls == "justify-center") return "justify-content:center;";
    if (cls == "justify-end") return "justify-content:flex-end;";
    if (cls == "justify-between") return "justify-content:space-between;";
    if (cls == "justify-around") return "justify-content:space-around;";
    if (cls.rfind("grid-cols-", 0) == 0) {
        std::string n = cls.substr(10);
        if (!n.empty() && std::isdigit(static_cast<unsigned char>(n[0]))) {
            return "grid-template-columns:repeat(" + n + ",minmax(0,1fr));";
        }
    }

    // Position & Inset & Z-Index
    if (cls == "relative") return "position:relative;";
    if (cls == "absolute") return "position:absolute;";
    if (cls == "fixed") return "position:fixed;";
    if (cls == "sticky") return "position:sticky;";
    if (cls == "inset-0") return "inset:0px;";
    if (cls.rfind("top-", 0) == 0 || cls.rfind("right-", 0) == 0 ||
        cls.rfind("bottom-", 0) == 0 || cls.rfind("left-", 0) == 0) {
        size_t dash = cls.find('-');
        std::string prop = cls.substr(0, dash);
        bool sok = false;
        std::string val = resolveSpacing(cls.substr(dash + 1), sok);
        if (sok) return prop + ":" + val + ";";
    }
    if (cls.rfind("z-", 0) == 0) {
        std::string zv = cls.substr(2);
        return "z-index:" + zv + ";";
    }

    // Sizing
    if (cls == "w-screen") return "width:100vw;";
    if (cls == "h-screen") return "height:100vh;";
    if (cls == "min-h-screen") return "min-height:100vh;";
    if (cls == "min-w-0") return "min-width:0px;";
    if (cls == "min-h-0") return "min-height:0px;";
    if (cls.rfind("w-", 0) == 0) {
        bool sok = false; std::string v = resolveSpacing(cls.substr(2), sok);
        if (sok) return "width:" + v + ";";
    }
    if (cls.rfind("h-", 0) == 0) {
        bool sok = false; std::string v = resolveSpacing(cls.substr(2), sok);
        if (sok) return "height:" + v + ";";
    }
    if (cls.rfind("min-w-", 0) == 0) {
        bool sok = false; std::string v = resolveSpacing(cls.substr(6), sok);
        if (sok) return "min-width:" + v + ";";
    }
    if (cls.rfind("min-h-", 0) == 0) {
        bool sok = false; std::string v = resolveSpacing(cls.substr(6), sok);
        if (sok) return "min-height:" + v + ";";
    }
    if (cls.rfind("max-h-", 0) == 0) {
        std::string rest = cls.substr(6);
        if (rest == "screen") return "max-height:100vh;";
        bool sok = false; std::string v = resolveSpacing(rest, sok);
        if (sok) return "max-height:" + v + ";";
    }
    if (cls.rfind("max-w-", 0) == 0) {
        std::string rest = cls.substr(6);
        static const std::unordered_map<std::string, std::string> mw = {
            {"xs", "20rem"}, {"sm", "24rem"}, {"md", "28rem"}, {"lg", "32rem"},
            {"xl", "36rem"}, {"2xl", "42rem"}, {"3xl", "48rem"}, {"4xl", "56rem"},
            {"5xl", "64rem"}, {"6xl", "72rem"}, {"7xl", "80rem"}, {"full", "100%"},
            {"prose", "65ch"}, {"none", "none"}
        };
        auto it = mw.find(rest);
        if (it != mw.end()) return "max-width:" + it->second + ";";
        bool sok = false; std::string v = resolveSpacing(rest, sok);
        if (sok) return "max-width:" + v + ";";
    }

    // Spacing (p/px/py/pt/pb/pl/pr, m/mx/my/mt/mb/ml/mr, gap)
    auto matchBox = [&](const std::string &prefix, const std::string &cssA, const std::string &cssB = "") -> std::string {
        if (cls.rfind(prefix, 0) != 0) return "";
        bool sok = false;
        std::string val = resolveSpacing(cls.substr(prefix.size()), sok);
        if (!sok) return "";
        std::string out = cssA + ":" + val + ";";
        if (!cssB.empty()) out += cssB + ":" + val + ";";
        return out;
    };
    for (auto &[p, a, b] : std::vector<std::tuple<std::string, std::string, std::string>>{
             {"p-", "padding", ""}, {"px-", "padding-left", "padding-right"}, {"py-", "padding-top", "padding-bottom"},
             {"pt-", "padding-top", ""}, {"pb-", "padding-bottom", ""}, {"pl-", "padding-left", ""}, {"pr-", "padding-right", ""},
             {"m-", "margin", ""}, {"mx-", "margin-left", "margin-right"}, {"my-", "margin-top", "margin-bottom"},
             {"mt-", "margin-top", ""}, {"mb-", "margin-bottom", ""}, {"ml-", "margin-left", ""}, {"mr-", "margin-right", ""},
             {"gap-", "gap", ""}, {"gap-x-", "column-gap", ""}, {"gap-y-", "row-gap", ""}}) {
        std::string res = matchBox(p, a, b);
        if (!res.empty()) return res;
    }

    // Typography
    if (cls == "font-sans") return "font-family:'Inter',-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;";
    if (cls == "font-mono") return "font-family:'JetBrains Mono','SF Mono',Menlo,Consolas,ui-monospace,monospace;";
    if (cls == "font-serif") return "font-family:Georgia,Cambria,'Times New Roman',serif;";
    static const std::unordered_map<std::string, std::string> fontSizes = {
        {"text-xs", "font-size:0.75rem;line-height:1.1rem;"},
        {"text-sm", "font-size:0.875rem;line-height:1.35rem;"},
        {"text-base", "font-size:1rem;line-height:1.55rem;"},
        {"text-lg", "font-size:1.125rem;line-height:1.75rem;"},
        {"text-xl", "font-size:1.25rem;line-height:1.75rem;"},
        {"text-2xl", "font-size:1.5rem;line-height:2rem;"},
        {"text-3xl", "font-size:1.875rem;line-height:2.25rem;"},
        {"text-4xl", "font-size:2.25rem;line-height:2.5rem;"}
    };
    if (auto it = fontSizes.find(cls); it != fontSizes.end()) return it->second;
    if (cls == "font-normal") return "font-weight:400;";
    if (cls == "font-medium") return "font-weight:500;";
    if (cls == "font-semibold") return "font-weight:600;";
    if (cls == "font-bold") return "font-weight:700;";
    if (cls == "font-extrabold") return "font-weight:800;";
    if (cls == "italic") return "font-style:italic;";
    if (cls == "uppercase") return "text-transform:uppercase;";
    if (cls == "lowercase") return "text-transform:lowercase;";
    if (cls == "capitalize") return "text-transform:capitalize;";
    if (cls == "text-left") return "text-align:left;";
    if (cls == "text-center") return "text-align:center;";
    if (cls == "text-right") return "text-align:right;";
    if (cls == "tracking-tight") return "letter-spacing:-0.025em;";
    if (cls == "tracking-normal") return "letter-spacing:0em;";
    if (cls == "tracking-wide") return "letter-spacing:0.025em;";
    if (cls == "tracking-wider") return "letter-spacing:0.05em;";
    if (cls == "leading-tight") return "line-height:1.25;";
    if (cls == "leading-normal") return "line-height:1.5;";
    if (cls == "leading-relaxed") return "line-height:1.625;";
    if (cls == "leading-loose") return "line-height:2;";
    if (cls == "whitespace-pre") return "white-space:pre;";
    if (cls == "whitespace-pre-wrap") return "white-space:pre-wrap;";
    if (cls == "whitespace-nowrap") return "white-space:nowrap;";
    if (cls == "break-words") return "overflow-wrap:break-word;";
    if (cls == "select-none") return "user-select:none;";
    if (cls == "truncate") return "overflow:hidden;text-overflow:ellipsis;white-space:nowrap;";

    // Colors
    if (cls.rfind("bg-", 0) == 0) {
        if (cls == "bg-gradient-to-r") return "background-image:linear-gradient(to right,var(--tw-grad-from,#f59e0b),var(--tw-grad-to,#f97316));";
        if (cls == "bg-gradient-to-b") return "background-image:linear-gradient(to bottom,var(--tw-grad-from,#f59e0b),var(--tw-grad-to,#f97316));";
        bool cok = false; std::string c = toCssColorRgba(cls.substr(3), cok);
        if (cok) return "background-color:" + c + ";";
    }
    if (cls.rfind("from-", 0) == 0) {
        bool cok = false; std::string c = toCssColorRgba(cls.substr(5), cok);
        if (cok) return "--tw-grad-from:" + c + ";";
    }
    if (cls.rfind("to-", 0) == 0) {
        bool cok = false; std::string c = toCssColorRgba(cls.substr(3), cok);
        if (cok) return "--tw-grad-to:" + c + ";";
    }
    if (cls.rfind("text-", 0) == 0) {
        std::string rest = cls.substr(5);
        if (rest.size() > 2 && rest.front() == '[' && rest.back() == ']' && rest[1] != '#') {
            return "font-size:" + rest.substr(1, rest.size() - 2) + ";";
        }
        bool cok = false; std::string c = toCssColorRgba(rest, cok);
        if (cok) return "color:" + c + ";";
    }
    if (cls.rfind("border-", 0) == 0) {
        std::string rest = cls.substr(7);
        if (rest == "0") return "border-width:0px;";
        if (rest == "2") return "border-width:2px;";
        if (rest == "4") return "border-width:4px;";
        if (rest == "t") return "border-top-width:1px;border-top-style:solid;";
        if (rest == "b") return "border-bottom-width:1px;border-bottom-style:solid;";
        if (rest == "l") return "border-left-width:1px;border-left-style:solid;";
        if (rest == "r") return "border-right-width:1px;border-right-style:solid;";
        bool cok = false; std::string c = toCssColorRgba(rest, cok);
        if (cok) return "border-color:" + c + ";";
    }
    if (cls == "border") return "border-width:1px;border-style:solid;";

    // Radius, Shadow, Overflow, Transitions & Animations
    if (cls == "rounded-none") return "border-radius:0px;";
    if (cls == "rounded-sm") return "border-radius:0.125rem;";
    if (cls == "rounded") return "border-radius:0.25rem;";
    if (cls == "rounded-md") return "border-radius:0.375rem;";
    if (cls == "rounded-lg") return "border-radius:0.5rem;";
    if (cls == "rounded-xl") return "border-radius:0.75rem;";
    if (cls == "rounded-2xl") return "border-radius:1rem;";
    if (cls == "rounded-3xl") return "border-radius:1.5rem;";
    if (cls == "rounded-full") return "border-radius:9999px;";
    if (cls == "shadow-sm") return "box-shadow:0 1px 2px 0 rgb(0 0 0 / 0.25);";
    if (cls == "shadow" || cls == "shadow-md") return "box-shadow:0 4px 12px -1px rgb(0 0 0 / 0.35);";
    if (cls == "shadow-lg") return "box-shadow:0 10px 20px -3px rgb(0 0 0 / 0.45);";
    if (cls == "shadow-xl") return "box-shadow:0 20px 25px -5px rgb(0 0 0 / 0.55);";
    if (cls == "shadow-2xl") return "box-shadow:0 25px 50px -12px rgb(0 0 0 / 0.72);";
    if (cls == "overflow-hidden") return "overflow:hidden;";
    if (cls == "overflow-auto") return "overflow:auto;";
    if (cls == "overflow-y-auto") return "overflow-y:auto;";
    if (cls == "overflow-x-auto") return "overflow-x:auto;";
    if (cls == "cursor-pointer") return "cursor:pointer;";
    if (cls.rfind("opacity-", 0) == 0) {
        try {
            int pct = std::stoi(cls.substr(8));
            std::ostringstream ss; ss << "opacity:" << (pct / 100.0) << ";";
            return ss.str();
        } catch (...) {}
    }
    if (cls == "transition" || cls == "transition-colors") return "transition-property:color,background-color,border-color,opacity;transition-timing-function:cubic-bezier(0.4,0,0.2,1);transition-duration:150ms;";
    if (cls == "transition-all") return "transition-property:all;transition-timing-function:cubic-bezier(0.4,0,0.2,1);transition-duration:150ms;";
    if (cls == "transition-transform") return "transition-property:transform;transition-timing-function:cubic-bezier(0.4,0,0.2,1);transition-duration:150ms;";
    if (cls == "transition-opacity") return "transition-property:opacity;transition-timing-function:cubic-bezier(0.4,0,0.2,1);transition-duration:150ms;";
    if (cls.rfind("duration-", 0) == 0) {
        return "transition-duration:" + cls.substr(9) + "ms;";
    }
    if (cls == "ease-out") return "transition-timing-function:cubic-bezier(0,0,0.2,1);";
    if (cls == "ease-in-out") return "transition-timing-function:cubic-bezier(0.4,0,0.2,1);";
    if (cls == "animate-pulse") return "animation:memePulse 2s cubic-bezier(0.4,0,0.6,1) infinite;";
    if (cls == "animate-spin") return "animation:memeSpin 1s linear infinite;";
    if (cls == "animate-bounce") return "animation:memeBounce 1s infinite;";
    if (cls == "animate-fade-in") return "animation:memeFadeIn 0.25s ease-out both;";
    if (cls == "animate-slide-up") return "animation:memeSlideUp 0.28s cubic-bezier(0.16,1,0.3,1) both;";
    if (cls == "backdrop-blur" || cls == "backdrop-blur-md") return "backdrop-filter:blur(12px);";
    if (cls == "backdrop-blur-sm") return "backdrop-filter:blur(4px);";
    if (cls == "backdrop-blur-lg") return "backdrop-filter:blur(16px);";

    ok = false;
    return "";
}

class MemeCssGenerator {
public:
    std::set<std::string> tokens;

    void addClassString(const std::string &raw) {
        std::istringstream iss(raw);
        std::string tok;
        while (iss >> tok) {
            while (!tok.empty() && (tok.back() == '"' || tok.back() == '\'' || tok.back() == '>' || tok.back() == '\\' || tok.back() == ',' || tok.back() == ';')) {
                tok.pop_back();
            }
            while (!tok.empty() && (tok.front() == '"' || tok.front() == '\'' || tok.front() == '\\')) {
                tok.erase(tok.begin());
            }
            if (!tok.empty() && tok.size() < 80) tokens.insert(tok);
        }
    }

    void addBaselineDefaults() {
        const char *defaults[] = {
            "flex", "inline-flex", "flex-row", "flex-col", "flex-wrap", "flex-1", "shrink-0",
            "items-center", "items-start", "justify-between", "justify-center", "hidden", "block",
            "w-full", "h-full", "min-h-screen", "min-h-0", "min-w-0",
            "p-1", "p-2", "p-3", "p-4", "p-5", "p-6", "p-8",
            "px-1.5", "px-2", "px-2.5", "px-3", "px-4", "py-0.5", "py-1", "py-1.5", "py-2", "py-2.5", "py-3",
            "gap-0.5", "gap-1", "gap-1.5", "gap-2", "gap-3", "gap-4",
            "rounded", "rounded-md", "rounded-lg", "rounded-xl", "rounded-2xl", "rounded-3xl", "rounded-full",
            "border", "border-t", "border-b", "border-l", "border-r",
            "font-sans", "font-mono", "text-xs", "text-sm", "text-base", "text-lg", "text-xl", "text-2xl", "text-3xl",
            "font-medium", "font-semibold", "font-bold", "font-extrabold", "select-none",
            "overflow-hidden", "overflow-y-auto", "overflow-x-auto", "shadow-xl", "shadow-2xl",
            "transition-colors", "transition-all", "duration-150", "duration-200", "cursor-pointer",
            "bg-zinc-950", "bg-zinc-900", "bg-zinc-800", "text-zinc-100", "text-zinc-300", "text-zinc-400",
            "border-zinc-800", "border-zinc-700", "bg-amber-500", "text-amber-300", "text-amber-400",
            "bg-emerald-500", "bg-sky-500", "text-white", "text-slate-950", "text-zinc-950"
        };
        for (const char *d : defaults) tokens.insert(d);
    }

    std::string buildStylesheet(const WebDomOptions &opts) const {
        std::ostringstream css;
        css << "/* Generated by MEMElang Web DOM Compiler (Zero-Runtime-CDN Offline Stylesheet) */\n";
        for (const auto &f : opts.fonts) {
            if (f.family.empty() || f.src.empty()) continue;
            css << "@font-face {\n"
                << "  font-family: '" << f.family << "';\n"
                << "  src: url('" << f.src << "');\n"
                << "  font-weight: " << f.weight << ";\n"
                << "  font-style: " << f.style << ";\n"
                << "  font-display: swap;\n"
                << "}\n";
        }

        std::string defaultFontStack = "'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif";
        if (!opts.fonts.empty() && !opts.fonts[0].family.empty()) {
            defaultFontStack = "'" + opts.fonts[0].family + "', " + defaultFontStack;
        }

        css << "*, *::before, *::after { box-sizing: border-box; border-width: 0; border-style: solid; border-color: #27272a; }\n"
            << "html, body { margin: 0; padding: 0; width: 100%; height: 100%; background: " << opts.bgColor << "; color: #f4f4f5; "
            << "font-family: " << defaultFontStack << "; -webkit-font-smoothing: antialiased; text-rendering: optimizeLegibility; }\n"
            << "#app { width: 100%; height: 100%; min-height: 100vh; display: flex; flex-direction: column; }\n"
            << "button { font: inherit; color: inherit; background: transparent; cursor: pointer; transition: background-color 140ms ease, border-color 140ms ease, color 140ms ease, transform 90ms ease; }\n"
            << "button:hover { filter: brightness(1.08); }\n"
            << "button:active { transform: scale(0.99); }\n"
            << "input, textarea, select { font: inherit; color: inherit; outline: none; }\n"
            << "input:focus, textarea:focus { border-color: #f59e0b !important; }\n"
            << "textarea { resize: vertical; line-height: 1.5; }\n"
            << "h1, h2, h3, h4, p { margin: 0; }\n"
            << "::-webkit-scrollbar { width: 8px; height: 8px; }\n"
            << "::-webkit-scrollbar-track { background: transparent; }\n"
            << "::-webkit-scrollbar-thumb { background: #3f3f46; border-radius: 9999px; }\n"
            << "::-webkit-scrollbar-thumb:hover { background: #52525b; }\n"
            << "@keyframes memeSpin { from { transform: rotate(0deg); } to { transform: rotate(360deg); } }\n"
            << "@keyframes memePulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.5; } }\n"
            << "@keyframes memeBounce { 0%, 100% { transform: translateY(-10%); } 50% { transform: translateY(0); } }\n"
            << "@keyframes memeFadeIn { from { opacity: 0; } to { opacity: 1; } }\n"
            << "@keyframes memeSlideUp { from { opacity: 0; transform: translateY(8px); } to { opacity: 1; transform: translateY(0); } }\n";

        std::vector<std::string> baseRules;
        std::map<std::string, std::vector<std::string>> mediaRules;

        for (const auto &rawTok : tokens) {
            std::string rest = rawTok;
            std::string bp;
            std::string pseudo;
            // Parse breakpoint prefix
            for (const std::string &p : {"sm:", "md:", "lg:", "xl:"}) {
                if (rest.rfind(p, 0) == 0) {
                    bp = p.substr(0, p.size() - 1);
                    rest = rest.substr(p.size());
                    break;
                }
            }
            // Parse state prefix
            for (const std::string &st : {"hover:", "focus:", "active:", "disabled:"}) {
                if (rest.rfind(st, 0) == 0) {
                    pseudo = ":" + st.substr(0, st.size() - 1);
                    rest = rest.substr(st.size());
                    break;
                }
            }
            bool ok = false;
            std::string decl = emitSingleTailwindDecl(rest, ok);
            if (!ok || decl.empty()) continue;
            std::string rule = "." + cssEscapeIdent(rawTok) + pseudo + "{" + decl + "}";
            if (bp.empty()) baseRules.push_back(rule);
            else mediaRules[bp].push_back(rule);
        }

        for (const auto &r : baseRules) css << r << "\n";

        static const std::pair<const char *, const char *> bpOrder[] = {
            {"sm", "640px"}, {"md", "768px"}, {"lg", "1024px"}, {"xl", "1280px"}
        };
        for (const auto &[k, px] : bpOrder) {
            auto it = mediaRules.find(k);
            if (it == mediaRules.end() || it->second.empty()) continue;
            css << "@media (min-width: " << px << ") {\n";
            for (const auto &r : it->second) css << "  " << r << "\n";
            css << "}\n";
        }
        return css.str();
    }
};

struct ParsedModuleUnit {
    std::string id;
    std::filesystem::path absPath;
    std::vector<std::unique_ptr<Stmt>> program;
    std::unordered_set<std::string> exports;
    std::unordered_set<std::string> asyncFunctions;
    bool hasEntryMount = false;
};

class AstAnalyzer {
public:
    MemeCssGenerator &css;
    bool usesEval = false;
    bool usesIcons = false;
    bool usesMotion = false;

    explicit AstAnalyzer(MemeCssGenerator &c) : css(c) {}

    void collectExpr(const Expr *e, const std::unordered_set<std::string> &asyncCalleeNames, bool &foundAsyncOp) {
        if (!e) return;
        if (auto *s = dynamic_cast<const StringExpr *>(e)) {
            css.addClassString(s->value);
            return;
        }
        if (auto *fs = dynamic_cast<const FStringExpr *>(e)) {
            for (const auto &p : fs->parts) collectExpr(p.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *aw = dynamic_cast<const AwaitExpr *>(e)) {
            foundAsyncOp = true;
            collectExpr(aw->expression.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *u = dynamic_cast<const UnaryExpr *>(e)) {
            collectExpr(u->operand.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *b = dynamic_cast<const BinaryExpr *>(e)) {
            collectExpr(b->left.get(), asyncCalleeNames, foundAsyncOp);
            collectExpr(b->right.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *t = dynamic_cast<const TernaryExpr *>(e)) {
            collectExpr(t->condition.get(), asyncCalleeNames, foundAsyncOp);
            collectExpr(t->thenBranch.get(), asyncCalleeNames, foundAsyncOp);
            collectExpr(t->elseBranch.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *asg = dynamic_cast<const AssignExpr *>(e)) {
            collectExpr(asg->value.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *dasg = dynamic_cast<const DestructureAssignExpr *>(e)) {
            collectExpr(dasg->value.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *get = dynamic_cast<const GetExpr *>(e)) {
            collectExpr(get->object.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *set = dynamic_cast<const SetExpr *>(e)) {
            collectExpr(set->object.get(), asyncCalleeNames, foundAsyncOp);
            collectExpr(set->value.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *arr = dynamic_cast<const ArrayLiteralExpr *>(e)) {
            for (const auto &el : arr->elements) collectExpr(el.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *tup = dynamic_cast<const TupleExpr *>(e)) {
            for (const auto &el : tup->elements) collectExpr(el.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *sl = dynamic_cast<const SetLiteralExpr *>(e)) {
            for (const auto &el : sl->elements) collectExpr(el.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *mp = dynamic_cast<const MapLiteralExpr *>(e)) {
            for (const auto &[k, v] : mp->entries) {
                collectExpr(k.get(), asyncCalleeNames, foundAsyncOp);
                collectExpr(v.get(), asyncCalleeNames, foundAsyncOp);
            }
            return;
        }
        if (auto *ix = dynamic_cast<const IndexExpr *>(e)) {
            collectExpr(ix->target.get(), asyncCalleeNames, foundAsyncOp);
            collectExpr(ix->index.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *ia = dynamic_cast<const IndexAssignExpr *>(e)) {
            collectExpr(ia->target.get(), asyncCalleeNames, foundAsyncOp);
            collectExpr(ia->index.get(), asyncCalleeNames, foundAsyncOp);
            collectExpr(ia->value.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *sp = dynamic_cast<const SpreadExpr *>(e)) {
            collectExpr(sp->expr.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *cal = dynamic_cast<const CallExpr *>(e)) {
            if (auto *g = dynamic_cast<const GetExpr *>(cal->callee.get())) {
                if (auto *objId = dynamic_cast<const IdentifierExpr *>(g->object.get())) {
                    if (objId->name == "meme" &&
                        (g->name == "eval" || g->name == "run" || g->name == "exec" || g->name == "chala")) {
                        usesEval = true;
                        foundAsyncOp = true;
                    }
                    if (objId->name == "permissions" &&
                        (g->name == "request" || g->name == "requestAsync")) {
                        foundAsyncOp = true;
                    }
                }
            } else if (auto *id = dynamic_cast<const IdentifierExpr *>(cal->callee.get())) {
                if (asyncCalleeNames.count(id->name)) foundAsyncOp = true;
            }
            collectExpr(cal->callee.get(), asyncCalleeNames, foundAsyncOp);
            for (const auto &a : cal->args) collectExpr(a.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *lam = dynamic_cast<const LambdaExpr *>(e)) {
            // Note: defining a lambda does NOT make the outer enclosing function async!
            if (lam->declaration) {
                bool innerAsync = lam->declaration->isAsync;
                collectStmt(lam->declaration->body.get(), asyncCalleeNames, innerAsync);
            }
            return;
        }
        if (auto *jsx = dynamic_cast<const JsxElementExpr *>(e)) {
            if (jsx->tag == "Icon" || jsx->tag.rfind("Icons.", 0) == 0) usesIcons = true;
            if (jsx->tag.rfind("motion.", 0) == 0) usesMotion = true;
            for (const auto &[pk, pv] : jsx->props) collectExpr(pv.get(), asyncCalleeNames, foundAsyncOp);
            for (const auto &ch : jsx->children) collectExpr(ch.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *je = dynamic_cast<const JaanchExpr *>(e)) {
            collectExpr(je->target.get(), asyncCalleeNames, foundAsyncOp);
            for (const auto &arm : je->arms) {
                collectExpr(arm.guard.get(), asyncCalleeNames, foundAsyncOp);
                collectExpr(arm.bodyExpr.get(), asyncCalleeNames, foundAsyncOp);
                collectStmt(arm.bodyStmt.get(), asyncCalleeNames, foundAsyncOp);
            }
            collectExpr(je->defaultBodyExpr.get(), asyncCalleeNames, foundAsyncOp);
            collectStmt(je->defaultBodyStmt.get(), asyncCalleeNames, foundAsyncOp);
        }
    }

    void collectStmt(const Stmt *s, const std::unordered_set<std::string> &asyncCalleeNames, bool &foundAsyncOp) {
        if (!s) return;
        if (auto *v = dynamic_cast<const VarDeclStmt *>(s)) {
            collectExpr(v->initializer.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *ad = dynamic_cast<const ArrayDestructureStmt *>(s)) {
            collectExpr(ad->initializer.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *md = dynamic_cast<const MapDestructureStmt *>(s)) {
            collectExpr(md->initializer.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *es = dynamic_cast<const ExprStmt *>(s)) {
            collectExpr(es->expression.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *ps = dynamic_cast<const PrintStmt *>(s)) {
            for (const auto &a : ps->args) collectExpr(a.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *bl = dynamic_cast<const BlockStmt *>(s)) {
            for (const auto &st : bl->statements) collectStmt(st.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *ifs = dynamic_cast<const IfStmt *>(s)) {
            collectExpr(ifs->condition.get(), asyncCalleeNames, foundAsyncOp);
            collectStmt(ifs->thenBranch.get(), asyncCalleeNames, foundAsyncOp);
            collectStmt(ifs->elseBranch.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *wh = dynamic_cast<const WhileStmt *>(s)) {
            collectExpr(wh->condition.get(), asyncCalleeNames, foundAsyncOp);
            collectStmt(wh->body.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *gh = dynamic_cast<const GhoomStmt *>(s)) {
            collectExpr(gh->start.get(), asyncCalleeNames, foundAsyncOp);
            collectExpr(gh->end.get(), asyncCalleeNames, foundAsyncOp);
            collectExpr(gh->step.get(), asyncCalleeNames, foundAsyncOp);
            collectStmt(gh->body.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *gm = dynamic_cast<const GhumaMeinStmt *>(s)) {
            collectExpr(gm->collection.get(), asyncCalleeNames, foundAsyncOp);
            collectStmt(gm->body.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *ret = dynamic_cast<const ReturnStmt *>(s)) {
            collectExpr(ret->value.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *fn = dynamic_cast<const FuncDeclStmt *>(s)) {
            // Do NOT taint enclosing scope when declaring inner function
            bool dummy = fn->isAsync;
            collectStmt(fn->body.get(), asyncCalleeNames, dummy);
            return;
        }
        if (auto *cls = dynamic_cast<const ClassDeclStmt *>(s)) {
            for (const auto &m : cls->methods) {
                bool dummy = m->isAsync;
                collectStmt(m->body.get(), asyncCalleeNames, dummy);
            }
            return;
        }
        if (auto *tc = dynamic_cast<const TryCatchStmt *>(s)) {
            collectStmt(tc->tryBlock.get(), asyncCalleeNames, foundAsyncOp);
            collectStmt(tc->catchBlock.get(), asyncCalleeNames, foundAsyncOp);
            collectStmt(tc->finallyBlock.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *th = dynamic_cast<const ThrowStmt *>(s)) {
            collectExpr(th->expression.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *imp = dynamic_cast<const ImportStmt *>(s)) {
            if (imp->path == "meme" || imp->path.rfind("py:", 0) == 0 ||
                imp->path.rfind("rust:", 0) == 0) {
                usesEval = true;
            }
            if (imp->path == "ui:icons" || imp->path == "lucide-react") usesIcons = true;
            if (imp->path == "ui:motion" || imp->path == "framer-motion") usesMotion = true;
            return;
        }
        if (auto *ex = dynamic_cast<const ExportStmt *>(s)) {
            collectStmt(ex->decl.get(), asyncCalleeNames, foundAsyncOp);
            return;
        }
        if (auto *js = dynamic_cast<const JaanchStmt *>(s)) {
            collectExpr(js->target.get(), asyncCalleeNames, foundAsyncOp);
            for (const auto &arm : js->arms) {
                collectExpr(arm.guard.get(), asyncCalleeNames, foundAsyncOp);
                collectExpr(arm.bodyExpr.get(), asyncCalleeNames, foundAsyncOp);
                collectStmt(arm.bodyStmt.get(), asyncCalleeNames, foundAsyncOp);
            }
            collectStmt(js->defaultArm.get(), asyncCalleeNames, foundAsyncOp);
        }
    }

    static bool stmtBlockIsAsync(const Stmt *body, const std::unordered_set<std::string> &knownAsync) {
        if (!body) return false;
        MemeCssGenerator dummyCss;
        AstAnalyzer sub(dummyCss);
        bool asyncFound = false;
        sub.collectStmt(body, knownAsync, asyncFound);
        return asyncFound;
    }

    void collectDeclaredVarsShallow(const Stmt *s, std::unordered_set<std::string> &declared) const {
        if (!s) return;
        if (auto *v = dynamic_cast<const VarDeclStmt *>(s)) declared.insert(v->name);
        else if (auto *ad = dynamic_cast<const ArrayDestructureStmt *>(s)) {
            for (const auto &it : ad->items) declared.insert(it.name);
        } else if (auto *md = dynamic_cast<const MapDestructureStmt *>(s)) {
            for (const auto &k : md->keys) declared.insert(k);
        } else if (auto *fn = dynamic_cast<const FuncDeclStmt *>(s)) declared.insert(fn->name);
        else if (auto *cls = dynamic_cast<const ClassDeclStmt *>(s)) declared.insert(cls->name);
        else if (auto *en = dynamic_cast<const EnumDeclStmt *>(s)) declared.insert(en->name);
        else if (auto *bl = dynamic_cast<const BlockStmt *>(s)) {
            for (const auto &st : bl->statements) collectDeclaredVarsShallow(st.get(), declared);
        } else if (auto *ifs = dynamic_cast<const IfStmt *>(s)) {
            collectDeclaredVarsShallow(ifs->thenBranch.get(), declared);
            collectDeclaredVarsShallow(ifs->elseBranch.get(), declared);
        } else if (auto *wh = dynamic_cast<const WhileStmt *>(s)) {
            collectDeclaredVarsShallow(wh->body.get(), declared);
        } else if (auto *gh = dynamic_cast<const GhoomStmt *>(s)) {
            declared.insert(gh->varName);
            collectDeclaredVarsShallow(gh->body.get(), declared);
        } else if (auto *gm = dynamic_cast<const GhumaMeinStmt *>(s)) {
            declared.insert(gm->varName);
            collectDeclaredVarsShallow(gm->body.get(), declared);
        } else if (auto *tc = dynamic_cast<const TryCatchStmt *>(s)) {
            collectDeclaredVarsShallow(tc->tryBlock.get(), declared);
            if (!tc->catchVar.empty()) declared.insert(tc->catchVar);
            collectDeclaredVarsShallow(tc->catchBlock.get(), declared);
            collectDeclaredVarsShallow(tc->finallyBlock.get(), declared);
        } else if (auto *imp = dynamic_cast<const ImportStmt *>(s)) {
            if (!imp->alias.empty()) declared.insert(imp->alias);
            for (const auto &n : imp->namedImports) declared.insert(n);
        } else if (auto *ex = dynamic_cast<const ExportStmt *>(s)) {
            collectDeclaredVarsShallow(ex->decl.get(), declared);
        }
    }

    void collectAssignedVarsExpr(const Expr *e, const std::unordered_set<std::string> &declaredInScopeAndParents, std::unordered_set<std::string> &implicitOut) const {
        if (!e) return;
        if (auto *asg = dynamic_cast<const AssignExpr *>(e)) {
            if (!declaredInScopeAndParents.count(asg->name) && !isBuiltinIdentifier(asg->name)) {
                implicitOut.insert(asg->name);
            }
            collectAssignedVarsExpr(asg->value.get(), declaredInScopeAndParents, implicitOut);
            return;
        }
        if (auto *dasg = dynamic_cast<const DestructureAssignExpr *>(e)) {
            for (const auto &it : dasg->items) {
                if (!declaredInScopeAndParents.count(it.name)) implicitOut.insert(it.name);
            }
            collectAssignedVarsExpr(dasg->value.get(), declaredInScopeAndParents, implicitOut);
            return;
        }
        if (auto *u = dynamic_cast<const UnaryExpr *>(e)) collectAssignedVarsExpr(u->operand.get(), declaredInScopeAndParents, implicitOut);
        else if (auto *b = dynamic_cast<const BinaryExpr *>(e)) {
            collectAssignedVarsExpr(b->left.get(), declaredInScopeAndParents, implicitOut);
            collectAssignedVarsExpr(b->right.get(), declaredInScopeAndParents, implicitOut);
        } else if (auto *t = dynamic_cast<const TernaryExpr *>(e)) {
            collectAssignedVarsExpr(t->condition.get(), declaredInScopeAndParents, implicitOut);
            collectAssignedVarsExpr(t->thenBranch.get(), declaredInScopeAndParents, implicitOut);
            collectAssignedVarsExpr(t->elseBranch.get(), declaredInScopeAndParents, implicitOut);
        } else if (auto *c = dynamic_cast<const CallExpr *>(e)) {
            collectAssignedVarsExpr(c->callee.get(), declaredInScopeAndParents, implicitOut);
            for (const auto &a : c->args) collectAssignedVarsExpr(a.get(), declaredInScopeAndParents, implicitOut);
        } else if (auto *g = dynamic_cast<const GetExpr *>(e)) collectAssignedVarsExpr(g->object.get(), declaredInScopeAndParents, implicitOut);
        else if (auto *set = dynamic_cast<const SetExpr *>(e)) {
            collectAssignedVarsExpr(set->object.get(), declaredInScopeAndParents, implicitOut);
            collectAssignedVarsExpr(set->value.get(), declaredInScopeAndParents, implicitOut);
        } else if (auto *arr = dynamic_cast<const ArrayLiteralExpr *>(e)) {
            for (const auto &el : arr->elements) collectAssignedVarsExpr(el.get(), declaredInScopeAndParents, implicitOut);
        } else if (auto *mp = dynamic_cast<const MapLiteralExpr *>(e)) {
            for (const auto &[k, v] : mp->entries) {
                collectAssignedVarsExpr(k.get(), declaredInScopeAndParents, implicitOut);
                collectAssignedVarsExpr(v.get(), declaredInScopeAndParents, implicitOut);
            }
        } else if (auto *jsx = dynamic_cast<const JsxElementExpr *>(e)) {
            for (const auto &[k, v] : jsx->props) collectAssignedVarsExpr(v.get(), declaredInScopeAndParents, implicitOut);
            for (const auto &ch : jsx->children) collectAssignedVarsExpr(ch.get(), declaredInScopeAndParents, implicitOut);
        }
    }

    void collectAssignedVarsStmt(const Stmt *s, const std::unordered_set<std::string> &declaredInScopeAndParents, std::unordered_set<std::string> &implicitOut) const {
        if (!s) return;
        if (auto *v = dynamic_cast<const VarDeclStmt *>(s)) collectAssignedVarsExpr(v->initializer.get(), declaredInScopeAndParents, implicitOut);
        else if (auto *ad = dynamic_cast<const ArrayDestructureStmt *>(s)) collectAssignedVarsExpr(ad->initializer.get(), declaredInScopeAndParents, implicitOut);
        else if (auto *md = dynamic_cast<const MapDestructureStmt *>(s)) collectAssignedVarsExpr(md->initializer.get(), declaredInScopeAndParents, implicitOut);
        else if (auto *es = dynamic_cast<const ExprStmt *>(s)) collectAssignedVarsExpr(es->expression.get(), declaredInScopeAndParents, implicitOut);
        else if (auto *ps = dynamic_cast<const PrintStmt *>(s)) {
            for (const auto &a : ps->args) collectAssignedVarsExpr(a.get(), declaredInScopeAndParents, implicitOut);
        } else if (auto *bl = dynamic_cast<const BlockStmt *>(s)) {
            for (const auto &st : bl->statements) collectAssignedVarsStmt(st.get(), declaredInScopeAndParents, implicitOut);
        } else if (auto *ifs = dynamic_cast<const IfStmt *>(s)) {
            collectAssignedVarsExpr(ifs->condition.get(), declaredInScopeAndParents, implicitOut);
            collectAssignedVarsStmt(ifs->thenBranch.get(), declaredInScopeAndParents, implicitOut);
            collectAssignedVarsStmt(ifs->elseBranch.get(), declaredInScopeAndParents, implicitOut);
        } else if (auto *wh = dynamic_cast<const WhileStmt *>(s)) {
            collectAssignedVarsExpr(wh->condition.get(), declaredInScopeAndParents, implicitOut);
            collectAssignedVarsStmt(wh->body.get(), declaredInScopeAndParents, implicitOut);
        } else if (auto *gh = dynamic_cast<const GhoomStmt *>(s)) {
            collectAssignedVarsStmt(gh->body.get(), declaredInScopeAndParents, implicitOut);
        } else if (auto *gm = dynamic_cast<const GhumaMeinStmt *>(s)) {
            collectAssignedVarsStmt(gm->body.get(), declaredInScopeAndParents, implicitOut);
        } else if (auto *ret = dynamic_cast<const ReturnStmt *>(s)) {
            collectAssignedVarsExpr(ret->value.get(), declaredInScopeAndParents, implicitOut);
        } else if (auto *tc = dynamic_cast<const TryCatchStmt *>(s)) {
            collectAssignedVarsStmt(tc->tryBlock.get(), declaredInScopeAndParents, implicitOut);
            collectAssignedVarsStmt(tc->catchBlock.get(), declaredInScopeAndParents, implicitOut);
            collectAssignedVarsStmt(tc->finallyBlock.get(), declaredInScopeAndParents, implicitOut);
        }
    }
};

class JsModuleEmitter {
public:
    const std::unordered_map<std::string, std::string> &pathToModId;
    std::filesystem::path currentDir;
    const std::unordered_set<std::string> &asyncFuncs;
    bool inAsyncBody = false;
    bool inClassMethod = false;
    int tmpCounter = 0;

    JsModuleEmitter(const std::unordered_map<std::string, std::string> &p2m,
                    const std::filesystem::path &curDir,
                    const std::unordered_set<std::string> &af)
        : pathToModId(p2m), currentDir(curDir), asyncFuncs(af) {}

    std::string nextTmp() { return "_mt" + std::to_string(tmpCounter++); }

    std::string emitBuiltinImport(const std::string &modPath, const std::string &alias, const std::vector<std::string> &named,
                                  std::unordered_set<std::string> &emitted) {
        std::string nsExpr = "$m.ui";
        if (modPath == "ui:shadcn") nsExpr = "$m.shadcn";
        else if (modPath == "ui:icons" || modPath == "lucide-react") nsExpr = "$m.icons";
        else if (modPath == "ui:motion" || modPath == "framer-motion") nsExpr = "$m.motion";
        else if (modPath == "native") nsExpr = "$m.native";
        else if (modPath == "platform") nsExpr = "$m.platform";
        else if (modPath == "permissions") nsExpr = "$m.permissions";
        else if (modPath == "meme") nsExpr = "$m.meme";
        else if (modPath == "tijori") nsExpr = "$m.tijori";
        else if (modPath == "json") nsExpr = "$m.json";
        else if (modPath == "math") nsExpr = "Math";
        else if (modPath == "crypto") nsExpr = "$m.crypto";
        else if (modPath.rfind("py:", 0) == 0 || modPath.rfind("rust:", 0) == 0 ||
                 modPath.rfind("npm:", 0) == 0 || modPath.rfind("js:", 0) == 0) {
            nsExpr = "$m.bridgePkg(" + jsEscapeString(modPath) + ")";
        }

        std::ostringstream os;
        if (!named.empty()) {
            for (const auto &n : named) {
                std::string safe = safeJsIdent(n);
                if (emitted.insert(n).second) os << "let " << safe << "; ";
                os << safe << " = " << nsExpr << "[" << jsEscapeString(n) << "]; ";
            }
            os << "\n";
        } else {
            std::string rawName = !alias.empty() ? alias : "";
            if (rawName.empty()) {
                size_t col = modPath.find(':');
                rawName = (col != std::string::npos ? modPath.substr(col + 1) : modPath);
            }
            std::string localName = safeJsIdent(rawName);
            if (emitted.insert(rawName).second) os << "let " << localName << " = " << nsExpr << ";\n";
            else os << localName << " = " << nsExpr << ";\n";
        }
        return os.str();
    }

    std::string emitParamList(const std::vector<FuncParam> &paramList, std::ostringstream &preamble) {
        std::ostringstream os;
        for (size_t i = 0; i < paramList.size(); ++i) {
            if (i > 0) os << ", ";
            const auto &p = paramList[i];
            if (p.isMapDestructure) {
                std::string tmp = nextTmp();
                os << tmp << " = {}";
                preamble << "  const { ";
                for (size_t k = 0; k < p.destructureKeys.size(); ++k) {
                    if (k > 0) preamble << ", ";
                    preamble << safeJsIdent(p.destructureKeys[k].name);
                    if (p.destructureKeys[k].defaultValue) {
                        preamble << " = " << emitExpr(p.destructureKeys[k].defaultValue.get());
                    }
                }
                preamble << " } = (" << tmp << " || {});\n";
            } else {
                if (p.isVariadic) os << "...";
                os << safeJsIdent(p.name);
                if (p.defaultValue && !p.isVariadic) {
                    os << " = " << emitExpr(p.defaultValue.get());
                }
            }
        }
        return os.str();
    }

    std::string emitFuncBody(const Stmt *bodyStmt, const std::unordered_set<std::string> &parentDeclared,
                             const std::vector<FuncParam> &params, bool makeAsync) {
        MemeCssGenerator dummyCss;
        AstAnalyzer an(dummyCss);
        std::unordered_set<std::string> declared = parentDeclared;
        for (const auto &p : params) {
            if (p.isMapDestructure) {
                for (const auto &dk : p.destructureKeys) declared.insert(dk.name);
            } else {
                declared.insert(p.name);
            }
        }
        an.collectDeclaredVarsShallow(bodyStmt, declared);
        std::unordered_set<std::string> implicitVars;
        an.collectAssignedVarsStmt(bodyStmt, declared, implicitVars);

        std::unordered_set<std::string> emitted;
        for (const auto &p : params) {
            if (p.isMapDestructure) {
                for (const auto &dk : p.destructureKeys) emitted.insert(dk.name);
            } else {
                emitted.insert(p.name);
            }
        }

        bool prevAsync = inAsyncBody;
        inAsyncBody = makeAsync;
        std::ostringstream os;
        for (const auto &iv : implicitVars) {
            if (emitted.insert(iv).second) {
                os << "  let " << safeJsIdent(iv) << ";\n";
            }
        }
        if (auto *bl = dynamic_cast<const BlockStmt *>(bodyStmt)) {
            for (const auto &st : bl->statements) os << emitStmt(st.get(), emitted, "  ");
        } else if (bodyStmt) {
            os << emitStmt(bodyStmt, emitted, "  ");
        }
        inAsyncBody = prevAsync;
        return os.str();
    }

    std::string emitJsxTagExpr(const std::string &tag) {
        if (tag.empty()) return "$m.Fragment";
        if (tag.rfind("motion.", 0) == 0) {
            return "$m.motionTag(" + jsEscapeString(tag.substr(7)) + ")";
        }
        if (tag.find('.') != std::string::npos) return tag;
        if (std::isupper(static_cast<unsigned char>(tag[0]))) return safeJsIdent(tag);
        return jsEscapeString(tag);
    }

    std::string emitExpr(const Expr *e) {
        if (!e) return "null";
        if (auto *v = dynamic_cast<const IntExpr *>(e)) return std::to_string(v->value);
        if (auto *b = dynamic_cast<const ByteExpr *>(e)) return std::to_string(static_cast<int>(b->value));
        if (auto *by = dynamic_cast<const BytesExpr *>(e)) {
            std::string s = "new Uint8Array([";
            for (size_t i = 0; i < by->value.size(); ++i) {
                if (i > 0) s += ",";
                s += std::to_string(static_cast<int>(by->value[i]));
            }
            return s + "])";
        }
        if (auto *f = dynamic_cast<const FloatExpr *>(e)) {
            std::ostringstream ss;
            ss << f->value;
            std::string res = ss.str();
            if (res == "nan" || res == "inf") return "0";
            return res;
        }
        if (auto *s = dynamic_cast<const StringExpr *>(e)) return jsEscapeString(s->value);
        if (auto *bo = dynamic_cast<const BoolExpr *>(e)) return bo->value ? "true" : "false";
        if (dynamic_cast<const NullExpr *>(e)) return "null";
        if (auto *id = dynamic_cast<const IdentifierExpr *>(e)) return mapJsExprIdent(id->name);
        if (auto *asg = dynamic_cast<const AssignExpr *>(e)) {
            return "(" + safeJsIdent(asg->name) + " = " + emitExpr(asg->value.get()) + ")";
        }
        if (auto *dasg = dynamic_cast<const DestructureAssignExpr *>(e)) {
            std::string s = "([";
            for (size_t i = 0; i < dasg->items.size(); ++i) {
                if (i > 0) s += ", ";
                if (dasg->items[i].isVariadic) s += "...";
                s += safeJsIdent(dasg->items[i].name);
            }
            return s + "] = $m.iter(" + emitExpr(dasg->value.get()) + "))";
        }
        if (auto *u = dynamic_cast<const UnaryExpr *>(e)) {
            std::string op = (u->op.type == TokenType::NOT || u->op.value == "!") ? "!" : "-";
            return "(" + op + emitExpr(u->operand.get()) + ")";
        }
        if (auto *b = dynamic_cast<const BinaryExpr *>(e)) {
            std::string l = emitExpr(b->left.get());
            std::string r = emitExpr(b->right.get());
            if (b->op.type == TokenType::AUR || b->op.value == "&&") return "(" + l + " && " + r + ")";
            if (b->op.type == TokenType::YA || b->op.value == "||") return "(" + l + " || " + r + ")";
            if (b->op.type == TokenType::EQUAL || b->op.value == "==") return "$m.eq(" + l + ", " + r + ")";
            if (b->op.type == TokenType::NOT_EQUAL || b->op.value == "!=") return "(!$m.eq(" + l + ", " + r + "))";
            if (b->op.type == TokenType::MULTIPLY || b->op.value == "*") return "$m.mul(" + l + ", " + r + ")";
            std::string op = b->op.value;
            return "(" + l + " " + op + " " + r + ")";
        }
        if (auto *t = dynamic_cast<const TernaryExpr *>(e)) {
            return "(" + emitExpr(t->condition.get()) + " ? " + emitExpr(t->thenBranch.get()) + " : " + emitExpr(t->elseBranch.get()) + ")";
        }
        if (auto *aw = dynamic_cast<const AwaitExpr *>(e)) {
            return "(await " + emitExpr(aw->expression.get()) + ")";
        }
        if (auto *po = dynamic_cast<const PoochExpr *>(e)) {
            std::string pr = po->prompt ? emitExpr(po->prompt.get()) : "\"\"";
            return "(typeof window !== 'undefined' && window.prompt ? (window.prompt(" + pr + ") ?? \"\") : \"\")";
        }
        if (auto *get = dynamic_cast<const GetExpr *>(e)) {
            return "$m.get(" + emitExpr(get->object.get()) + ", " + jsEscapeString(get->name) + ")";
        }
        if (auto *set = dynamic_cast<const SetExpr *>(e)) {
            return "(" + emitExpr(set->object.get()) + "[" + jsEscapeString(set->name) + "] = " + emitExpr(set->value.get()) + ")";
        }
        if (auto *sc = dynamic_cast<const ScopedAccessExpr *>(e)) {
            return "$m.get(" + emitExpr(sc->scope.get()) + ", " + jsEscapeString(sc->member) + ")";
        }
        if (auto *ss = dynamic_cast<const ScopedSetExpr *>(e)) {
            return "(" + emitExpr(ss->scope.get()) + "[" + jsEscapeString(ss->member) + "] = " + emitExpr(ss->value.get()) + ")";
        }
        if (auto *arr = dynamic_cast<const ArrayLiteralExpr *>(e)) {
            std::string out = "[";
            for (size_t i = 0; i < arr->elements.size(); ++i) {
                if (i > 0) out += ", ";
                out += emitExpr(arr->elements[i].get());
            }
            return out + "]";
        }
        if (auto *tup = dynamic_cast<const TupleExpr *>(e)) {
            std::string out = "[";
            for (size_t i = 0; i < tup->elements.size(); ++i) {
                if (i > 0) out += ", ";
                out += emitExpr(tup->elements[i].get());
            }
            return out + "]";
        }
        if (auto *sl = dynamic_cast<const SetLiteralExpr *>(e)) {
            std::string out = "new Set([";
            for (size_t i = 0; i < sl->elements.size(); ++i) {
                if (i > 0) out += ", ";
                out += emitExpr(sl->elements[i].get());
            }
            return out + "])";
        }
        if (auto *mp = dynamic_cast<const MapLiteralExpr *>(e)) {
            std::string out = "{";
            for (size_t i = 0; i < mp->entries.size(); ++i) {
                if (i > 0) out += ", ";
                const auto &[k, val] = mp->entries[i];
                if (!val) {
                    if (auto *sp = dynamic_cast<const SpreadExpr *>(k.get())) {
                        out += "..." + emitExpr(sp->expr.get());
                    }
                } else if (auto *ks = dynamic_cast<const StringExpr *>(k.get())) {
                    out += jsEscapeString(ks->value) + ": " + emitExpr(val.get());
                } else {
                    out += "[" + emitExpr(k.get()) + "]: " + emitExpr(val.get());
                }
            }
            return out + "}";
        }
        if (auto *ix = dynamic_cast<const IndexExpr *>(e)) {
            return "$m.idx(" + emitExpr(ix->target.get()) + ", " + emitExpr(ix->index.get()) + ")";
        }
        if (auto *ia = dynamic_cast<const IndexAssignExpr *>(e)) {
            return "$m.idxSet(" + emitExpr(ia->target.get()) + ", " + emitExpr(ia->index.get()) + ", " + emitExpr(ia->value.get()) + ")";
        }
        if (auto *sp = dynamic_cast<const SpreadExpr *>(e)) {
            return "..." + emitExpr(sp->expr.get());
        }
        if (auto *fs = dynamic_cast<const FStringExpr *>(e)) {
            std::string out = "`";
            for (const auto &p : fs->parts) {
                if (auto *lit = dynamic_cast<const StringExpr *>(p.get())) {
                    for (char c : lit->value) {
                        if (c == '\\') out += "\\\\";
                        else if (c == '`') out += "\\`";
                        else if (c == '$') out += "\\$";
                        else if (c == '\n') out += "\\n";
                        else if (c == '\r') out += "\\r";
                        else out += c;
                    }
                } else {
                    out += "${$m.str(" + emitExpr(p.get()) + ")}";
                }
            }
            return out + "`";
        }
        if (auto *cal = dynamic_cast<const CallExpr *>(e)) {
            std::string argsStr;
            for (size_t i = 0; i < cal->args.size(); ++i) {
                if (i > 0) argsStr += ", ";
                argsStr += emitExpr(cal->args[i].get());
            }
            if (auto *g = dynamic_cast<const GetExpr *>(cal->callee.get())) {
                if (auto *oid = dynamic_cast<const IdentifierExpr *>(g->object.get())) {
                    if (oid->name == "meme" && (g->name == "eval" || g->name == "run" || g->name == "exec" || g->name == "chala")) {
                        std::string call = "$m.eval(" + argsStr + ")";
                        return inAsyncBody ? ("(await " + call + ")") : call;
                    }
                    if (oid->name == "permissions" && (g->name == "request" || g->name == "requestAsync")) {
                        std::string call = "$m.permissions.request(" + argsStr + ")";
                        return inAsyncBody ? ("(await " + call + ")") : call;
                    }
                }
                return "$m.call(" + emitExpr(g->object.get()) + ", " + jsEscapeString(g->name) + ", [" + argsStr + "])";
            }
            if (auto *id = dynamic_cast<const IdentifierExpr *>(cal->callee.get())) {
                std::string fnCall = mapJsExprIdent(id->name) + "(" + argsStr + ")";
                if (inAsyncBody && asyncFuncs.count(id->name)) {
                    return "(await " + fnCall + ")";
                }
                return fnCall;
            }
            return "(" + emitExpr(cal->callee.get()) + ")(" + argsStr + ")";
        }
        if (auto *lam = dynamic_cast<const LambdaExpr *>(e)) {
            const auto *d = lam->declaration.get();
            if (!d) return "(() => null)";
            bool asyncLam = d->isAsync || AstAnalyzer::stmtBlockIsAsync(d->body.get(), asyncFuncs);
            std::ostringstream pre;
            std::string pList = emitParamList(d->paramList, pre);
            std::string bodyCode = emitFuncBody(d->body.get(), {}, d->paramList, asyncLam);
            return std::string(asyncLam ? "(async (" : "((") + pList + ") => {\n" + pre.str() + bodyCode + "})";
        }
        if (auto *jsx = dynamic_cast<const JsxElementExpr *>(e)) {
            std::string tagExpr = jsx->isFragment ? "$m.Fragment" : emitJsxTagExpr(jsx->tag);
            std::ostringstream propsOs;
            if (jsx->props.empty()) {
                propsOs << "null";
            } else {
                propsOs << "{ ";
                for (size_t i = 0; i < jsx->props.size(); ++i) {
                    if (i > 0) propsOs << ", ";
                    std::string pk = jsx->props[i].first;
                    if (pk == "class") pk = "className";
                    propsOs << jsEscapeString(pk) << ": " << emitExpr(jsx->props[i].second.get());
                }
                propsOs << " }";
            }
            std::ostringstream out;
            out << "$m.h(" << tagExpr << ", " << propsOs.str();
            for (const auto &ch : jsx->children) {
                out << ", " << emitExpr(ch.get());
            }
            out << ")";
            return out.str();
        }
        if (auto *je = dynamic_cast<const JaanchExpr *>(e)) {
            std::string tgtVar = nextTmp();
            std::ostringstream os;
            os << "(() => { const " << tgtVar << " = " << emitExpr(je->target.get()) << ";\n";
            for (size_t i = 0; i < je->arms.size(); ++i) {
                const auto &arm = je->arms[i];
                if (!arm.pattern) continue;
                os << emitJaanchArm(arm, tgtVar, true);
            }
            if (je->defaultBodyExpr) os << "return " << emitExpr(je->defaultBodyExpr.get()) << ";\n";
            else if (je->defaultBodyStmt) {
                std::unordered_set<std::string> d;
                os << emitStmt(je->defaultBodyStmt.get(), d, "");
            }
            os << "return null; })()";
            return os.str();
        }
        if (auto *tp = dynamic_cast<const TryPropagateExpr *>(e)) {
            return emitExpr(tp->expression.get());
        }
        return "null";
    }

    std::string emitPatternCheck(const MemePattern *pat, const std::string &valVar, std::vector<std::string> &binds) {
        if (!pat) return "true";
        if (dynamic_cast<const WildcardPattern *>(pat)) return "true";
        if (auto *lp = dynamic_cast<const LiteralPattern *>(pat)) {
            return "$m.eq(" + valVar + ", " + emitExpr(lp->literalExpr.get()) + ")";
        }
        if (auto *vp = dynamic_cast<const VariablePattern *>(pat)) {
            binds.push_back("const " + safeJsIdent(vp->name) + " = " + valVar + ";");
            return "true";
        }
        if (auto *listP = dynamic_cast<const ListPattern *>(pat)) {
            std::string cond = "Array.isArray(" + valVar + ") && " + valVar + ".length " +
                               (listP->hasRest ? ">=" : "===") + " " + std::to_string(listP->elements.size());
            for (size_t i = 0; i < listP->elements.size(); ++i) {
                std::string elV = valVar + "[" + std::to_string(i) + "]";
                cond += " && (" + emitPatternCheck(listP->elements[i].get(), elV, binds) + ")";
            }
            if (listP->hasRest && !listP->restName.empty()) {
                binds.push_back("const " + safeJsIdent(listP->restName) + " = " + valVar + ".slice(" + std::to_string(listP->elements.size()) + ");");
            }
            return cond;
        }
        if (auto *varP = dynamic_cast<const VariantPattern *>(pat)) {
            return "(" + valVar + " && " + valVar + ".__variant === " + jsEscapeString(varP->variantName) + ")";
        }
        return "true";
    }

    std::string emitJaanchArm(const JaanchArm &arm, const std::string &tgtVar, bool isExpr) {
        std::vector<std::string> binds;
        std::string cond = emitPatternCheck(arm.pattern.get(), tgtVar, binds);
        std::ostringstream os;
        os << "if (" << cond << ") {\n";
        for (const auto &b : binds) os << "  " << b << "\n";
        if (arm.guard) os << "  if (" << emitExpr(arm.guard.get()) << ") {\n";
        if (arm.bodyExpr) {
            os << (isExpr ? "  return " : "  ") << emitExpr(arm.bodyExpr.get()) << ";\n";
        } else if (arm.bodyStmt) {
            std::unordered_set<std::string> d;
            os << emitStmt(arm.bodyStmt.get(), d, "  ");
        }
        if (arm.guard) os << "  }\n";
        os << "}\n";
        return os.str();
    }

    std::string emitStmt(const Stmt *s, std::unordered_set<std::string> &declared, const std::string &indent = "") {
        if (!s) return "";
        std::ostringstream os;
        if (auto *v = dynamic_cast<const VarDeclStmt *>(s)) {
            bool isNew = declared.insert(v->name).second;
            os << indent << (isNew ? "let " : "") << safeJsIdent(v->name) << " = "
               << (v->initializer ? emitExpr(v->initializer.get()) : "null") << ";\n";
        } else if (auto *ad = dynamic_cast<const ArrayDestructureStmt *>(s)) {
            for (const auto &it : ad->items) {
                if (declared.insert(it.name).second) {
                    os << indent << "let " << safeJsIdent(it.name) << ";\n";
                }
            }
            os << indent << "[";
            for (size_t i = 0; i < ad->items.size(); ++i) {
                if (i > 0) os << ", ";
                if (ad->items[i].isVariadic) os << "...";
                os << safeJsIdent(ad->items[i].name);
            }
            os << "] = $m.iter(" << emitExpr(ad->initializer.get()) << ");\n";
        } else if (auto *md = dynamic_cast<const MapDestructureStmt *>(s)) {
            for (const auto &k : md->keys) {
                if (declared.insert(k).second) {
                    os << indent << "let " << safeJsIdent(k) << ";\n";
                }
            }
            os << indent << "({ ";
            for (size_t i = 0; i < md->keys.size(); ++i) {
                if (i > 0) os << ", ";
                os << safeJsIdent(md->keys[i]);
            }
            os << " } = (" << emitExpr(md->initializer.get()) << " || {}));\n";
        } else if (auto *es = dynamic_cast<const ExprStmt *>(s)) {
            os << indent << emitExpr(es->expression.get()) << ";\n";
        } else if (auto *ps = dynamic_cast<const PrintStmt *>(s)) {
            os << indent << "$m.print(";
            for (size_t i = 0; i < ps->args.size(); ++i) {
                if (i > 0) os << ", ";
                os << emitExpr(ps->args[i].get());
            }
            os << ");\n";
        } else if (auto *bl = dynamic_cast<const BlockStmt *>(s)) {
            os << indent << "{\n";
            std::unordered_set<std::string> innerDeclared = declared;
            for (const auto &st : bl->statements) os << emitStmt(st.get(), innerDeclared, indent + "  ");
            os << indent << "}\n";
        } else if (auto *ifs = dynamic_cast<const IfStmt *>(s)) {
            os << indent << "if (" << emitExpr(ifs->condition.get()) << ") {\n";
            if (auto *tb = dynamic_cast<const BlockStmt *>(ifs->thenBranch.get())) {
                std::unordered_set<std::string> d = declared;
                for (const auto &st : tb->statements) os << emitStmt(st.get(), d, indent + "  ");
            } else {
                os << emitStmt(ifs->thenBranch.get(), declared, indent + "  ");
            }
            os << indent << "}";
            if (ifs->elseBranch) {
                if (auto *elifStmt = dynamic_cast<const IfStmt *>(ifs->elseBranch.get())) {
                    os << " else " << emitStmt(elifStmt, declared, indent).substr(indent.size());
                } else {
                    os << " else {\n";
                    if (auto *eb = dynamic_cast<const BlockStmt *>(ifs->elseBranch.get())) {
                        std::unordered_set<std::string> d = declared;
                        for (const auto &st : eb->statements) os << emitStmt(st.get(), d, indent + "  ");
                    } else {
                        os << emitStmt(ifs->elseBranch.get(), declared, indent + "  ");
                    }
                    os << indent << "}\n";
                }
            } else {
                os << "\n";
            }
        } else if (auto *wh = dynamic_cast<const WhileStmt *>(s)) {
            os << indent << "while (" << emitExpr(wh->condition.get()) << ") {\n";
            if (auto *wb = dynamic_cast<const BlockStmt *>(wh->body.get())) {
                std::unordered_set<std::string> d = declared;
                for (const auto &st : wb->statements) os << emitStmt(st.get(), d, indent + "  ");
            } else {
                os << emitStmt(wh->body.get(), declared, indent + "  ");
            }
            os << indent << "}\n";
        } else if (auto *gh = dynamic_cast<const GhoomStmt *>(s)) {
            declared.insert(gh->varName);
            std::string sVar = nextTmp(), eVar = nextTmp(), stepVar = nextTmp();
            std::string vn = safeJsIdent(gh->varName);
            os << indent << "{\n"
               << indent << "  const " << sVar << " = " << emitExpr(gh->start.get()) << ", "
               << eVar << " = " << emitExpr(gh->end.get()) << ";\n"
               << indent << "  const " << stepVar << " = " << (gh->step ? emitExpr(gh->step.get()) : ("(" + sVar + " <= " + eVar + " ? 1 : -1)")) << ";\n"
               << indent << "  for (let " << vn << " = " << sVar << "; "
               << stepVar << " >= 0 ? (" << vn << (gh->inclusive ? " <= " : " < ") << eVar << ") : (" << vn << (gh->inclusive ? " >= " : " > ") << eVar << "); "
               << vn << " += " << stepVar << ") {\n";
            if (auto *gb = dynamic_cast<const BlockStmt *>(gh->body.get())) {
                std::unordered_set<std::string> d = declared;
                for (const auto &st : gb->statements) os << emitStmt(st.get(), d, indent + "    ");
            } else {
                os << emitStmt(gh->body.get(), declared, indent + "    ");
            }
            os << indent << "  }\n" << indent << "}\n";
        } else if (auto *gm = dynamic_cast<const GhumaMeinStmt *>(s)) {
            declared.insert(gm->varName);
            os << indent << "for (const " << safeJsIdent(gm->varName) << " of $m.iter(" << emitExpr(gm->collection.get()) << ")) {\n";
            if (auto *gb = dynamic_cast<const BlockStmt *>(gm->body.get())) {
                std::unordered_set<std::string> d = declared;
                for (const auto &st : gb->statements) os << emitStmt(st.get(), d, indent + "  ");
            } else {
                os << emitStmt(gm->body.get(), declared, indent + "  ");
            }
            os << indent << "}\n";
        } else if (auto *ret = dynamic_cast<const ReturnStmt *>(s)) {
            os << indent << "return " << (ret->value ? emitExpr(ret->value.get()) : "null") << ";\n";
        } else if (dynamic_cast<const BreakStmt *>(s)) {
            os << indent << "break;\n";
        } else if (dynamic_cast<const ContinueStmt *>(s)) {
            os << indent << "continue;\n";
        } else if (auto *fn = dynamic_cast<const FuncDeclStmt *>(s)) {
            declared.insert(fn->name);
            bool fnAsync = fn->isAsync || asyncFuncs.count(fn->name) > 0;
            std::ostringstream pre;
            std::string pList = emitParamList(fn->paramList, pre);
            std::string bodyCode = emitFuncBody(fn->body.get(), declared, fn->paramList, fnAsync);
            os << indent << (fnAsync ? "async function " : "function ") << safeJsIdent(fn->name) << "(" << pList << ") {\n"
               << pre.str() << bodyCode << indent << "}\n";
        } else if (auto *cls = dynamic_cast<const ClassDeclStmt *>(s)) {
            declared.insert(cls->name);
            std::string clsIdent = safeJsIdent(cls->name);
            os << indent << "const " << clsIdent << " = $m.cls(class " << clsIdent;
            if (!cls->superclass.empty()) os << " extends " << safeJsIdent(cls->superclass);
            os << " {\n";
            for (const auto &fld : cls->fields) {
                os << indent << "  " << safeJsIdent(fld.name) << " = "
                   << (fld.initializer ? emitExpr(fld.initializer.get()) : "null") << ";\n";
            }
            for (const auto &m : cls->methods) {
                std::string mName = (m->name == "shuru" || m->name == "init") ? "constructor" : safeJsIdent(m->name);
                bool mAsync = mName != "constructor" && (m->isAsync || asyncFuncs.count(m->name) > 0);
                std::ostringstream pre;
                std::string pList = emitParamList(m->paramList, pre);
                std::unordered_set<std::string> methDecl = declared;
                std::string bodyCode = emitFuncBody(m->body.get(), methDecl, m->paramList, mAsync);
                os << indent << "  " << (mAsync ? "async " : "") << mName << "(" << pList << ") {\n";
                if (mName == "constructor" && !cls->superclass.empty() && bodyCode.find("super(") == std::string::npos) {
                    os << indent << "    super();\n";
                }
                os << pre.str() << bodyCode << indent << "  }\n";
            }
            os << indent << "});\n";
        } else if (auto *en = dynamic_cast<const EnumDeclStmt *>(s)) {
            declared.insert(en->name);
            os << indent << "const " << safeJsIdent(en->name) << " = Object.freeze({\n";
            long long autoIdx = 0;
            for (size_t i = 0; i < en->members.size(); ++i) {
                os << indent << "  " << jsEscapeString(en->members[i].name) << ": "
                   << (en->members[i].value ? emitExpr(en->members[i].value.get()) : std::to_string(autoIdx)) << ",\n";
                autoIdx++;
            }
            os << indent << "});\n";
        } else if (auto *tc = dynamic_cast<const TryCatchStmt *>(s)) {
            os << indent << "try {\n" << emitStmt(tc->tryBlock.get(), declared, indent + "  ") << indent << "} catch ("
               << (tc->catchVar.empty() ? "_err" : safeJsIdent(tc->catchVar)) << ") {\n";
            if (!tc->catchVar.empty()) declared.insert(tc->catchVar);
            os << emitStmt(tc->catchBlock.get(), declared, indent + "  ") << indent << "}";
            if (tc->finallyBlock) {
                os << " finally {\n" << emitStmt(tc->finallyBlock.get(), declared, indent + "  ") << indent << "}";
            }
            os << "\n";
        } else if (auto *th = dynamic_cast<const ThrowStmt *>(s)) {
            os << indent << "throw " << emitExpr(th->expression.get()) << ";\n";
        } else if (auto *imp = dynamic_cast<const ImportStmt *>(s)) {
            bool isRel = (!imp->path.empty() && (imp->path[0] == '.' || imp->path[0] == '/'));
            if (isRel) {
                std::filesystem::path rawCand = currentDir / imp->path;
                std::filesystem::path resP = MemePlatform::probeCandidateBase(rawCand, "web");
                std::string canStr = std::filesystem::weakly_canonical(resP.empty() ? rawCand : resP).string();
                auto mit = pathToModId.find(canStr);
                if (mit != pathToModId.end()) {
                    std::string tmpMod = nextTmp();
                    os << indent << "const " << tmpMod << " = " << mit->second << "();\n";
                    if (!imp->namedImports.empty()) {
                        for (const auto &ni : imp->namedImports) {
                            std::string sN = safeJsIdent(ni);
                            if (declared.insert(ni).second) os << indent << "let " << sN << ";\n";
                            os << indent << sN << " = " << tmpMod << "[" << jsEscapeString(ni) << "];\n";
                        }
                    } else {
                        std::string aName = !imp->alias.empty() ? imp->alias : std::filesystem::path(imp->path).stem().string();
                        bool isNew = declared.insert(aName).second;
                        os << indent << (isNew ? "let " : "") << safeJsIdent(aName) << " = " << tmpMod << ";\n";
                    }
                }
            } else {
                os << indent << emitBuiltinImport(imp->path, imp->alias, imp->namedImports, declared);
            }
        } else if (auto *ex = dynamic_cast<const ExportStmt *>(s)) {
            os << emitStmt(ex->decl.get(), declared, indent);
        } else if (auto *js = dynamic_cast<const JaanchStmt *>(s)) {
            std::string tgtVar = nextTmp();
            os << indent << "{ const " << tgtVar << " = " << emitExpr(js->target.get()) << ";\n";
            for (const auto &arm : js->arms) os << emitJaanchArm(arm, tgtVar, false);
            if (js->defaultArm) os << emitStmt(js->defaultArm.get(), declared, indent + "  ");
            os << indent << "}\n";
        } else if (auto *dr = dynamic_cast<const DropStmt *>(s)) {
            os << indent << "/* drop " << emitExpr(dr->expr.get()) << " */\n";
        }
        return os.str();
    }
};

inline std::string buildMemeDomRuntimeJs(const WebDomOptions &opts) {
    std::ostringstream cfg;
    cfg << "{\n"
        << "  title: " << jsEscapeString(opts.title) << ",\n"
        << "  strictPermissions: " << (opts.strictPermissions ? "true" : "false") << ",\n"
        << "  webPermissions: [";
    for (size_t i = 0; i < opts.webPermissions.size(); ++i) {
        if (i > 0) cfg << ", ";
        cfg << jsEscapeString(opts.webPermissions[i]);
    }
    cfg << "]\n}";

    std::ostringstream pathsJs;
    pathsJs << "const LUCIDE_PATHS = {\n";
    bool firstIcon = true;
    for (const auto &[iname, ipath] : MemeUI::Icons::registry()) {
        if (!firstIcon) pathsJs << ",\n";
        firstIcon = false;
        pathsJs << "  " << jsEscapeString(iname) << ": " << jsEscapeString(ipath);
    }
    pathsJs << "\n};\n";

    std::string head = "// MEMElang Reactive Web DOM Runtime (60-120 FPS Standard HTML/CSS/JS + Lazy Wasm Coprocessor)\nexport const __MEME_CFG__ = " + cfg.str() + ";\n" + pathsJs.str();
    return head + R"JS(
let rootContainer = null;
let rootEntry = null;
let currentVTree = null;
let renderScheduled = false;
let isRendering = false;
let stateChangedDuringRender = false;
const hookStateMap = new Map();
const pathCounts = new Map();
const componentStack = [];
let activeHooks = [];
let hookCursor = 0;

function flattenChildren(arr, out = []) {
  for (const ch of arr) {
    if (Array.isArray(ch)) flattenChildren(ch, out);
    else if (ch !== null && ch !== undefined && ch !== false && ch !== true) out.push(ch);
  }
  return out;
}

function h(type, props, ...children) {
  return { __vnode: true, type, props: props || {}, children: flattenChildren(children) };
}

function Fragment(props) {
  return h("__frag__", null, ...(props.children || []));
}

function createPainter2d(canvas) {
  const ctx = canvas.getContext("2d");
  return {
    rect_filled(x, y, w, h, r = 0, col = "#ffffff", a = 1.0) {
      if (!ctx) return;
      ctx.save();
      ctx.globalAlpha = a ?? 1.0;
      ctx.fillStyle = col || "#ffffff";
      ctx.beginPath();
      if (ctx.roundRect && r > 0) ctx.roundRect(x, y, w, h, r);
      else ctx.rect(x, y, w, h);
      ctx.fill();
      ctx.restore();
    },
    rect_stroke(x, y, w, h, r = 0, lw = 1, col = "#ffffff", a = 1.0) {
      if (!ctx) return;
      ctx.save();
      ctx.globalAlpha = a ?? 1.0;
      ctx.strokeStyle = col || "#ffffff";
      ctx.lineWidth = Math.max(0.5, lw ?? 1);
      ctx.beginPath();
      if (ctx.roundRect && r > 0) ctx.roundRect(x, y, w, h, r);
      else ctx.rect(x, y, w, h);
      ctx.stroke();
      ctx.restore();
    },
    circle_filled(cx, cy, r = 4, col = "#ffffff", a = 1.0) {
      if (!ctx) return;
      ctx.save();
      ctx.globalAlpha = a ?? 1.0;
      ctx.fillStyle = col || "#ffffff";
      ctx.beginPath();
      ctx.arc(cx, cy, Math.max(0.5, r), 0, Math.PI * 2);
      ctx.fill();
      ctx.restore();
    },
    circle_stroke(cx, cy, r = 4, lw = 1, col = "#ffffff", a = 1.0) {
      if (!ctx) return;
      ctx.save();
      ctx.globalAlpha = a ?? 1.0;
      ctx.strokeStyle = col || "#ffffff";
      ctx.lineWidth = Math.max(0.5, lw ?? 1);
      ctx.beginPath();
      ctx.arc(cx, cy, Math.max(0.5, r), 0, Math.PI * 2);
      ctx.stroke();
      ctx.restore();
    },
    line(x1, y1, x2, y2, lw = 1, col = "#ffffff", a = 1.0) {
      if (!ctx) return;
      ctx.save();
      ctx.globalAlpha = a ?? 1.0;
      ctx.strokeStyle = col || "#ffffff";
      ctx.lineWidth = Math.max(0.5, lw ?? 1);
      ctx.beginPath();
      ctx.moveTo(x1, y1);
      ctx.lineTo(x2, y2);
      ctx.stroke();
      ctx.restore();
    },
    text(x, y, txt, size = 13, col = "#e4e4e7", weight = 400, isMono = false, a = 1.0) {
      if (!ctx || !txt) return;
      ctx.save();
      ctx.globalAlpha = a ?? 1.0;
      ctx.fillStyle = col || "#e4e4e7";
      const fam = isMono ? "'JetBrains Mono', monospace" : "'Inter', system-ui, sans-serif";
      ctx.font = `${weight || 400} ${Math.max(8, size || 13)}px ${fam}`;
      ctx.textBaseline = "top";
      ctx.fillText(String(txt), x, y);
      ctx.restore();
    }
  };
}

function paintCanvasDom(el, paintFn) {
  if (!el || typeof paintFn !== "function") return;
  const rect = el.getBoundingClientRect();
  const w = Math.max(300, Math.round(rect.width || el.clientWidth || 520));
  const h = Math.max(220, Math.round(rect.height || el.clientHeight || 320));
  const dpr = window.devicePixelRatio || 1;
  if (el.width !== Math.round(w * dpr) || el.height !== Math.round(h * dpr)) {
    el.width = Math.round(w * dpr);
    el.height = Math.round(h * dpr);
  }
  const ctx = el.getContext("2d");
  if (ctx) {
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.clearRect(0, 0, w, h);
  }
  try {
    paintFn(createPainter2d(el), { x: 0, y: 0, width: w, height: h });
  } catch (err) {
    console.warn("onPaint error:", err);
  }
}

function expandVNode(vnode) {
  if (vnode === null || vnode === undefined || typeof vnode === "boolean") return null;
  if (typeof vnode !== "object" || !vnode.__vnode) {
    return { kind: "text", text: String(vnode) };
  }
  if (vnode.type === "__frag__") {
    return { kind: "el", tag: "div", props: { style: "display:contents" }, children: vnode.children.map(expandVNode).filter(Boolean) };
  }
  if (typeof vnode.type === "function") {
    const fnName = vnode.type.name || "Comp";
    const keyPart = vnode.props?.key !== undefined && vnode.props?.key !== null ? ":" + String(vnode.props.key) : "";
    const basePath = (componentStack.length ? componentStack[componentStack.length - 1] + "/" : "") + fnName + keyPart;
    const count = pathCounts.get(basePath) || 0;
    pathCounts.set(basePath, count + 1);
    const fullPath = basePath + "#" + count;
    componentStack.push(fullPath);
    if (!hookStateMap.has(fullPath)) hookStateMap.set(fullPath, []);
    const prevHooks = activeHooks;
    const prevCursor = hookCursor;
    let rendered = null;
    for (let pass = 0; pass < 20; pass++) {
      activeHooks = hookStateMap.get(fullPath);
      hookCursor = 0;
      stateChangedDuringRender = false;
      const compProps = { ...(vnode.props || {}), children: vnode.children };
      rendered = vnode.type(compProps);
      if (!stateChangedDuringRender) break;
    }
    activeHooks = prevHooks;
    hookCursor = prevCursor;
    componentStack.pop();
    const out = expandVNode(rendered);
    if (out && vnode.props?.key !== undefined && out.props) {
      out.key = vnode.props.key;
    }
    return out;
  }
  const tag = String(vnode.type || "div");
  const props = { ...(vnode.props || {}) };
  const expandedChildren = [];
  for (const ch of vnode.children) {
    const ec = expandVNode(ch);
    if (ec) expandedChildren.push(ec);
  }
  return { kind: "el", tag, key: props.key ?? null, props, children: expandedChildren };
}

function applyProps(el, prevProps, nextProps) {
  const prev = prevProps || {};
  const next = nextProps || {};
  for (const k of Object.keys(prev)) {
    if (k in next || k === "key" || k === "children") continue;
    if (k === "className" || k === "class") {
      if (el instanceof SVGElement) el.removeAttribute("class");
      else el.className = "";
    }
    else if (k.startsWith("on")) {
      const evName = k === "onChange" ? "input" : k.slice(2).toLowerCase();
      if (el.__memeHandlers?.[evName]) el.removeEventListener(evName, el.__memeHandlers[evName]);
    } else if (k !== "onPaint" && k !== "value" && k !== "checked") {
      el.removeAttribute(k);
    }
  }
  if (!el.__memeHandlers) el.__memeHandlers = {};
  for (const [k, v] of Object.entries(next)) {
    if (k === "key" || k === "children" || v === undefined) continue;
    if (k === "className" || k === "class") {
      const clsStr = v ? String(v) : "";
      if (el instanceof SVGElement) {
        if (el.getAttribute("class") !== clsStr) el.setAttribute("class", clsStr);
      } else if (el.className !== clsStr) {
        el.className = clsStr;
      }
    } else if (k === "testId") {
      el.setAttribute("data-meme-testid", String(v));
    } else if (k === "value" && ("value" in el)) {
      const sv = v === null ? "" : String(v);
      if (el.value !== sv) el.value = sv;
    } else if (k === "checked" && ("checked" in el)) {
      el.checked = Boolean(v);
    } else if (k === "style") {
      if (typeof v === "string") el.setAttribute("style", v);
      else if (v && typeof v === "object") Object.assign(el.style, v);
    } else if (k === "onPaint" && typeof v === "function") {
      el.__memeOnPaint = v;
      requestAnimationFrame(() => paintCanvasDom(el, v));
    } else if (k.startsWith("on") && typeof v === "function") {
      const evName = (k === "onChange" && (el.tagName === "INPUT" || el.tagName === "TEXTAREA"))
        ? "input"
        : (k === "onChange" ? "change" : k.slice(2).toLowerCase());
      if (el.__memeHandlers[evName] !== v) {
        if (el.__memeHandlers[evName]) el.removeEventListener(evName, el.__memeHandlers[evName]);
        el.__memeHandlers[evName] = v;
        el.addEventListener(evName, v);
      }
      el.__memeRawProps = el.__memeRawProps || {};
      el.__memeRawProps[k] = v;
    } else if (typeof v === "boolean") {
      if (v) el.setAttribute(k, "");
      else el.removeAttribute(k);
    } else if (v !== null) {
      el.setAttribute(k, String(v));
    }
  }
}

function createDomNode(v) {
  if (!v) return document.createTextNode("");
  if (v.kind === "text") {
    const tn = document.createTextNode(v.text);
    v._dom = tn;
    return tn;
  }
  const svgTags = new Set(["svg", "path", "circle", "rect", "line", "polyline", "polygon"]);
  const el = svgTags.has(v.tag)
    ? document.createElementNS("http://www.w3.org/2000/svg", v.tag)
    : document.createElement(v.tag);
  applyProps(el, null, v.props);
  for (const ch of v.children) {
    el.appendChild(createDomNode(ch));
  }
  v._dom = el;
  return el;
}

function patchDom(parent, prevV, nextV, index = 0) {
  const existing = prevV?._dom || parent.childNodes[index];
  if (!prevV && nextV) {
    const nd = createDomNode(nextV);
    if (existing) parent.insertBefore(nd, existing);
    else parent.appendChild(nd);
    return;
  }
  if (prevV && !nextV) {
    if (existing && existing.parentNode === parent) parent.removeChild(existing);
    return;
  }
  if (prevV.kind === "text" && nextV.kind === "text") {
    if (prevV.text !== nextV.text) existing.nodeValue = nextV.text;
    nextV._dom = existing;
    return;
  }
  if (prevV.kind !== nextV.kind || prevV.tag !== nextV.tag || prevV.key !== nextV.key) {
    const nd = createDomNode(nextV);
    if (existing && existing.parentNode === parent) parent.replaceChild(nd, existing);
    else parent.appendChild(nd);
    return;
  }
  nextV._dom = existing;
  applyProps(existing, prevV.props, nextV.props);
  const maxLen = Math.max(prevV.children.length, nextV.children.length);
  for (let i = 0; i < maxLen; i++) {
    patchDom(existing, prevV.children[i] || null, nextV.children[i] || null, i);
  }
}

function getInitialData() {
  if (typeof window === "undefined") return null;
  if (window.__MEME_SSR_DATA__) return window.__MEME_SSR_DATA__;
  if (typeof document !== "undefined") {
    const el = document.getElementById("__MEME_SSR_DATA__");
    if (el && el.textContent) {
      try {
        window.__MEME_SSR_DATA__ = JSON.parse(el.textContent);
        return window.__MEME_SSR_DATA__;
      } catch (_) {}
    }
  }
  return null;
}

function renderNow() {
  if (!rootContainer || !rootEntry) return;
  const ssrData = getInitialData() || {};
  isRendering = true;
  renderScheduled = false;
  pathCounts.clear();
  componentStack.length = 0;
  const raw = typeof rootEntry === "function" ? h(rootEntry, ssrData) : rootEntry;
  const nextVTree = expandVNode(raw);
  isRendering = false;
  if (!currentVTree && rootContainer.firstChild) {
    rootContainer.innerHTML = "";
  }
  patchDom(rootContainer, currentVTree, nextVTree, 0);
  currentVTree = nextVTree;
}

function scheduleRender() {
  if (isRendering) {
    stateChangedDuringRender = true;
    return;
  }
  if (renderScheduled) return;
  renderScheduled = true;
  queueMicrotask(renderNow);
}

function useState(initVal) {
  const idx = hookCursor++;
  const hooks = activeHooks;
  if (hooks.length <= idx) {
    hooks[idx] = typeof initVal === "function" ? initVal() : initVal;
  }
  const setter = (valOrFn) => {
    const next = typeof valOrFn === "function" ? valOrFn(hooks[idx]) : valOrFn;
    if (!Object.is(hooks[idx], next)) {
      hooks[idx] = next;
      scheduleRender();
    }
  };
  return [hooks[idx], setter];
}

function useRef(initVal) {
  const idx = hookCursor++;
  if (activeHooks.length <= idx) activeHooks[idx] = { current: initVal };
  return activeHooks[idx];
}

function useEffect(fn, deps) {
  const idx = hookCursor++;
  const prev = activeHooks[idx];
  const changed = !prev || !deps || deps.some((d, i) => !Object.is(d, prev.deps?.[i]));
  activeHooks[idx] = { deps };
  if (changed) queueMicrotask(() => { try { fn(); } catch (_) {} });
}

function useMemo(fn, deps) {
  const idx = hookCursor++;
  const prev = activeHooks[idx];
  if (prev && deps && deps.every((d, i) => Object.is(d, prev.deps?.[i]))) return prev.val;
  const val = fn();
  activeHooks[idx] = { val, deps };
  return val;
}

function useRouter() {
  const parseQuery = () => {
    const q = {};
    if (typeof window !== "undefined") {
      const sp = new URLSearchParams(window.location.search || "");
      for (const [k, v] of sp.entries()) q[k] = v;
    }
    return q;
  };
  return {
    path: typeof window !== "undefined" ? window.location.pathname : "/",
    query: parseQuery(),
    navigate(nextPath) {
      if (typeof window !== "undefined" && nextPath && window.location.pathname !== nextPath) {
        window.history.pushState({}, "", nextPath);
        scheduleRender();
      }
    }
  };
}

if (typeof window !== "undefined") {
  window.addEventListener("popstate", () => scheduleRender());
  window.addEventListener("resize", () => scheduleRender());
}

const shadcn = {
  ScrollArea: (p) => h("div", { className: p.className || "overflow-y-auto", testId: p.testId }, ...(p.children || [])),
  Popover: (p) => h("div", { className: p.className ? ("z-50 " + p.className) : "z-50 rounded-xl border border-zinc-700 bg-zinc-900 p-2 shadow-2xl", testId: p.testId }, ...(p.children || [])),
  Dialog: (p) => (p.open === false ? null : h("div", { className: "fixed inset-0 z-50 flex items-center justify-center bg-zinc-950/75 p-4" }, h("div", { className: p.className || "rounded-2xl border border-zinc-700 bg-zinc-900 p-6 max-w-lg w-full shadow-2xl", testId: p.testId }, ...(p.children || [])))),
  Card: (p) => h("div", { className: p.className || "rounded-xl border border-zinc-800 bg-zinc-900 p-4", testId: p.testId }, ...(p.children || [])),
  CardHeader: (p) => h("div", { className: p.className || "pb-2" }, ...(p.children || [])),
  CardTitle: (p) => h("h3", { className: p.className || "text-lg font-bold text-white" }, ...(p.children || [])),
  CardContent: (p) => h("div", { className: p.className || "pt-2" }, ...(p.children || [])),
  Button: (p) => h("button", { ...p, className: p.className || "px-3 py-1.5 rounded-lg bg-amber-500 text-zinc-950 font-semibold text-xs" }, ...(p.children || [])),
  Badge: (p) => h("span", { className: p.className || "px-2 py-0.5 rounded bg-zinc-800 text-xs font-medium text-zinc-200" }, ...(p.children || [])),
  Input: (p) => h("input", p),
  Tabs: (p) => h("div", { className: p.className || "flex flex-row gap-2" }, ...(p.children || [])),
  Separator: (p) => h("div", { className: p.className || "w-full border-t border-zinc-800 my-2" }),
  Platform: (p) => {
    const wants = p.web || p.browser || (!p.ios && !p.android && !p.mac && !p.windows && !p.linux && !p.desktop && !p.native);
    return wants ? h(Fragment, null, ...(p.children || [])) : null;
  }
};

function normalizeIconKey(raw) {
  let k = String(raw || "sparkles")
    .replace(/([a-z0-9])([A-Z])/g, "$1-$2")
    .replace(/[_\s]+/g, "-")
    .toLowerCase();
  if (k === "close") return "x";
  if (k === "warn" || k === "warning") return "alert-triangle";
  if (k === "gear") return "settings";
  if (k === "doc" || k === "docs") return "book-open";
  if (k === "code2") return "code";
  return k;
}

function IconComp(props) {
  const name = normalizeIconKey(props.name || "sparkles");
  const sz = props.size || 16;
  const strokeW = props.strokeWidth || 2;
  const color = props.color || "currentColor";
  const pathData = LUCIDE_PATHS[name] || LUCIDE_PATHS["sparkles"] || "M12 3L14 9L20 12L14 15L12 21L10 15L4 12L10 9Z";
  const dList = pathData.split(/\s+(?=M)/).filter(Boolean);
  return h("svg", {
    xmlns: "http://www.w3.org/2000/svg",
    width: sz,
    height: sz,
    viewBox: "0 0 24 24",
    fill: "none",
    stroke: color,
    "stroke-width": strokeW,
    "stroke-linecap": "round",
    "stroke-linejoin": "round",
    className: props.className || "inline-block shrink-0"
  }, ...dList.map((d) => h("path", { d })));
}

const iconsMod = new Proxy({ Icon: IconComp }, {
  get(target, prop) {
    if (prop in target) return target[prop];
    const name = normalizeIconKey(prop);
    return (p) => IconComp({ ...(p || {}), name });
  }
});

function useSpring(target) {
  const tgt = Number(target || 0);
  const [val, setVal] = useState(tgt);
  const stateRef = useRef({ cur: tgt, vel: 0, tgt });
  stateRef.current.tgt = tgt;
  useEffect(() => {
    let raf = 0;
    const step = () => {
      const s = stateRef.current;
      const disp = s.cur - s.tgt;
      if (Math.abs(disp) < 0.005 && Math.abs(s.vel) < 0.01) {
        s.cur = s.tgt;
        s.vel = 0;
        setVal(s.cur);
        return;
      }
      const force = -220 * disp - 24 * s.vel;
      s.vel += force * 0.016;
      s.cur += s.vel * 0.016;
      setVal(s.cur);
      raf = requestAnimationFrame(step);
    };
    if (Math.abs(stateRef.current.cur - stateRef.current.tgt) > 0.005) {
      raf = requestAnimationFrame(step);
    }
    return () => { if (raf) cancelAnimationFrame(raf); };
  }, [tgt]);
  return val;
}

const motionTag = (tag) => (props) => {
  const { initial, animate, transition, children, ...rest } = props || {};
  const animCls = (initial && initial.y != null) ? "animate-slide-up" : "animate-fade-in";
  const cls = ["transition-all duration-200", animCls, rest.className || ""].filter(Boolean).join(" ");
  const st = (typeof rest.style === "object" && rest.style) ? { ...rest.style } : {};
  if (animate && typeof animate === "object") {
    if (animate.opacity != null) st.opacity = String(animate.opacity);
    if (animate.y != null) st.transform = `translateY(${animate.y}px)`;
  }
  return h(tag, { ...rest, style: Object.keys(st).length ? st : rest.style, className: cls }, ...(children || []));
};

let evalModulePromise = null;
async function lazyEval(code, stdinText = "") {
  if (!evalModulePromise) {
    evalModulePromise = import("./meme-eval.js").then((mod) => {
      const factory = mod.default || mod.createMemeEvalModule;
      return typeof factory === "function" ? factory() : mod;
    });
  }
  const wasm = await evalModulePromise;
  const rawJson = wasm.ccall("meme_eval_json", "string", ["string", "string"], [String(code || ""), String(stdinText || "")]);
  return JSON.parse(rawJson);
}

const native = {
  clipboard: {
    write(txt) {
      try { navigator.clipboard?.writeText(String(txt || "")); } catch (_) {}
      return true;
    },
    set(txt) { return this.write(txt); },
    read() { return ""; },
    get() { return ""; }
  },
  device: {
    screen() {
      return {
        width: typeof window !== "undefined" ? window.innerWidth : 1280,
        height: typeof window !== "undefined" ? window.innerHeight : 820,
        scale: typeof window !== "undefined" ? (window.devicePixelRatio || 1) : 1
      };
    }
  },
  haptics: { impact() { return true; }, selection() { return true; } },
  notifications: { send() { return true; } }
};

const platform = {
  os() { return "web"; },
  name() { return "web"; },
  isWeb() { return true; },
  isNative() { return false; },
  info() { return { os: "web", platform: "web", engine: "dom-es6" }; }
};

const permissions = {
  check(cap) {
    const c = String(cap || "").toLowerCase();
    const ok = !__MEME_CFG__.strictPermissions || __MEME_CFG__.webPermissions.includes(c);
    return { state: ok ? "granted" : "denied", granted: ok, capability: c };
  },
  async request(cap) {
    return this.check(cap);
  },
  canAsk(cap) { return this.check(cap).granted; }
};

const tijori = {
  rakh(k, v) { try { localStorage.setItem("meme_tijori_" + k, JSON.stringify(v)); } catch (_) {} return true; },
  nikal(k) { try { const v = localStorage.getItem("meme_tijori_" + k); return v ? JSON.parse(v) : null; } catch (_) { return null; } }
};

function mount(entry, title) {
  rootEntry = entry;
  if (title && typeof document !== "undefined") document.title = title;
  if (!rootContainer && typeof document !== "undefined") {
    rootContainer = document.getElementById("app");
    if (!rootContainer) {
      rootContainer = document.createElement("div");
      rootContainer.id = "app";
      document.body.appendChild(rootContainer);
    }
  }
  renderNow();
}

function installAgentBridge() {
  if (typeof window === "undefined") return;
  window.__MEME_AGENT__ = {
    snapshot() {
      const nodes = [];
      let nextId = 1;
      const walk = (el) => {
        if (!el || el.nodeType !== 1) return;
        const rect = el.getBoundingClientRect();
        const style = window.getComputedStyle(el);
        const visible = style.display !== "none" && style.visibility !== "hidden" && rect.width > 0 && rect.height > 0;
        const hasClick = Boolean(el.__memeHandlers?.click) || el.tagName === "BUTTON";
        const hasInput = el.tagName === "INPUT" || el.tagName === "TEXTAREA";
        let name = "";
        if (hasInput) name = el.value || el.getAttribute("placeholder") || "";
        else name = (el.innerText || el.textContent || "").trim();
        if (visible && (hasClick || hasInput || el.getAttribute("data-meme-testid") || name)) {
          const nid = nextId++;
          el.__memeAgentId = nid;
          nodes.push({
            id: nid,
            tag: el.tagName.toLowerCase(),
            testId: el.getAttribute("data-meme-testid") || "",
            accessibleName: name.slice(0, 240),
            hasClick,
            hasInput,
            x: rect.x,
            y: rect.y,
            width: rect.width,
            height: rect.height
          });
        }
        for (const ch of el.children) walk(ch);
      };
      walk(rootContainer || document.body);
      return {
        viewport: { width: window.innerWidth, height: window.innerHeight },
        renderer: "dom",
        nodes
      };
    },
    act(idOrTestId, action = "click", value = "") {
      this.snapshot();
      const target = [...(rootContainer || document.body).querySelectorAll("*")].find(
        (el) => el.__memeAgentId === Number(idOrTestId) || el.getAttribute("data-meme-testid") === String(idOrTestId)
      );
      if (!target) return 0;
      if (action === "click") {
        target.click();
        return 1;
      }
      if ((action === "type" || action === "change") && ("value" in target)) {
        target.value = String(value);
        target.dispatchEvent(new Event("input", { bubbles: true }));
        return 1;
      }
      return 0;
    }
  };
}
installAgentBridge();

export const $m = {
  h,
  Fragment,
  motionTag,
  ui: {
    mount,
    dikha: mount,
    show(t) { if (t && typeof document !== "undefined") document.title = t; renderNow(); },
    useState,
    useEffect,
    useRef,
    useMemo,
    useRouter,
    getInitialData,
    Router: (p) => h(Fragment, null, ...(p.children || [])),
    Route: (p) => h(Fragment, null, ...(p.children || [])),
    Link: (p) => h("a", { href: p.to || "/", className: p.className || "", onClick: (e) => { e.preventDefault(); useRouter().navigate(p.to || "/"); } }, ...(p.children || [])),
    Icon: IconComp,
    useSpring,
    registerFont: () => true
  },
  shadcn: { ...shadcn, Icon: IconComp },
  icons: iconsMod,
  motion: (() => {
    const mt = {
      div: motionTag("div"),
      span: motionTag("span"),
      button: motionTag("button"),
      section: motionTag("section"),
      article: motionTag("article"),
      aside: motionTag("aside"),
      header: motionTag("header"),
      footer: motionTag("footer"),
      p: motionTag("p"),
      h1: motionTag("h1"),
      h2: motionTag("h2"),
      h3: motionTag("h3"),
      ul: motionTag("ul"),
      li: motionTag("li"),
      View: motionTag("div"),
      useSpring,
      spring: useSpring
    };
    mt.motion = mt;
    return mt;
  })(),
  native,
  platform,
  permissions,
  tijori,
  meme: { eval: lazyEval, run: lazyEval, exec: lazyEval, chala: lazyEval, version: () => "MEMElang Web DOM 1.0" },
  eval: lazyEval,
  json: { parse: (s) => JSON.parse(s), stringify: (v) => JSON.stringify(v) },
  crypto: { sha256: async (s) => String(s), uuid_v4: () => (crypto?.randomUUID ? crypto.randomUUID() : "00000000-0000-4000-8000-000000000000") },
  bridgePkg: (name) => ({ __bridge: name }),
  size(x) {
    if (x === null || x === undefined) return 0;
    if (typeof x === "string" || Array.isArray(x)) return x.length;
    if (x instanceof Set || x instanceof Map) return x.size;
    if (typeof x === "object") return Object.keys(x).length;
    return 0;
  },
  str(x) {
    if (x === null || x === undefined) return "nalla";
    if (typeof x === "object") { try { return JSON.stringify(x); } catch (_) { return String(x); } }
    return String(x);
  },
  int(x) { const n = Math.trunc(Number(x)); return Number.isFinite(n) ? n : 0; },
  float(x) { const n = Number(x); return Number.isFinite(n) ? n : 0.0; },
  chr(n) { return String.fromCodePoint(Math.trunc(Number(n) || 0)); },
  ord(s) { return String(s || "").codePointAt(0) ?? 0; },
  type(x) {
    if (x === null || x === undefined) return "null";
    if (Array.isArray(x)) return "list";
    return typeof x;
  },
  keys(x) { return (x && typeof x === "object") ? Object.keys(x) : []; },
  values(x) { return (x && typeof x === "object") ? Object.values(x) : []; },
  range(a, b, step = 1) {
    const out = [];
    const start = b === undefined ? 0 : a;
    const end = b === undefined ? a : b;
    for (let i = start; step > 0 ? i < end : i > end; i += step) out.push(i);
    return out;
  },
  mul(a, b) {
    if (typeof a === "string" && typeof b === "number") return a.repeat(Math.max(0, Math.trunc(b)));
    if (typeof b === "string" && typeof a === "number") return b.repeat(Math.max(0, Math.trunc(a)));
    return a * b;
  },
  eq(a, b) {
    if (a === b) return true;
    if ((a === null || a === undefined) && (b === null || b === undefined)) return true;
    return false;
  },
  iter(x) {
    if (x === null || x === undefined) return [];
    if (Array.isArray(x) || typeof x === "string" || x instanceof Set) return x;
    if (typeof x === "object") return Object.keys(x);
    return [];
  },
  get(obj, prop) {
    if (obj === null || obj === undefined) return null;
    const v = obj[prop];
    return v === undefined ? null : v;
  },
  idx(obj, i) {
    if (obj === null || obj === undefined) return null;
    const v = obj[i];
    return v === undefined ? null : v;
  },
  idxSet(obj, i, val) {
    if (obj && i !== undefined) obj[i] = val;
    return val;
  },
  cls(C) {
    return new Proxy(C, {
      apply(target, thisArg, args) { return new target(...args); }
    });
  },
  print(...args) { console.log(...args); },
  call(obj, name, args) {
    if (obj === null || obj === undefined) return null;
    if (typeof obj === "string") {
      switch (name) {
        case "to_lower": case "lower": case "dheere_bol": return obj.toLowerCase();
        case "to_upper": case "upper": case "chilla": return obj.toUpperCase();
        case "find": case "dhund": return obj.indexOf(String(args[0] ?? ""));
        case "startsWith": case "shuru_hota": return obj.startsWith(String(args[0] ?? ""));
        case "endsWith": case "khatam_hota": return obj.endsWith(String(args[0] ?? ""));
        case "slice": return obj.slice(args[0], args[1]);
        case "split": return obj.split(args[0] ?? "");
        case "trim": return obj.trim();
        case "replace": return obj.split(String(args[0] ?? "")).join(String(args[1] ?? ""));
        case "length": case "lambai": return obj.length;
        case "includes": return obj.includes(String(args[0] ?? ""));
        case "number_hai": return /^\d+$/.test(obj);
        case "aage_bhar": return obj.padStart(Number(args[0] || 0), String(args[1] ?? " "));
        case "rang_hata": return obj.replace(/\x1b\[[0-9;]*m/g, "");
      }
    }
    if (Array.isArray(obj)) {
      switch (name) {
        case "push": case "daal": return obj.push(...args);
        case "pop": return obj.pop();
        case "join": case "jod": return obj.join(args[0] ?? ",");
        case "filter": case "chhan": case "chhaan": return obj.filter(args[0]);
        case "map": case "badal": case "sab_pe": return obj.map(args[0]);
        case "reduce": case "kul": case "nichod": return args.length > 1 ? obj.reduce(args[0], args[1]) : obj.reduce(args[0]);
        case "find": case "dhund": return typeof args[0] === "function" ? (obj.find(args[0]) ?? null) : obj.indexOf(args[0]);
        case "some": case "koi": return obj.some(args[0]);
        case "every": case "sab": return obj.every(args[0]);
        case "sort": case "saja": case "line_mein": return [...obj].sort(args[0]);
        case "slice": return obj.slice(args[0], args[1]);
        case "includes": case "has": return obj.includes(args[0]);
        case "length": case "lambai": return obj.length;
        case "pehla": return obj.length ? obj[0] : null;
        case "aakhri": return obj.length ? obj[obj.length - 1] : null;
      }
    }
    if (typeof obj[name] === "function") {
      return obj[name](...args);
    }
    if (typeof obj === "object") {
      if (name === "has") return Object.prototype.hasOwnProperty.call(obj, args[0]);
      if (name === "get") return Object.prototype.hasOwnProperty.call(obj, args[0]) ? obj[args[0]] : (args[1] ?? null);
      if (name === "set") { obj[args[0]] = args[1]; return obj; }
      if (name === "keys") return Object.keys(obj);
      if (name === "values") return Object.values(obj);
      if (name === "length" || name === "lambai") return Object.keys(obj).length;
    }
    return null;
  }
};
)JS";
}

inline std::string buildDomHtmlShell(const WebDomOptions &opts) {
    std::string pwaHead;
    if (opts.includePwaMeta) {
        pwaHead =
            "<link rel=\"manifest\" href=\"manifest.webmanifest\" />\n"
            "<link rel=\"icon\" href=\"favicon.ico\" />\n"
            "<link rel=\"apple-touch-icon\" href=\"apple-touch-icon.png\" />\n"
            "<meta name=\"theme-color\" content=\"" + htmlEscape(opts.themeColor) + "\" />\n";
    }
    if (!opts.permissionsPolicy.empty()) {
        pwaHead += "<meta http-equiv=\"Permissions-Policy\" content=\"" + htmlEscape(opts.permissionsPolicy) + "\" />\n";
    }
    return "<!doctype html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\" />\n<base href=\"/\" />\n"
           "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1,viewport-fit=cover\" />\n" +
           pwaHead +
           "<title>" + htmlEscape(opts.title) + "</title>\n"
           "<link rel=\"stylesheet\" href=\"styles.css\" />\n"
           "</head>\n<body>\n<div id=\"app\"></div>\n"
           "<script type=\"module\" src=\"app.js\"></script>\n</body>\n</html>\n";
}

} // namespace detail

inline bool loadModuleGraph(
    const std::filesystem::path &entryFile,
    std::vector<std::unique_ptr<detail::ParsedModuleUnit>> &outModules,
    std::unordered_map<std::string, std::string> &pathToModId,
    detail::AstAnalyzer &analyzer,
    std::string &errorOut)
{
    std::filesystem::path resEntry = MemePlatform::probeCandidateBase(entryFile, "web");
    std::filesystem::path canEntry = std::filesystem::weakly_canonical(resEntry.empty() ? entryFile : resEntry);
    if (!std::filesystem::exists(canEntry)) {
        errorOut = "Entry file not found: " + entryFile.string();
        return false;
    }

    std::unordered_set<std::string> visiting;
    std::unordered_set<std::string> visited;

    std::function<bool(const std::filesystem::path &)> visit = [&](const std::filesystem::path &canPath) -> bool {
        std::string key = canPath.string();
        if (visited.count(key) || visiting.count(key)) return true;
        visiting.insert(key);

        std::ifstream f(canPath);
        if (!f.is_open()) {
            errorOut = "Failed to open source file: " + key;
            return false;
        }
        std::ostringstream ss;
        ss << f.rdbuf();
        std::string src = ss.str();

        std::vector<std::unique_ptr<Stmt>> prog;
        try {
            Lexer lex(src);
            auto toks = lex.tokenize();
            Parser parser(toks);
            prog = parser.parseProgram();
        } catch (const std::exception &e) {
            errorOut = "Parse error in " + key + ": " + e.what();
            return false;
        }

        // First recursively visit relative imports so topological order guarantees callee modules are defined first
        for (const auto &stmt : prog) {
            if (auto *imp = dynamic_cast<const ImportStmt *>(stmt.get())) {
                if (!imp->path.empty() && (imp->path[0] == '.' || imp->path[0] == '/')) {
                    std::filesystem::path cand = canPath.parent_path() / imp->path;
                    std::filesystem::path resP = MemePlatform::probeCandidateBase(cand, "web");
                    std::filesystem::path childCan = std::filesystem::weakly_canonical(resP.empty() ? cand : resP);
                    if (!visit(childCan)) return false;
                }
            }
        }

        auto unit = std::make_unique<detail::ParsedModuleUnit>();
        unit->id = "$mod_" + std::to_string(outModules.size());
        unit->absPath = canPath;
        unit->program = std::move(prog);
        pathToModId[key] = unit->id;

        bool topAsync = false;
        std::unordered_map<std::string, const FuncDeclStmt *> localFuncs;
        for (const auto &st : unit->program) {
            analyzer.collectStmt(st.get(), unit->asyncFunctions, topAsync);
            if (auto *fn = dynamic_cast<const FuncDeclStmt *>(st.get())) {
                localFuncs[fn->name] = fn;
                unit->exports.insert(fn->name);
                if (fn->isAsync) unit->asyncFunctions.insert(fn->name);
            } else if (auto *vd = dynamic_cast<const VarDeclStmt *>(st.get())) {
                unit->exports.insert(vd->name);
            } else if (auto *ad = dynamic_cast<const ArrayDestructureStmt *>(st.get())) {
                for (const auto &it : ad->items) unit->exports.insert(it.name);
            } else if (auto *md = dynamic_cast<const MapDestructureStmt *>(st.get())) {
                for (const auto &k : md->keys) unit->exports.insert(k);
            } else if (auto *cls = dynamic_cast<const ClassDeclStmt *>(st.get())) {
                unit->exports.insert(cls->name);
            } else if (auto *en = dynamic_cast<const EnumDeclStmt *>(st.get())) {
                unit->exports.insert(en->name);
            } else if (auto *ex = dynamic_cast<const ExportStmt *>(st.get())) {
                if (auto *ef = dynamic_cast<const FuncDeclStmt *>(ex->decl.get())) {
                    localFuncs[ef->name] = ef;
                    unit->exports.insert(ef->name);
                } else if (auto *ev = dynamic_cast<const VarDeclStmt *>(ex->decl.get())) {
                    unit->exports.insert(ev->name);
                } else if (auto *ec = dynamic_cast<const ClassDeclStmt *>(ex->decl.get())) {
                    unit->exports.insert(ec->name);
                } else if (auto *ee = dynamic_cast<const EnumDeclStmt *>(ex->decl.get())) {
                    unit->exports.insert(ee->name);
                }
            }
        }

        // Fixed-point async call-graph discovery across all nested/local functions inside module
        std::vector<const FuncDeclStmt *> allNestedFuncs;
        std::function<void(const Stmt *, const std::unordered_set<std::string> &)> gatherFns;
        std::function<void(const Expr *, const std::unordered_set<std::string> &)> gatherExprFns;
        gatherExprFns = [&](const Expr *e, const std::unordered_set<std::string> &) {
            if (!e) return;
            if (auto *lam = dynamic_cast<const LambdaExpr *>(e)) {
                if (lam->declaration) {
                    if (!lam->declaration->name.empty()) allNestedFuncs.push_back(lam->declaration.get());
                    gatherFns(lam->declaration->body.get(), unit->asyncFunctions);
                }
            } else if (auto *c = dynamic_cast<const CallExpr *>(e)) {
                gatherExprFns(c->callee.get(), unit->asyncFunctions);
                for (const auto &a : c->args) gatherExprFns(a.get(), unit->asyncFunctions);
            } else if (auto *jsx = dynamic_cast<const JsxElementExpr *>(e)) {
                for (const auto &[k, v] : jsx->props) gatherExprFns(v.get(), unit->asyncFunctions);
                for (const auto &ch : jsx->children) gatherExprFns(ch.get(), unit->asyncFunctions);
            } else if (auto *b = dynamic_cast<const BinaryExpr *>(e)) {
                gatherExprFns(b->left.get(), unit->asyncFunctions);
                gatherExprFns(b->right.get(), unit->asyncFunctions);
            } else if (auto *t = dynamic_cast<const TernaryExpr *>(e)) {
                gatherExprFns(t->condition.get(), unit->asyncFunctions);
                gatherExprFns(t->thenBranch.get(), unit->asyncFunctions);
                gatherExprFns(t->elseBranch.get(), unit->asyncFunctions);
            }
        };
        gatherFns = [&](const Stmt *st, const std::unordered_set<std::string> &) {
            if (!st) return;
            if (auto *fn = dynamic_cast<const FuncDeclStmt *>(st)) {
                allNestedFuncs.push_back(fn);
                gatherFns(fn->body.get(), unit->asyncFunctions);
            } else if (auto *bl = dynamic_cast<const BlockStmt *>(st)) {
                for (const auto &ch : bl->statements) gatherFns(ch.get(), unit->asyncFunctions);
            } else if (auto *ifs = dynamic_cast<const IfStmt *>(st)) {
                gatherFns(ifs->thenBranch.get(), unit->asyncFunctions);
                gatherFns(ifs->elseBranch.get(), unit->asyncFunctions);
            } else if (auto *es = dynamic_cast<const ExprStmt *>(st)) {
                gatherExprFns(es->expression.get(), unit->asyncFunctions);
            } else if (auto *vd = dynamic_cast<const VarDeclStmt *>(st)) {
                gatherExprFns(vd->initializer.get(), unit->asyncFunctions);
            } else if (auto *ret = dynamic_cast<const ReturnStmt *>(st)) {
                gatherExprFns(ret->value.get(), unit->asyncFunctions);
            } else if (auto *ex = dynamic_cast<const ExportStmt *>(st)) {
                gatherFns(ex->decl.get(), unit->asyncFunctions);
            }
        };
        for (const auto &st : unit->program) gatherFns(st.get(), unit->asyncFunctions);

        bool changed = true;
        while (changed) {
            changed = false;
            for (const FuncDeclStmt *fn : allNestedFuncs) {
                if (fn->name.empty() || unit->asyncFunctions.count(fn->name)) continue;
                if (fn->isAsync || detail::AstAnalyzer::stmtBlockIsAsync(fn->body.get(), unit->asyncFunctions)) {
                    unit->asyncFunctions.insert(fn->name);
                    changed = true;
                }
            }
        }

        outModules.push_back(std::move(unit));
        visiting.erase(key);
        visited.insert(key);
        return true;
    };

    return visit(canEntry);
}

inline WebDomBundle transpileToWebDomBundle(const std::filesystem::path &entryFile, const WebDomOptions &opts) {
    WebDomBundle bundle;
    detail::MemeCssGenerator cssGen;
    cssGen.addBaselineDefaults();
    detail::AstAnalyzer analyzer(cssGen);

    std::vector<std::unique_ptr<detail::ParsedModuleUnit>> modules;
    std::unordered_map<std::string, std::string> pathToModId;
    if (!loadModuleGraph(entryFile, modules, pathToModId, analyzer, bundle.error)) {
        return bundle;
    }

    std::ostringstream appJs;
    appJs << "// Compiled from MEMElang AST -> Standard ES6 Modules\n"
          << "import { $m } from './meme-dom.js';\n\n";

    for (size_t mi = 0; mi < modules.size(); ++mi) {
        const auto &mod = modules[mi];
        bundle.sourceFiles.push_back(mod->absPath.string());
        appJs << "// Module: " << mod->absPath.filename().string() << "\n"
              << "function " << mod->id << "() {\n"
              << "  if (" << mod->id << ".__cache) return " << mod->id << ".__cache;\n"
              << "  let ui = $m.ui, native = $m.native, platform = $m.platform, permissions = $m.permissions, meme = $m.meme, tijori = $m.tijori;\n";

        std::unordered_set<std::string> allDecls = {"ui", "native", "platform", "permissions", "meme", "tijori"};
        for (const auto &st : mod->program) analyzer.collectDeclaredVarsShallow(st.get(), allDecls);
        std::unordered_set<std::string> implicitTop;
        for (const auto &st : mod->program) analyzer.collectAssignedVarsStmt(st.get(), allDecls, implicitTop);
        std::unordered_set<std::string> emitted = {"ui", "native", "platform", "permissions", "meme", "tijori"};
        for (const auto &iv : implicitTop) {
            if (emitted.insert(iv).second) {
                appJs << "  let " << detail::safeJsIdent(iv) << ";\n";
            }
        }

        detail::JsModuleEmitter emitter(pathToModId, mod->absPath.parent_path(), mod->asyncFunctions);
        for (const auto &st : mod->program) {
            appJs << emitter.emitStmt(st.get(), emitted, "  ");
        }

        appJs << "  const __ns = { ";
        size_t ec = 0;
        for (const auto &exp : mod->exports) {
            if (exp.empty()) continue;
            if (ec++ > 0) appJs << ", ";
            std::string safe = detail::safeJsIdent(exp);
            if (safe == exp) appJs << exp;
            else appJs << exp << ": " << safe;
        }
        appJs << " };\n"
              << "  " << mod->id << ".__cache = __ns;\n"
              << "  return __ns;\n"
              << "}\n\n";
    }

    if (!modules.empty()) {
        appJs << modules.back()->id << "();\n";
    }

    bundle.indexHtml = detail::buildDomHtmlShell(opts);
    bundle.stylesCss = cssGen.buildStylesheet(opts);
    bundle.memeDomJs = detail::buildMemeDomRuntimeJs(opts);
    bundle.appJs = appJs.str();
    bundle.usesEval = analyzer.usesEval;
    bundle.usesIcons = analyzer.usesIcons;
    bundle.usesMotion = analyzer.usesMotion;
    bundle.success = true;
    return bundle;
}

} // namespace MemeWebDom
