#pragma once

#include "../lexer/Lexer.h"
#include "../parser/Parser.h"
#include "../interpreter/Interpreter.h"
#include "../ui/MemeIconsRegistry.h"
#include "../compiler/WebDomTranspiler.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace MemeSsr {

struct SsrHttpRequest {
    std::string method = "GET";
    std::string rawUrl = "/";
    std::string path = "/";
    std::string queryString;
    std::unordered_map<std::string, std::string> query;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

struct SsrHttpResponse {
    int status = 200;
    std::string statusText = "OK";
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

struct AppRouteEntry {
    std::string pattern;           // e.g. "/blog/[slug]"
    std::filesystem::path file;    // page.meme or route.meme
    bool isApiRoute = false;
    std::vector<std::filesystem::path> layouts; // outer -> inner layout.meme files
    struct Seg {
        enum Kind { STATIC, PARAM, CATCH_ALL } kind = STATIC;
        std::string text;
    };
    std::vector<Seg> segments;
    int staticScore = 0;
};

namespace detail {

class SsrCallable : public Callable {
public:
    using Fn = std::function<Value(Interpreter &, const std::vector<Value> &)>;
    std::string fnName;
    size_t minA = 0;
    size_t maxA = 0;
    Fn impl;

    SsrCallable(std::string n, size_t mn, size_t mx, Fn f)
        : fnName(std::move(n)), minA(mn), maxA(mx), impl(std::move(f)) {}

    size_t arity() const override { return minA; }
    size_t minArity() const override { return minA; }
    size_t maxArity() const override { return maxA; }
    bool checkArity(size_t count) const override { return count >= minA && count <= maxA; }
    Value call(Interpreter &it, const std::vector<Value> &args) override { return impl(it, args); }
    std::string toString() const override { return "<ssr fn " + fnName + ">"; }
};

inline Value makeSsrFn(const std::string &name, size_t minA, size_t maxA, SsrCallable::Fn fn) {
    return Value(std::static_pointer_cast<Callable>(std::make_shared<SsrCallable>(name, minA, maxA, std::move(fn))));
}

inline std::string htmlEscape(const std::string &s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        if (c == '&') out += "&amp;";
        else if (c == '<') out += "&lt;";
        else if (c == '>') out += "&gt;";
        else if (c == '"') out += "&quot;";
        else if (c == '\'') out += "&#39;";
        else out.push_back(c);
    }
    return out;
}

inline std::string jsonEscape(const std::string &s) {
    std::string out = "\"";
    for (unsigned char c : s) {
        if (c == '\\') out += "\\\\";
        else if (c == '"') out += "\\\"";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (c == '<') out += "\\u003c";
        else if (c == '>') out += "\\u003e";
        else if (c == '&') out += "\\u0026";
        else out.push_back(static_cast<char>(c));
    }
    out.push_back('"');
    return out;
}

inline std::string urlDecode(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '+' ) {
            out.push_back(' ');
        } else if (s[i] == '%' && i + 2 < s.size()) {
            auto hexVal = [](char c) -> int {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                return -1;
            };
            int hi = hexVal(s[i + 1]);
            int lo = hexVal(s[i + 2]);
            if (hi >= 0 && lo >= 0) {
                out.push_back(static_cast<char>((hi << 4) | lo));
                i += 2;
                continue;
            }
            out.push_back(s[i]);
        } else {
            out.push_back(s[i]);
        }
    }
    return out;
}

inline std::unordered_map<std::string, std::string> parseQueryParams(const std::string &qs) {
    std::unordered_map<std::string, std::string> out;
    size_t start = 0;
    while (start < qs.size()) {
        size_t amp = qs.find('&', start);
        std::string pair = (amp == std::string::npos) ? qs.substr(start) : qs.substr(start, amp - start);
        size_t eq = pair.find('=');
        if (eq != std::string::npos) {
            out[urlDecode(pair.substr(0, eq))] = urlDecode(pair.substr(eq + 1));
        } else if (!pair.empty()) {
            out[urlDecode(pair)] = "";
        }
        if (amp == std::string::npos) break;
        start = amp + 1;
    }
    return out;
}

inline std::string valueToJson(const Value &v, int depth = 0) {
    if (depth >= 12) return "null";
    Value d = v.deref();
    if (d.isNil()) return "null";
    if (std::holds_alternative<bool>(d.data)) return std::get<bool>(d.data) ? "true" : "false";
    if (std::holds_alternative<long long>(d.data)) return std::to_string(std::get<long long>(d.data));
    if (std::holds_alternative<double>(d.data)) {
        std::ostringstream os;
        os << std::get<double>(d.data);
        return os.str();
    }
    if (d.isNumeric()) {
        std::ostringstream os;
        os << d.asDouble();
        return os.str();
    }
    if (d.isString()) return jsonEscape(d.toString());
    if (d.isList()) {
        std::string out = "[";
        auto l = d.getList();
        for (size_t i = 0; i < l->elements.size(); ++i) {
            if (i > 0) out += ",";
            out += valueToJson(l->elements[i], depth + 1);
        }
        out += "]";
        return out;
    }
    if (d.isMap()) {
        std::string out = "{";
        auto m = d.getMap();
        bool first = true;
        for (const auto &k : m->keyOrder) {
            auto it = m->entries.find(k);
            if (it == m->entries.end()) continue;
            if (!first) out += ",";
            first = false;
            out += jsonEscape(k) + ":" + valueToJson(it->second, depth + 1);
        }
        out += "}";
        return out;
    }
    return jsonEscape(d.toString());
}

inline std::vector<std::string> splitUrlSegments(const std::string &path) {
    std::vector<std::string> segs;
    size_t i = 0;
    while (i < path.size()) {
        while (i < path.size() && path[i] == '/') ++i;
        if (i >= path.size()) break;
        size_t j = path.find('/', i);
        std::string part = (j == std::string::npos) ? path.substr(i) : path.substr(i, j - i);
        if (!part.empty()) segs.push_back(part);
        if (j == std::string::npos) break;
        i = j + 1;
    }
    return segs;
}

inline std::string httpStatusReason(int code) {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 307: return "Temporary Redirect";
        case 308: return "Permanent Redirect";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 500: return "Internal Server Error";
        default: return "OK";
    }
}

inline bool matchMiddlewareGlob(const std::string &pat, const std::string &path) {
    if (pat.empty() || pat == "*" || pat == "/*") return true;
    if (pat.back() == '*') {
        std::string prefix = pat.substr(0, pat.size() - 1);
        return path.rfind(prefix, 0) == 0;
    }
    return pat == path || path.rfind(pat + "/", 0) == 0;
}

inline std::string renderVdomToHtml(const Value &nodeVal, MemeWebDom::detail::MemeCssGenerator &cssGen, int depth = 0) {
    if (depth > 64) return "";
    Value d = nodeVal.deref();
    if (d.isNil() || std::holds_alternative<bool>(d.data)) return "";
    if (d.isList()) {
        std::string out;
        for (const auto &ch : d.getList()->elements) out += renderVdomToHtml(ch, cssGen, depth + 1);
        return out;
    }
    if (!d.isMap()) return htmlEscape(d.toString());

    auto map = d.getMap();
    std::string type = map->has("type") ? map->entries["type"].deref().toString() : "";
    std::string tag = map->has("tag") ? map->entries["tag"].deref().toString() : "";
    if (type != "jsx_element" && tag.empty()) return "";

    if (tag == "fragment" || tag == "__frag__" || tag == "Fragment") {
        std::string out;
        if (map->has("children")) out += renderVdomToHtml(map->entries["children"], cssGen, depth + 1);
        return out;
    }
    if (tag.empty()) tag = "div";

    std::shared_ptr<MemeMap> pm = (map->has("props") && map->entries["props"].deref().isMap())
        ? map->entries["props"].deref().getMap()
        : nullptr;

    std::string cls = map->has("class") ? map->entries["class"].deref().toString() : "";
    if (cls.empty() && pm) {
        if (pm->has("class")) cls = pm->entries["class"].deref().toString();
        else if (pm->has("className")) cls = pm->entries["className"].deref().toString();
    }
    if (!cls.empty()) cssGen.addClassString(cls);

    if (tag == "icon") {
        std::string iconName = (pm && pm->has("name")) ? pm->entries["name"].deref().toString() : "sparkles";
        std::string sz = (pm && pm->has("size")) ? pm->entries["size"].deref().toString() : "16";
        std::string strokeW = (pm && pm->has("strokeWidth")) ? pm->entries["strokeWidth"].deref().toString() : "2";
        std::string color = (pm && pm->has("color")) ? pm->entries["color"].deref().toString() : "currentColor";
        std::string pathData = MemeUI::Icons::lookupIconSvgPath(iconName);
        if (pathData.empty()) pathData = MemeUI::Icons::lookupIconSvgPath("sparkles");
        std::ostringstream svg;
        svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << htmlEscape(sz)
            << "\" height=\"" << htmlEscape(sz) << "\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\""
            << htmlEscape(color) << "\" stroke-width=\"" << htmlEscape(strokeW)
            << "\" stroke-linecap=\"round\" stroke-linejoin=\"round\"";
        if (!cls.empty()) svg << " class=\"" << htmlEscape(cls) << "\"";
        svg << ">";
        size_t pos = 0;
        while (pos < pathData.size()) {
            size_t nextM = pathData.find(" M", pos + 1);
            std::string sub = (nextM == std::string::npos) ? pathData.substr(pos) : pathData.substr(pos, nextM - pos);
            if (!sub.empty() && sub.front() == ' ') sub.erase(sub.begin());
            if (!sub.empty()) svg << "<path d=\"" << htmlEscape(sub) << "\" />";
            if (nextM == std::string::npos) break;
            pos = nextM + 1;
        }
        svg << "</svg>";
        return svg.str();
    }

    std::string htmlTag = (tag == "scrollarea") ? "div" : tag;
    std::ostringstream out;
    out << "<" << htmlTag;
    if (!cls.empty()) out << " class=\"" << htmlEscape(cls) << "\"";
    if (pm) {
        for (const auto &k : pm->keyOrder) {
            if (k == "class" || k == "className" || k == "children" || k == "initial" || k == "animate" ||
                k == "transition" || k == "initialOpacity" || k == "animateOpacity" ||
                k == "initialY" || k == "animateY" || k == "onPaint" || k.rfind("on", 0) == 0) {
                continue;
            }
            if (k == "testId") {
                out << " data-meme-testid=\"" << htmlEscape(pm->entries[k].deref().toString()) << "\"";
                continue;
            }
            Value pv = pm->entries[k].deref();
            if (std::holds_alternative<bool>(pv.data)) {
                if (std::get<bool>(pv.data)) out << " " << k;
            } else if (!pv.isNil() && !pv.isMap() && !pv.isList() && !pv.isCallable()) {
                out << " " << k << "=\"" << htmlEscape(pv.toString()) << "\"";
            }
        }
    }

    if (htmlTag == "input" || htmlTag == "img" || htmlTag == "hr" || htmlTag == "br") {
        out << " />";
        return out.str();
    }
    out << ">";
    if (map->has("children")) {
        out << renderVdomToHtml(map->entries["children"], cssGen, depth + 1);
    }
    out << "</" << htmlTag << ">";
    return out.str();
}

} // namespace detail

class MemeSsrEngine {
public:
    std::filesystem::path projectRoot;
    std::filesystem::path appDir;
    std::filesystem::path middlewareFile;
    std::vector<AppRouteEntry> routes;
    std::filesystem::path singleEntryFallback;

    bool discover(const std::filesystem::path &targetPath, std::string &errOut) {
        std::error_code ec;
        if (!std::filesystem::exists(targetPath, ec)) {
            errOut = "SSR target not found: " + targetPath.string();
            return false;
        }
        routes.clear();
        middlewareFile.clear();
        singleEntryFallback.clear();

        if (std::filesystem::is_regular_file(targetPath, ec)) {
            singleEntryFallback = std::filesystem::weakly_canonical(targetPath, ec);
            projectRoot = singleEntryFallback.parent_path();
            if (projectRoot.filename() == "src" && projectRoot.has_parent_path()) {
                projectRoot = projectRoot.parent_path();
            }
        } else {
            projectRoot = std::filesystem::weakly_canonical(targetPath, ec);
        }

        if (std::filesystem::is_directory(projectRoot / "app", ec)) appDir = projectRoot / "app";
        else if (std::filesystem::is_directory(projectRoot / "src" / "app", ec)) appDir = projectRoot / "src" / "app";
        else if (std::filesystem::is_directory(targetPath, ec) &&
                 (std::filesystem::exists(targetPath / "page.meme", ec) || std::filesystem::exists(targetPath / "layout.meme", ec))) {
            appDir = projectRoot;
        }

        if (std::filesystem::is_regular_file(projectRoot / "middleware.meme", ec)) middlewareFile = projectRoot / "middleware.meme";
        else if (std::filesystem::is_regular_file(projectRoot / "src" / "middleware.meme", ec)) middlewareFile = projectRoot / "src" / "middleware.meme";

        if (!appDir.empty()) {
            std::vector<std::filesystem::path> layoutStack;
            scanAppDir(appDir, "", layoutStack);
            std::sort(routes.begin(), routes.end(), [](const AppRouteEntry &a, const AppRouteEntry &b) {
                if (a.staticScore != b.staticScore) return a.staticScore > b.staticScore;
                if (a.segments.size() != b.segments.size()) return a.segments.size() > b.segments.size();
                return a.pattern < b.pattern;
            });
        }
        if (routes.empty() && singleEntryFallback.empty()) {
            for (const char *cand : {"src/App.meme", "App.meme", "main.meme"}) {
                if (std::filesystem::is_regular_file(projectRoot / cand, ec)) {
                    singleEntryFallback = projectRoot / cand;
                    break;
                }
            }
        }
        return !routes.empty() || !singleEntryFallback.empty();
    }

    SsrHttpResponse handleRequest(const SsrHttpRequest &rawReq) const {
        SsrHttpRequest req = rawReq;
        if (req.path.empty()) req.path = "/";
        size_t qPos = req.path.find('?');
        if (qPos != std::string::npos) {
            req.queryString = req.path.substr(qPos + 1);
            req.path = req.path.substr(0, qPos);
            req.query = detail::parseQueryParams(req.queryString);
        } else if (!req.queryString.empty() && req.query.empty()) {
            req.query = detail::parseQueryParams(req.queryString);
        }

        SsrHttpResponse resp;
        resp.headers["x-powered-by"] = "MEMElang SSR";

        // 1. Middleware Pipeline (middleware.meme)
        if (!middlewareFile.empty()) {
            bool shortCircuit = false;
            SsrHttpResponse mwResp = executeMiddleware(req, resp.headers, shortCircuit);
            if (shortCircuit) return mwResp;
        }

        // 2. App Router Matching (/page.meme & /api/.../route.meme)
        std::unordered_map<std::string, std::string> matchedParams;
        const AppRouteEntry *route = findMatchingRoute(req.path, matchedParams);
        if (route) {
            if (route->isApiRoute) return executeApiRoute(*route, req, matchedParams, resp.headers);
            return executePageSsr(*route, req, matchedParams, resp.headers);
        }

        // 3. Static hydration / SPA fallback if singleEntryFallback is configured
        if (!singleEntryFallback.empty()) {
            if (req.path == "/styles.css" || req.path == "/meme-dom.js" || req.path == "/app.js") {
                return serveSingleEntryAsset(req.path, resp.headers);
            }
            return executeSingleEntrySsr(req, resp.headers);
        }

        resp.status = 404;
        resp.statusText = "Not Found";
        resp.headers["Content-Type"] = "text/html; charset=utf-8";
        resp.body = "<!doctype html><html><body style=\"background:#09090b;color:#f4f4f5;font-family:sans-serif;padding:2rem\">"
                    "<h1>404 &mdash; Route Not Found</h1><p>" + detail::htmlEscape(req.path) + "</p></body></html>";
        return resp;
    }

    int serveHttp(int port, const std::string &host = "127.0.0.1", int maxRequests = 0) const {
#if defined(_WIN32) || defined(__EMSCRIPTEN__)
        (void)port; (void)host; (void)maxRequests;
        std::cerr << "POSIX socket server unavailable on this platform build\n";
        return 1;
#else
        int sfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sfd < 0) return 1;
        int opt = 1;
        setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#ifdef SO_REUSEPORT
        setsockopt(sfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
#endif
        sockaddr_in sin{};
        sin.sin_family = AF_INET;
        sin.sin_port = htons(static_cast<uint16_t>(port));
        if (host.empty() || host == "0.0.0.0") sin.sin_addr.s_addr = INADDR_ANY;
        else inet_pton(AF_INET, host.c_str(), &sin.sin_addr);

        if (bind(sfd, reinterpret_cast<sockaddr *>(&sin), sizeof(sin)) != 0) {
            close(sfd);
            return 1;
        }
        socklen_t slen = sizeof(sin);
        getsockname(sfd, reinterpret_cast<sockaddr *>(&sin), &slen);
        int actualPort = ntohs(sin.sin_port);
        ::listen(sfd, 128);

        std::cout << "[meme serve] SSR + Middleware Server running at http://" << host << ":" << actualPort
                  << " (" << routes.size() << " App routes"
                  << (!middlewareFile.empty() ? ", middleware active" : "") << ")\n" << std::flush;

        int handled = 0;
        while (maxRequests <= 0 || handled < maxRequests) {
            sockaddr_in csin{};
            socklen_t cslen = sizeof(csin);
            int cfd = accept(sfd, reinterpret_cast<sockaddr *>(&csin), &cslen);
            if (cfd < 0) break;

            std::string raw;
            char buf[8192];
            size_t hend = std::string::npos;
            while ((hend = raw.find("\r\n\r\n")) == std::string::npos) {
                ssize_t n = recv(cfd, buf, sizeof(buf), 0);
                if (n <= 0) break;
                raw.append(buf, static_cast<size_t>(n));
                if (raw.size() > 1024 * 1024) break;
            }
            if (hend != std::string::npos) {
                SsrHttpRequest req = parseRawHttp(raw, hend, cfd);
                SsrHttpResponse resp = handleRequest(req);
                std::string wire = formatWireHttp(resp);
                size_t sent = 0;
                while (sent < wire.size()) {
                    ssize_t w = send(cfd, wire.data() + sent, wire.size() - sent, 0);
                    if (w <= 0) break;
                    sent += static_cast<size_t>(w);
                }
            }
            close(cfd);
            handled++;
        }
        close(sfd);
        return 0;
#endif
    }

private:
    void scanAppDir(const std::filesystem::path &dir, const std::string &urlPrefix,
                    std::vector<std::filesystem::path> &layoutStack) {
        std::error_code ec;
        std::filesystem::path layoutFile = dir / "layout.meme";
        bool pushedLayout = false;
        if (std::filesystem::is_regular_file(layoutFile, ec)) {
            layoutStack.push_back(layoutFile);
            pushedLayout = true;
        }

        auto addEntry = [&](const std::filesystem::path &file, bool isApi) {
            AppRouteEntry e;
            e.pattern = urlPrefix.empty() ? "/" : urlPrefix;
            e.file = file;
            e.isApiRoute = isApi;
            e.layouts = layoutStack;
            auto parts = detail::splitUrlSegments(e.pattern);
            for (const auto &p : parts) {
                AppRouteEntry::Seg seg;
                if (p.size() >= 6 && p.rfind("[...", 0) == 0 && p.back() == ']') {
                    seg.kind = AppRouteEntry::Seg::CATCH_ALL;
                    seg.text = p.substr(4, p.size() - 5);
                    e.staticScore += 1;
                } else if (p.size() >= 3 && p.front() == '[' && p.back() == ']') {
                    seg.kind = AppRouteEntry::Seg::PARAM;
                    seg.text = p.substr(1, p.size() - 2);
                    e.staticScore += 10;
                } else {
                    seg.kind = AppRouteEntry::Seg::STATIC;
                    seg.text = p;
                    e.staticScore += 100;
                }
                e.segments.push_back(std::move(seg));
            }
            routes.push_back(std::move(e));
        };

        if (std::filesystem::is_regular_file(dir / "page.meme", ec)) addEntry(dir / "page.meme", false);
        if (std::filesystem::is_regular_file(dir / "route.meme", ec)) addEntry(dir / "route.meme", true);

        std::vector<std::filesystem::path> subdirs;
        for (const auto &ent : std::filesystem::directory_iterator(dir, ec)) {
            if (ent.is_directory(ec)) subdirs.push_back(ent.path());
        }
        std::sort(subdirs.begin(), subdirs.end());
        for (const auto &sd : subdirs) {
            std::string segName = sd.filename().string();
            if (segName.empty() || segName.front() == '_' || segName.front() == '.') continue;
            // Route group folder like (marketing) omits segment from URL while still inheriting layouts
            std::string nextPrefix = (segName.front() == '(' && segName.back() == ')')
                ? urlPrefix
                : (urlPrefix + "/" + segName);
            scanAppDir(sd, nextPrefix, layoutStack);
        }

        if (pushedLayout) layoutStack.pop_back();
    }

    const AppRouteEntry *findMatchingRoute(const std::string &path, std::unordered_map<std::string, std::string> &outParams) const {
        auto urlSegs = detail::splitUrlSegments(path);
        for (const auto &r : routes) {
            std::unordered_map<std::string, std::string> captured;
            size_t ui = 0;
            bool ok = true;
            bool hasCatchAll = false;
            for (size_t ri = 0; ri < r.segments.size(); ++ri) {
                const auto &rs = r.segments[ri];
                if (rs.kind == AppRouteEntry::Seg::CATCH_ALL) {
                    hasCatchAll = true;
                    std::string rest;
                    while (ui < urlSegs.size()) {
                        if (!rest.empty()) rest += "/";
                        rest += detail::urlDecode(urlSegs[ui++]);
                    }
                    captured[rs.text] = rest;
                    break;
                }
                if (ui >= urlSegs.size()) { ok = false; break; }
                std::string uSeg = detail::urlDecode(urlSegs[ui++]);
                if (rs.kind == AppRouteEntry::Seg::PARAM) {
                    captured[rs.text] = uSeg;
                } else if (uSeg != rs.text) {
                    ok = false; break;
                }
            }
            if (ok && (hasCatchAll || ui == urlSegs.size())) {
                outParams = std::move(captured);
                return &r;
            }
        }
        return nullptr;
    }

    static Value buildReqValue(const SsrHttpRequest &req, const std::unordered_map<std::string, std::string> &params) {
        auto m = std::make_shared<MemeMap>();
        m->set("method", Value(req.method));
        m->set("path", Value(req.path));
        m->set("pathname", Value(req.path));
        m->set("url", Value(req.rawUrl));
        m->set("body", Value(req.body));

        auto qm = std::make_shared<MemeMap>();
        for (const auto &kv : req.query) qm->set(kv.first, Value(kv.second));
        m->set("query", Value(qm));

        auto pm = std::make_shared<MemeMap>();
        for (const auto &kv : params) pm->set(kv.first, Value(kv.second));
        m->set("params", Value(pm));

        auto hm = std::make_shared<MemeMap>();
        for (const auto &kv : req.headers) hm->set(kv.first, Value(kv.second));
        m->set("headers", Value(hm));
        return Value(m);
    }

    SsrHttpResponse executeMiddleware(SsrHttpRequest &req,
                                      std::unordered_map<std::string, std::string> &resHeaders,
                                      bool &outShortCircuit) const {
        outShortCircuit = false;
        SsrHttpResponse mwResp;
        Interpreter interp;
        interp.setRouterPath(req.path);
        interp.setCurrentFilePath(middlewareFile.string());

        Value modVal = interp.importModule(middlewareFile.string(), 1);
        if (!modVal.isMap()) return mwResp;
        auto modMap = modVal.getMap();

        // Check optional matcher config
        if (modMap->has("config") && modMap->entries["config"].deref().isMap()) {
            auto cfgMap = modMap->entries["config"].deref().getMap();
            if (cfgMap->has("matcher") && cfgMap->entries["matcher"].deref().isList()) {
                bool matched = false;
                for (const auto &pat : cfgMap->entries["matcher"].deref().getList()->elements) {
                    if (detail::matchMiddlewareGlob(pat.deref().toString(), req.path)) {
                        matched = true; break;
                    }
                }
                if (!matched) return mwResp;
            }
        }

        Value mwFn(std::monostate{});
        for (const char *k : {"middleware", "default", "handle"}) {
            if (modMap->has(k) && modMap->entries[k].deref().isCallable()) {
                mwFn = modMap->entries[k].deref();
                break;
            }
        }
        if (!mwFn.isCallable()) return mwResp;

        auto sharedHeaders = std::make_shared<std::unordered_map<std::string, std::string>>(resHeaders);
        auto ctxMap = std::make_shared<MemeMap>();
        ctxMap->set("req", buildReqValue(req, {}));
        ctxMap->set("next", detail::makeSsrFn("next", 0, 0, [](Interpreter &, const std::vector<Value> &) -> Value {
            auto rm = std::make_shared<MemeMap>();
            rm->set("__mwAction", Value("next"));
            return Value(rm);
        }));
        ctxMap->set("redirect", detail::makeSsrFn("redirect", 1, 2, [](Interpreter &, const std::vector<Value> &a) -> Value {
            auto rm = std::make_shared<MemeMap>();
            rm->set("__mwAction", Value("redirect"));
            rm->set("location", Value(a[0].deref().toString()));
            rm->set("status", Value(a.size() > 1 ? a[1].deref().asInt() : 302LL));
            return Value(rm);
        }));
        ctxMap->set("rewrite", detail::makeSsrFn("rewrite", 1, 1, [](Interpreter &, const std::vector<Value> &a) -> Value {
            auto rm = std::make_shared<MemeMap>();
            rm->set("__mwAction", Value("rewrite"));
            rm->set("target", Value(a[0].deref().toString()));
            return Value(rm);
        }));
        ctxMap->set("json", detail::makeSsrFn("json", 1, 2, [](Interpreter &, const std::vector<Value> &a) -> Value {
            auto rm = std::make_shared<MemeMap>();
            rm->set("__mwAction", Value("json"));
            rm->set("body", Value(detail::valueToJson(a[0])));
            rm->set("status", Value(a.size() > 1 ? a[1].deref().asInt() : 200LL));
            return Value(rm);
        }));
        ctxMap->set("text", detail::makeSsrFn("text", 1, 2, [](Interpreter &, const std::vector<Value> &a) -> Value {
            auto rm = std::make_shared<MemeMap>();
            rm->set("__mwAction", Value("text"));
            rm->set("body", Value(a[0].deref().toString()));
            rm->set("status", Value(a.size() > 1 ? a[1].deref().asInt() : 200LL));
            return Value(rm);
        }));
        ctxMap->set("setHeader", detail::makeSsrFn("setHeader", 2, 2, [sharedHeaders](Interpreter &, const std::vector<Value> &a) -> Value {
            (*sharedHeaders)[a[0].deref().toString()] = a[1].deref().toString();
            return Value(true);
        }));

        Value ret = mwFn.getCallable()->call(interp, {Value(ctxMap)}).deref();
        resHeaders = *sharedHeaders;

        if (ret.isMap()) {
            auto rm = ret.getMap();
            std::string act = rm->has("__mwAction") ? rm->entries["__mwAction"].deref().toString() : "";
            if (act == "redirect") {
                outShortCircuit = true;
                int code = rm->has("status") ? static_cast<int>(rm->entries["status"].deref().asInt()) : 302;
                std::string loc = rm->has("location") ? rm->entries["location"].deref().toString() : "/";
                mwResp.status = code;
                mwResp.statusText = detail::httpStatusReason(code);
                mwResp.headers = resHeaders;
                mwResp.headers["Location"] = loc;
                mwResp.headers["Content-Type"] = "text/plain; charset=utf-8";
                mwResp.body = "Redirecting to " + loc;
                return mwResp;
            }
            if (act == "rewrite") {
                std::string tgt = rm->has("target") ? rm->entries["target"].deref().toString() : req.path;
                size_t qPos = tgt.find('?');
                if (qPos != std::string::npos) {
                    req.path = tgt.substr(0, qPos);
                    req.queryString = tgt.substr(qPos + 1);
                    req.query = detail::parseQueryParams(req.queryString);
                } else {
                    req.path = tgt;
                }
                return mwResp;
            }
            if (act == "json" || act == "text") {
                outShortCircuit = true;
                int code = rm->has("status") ? static_cast<int>(rm->entries["status"].deref().asInt()) : 200;
                mwResp.status = code;
                mwResp.statusText = detail::httpStatusReason(code);
                mwResp.headers = resHeaders;
                mwResp.headers["Content-Type"] = (act == "json") ? "application/json; charset=utf-8" : "text/plain; charset=utf-8";
                mwResp.body = rm->has("body") ? rm->entries["body"].deref().toString() : "";
                return mwResp;
            }
        }
        return mwResp;
    }

    SsrHttpResponse executeApiRoute(const AppRouteEntry &route, const SsrHttpRequest &req,
                                    const std::unordered_map<std::string, std::string> &params,
                                    const std::unordered_map<std::string, std::string> &baseHeaders) const {
        SsrHttpResponse resp;
        resp.headers = baseHeaders;
        Interpreter interp;
        interp.setRouterPath(req.path);
        interp.setCurrentFilePath(route.file.string());
        Value modVal = interp.importModule(route.file.string(), 1);
        if (!modVal.isMap()) {
            resp.status = 500;
            resp.statusText = "Internal Server Error";
            resp.headers["Content-Type"] = "application/json";
            resp.body = "{\"error\":\"Invalid API route module\"}";
            return resp;
        }
        auto modMap = modVal.getMap();
        std::string lowerM = req.method;
        for (char &c : lowerM) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        Value handler(std::monostate{});
        for (const std::string &cand : {req.method, lowerM, std::string("handler"), std::string("default")}) {
            if (modMap->has(cand) && modMap->entries[cand].deref().isCallable()) {
                handler = modMap->entries[cand].deref();
                break;
            }
        }
        if (!handler.isCallable()) {
            resp.status = 405;
            resp.statusText = "Method Not Allowed";
            resp.headers["Content-Type"] = "application/json; charset=utf-8";
            resp.body = "{\"error\":\"Method " + req.method + " not exported\"}";
            return resp;
        }

        auto ctxMap = std::make_shared<MemeMap>();
        ctxMap->set("req", buildReqValue(req, params));
        ctxMap->set("params", buildReqValue(req, params).getMap()->entries["params"]);
        ctxMap->set("query", buildReqValue(req, params).getMap()->entries["query"]);
        ctxMap->set("method", Value(req.method));
        ctxMap->set("body", Value(req.body));

        Value outVal = handler.getCallable()->call(interp, {Value(ctxMap)}).deref();
        if (outVal.isMap() && outVal.getMap()->has("__apiResp")) {
            auto rm = outVal.getMap();
            resp.status = rm->has("status") ? static_cast<int>(rm->entries["status"].deref().asInt()) : 200;
            resp.statusText = detail::httpStatusReason(resp.status);
            resp.body = rm->has("body") ? detail::valueToJson(rm->entries["body"]) : "{}";
        } else {
            resp.status = 200;
            resp.statusText = "OK";
            resp.body = detail::valueToJson(outVal);
        }
        if (resp.headers.find("Content-Type") == resp.headers.end()) {
            resp.headers["Content-Type"] = "application/json; charset=utf-8";
        }
        return resp;
    }

    SsrHttpResponse executePageSsr(const AppRouteEntry &route, const SsrHttpRequest &req,
                                   const std::unordered_map<std::string, std::string> &params,
                                   const std::unordered_map<std::string, std::string> &baseHeaders) const {
        SsrHttpResponse resp;
        resp.headers = baseHeaders;
        resp.headers["Content-Type"] = "text/html; charset=utf-8";

        Interpreter interp;
        interp.setRouterPath(req.path);
        interp.setCurrentFilePath(route.file.string());

        Value pageMod = interp.importModule(route.file.string(), 1);
        if (!pageMod.isMap()) {
            resp.status = 500;
            resp.statusText = "Internal Server Error";
            resp.body = "<!doctype html><html><body>500: Invalid page module</body></html>";
            return resp;
        }
        auto pm = pageMod.getMap();

        Value reqVal = buildReqValue(req, params);
        auto ctxMap = std::make_shared<MemeMap>();
        ctxMap->set("req", reqVal);
        ctxMap->set("params", reqVal.getMap()->entries["params"]);
        ctxMap->set("query", reqVal.getMap()->entries["query"]);
        ctxMap->set("path", Value(req.path));

        // 1. Data loader (`loader(ctx)`)
        Value loaderData(std::monostate{});
        if (pm->has("loader") && pm->entries["loader"].deref().isCallable()) {
            loaderData = pm->entries["loader"].deref().getCallable()->call(interp, {Value(ctxMap)}).deref();
        }
        ctxMap->set("data", loaderData);

        // 2. Document head metadata (`head(ctx)` or `metadata`)
        std::string docTitle = "MEMElang App";
        std::string docDesc;
        Value metaVal(std::monostate{});
        if (pm->has("head") && pm->entries["head"].deref().isCallable()) {
            metaVal = pm->entries["head"].deref().getCallable()->call(interp, {Value(ctxMap)}).deref();
        } else if (pm->has("metadata")) {
            metaVal = pm->entries["metadata"].deref();
        }
        if (metaVal.isMap()) {
            auto mm = metaVal.getMap();
            if (mm->has("title")) docTitle = mm->entries["title"].deref().toString();
            if (mm->has("description")) docDesc = mm->entries["description"].deref().toString();
        }

        // 3. Render Page component VDOM
        Value vdom(std::monostate{});
        Value pageFn(std::monostate{});
        for (const char *k : {"Page", "default", "App"}) {
            if (pm->has(k) && pm->entries[k].deref().isCallable()) {
                pageFn = pm->entries[k].deref();
                break;
            }
        }
        if (pageFn.isCallable()) {
            vdom = pageFn.getCallable()->call(interp, {Value(ctxMap)}).deref();
        }

        // 4. Wrap with layouts (inner -> outer)
        for (auto it = route.layouts.rbegin(); it != route.layouts.rend(); ++it) {
            Value layMod = interp.importModule(it->string(), 1);
            if (!layMod.isMap()) continue;
            auto lm = layMod.getMap();
            Value layFn(std::monostate{});
            for (const char *k : {"Layout", "default"}) {
                if (lm->has(k) && lm->entries[k].deref().isCallable()) {
                    layFn = lm->entries[k].deref();
                    break;
                }
            }
            if (layFn.isCallable()) {
                auto layProps = std::make_shared<MemeMap>();
                layProps->set("children", vdom);
                layProps->set("params", reqVal.getMap()->entries["params"]);
                layProps->set("query", reqVal.getMap()->entries["query"]);
                layProps->set("data", loaderData);
                layProps->set("path", Value(req.path));
                vdom = layFn.getCallable()->call(interp, {Value(layProps)}).deref();
            }
        }

        MemeWebDom::detail::MemeCssGenerator cssGen;
        cssGen.addBaselineDefaults();
        std::string bodyHtml = detail::renderVdomToHtml(vdom, cssGen);
        MemeWebDom::WebDomOptions cssOpts;
        cssOpts.title = docTitle;
        std::string cssText = cssGen.buildStylesheet(cssOpts);

        // Hydration JSON payload
        auto payloadMap = std::make_shared<MemeMap>();
        payloadMap->set("path", Value(req.path));
        payloadMap->set("params", reqVal.getMap()->entries["params"]);
        payloadMap->set("query", reqVal.getMap()->entries["query"]);
        payloadMap->set("data", loaderData);
        std::string ssrJson = detail::valueToJson(Value(payloadMap));

        std::ostringstream html;
        html << "<!doctype html>\n<html lang=\"en\" class=\"dark\">\n<head>\n"
             << "  <meta charset=\"utf-8\" />\n"
             << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />\n"
             << "  <title>" << detail::htmlEscape(docTitle) << "</title>\n";
        if (!docDesc.empty()) {
            html << "  <meta name=\"description\" content=\"" << detail::htmlEscape(docDesc) << "\" />\n";
        }
        html << "  <style>" << cssText << "</style>\n</head>\n"
             << "<body class=\"bg-zinc-950 text-zinc-100 antialiased\">\n"
             << "  <div id=\"app\">" << bodyHtml << "</div>\n"
             << "  <script id=\"__MEME_SSR_DATA__\" type=\"application/json\">" << ssrJson << "</script>\n"
             << "  <script>try{window.__MEME_SSR_DATA__=JSON.parse(document.getElementById('__MEME_SSR_DATA__').textContent);}catch(_){}</script>\n"
             << "</body>\n</html>\n";
        resp.status = 200;
        resp.statusText = "OK";
        resp.body = html.str();
        return resp;
    }

    SsrHttpResponse serveSingleEntryAsset(const std::string &path, const std::unordered_map<std::string, std::string> &baseHeaders) const {
        SsrHttpResponse resp;
        resp.headers = baseHeaders;
        MemeWebDom::WebDomOptions opts;
        auto bundle = MemeWebDom::transpileToWebDomBundle(singleEntryFallback, opts);
        if (!bundle.success) {
            resp.status = 500;
            resp.body = bundle.error;
            return resp;
        }
        resp.status = 200;
        if (path == "/styles.css") {
            resp.headers["Content-Type"] = "text/css; charset=utf-8";
            resp.body = bundle.stylesCss;
        } else if (path == "/meme-dom.js") {
            resp.headers["Content-Type"] = "application/javascript; charset=utf-8";
            resp.body = bundle.memeDomJs;
        } else {
            resp.headers["Content-Type"] = "application/javascript; charset=utf-8";
            resp.body = bundle.appJs;
        }
        return resp;
    }

    SsrHttpResponse executeSingleEntrySsr(const SsrHttpRequest &req, const std::unordered_map<std::string, std::string> &baseHeaders) const {
        SsrHttpResponse resp;
        resp.headers = baseHeaders;
        resp.headers["Content-Type"] = "text/html; charset=utf-8";

        Interpreter interp;
        interp.setRouterPath(req.path);
        interp.setCurrentFilePath(singleEntryFallback.string());
        std::string src;
        {
            std::ifstream ifs(singleEntryFallback);
            std::stringstream ss; ss << ifs.rdbuf();
            src = ss.str();
        }
        Lexer lex(src);
        auto toks = lex.tokenize();
        Parser parser(toks);
        auto prog = parser.parseProgram();
        interp.interpret(prog);

        Value vdom = interp.getRootUiComponent();
        if (vdom.deref().isNil() && interp.rootCallable.deref().isCallable()) {
            auto props = std::make_shared<MemeMap>();
            vdom = interp.rootCallable.deref().getCallable()->call(interp, {Value(props)}).deref();
        }

        MemeWebDom::WebDomOptions domOpts;
        domOpts.title = !interp.getRequestedWindowTitle().empty() ? interp.getRequestedWindowTitle() : "MEMElang App";
        auto bundle = MemeWebDom::transpileToWebDomBundle(singleEntryFallback, domOpts);

        MemeWebDom::detail::MemeCssGenerator cssGen;
        cssGen.addBaselineDefaults();
        std::string ssrHtml = detail::renderVdomToHtml(vdom, cssGen);
        std::string ssrJson = "{\"path\":" + detail::jsonEscape(req.path) + ",\"params\":{},\"query\":{}}";

        std::ostringstream html;
        html << "<!doctype html>\n<html lang=\"en\" class=\"dark\">\n<head>\n"
             << "  <meta charset=\"utf-8\" />\n"
             << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />\n"
             << "  <title>" << detail::htmlEscape(domOpts.title) << "</title>\n"
             << "  <link rel=\"stylesheet\" href=\"/styles.css\" />\n"
             << "</head>\n<body class=\"bg-zinc-950 text-zinc-100 antialiased\">\n"
             << "  <div id=\"app\">" << ssrHtml << "</div>\n"
             << "  <script id=\"__MEME_SSR_DATA__\" type=\"application/json\">" << ssrJson << "</script>\n"
             << "  <script>try{window.__MEME_SSR_DATA__=JSON.parse(document.getElementById('__MEME_SSR_DATA__').textContent);}catch(_){}</script>\n"
             << "  <script type=\"module\" src=\"/app.js\"></script>\n"
             << "</body>\n</html>\n";
        resp.status = 200;
        resp.body = html.str();
        return resp;
    }

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
    static SsrHttpRequest parseRawHttp(const std::string &raw, size_t hend, int cfd) {
        SsrHttpRequest req;
        size_t lineEnd = raw.find("\r\n");
        std::string reqLine = raw.substr(0, lineEnd);
        size_t s1 = reqLine.find(' ');
        size_t s2 = (s1 != std::string::npos) ? reqLine.find(' ', s1 + 1) : std::string::npos;
        if (s1 != std::string::npos) {
            req.method = reqLine.substr(0, s1);
            req.rawUrl = (s2 != std::string::npos) ? reqLine.substr(s1 + 1, s2 - s1 - 1) : reqLine.substr(s1 + 1);
            req.path = req.rawUrl;
        }
        size_t pos = lineEnd + 2;
        size_t contentLen = 0;
        while (pos < hend) {
            size_t next = raw.find("\r\n", pos);
            if (next == std::string::npos || next > hend) break;
            std::string hline = raw.substr(pos, next - pos);
            pos = next + 2;
            size_t col = hline.find(':');
            if (col != std::string::npos) {
                std::string hk = hline.substr(0, col);
                for (char &ch : hk) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                std::string hv = hline.substr(col + 1);
                while (!hv.empty() && hv.front() == ' ') hv.erase(hv.begin());
                req.headers[hk] = hv;
                if (hk == "content-length") {
                    try { contentLen = static_cast<size_t>(std::stoul(hv)); } catch (...) {}
                }
            }
        }
        std::string bodyAcc = (hend + 4 < raw.size()) ? raw.substr(hend + 4) : "";
        char buf[4096];
        while (bodyAcc.size() < contentLen) {
            ssize_t n = recv(cfd, buf, sizeof(buf), 0);
            if (n <= 0) break;
            bodyAcc.append(buf, static_cast<size_t>(n));
        }
        if (contentLen > 0 && bodyAcc.size() >= contentLen) req.body = bodyAcc.substr(0, contentLen);
        else req.body = bodyAcc;
        return req;
    }

    static std::string formatWireHttp(const SsrHttpResponse &resp) {
        std::ostringstream os;
        os << "HTTP/1.1 " << resp.status << " " << resp.statusText << "\r\n";
        bool hasLen = false;
        for (const auto &kv : resp.headers) {
            os << kv.first << ": " << kv.second << "\r\n";
            std::string lk = kv.first;
            for (char &ch : lk) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            if (lk == "content-length") hasLen = true;
        }
        if (!hasLen) os << "Content-Length: " << resp.body.size() << "\r\n";
        os << "Connection: close\r\n\r\n" << resp.body;
        return os.str();
    }
#endif
};

} // namespace MemeSsr
