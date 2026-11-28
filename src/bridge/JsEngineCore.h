#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#if !defined(__EMSCRIPTEN__)
  #if defined(__APPLE__) && !defined(MEME_USE_QUICKJS)
    #define MEME_JS_USE_JSC 1
    #include <JavaScriptCore/JavaScriptCore.h>
    #ifdef pascal
      #undef pascal
    #endif
  #else
    #define MEME_JS_USE_QUICKJS 1
    #include "../../third_party/quickjs/quickjs.h"
  #endif
#endif

namespace MemeJs {

struct JsNode {
    enum Kind {
        K_NULL,
        K_BOOL,
        K_INT,
        K_DOUBLE,
        K_STRING,
        K_ARRAY,
        K_OBJECT,
        K_FUNCTION,
        K_HOST_FUNCTION
    };

    Kind kind = K_NULL;
    bool bVal = false;
    int64_t iVal = 0;
    double dVal = 0.0;
    std::string sVal;
    std::vector<JsNode> arrVal;
    std::vector<std::pair<std::string, JsNode>> objVal;
    uint64_t fnId = 0;
    std::function<JsNode(const std::vector<JsNode> &)> hostFn;

    static JsNode makeNull() { return JsNode{}; }
    static JsNode makeBool(bool b) { JsNode n; n.kind = K_BOOL; n.bVal = b; return n; }
    static JsNode makeInt(int64_t v) { JsNode n; n.kind = K_INT; n.iVal = v; n.dVal = static_cast<double>(v); return n; }
    static JsNode makeDouble(double v) { JsNode n; n.kind = K_DOUBLE; n.dVal = v; return n; }
    static JsNode makeString(std::string s) { JsNode n; n.kind = K_STRING; n.sVal = std::move(s); return n; }
    static JsNode makeArray(std::vector<JsNode> a) { JsNode n; n.kind = K_ARRAY; n.arrVal = std::move(a); return n; }
    static JsNode makeObject(std::vector<std::pair<std::string, JsNode>> o) { JsNode n; n.kind = K_OBJECT; n.objVal = std::move(o); return n; }
    static JsNode makeFunction(uint64_t id, std::vector<std::pair<std::string, JsNode>> props = {}) {
        JsNode n; n.kind = K_FUNCTION; n.fnId = id; n.objVal = std::move(props); return n;
    }
    static JsNode makeHostFn(std::function<JsNode(const std::vector<JsNode> &)> cb) {
        JsNode n; n.kind = K_HOST_FUNCTION; n.hostFn = std::move(cb); return n;
    }

    const JsNode *getProp(const std::string &k) const {
        for (const auto &kv : objVal) {
            if (kv.first == k) return &kv.second;
        }
        return nullptr;
    }
};

namespace detail {

inline std::string readTextFile(const std::filesystem::path &p) {
    std::ifstream ifs(p, std::ios::binary);
    if (!ifs.is_open()) return "";
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}

inline std::string trimCopy(const std::string &s) {
    size_t a = 0;
    while (a < s.size() && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    size_t b = s.size();
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return s.substr(a, b - a);
}

inline std::string escapeJsStr(const std::string &s) {
    std::string out = "\"";
    for (unsigned char c : s) {
        if (c == '\\') out += "\\\\";
        else if (c == '"') out += "\\\"";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out.push_back(static_cast<char>(c));
    }
    out.push_back('"');
    return out;
}

inline std::string extractJsonStringField(const std::string &json, const std::string &key) {
    std::string needle = "\"" + key + "\"";
    size_t pos = json.find(needle);
    while (pos != std::string::npos) {
        size_t after = pos + needle.size();
        while (after < json.size() && std::isspace(static_cast<unsigned char>(json[after]))) ++after;
        if (after < json.size() && json[after] == ':') {
            ++after;
            while (after < json.size() && std::isspace(static_cast<unsigned char>(json[after]))) ++after;
            if (after < json.size() && json[after] == '"') {
                size_t q = after + 1;
                std::string val;
                while (q < json.size()) {
                    if (json[q] == '\\' && q + 1 < json.size()) {
                        val.push_back(json[q + 1]);
                        q += 2;
                    } else if (json[q] == '"') {
                        return val;
                    } else {
                        val.push_back(json[q++]);
                    }
                }
            } else if (after < json.size() && json[after] == '{') {
                size_t braceDepth = 0;
                size_t subEnd = after;
                for (size_t i = after; i < json.size(); ++i) {
                    if (json[i] == '{') ++braceDepth;
                    else if (json[i] == '}') {
                        --braceDepth;
                        if (braceDepth == 0) { subEnd = i; break; }
                    }
                }
                std::string subObj = json.substr(after, subEnd - after + 1);
                if (auto s = extractJsonStringField(subObj, "."); !s.empty()) return s;
                if (auto s = extractJsonStringField(subObj, "require"); !s.empty()) return s;
                if (auto s = extractJsonStringField(subObj, "default"); !s.empty()) return s;
                if (auto s = extractJsonStringField(subObj, "import"); !s.empty()) return s;
            }
            break;
        }
        pos = json.find(needle, pos + needle.size());
    }
    return "";
}

// Converts simple top-level ES module syntax into CommonJS for embedded headless execution
inline std::string stripSimpleEsm(const std::string &src) {
    bool maybeEsm = (src.find("export ") != std::string::npos ||
                     src.find("import ") != std::string::npos ||
                     src.find("export{") != std::string::npos);
    if (!maybeEsm) return src;

    std::istringstream iss(src);
    std::ostringstream out;
    std::string line;
    while (std::getline(iss, line)) {
        std::string t = trimCopy(line);
        if (t.rfind("import ", 0) == 0 && t.find(" from ") != std::string::npos) {
            size_t fromPos = t.find(" from ");
            std::string binding = trimCopy(t.substr(7, fromPos - 7));
            std::string modPart = trimCopy(t.substr(fromPos + 6));
            if (!modPart.empty() && modPart.back() == ';') modPart.pop_back();
            modPart = trimCopy(modPart);
            if (binding.rfind("* as ", 0) == 0) {
                std::string ns = trimCopy(binding.substr(5));
                out << "var " << ns << " = require(" << modPart << ");\n";
            } else if (!binding.empty() && binding.front() == '{') {
                std::string fixed = binding;
                for (size_t p = fixed.find(" as "); p != std::string::npos; p = fixed.find(" as ", p)) {
                    fixed.replace(p, 4, ": ");
                }
                out << "var " << fixed << " = require(" << modPart << ");\n";
            } else {
                out << "var _imp_" << binding << " = require(" << modPart << ");\n"
                    << "var " << binding << " = (_imp_" << binding << " && _imp_" << binding << ".default !== undefined) ? _imp_" << binding << ".default : _imp_" << binding << ";\n";
            }
            continue;
        }
        if (t.rfind("export default function ", 0) == 0) {
            std::string rest = t.substr(24);
            size_t lp = rest.find('(');
            std::string fnName = (lp != std::string::npos) ? trimCopy(rest.substr(0, lp)) : "";
            if (!fnName.empty()) {
                out << "function " << rest << "\nmodule.exports = " << fnName << ";\nmodule.exports.default = " << fnName << ";\n";
            } else {
                out << "module.exports = function " << rest << "\n";
            }
            continue;
        }
        if (t.rfind("export default ", 0) == 0) {
            out << "var _memeDefExp = " << t.substr(15) << "\nmodule.exports = _memeDefExp;\nif (_memeDefExp && (typeof _memeDefExp === 'object' || typeof _memeDefExp === 'function')) _memeDefExp.default = _memeDefExp;\n";
            continue;
        }
        if (t.rfind("export async function ", 0) == 0) {
            std::string rest = t.substr(22);
            size_t lp = rest.find('(');
            std::string fnName = (lp != std::string::npos) ? trimCopy(rest.substr(0, lp)) : "";
            out << "async function " << rest << "\n";
            if (!fnName.empty()) out << "exports." << fnName << " = " << fnName << ";\n";
            continue;
        }
        if (t.rfind("export function ", 0) == 0) {
            std::string rest = t.substr(16);
            size_t lp = rest.find('(');
            std::string fnName = (lp != std::string::npos) ? trimCopy(rest.substr(0, lp)) : "";
            out << "function " << rest << "\n";
            if (!fnName.empty()) out << "exports." << fnName << " = " << fnName << ";\n";
            continue;
        }
        if (t.rfind("export const ", 0) == 0 || t.rfind("export let ", 0) == 0 || t.rfind("export var ", 0) == 0) {
            size_t sp = t.find(' ', 7);
            std::string rest = t.substr(sp + 1);
            size_t eq = rest.find('=');
            if (eq != std::string::npos) {
                std::string vname = trimCopy(rest.substr(0, eq));
                out << "var " << vname << " = exports." << vname << " = " << trimCopy(rest.substr(eq + 1)) << "\n";
                continue;
            }
        }
        if (t.rfind("export class ", 0) == 0) {
            std::string rest = t.substr(13);
            size_t sp = rest.find_first_of(" {");
            std::string cname = (sp != std::string::npos) ? trimCopy(rest.substr(0, sp)) : "";
            out << "class " << rest << "\n";
            if (!cname.empty()) out << "exports." << cname << " = " << cname << ";\n";
            continue;
        }
        if (t.rfind("export {", 0) == 0 || t.rfind("export {", 0) == 0) {
            size_t lb = t.find('{');
            size_t rb = t.find('}', lb);
            if (lb != std::string::npos && rb != std::string::npos) {
                std::string inner = t.substr(lb + 1, rb - lb - 1);
                std::stringstream ss(inner);
                std::string item;
                while (std::getline(ss, item, ',')) {
                    std::string it = trimCopy(item);
                    if (it.empty()) continue;
                    size_t asPos = it.find(" as ");
                    if (asPos != std::string::npos) {
                        std::string local = trimCopy(it.substr(0, asPos));
                        std::string exported = trimCopy(it.substr(asPos + 4));
                        out << "exports[" << escapeJsStr(exported) << "] = " << local << ";\n";
                    } else {
                        out << "exports[" << escapeJsStr(it) << "] = " << it << ";\n";
                    }
                }
                continue;
            }
        }
        out << line << "\n";
    }
    return out.str();
}

inline std::filesystem::path resolveFileTarget(const std::filesystem::path &base) {
    std::error_code ec;
    if (std::filesystem::is_regular_file(base, ec)) return std::filesystem::weakly_canonical(base, ec);
    for (const char *ext : {".js", ".cjs", ".mjs", ".json"}) {
        std::filesystem::path p(base.string() + ext);
        if (std::filesystem::is_regular_file(p, ec)) return std::filesystem::weakly_canonical(p, ec);
    }
    if (std::filesystem::is_directory(base, ec)) {
        std::filesystem::path pj = base / "package.json";
        if (std::filesystem::is_regular_file(pj, ec)) {
            std::string json = readTextFile(pj);
            std::string mainRel = extractJsonStringField(json, "exports");
            if (mainRel.empty()) mainRel = extractJsonStringField(json, "main");
            if (mainRel.empty()) mainRel = extractJsonStringField(json, "module");
            if (!mainRel.empty()) {
                auto resolved = resolveFileTarget(base / mainRel);
                if (!resolved.empty()) return resolved;
            }
        }
        for (const char *idx : {"index.js", "index.cjs", "index.mjs"}) {
            std::filesystem::path p = base / idx;
            if (std::filesystem::is_regular_file(p, ec)) return std::filesystem::weakly_canonical(p, ec);
        }
    }
    return {};
}

inline std::filesystem::path findNpmPackageEntry(const std::string &spec, const std::filesystem::path &callerDir) {
    std::error_code ec;
    if (spec.empty()) return {};

    // Relative module specifier inside package
    if (spec.rfind("./", 0) == 0 || spec.rfind("../", 0) == 0 || spec.front() == '/') {
        std::filesystem::path target = (spec.front() == '/') ? std::filesystem::path(spec) : (callerDir / spec);
        return resolveFileTarget(target);
    }

    // Parse scoped (@org/pkg[/sub]) vs unscoped (pkg[/sub])
    std::string pkgRoot = spec;
    std::string subPath;
    if (spec.front() == '@') {
        size_t s1 = spec.find('/');
        if (s1 != std::string::npos) {
            size_t s2 = spec.find('/', s1 + 1);
            if (s2 != std::string::npos) {
                pkgRoot = spec.substr(0, s2);
                subPath = spec.substr(s2 + 1);
            }
        }
    } else {
        size_t s1 = spec.find('/');
        if (s1 != std::string::npos) {
            pkgRoot = spec.substr(0, s1);
            subPath = spec.substr(s1 + 1);
        }
    }

    std::vector<std::filesystem::path> searchRoots;
    if (!callerDir.empty()) {
        std::filesystem::path cur = callerDir;
        for (int i = 0; i < 6; ++i) {
            searchRoots.push_back(cur / ".tapri_modules" / "npm");
            searchRoots.push_back(cur / "node_modules");
            if (!cur.has_parent_path() || cur.parent_path() == cur) break;
            cur = cur.parent_path();
        }
    }
    std::filesystem::path cwd = std::filesystem::current_path(ec);
    if (!cwd.empty()) {
        searchRoots.push_back(cwd / ".tapri_modules" / "npm");
        searchRoots.push_back(cwd / "node_modules");
    }

    for (const auto &root : searchRoots) {
        std::filesystem::path pkgDir = root / pkgRoot;
        if (!std::filesystem::exists(pkgDir, ec)) continue;
        std::filesystem::path target = subPath.empty() ? pkgDir : (pkgDir / subPath);
        auto res = resolveFileTarget(target);
        if (!res.empty()) return res;
    }
    return {};
}

inline const std::unordered_map<std::string, std::string> &builtinVirtualModules() {
    static const std::unordered_map<std::string, std::string> mods = {
        {"clsx", R"JS(
function toVal(mix) {
  var k, y, str = '';
  if (typeof mix === 'string' || typeof mix === 'number') {
    str += mix;
  } else if (typeof mix === 'object' && mix !== null) {
    if (Array.isArray(mix)) {
      for (k = 0; k < mix.length; k++) {
        if (mix[k]) {
          if ((y = toVal(mix[k]))) {
            if (str) str += ' ';
            str += y;
          }
        }
      }
    } else {
      for (k in mix) {
        if (mix[k]) {
          if (str) str += ' ';
          str += k;
        }
      }
    }
  }
  return str;
}
function clsx() {
  var i = 0, tmp, x, str = '';
  while (i < arguments.length) {
    if ((tmp = arguments[i++])) {
      if ((x = toVal(tmp))) {
        if (str) str += ' ';
        str += x;
      }
    }
  }
  return str;
}
module.exports = clsx;
module.exports.clsx = clsx;
module.exports.default = clsx;
)JS"},
        {"tailwind-merge", R"JS(
function twGroup(cls) {
  if (/^p-\d/.test(cls)) return 'p';
  if (/^px-/.test(cls)) return 'px';
  if (/^py-/.test(cls)) return 'py';
  if (/^pt-/.test(cls)) return 'pt';
  if (/^pb-/.test(cls)) return 'pb';
  if (/^pl-/.test(cls)) return 'pl';
  if (/^pr-/.test(cls)) return 'pr';
  if (/^m-/.test(cls)) return 'm';
  if (/^mx-/.test(cls)) return 'mx';
  if (/^my-/.test(cls)) return 'my';
  if (/^w-/.test(cls)) return 'w';
  if (/^h-/.test(cls)) return 'h';
  if (/^bg-/.test(cls)) return 'bg';
  if (/^border(-\d)?$/.test(cls)) return 'bw';
  if (/^border-[a-z]/.test(cls)) return 'bc';
  if (/^rounded/.test(cls)) return 'rounded';
  if (/^text-(xs|sm|base|lg|xl|\d+xl)$/.test(cls)) return 'tsize';
  if (/^text-/.test(cls)) return 'tcolor';
  if (/^font-/.test(cls)) return 'font';
  if (/^(flex|block|inline|inline-flex|grid|hidden)$/.test(cls)) return 'display';
  if (/^flex-(row|col)/.test(cls)) return 'fdir';
  if (/^gap-/.test(cls)) return 'gap';
  if (/^opacity-/.test(cls)) return 'opacity';
  return 'u:' + cls;
}
function twMerge() {
  var tokens = [];
  for (var i = 0; i < arguments.length; i++) {
    var arg = arguments[i];
    if (!arg) continue;
    var parts = String(arg).trim().split(/\s+/);
    for (var j = 0; j < parts.length; j++) {
      if (parts[j]) tokens.push(parts[j]);
    }
  }
  var seen = Object.create(null);
  var out = [];
  for (var k = tokens.length - 1; k >= 0; k--) {
    var g = twGroup(tokens[k]);
    if (!seen[g]) {
      seen[g] = true;
      out.unshift(tokens[k]);
    }
  }
  return out.join(' ');
}
exports.twMerge = twMerge;
exports.default = twMerge;
module.exports = { twMerge: twMerge, default: twMerge };
)JS"},
        {"zustand", R"JS(
function createStore(initializer) {
  var state;
  var listeners = [];
  var setState = function(partial, replace) {
    var next = typeof partial === 'function' ? partial(state) : partial;
    if (!Object.is(next, state)) {
      var prev = state;
      state = replace ? next : Object.assign({}, state, next);
      for (var i = 0; i < listeners.length; i++) listeners[i](state, prev);
    }
  };
  var getState = function() { return state; };
  var subscribe = function(fn) {
    listeners.push(fn);
    return function() { listeners = listeners.filter(function(f) { return f !== fn; }); };
  };
  var api = { setState: setState, getState: getState, subscribe: subscribe };
  state = initializer(setState, getState, api);
  var useStore = function(selector) {
    return selector ? selector(state) : state;
  };
  useStore.setState = setState;
  useStore.getState = getState;
  useStore.subscribe = subscribe;
  return useStore;
}
exports.create = createStore;
exports.createStore = createStore;
exports.default = createStore;
)JS"},
        {"path", R"JS(
exports.join = function() {
  var parts = Array.prototype.slice.call(arguments).filter(Boolean).map(String);
  return parts.join('/').replace(/\/+/g, '/');
};
exports.basename = function(p, ext) {
  var s = String(p || '');
  var b = s.slice(s.lastIndexOf('/') + 1);
  if (ext && b.endsWith(ext)) b = b.slice(0, -ext.length);
  return b;
};
exports.dirname = function(p) {
  var s = String(p || '');
  var idx = s.lastIndexOf('/');
  return idx <= 0 ? '.' : s.slice(0, idx);
};
exports.extname = function(p) {
  var b = exports.basename(p);
  var dot = b.lastIndexOf('.');
  return dot <= 0 ? '' : b.slice(dot);
};
)JS"},
        {"events", R"JS(
function EventEmitter() { this._ev = Object.create(null); }
EventEmitter.prototype.on = function(name, fn) {
  if (!this._ev[name]) this._ev[name] = [];
  this._ev[name].push(fn);
  return this;
};
EventEmitter.prototype.off = function(name, fn) {
  if (this._ev[name]) this._ev[name] = this._ev[name].filter(function(f) { return f !== fn; });
  return this;
};
EventEmitter.prototype.emit = function(name) {
  var list = this._ev[name];
  if (!list) return false;
  var args = Array.prototype.slice.call(arguments, 1);
  for (var i = 0; i < list.length; i++) list[i].apply(this, args);
  return true;
};
exports.EventEmitter = EventEmitter;
module.exports = EventEmitter;
module.exports.EventEmitter = EventEmitter;
)JS"}
    };
    return mods;
}

} // namespace detail

class JsEngineSession {
public:
    JsEngineSession() { initEngine(); }
    ~JsEngineSession() { shutdownEngine(); }

    JsEngineSession(const JsEngineSession &) = delete;
    JsEngineSession &operator=(const JsEngineSession &) = delete;

    static const char *engineName() {
#if defined(MEME_JS_USE_JSC)
        return "JavaScriptCore";
#elif defined(MEME_JS_USE_QUICKJS)
        return "QuickJS-NG";
#else
        return "WebJS";
#endif
    }

    JsNode loadPackage(const std::string &pkgSpec, const std::string &callerDir = "") {
        std::lock_guard<std::recursive_mutex> lk(mtx);
        return loadModuleInternal(pkgSpec, callerDir.empty() ? std::filesystem::current_path().string() : callerDir);
    }

    JsNode evalScript(const std::string &code, const std::string &filename = "<eval>") {
        std::lock_guard<std::recursive_mutex> lk(mtx);
        return evalScriptInternal(code, filename);
    }

    JsNode callFunction(uint64_t fnId, const std::vector<JsNode> &args) {
        std::lock_guard<std::recursive_mutex> lk(mtx);
        return callFunctionInternal(fnId, args);
    }

    std::vector<std::string> drainConsoleLogs() {
        std::lock_guard<std::recursive_mutex> lk(mtx);
        std::vector<std::string> copy = std::move(consoleLogs);
        consoleLogs.clear();
        return copy;
    }

private:
    std::recursive_mutex mtx;
    uint64_t nextFnId = 1;
    uint64_t nextHostCbId = 1;
    std::unordered_map<uint64_t, std::function<JsNode(const std::vector<JsNode> &)>> hostCallbacks;
    std::vector<std::string> consoleLogs;
    std::unordered_map<std::string, uint64_t> cacheKeys; // normalized spec/path -> global cache property name id

#if defined(MEME_JS_USE_JSC)
    JSContextGroupRef group = nullptr;
    JSGlobalContextRef ctx = nullptr;
    std::unordered_map<uint64_t, JSObjectRef> fnRefs;
    std::unordered_map<std::string, JSValueRef> moduleCache;

    static inline std::mutex s_sessRegMtx;
    static inline std::unordered_map<uint64_t, JsEngineSession *> s_sessReg;
    static inline uint64_t s_nextSessId = 1;
    uint64_t sessId = 0;

    static JsEngineSession *lookupSession(JSContextRef c) {
        JSStringRef k = JSStringCreateWithUTF8CString("__memeSessId");
        JSValueRef v = JSObjectGetProperty(c, JSContextGetGlobalObject(c), k, nullptr);
        JSStringRelease(k);
        if (!JSValueIsNumber(c, v)) return nullptr;
        uint64_t id = static_cast<uint64_t>(JSValueToNumber(c, v, nullptr));
        std::lock_guard<std::mutex> lk(s_sessRegMtx);
        auto it = s_sessReg.find(id);
        return (it != s_sessReg.end()) ? it->second : nullptr;
    }

    static std::string jsStrToStd(JSStringRef s) {
        if (!s) return "";
        size_t maxB = JSStringGetMaximumUTF8CStringSize(s);
        std::string out(maxB, '\0');
        size_t wrote = JSStringGetUTF8CString(s, out.data(), maxB);
        if (wrote > 0) out.resize(wrote - 1);
        else out.clear();
        return out;
    }

    static std::string jsValToErrorStr(JSContextRef c, JSValueRef exc) {
        if (!exc) return "Unknown JS error";
        JSStringRef s = JSValueToStringCopy(c, exc, nullptr);
        std::string out = jsStrToStd(s);
        if (s) JSStringRelease(s);
        return out;
    }

    static JSValueRef jscRequireCb(JSContextRef c, JSObjectRef, JSObjectRef, size_t argc, const JSValueRef argv[], JSValueRef *exc) {
        JsEngineSession *self = lookupSession(c);
        if (!self || argc < 1) return JSValueMakeUndefined(c);
        JSStringRef s = JSValueToStringCopy(c, argv[0], exc);
        std::string reqSpec = jsStrToStd(s);
        if (s) JSStringRelease(s);
        std::string baseDir;
        if (argc >= 2 && JSValueIsString(c, argv[1])) {
            JSStringRef ds = JSValueToStringCopy(c, argv[1], exc);
            baseDir = jsStrToStd(ds);
            if (ds) JSStringRelease(ds);
        }
        try {
            return self->loadModuleRawJsc(reqSpec, baseDir, exc);
        } catch (const std::exception &e) {
            JSStringRef es = JSStringCreateWithUTF8CString(e.what());
            if (exc) *exc = JSValueMakeString(c, es);
            JSStringRelease(es);
            return JSValueMakeUndefined(c);
        }
    }

    static JSValueRef jscConsoleLogCb(JSContextRef c, JSObjectRef, JSObjectRef, size_t argc, const JSValueRef argv[], JSValueRef *exc) {
        JsEngineSession *self = lookupSession(c);
        std::string line;
        for (size_t i = 0; i < argc; ++i) {
            if (i > 0) line += " ";
            JSStringRef s = JSValueToStringCopy(c, argv[i], exc);
            line += jsStrToStd(s);
            if (s) JSStringRelease(s);
        }
        if (self) self->consoleLogs.push_back(line);
        return JSValueMakeUndefined(c);
    }

    static JSValueRef jscInvokeHostCb(JSContextRef c, JSObjectRef, JSObjectRef, size_t argc, const JSValueRef argv[], JSValueRef *exc) {
        JsEngineSession *self = lookupSession(c);
        if (!self || argc < 1) return JSValueMakeUndefined(c);
        uint64_t cbId = static_cast<uint64_t>(JSValueToNumber(c, argv[0], exc));
        auto it = self->hostCallbacks.find(cbId);
        if (it == self->hostCallbacks.end()) return JSValueMakeUndefined(c);
        std::vector<JsNode> args;
        if (argc >= 2 && JSValueIsArray(c, argv[1])) {
            JSObjectRef arr = JSValueToObject(c, argv[1], exc);
            JSStringRef lk = JSStringCreateWithUTF8CString("length");
            size_t len = static_cast<size_t>(JSValueToNumber(c, JSObjectGetProperty(c, arr, lk, exc), exc));
            JSStringRelease(lk);
            for (size_t i = 0; i < len; ++i) {
                args.push_back(self->jscToNode(JSObjectGetPropertyAtIndex(c, arr, static_cast<unsigned>(i), exc)));
            }
        }
        try {
            JsNode res = it->second(args);
            return self->nodeToJsc(res);
        } catch (const std::exception &e) {
            JSStringRef es = JSStringCreateWithUTF8CString(e.what());
            if (exc) *exc = JSValueMakeString(c, es);
            JSStringRelease(es);
            return JSValueMakeUndefined(c);
        }
    }

    JSValueRef nodeToJsc(const JsNode &n) {
        switch (n.kind) {
            case JsNode::K_NULL: return JSValueMakeNull(ctx);
            case JsNode::K_BOOL: return JSValueMakeBoolean(ctx, n.bVal);
            case JsNode::K_INT: return JSValueMakeNumber(ctx, static_cast<double>(n.iVal));
            case JsNode::K_DOUBLE: return JSValueMakeNumber(ctx, n.dVal);
            case JsNode::K_STRING: {
                JSStringRef s = JSStringCreateWithUTF8CString(n.sVal.c_str());
                JSValueRef v = JSValueMakeString(ctx, s);
                JSStringRelease(s);
                return v;
            }
            case JsNode::K_ARRAY: {
                std::vector<JSValueRef> items;
                items.reserve(n.arrVal.size());
                for (const auto &it : n.arrVal) items.push_back(nodeToJsc(it));
                return JSObjectMakeArray(ctx, items.size(), items.empty() ? nullptr : items.data(), nullptr);
            }
            case JsNode::K_OBJECT: {
                JSObjectRef obj = JSObjectMake(ctx, nullptr, nullptr);
                for (const auto &kv : n.objVal) {
                    JSStringRef k = JSStringCreateWithUTF8CString(kv.first.c_str());
                    JSObjectSetProperty(ctx, obj, k, nodeToJsc(kv.second), kJSPropertyAttributeNone, nullptr);
                    JSStringRelease(k);
                }
                return obj;
            }
            case JsNode::K_FUNCTION: {
                auto it = fnRefs.find(n.fnId);
                return (it != fnRefs.end()) ? it->second : JSValueMakeUndefined(ctx);
            }
            case JsNode::K_HOST_FUNCTION: {
                uint64_t cbId = nextHostCbId++;
                hostCallbacks[cbId] = n.hostFn;
                std::string expr = "(function(){ return __memeInvokeHostCb(" + std::to_string(cbId) + ", Array.prototype.slice.call(arguments)); })";
                JSStringRef es = JSStringCreateWithUTF8CString(expr.c_str());
                JSValueRef fnVal = JSEvaluateScript(ctx, es, nullptr, nullptr, 1, nullptr);
                JSStringRelease(es);
                return fnVal ? fnVal : JSValueMakeUndefined(ctx);
            }
        }
        return JSValueMakeNull(ctx);
    }

    JsNode jscToNode(JSValueRef v, int depth = 0) {
        if (!v || JSValueIsUndefined(ctx, v) || JSValueIsNull(ctx, v)) return JsNode::makeNull();
        if (JSValueIsBoolean(ctx, v)) return JsNode::makeBool(JSValueToBoolean(ctx, v));
        if (JSValueIsNumber(ctx, v)) {
            double d = JSValueToNumber(ctx, v, nullptr);
            if (std::isfinite(d) && std::floor(d) == d && d >= -9007199254740991.0 && d <= 9007199254740991.0) {
                return JsNode::makeInt(static_cast<int64_t>(d));
            }
            return JsNode::makeDouble(d);
        }
        if (JSValueIsString(ctx, v)) {
            JSStringRef s = JSValueToStringCopy(ctx, v, nullptr);
            std::string out = jsStrToStd(s);
            if (s) JSStringRelease(s);
            return JsNode::makeString(std::move(out));
        }
        if (depth >= 12 || !JSValueIsObject(ctx, v)) return JsNode::makeNull();
        JSObjectRef obj = JSValueToObject(ctx, v, nullptr);
        if (!obj) return JsNode::makeNull();

        auto collectProps = [&]() {
            std::vector<std::pair<std::string, JsNode>> props;
            JSPropertyNameArrayRef keys = JSObjectCopyPropertyNames(ctx, obj);
            size_t cnt = JSPropertyNameArrayGetCount(keys);
            for (size_t i = 0; i < cnt; ++i) {
                JSStringRef k = JSPropertyNameArrayGetNameAtIndex(keys, i);
                std::string kStr = jsStrToStd(k);
                JSValueRef pv = JSObjectGetProperty(ctx, obj, k, nullptr);
                props.emplace_back(std::move(kStr), jscToNode(pv, depth + 1));
            }
            JSPropertyNameArrayRelease(keys);
            return props;
        };

        if (JSObjectIsFunction(ctx, obj)) {
            uint64_t id = nextFnId++;
            JSValueProtect(ctx, obj);
            fnRefs[id] = obj;
            return JsNode::makeFunction(id, collectProps());
        }
        if (JSValueIsArray(ctx, v)) {
            JSStringRef lk = JSStringCreateWithUTF8CString("length");
            JSValueRef lv = JSObjectGetProperty(ctx, obj, lk, nullptr);
            JSStringRelease(lk);
            size_t len = static_cast<size_t>(JSValueToNumber(ctx, lv, nullptr));
            std::vector<JsNode> arr;
            arr.reserve(std::min<size_t>(len, 10000));
            for (size_t i = 0; i < len && i < 10000; ++i) {
                JSValueRef iv = JSObjectGetPropertyAtIndex(ctx, obj, static_cast<unsigned>(i), nullptr);
                arr.push_back(jscToNode(iv, depth + 1));
            }
            return JsNode::makeArray(std::move(arr));
        }
        return JsNode::makeObject(collectProps());
    }

    void initEngine() {
        group = JSContextGroupCreate();
        ctx = JSGlobalContextCreateInGroup(group, nullptr);
        JSObjectRef global = JSContextGetGlobalObject(ctx);
        {
            std::lock_guard<std::mutex> lk(s_sessRegMtx);
            sessId = s_nextSessId++;
            s_sessReg[sessId] = this;
        }
        JSStringRef sidKey = JSStringCreateWithUTF8CString("__memeSessId");
        JSObjectSetProperty(ctx, global, sidKey, JSValueMakeNumber(ctx, static_cast<double>(sessId)), kJSPropertyAttributeNone, nullptr);
        JSStringRelease(sidKey);

        JSStringRef gThis = JSStringCreateWithUTF8CString("globalThis");
        JSObjectSetProperty(ctx, global, gThis, global, kJSPropertyAttributeNone, nullptr);
        JSStringRelease(gThis);

        JSStringRef reqName = JSStringCreateWithUTF8CString("__memeHostRequire");
        JSObjectRef reqFn = JSObjectMakeFunctionWithCallback(ctx, reqName, &jscRequireCb);
        JSObjectSetProperty(ctx, global, reqName, reqFn, kJSPropertyAttributeNone, nullptr);
        JSStringRelease(reqName);

        JSStringRef logName = JSStringCreateWithUTF8CString("__memeConsoleLog");
        JSObjectRef logFn = JSObjectMakeFunctionWithCallback(ctx, logName, &jscConsoleLogCb);
        JSObjectSetProperty(ctx, global, logName, logFn, kJSPropertyAttributeNone, nullptr);
        JSStringRelease(logName);

        JSStringRef hcbName = JSStringCreateWithUTF8CString("__memeInvokeHostCb");
        JSObjectRef hcbFn = JSObjectMakeFunctionWithCallback(ctx, hcbName, &jscInvokeHostCb);
        JSObjectSetProperty(ctx, global, hcbName, hcbFn, kJSPropertyAttributeNone, nullptr);
        JSStringRelease(hcbName);

        const char *bootstrap = R"JS(
var global = globalThis;
var console = {
  log: function() { __memeConsoleLog.apply(null, arguments); },
  info: function() { __memeConsoleLog.apply(null, arguments); },
  warn: function() { __memeConsoleLog.apply(null, arguments); },
  error: function() { __memeConsoleLog.apply(null, arguments); }
};
var process = { env: { NODE_ENV: "production" }, platform: "darwin", versions: { meme: "1.0" } };
function setTimeout(fn) { try { if (typeof fn === "function") fn(); } catch (_) {} return 0; }
function clearTimeout() {}
function require(id) { return __memeHostRequire(String(id), ""); }
)JS";
        JSStringRef bs = JSStringCreateWithUTF8CString(bootstrap);
        JSEvaluateScript(ctx, bs, nullptr, nullptr, 1, nullptr);
        JSStringRelease(bs);
    }

    void shutdownEngine() {
        for (auto &[id, fnObj] : fnRefs) {
            if (ctx && fnObj) JSValueUnprotect(ctx, fnObj);
        }
        fnRefs.clear();
        for (auto &[k, v] : moduleCache) {
            if (ctx && v) JSValueUnprotect(ctx, v);
        }
        moduleCache.clear();
        {
            std::lock_guard<std::mutex> lk(s_sessRegMtx);
            s_sessReg.erase(sessId);
        }
        if (ctx) {
            JSGlobalContextRelease(ctx);
            ctx = nullptr;
        }
        if (group) {
            JSContextGroupRelease(group);
            group = nullptr;
        }
    }

    JSValueRef evalWrappedCjsJsc(const std::string &code, const std::string &filename, const std::string &dir, const std::string &cacheKey, JSValueRef *excOut) {
        if (cacheKey.size() > 5 && code.empty()) return JSValueMakeUndefined(ctx);
        std::string trimmed = detail::trimCopy(code);
        if (filename.size() >= 5 && filename.substr(filename.size() - 5) == ".json") {
            std::string js = "JSON.parse(" + detail::escapeJsStr(trimmed) + ")";
            JSStringRef s = JSStringCreateWithUTF8CString(js.c_str());
            JSStringRef fn = JSStringCreateWithUTF8CString(filename.c_str());
            JSValueRef v = JSEvaluateScript(ctx, s, nullptr, fn, 1, excOut);
            JSStringRelease(s);
            JSStringRelease(fn);
            if (v) { JSValueProtect(ctx, v); moduleCache[cacheKey] = v; }
            return v;
        }

        std::string cjsBody = detail::stripSimpleEsm(code);
        std::ostringstream ws;
        ws << "(function() {\n"
           << "  var module = { exports: {} };\n"
           << "  var exports = module.exports;\n"
           << "  var __filename = " << detail::escapeJsStr(filename) << ";\n"
           << "  var __dirname = " << detail::escapeJsStr(dir) << ";\n"
           << "  var require = function(id) { return __memeHostRequire(String(id), __dirname); };\n"
           << "  require.resolve = function(id) { return String(id); };\n"
           << cjsBody << "\n"
           << "  return module.exports;\n"
           << "})()";
        std::string wrapped = ws.str();
        JSStringRef scriptStr = JSStringCreateWithUTF8CString(wrapped.c_str());
        JSStringRef fileStr = JSStringCreateWithUTF8CString(filename.c_str());
        JSValueRef localExc = nullptr;
        JSValueRef res = JSEvaluateScript(ctx, scriptStr, nullptr, fileStr, 1, &localExc);
        JSStringRelease(scriptStr);
        JSStringRelease(fileStr);
        if (localExc) {
            std::string errMsg = jsValToErrorStr(ctx, localExc);
            if (excOut) *excOut = localExc;
            throw std::runtime_error("JavaScript error in '" + filename + "': " + errMsg);
        }
        if (res) {
            JSValueProtect(ctx, res);
            moduleCache[cacheKey] = res;
        }
        return res;
    }

    JSValueRef loadModuleRawJsc(const std::string &spec, const std::string &callerDir, JSValueRef *excOut) {
        std::filesystem::path cDir = callerDir.empty() ? std::filesystem::current_path() : std::filesystem::path(callerDir);
        auto entry = detail::findNpmPackageEntry(spec, cDir);
        if (!entry.empty()) {
            std::string key = "file:" + entry.string();
            auto cit = moduleCache.find(key);
            if (cit != moduleCache.end()) return cit->second;
            std::string src = detail::readTextFile(entry);
            return evalWrappedCjsJsc(src, entry.string(), entry.parent_path().string(), key, excOut);
        }

        const auto &vmods = detail::builtinVirtualModules();
        auto vit = vmods.find(spec);
        if (vit != vmods.end()) {
            std::string key = "builtin:" + spec;
            auto cit = moduleCache.find(key);
            if (cit != moduleCache.end()) return cit->second;
            return evalWrappedCjsJsc(vit->second, "<builtin:" + spec + ">", cDir.string(), key, excOut);
        }
        throw std::runtime_error("NPM module '" + spec + "' not found locally. Install via './meme tapri install npm:" + spec + "'");
    }

    JsNode loadModuleInternal(const std::string &spec, const std::string &callerDir) {
        JSValueRef exc = nullptr;
        JSValueRef res = loadModuleRawJsc(spec, callerDir, &exc);
        return jscToNode(res);
    }

    JsNode evalScriptInternal(const std::string &code, const std::string &filename) {
        JSStringRef s = JSStringCreateWithUTF8CString(detail::stripSimpleEsm(code).c_str());
        JSStringRef f = JSStringCreateWithUTF8CString(filename.c_str());
        JSValueRef exc = nullptr;
        JSValueRef res = JSEvaluateScript(ctx, s, nullptr, f, 1, &exc);
        JSStringRelease(s);
        JSStringRelease(f);
        if (exc) throw std::runtime_error("JS eval error: " + jsValToErrorStr(ctx, exc));
        return jscToNode(res);
    }

    JsNode callFunctionInternal(uint64_t fnId, const std::vector<JsNode> &args) {
        auto it = fnRefs.find(fnId);
        if (it == fnRefs.end()) throw std::runtime_error("JS function handle expired");
        std::vector<JSValueRef> jargs;
        jargs.reserve(args.size());
        for (const auto &a : args) jargs.push_back(nodeToJsc(a));
        JSValueRef exc = nullptr;
        JSValueRef res = JSObjectCallAsFunction(ctx, it->second, nullptr, jargs.size(), jargs.empty() ? nullptr : jargs.data(), &exc);
        if (exc) throw std::runtime_error("JS call error: " + jsValToErrorStr(ctx, exc));
        return jscToNode(res);
    }

#elif defined(MEME_JS_USE_QUICKJS)
    JSRuntime *rt = nullptr;
    JSContext *ctx = nullptr;
    std::unordered_map<uint64_t, JSValue> fnRefs;
    std::unordered_map<std::string, JSValue> moduleCache;

    static std::string qjsValToStr(JSContext *c, JSValueConst v) {
        const char *cs = JS_ToCString(c, v);
        std::string s = cs ? cs : "";
        if (cs) JS_FreeCString(c, cs);
        return s;
    }

    static std::string qjsExceptionStr(JSContext *c) {
        JSValue exc = JS_GetException(c);
        std::string out = qjsValToStr(c, exc);
        JSValue stack = JS_GetPropertyStr(c, exc, "stack");
        if (!JS_IsUndefined(stack)) {
            std::string st = qjsValToStr(c, stack);
            if (!st.empty()) out += "\n" + st;
        }
        JS_FreeValue(c, stack);
        JS_FreeValue(c, exc);
        return out;
    }

    static JSValue qjsRequireCb(JSContext *c, JSValueConst, int argc, JSValueConst *argv) {
        auto *self = static_cast<JsEngineSession *>(JS_GetContextOpaque(c));
        if (!self || argc < 1) return JS_UNDEFINED;
        std::string reqSpec = qjsValToStr(c, argv[0]);
        std::string baseDir = (argc >= 2 && JS_IsString(argv[1])) ? qjsValToStr(c, argv[1]) : "";
        try {
            JSValue v = self->loadModuleRawQjs(reqSpec, baseDir);
            return JS_DupValue(c, v);
        } catch (const std::exception &e) {
            return JS_ThrowInternalError(c, "%s", e.what());
        }
    }

    static JSValue qjsConsoleLogCb(JSContext *c, JSValueConst, int argc, JSValueConst *argv) {
        auto *self = static_cast<JsEngineSession *>(JS_GetContextOpaque(c));
        std::string line;
        for (int i = 0; i < argc; ++i) {
            if (i > 0) line += " ";
            line += qjsValToStr(c, argv[i]);
        }
        if (self) self->consoleLogs.push_back(line);
        return JS_UNDEFINED;
    }

    static JSValue qjsInvokeHostCb(JSContext *c, JSValueConst, int argc, JSValueConst *argv) {
        auto *self = static_cast<JsEngineSession *>(JS_GetContextOpaque(c));
        if (!self || argc < 1) return JS_UNDEFINED;
        int64_t cbId = 0;
        JS_ToInt64(c, &cbId, argv[0]);
        auto it = self->hostCallbacks.find(static_cast<uint64_t>(cbId));
        if (it == self->hostCallbacks.end()) return JS_UNDEFINED;
        std::vector<JsNode> args;
        if (argc >= 2 && JS_IsArray(argv[1])) {
            JSValue lv = JS_GetPropertyStr(c, argv[1], "length");
            int64_t len = 0;
            JS_ToInt64(c, &len, lv);
            JS_FreeValue(c, lv);
            for (int64_t i = 0; i < len; ++i) {
                JSValue iv = JS_GetPropertyUint32(c, argv[1], static_cast<uint32_t>(i));
                args.push_back(self->qjsToNode(iv));
                JS_FreeValue(c, iv);
            }
        }
        try {
            return self->nodeToQjs(it->second(args));
        } catch (const std::exception &e) {
            return JS_ThrowInternalError(c, "%s", e.what());
        }
    }

    JSValue nodeToQjs(const JsNode &n) {
        switch (n.kind) {
            case JsNode::K_NULL: return JS_NULL;
            case JsNode::K_BOOL: return JS_NewBool(ctx, n.bVal);
            case JsNode::K_INT: return JS_NewInt64(ctx, n.iVal);
            case JsNode::K_DOUBLE: return JS_NewFloat64(ctx, n.dVal);
            case JsNode::K_STRING: return JS_NewStringLen(ctx, n.sVal.data(), n.sVal.size());
            case JsNode::K_ARRAY: {
                JSValue arr = JS_NewArray(ctx);
                for (uint32_t i = 0; i < n.arrVal.size(); ++i) {
                    JS_SetPropertyUint32(ctx, arr, i, nodeToQjs(n.arrVal[i]));
                }
                return arr;
            }
            case JsNode::K_OBJECT: {
                JSValue obj = JS_NewObject(ctx);
                for (const auto &kv : n.objVal) {
                    JS_SetPropertyStr(ctx, obj, kv.first.c_str(), nodeToQjs(kv.second));
                }
                return obj;
            }
            case JsNode::K_FUNCTION: {
                auto it = fnRefs.find(n.fnId);
                return (it != fnRefs.end()) ? JS_DupValue(ctx, it->second) : JS_UNDEFINED;
            }
            case JsNode::K_HOST_FUNCTION: {
                uint64_t cbId = nextHostCbId++;
                hostCallbacks[cbId] = n.hostFn;
                std::string expr = "(function(){ return __memeInvokeHostCb(" + std::to_string(cbId) + ", Array.prototype.slice.call(arguments)); })";
                return JS_Eval(ctx, expr.c_str(), expr.size(), "<host_cb>", JS_EVAL_TYPE_GLOBAL);
            }
        }
        return JS_NULL;
    }

    JsNode qjsToNode(JSValueConst v, int depth = 0) {
        if (JS_IsUndefined(v) || JS_IsNull(v)) return JsNode::makeNull();
        if (JS_IsBool(v)) return JsNode::makeBool(JS_ToBool(ctx, v) != 0);
        if (JS_IsNumber(v)) {
            int64_t iv = 0;
            double dv = 0.0;
            JS_ToFloat64(ctx, &dv, v);
            if (std::isfinite(dv) && std::floor(dv) == dv && dv >= -9007199254740991.0 && dv <= 9007199254740991.0 &&
                JS_ToInt64(ctx, &iv, v) == 0) {
                return JsNode::makeInt(iv);
            }
            return JsNode::makeDouble(dv);
        }
        if (JS_IsString(v)) return JsNode::makeString(qjsValToStr(ctx, v));
        if (depth >= 12 || !JS_IsObject(v)) return JsNode::makeNull();

        auto collectProps = [&]() {
            std::vector<std::pair<std::string, JsNode>> props;
            JSPropertyEnum *tab = nullptr;
            uint32_t len = 0;
            if (JS_GetOwnPropertyNames(ctx, &tab, &len, v, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) == 0) {
                for (uint32_t i = 0; i < len; ++i) {
                    const char *kn = JS_AtomToCString(ctx, tab[i].atom);
                    std::string kStr = kn ? kn : "";
                    if (kn) JS_FreeCString(ctx, kn);
                    JSValue pv = JS_GetProperty(ctx, v, tab[i].atom);
                    props.emplace_back(std::move(kStr), qjsToNode(pv, depth + 1));
                    JS_FreeValue(ctx, pv);
                }
                JS_FreePropertyEnum(ctx, tab, len);
            }
            return props;
        };

        if (JS_IsFunction(ctx, v)) {
            uint64_t id = nextFnId++;
            fnRefs[id] = JS_DupValue(ctx, v);
            return JsNode::makeFunction(id, collectProps());
        }
        if (JS_IsArray(v)) {
            JSValue lv = JS_GetPropertyStr(ctx, v, "length");
            int64_t len = 0;
            JS_ToInt64(ctx, &len, lv);
            JS_FreeValue(ctx, lv);
            std::vector<JsNode> arr;
            for (uint32_t i = 0; i < static_cast<uint32_t>(len) && i < 10000; ++i) {
                JSValue iv = JS_GetPropertyUint32(ctx, v, i);
                arr.push_back(qjsToNode(iv, depth + 1));
                JS_FreeValue(ctx, iv);
            }
            return JsNode::makeArray(std::move(arr));
        }
        return JsNode::makeObject(collectProps());
    }

    void initEngine() {
        rt = JS_NewRuntime();
        ctx = JS_NewContext(rt);
        JS_SetContextOpaque(ctx, this);
        JSValue global = JS_GetGlobalObject(ctx);
        JS_SetPropertyStr(ctx, global, "__memeHostRequire", JS_NewCFunction(ctx, &qjsRequireCb, "__memeHostRequire", 2));
        JS_SetPropertyStr(ctx, global, "__memeConsoleLog", JS_NewCFunction(ctx, &qjsConsoleLogCb, "__memeConsoleLog", 1));
        JS_SetPropertyStr(ctx, global, "__memeInvokeHostCb", JS_NewCFunction(ctx, &qjsInvokeHostCb, "__memeInvokeHostCb", 2));
        JS_FreeValue(ctx, global);

        const char *bootstrap = R"JS(
var global = globalThis;
var console = {
  log: function() { __memeConsoleLog.apply(null, arguments); },
  info: function() { __memeConsoleLog.apply(null, arguments); },
  warn: function() { __memeConsoleLog.apply(null, arguments); },
  error: function() { __memeConsoleLog.apply(null, arguments); }
};
var process = { env: { NODE_ENV: "production" }, platform: "native", versions: { meme: "1.0" } };
function setTimeout(fn) { try { if (typeof fn === "function") fn(); } catch (_) {} return 0; }
function clearTimeout() {}
function require(id) { return __memeHostRequire(String(id), ""); }
)JS";
        JSValue r = JS_Eval(ctx, bootstrap, strlen(bootstrap), "<bootstrap>", JS_EVAL_TYPE_GLOBAL);
        JS_FreeValue(ctx, r);
    }

    void shutdownEngine() {
        if (ctx) {
            for (auto &[id, v] : fnRefs) JS_FreeValue(ctx, v);
            for (auto &[k, v] : moduleCache) JS_FreeValue(ctx, v);
            fnRefs.clear();
            moduleCache.clear();
            JS_FreeContext(ctx);
            ctx = nullptr;
        }
        if (rt) {
            JS_FreeRuntime(rt);
            rt = nullptr;
        }
    }

    JSValue evalWrappedCjsQjs(const std::string &code, const std::string &filename, const std::string &dir, const std::string &cacheKey) {
        std::string trimmed = detail::trimCopy(code);
        if (filename.size() >= 5 && filename.substr(filename.size() - 5) == ".json") {
            std::string js = "JSON.parse(" + detail::escapeJsStr(trimmed) + ")";
            JSValue v = JS_Eval(ctx, js.c_str(), js.size(), filename.c_str(), JS_EVAL_TYPE_GLOBAL);
            if (JS_IsException(v)) throw std::runtime_error("JSON parse error: " + qjsExceptionStr(ctx));
            moduleCache[cacheKey] = v;
            return v;
        }
        std::string cjsBody = detail::stripSimpleEsm(code);
        std::ostringstream ws;
        ws << "(function() {\n"
           << "  var module = { exports: {} };\n"
           << "  var exports = module.exports;\n"
           << "  var __filename = " << detail::escapeJsStr(filename) << ";\n"
           << "  var __dirname = " << detail::escapeJsStr(dir) << ";\n"
           << "  var require = function(id) { return __memeHostRequire(String(id), __dirname); };\n"
           << "  require.resolve = function(id) { return String(id); };\n"
           << cjsBody << "\n"
           << "  return module.exports;\n"
           << "})()";
        std::string wrapped = ws.str();
        JSValue res = JS_Eval(ctx, wrapped.c_str(), wrapped.size(), filename.c_str(), JS_EVAL_TYPE_GLOBAL);
        if (JS_IsException(res)) {
            std::string err = qjsExceptionStr(ctx);
            JS_FreeValue(ctx, res);
            throw std::runtime_error("QuickJS error in '" + filename + "': " + err);
        }
        moduleCache[cacheKey] = res;
        return res;
    }

    JSValue loadModuleRawQjs(const std::string &spec, const std::string &callerDir) {
        std::filesystem::path cDir = callerDir.empty() ? std::filesystem::current_path() : std::filesystem::path(callerDir);
        auto entry = detail::findNpmPackageEntry(spec, cDir);
        if (!entry.empty()) {
            std::string key = "file:" + entry.string();
            auto cit = moduleCache.find(key);
            if (cit != moduleCache.end()) return cit->second;
            std::string src = detail::readTextFile(entry);
            return evalWrappedCjsQjs(src, entry.string(), entry.parent_path().string(), key);
        }
        const auto &vmods = detail::builtinVirtualModules();
        auto vit = vmods.find(spec);
        if (vit != vmods.end()) {
            std::string key = "builtin:" + spec;
            auto cit = moduleCache.find(key);
            if (cit != moduleCache.end()) return cit->second;
            return evalWrappedCjsQjs(vit->second, "<builtin:" + spec + ">", cDir.string(), key);
        }
        throw std::runtime_error("NPM module '" + spec + "' not found locally. Install via './meme tapri install npm:" + spec + "'");
    }

    JsNode loadModuleInternal(const std::string &spec, const std::string &callerDir) {
        return qjsToNode(loadModuleRawQjs(spec, callerDir));
    }

    JsNode evalScriptInternal(const std::string &code, const std::string &filename) {
        std::string s = detail::stripSimpleEsm(code);
        JSValue res = JS_Eval(ctx, s.c_str(), s.size(), filename.c_str(), JS_EVAL_TYPE_GLOBAL);
        if (JS_IsException(res)) {
            std::string err = qjsExceptionStr(ctx);
            JS_FreeValue(ctx, res);
            throw std::runtime_error("QuickJS eval error: " + err);
        }
        JsNode n = qjsToNode(res);
        JS_FreeValue(ctx, res);
        return n;
    }

    JsNode callFunctionInternal(uint64_t fnId, const std::vector<JsNode> &args) {
        auto it = fnRefs.find(fnId);
        if (it == fnRefs.end()) throw std::runtime_error("QuickJS function handle expired");
        std::vector<JSValue> qargs;
        qargs.reserve(args.size());
        for (const auto &a : args) qargs.push_back(nodeToQjs(a));
        JSValue res = JS_Call(ctx, it->second, JS_UNDEFINED, static_cast<int>(qargs.size()), qargs.empty() ? nullptr : qargs.data());
        for (auto &a : qargs) JS_FreeValue(ctx, a);
        if (JS_IsException(res)) {
            std::string err = qjsExceptionStr(ctx);
            JS_FreeValue(ctx, res);
            throw std::runtime_error("QuickJS call error: " + err);
        }
        JsNode out = qjsToNode(res);
        JS_FreeValue(ctx, res);
        return out;
    }
#else
    void initEngine() {}
    void shutdownEngine() {}
    JsNode loadModuleInternal(const std::string &spec, const std::string &) {
        std::vector<std::pair<std::string, JsNode>> obj;
        obj.emplace_back("__bridge", JsNode::makeString(spec));
        return JsNode::makeObject(std::move(obj));
    }
    JsNode evalScriptInternal(const std::string &, const std::string &) { return JsNode::makeNull(); }
    JsNode callFunctionInternal(uint64_t, const std::vector<JsNode> &) { return JsNode::makeNull(); }
#endif
};

inline std::shared_ptr<JsEngineSession> &sharedSession() {
    static std::shared_ptr<JsEngineSession> s = std::make_shared<JsEngineSession>();
    return s;
}

inline std::string deriveModuleSymbolAlias(const std::string &pkg) {
    std::string tail = pkg;
    size_t sl = tail.rfind('/');
    if (sl != std::string::npos) tail = tail.substr(sl + 1);
    std::string out;
    bool upper = false;
    for (unsigned char c : tail) {
        if (c == '-' || c == '_' || c == '.') {
            upper = true;
        } else {
            out.push_back(upper ? static_cast<char>(std::toupper(c)) : static_cast<char>(c));
            upper = false;
        }
    }
    return out;
}

} // namespace MemeJs
