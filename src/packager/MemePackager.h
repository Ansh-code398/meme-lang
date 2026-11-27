#pragma once

#include "ArchiveBuilders.h"
#include "IconAssets.h"
#include "../compiler/NativeCompiler.h"
#include "../hosts/web/MemeWebCanvasHost.h"
#include "../platform/MemePlatform.h"
#include "../platform/MemePermissions.h"
#include "../lexer/Lexer.h"
#include "../parser/Parser.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace MemePackager {

struct PackageConfig {
    std::string displayName;
    std::string bundleId;
    std::string version = "1.0.0";
    std::string description;
    std::string author = "MEMElang";
    std::string iconPath;
    std::string urlScheme;
    std::string themeColor = "#020617";
    std::string bgColor = "#020617";
    std::string categories = "Utility;";
    MemePermissions::PlatformPermissionsConfig permissions;
};

struct PackageResult {
    bool success = false;
    std::string target;
    std::string outputPath;
    std::vector<std::string> artifacts;
    std::string error;
    double elapsedMs = 0.0;
};

namespace detail {

inline std::filesystem::path findRepoRoot() {
    std::filesystem::path p = std::filesystem::current_path();
    for (int i = 0; i < 6; ++i) {
        if (std::filesystem::exists(p / "src" / "compiler" / "MemeRuntime.h")) {
            return p;
        }
        if (!p.has_parent_path() || p == p.parent_path()) break;
        p = p.parent_path();
    }
    return std::filesystem::current_path();
}

inline std::string sanitizePkgName(const std::string &raw) {
    std::string out;
    for (unsigned char c : raw) {
        if (std::isalnum(c)) out.push_back(static_cast<char>(std::tolower(c)));
        else if (c == '-' || c == '_') out.push_back('-');
    }
    while (!out.empty() && !std::isalnum(static_cast<unsigned char>(out.front()))) out.erase(out.begin());
    while (!out.empty() && !std::isalnum(static_cast<unsigned char>(out.back()))) out.pop_back();
    if (out.empty()) out = "meme-app";
    return out;
}

inline std::string sanitizeJavaIdent(const std::string &raw) {
    std::string out;
    for (unsigned char c : raw) {
        if (std::isalnum(c)) out.push_back(static_cast<char>(std::tolower(c)));
        else out.push_back('_');
    }
    if (out.empty() || !std::isalpha(static_cast<unsigned char>(out.front()))) {
        out = "app_" + out;
    }
    return out;
}

inline std::string toAppName(const std::string &raw) {
    std::string out;
    bool capNext = true;
    for (unsigned char c : raw) {
        if (std::isalnum(c)) {
            out.push_back(capNext ? static_cast<char>(std::toupper(c)) : static_cast<char>(c));
            capNext = false;
        } else {
            capNext = true;
        }
    }
    if (out.empty()) out = "MemeApp";
    return out;
}

inline int execCmd(const std::string &cmd, std::string &output) {
    output.clear();
    std::string full = cmd + " 2>&1";
    FILE *pipe = popen(full.c_str(), "r");
    if (!pipe) {
        output = "Failed to spawn command: " + cmd;
        return -1;
    }
    char buf[512];
    while (fgets(buf, sizeof(buf), pipe) != nullptr) {
        output += buf;
    }
    int status = pclose(pipe);
    return status;
}

inline std::string findInPathOrPaths(const std::string &bin, const std::vector<std::string> &extra) {
    for (const auto &p : extra) {
        if (std::filesystem::exists(p)) return p;
    }
    std::string out;
    if (execCmd("command -v " + bin, out) == 0) {
        while (!out.empty() && (out.back() == '\n' || out.back() == '\r' || out.back() == ' ')) out.pop_back();
        if (!out.empty()) return out;
    }
    return "";
}

inline std::vector<uint8_t> readFileBytes(const std::filesystem::path &p) {
    std::ifstream ifs(p, std::ios::binary);
    if (!ifs.is_open()) return {};
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
}

inline bool writeFileBytes(const std::filesystem::path &p, const std::vector<uint8_t> &bytes) {
    if (p.has_parent_path()) std::filesystem::create_directories(p.parent_path());
    std::ofstream ofs(p, std::ios::binary);
    if (!ofs.is_open()) return false;
    if (!bytes.empty()) ofs.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return ofs.good();
}

inline bool writeFileText(const std::filesystem::path &p, const std::string &txt) {
    if (p.has_parent_path()) std::filesystem::create_directories(p.parent_path());
    std::ofstream ofs(p);
    if (!ofs.is_open()) return false;
    ofs << txt;
    return ofs.good();
}

inline std::vector<uint8_t> strToBytes(const std::string &s) {
    return std::vector<uint8_t>(s.begin(), s.end());
}

inline std::filesystem::path findAndroidSdk() {
    std::vector<std::filesystem::path> cands;
    if (const char *h = std::getenv("ANDROID_HOME")) cands.emplace_back(h);
    if (const char *r = std::getenv("ANDROID_SDK_ROOT")) cands.emplace_back(r);
    if (const char *home = std::getenv("HOME")) {
        cands.push_back(std::filesystem::path(home) / "Library" / "Android" / "sdk");
        cands.push_back(std::filesystem::path(home) / "Android" / "Sdk");
    }
    for (const auto &c : cands) {
        if (std::filesystem::exists(c)) return c;
    }
    return {};
}

inline std::filesystem::path newestSubdir(const std::filesystem::path &dir) {
    std::vector<std::filesystem::path> items;
    if (!std::filesystem::exists(dir)) return {};
    for (const auto &e : std::filesystem::directory_iterator(dir)) {
        if (e.is_directory()) items.push_back(e.path());
    }
    if (items.empty()) return {};
    std::sort(items.begin(), items.end());
    return items.back();
}

inline bool buildLinuxElf(const std::filesystem::path &repoRoot, const std::filesystem::path &cppPath,
                          const std::filesystem::path &elfOut, std::string &errOut, bool verbose, bool usesEval = false) {
    std::string zig = findInPathOrPaths("zig", {"/opt/homebrew/bin/zig", "/usr/local/bin/zig"});
    if (zig.empty()) {
        errOut = "zig compiler not found for Linux cross-compilation";
        return false;
    }
    if (std::filesystem::exists(elfOut)) return true;
    std::string extraEval;
    if (usesEval) {
        extraEval = " -DMEME_HAS_EVAL -DMEME_NO_INTERP_DESKTOP \"" +
                    (repoRoot / "src" / "interpreter" / "Interpreter.cpp").string() + "\" \"" +
                    (repoRoot / "src" / "types" / "TypeChecker.cpp").string() + "\"";
    }
    std::string cmd = "\"" + zig + "\" c++ -std=c++20 -O3 -DNDEBUG -DMEME_NO_DEPTH_GUARD "
                      "-target x86_64-linux-musl -I\"" + repoRoot.string() + "\" "
                      "\"" + cppPath.string() + "\"" + extraEval + " -ldl -lpthread -o \"" + elfOut.string() + "\"";
    if (verbose) std::cout << "[package] " << cmd << "\n";
    std::string out;
    if (execCmd(cmd, out) != 0) {
        errOut = "Linux x86_64-musl compilation failed:\n" + out;
        return false;
    }
    return std::filesystem::exists(elfOut);
}

inline std::string xmlEscape(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '&') out += "&amp;";
        else if (c == '<') out += "&lt;";
        else if (c == '>') out += "&gt;";
        else if (c == '"') out += "&quot;";
        else out.push_back(c);
    }
    return out;
}

inline std::string sanitizeAndroidPkg(const std::string &id, const std::string &fallbackPkg) {
    std::string base = id.empty() ? ("dev.memelang." + sanitizeJavaIdent(fallbackPkg)) : id;
    std::string out;
    for (unsigned char c : base) {
        if (std::isalnum(c)) out.push_back(static_cast<char>(std::tolower(c)));
        else if (c == '.') out.push_back('.');
        else out.push_back('_');
    }
    if (out.find('.') == std::string::npos) out = "dev.memelang." + out;
    return out;
}

inline std::string extractJsonStrField(const std::string &content, const std::string &key) {
    std::string search = "\"" + key + "\"";
    size_t pos = 0;
    while ((pos = content.find(search, pos)) != std::string::npos) {
        size_t colon = content.find(':', pos + search.size());
        if (colon != std::string::npos) {
            size_t q1 = content.find('"', colon + 1);
            if (q1 != std::string::npos) {
                size_t q2 = content.find('"', q1 + 1);
                if (q2 != std::string::npos) return content.substr(q1 + 1, q2 - q1 - 1);
            }
        }
        pos += search.size();
    }
    return "";
}

struct TinyJsonVal {
    enum Kind { J_NULL, J_BOOL, J_STR, J_NUM, J_ARR, J_OBJ };
    Kind kind = J_NULL;
    bool bval = false;
    std::string sval;
    std::vector<TinyJsonVal> arr;
    std::map<std::string, TinyJsonVal> obj;
};

struct TinyJsonParser {
    const std::string &s;
    size_t i = 0;

    void skipWs() {
        while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) i++;
    }
    std::string parseStr() {
        if (i >= s.size() || s[i] != '"') return "";
        i++;
        std::string out;
        while (i < s.size()) {
            char c = s[i++];
            if (c == '"') break;
            if (c == '\\' && i < s.size()) {
                char e = s[i++];
                if (e == 'n') out.push_back('\n');
                else if (e == 'r') out.push_back('\r');
                else if (e == 't') out.push_back('\t');
                else out.push_back(e);
            } else {
                out.push_back(c);
            }
        }
        return out;
    }
    TinyJsonVal parseVal(int depth = 0) {
        TinyJsonVal v;
        if (depth > 32) return v;
        skipWs();
        if (i >= s.size()) return v;
        char c = s[i];
        if (c == '"') {
            v.kind = TinyJsonVal::J_STR;
            v.sval = parseStr();
            return v;
        }
        if (c == '{') {
            v.kind = TinyJsonVal::J_OBJ;
            i++;
            skipWs();
            while (i < s.size() && s[i] != '}') {
                skipWs();
                std::string k = parseStr();
                skipWs();
                if (i < s.size() && s[i] == ':') i++;
                TinyJsonVal val = parseVal(depth + 1);
                if (!k.empty()) v.obj[k] = std::move(val);
                skipWs();
                if (i < s.size() && s[i] == ',') i++;
                else break;
            }
            if (i < s.size() && s[i] == '}') i++;
            return v;
        }
        if (c == '[') {
            v.kind = TinyJsonVal::J_ARR;
            i++;
            skipWs();
            while (i < s.size() && s[i] != ']') {
                v.arr.push_back(parseVal(depth + 1));
                skipWs();
                if (i < s.size() && s[i] == ',') i++;
                else break;
            }
            if (i < s.size() && s[i] == ']') i++;
            return v;
        }
        if (s.compare(i, 4, "true") == 0) {
            i += 4;
            v.kind = TinyJsonVal::J_BOOL;
            v.bval = true;
            return v;
        }
        if (s.compare(i, 5, "false") == 0) {
            i += 5;
            v.kind = TinyJsonVal::J_BOOL;
            v.bval = false;
            return v;
        }
        if (s.compare(i, 4, "null") == 0) {
            i += 4;
            v.kind = TinyJsonVal::J_NULL;
            return v;
        }
        size_t start = i;
        while (i < s.size() && (std::isdigit(static_cast<unsigned char>(s[i])) || s[i] == '-' || s[i] == '+' || s[i] == '.' || s[i] == 'e' || s[i] == 'E')) i++;
        if (i == start) i++;
        v.kind = TinyJsonVal::J_NUM;
        v.sval = s.substr(start, i - start);
        return v;
    }
};

inline void parseCapListOrObj(const TinyJsonVal &node, const std::string &scopeKey,
                              std::vector<std::string> &outCaps,
                              MemePermissions::PlatformPermissionsConfig &permCfg) {
    if (node.kind == TinyJsonVal::J_ARR) {
        for (const auto &el : node.arr) {
            if (el.kind == TinyJsonVal::J_STR && !el.sval.empty()) {
                outCaps.push_back(MemePermissions::normalizeCapability(el.sval));
            }
        }
    } else if (node.kind == TinyJsonVal::J_OBJ) {
        for (const auto &[k, v] : node.obj) {
            std::string cap = MemePermissions::normalizeCapability(k);
            if (v.kind == TinyJsonVal::J_BOOL) {
                if (v.bval) outCaps.push_back(cap);
            } else if (v.kind == TinyJsonVal::J_STR) {
                outCaps.push_back(cap);
                if (!v.sval.empty()) permCfg.descriptions[scopeKey][cap] = v.sval;
            } else if (v.kind == TinyJsonVal::J_OBJ) {
                outCaps.push_back(cap);
                auto rit = v.obj.find("reason");
                if (rit == v.obj.end()) rit = v.obj.find("description");
                if (rit == v.obj.end()) rit = v.obj.find("rationale");
                if (rit != v.obj.end() && rit->second.kind == TinyJsonVal::J_STR && !rit->second.sval.empty()) {
                    permCfg.descriptions[scopeKey][cap] = rit->second.sval;
                }
                auto mit = v.obj.find("mode");
                if (mit != v.obj.end() && mit->second.kind == TinyJsonVal::J_STR && !mit->second.sval.empty()) {
                    permCfg.modes[cap] = mit->second.sval;
                }
            }
        }
    }
}

inline PackageConfig mergeTapriConfig(const std::filesystem::path &srcPath, const PackageConfig &overrides) {
    PackageConfig cfg;
    std::filesystem::path absSrc = std::filesystem::absolute(srcPath);
    std::vector<std::filesystem::path> cands;
    if (absSrc.has_parent_path()) {
        std::filesystem::path pdir = absSrc.parent_path();
        cands.push_back(pdir / "tapri.json");
        if (pdir.filename() == "src" && pdir.has_parent_path()) {
            cands.push_back(pdir.parent_path() / "tapri.json");
        }
    }
    std::filesystem::path tapriDir;
    for (const auto &tp : cands) {
        if (!std::filesystem::exists(tp)) continue;
        tapriDir = tp.parent_path();
        std::ifstream tf(tp);
        if (!tf.is_open()) break;
        std::ostringstream ss;
        ss << tf.rdbuf();
        std::string txt = ss.str();

        TinyJsonParser jp{txt, 0};
        TinyJsonVal root = jp.parseVal();
        auto pick = [&](const char *k) -> std::string {
            if (root.kind == TinyJsonVal::J_OBJ) {
                auto it = root.obj.find(k);
                if (it != root.obj.end() && it->second.kind == TinyJsonVal::J_STR) return it->second.sval;
            }
            return extractJsonStrField(txt, k);
        };
        if (auto v = pick("displayName"); !v.empty()) cfg.displayName = v;
        else if (auto v2 = pick("name"); !v2.empty()) cfg.displayName = v2;
        if (auto v = pick("bundleId"); !v.empty()) cfg.bundleId = v;
        if (auto v = pick("version"); !v.empty()) cfg.version = v;
        if (auto v = pick("description"); !v.empty()) cfg.description = v;
        if (auto v = pick("author"); !v.empty()) cfg.author = v;
        if (auto v = pick("icon"); !v.empty()) cfg.iconPath = v;
        if (auto v = pick("scheme"); !v.empty()) cfg.urlScheme = v;
        else if (auto v2 = pick("urlScheme"); !v2.empty()) cfg.urlScheme = v2;
        if (auto v = pick("themeColor"); !v.empty()) cfg.themeColor = v;
        if (auto v = pick("bgColor"); !v.empty()) cfg.bgColor = v;
        if (auto v = pick("categories"); !v.empty()) cfg.categories = v;

        if (root.kind == TinyJsonVal::J_OBJ) {
            auto sit = root.obj.find("strictPermissions");
            if (sit != root.obj.end() && sit->second.kind == TinyJsonVal::J_BOOL) {
                cfg.permissions.strictMode = sit->second.bval;
            }
            auto pit = root.obj.find("permissions");
            if (pit != root.obj.end()) {
                cfg.permissions.hasPermissionsBlock = true;
                const TinyJsonVal &pnode = pit->second;
                if (pnode.kind == TinyJsonVal::J_ARR) {
                    parseCapListOrObj(pnode, "common", cfg.permissions.common, cfg.permissions);
                } else if (pnode.kind == TinyJsonVal::J_OBJ) {
                    for (const auto &[pk, pv] : pnode.obj) {
                        std::string low = MemePlatform::toLowerTrimmed(pk);
                        if (low == "strict" || low == "strictmode") {
                            if (pv.kind == TinyJsonVal::J_BOOL) cfg.permissions.strictMode = pv.bval;
                        } else if (low == "common" || low == "all" || low == "shared") {
                            parseCapListOrObj(pv, "common", cfg.permissions.common, cfg.permissions);
                        } else if (low == "descriptions" || low == "reasons" || low == "rationale") {
                            if (pv.kind == TinyJsonVal::J_OBJ) {
                                for (const auto &[dk, dv] : pv.obj) {
                                    std::string dplat = MemePlatform::normalizePlatformName(dk);
                                    if (dv.kind == TinyJsonVal::J_STR && !dv.sval.empty()) {
                                        cfg.permissions.descriptions["common"][MemePermissions::normalizeCapability(dk)] = dv.sval;
                                    } else if (dv.kind == TinyJsonVal::J_OBJ && MemePlatform::isKnownConcretePlatform(dplat)) {
                                        for (const auto &[ck, cv] : dv.obj) {
                                            if (cv.kind == TinyJsonVal::J_STR && !cv.sval.empty()) {
                                                cfg.permissions.descriptions[dplat][MemePermissions::normalizeCapability(ck)] = cv.sval;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            std::string normPlat = MemePlatform::normalizePlatformName(pk);
                            if (MemePlatform::isKnownConcretePlatform(normPlat)) {
                                parseCapListOrObj(pv, normPlat, cfg.permissions.perPlatform[normPlat], cfg.permissions);
                            } else {
                                // Direct capability key inside permissions object
                                std::string cap = MemePermissions::normalizeCapability(pk);
                                if (pv.kind == TinyJsonVal::J_BOOL && pv.bval) {
                                    cfg.permissions.common.push_back(cap);
                                } else if (pv.kind == TinyJsonVal::J_STR) {
                                    cfg.permissions.common.push_back(cap);
                                    if (!pv.sval.empty()) cfg.permissions.descriptions["common"][cap] = pv.sval;
                                } else if (pv.kind == TinyJsonVal::J_OBJ) {
                                    cfg.permissions.common.push_back(cap);
                                }
                            }
                        }
                    }
                }
            }
        }
        break;
    }
    if (!overrides.displayName.empty()) cfg.displayName = overrides.displayName;
    if (!overrides.bundleId.empty()) cfg.bundleId = overrides.bundleId;
    if (!overrides.version.empty() && overrides.version != "1.0.0") cfg.version = overrides.version;
    else if (cfg.version.empty()) cfg.version = "1.0.0";
    if (!overrides.description.empty()) cfg.description = overrides.description;
    if (!overrides.author.empty() && overrides.author != "MEMElang") cfg.author = overrides.author;
    if (!overrides.iconPath.empty()) cfg.iconPath = overrides.iconPath;
    else if (!cfg.iconPath.empty() && !tapriDir.empty() && std::filesystem::path(cfg.iconPath).is_relative()) {
        cfg.iconPath = (tapriDir / cfg.iconPath).string();
    }
    if (!overrides.urlScheme.empty()) cfg.urlScheme = overrides.urlScheme;
    if (!overrides.themeColor.empty() && overrides.themeColor != "#020617") cfg.themeColor = overrides.themeColor;
    if (!overrides.bgColor.empty() && overrides.bgColor != "#020617") cfg.bgColor = overrides.bgColor;
    if (!overrides.categories.empty() && overrides.categories != "Utility;") cfg.categories = overrides.categories;
    if (overrides.permissions.hasPermissionsBlock) cfg.permissions = overrides.permissions;
    return cfg;
}

} // namespace detail

inline PackageResult packageForTarget(
    const std::string &sourceFile,
    const std::string &rawTarget,
    const std::string &outDirOrPath,
    bool verbose,
    const PackageConfig &cliConfig)
{
    auto t0 = std::chrono::high_resolution_clock::now();
    std::string target = rawTarget;
    for (auto &c : target) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    PackageResult res;
    res.target = target;

    std::filesystem::path srcPath(sourceFile);
    if (!std::filesystem::exists(srcPath)) {
        res.error = "Source file not found: " + sourceFile;
        return res;
    }

    std::filesystem::path absSrc = std::filesystem::absolute(srcPath);
    std::string stem = absSrc.stem().string();
    std::string pkgName = detail::sanitizePkgName(stem);
    std::string appName = detail::toAppName(stem);
    if (stem == "App" && absSrc.has_parent_path()) {
        auto parentName = absSrc.parent_path().filename().string();
        if (parentName == "src" && absSrc.parent_path().has_parent_path()) {
            parentName = absSrc.parent_path().parent_path().filename().string();
        }
        if (!parentName.empty() && parentName != "." && parentName != "desktop") {
            pkgName = detail::sanitizePkgName(parentName);
            appName = detail::toAppName(parentName);
        }
    }

    PackageConfig cfg = detail::mergeTapriConfig(srcPath, cliConfig);
    std::string displayTitle = !cfg.displayName.empty() ? cfg.displayName : appName;
    if (!cliConfig.displayName.empty()) {
        appName = cliConfig.displayName;
        pkgName = detail::sanitizePkgName(cliConfig.displayName);
    }
    if (cfg.bundleId.empty()) cfg.bundleId = "dev.memelang." + pkgName;
    if (cfg.version.empty()) cfg.version = "1.0.0";
    if (cfg.description.empty()) cfg.description = displayTitle + " built natively with MEME-UI";
    std::string schemeClean = detail::sanitizePkgName(cfg.urlScheme);

    std::string normTargetPlat = MemePlatform::normalizePlatformName(target);
    std::vector<std::string> declaredCaps = cfg.permissions.resolvedForTarget(target);
    auto hasDeclaredCap = [&](const std::string &rawCap) {
        std::string nc = MemePermissions::normalizeCapability(rawCap);
        for (const auto &c : declaredCaps) {
            if (c == nc) return true;
        }
        return false;
    };

    IconImage baseIcon = loadOrDefaultIcon(cfg.iconPath, 512);
    IconImage smallIcon64 = resizeBilinearRgba(baseIcon, 64, 64);

    std::filesystem::path repoRoot = detail::findRepoRoot();
    std::filesystem::path outBase = outDirOrPath.empty() ? std::filesystem::path("dist") : std::filesystem::path(outDirOrPath);
    std::filesystem::create_directories(outBase);

    // Parse & generate C++ source once for the target platform
    std::ifstream ifs(srcPath);
    std::stringstream ss;
    ss << ifs.rdbuf();
    std::string cppCode;
    try {
        Lexer lexer(ss.str());
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto program = parser.parseProgram();
        std::string targetArg = MemePlatform::isKnownConcretePlatform(normTargetPlat) ? normTargetPlat : "";
        cppCode = NativeCompiler::generateCpp(program, sourceFile, targetArg);
    } catch (const std::exception &e) {
        res.error = std::string("Failed to compile MEMElang AST: ") + e.what();
        return res;
    }

    // Inject runtime app identity + target + declared permissions + embedded icon RGBA cleanly into generated C++
    {
        std::ostringstream bootInject;
        bootInject << "\nnamespace {\n";
        bootInject << "struct MemePkgBootInit {\n    MemePkgBootInit() {\n";
        bootInject << "        MemeNative::setAppIdentity(\"" << MemeNative::detail::jsonEscape(cfg.bundleId) << "\", \""
                   << MemeNative::detail::jsonEscape(displayTitle) << "\");\n";
        if (MemePlatform::isKnownConcretePlatform(normTargetPlat)) {
            bootInject << "        MemeNative::setBuildTarget(\"" << MemeNative::detail::jsonEscape(normTargetPlat) << "\");\n";
        }
        if (cfg.permissions.hasPermissionsBlock || !declaredCaps.empty()) {
            bootInject << "        std::vector<std::string> _declCaps = {";
            for (size_t i = 0; i < declaredCaps.size(); ++i) {
                if (i > 0) bootInject << ",";
                bootInject << "\"" << MemeNative::detail::jsonEscape(declaredCaps[i]) << "\"";
            }
            bootInject << "};\n";
            bootInject << "        std::unordered_map<std::string, std::string> _declDescs;\n";
            for (const auto &dc : declaredCaps) {
                std::string rs = cfg.permissions.reasonFor(target, dc, displayTitle);
                if (!rs.empty()) {
                    bootInject << "        _declDescs[\"" << MemeNative::detail::jsonEscape(dc) << "\"] = \""
                               << MemeNative::detail::jsonEscape(rs) << "\";\n";
                }
            }
            bootInject << "        MemePermissions::setDeclaredPermissions(_declCaps, "
                       << (cfg.permissions.strictMode ? "true" : "false") << ", _declDescs);\n";
        }
        if (!smallIcon64.rgba.empty()) {
            bootInject << "        static const uint8_t kIconRgba[" << smallIcon64.rgba.size() << "] = {";
            for (size_t i = 0; i < smallIcon64.rgba.size(); ++i) {
                if (i > 0) bootInject << ",";
                bootInject << static_cast<unsigned int>(smallIcon64.rgba[i]);
            }
            bootInject << "};\n";
            bootInject << "        MemeNative::setAppIconRgba(kIconRgba, " << smallIcon64.width << ", " << smallIcon64.height << ");\n";
        }
        bootInject << "    }\n} g_memePkgBootInit;\n}\n\n";
        size_t epPos = cppCode.find("#ifdef MEME_HOST_PROVIDES_MAIN");
        if (epPos != std::string::npos) {
            cppCode.insert(epPos, bootInject.str());
        }
    }

    bool usesUi = (cppCode.find("#include \"src/compiler/MemeRuntimeUi.h\"") != std::string::npos);
    bool usesEval = (cppCode.find("#define MEME_HAS_EVAL 1") != std::string::npos);
    std::string evalFlags = usesEval
        ? (" -DMEME_HAS_EVAL -DMEME_NO_INTERP_DESKTOP -x c++ \"" +
           (repoRoot / "src" / "interpreter" / "Interpreter.cpp").string() + "\" \"" +
           (repoRoot / "src" / "types" / "TypeChecker.cpp").string() + "\"")
        : "";
    std::string evalFlagsNoLang = usesEval
        ? (" -DMEME_HAS_EVAL -DMEME_NO_INTERP_DESKTOP \"" +
           (repoRoot / "src" / "interpreter" / "Interpreter.cpp").string() + "\" \"" +
           (repoRoot / "src" / "types" / "TypeChecker.cpp").string() + "\"")
        : "";
    std::filesystem::path workDir = std::filesystem::temp_directory_path() / ("meme_pkg_" + pkgName);
    std::filesystem::create_directories(workDir);
    std::filesystem::path cppPath = workDir / (pkgName + "_gen.cpp");
    detail::writeFileText(cppPath, cppCode);

    auto finishOk = [&](const std::string &primary, const std::vector<std::string> &arts) {
        auto t1 = std::chrono::high_resolution_clock::now();
        res.success = true;
        res.outputPath = primary;
        res.artifacts = arts;
        res.elapsedMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
        return res;
    };

    if (target == "mac" || target == "macos") {
        std::filesystem::path appBundle = outBase / (appName + ".app");
        std::filesystem::path macOsDir = appBundle / "Contents" / "MacOS";
        std::filesystem::path resDir = appBundle / "Contents" / "Resources";
        std::filesystem::create_directories(macOsDir);
        std::filesystem::create_directories(resDir);
        std::filesystem::path binPath = macOsDir / appName;

        std::string cmd = "clang++ -std=c++20 -O3 -DNDEBUG -DMEME_NO_DEPTH_GUARD -I\"" + repoRoot.string() + "\" ";
        if (usesUi) cmd += "-x objective-c++ ";
        cmd += "\"" + cppPath.string() + "\"" + evalFlags + " ";
        if (usesUi) {
            cmd += "-framework Cocoa -framework CoreGraphics -framework Metal -framework QuartzCore ";
            if (hasDeclaredCap("notifications")) cmd += "-framework UserNotifications ";
        }
        cmd += "-o \"" + binPath.string() + "\"";
        if (verbose) std::cout << "[package:mac] " << cmd << "\n";
        std::string out;
        if (detail::execCmd(cmd, out) != 0) {
            res.error = "macOS build failed:\n" + out;
            return res;
        }

        std::filesystem::path icnsPath = resDir / "AppIcon.icns";
        detail::writeFileBytes(icnsPath, buildIcnsFromRgba(baseIcon));

        std::string urlTypesXml;
        if (!schemeClean.empty()) {
            urlTypesXml =
                "  <key>CFBundleURLTypes</key>\n"
                "  <array><dict>\n"
                "    <key>CFBundleURLName</key><string>" + detail::xmlEscape(cfg.bundleId) + "</string>\n"
                "    <key>CFBundleURLSchemes</key><array><string>" + detail::xmlEscape(schemeClean) + "</string></array>\n"
                "  </dict></array>\n";
        }
        std::string macUsageXml;
        std::unordered_set<std::string> seenUsageKeys;
        for (const auto &cap : declaredCaps) {
            auto kv = MemePermissions::appleUsagePlistKeyFor(cap, displayTitle);
            if (!kv.first.empty() && seenUsageKeys.insert(kv.first).second) {
                std::string reason = cfg.permissions.reasonFor("mac", cap, displayTitle);
                if (reason.empty()) reason = kv.second;
                macUsageXml += "  <key>" + kv.first + "</key><string>" + detail::xmlEscape(reason) + "</string>\n";
            }
        }
        std::string plist = R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
  <key>CFBundleDevelopmentRegion</key><string>en</string>
  <key>CFBundleExecutable</key><string>)" + detail::xmlEscape(appName) + R"(</string>
  <key>CFBundleDisplayName</key><string>)" + detail::xmlEscape(displayTitle) + R"(</string>
  <key>CFBundleIdentifier</key><string>)" + detail::xmlEscape(cfg.bundleId) + R"(</string>
  <key>CFBundleInfoDictionaryVersion</key><string>6.0</string>
  <key>CFBundleName</key><string>)" + detail::xmlEscape(displayTitle) + R"(</string>
  <key>CFBundleIconFile</key><string>AppIcon</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>CFBundleShortVersionString</key><string>)" + detail::xmlEscape(cfg.version) + R"(</string>
  <key>CFBundleVersion</key><string>1</string>
  <key>LSApplicationCategoryType</key><string>public.app-category.utilities</string>
  <key>NSHighResolutionCapable</key><true/>
  <key>NSSupportsAutomaticGraphicsSwitching</key><true/>
)" + macUsageXml + urlTypesXml + R"(</dict></plist>)";
        detail::writeFileText(appBundle / "Contents" / "Info.plist", plist);
        detail::writeFileText(appBundle / "Contents" / "PkgInfo", "APPL????");

        std::filesystem::path entitlementsPath = workDir / (pkgName + "_entitlements.plist");
        std::ostringstream entXml;
        entXml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
               << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
               << "<plist version=\"1.0\"><dict>\n"
               << "  <key>com.apple.security.network.client</key><true/>\n";
        std::unordered_set<std::string> seenEnt;
        for (const auto &cap : declaredCaps) {
            std::string ek = MemePermissions::macEntitlementKeyFor(cap);
            if (!ek.empty() && ek != "com.apple.security.network.client" && seenEnt.insert(ek).second) {
                entXml << "  <key>" << ek << "</key><true/>\n";
            }
        }
        entXml << "</dict></plist>\n";
        detail::writeFileText(entitlementsPath, entXml.str());

        std::string signOut;
        if (detail::execCmd("codesign --force --sign - --entitlements \"" + entitlementsPath.string() + "\" \"" + appBundle.string() + "\"", signOut) != 0) {
            detail::execCmd("codesign --force --sign - \"" + appBundle.string() + "\"", signOut);
        }
        return finishOk(appBundle.string(), {appBundle.string(), binPath.string(), icnsPath.string()});
    }

    if (target == "win" || target == "windows") {
        std::string zig = detail::findInPathOrPaths("zig", {"/opt/homebrew/bin/zig", "/usr/local/bin/zig"});
        if (zig.empty()) {
            res.error = "zig compiler not found for Windows cross-compilation";
            return res;
        }
        std::filesystem::path exePath = outBase / (pkgName + ".exe");
        std::filesystem::path icoPath = outBase / (pkgName + ".ico");
        detail::writeFileBytes(icoPath, buildIcoFromRgba(baseIcon));

        std::string extraResArg;
        std::string rcTool = detail::findInPathOrPaths("llvm-rc", {"/opt/homebrew/opt/llvm/bin/llvm-rc", "/usr/local/opt/llvm/bin/llvm-rc"});
        if (rcTool.empty()) rcTool = detail::findInPathOrPaths("x86_64-w64-mingw32-windres", {"/opt/homebrew/bin/x86_64-w64-mingw32-windres"});
        if (!rcTool.empty()) {
            std::filesystem::path rcFile = workDir / (pkgName + ".rc");
            std::filesystem::path resFile = workDir / (pkgName + ".res");
            std::filesystem::path winManifestPath = workDir / (pkgName + ".manifest");
            std::string capListComment;
            for (size_t i = 0; i < declaredCaps.size(); ++i) {
                if (i > 0) capListComment += ",";
                capListComment += declaredCaps[i];
            }
            std::string winManifest =
                "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
                "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">\n"
                "  <assemblyIdentity version=\"" + detail::xmlEscape(cfg.version) + ".0\" processorArchitecture=\"amd64\" name=\"" + detail::xmlEscape(cfg.bundleId) + "\" type=\"win32\"/>\n"
                "  <description>" + detail::xmlEscape(cfg.description) + " (capabilities: " + detail::xmlEscape(capListComment) + ")</description>\n"
                "  <trustInfo xmlns=\"urn:schemas-microsoft-com:asm.v3\">\n"
                "    <security><requestedPrivileges><requestedExecutionLevel level=\"asInvoker\" uiAccess=\"false\"/></requestedPrivileges></security>\n"
                "  </trustInfo>\n"
                "</assembly>\n";
            detail::writeFileText(winManifestPath, winManifest);
            detail::writeFileText(rcFile, "1 ICON \"" + icoPath.string() + "\"\n1 24 \"" + winManifestPath.string() + "\"\n");
            std::string rcOut;
            std::string rcCmd = (rcTool.find("windres") != std::string::npos)
                ? ("\"" + rcTool + "\" \"" + rcFile.string() + "\" -O coff -o \"" + resFile.string() + "\"")
                : ("\"" + rcTool + "\" /fo \"" + resFile.string() + "\" \"" + rcFile.string() + "\"");
            if (detail::execCmd(rcCmd, rcOut) == 0 && std::filesystem::exists(resFile)) {
                extraResArg = " \"" + resFile.string() + "\"";
            }
        }

        std::string cmd = "\"" + zig + "\" c++ -std=c++20 -O3 -DNDEBUG -DMEME_NO_DEPTH_GUARD "
                          "-target x86_64-windows-gnu -I\"" + repoRoot.string() + "\" "
                          "\"" + cppPath.string() + "\"" + evalFlagsNoLang + extraResArg + " -luser32 -lgdi32 -ld2d1 -ldwrite -lws2_32 -lole32 "
                          "-o \"" + exePath.string() + "\"";
        if (verbose) std::cout << "[package:win] " << cmd << "\n";
        std::string out;
        if (detail::execCmd(cmd, out) != 0) {
            res.error = "Windows PE32+ build failed:\n" + out;
            return res;
        }
        return finishOk(exePath.string(), {exePath.string(), icoPath.string()});
    }

    auto makeLinuxDesktop = [&]() {
        std::string d = "[Desktop Entry]\nType=Application\nName=" + displayTitle +
                        "\nComment=" + cfg.description +
                        "\nExec=/usr/local/bin/" + pkgName +
                        "\nIcon=" + pkgName +
                        "\nTerminal=false\nCategories=" + cfg.categories +
                        "\nStartupWMClass=" + pkgName + "\n";
        if (!schemeClean.empty()) d += "MimeType=x-scheme-handler/" + schemeClean + ";\n";
        return d;
    };

    auto makeLinuxCapJson = [&]() {
        std::ostringstream js;
        js << "{\n  \"package\": \"" << MemeNative::detail::jsonEscape(pkgName) << "\",\n"
           << "  \"platform\": \"linux\",\n"
           << "  \"strict\": " << (cfg.permissions.strictMode ? "true" : "false") << ",\n"
           << "  \"permissions\": [";
        for (size_t i = 0; i < declaredCaps.size(); ++i) {
            if (i > 0) js << ", ";
            js << "\"" << MemeNative::detail::jsonEscape(declaredCaps[i]) << "\"";
        }
        js << "]\n}\n";
        return js.str();
    };

    if (target == "deb") {
        std::filesystem::path elfPath = workDir / (pkgName + "_linux_x86_64");
        std::string err;
        if (!detail::buildLinuxElf(repoRoot, cppPath, elfPath, err, verbose, usesEval)) {
            res.error = err;
            return res;
        }
        auto elfBytes = detail::readFileBytes(elfPath);
        auto icon256 = encodePngRgba(resizeBilinearRgba(baseIcon, 256, 256));
        std::string desktop = makeLinuxDesktop();
        std::string capJson = makeLinuxCapJson();
        std::vector<ArchiveFileEntry> dataEntries = {
            {"./usr/", {}, 0755, true},
            {"./usr/local/", {}, 0755, true},
            {"./usr/local/bin/", {}, 0755, true},
            {"./usr/local/bin/" + pkgName, elfBytes, 0755, false},
            {"./usr/share/", {}, 0755, true},
            {"./usr/share/applications/", {}, 0755, true},
            {"./usr/share/applications/" + pkgName + ".desktop", detail::strToBytes(desktop), 0644, false},
            {"./usr/share/" + pkgName + "/", {}, 0755, true},
            {"./usr/share/" + pkgName + "/capabilities.json", detail::strToBytes(capJson), 0644, false},
            {"./usr/share/icons/", {}, 0755, true},
            {"./usr/share/icons/hicolor/", {}, 0755, true},
            {"./usr/share/icons/hicolor/256x256/", {}, 0755, true},
            {"./usr/share/icons/hicolor/256x256/apps/", {}, 0755, true},
            {"./usr/share/icons/hicolor/256x256/apps/" + pkgName + ".png", icon256, 0644, false},
            {"./usr/share/pixmaps/", {}, 0755, true},
            {"./usr/share/pixmaps/" + pkgName + ".png", icon256, 0644, false},
        };
        auto dataGz = gzipBytes(buildTar(dataEntries));

        size_t kb = std::max<size_t>(1, (elfBytes.size() + desktop.size() + icon256.size()) / 1024);
        std::string control = "Package: " + pkgName + "\nVersion: " + cfg.version + "\nArchitecture: amd64\n"
                              "Maintainer: " + cfg.author + " <dev@memelang.dev>\nInstalled-Size: " + std::to_string(kb) +
                              "\nSection: utils\nPriority: optional\nDescription: " + cfg.description + "\n";
        auto ctrlGz = gzipBytes(buildTar({{"./control", detail::strToBytes(control), 0644, false}}));
        auto arBytes = buildArArchive({
            {"debian-binary", detail::strToBytes("2.0\n")},
            {"control.tar.gz", ctrlGz},
            {"data.tar.gz", dataGz},
        });
        std::filesystem::path debPath = outBase / (pkgName + "_" + cfg.version + "_amd64.deb");
        detail::writeFileBytes(debPath, arBytes);
        return finishOk(debPath.string(), {debPath.string()});
    }

    if (target == "rpm") {
        std::filesystem::path elfPath = workDir / (pkgName + "_linux_x86_64");
        std::string err;
        if (!detail::buildLinuxElf(repoRoot, cppPath, elfPath, err, verbose, usesEval)) {
            res.error = err;
            return res;
        }
        auto elfBytes = detail::readFileBytes(elfPath);
        auto icon256 = encodePngRgba(resizeBilinearRgba(baseIcon, 256, 256));
        std::string desktop = makeLinuxDesktop();
        std::string capJson = makeLinuxCapJson();
        std::vector<ArchiveFileEntry> files = {
            {"/usr/local/bin/" + pkgName, elfBytes, 0755, false},
            {"/usr/share/applications/" + pkgName + ".desktop", detail::strToBytes(desktop), 0644, false},
            {"/usr/share/" + pkgName + "/capabilities.json", detail::strToBytes(capJson), 0644, false},
            {"/usr/share/icons/hicolor/256x256/apps/" + pkgName + ".png", icon256, 0644, false},
            {"/usr/share/pixmaps/" + pkgName + ".png", icon256, 0644, false},
        };
        auto rpmBytes = buildRpmPackage(pkgName, cfg.version, "1", "x86_64", cfg.description, files);
        std::filesystem::path rpmPath = outBase / (pkgName + "-" + cfg.version + "-1.x86_64.rpm");
        detail::writeFileBytes(rpmPath, rpmBytes);
        return finishOk(rpmPath.string(), {rpmPath.string()});
    }

    if (target == "ipk") {
        std::filesystem::path elfPath = workDir / (pkgName + "_linux_x86_64");
        std::string err;
        if (!detail::buildLinuxElf(repoRoot, cppPath, elfPath, err, verbose, usesEval)) {
            res.error = err;
            return res;
        }
        auto elfBytes = detail::readFileBytes(elfPath);
        auto icon128 = encodePngRgba(resizeBilinearRgba(baseIcon, 128, 128));
        std::string desktop = makeLinuxDesktop();
        std::string capJson = makeLinuxCapJson();
        std::vector<ArchiveFileEntry> dataEntries = {
            {"./usr/", {}, 0755, true},
            {"./usr/bin/", {}, 0755, true},
            {"./usr/bin/" + pkgName, elfBytes, 0755, false},
            {"./usr/share/", {}, 0755, true},
            {"./usr/share/applications/", {}, 0755, true},
            {"./usr/share/applications/" + pkgName + ".desktop", detail::strToBytes(desktop), 0644, false},
            {"./usr/share/" + pkgName + "/", {}, 0755, true},
            {"./usr/share/" + pkgName + "/capabilities.json", detail::strToBytes(capJson), 0644, false},
            {"./usr/share/pixmaps/", {}, 0755, true},
            {"./usr/share/pixmaps/" + pkgName + ".png", icon128, 0644, false},
        };
        auto dataGz = gzipBytes(buildTar(dataEntries));
        size_t kb = std::max<size_t>(1, (elfBytes.size() + icon128.size()) / 1024);
        std::string control = "Package: " + pkgName + "\nVersion: " + cfg.version + "\nArchitecture: x86_64\n"
                              "Maintainer: " + cfg.author + " <dev@memelang.dev>\nInstalled-Size: " + std::to_string(kb) +
                              "\nSection: utils\nDescription: " + cfg.description + "\n";
        auto ctrlGz = gzipBytes(buildTar({{"./control", detail::strToBytes(control), 0644, false}}));
        auto ipkBytes = gzipBytes(buildTar({
            {"./debian-binary", detail::strToBytes("2.0\n"), 0644, false},
            {"./control.tar.gz", ctrlGz, 0644, false},
            {"./data.tar.gz", dataGz, 0644, false},
        }));
        std::filesystem::path ipkPath = outBase / (pkgName + "_" + cfg.version + "_x86_64.ipk");
        detail::writeFileBytes(ipkPath, ipkBytes);
        return finishOk(ipkPath.string(), {ipkPath.string()});
    }

    if (target == "apk") {
        auto sdk = detail::findAndroidSdk();
        if (sdk.empty()) {
            res.error = "Android SDK not found (set ANDROID_HOME)";
            return res;
        }
        auto ndk = detail::newestSubdir(sdk / "ndk");
        auto bt = detail::newestSubdir(sdk / "build-tools");
        auto platDir = detail::newestSubdir(sdk / "platforms");
        std::filesystem::path androidJar = platDir / "android.jar";
        std::filesystem::path toolchainBin = ndk / "toolchains" / "llvm" / "prebuilt" / "darwin-x86_64" / "bin";
        if (!std::filesystem::exists(toolchainBin)) {
            toolchainBin = ndk / "toolchains" / "llvm" / "prebuilt" / "linux-x86_64" / "bin";
        }
        std::filesystem::path ccArm = toolchainBin / "aarch64-linux-android24-clang++";
        std::filesystem::path ccX64 = toolchainBin / "x86_64-linux-android24-clang++";
        std::filesystem::path aapt2 = bt / "aapt2";
        std::filesystem::path zipalign = bt / "zipalign";
        std::filesystem::path apksigner = bt / "apksigner";
        if (!std::filesystem::exists(ccArm) || !std::filesystem::exists(aapt2) || !std::filesystem::exists(apksigner) || !std::filesystem::exists(androidJar)) {
            res.error = "Android SDK/NDK components missing in " + sdk.string();
            return res;
        }

        std::filesystem::path stage = workDir / "apk_stage";
        std::filesystem::remove_all(stage);
        std::filesystem::create_directories(stage / "lib" / "arm64-v8a");
        std::filesystem::create_directories(stage / "lib" / "x86_64");

        std::filesystem::path hostCpp = repoRoot / "src" / "hosts" / "android" / "MemeAndroidHost.cpp";
        auto buildAbi = [&](const std::filesystem::path &cc, const std::string &abi) -> bool {
            std::filesystem::path soOut = stage / "lib" / abi / "libmemeapp.so";
            std::string cmd = "\"" + cc.string() + "\" -std=c++20 -O3 -DNDEBUG -DMEME_NO_DEPTH_GUARD -DMEME_HOST_PROVIDES_MAIN "
                              "-shared -fPIC -I\"" + repoRoot.string() + "\" \"" + cppPath.string() + "\"" + evalFlagsNoLang + " "
                              "\"" + hostCpp.string() + "\" -landroid -llog -lEGL -lGLESv2 -o \"" + soOut.string() + "\"";
            if (verbose) std::cout << "[package:apk:" << abi << "] " << cmd << "\n";
            std::string out;
            if (detail::execCmd(cmd, out) != 0) {
                res.error = "Android NDK (" + abi + ") build failed:\n" + out;
                return false;
            }
            return true;
        };
        if (!buildAbi(ccArm, "arm64-v8a")) return res;
        if (std::filesystem::exists(ccX64) && !buildAbi(ccX64, "x86_64")) return res;

        std::string javaPkg = detail::sanitizeAndroidPkg(cfg.bundleId, pkgName);
        struct AndroidMip { const char *dir; int size; };
        static constexpr AndroidMip kMips[] = {
            {"mipmap-mdpi", 48}, {"mipmap-hdpi", 72}, {"mipmap-xhdpi", 96}, {"mipmap-xxhdpi", 144}, {"mipmap-xxxhdpi", 192}
        };
        std::filesystem::path resStageDir = stage / "res";
        for (const auto &m : kMips) {
            auto png = encodePngRgba(resizeBilinearRgba(baseIcon, m.size, m.size));
            detail::writeFileBytes(resStageDir / m.dir / "ic_launcher.png", png);
            detail::writeFileBytes(resStageDir / m.dir / "ic_launcher_round.png", png);
        }

        // Build Java NativeActivity shim (classes.dex) for runtime permission callbacks when javac + d8 exist
        bool hasDex = false;
        std::filesystem::path d8Tool = bt / "d8";
        std::string javacTool = detail::findInPathOrPaths("javac", {"/usr/bin/javac"});
        if (!javacTool.empty() && std::filesystem::exists(d8Tool)) {
            std::filesystem::path javaSrcDir = stage / "java_src" / "dev" / "memelang" / "host";
            std::filesystem::path javaClsDir = stage / "java_classes";
            std::filesystem::create_directories(javaSrcDir);
            std::filesystem::create_directories(javaClsDir);
            std::filesystem::path javaFile = javaSrcDir / "MemeNativeActivity.java";
            const char *kMemeNativeActivityJava = R"JAVA(package dev.memelang.host;

import android.app.NativeActivity;
import android.content.pm.PackageManager;
import android.os.Build;

public class MemeNativeActivity extends NativeActivity {
    static {
        try {
            System.loadLibrary("memeapp");
        } catch (Throwable ignored) {}
    }

    public static native void nativeOnPermissionResult(int requestCode, boolean granted, boolean canAskAgain);

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        boolean granted = grantResults != null && grantResults.length > 0
                && grantResults[0] == PackageManager.PERMISSION_GRANTED;
        boolean canAskAgain = false;
        if (!granted && permissions != null && permissions.length > 0 && Build.VERSION.SDK_INT >= 23) {
            canAskAgain = shouldShowRequestPermissionRationale(permissions[0]);
        }
        try {
            nativeOnPermissionResult(requestCode, granted, canAskAgain);
        } catch (Throwable ignored) {}
    }
}
)JAVA";
            detail::writeFileText(javaFile, kMemeNativeActivityJava);
            std::string jOut;
            std::string jcCmd = "\"" + javacTool + "\" -source 11 -target 11 -cp \"" + androidJar.string() + "\" -d \"" +
                                javaClsDir.string() + "\" \"" + javaFile.string() + "\"";
            if (detail::execCmd(jcCmd, jOut) == 0) {
                std::filesystem::path classFile = javaClsDir / "dev" / "memelang" / "host" / "MemeNativeActivity.class";
                std::string d8Cmd = "\"" + d8Tool.string() + "\" --min-api 24 --lib \"" + androidJar.string() +
                                    "\" --output \"" + stage.string() + "\" \"" + classFile.string() + "\"";
                if (detail::execCmd(d8Cmd, jOut) == 0 && std::filesystem::exists(stage / "classes.dex")) {
                    hasDex = true;
                }
            }
        }

        // Build selective Android <uses-permission> XML tags
        std::vector<MemePermissions::AndroidPermissionEntry> androidPerms = {
            {"android.permission.INTERNET", 0, ""},
            {"android.permission.ACCESS_NETWORK_STATE", 0, ""},
            {"android.permission.VIBRATE", 0, ""}
        };
        std::unordered_set<std::string> seenAndPerms = {
            "android.permission.INTERNET",
            "android.permission.ACCESS_NETWORK_STATE",
            "android.permission.VIBRATE"
        };
        for (const auto &cap : declaredCaps) {
            std::string mode;
            auto mit = cfg.permissions.modes.find(cap);
            if (mit != cfg.permissions.modes.end()) mode = mit->second;
            for (const auto &entry : MemePermissions::androidPermissionsFor(cap, mode)) {
                if (seenAndPerms.insert(entry.name).second) {
                    androidPerms.push_back(entry);
                }
            }
        }
        std::string androidPermsXml;
        for (const auto &ap : androidPerms) {
            androidPermsXml += "    <uses-permission android:name=\"" + detail::xmlEscape(ap.name) + "\"";
            if (ap.maxSdkVersion > 0) {
                androidPermsXml += " android:maxSdkVersion=\"" + std::to_string(ap.maxSdkVersion) + "\"";
            }
            if (!ap.flags.empty()) {
                androidPermsXml += " android:usesPermissionFlags=\"" + detail::xmlEscape(ap.flags) + "\"";
            }
            androidPermsXml += " />\n";
        }

        std::string schemeFilter;
        if (!schemeClean.empty()) {
            schemeFilter =
                "\n            <intent-filter>\n"
                "                <action android:name=\"android.intent.action.VIEW\" />\n"
                "                <category android:name=\"android.intent.category.DEFAULT\" />\n"
                "                <category android:name=\"android.intent.category.BROWSABLE\" />\n"
                "                <data android:scheme=\"" + detail::xmlEscape(schemeClean) + "\" />\n"
                "            </intent-filter>";
        }
        std::string activityCls = hasDex ? "dev.memelang.host.MemeNativeActivity" : "android.app.NativeActivity";
        std::string manifest = R"(<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package=")" + detail::xmlEscape(javaPkg) + R"("
    android:versionCode="1"
    android:versionName=")" + detail::xmlEscape(cfg.version) + R"(">
    <uses-sdk android:minSdkVersion="24" android:targetSdkVersion="35" />
)" + androidPermsXml + R"(    <application android:label=")" + detail::xmlEscape(displayTitle) + R"(" android:icon="@mipmap/ic_launcher" android:roundIcon="@mipmap/ic_launcher_round" android:hasCode=")" + (hasDex ? "true" : "false") + R"(" android:extractNativeLibs="false">
        <activity android:name=")" + activityCls + R"("
            android:label=")" + detail::xmlEscape(displayTitle) + R"("
            android:configChanges="orientation|keyboardHidden|screenSize|smallestScreenSize"
            android:exported="true">
            <meta-data android:name="android.app.lib_name" android:value="memeapp" />
            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
                <category android:name="android.intent.category.LAUNCHER" />
            </intent-filter>)" + schemeFilter + R"(
        </activity>
    </application>
</manifest>
)";
        std::filesystem::path manifestPath = stage / "AndroidManifest.xml";
        detail::writeFileText(manifestPath, manifest);

        std::filesystem::path compiledResZip = stage / "compiled_res.zip";
        std::string out;
        if (detail::execCmd("\"" + aapt2.string() + "\" compile --dir \"" + resStageDir.string() + "\" -o \"" + compiledResZip.string() + "\"", out) != 0) {
            res.error = "aapt2 compile failed:\n" + out;
            return res;
        }

        std::filesystem::path resApk = stage / "res.apk";
        std::filesystem::path alignedApk = stage / "aligned.apk";
        if (detail::execCmd("\"" + aapt2.string() + "\" link -I \"" + androidJar.string() + "\" --manifest \"" +
                            manifestPath.string() + "\" -R \"" + compiledResZip.string() + "\" -o \"" + resApk.string() + "\" --auto-add-overlay", out) != 0) {
            res.error = "aapt2 link failed:\n" + out;
            return res;
        }
        std::string zipItems = hasDex ? "lib classes.dex" : "lib";
        if (detail::execCmd("cd \"" + stage.string() + "\" && zip -0 -r \"" + resApk.string() + "\" " + zipItems, out) != 0) {
            res.error = "zip .so/.dex injection failed:\n" + out;
            return res;
        }
        if (detail::execCmd("\"" + zipalign.string() + "\" -f -p 4 \"" + resApk.string() + "\" \"" + alignedApk.string() + "\"", out) != 0) {
            res.error = "zipalign failed:\n" + out;
            return res;
        }

        std::filesystem::path ks = workDir / "meme_debug.keystore";
        if (!std::filesystem::exists(ks)) {
            std::string ktCmd = "keytool -genkeypair -noprompt -keystore \"" + ks.string() +
                                "\" -storepass android -keypass android -alias androiddebugkey "
                                "-dname \"CN=MEME-UI Debug,O=MEMElang,C=US\" -keyalg RSA -keysize 2048 -validity 10000";
            if (detail::execCmd(ktCmd, out) != 0) {
                res.error = "keytool keystore generation failed:\n" + out;
                return res;
            }
        }

        std::filesystem::path finalApk = outBase / (pkgName + ".apk");
        if (detail::execCmd("\"" + apksigner.string() + "\" sign --ks \"" + ks.string() +
                            "\" --ks-pass pass:android --key-pass pass:android --out \"" +
                            finalApk.string() + "\" \"" + alignedApk.string() + "\"", out) != 0) {
            res.error = "apksigner failed:\n" + out;
            return res;
        }
        return finishOk(finalApk.string(), {finalApk.string()});
    }

    if (target == "ipa" || target == "ios") {
        std::filesystem::path stage = workDir / "ipa_stage";
        std::filesystem::remove_all(stage);
        std::filesystem::path devApp = stage / "Payload" / (appName + ".app");
        std::filesystem::create_directories(devApp);
        std::filesystem::path hostMm = repoRoot / "src" / "hosts" / "ios" / "MemeIosHost.mm";

        auto writeIosIcons = [&](const std::filesystem::path &appDir) {
            detail::writeFileBytes(appDir / "AppIcon60x60@2x.png", encodePngRgba(resizeBilinearRgba(baseIcon, 120, 120)));
            detail::writeFileBytes(appDir / "AppIcon60x60@3x.png", encodePngRgba(resizeBilinearRgba(baseIcon, 180, 180)));
            detail::writeFileBytes(appDir / "AppIcon76x76@2x~ipad.png", encodePngRgba(resizeBilinearRgba(baseIcon, 152, 152)));
            detail::writeFileBytes(appDir / "AppIcon1024x1024.png", encodePngRgba(resizeBilinearRgba(baseIcon, 1024, 1024)));
        };

        auto writeIosPlist = [&](const std::filesystem::path &plistPath, const std::string &plat) {
            std::string urlTypesXml;
            if (!schemeClean.empty()) {
                urlTypesXml =
                    "  <key>CFBundleURLTypes</key>\n"
                    "  <array><dict>\n"
                    "    <key>CFBundleURLName</key><string>" + detail::xmlEscape(cfg.bundleId) + "</string>\n"
                    "    <key>CFBundleURLSchemes</key><array><string>" + detail::xmlEscape(schemeClean) + "</string></array>\n"
                    "  </dict></array>\n";
            }
            std::string iosUsageXml;
            std::unordered_set<std::string> seenIosKeys;
            for (const auto &cap : declaredCaps) {
                auto kv = MemePermissions::appleUsagePlistKeyFor(cap, displayTitle);
                if (!kv.first.empty() && seenIosKeys.insert(kv.first).second) {
                    std::string reason = cfg.permissions.reasonFor("ios", cap, displayTitle);
                    if (reason.empty()) reason = kv.second;
                    iosUsageXml += "  <key>" + kv.first + "</key><string>" + detail::xmlEscape(reason) + "</string>\n";
                }
            }
            if (hasDeclaredCap("locationAlways")) {
                iosUsageXml += "  <key>UIBackgroundModes</key><array><string>location</string></array>\n";
            }
            std::string pl = R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
  <key>CFBundleDevelopmentRegion</key><string>en</string>
  <key>CFBundleExecutable</key><string>)" + detail::xmlEscape(appName) + R"(</string>
  <key>CFBundleDisplayName</key><string>)" + detail::xmlEscape(displayTitle) + R"(</string>
  <key>CFBundleIdentifier</key><string>)" + detail::xmlEscape(cfg.bundleId) + R"(</string>
  <key>CFBundleInfoDictionaryVersion</key><string>6.0</string>
  <key>CFBundleName</key><string>)" + detail::xmlEscape(displayTitle) + R"(</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>CFBundleShortVersionString</key><string>)" + detail::xmlEscape(cfg.version) + R"(</string>
  <key>CFBundleVersion</key><string>1</string>
  <key>CFBundleIcons</key>
  <dict><key>CFBundlePrimaryIcon</key><dict>
    <key>CFBundleIconFiles</key><array><string>AppIcon60x60</string></array>
    <key>UIPrerenderedIcon</key><true/>
  </dict></dict>
  <key>CFBundleIcons~ipad</key>
  <dict><key>CFBundlePrimaryIcon</key><dict>
    <key>CFBundleIconFiles</key><array><string>AppIcon60x60</string><string>AppIcon76x76</string></array>
    <key>UIPrerenderedIcon</key><true/>
  </dict></dict>
  <key>CFBundleSupportedPlatforms</key><array><string>)" + plat + R"(</string></array>
  <key>LSRequiresIPhoneOS</key><true/>
  <key>MinimumOSVersion</key><string>15.0</string>
  <key>UIDeviceFamily</key><array><integer>1</integer><integer>2</integer></array>
  <key>UIRequiredDeviceCapabilities</key><array><string>arm64</string></array>
  <key>UILaunchScreen</key><dict/>
)" + iosUsageXml + R"(  <key>UIApplicationSceneManifest</key>
  <dict>
    <key>UIApplicationSupportsMultipleScenes</key><false/>
    <key>UISceneConfigurations</key>
    <dict>
      <key>UIWindowSceneSessionRoleApplication</key>
      <array>
        <dict>
          <key>UISceneConfigurationName</key><string>Default Configuration</string>
          <key>UISceneDelegateClassName</key><string>MemeIosSceneDelegate</string>
        </dict>
      </array>
    </dict>
  </dict>
  <key>UIStatusBarStyle</key><string>UIStatusBarStyleLightContent</string>
  <key>UISupportedInterfaceOrientations</key><array>
    <string>UIInterfaceOrientationPortrait</string>
    <string>UIInterfaceOrientationLandscapeLeft</string>
    <string>UIInterfaceOrientationLandscapeRight</string>
  </array>
)" + urlTypesXml + R"(</dict></plist>)";
            detail::writeFileText(plistPath, pl);
        };

        std::filesystem::path devBin = devApp / appName;
        std::string cmdDev = "xcrun --sdk iphoneos clang++ -std=c++20 -O3 -DNDEBUG -DMEME_NO_DEPTH_GUARD "
                             "-arch arm64 -miphoneos-version-min=15.0 -fobjc-arc -DMEME_HOST_PROVIDES_MAIN "
                             "-I\"" + repoRoot.string() + "\" -x objective-c++ \"" + cppPath.string() + "\" "
                             "\"" + hostMm.string() + "\"" + evalFlags + " -framework UIKit -framework Metal -framework QuartzCore -framework Foundation -framework UserNotifications "
                             "-o \"" + devBin.string() + "\"";
        if (verbose) std::cout << "[package:ipa] " << cmdDev << "\n";
        std::string out;
        if (detail::execCmd(cmdDev, out) != 0) {
            res.error = "iOS device build failed:\n" + out;
            return res;
        }
        writeIosIcons(devApp);
        writeIosPlist(devApp / "Info.plist", "iPhoneOS");
        detail::execCmd("codesign --force --sign - \"" + devApp.string() + "\"", out);

        std::filesystem::path ipaPath = outBase / (appName + ".ipa");
        std::filesystem::remove(ipaPath);
        if (detail::execCmd("cd \"" + stage.string() + "\" && zip -qry \"" + std::filesystem::absolute(ipaPath).string() + "\" Payload", out) != 0) {
            res.error = "Failed to archive .ipa:\n" + out;
            return res;
        }

        // Also build iOS Simulator companion .app so xcrun simctl can boot/verify directly
        std::filesystem::path simApp = outBase / (appName + "-Simulator.app");
        std::filesystem::create_directories(simApp);
        std::filesystem::path simBin = simApp / appName;
        std::string simArch = "arm64";
#if defined(__x86_64__)
        simArch = "x86_64";
#endif
        std::string cmdSim = "xcrun --sdk iphonesimulator clang++ -std=c++20 -O3 -DNDEBUG -DMEME_NO_DEPTH_GUARD "
                             "-arch " + simArch + " -mios-simulator-version-min=15.0 -fobjc-arc -DMEME_HOST_PROVIDES_MAIN "
                             "-I\"" + repoRoot.string() + "\" -x objective-c++ \"" + cppPath.string() + "\" "
                             "\"" + hostMm.string() + "\"" + evalFlags + " -framework UIKit -framework Metal -framework QuartzCore -framework Foundation -framework UserNotifications "
                             "-o \"" + simBin.string() + "\"";
        if (detail::execCmd(cmdSim, out) == 0) {
            writeIosIcons(simApp);
            writeIosPlist(simApp / "Info.plist", "iPhoneSimulator");
            detail::execCmd("codesign --force --sign - \"" + simApp.string() + "\"", out);
        }
        return finishOk(ipaPath.string(), {ipaPath.string(), simApp.string()});
    }

    if (target == "web" || target == "wasm") {
        std::filesystem::path emppLocal = repoRoot / "tools" / "emsdk" / "upstream" / "emscripten" / "em++";
        std::string empp = detail::findInPathOrPaths("em++", {emppLocal.string()});
        if (empp.empty()) {
            res.error = "em++ (Emscripten) not found";
            return res;
        }
        std::filesystem::path webDir = outBase / "web";
        std::filesystem::create_directories(webDir);
        std::filesystem::path jsPath = webDir / (pkgName + ".js");
        std::filesystem::path htmlPath = webDir / "index.html";
        std::filesystem::path notFoundPath = webDir / "404.html";
        std::filesystem::path vercelPath = webDir / "vercel.json";
        std::filesystem::path redirectsPath = webDir / "_redirects";
        std::filesystem::path favPath = webDir / "favicon.ico";
        std::filesystem::path icon192Path = webDir / "icon-192.png";
        std::filesystem::path icon512Path = webDir / "icon-512.png";
        std::filesystem::path appleTouchPath = webDir / "apple-touch-icon.png";
        std::filesystem::path manifestWebPath = webDir / "manifest.webmanifest";

        std::string envPrefix;
        std::filesystem::path emConfig = repoRoot / "tools" / "emsdk" / ".emscripten";
        if (std::filesystem::exists(emConfig)) {
            envPrefix = "EM_CONFIG=\"" + emConfig.string() + "\" ";
        }

        std::string exportedFns = "'[\"_meme_web_start\",\"_meme_web_render\",\"_meme_web_get_mesh_vertices\","
                                  "\"_meme_web_get_mesh_vertex_count\",\"_meme_web_get_mesh_indices\","
                                  "\"_meme_web_get_mesh_index_count\",\"_meme_web_get_batches_json\","
                                  "\"_meme_web_get_atlas_rgba\",\"_meme_web_get_atlas_width\",\"_meme_web_get_atlas_height\",\"_meme_web_get_commands_json\","
                                  "\"_meme_web_get_hit_regions_json\",\"_meme_web_get_agent_snapshot\","
                                  "\"_meme_web_pointer_move\",\"_meme_web_pointer_down\",\"_meme_web_pointer_up\","
                                  "\"_meme_web_scroll\",\"_meme_web_key_event\",\"_meme_web_tick\","
                                  "\"_meme_web_set_clipboard\",\"_meme_web_get_clipboard\",\"_meme_web_agent_act\",\"_meme_web_set_route\","
                                  "\"_meme_native_complete_async\",\"_meme_native_complete_perm_async\",\"_malloc\",\"_free\"]'";
        std::string cmd = envPrefix + "\"" + empp + "\" -std=c++20 -O3 -fexceptions -DMEME_HOST_PROVIDES_MAIN "
                          "-I\"" + repoRoot.string() + "\" -s WASM=1 -s EXPORTED_FUNCTIONS=" + exportedFns + " "
                          "-s EXPORTED_RUNTIME_METHODS='[\"ccall\",\"cwrap\",\"UTF8ToString\",\"stringToUTF8\",\"HEAPU8\",\"HEAPU32\"]' "
                          "-s STACK_SIZE=4194304 -s INITIAL_MEMORY=33554432 -s ALLOW_MEMORY_GROWTH=1 \"" + cppPath.string() + "\"" + evalFlagsNoLang + " -o \"" + jsPath.string() + "\"";
        if (verbose) std::cout << "[package:web] " << cmd << "\n";
        std::string out;
        if (detail::execCmd(cmd, out) != 0) {
            res.error = "Wasm web build failed:\n" + out;
            return res;
        }

        detail::writeFileBytes(favPath, buildIcoFromRgba(baseIcon));
        detail::writeFileBytes(icon192Path, encodePngRgba(resizeBilinearRgba(baseIcon, 192, 192)));
        detail::writeFileBytes(icon512Path, encodePngRgba(resizeBilinearRgba(baseIcon, 512, 512)));
        detail::writeFileBytes(appleTouchPath, encodePngRgba(resizeBilinearRgba(baseIcon, 180, 180)));

        std::string webManifest = "{\n"
                                  "  \"name\": \"" + MemeNative::detail::jsonEscape(displayTitle) + "\",\n"
                                  "  \"short_name\": \"" + MemeNative::detail::jsonEscape(displayTitle) + "\",\n"
                                  "  \"description\": \"" + MemeNative::detail::jsonEscape(cfg.description) + "\",\n"
                                  "  \"start_url\": \".\",\n"
                                  "  \"display\": \"standalone\",\n"
                                  "  \"theme_color\": \"" + MemeNative::detail::jsonEscape(cfg.themeColor) + "\",\n"
                                  "  \"background_color\": \"" + MemeNative::detail::jsonEscape(cfg.bgColor) + "\",\n"
                                  "  \"icons\": [\n"
                                  "    { \"src\": \"icon-192.png\", \"sizes\": \"192x192\", \"type\": \"image/png\" },\n"
                                  "    { \"src\": \"icon-512.png\", \"sizes\": \"512x512\", \"type\": \"image/png\", \"purpose\": \"any maskable\" }\n"
                                  "  ]\n"
                                  "}\n";
        detail::writeFileText(manifestWebPath, webManifest);
        std::string permissionsPolicy;
        std::unordered_set<std::string> seenWebDirectives;
        for (const auto &cap : declaredCaps) {
            for (const auto &dir : MemePermissions::webPolicyDirectivesFor(cap)) {
                if (seenWebDirectives.insert(dir).second) {
                    if (!permissionsPolicy.empty()) permissionsPolicy += ", ";
                    permissionsPolicy += dir;
                }
            }
        }
        std::string htmlShell = MemeWebHost::generateZeroDomHtmlShell(pkgName + ".js", displayTitle, cfg.themeColor, true, permissionsPolicy);
        detail::writeFileText(htmlPath, htmlShell);
        detail::writeFileText(notFoundPath, htmlShell);

        std::string permHeaderEntry;
        if (!permissionsPolicy.empty()) {
            permHeaderEntry =
                "    {\n"
                "      \"source\": \"/(.*)\",\n"
                "      \"headers\": [\n"
                "        { \"key\": \"Permissions-Policy\", \"value\": \"" + MemeNative::detail::jsonEscape(permissionsPolicy) + "\" }\n"
                "      ]\n"
                "    },\n";
        }
        std::string vercelJson =
            "{\n"
            "  \"$schema\": \"https://openapi.vercel.sh/vercel.json\",\n"
            "  \"rewrites\": [\n"
            "    { \"source\": \"/(.*)\", \"destination\": \"/index.html\" }\n"
            "  ],\n"
            "  \"headers\": [\n" + permHeaderEntry +
            "    {\n"
            "      \"source\": \"/(.*)\\\\.wasm\",\n"
            "      \"headers\": [\n"
            "        { \"key\": \"Content-Type\", \"value\": \"application/wasm\" },\n"
            "        { \"key\": \"Cache-Control\", \"value\": \"public, max-age=31536000, immutable\" }\n"
            "      ]\n"
            "    }\n"
            "  ]\n"
            "}\n";
        detail::writeFileText(vercelPath, vercelJson);
        detail::writeFileText(redirectsPath, "/* /index.html 200\n");

        std::filesystem::path wasmPath = webDir / (pkgName + ".wasm");
        return finishOk(htmlPath.string(), {
            htmlPath.string(), jsPath.string(), wasmPath.string(), favPath.string(),
            icon192Path.string(), icon512Path.string(), manifestWebPath.string(),
            vercelPath.string(), redirectsPath.string(), notFoundPath.string()
        });
    }

    if (target == "all") {
        std::vector<std::string> targets = {"mac", "win", "deb", "rpm", "ipk", "apk", "ipa", "web"};
        std::vector<std::string> allArts;
        std::string errors;
        for (const auto &t : targets) {
            PackageResult sub = packageForTarget(sourceFile, t, outDirOrPath, verbose, cliConfig);
            if (!sub.success) {
                errors += "[" + t + "] " + sub.error + "\n";
            } else {
                allArts.insert(allArts.end(), sub.artifacts.begin(), sub.artifacts.end());
            }
        }
        if (!errors.empty()) {
            res.error = errors;
            res.artifacts = allArts;
            return res;
        }
        return finishOk(outBase.string(), allArts);
    }

    res.error = "Unknown package target '" + target + "'. Supported: mac, win, deb, rpm, ipk, apk, ipa, web, all";
    return res;
}

inline PackageResult packageForTarget(
    const std::string &sourceFile,
    const std::string &rawTarget,
    const std::string &outDirOrPath = "dist",
    bool verbose = false)
{
    return packageForTarget(sourceFile, rawTarget, outDirOrPath, verbose, PackageConfig{});
}

} // namespace MemePackager
