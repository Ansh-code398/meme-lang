#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace MemePlatform {

inline std::string toLowerTrimmed(const std::string &raw) {
    size_t start = 0;
    while (start < raw.size() && std::isspace(static_cast<unsigned char>(raw[start]))) start++;
    size_t end = raw.size();
    while (end > start && std::isspace(static_cast<unsigned char>(raw[end - 1]))) end--;
    std::string out;
    out.reserve(end - start);
    for (size_t i = start; i < end; ++i) {
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(raw[i]))));
    }
    return out;
}

inline std::string normalizePlatformName(const std::string &raw) {
    std::string s = toLowerTrimmed(raw);
    if (s == "ios" || s == "iphone" || s == "iphoneos" || s == "iphonesimulator" || s == "ipad" || s == "ipa") return "ios";
    if (s == "android" || s == "apk") return "android";
    if (s == "mac" || s == "macos" || s == "darwin" || s == "osx") return "mac";
    if (s == "win" || s == "windows" || s == "win32" || s == "mingw") return "windows";
    if (s == "linux" || s == "deb" || s == "rpm" || s == "ipk") return "linux";
    if (s == "web" || s == "wasm" || s == "browser" || s == "emscripten" || s == "html") return "web";
    return s;
}

inline std::string hostDefaultPlatform() {
#if defined(__EMSCRIPTEN__)
    return "web";
#elif defined(__APPLE__)
#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
    return "ios";
#else
    return "mac";
#endif
#elif defined(__ANDROID__)
    return "android";
#elif defined(_WIN32)
    return "windows";
#elif defined(__linux__)
    return "linux";
#else
    return "unknown";
#endif
}

inline std::string &runtimePlatformOverrideStorage() {
    static std::string s_override;
    return s_override;
}

inline void setRuntimePlatformOverride(const std::string &plat) {
    if (plat.empty()) {
        runtimePlatformOverrideStorage().clear();
        return;
    }
    std::string norm = normalizePlatformName(plat);
    if (norm == "ios" || norm == "android" || norm == "mac" || norm == "windows" || norm == "linux" || norm == "web") {
        runtimePlatformOverrideStorage() = norm;
    }
}

inline bool isKnownConcretePlatform(const std::string &norm) {
    return norm == "ios" || norm == "android" || norm == "mac" ||
           norm == "windows" || norm == "linux" || norm == "web";
}

inline std::string currentPlatform() {
    const std::string &ov = runtimePlatformOverrideStorage();
    if (!ov.empty()) return ov;
    if (const char *envPlat = std::getenv("MEME_TARGET")) {
        std::string norm = normalizePlatformName(envPlat);
        if (isKnownConcretePlatform(norm)) return norm;
    }
    if (const char *envPlat = std::getenv("MEME_PLATFORM")) {
        std::string norm = normalizePlatformName(envPlat);
        if (isKnownConcretePlatform(norm)) return norm;
    }
    return hostDefaultPlatform();
}

inline std::string currentArch() {
#if defined(__aarch64__) || defined(_M_ARM64)
    return "arm64";
#elif defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
#elif defined(__wasm64__)
    return "wasm64";
#elif defined(__wasm32__) || defined(__EMSCRIPTEN__)
    return "wasm32";
#elif defined(__arm__) || defined(_M_ARM)
    return "arm";
#else
    return "unknown";
#endif
}

inline bool isMobile(const std::string &plat = "") {
    std::string p = plat.empty() ? currentPlatform() : normalizePlatformName(plat);
    return p == "ios" || p == "android";
}

inline bool isDesktop(const std::string &plat = "") {
    std::string p = plat.empty() ? currentPlatform() : normalizePlatformName(plat);
    return p == "mac" || p == "windows" || p == "linux";
}

inline bool isWeb(const std::string &plat = "") {
    std::string p = plat.empty() ? currentPlatform() : normalizePlatformName(plat);
    return p == "web";
}

inline bool isApple(const std::string &plat = "") {
    std::string p = plat.empty() ? currentPlatform() : normalizePlatformName(plat);
    return p == "ios" || p == "mac";
}

inline bool isNative(const std::string &plat = "") {
    return !isWeb(plat);
}

inline bool isPosix(const std::string &plat = "") {
    std::string p = plat.empty() ? currentPlatform() : normalizePlatformName(plat);
    return p == "mac" || p == "linux" || p == "ios" || p == "android";
}

inline bool matchSingleToken(const std::string &tokRaw, const std::string &actualNorm) {
    std::string tok = toLowerTrimmed(tokRaw);
    if (tok.empty()) return false;
    bool negate = false;
    if (tok[0] == '!') {
        negate = true;
        tok = toLowerTrimmed(tok.substr(1));
    }
    bool hit = false;
    if (tok == "all" || tok == "*") {
        hit = true;
    } else if (tok == "mobile") {
        hit = isMobile(actualNorm);
    } else if (tok == "desktop") {
        hit = isDesktop(actualNorm);
    } else if (tok == "apple") {
        hit = isApple(actualNorm);
    } else if (tok == "native") {
        hit = isNative(actualNorm);
    } else if (tok == "posix" || tok == "unix") {
        hit = isPosix(actualNorm);
    } else {
        hit = (normalizePlatformName(tok) == actualNorm);
    }
    return negate ? !hit : hit;
}

inline bool matchPlatformSpec(const std::string &spec, const std::string &actual = "") {
    std::string actualNorm = actual.empty() ? currentPlatform() : normalizePlatformName(actual);
    std::string s = toLowerTrimmed(spec);
    if (s.empty()) return false;

    std::vector<std::string> tokens;
    std::string cur;
    for (char c : s) {
        if (c == ',' || c == '|' || std::isspace(static_cast<unsigned char>(c))) {
            if (!cur.empty()) {
                tokens.push_back(cur);
                cur.clear();
            }
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) tokens.push_back(cur);
    if (tokens.empty()) return false;

    // If all tokens are negated (e.g. "!ios !android"), require all to pass;
    // otherwise treat positive list as union ("ios,android").
    bool allNegated = true;
    for (const auto &t : tokens) {
        if (t.empty() || t[0] != '!') { allNegated = false; break; }
    }
    if (allNegated) {
        for (const auto &t : tokens) {
            if (!matchSingleToken(t, actualNorm)) return false;
        }
        return true;
    }
    for (const auto &t : tokens) {
        if (matchSingleToken(t, actualNorm)) return true;
    }
    return false;
}

inline std::vector<std::string> platformSuffixPriority(const std::string &rawTarget = "") {
    std::string p = rawTarget.empty() ? currentPlatform() : normalizePlatformName(rawTarget);
    if (p == "ios") return {".ios", ".mobile", ".apple", ".native"};
    if (p == "android") return {".android", ".mobile", ".native"};
    if (p == "mac") return {".mac", ".macos", ".desktop", ".apple", ".native"};
    if (p == "windows") return {".win", ".windows", ".desktop", ".native"};
    if (p == "linux") return {".linux", ".desktop", ".native"};
    if (p == "web") return {".web", ".wasm"};
    return {};
}

inline std::vector<std::string> platformSubdirPriority(const std::string &rawTarget = "") {
    std::string p = rawTarget.empty() ? currentPlatform() : normalizePlatformName(rawTarget);
    if (p == "ios") return {"ios", "mobile", "apple", "native"};
    if (p == "android") return {"android", "mobile", "native"};
    if (p == "mac") return {"mac", "macos", "desktop", "apple", "native"};
    if (p == "windows") return {"windows", "win", "desktop", "native"};
    if (p == "linux") return {"linux", "desktop", "native"};
    if (p == "web") return {"web", "wasm"};
    return {};
}

inline std::vector<std::string> platformSelectPriority(const std::string &rawTarget = "") {
    std::string p = rawTarget.empty() ? currentPlatform() : normalizePlatformName(rawTarget);
    if (p == "ios") return {"ios", "mobile", "apple", "native", "default"};
    if (p == "android") return {"android", "mobile", "native", "default"};
    if (p == "mac") return {"mac", "macos", "darwin", "desktop", "apple", "native", "default"};
    if (p == "windows") return {"windows", "win", "win32", "desktop", "native", "default"};
    if (p == "linux") return {"linux", "unix", "posix", "desktop", "native", "default"};
    if (p == "web") return {"web", "wasm", "browser", "default"};
    return {"default"};
}

inline bool isRegularFileSafe(const std::filesystem::path &p) {
    std::error_code ec;
    return std::filesystem::exists(p, ec) && std::filesystem::is_regular_file(p, ec);
}

inline std::filesystem::path probeCandidateBase(
    const std::filesystem::path &basePath,
    const std::string &targetPlatform)
{
    std::string raw = basePath.string();
    std::filesystem::path stemNoExt = basePath;
    if (basePath.extension() == ".meme") {
        stemNoExt = basePath.parent_path() / basePath.stem();
    }

    auto suffixes = platformSuffixPriority(targetPlatform);
    // 1. Sibling platform suffixed files: <stem>.<plat>.meme
    for (const auto &suf : suffixes) {
        std::filesystem::path cand(stemNoExt.string() + suf + ".meme");
        if (isRegularFileSafe(cand)) return cand;
    }

    // 2. Folder override: <parent>/platforms/<plat>/<filename>.meme
    if (stemNoExt.has_filename()) {
        std::filesystem::path parent = stemNoExt.parent_path();
        std::string fname = stemNoExt.filename().string();
        for (const auto &sub : platformSubdirPriority(targetPlatform)) {
            std::filesystem::path cand = parent / "platforms" / sub / (fname + ".meme");
            if (isRegularFileSafe(cand)) return cand;
        }
    }

    // 3. Exact or generic .meme fallback
    if (isRegularFileSafe(basePath)) return basePath;
    if (basePath.extension() != ".meme") {
        std::filesystem::path withExt(raw + ".meme");
        if (isRegularFileSafe(withExt)) return withExt;
    }

    // 4. Directory / <dirName>.<plat>.meme -> index.<plat>.meme -> index.meme -> main.meme
    std::error_code ec;
    if (std::filesystem::exists(basePath, ec) && std::filesystem::is_directory(basePath, ec)) {
        std::string dirStem = basePath.filename().string();
        if (!dirStem.empty()) {
            for (const auto &suf : suffixes) {
                std::filesystem::path cand = basePath / (dirStem + suf + ".meme");
                if (isRegularFileSafe(cand)) return cand;
            }
            if (isRegularFileSafe(basePath / (dirStem + ".meme"))) return basePath / (dirStem + ".meme");
        }
        for (const auto &suf : suffixes) {
            std::filesystem::path cand = basePath / ("index" + suf + ".meme");
            if (isRegularFileSafe(cand)) return cand;
        }
        if (isRegularFileSafe(basePath / "index.meme")) return basePath / "index.meme";
        if (isRegularFileSafe(basePath / "main.meme")) return basePath / "main.meme";
    }
    return {};
}

} // namespace MemePlatform
