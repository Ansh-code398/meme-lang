#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#elif defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <windows.h>
#ifdef RELATIVE
#undef RELATIVE
#endif
#ifdef ABSOLUTE
#undef ABSOLUTE
#endif
#ifdef ERROR
#undef ERROR
#endif
#ifdef DELETE
#undef DELETE
#endif
#ifdef TRANSPARENT
#undef TRANSPARENT
#endif
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

#include "MemePlatform.h"
#include "MemePermissions.h"

namespace MemeNative {

struct DeviceInfo {
    std::string os = "unknown";
    std::string arch = "unknown";
    std::string model = "unknown";
    std::string osVersion = "";
    std::string locale = "en-US";
    std::string timezone = "UTC";
    std::string theme = "dark";
    bool isMobile = false;
    bool isDesktop = true;
    bool isWeb = false;
    bool isDark = true;
};

struct ScreenInfo {
    float width = 1024.0f;
    float height = 768.0f;
    float scale = 1.0f;
    float safeTop = 0.0f;
    float safeBottom = 0.0f;
    float safeLeft = 0.0f;
    float safeRight = 0.0f;
};

struct BatteryInfo {
    double level = -1.0;
    bool charging = false;
    std::string state = "unknown";
    bool supported = false;
};

struct NetworkInfo {
    bool online = true;
    std::string type = "unknown";
};

struct PathsInfo {
    std::string appData;
    std::string cache;
    std::string documents;
    std::string temp;
};

struct SensorSnapshot {
    double accelX = 0.0;
    double accelY = 0.0;
    double accelZ = 0.0;
    double gyroX = 0.0;
    double gyroY = 0.0;
    double gyroZ = 0.0;
    bool available = false;
};

struct AsyncResponse {
    bool ok = true;
    bool accepted = true;
    bool cancelled = false;
    std::string value = "";
    std::string path = "";
    std::vector<std::string> paths;
    double lat = 0.0;
    double lon = 0.0;
    double accuracy = 0.0;
    std::string permissionState = "";
    bool canAskAgain = true;
    bool limited = false;
    bool restricted = false;
    std::string error = "";
};

using AsyncCallback = std::function<void(const AsyncResponse &)>;

struct HostHooks {
    std::function<std::string()> clipboardGet;
    std::function<bool(const std::string &)> clipboardSet;

    std::function<void(const std::string &title, const std::string &msg, AsyncCallback cb)> alertAsync;
    std::function<void(const std::string &title, const std::string &msg, AsyncCallback cb)> confirmAsync;
    std::function<void(const std::string &title, const std::string &msg, const std::string &defVal, AsyncCallback cb)> promptAsync;
    std::function<void(const std::string &mode, const std::string &title, const std::string &defaultName, AsyncCallback cb)> fileDialogAsync;

    std::function<bool(const std::string &style)> hapticImpact;
    std::function<bool(const std::string &type)> hapticNotify;
    std::function<bool()> hapticSelection;
    std::function<bool(int ms)> hapticVibrate;

    std::function<void(AsyncCallback cb)> notifyRequestPermissionAsync;
    std::function<bool(const std::string &title, const std::string &body)> notifySend;
    std::function<bool(int count)> setBadgeCount;

    std::function<MemePermissions::PermissionStatus(const std::string &cap)> checkPermission;
    std::function<void(const std::string &cap, AsyncCallback cb)> requestPermissionAsync;
    std::function<bool(const std::string &cap)> openAppSettings;

    std::function<bool(const std::string &url)> canOpenUrl;
    std::function<bool(const std::string &url)> openUrl;
    std::function<void(const std::string &text, const std::string &title, const std::string &url, AsyncCallback cb)> shareAsync;

    std::function<DeviceInfo()> getDeviceInfo;
    std::function<ScreenInfo()> getScreenInfo;
    std::function<BatteryInfo()> getBatteryInfo;
    std::function<NetworkInfo()> getNetworkInfo;
    std::function<PathsInfo()> getAppPaths;
    std::function<SensorSnapshot()> getSensorSnapshot;

    std::function<std::string(const std::string &service, const std::string &key)> keychainGet;
    std::function<bool(const std::string &service, const std::string &key, const std::string &val)> keychainSet;
    std::function<bool(const std::string &service, const std::string &key)> keychainRemove;

    std::function<bool()> biometricsAvailable;
    std::function<void(const std::string &reason, AsyncCallback cb)> biometricsAuthAsync;
    std::function<void(AsyncCallback cb)> getLocationAsync;

    std::function<bool(double freqHz, int durationMs)> beep;
    std::function<bool(const std::string &text, double rate)> speak;
    std::function<bool()> stopSpeaking;
    std::function<bool(const std::string &pathOrId)> playSound;

    std::function<bool(const std::string &title)> setWindowTitle;
    std::function<bool(int w, int h)> setWindowSize;
    std::function<bool(bool on)> setFullscreen;
    std::function<bool()> minimizeWindow;
    std::function<bool()> focusWindow;
    std::function<bool()> hideKeyboard;

    std::function<void(std::function<void()>)> dispatchMainThread;
    std::function<void()> requestRerender;
};

struct NotificationRecord {
    std::string title;
    std::string body;
};

struct RuntimeState {
    std::mutex mu;
    std::mutex storageMu;
    std::string appId = "com.memelang.app";
    std::string appName = "MEME App";
    std::string windowTitle = "MEME App";
    std::string clipboardFallback = "";
    int badgeCount = 0;
    bool notifyGranted = true;
    std::vector<NotificationRecord> notifications;
    std::unordered_map<int, AsyncCallback> pendingAsync;
    int nextAsyncId = 1;
    int iconWidth = 0;
    int iconHeight = 0;
    std::vector<uint8_t> iconRgba;
};

inline HostHooks &hooks() {
    static HostHooks h;
    return h;
}

inline RuntimeState &state() {
    static RuntimeState s;
    return s;
}

inline std::vector<uint8_t> buildDefaultSquircleIconRgba(int size) {
    if (size < 8) size = 8;
    std::vector<uint8_t> rgba(static_cast<size_t>(size) * static_cast<size_t>(size) * 4u, 0);
    const float half = static_cast<float>(size) * 0.5f;
    const float radius = half * 0.88f;
    auto evalLightning = [](float nx, float ny) -> float {
        // Normalized [-1, 1] stylized lightning bolt silhouette inside the squircle badge
        if (ny < -0.52f || ny > 0.56f) return 0.0f;
        float xL = 0.0f, xR = 0.0f;
        if (ny < 0.02f) {
            float t = (ny + 0.52f) / 0.54f;
            xL = 0.08f - 0.34f * t;
            xR = 0.24f - 0.14f * t;
        } else {
            float t = (ny - 0.02f) / 0.54f;
            xL = -0.02f - 0.10f * t;
            xR = 0.28f - 0.38f * t;
        }
        float d = std::min(nx - xL, xR - nx);
        return std::clamp(d * 18.0f, 0.0f, 1.0f);
    };
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float px = (static_cast<float>(x) + 0.5f - half);
            float py = (static_cast<float>(y) + 0.5f - half);
            float ax = std::abs(px) / radius;
            float ay = std::abs(py) / radius;
            // Superellipse exponent n=4 squircle SDF
            float sdf = std::pow(ax * ax * ax * ax + ay * ay * ay * ay, 0.25f) - 1.0f;
            float cov = std::clamp(0.5f - sdf * (radius * 0.45f), 0.0f, 1.0f);
            if (cov <= 0.001f) continue;

            float gx = static_cast<float>(x) / static_cast<float>(size);
            float gy = static_cast<float>(y) / static_cast<float>(size);
            float mixT = std::clamp(0.55f * gx + 0.45f * gy, 0.0f, 1.0f);
            // Gradient indigo (#4f46e5) -> violet (#6366f1) -> emerald accent (#10b981)
            float r = (1.0f - mixT) * 79.0f + mixT * 16.0f;
            float g = (1.0f - mixT) * 70.0f + mixT * 185.0f;
            float b = (1.0f - mixT) * 229.0f + mixT * 129.0f;
            if (sdf > -0.07f) {
                r = r * 0.72f + 224.0f * 0.28f;
                g = g * 0.72f + 231.0f * 0.28f;
                b = b * 0.72f + 255.0f * 0.28f;
            }
            float bolt = evalLightning(px / radius, py / radius);
            if (bolt > 0.0f) {
                r = r * (1.0f - bolt) + 250.0f * bolt;
                g = g * (1.0f - bolt) + 252.0f * bolt;
                b = b * (1.0f - bolt) + 245.0f * bolt;
            }
            size_t off = (static_cast<size_t>(y) * static_cast<size_t>(size) + static_cast<size_t>(x)) * 4u;
            rgba[off + 0] = static_cast<uint8_t>(std::clamp(r, 0.0f, 255.0f));
            rgba[off + 1] = static_cast<uint8_t>(std::clamp(g, 0.0f, 255.0f));
            rgba[off + 2] = static_cast<uint8_t>(std::clamp(b, 0.0f, 255.0f));
            rgba[off + 3] = static_cast<uint8_t>(std::clamp(cov * 255.0f, 0.0f, 255.0f));
        }
    }
    return rgba;
}

inline void setAppIconRgba(const uint8_t *rgba, int w, int h) {
    if (!rgba || w <= 0 || h <= 0) return;
    std::lock_guard<std::mutex> lock(state().mu);
    state().iconWidth = w;
    state().iconHeight = h;
    state().iconRgba.assign(rgba, rgba + static_cast<size_t>(w) * static_cast<size_t>(h) * 4u);
}

inline void ensureDefaultAppIconRgba(int size, std::vector<uint8_t> &outRgba, int &outW, int &outH) {
    std::lock_guard<std::mutex> lock(state().mu);
    if (!state().iconRgba.empty() && state().iconWidth > 0 && state().iconHeight > 0) {
        outRgba = state().iconRgba;
        outW = state().iconWidth;
        outH = state().iconHeight;
        return;
    }
    outRgba = buildDefaultSquircleIconRgba(size);
    outW = size;
    outH = size;
}

inline void setAppIdentity(const std::string &id, const std::string &name) {
    std::lock_guard<std::mutex> lock(state().mu);
    if (!id.empty()) state().appId = id;
    if (!name.empty()) {
        state().appName = name;
        state().windowTitle = name;
    }
}

inline int registerAsyncCallback(AsyncCallback cb) {
    std::lock_guard<std::mutex> lock(state().mu);
    int id = state().nextAsyncId++;
    state().pendingAsync[id] = std::move(cb);
    return id;
}

inline void dispatchCallbackOnUiThread(AsyncCallback cb, const AsyncResponse &res) {
    if (!cb) return;
    auto disp = hooks().dispatchMainThread;
    if (disp) {
        disp([cb, res]() {
            cb(res);
            if (hooks().requestRerender) hooks().requestRerender();
        });
    } else {
        cb(res);
        if (hooks().requestRerender) hooks().requestRerender();
    }
}

inline void resolveAsyncCallback(int id, const AsyncResponse &res) {
    AsyncCallback cb;
    {
        std::lock_guard<std::mutex> lock(state().mu);
        auto it = state().pendingAsync.find(id);
        if (it == state().pendingAsync.end()) return;
        cb = std::move(it->second);
        state().pendingAsync.erase(it);
    }
    dispatchCallbackOnUiThread(cb, res);
}

namespace detail {

inline std::string jsonEscape(const std::string &s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (unsigned char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned>(c));
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out;
}

inline std::map<std::string, std::string> parseFlatJsonMap(const std::string &json) {
    std::map<std::string, std::string> out;
    size_t i = 0;
    const size_t n = json.size();
    auto skipWs = [&]() {
        while (i < n && (json[i] == ' ' || json[i] == '\n' || json[i] == '\r' || json[i] == '\t' || json[i] == ',' || json[i] == '{' || json[i] == '}')) {
            ++i;
        }
    };
    auto readStr = [&]() -> std::string {
        if (i >= n || json[i] != '"') return "";
        ++i;
        std::string s;
        while (i < n && json[i] != '"') {
            if (json[i] == '\\' && i + 1 < n) {
                ++i;
                switch (json[i]) {
                    case '"': s += '"'; break;
                    case '\\': s += '\\'; break;
                    case 'n': s += '\n'; break;
                    case 'r': s += '\r'; break;
                    case 't': s += '\t'; break;
                    default: s += json[i]; break;
                }
            } else {
                s += json[i];
            }
            ++i;
        }
        if (i < n && json[i] == '"') ++i;
        return s;
    };
    while (i < n) {
        skipWs();
        if (i >= n) break;
        if (json[i] != '"') { ++i; continue; }
        std::string k = readStr();
        while (i < n && (json[i] == ' ' || json[i] == ':')) ++i;
        if (i < n && json[i] == '"') {
            out[k] = readStr();
        }
    }
    return out;
}

inline std::string serializeFlatJsonMap(const std::map<std::string, std::string> &m) {
    std::string out = "{\n";
    size_t idx = 0;
    for (const auto &kv : m) {
        out += "  \"" + jsonEscape(kv.first) + "\": \"" + jsonEscape(kv.second) + "\"";
        out += (++idx < m.size()) ? ",\n" : "\n";
    }
    out += "}\n";
    return out;
}

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__) && !defined(__ANDROID__) && !(defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
inline bool execArgv(const std::vector<std::string> &argv, const std::string *stdinData, std::string *stdoutOut, int *exitCodeOut = nullptr, bool detached = false) {
    if (argv.empty()) return false;
    int inPipe[2] = {-1, -1};
    int outPipe[2] = {-1, -1};
    if (stdinData && ::pipe(inPipe) != 0) return false;
    if (stdoutOut && ::pipe(outPipe) != 0) {
        if (inPipe[0] >= 0) { ::close(inPipe[0]); ::close(inPipe[1]); }
        return false;
    }

    pid_t pid = ::fork();
    if (pid < 0) {
        if (inPipe[0] >= 0) { ::close(inPipe[0]); ::close(inPipe[1]); }
        if (outPipe[0] >= 0) { ::close(outPipe[0]); ::close(outPipe[1]); }
        return false;
    }
    if (pid == 0) {
        if (stdinData) {
            ::dup2(inPipe[0], STDIN_FILENO);
            ::close(inPipe[0]);
            ::close(inPipe[1]);
        }
        if (stdoutOut) {
            ::dup2(outPipe[1], STDOUT_FILENO);
            ::close(outPipe[0]);
            ::close(outPipe[1]);
        }
        if (detached && !stdoutOut) {
            ::setsid();
        }
        std::vector<char *> cargs;
        cargs.reserve(argv.size() + 1);
        for (const auto &a : argv) cargs.push_back(const_cast<char *>(a.c_str()));
        cargs.push_back(nullptr);
        ::execvp(cargs[0], cargs.data());
        ::_exit(127);
    }

    if (inPipe[0] >= 0) ::close(inPipe[0]);
    if (outPipe[1] >= 0) ::close(outPipe[1]);

    if (stdinData && inPipe[1] >= 0) {
        size_t off = 0;
        while (off < stdinData->size()) {
            ssize_t w = ::write(inPipe[1], stdinData->data() + off, stdinData->size() - off);
            if (w <= 0) break;
            off += static_cast<size_t>(w);
        }
        ::close(inPipe[1]);
    }
    if (stdoutOut && outPipe[0] >= 0) {
        stdoutOut->clear();
        char buf[1024];
        ssize_t r = 0;
        while ((r = ::read(outPipe[0], buf, sizeof(buf))) > 0) {
            stdoutOut->append(buf, static_cast<size_t>(r));
        }
        ::close(outPipe[0]);
    }
    if (detached && !stdoutOut) {
        return true;
    }
    int status = 0;
    ::waitpid(pid, &status, 0);
    int code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    if (exitCodeOut) *exitCodeOut = code;
    return code == 0;
}
#endif

#if defined(__EMSCRIPTEN__)
inline char *js_alloc_utf8(const char *s) { (void)s; return nullptr; }
inline char *js_clip_get() {
    return reinterpret_cast<char *>(EM_ASM_PTR({
        return (function() {
            var v = "";
            try {
                var w = (typeof window !== "undefined") ? window : globalThis;
                v = (w.__memeClipboard || "");
                if (!v && w.__memeStorageClip) v = w.__memeStorageClip;
            } catch (e) {}
            var len = lengthBytesUTF8(v) + 1;
            var p = _malloc(len);
            stringToUTF8(v, p, len);
            return p;
        })();
    }));
}
inline void js_clip_set(const char *ptr) {
    EM_ASM({
        (function(p) {
            var s = UTF8ToString(p);
            try {
                var w = (typeof window !== "undefined") ? window : globalThis;
                w.__memeClipboard = s;
                if (typeof navigator !== "undefined" && navigator.clipboard && navigator.clipboard.writeText) {
                    navigator.clipboard.writeText(s).catch(function() {});
                }
            } catch (e) {}
        })($0);
    }, ptr ? ptr : "");
}
inline void js_open_url(const char *ptr) {
    EM_ASM({
        (function(p) {
            try {
                if (typeof window !== "undefined") window.open(UTF8ToString(p), "_blank", "noopener,noreferrer");
            } catch (e) {}
        })($0);
    }, ptr ? ptr : "");
}
inline void js_vibrate(int ms) {
    EM_ASM({
        try { if (typeof navigator !== "undefined" && navigator.vibrate) navigator.vibrate($0); } catch (e) {}
    }, ms);
}
inline void js_set_title(const char *ptr) {
    EM_ASM({
        try { if (typeof document !== "undefined") document.title = UTF8ToString($0); } catch (e) {}
    }, ptr ? ptr : "");
}
inline int js_is_dark() {
    return EM_ASM_INT({
        try {
            return (typeof window !== "undefined" && window.matchMedia &&
                    window.matchMedia("(prefers-color-scheme: dark)").matches) ? 1 : 0;
        } catch (e) { return 1; }
    });
}
inline int js_is_online() {
    return EM_ASM_INT({
        try { return (typeof navigator !== "undefined" && navigator.onLine === false) ? 0 : 1; } catch (e) { return 1; }
    });
}
inline char *js_locale() {
    return reinterpret_cast<char *>(EM_ASM_PTR({
        return (function() {
            var v = "en-US";
            try { if (typeof navigator !== "undefined" && navigator.language) v = navigator.language; } catch (e) {}
            var len = lengthBytesUTF8(v) + 1;
            var p = _malloc(len);
            stringToUTF8(v, p, len);
            return p;
        })();
    }));
}
inline char *js_timezone() {
    return reinterpret_cast<char *>(EM_ASM_PTR({
        return (function() {
            var v = "UTC";
            try { v = Intl.DateTimeFormat().resolvedOptions().timeZone || "UTC"; } catch (e) {}
            var len = lengthBytesUTF8(v) + 1;
            var p = _malloc(len);
            stringToUTF8(v, p, len);
            return p;
        })();
    }));
}
inline char *js_storage_get(const char *kPtr) {
    return reinterpret_cast<char *>(EM_ASM_PTR({
        return (function(kp) {
            var k = "meme_kv_" + UTF8ToString(kp);
            var v = "";
            var found = false;
            try {
                if (typeof window !== "undefined" && window.localStorage) {
                    var ls = window.localStorage.getItem(k);
                    if (ls !== null && ls !== undefined) { v = ls; found = true; }
                }
            } catch (e) {}
            if (!found) {
                try {
                    var fb = globalThis.__memeKvFallback;
                    if (fb && Object.prototype.hasOwnProperty.call(fb, k)) v = fb[k];
                } catch (e) {}
            }
            var len = lengthBytesUTF8(v) + 1;
            var p = _malloc(len);
            stringToUTF8(v, p, len);
            return p;
        })($0);
    }, kPtr ? kPtr : ""));
}
inline void js_storage_set(const char *kPtr, const char *vPtr) {
    EM_ASM({
        (function(kp, vp) {
            var k = "meme_kv_" + UTF8ToString(kp);
            var v = UTF8ToString(vp);
            try {
                if (!globalThis.__memeKvFallback) globalThis.__memeKvFallback = {};
                globalThis.__memeKvFallback[k] = v;
            } catch (e) {}
            try {
                if (typeof window !== "undefined" && window.localStorage) {
                    window.localStorage.setItem(k, v);
                }
            } catch (e) {}
        })($0, $1);
    }, kPtr ? kPtr : "", vPtr ? vPtr : "");
}
inline void js_storage_remove(const char *kPtr) {
    EM_ASM({
        (function(kp) {
            var k = "meme_kv_" + UTF8ToString(kp);
            try {
                if (globalThis.__memeKvFallback) delete globalThis.__memeKvFallback[k];
            } catch (e) {}
            try {
                if (typeof window !== "undefined" && window.localStorage) {
                    window.localStorage.removeItem(k);
                }
            } catch (e) {}
        })($0);
    }, kPtr ? kPtr : "");
}
inline char *js_storage_keys() {
    return reinterpret_cast<char *>(EM_ASM_PTR({
        return (function() {
            var ks = [];
            var seen = {};
            try {
                if (typeof window !== "undefined" && window.localStorage) {
                    for (var i = 0; i < window.localStorage.length; i++) {
                        var k = window.localStorage.key(i);
                        if (k && k.indexOf("meme_kv_") === 0 && !seen[k]) {
                            seen[k] = true;
                            ks.push(k.slice(8));
                        }
                    }
                }
            } catch (e) {}
            try {
                var fb = globalThis.__memeKvFallback;
                if (fb) {
                    for (var k2 in fb) {
                        if (Object.prototype.hasOwnProperty.call(fb, k2) && k2.indexOf("meme_kv_") === 0 && !seen[k2]) {
                            seen[k2] = true;
                            ks.push(k2.slice(8));
                        }
                    }
                }
            } catch (e) {}
            var s = ks.join("\n");
            var len = lengthBytesUTF8(s) + 1;
            var p = _malloc(len);
            stringToUTF8(s, p, len);
            return p;
        })();
    }));
}
inline void js_beep(double freq, int durMs) {
    EM_ASM({
        (function(freqHz, ms) {
            try {
                var w = (typeof window !== "undefined") ? window : null;
                if (!w) return;
                var Ctx = w.AudioContext || w.webkitAudioContext;
                if (!Ctx) return;
                if (!w.__memeAudioCtx) w.__memeAudioCtx = new Ctx();
                var ctx = w.__memeAudioCtx;
                var osc = ctx.createOscillator();
                var gain = ctx.createGain();
                osc.frequency.value = freqHz > 0 ? freqHz : 440;
                gain.gain.value = 0.08;
                osc.connect(gain);
                gain.connect(ctx.destination);
                osc.start();
                setTimeout(function() { try { osc.stop(); } catch (e) {} }, Math.max(20, ms));
            } catch (e) {}
        })($0, $1);
    }, freq, durMs);
}
inline void js_speak(const char *tPtr, double rate) {
    EM_ASM({
        (function(tp, r) {
            try {
                if (typeof window === "undefined" || !window.speechSynthesis || typeof SpeechSynthesisUtterance === "undefined") return;
                var u = new SpeechSynthesisUtterance(UTF8ToString(tp));
                if (r > 0) u.rate = r;
                window.speechSynthesis.speak(u);
            } catch (e) {}
        })($0, $1);
    }, tPtr ? tPtr : "", rate);
}
inline void js_stop_speaking() {
    EM_ASM({
        try { if (typeof window !== "undefined" && window.speechSynthesis) window.speechSynthesis.cancel(); } catch (e) {}
    });
}
inline void js_notify(const char *titlePtr, const char *bodyPtr) {
    EM_ASM({
        (function(tp, bp) {
            try {
                if (typeof Notification === "undefined") return;
                var t = UTF8ToString(tp);
                var b = UTF8ToString(bp);
                if (Notification.permission === "granted") {
                    new Notification(t, { body: b });
                } else if (Notification.permission === "default" && Notification.requestPermission) {
                    Notification.requestPermission().then(function(st) {
                        if (st === "granted") new Notification(t, { body: b });
                    }).catch(function() {});
                }
            } catch (e) {}
        })($0, $1);
    }, titlePtr ? titlePtr : "", bodyPtr ? bodyPtr : "");
}
inline int js_alert(const char *titlePtr, const char *msgPtr) {
    return EM_ASM_INT({
        return (function(tp, mp) {
            try {
                if (typeof window !== "undefined" && window.alert) window.alert(UTF8ToString(tp) + "\n\n" + UTF8ToString(mp));
                return 1;
            } catch (e) { return 1; }
        })($0, $1);
    }, titlePtr ? titlePtr : "", msgPtr ? msgPtr : "");
}
inline int js_confirm(const char *titlePtr, const char *msgPtr) {
    return EM_ASM_INT({
        return (function(tp, mp) {
            try {
                if (typeof window !== "undefined" && window.confirm) {
                    return window.confirm(UTF8ToString(tp) + "\n\n" + UTF8ToString(mp)) ? 1 : 0;
                }
                return 0;
            } catch (e) { return 0; }
        })($0, $1);
    }, titlePtr ? titlePtr : "", msgPtr ? msgPtr : "");
}
inline char *js_prompt(const char *titlePtr, const char *msgPtr, const char *defPtr) {
    return reinterpret_cast<char *>(EM_ASM_PTR({
        return (function(tp, mp, dp) {
            var res = null;
            try {
                if (typeof window !== "undefined" && window.prompt) {
                    res = window.prompt(UTF8ToString(tp) + ": " + UTF8ToString(mp), UTF8ToString(dp));
                }
            } catch (e) {}
            if (res === null) return 0;
            var len = lengthBytesUTF8(res) + 1;
            var p = _malloc(len);
            stringToUTF8(res, p, len);
            return p;
        })($0, $1, $2);
    }, titlePtr ? titlePtr : "", msgPtr ? msgPtr : "", defPtr ? defPtr : ""));
}
inline void js_fullscreen(int on) {
    EM_ASM({
        (function(enable) {
            try {
                if (typeof document === "undefined") return;
                if (enable && document.documentElement.requestFullscreen) document.documentElement.requestFullscreen();
                else if (!enable && document.exitFullscreen && document.fullscreenElement) document.exitFullscreen();
            } catch (e) {}
        })($0);
    }, on);
}
inline void js_share(const char *titlePtr, const char *textPtr, const char *urlPtr) {
    EM_ASM({
        (function(tp, xp, up) {
            try {
                if (typeof navigator !== "undefined" && navigator.share) {
                    navigator.share({ title: UTF8ToString(tp), text: UTF8ToString(xp), url: UTF8ToString(up) }).catch(function() {});
                }
            } catch (e) {}
        })($0, $1, $2);
    }, titlePtr ? titlePtr : "", textPtr ? textPtr : "", urlPtr ? urlPtr : "");
}
inline void js_init_web_probes() {
    EM_ASM({
        (function() {
            try {
                if (typeof window === "undefined") return;
                if (!window.__memeProbesInit) {
                    window.__memeProbesInit = true;
                    window.__memeBattery = { supported: false, level: -1, charging: false };
                    window.__memeSensors = { available: false, ax: 0, ay: 0, az: 0, gx: 0, gy: 0, gz: 0 };
                    if (typeof navigator !== "undefined" && navigator.getBattery) {
                        navigator.getBattery().then(function(b) {
                            var upd = function() {
                                window.__memeBattery = { supported: true, level: Math.round((b.level || 0) * 100), charging: !!b.charging };
                            };
                            upd();
                            b.addEventListener("levelchange", upd);
                            b.addEventListener("chargingchange", upd);
                        }).catch(function() {});
                    }
                    window.addEventListener("devicemotion", function(ev) {
                        var a = ev.accelerationIncludingGravity || ev.acceleration;
                        var r = ev.rotationRate;
                        window.__memeSensors = {
                            available: true,
                            ax: a && a.x ? a.x : 0, ay: a && a.y ? a.y : 0, az: a && a.z ? a.z : 0,
                            gx: r && r.alpha ? r.alpha : 0, gy: r && r.beta ? r.beta : 0, gz: r && r.gamma ? r.gamma : 0
                        };
                    });
                }
            } catch (e) {}
        })();
    });
}
inline double js_battery_level() {
    return EM_ASM_DOUBLE({
        try { return (typeof window !== "undefined" && window.__memeBattery && window.__memeBattery.supported) ? window.__memeBattery.level : -1; } catch (e) { return -1; }
    });
}
inline int js_battery_charging() {
    return EM_ASM_INT({
        try { return (typeof window !== "undefined" && window.__memeBattery && window.__memeBattery.charging) ? 1 : 0; } catch (e) { return 0; }
    });
}
inline int js_sensor_available() {
    return EM_ASM_INT({
        try { return (typeof window !== "undefined" && window.__memeSensors && window.__memeSensors.available) ? 1 : 0; } catch (e) { return 0; }
    });
}
inline double js_sensor_axis(int idx) {
    return EM_ASM_DOUBLE({
        return (function(i) {
            try {
                var s = (typeof window !== "undefined" && window.__memeSensors) ? window.__memeSensors : {};
                if (i === 0) return s.ax || 0;
                if (i === 1) return s.ay || 0;
                if (i === 2) return s.az || 0;
                if (i === 3) return s.gx || 0;
                if (i === 4) return s.gy || 0;
                if (i === 5) return s.gz || 0;
            } catch (e) {}
            return 0;
        })($0);
    }, idx);
}
inline void js_notify_perm_async(int reqId) {
    EM_ASM({
        (function(rid) {
            var completeFn = (typeof _meme_native_complete_async === "function")
                ? _meme_native_complete_async
                : ((typeof Module !== "undefined" && typeof Module._meme_native_complete_async === "function")
                    ? Module._meme_native_complete_async : null);
            var done = function(ok) {
                try { if (completeFn) completeFn(rid, ok ? 1 : 0, 0, 0, 0, 0, 0); } catch (e) {}
            };
            try {
                if (typeof Notification !== "undefined" && Notification.requestPermission) {
                    Notification.requestPermission().then(function(st) { done(st === "granted"); }).catch(function() { done(false); });
                    return;
                }
            } catch (e) {}
            done(false);
        })($0);
    }, reqId);
}
inline void js_pick_file_async(int reqId, int acceptImage) {
    EM_ASM({
        (function(rid, imgOnly) {
            var completeFn = (typeof _meme_native_complete_async === "function")
                ? _meme_native_complete_async
                : ((typeof Module !== "undefined" && typeof Module._meme_native_complete_async === "function")
                    ? Module._meme_native_complete_async : null);
            var done = function(ok, val) {
                try {
                    if (completeFn) {
                        var p = 0;
                        if (val) { var l = lengthBytesUTF8(val) + 1; p = _malloc(l); stringToUTF8(val, p, l); }
                        completeFn(rid, ok ? 1 : 0, p, 0, 0, 0, 0);
                        if (p) _free(p);
                    }
                } catch (e) {}
            };
            try {
                if (typeof document === "undefined") { done(false, ""); return; }
                var tag = "in" + "put";
                var el = document.createElement(tag);
                el.type = "file";
                if (imgOnly) el.accept = "image/*";
                el.style.display = "none";
                el.onchange = function() {
                    var f = el.files && el.files[0];
                    done(!!f, f ? f.name : "");
                    if (el.parentNode) el.parentNode.removeChild(el);
                };
                el.oncancel = function() {
                    done(false, "");
                    if (el.parentNode) el.parentNode.removeChild(el);
                };
                document.body.appendChild(el);
                el.click();
            } catch (e) { done(false, ""); }
        })($0, $1);
    }, reqId, acceptImage);
}
inline void js_geo_async(int reqId) {
    EM_ASM({
        (function(rid) {
            var completeFn = (typeof _meme_native_complete_async === "function")
                ? _meme_native_complete_async
                : ((typeof Module !== "undefined" && typeof Module._meme_native_complete_async === "function")
                    ? Module._meme_native_complete_async : null);
            var done = function(ok, err, lat, lon, acc) {
                try {
                    if (completeFn) {
                        var ep = 0;
                        if (err) { var l = lengthBytesUTF8(err) + 1; ep = _malloc(l); stringToUTF8(err, ep, l); }
                        completeFn(rid, ok ? 1 : 0, 0, ep, lat, lon, acc);
                        if (ep) _free(ep);
                    }
                } catch (e) {}
            };
            try {
                if (typeof navigator === "undefined" || !navigator.geolocation) {
                    done(false, "Geolocation unsupported", 0, 0, 0);
                    return;
                }
                navigator.geolocation.getCurrentPosition(
                    function(pos) { done(true, "", pos.coords.latitude, pos.coords.longitude, pos.coords.accuracy || 0); },
                    function(err) { done(false, err && err.message ? err.message : "Geolocation denied", 0, 0, 0); },
                    { timeout: 8000 }
                );
            } catch (e) { done(false, "Geolocation error", 0, 0, 0); }
        })($0);
    }, reqId);
}
inline char *js_perm_sync_status(const char *capPtr) {
    return reinterpret_cast<char *>(EM_ASM_PTR({
        return (function(cp) {
            var cap = UTF8ToString(cp);
            var st = "prompt";
            try {
                var w = (typeof window !== "undefined") ? window : globalThis;
                if (!w.__memePermCache) w.__memePermCache = {};
                if (cap === "notifications") {
                    if (typeof Notification !== "undefined") {
                        st = (Notification.permission === "granted") ? "granted" : ((Notification.permission === "denied") ? "denied" : "prompt");
                    } else if (typeof window === "undefined") {
                        st = "prompt";
                    } else {
                        st = "unavailable";
                    }
                } else if (cap === "photos" || cap === "storage") {
                    st = "granted";
                } else if (cap === "biometrics" || cap === "contacts" || cap === "calendar" || cap === "bluetooth" || cap === "locationAlways") {
                    st = w.__memePermCache[cap] || "unavailable";
                } else if (w.__memePermCache[cap]) {
                    st = w.__memePermCache[cap];
                } else if (typeof navigator !== "undefined" && navigator.permissions && navigator.permissions.query) {
                    var qName = (cap === "location") ? "geolocation" : ((cap === "clipboard") ? "clipboard-read" : cap);
                    if (qName === "geolocation" || qName === "camera" || qName === "microphone" || qName === "clipboard-read" || qName === "notifications") {
                        navigator.permissions.query({ name: qName }).then(function(ps) {
                            w.__memePermCache[cap] = ps.state;
                        }).catch(function() {});
                    }
                }
            } catch (e) {}
            var len = lengthBytesUTF8(st) + 1;
            var p = _malloc(len);
            stringToUTF8(st, p, len);
            return p;
        })($0);
    }, capPtr ? capPtr : ""));
}
inline void js_perm_request_async(int reqId, const char *capPtr) {
    EM_ASM({
        (function(rid, cp) {
            var cap = UTF8ToString(cp);
            var completeFn = (typeof _meme_native_complete_perm_async === "function")
                ? _meme_native_complete_perm_async
                : ((typeof Module !== "undefined" && typeof Module._meme_native_complete_perm_async === "function")
                    ? Module._meme_native_complete_perm_async : null);
            var finish = function(stateStr, errMsg) {
                try {
                    var w = (typeof window !== "undefined") ? window : globalThis;
                    if (!w.__memePermCache) w.__memePermCache = {};
                    w.__memePermCache[cap] = stateStr;
                    if (completeFn) {
                        var sl = lengthBytesUTF8(stateStr) + 1;
                        var sp = _malloc(sl);
                        stringToUTF8(stateStr, sp, sl);
                        var ep = 0;
                        if (errMsg) { var el = lengthBytesUTF8(errMsg) + 1; ep = _malloc(el); stringToUTF8(errMsg, ep, el); }
                        completeFn(rid, sp, ep);
                        _free(sp);
                        if (ep) _free(ep);
                    }
                } catch (e) {}
            };
            try {
                if (typeof window === "undefined" || typeof navigator === "undefined") {
                    finish("unavailable", "Browser APIs unavailable in headless Node runtime");
                    return;
                }
                if (cap === "notifications") {
                    if (typeof Notification === "undefined" || !Notification.requestPermission) {
                        finish("unavailable", "Notifications API unavailable");
                        return;
                    }
                    Notification.requestPermission().then(function(p) {
                        finish(p === "granted" ? "granted" : (p === "denied" ? "denied" : "prompt"), "");
                    }).catch(function(e) { finish("denied", String(e || "")); });
                    return;
                }
                if (cap === "camera" || cap === "microphone") {
                    if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia) {
                        finish("unavailable", "MediaDevices unavailable");
                        return;
                    }
                    var constraints = (cap === "camera") ? { video: true } : { audio: true };
                    navigator.mediaDevices.getUserMedia(constraints).then(function(stream) {
                        try { stream.getTracks().forEach(function(t) { t.stop(); }); } catch (e) {}
                        finish("granted", "");
                    }).catch(function(e) {
                        var nm = e && e.name ? e.name : "";
                        finish(nm === "NotFoundError" ? "unavailable" : "denied", e && e.message ? e.message : "Permission denied");
                    });
                    return;
                }
                if (cap === "location") {
                    if (!navigator.geolocation) {
                        finish("unavailable", "Geolocation unsupported");
                        return;
                    }
                    navigator.geolocation.getCurrentPosition(
                        function() { finish("granted", ""); },
                        function(err) { finish("denied", err && err.message ? err.message : "Denied"); },
                        { timeout: 8000 }
                    );
                    return;
                }
                if (cap === "sensors") {
                    if (typeof DeviceMotionEvent !== "undefined" && typeof DeviceMotionEvent.requestPermission === "function") {
                        DeviceMotionEvent.requestPermission().then(function(p) {
                            finish(p === "granted" ? "granted" : "denied", "");
                        }).catch(function() { finish("denied", "Sensor permission denied"); });
                    } else {
                        finish("granted", "");
                    }
                    return;
                }
                if (cap === "clipboard") {
                    if (navigator.clipboard && navigator.clipboard.readText) {
                        navigator.clipboard.readText().then(function() { finish("granted", ""); })
                            .catch(function() { finish("denied", "Clipboard read denied"); });
                    } else {
                        finish("granted", "");
                    }
                    return;
                }
                if (cap === "photos" || cap === "storage") {
                    finish("granted", "");
                    return;
                }
                finish("unavailable", "Capability unsupported on web platform");
            } catch (e) { finish("denied", String(e || "")); }
        })($0, $1);
    }, reqId, capPtr ? capPtr : "");
}
#endif

} // namespace detail

#if defined(__EMSCRIPTEN__)
extern "C" EMSCRIPTEN_KEEPALIVE inline void meme_native_complete_async(
    int reqId, int ok, const char *val, const char *err, double lat, double lon, double acc)
{
    AsyncResponse r;
    r.ok = (ok != 0);
    r.accepted = r.ok;
    r.cancelled = !r.ok;
    r.value = val ? val : "";
    r.path = r.value;
    if (!r.path.empty()) r.paths.push_back(r.path);
    r.error = err ? err : "";
    r.lat = lat;
    r.lon = lon;
    r.accuracy = acc;
    resolveAsyncCallback(reqId, r);
}

extern "C" EMSCRIPTEN_KEEPALIVE inline void meme_native_complete_perm_async(
    int reqId, const char *stateStr, const char *errStr)
{
    std::string st = MemePermissions::normalizeStateString(stateStr ? stateStr : "denied");
    AsyncResponse r;
    r.permissionState = st;
    r.ok = (st == "granted" || st == "limited");
    r.accepted = r.ok;
    r.cancelled = (st == "denied" || st == "prompt");
    r.limited = (st == "limited");
    r.restricted = (st == "restricted");
    r.canAskAgain = (st == "prompt" || st == "prompt-with-rationale");
    r.value = st;
    r.error = errStr ? errStr : "";
    resolveAsyncCallback(reqId, r);
}
#endif

inline std::string defaultPlatformName() {
#if defined(__EMSCRIPTEN__)
    return "wasm";
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

inline std::string defaultArchName() {
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

inline PathsInfo getAppPaths() {
    if (hooks().getAppPaths) return hooks().getAppPaths();
    PathsInfo p;
    std::string appKey;
    {
        std::lock_guard<std::mutex> lock(state().mu);
        appKey = state().appId;
    }
#if defined(__EMSCRIPTEN__)
    p.appData = "/memefs/data";
    p.cache = "/memefs/cache";
    p.documents = "/memefs/docs";
    p.temp = "/tmp";
#elif defined(_WIN32)
    const char *appd = std::getenv("APPDATA");
    const char *loc = std::getenv("LOCALAPPDATA");
    const char *tmp = std::getenv("TEMP");
    const char *prof = std::getenv("USERPROFILE");
    std::string base = appd ? appd : (prof ? prof : ".");
    p.appData = base + "\\" + appKey;
    p.cache = (loc ? std::string(loc) : base) + "\\" + appKey + "\\Cache";
    p.documents = (prof ? std::string(prof) : ".") + "\\Documents";
    p.temp = tmp ? tmp : "C:\\Windows\\Temp";
#elif defined(__APPLE__)
    const char *home = std::getenv("HOME");
    std::string h = home ? home : "/tmp";
#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
    p.appData = h + "/Library/Application Support/" + appKey;
    p.cache = h + "/Library/Caches/" + appKey;
    p.documents = h + "/Documents";
    p.temp = "/tmp";
#else
    p.appData = h + "/Library/Application Support/" + appKey;
    p.cache = h + "/Library/Caches/" + appKey;
    p.documents = h + "/Documents";
    p.temp = "/tmp";
#endif
#else
    const char *xdgData = std::getenv("XDG_DATA_HOME");
    const char *xdgCache = std::getenv("XDG_CACHE_HOME");
    const char *home = std::getenv("HOME");
    std::string h = home ? home : "/tmp";
    p.appData = (xdgData ? std::string(xdgData) : (h + "/.local/share")) + "/" + appKey;
    p.cache = (xdgCache ? std::string(xdgCache) : (h + "/.cache")) + "/" + appKey;
    p.documents = h + "/Documents";
    p.temp = "/tmp";
#endif
    std::error_code ec;
    std::filesystem::create_directories(p.appData, ec);
    std::filesystem::create_directories(p.cache, ec);
    return p;
}

inline std::string clipboardGet() {
    if (hooks().clipboardGet) return hooks().clipboardGet();
    if (std::getenv("MEME_HEADLESS") != nullptr) {
        std::lock_guard<std::mutex> lock(state().mu);
        return state().clipboardFallback;
    }
#if defined(__EMSCRIPTEN__)
    char *raw = detail::js_clip_get();
    std::string out = raw ? std::string(raw) : "";
    if (raw) std::free(raw);
    return out;
#elif defined(__APPLE__) && !(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
    std::string out;
    if (detail::execArgv({"/usr/bin/pbpaste"}, nullptr, &out)) return out;
#elif defined(__linux__) && !defined(__ANDROID__)
    std::string out;
    if (std::getenv("WAYLAND_DISPLAY") && detail::execArgv({"wl-paste", "-n"}, nullptr, &out)) return out;
    if (detail::execArgv({"xclip", "-o", "-selection", "clipboard"}, nullptr, &out)) return out;
    if (detail::execArgv({"xsel", "--clipboard", "--output"}, nullptr, &out)) return out;
#elif defined(_WIN32)
    typedef BOOL(WINAPI *OpenClipboardFn)(HWND);
    typedef BOOL(WINAPI *CloseClipboardFn)();
    typedef HANDLE(WINAPI *GetClipboardDataFn)(UINT);
    typedef BOOL(WINAPI *IsClipboardFormatAvailableFn)(UINT);
    HMODULE u32 = LoadLibraryA("user32.dll");
    if (u32) {
        auto pOpen = reinterpret_cast<OpenClipboardFn>(GetProcAddress(u32, "OpenClipboard"));
        auto pClose = reinterpret_cast<CloseClipboardFn>(GetProcAddress(u32, "CloseClipboard"));
        auto pGet = reinterpret_cast<GetClipboardDataFn>(GetProcAddress(u32, "GetClipboardData"));
        auto pAvail = reinterpret_cast<IsClipboardFormatAvailableFn>(GetProcAddress(u32, "IsClipboardFormatAvailable"));
        if (pOpen && pClose && pGet && pAvail && pAvail(1 /*CF_TEXT*/) && pOpen(nullptr)) {
            std::string res;
            HANDLE h = pGet(1);
            if (h) {
                const char *ptr = static_cast<const char *>(GlobalLock(h));
                if (ptr) { res = ptr; GlobalUnlock(h); }
            }
            pClose();
            return res;
        }
    }
#endif
    std::lock_guard<std::mutex> lock(state().mu);
    return state().clipboardFallback;
}

inline bool clipboardSet(const std::string &text) {
    {
        std::lock_guard<std::mutex> lock(state().mu);
        state().clipboardFallback = text;
    }
    if (hooks().clipboardSet) return hooks().clipboardSet(text);
    if (std::getenv("MEME_HEADLESS") != nullptr) return true;
#if defined(__EMSCRIPTEN__)
    detail::js_clip_set(text.c_str());
    return true;
#elif defined(__APPLE__) && !(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
    if (detail::execArgv({"/usr/bin/pbcopy"}, &text, nullptr)) return true;
#elif defined(__linux__) && !defined(__ANDROID__)
    if (std::getenv("WAYLAND_DISPLAY") && detail::execArgv({"wl-copy"}, &text, nullptr)) return true;
    if (detail::execArgv({"xclip", "-selection", "clipboard"}, &text, nullptr)) return true;
    if (detail::execArgv({"xsel", "--clipboard", "--input"}, &text, nullptr)) return true;
#elif defined(_WIN32)
    typedef BOOL(WINAPI *OpenClipboardFn)(HWND);
    typedef BOOL(WINAPI *CloseClipboardFn)();
    typedef BOOL(WINAPI *EmptyClipboardFn)();
    typedef HANDLE(WINAPI *SetClipboardDataFn)(UINT, HANDLE);
    HMODULE u32 = LoadLibraryA("user32.dll");
    if (u32) {
        auto pOpen = reinterpret_cast<OpenClipboardFn>(GetProcAddress(u32, "OpenClipboard"));
        auto pClose = reinterpret_cast<CloseClipboardFn>(GetProcAddress(u32, "CloseClipboard"));
        auto pEmpty = reinterpret_cast<EmptyClipboardFn>(GetProcAddress(u32, "EmptyClipboard"));
        auto pSet = reinterpret_cast<SetClipboardDataFn>(GetProcAddress(u32, "SetClipboardData"));
        if (pOpen && pClose && pEmpty && pSet && pOpen(nullptr)) {
            pEmpty();
            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
            if (hMem) {
                char *dst = static_cast<char *>(GlobalLock(hMem));
                if (dst) {
                    std::memcpy(dst, text.data(), text.size());
                    dst[text.size()] = '\0';
                    GlobalUnlock(hMem);
                    pSet(1 /*CF_TEXT*/, hMem);
                }
            }
            pClose();
            return true;
        }
    }
#endif
    return true;
}

inline bool clipboardHasText() {
    return !clipboardGet().empty();
}

inline bool clipboardClear() {
    return clipboardSet("");
}

inline DeviceInfo getDeviceInfo() {
    if (hooks().getDeviceInfo) return hooks().getDeviceInfo();
    DeviceInfo d;
    d.os = defaultPlatformName();
    d.arch = defaultArchName();
    d.isWeb = (d.os == "wasm");
    d.isMobile = (d.os == "ios" || d.os == "android");
    d.isDesktop = (!d.isWeb && !d.isMobile);
#if defined(__EMSCRIPTEN__)
    d.model = "Browser Wasm";
    d.isDark = (detail::js_is_dark() != 0);
    d.theme = d.isDark ? "dark" : "light";
    char *loc = detail::js_locale();
    if (loc) { if (*loc) d.locale = loc; std::free(loc); }
    char *tz = detail::js_timezone();
    if (tz) { if (*tz) d.timezone = tz; std::free(tz); }
#else
    if (const char *lang = std::getenv("LC_ALL")) d.locale = lang;
    else if (const char *lang2 = std::getenv("LANG")) d.locale = lang2;
    size_t dot = d.locale.find('.');
    if (dot != std::string::npos) d.locale = d.locale.substr(0, dot);
    std::replace(d.locale.begin(), d.locale.end(), '_', '-');
    if (const char *tz = std::getenv("TZ")) d.timezone = tz;
    d.model = d.os + "-" + d.arch;
    d.theme = d.isDark ? "dark" : "light";
#endif
    return d;
}

inline ScreenInfo getScreenInfo() {
    if (hooks().getScreenInfo) return hooks().getScreenInfo();
    return ScreenInfo{};
}

inline BatteryInfo getBatteryInfo() {
    if (hooks().getBatteryInfo) return hooks().getBatteryInfo();
    BatteryInfo b;
#if defined(__EMSCRIPTEN__)
    detail::js_init_web_probes();
    double lvl = detail::js_battery_level();
    if (lvl >= 0.0) {
        b.supported = true;
        b.level = lvl;
        b.charging = (detail::js_battery_charging() != 0);
        b.state = b.charging ? "charging" : (lvl >= 99.5 ? "full" : "discharging");
    }
#elif defined(_WIN32)
    SYSTEM_POWER_STATUS sps;
    if (GetSystemPowerStatus(&sps)) {
        b.supported = (sps.BatteryLifePercent <= 100);
        b.level = b.supported ? static_cast<double>(sps.BatteryLifePercent) : -1.0;
        b.charging = (sps.ACLineStatus == 1) || ((sps.BatteryFlag & 8) != 0);
        b.state = b.charging ? "charging" : (b.supported ? "discharging" : "unknown");
    }
#elif defined(__linux__) && !defined(__ANDROID__)
    std::error_code ec;
    if (std::filesystem::exists("/sys/class/power_supply", ec)) {
        for (const auto &entry : std::filesystem::directory_iterator("/sys/class/power_supply", ec)) {
            std::ifstream tf(entry.path() / "type");
            std::string tp;
            if (tf && std::getline(tf, tp) && tp.find("Battery") != std::string::npos) {
                std::ifstream cf(entry.path() / "capacity");
                std::ifstream sf(entry.path() / "status");
                int cap = -1;
                std::string st;
                if (cf) cf >> cap;
                if (sf) std::getline(sf, st);
                b.supported = (cap >= 0);
                b.level = static_cast<double>(cap);
                for (char &c : st) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                b.charging = (st.find("charging") != std::string::npos && st.find("discharging") == std::string::npos);
                b.state = st.empty() ? (b.charging ? "charging" : "discharging") : st;
                break;
            }
        }
    }
#elif defined(__APPLE__) && !(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
    std::string out;
    if (detail::execArgv({"/usr/bin/pmset", "-g", "batt"}, nullptr, &out)) {
        size_t pct = out.find('%');
        if (pct != std::string::npos && pct > 0) {
            size_t start = pct;
            while (start > 0 && std::isdigit(static_cast<unsigned char>(out[start - 1]))) --start;
            if (start < pct) {
                b.level = std::stod(out.substr(start, pct - start));
                b.supported = true;
            }
        }
        b.charging = (out.find("AC Power") != std::string::npos || out.find("charging;") != std::string::npos);
        b.state = (out.find("charged;") != std::string::npos) ? "full" : (b.charging ? "charging" : "discharging");
    }
#endif
    return b;
}

inline NetworkInfo getNetworkInfo() {
    if (hooks().getNetworkInfo) return hooks().getNetworkInfo();
    NetworkInfo n;
#if defined(__EMSCRIPTEN__)
    n.online = (detail::js_is_online() != 0);
    n.type = n.online ? "browser" : "offline";
#elif defined(__linux__) && !defined(__ANDROID__)
    std::error_code ec;
    n.online = false;
    n.type = "offline";
    if (std::filesystem::exists("/sys/class/net", ec)) {
        for (const auto &entry : std::filesystem::directory_iterator("/sys/class/net", ec)) {
            std::string ifn = entry.path().filename().string();
            if (ifn == "lo") continue;
            std::ifstream os(entry.path() / "operstate");
            std::string st;
            if (os && std::getline(os, st) && st == "up") {
                n.online = true;
                n.type = (ifn.rfind("wl", 0) == 0) ? "wifi" : "ethernet";
                break;
            }
        }
    }
#else
    n.online = true;
    n.type = "unknown";
#endif
    return n;
}

inline SensorSnapshot getSensorSnapshot() {
    if (hooks().getSensorSnapshot) return hooks().getSensorSnapshot();
    SensorSnapshot s;
#if defined(__EMSCRIPTEN__)
    detail::js_init_web_probes();
    if (detail::js_sensor_available()) {
        s.available = true;
        s.accelX = detail::js_sensor_axis(0);
        s.accelY = detail::js_sensor_axis(1);
        s.accelZ = detail::js_sensor_axis(2);
        s.gyroX = detail::js_sensor_axis(3);
        s.gyroY = detail::js_sensor_axis(4);
        s.gyroZ = detail::js_sensor_axis(5);
    }
#endif
    return s;
}

inline bool hapticImpact(const std::string &style = "medium") {
    if (hooks().hapticImpact) return hooks().hapticImpact(style);
#if defined(__EMSCRIPTEN__)
    detail::js_vibrate(style == "heavy" ? 30 : (style == "light" ? 10 : 18));
    return true;
#else
    return false;
#endif
}

inline bool hapticNotify(const std::string &type = "success") {
    if (hooks().hapticNotify) return hooks().hapticNotify(type);
#if defined(__EMSCRIPTEN__)
    detail::js_vibrate(type == "error" ? 45 : 22);
    return true;
#else
    return false;
#endif
}

inline bool hapticSelection() {
    if (hooks().hapticSelection) return hooks().hapticSelection();
#if defined(__EMSCRIPTEN__)
    detail::js_vibrate(8);
    return true;
#else
    return false;
#endif
}

inline bool hapticVibrate(int ms = 100) {
    if (hooks().hapticVibrate) return hooks().hapticVibrate(ms);
#if defined(__EMSCRIPTEN__)
    detail::js_vibrate(std::max(1, ms));
    return true;
#else
    return false;
#endif
}

inline bool canOpenUrl(const std::string &url) {
    if (hooks().canOpenUrl) return hooks().canOpenUrl(url);
    return url.find("://") != std::string::npos || url.rfind("mailto:", 0) == 0 || url.rfind("tel:", 0) == 0;
}

inline bool openUrl(const std::string &url) {
    if (url.empty()) return false;
    if (hooks().openUrl) return hooks().openUrl(url);
#if defined(__EMSCRIPTEN__)
    detail::js_open_url(url.c_str());
    return true;
#elif defined(__APPLE__) && !(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
    return detail::execArgv({"/usr/bin/open", url}, nullptr, nullptr, nullptr, true);
#elif defined(_WIN32)
    typedef HINSTANCE(WINAPI *ShellExecuteAFn)(HWND, LPCSTR, LPCSTR, LPCSTR, LPCSTR, INT);
    HMODULE sh = LoadLibraryA("shell32.dll");
    if (sh) {
        auto pShell = reinterpret_cast<ShellExecuteAFn>(GetProcAddress(sh, "ShellExecuteA"));
        if (pShell) {
            return reinterpret_cast<intptr_t>(pShell(nullptr, "open", url.c_str(), nullptr, nullptr, 1 /*SW_SHOWNORMAL*/)) > 32;
        }
    }
    return false;
#elif defined(__linux__) && !defined(__ANDROID__)
    return detail::execArgv({"xdg-open", url}, nullptr, nullptr, nullptr, true);
#else
    return false;
#endif
}

inline void shareAsync(const std::string &text, const std::string &title, const std::string &url, AsyncCallback cb) {
    if (hooks().shareAsync) { hooks().shareAsync(text, title, url, std::move(cb)); return; }
#if defined(__EMSCRIPTEN__)
    detail::js_share(title.c_str(), text.c_str(), url.c_str());
#endif
    AsyncResponse res;
    res.ok = true;
    res.value = text.empty() ? url : text;
    dispatchCallbackOnUiThread(cb, res);
}

inline void notifyRequestPermissionAsync(AsyncCallback cb) {
    if (hooks().notifyRequestPermissionAsync) { hooks().notifyRequestPermissionAsync(std::move(cb)); return; }
#if defined(__EMSCRIPTEN__)
    int id = registerAsyncCallback(std::move(cb));
    detail::js_notify_perm_async(id);
    return;
#else
    AsyncResponse res;
    res.ok = true;
    res.accepted = state().notifyGranted;
    dispatchCallbackOnUiThread(cb, res);
#endif
}

inline bool notifySend(const std::string &title, const std::string &body) {
    {
        std::lock_guard<std::mutex> lock(state().mu);
        state().notifications.push_back({title, body});
    }
    if (hooks().notifySend) return hooks().notifySend(title, body);
#if defined(__EMSCRIPTEN__)
    detail::js_notify(title.c_str(), body.c_str());
    return true;
#elif defined(__APPLE__) && !(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
    auto escApple = [](const std::string &s) {
        std::string out;
        for (char c : s) {
            if (c == '\\' || c == '"') out.push_back('\\');
            out.push_back(c);
        }
        return out;
    };
    std::string script = "display notification \"" + escApple(body) + "\" with title \"" + escApple(title) + "\"";
    return detail::execArgv({"/usr/bin/osascript", "-e", script}, nullptr, nullptr, nullptr, true);
#elif defined(__linux__) && !defined(__ANDROID__)
    return detail::execArgv({"notify-send", title, body}, nullptr, nullptr, nullptr, true);
#else
    return true;
#endif
}

inline void setBuildTarget(const std::string &target) {
    MemePlatform::setRuntimePlatformOverride(target);
}

inline std::string getBuildTarget() {
    return MemePlatform::currentPlatform();
}

inline MemePermissions::PermissionStatus checkPermission(const std::string &rawCap) {
    std::string cap = MemePermissions::normalizeCapability(rawCap);
    std::string plat = MemePlatform::currentPlatform();
    MemePermissions::PermissionStatus st;
    st.permission = cap;
    st.platform = plat;
    st.reason = MemePermissions::getPermissionDescription(cap, state().appName);

    // Check explicit headless/test overrides first
    {
        auto &pst = MemePermissions::permStore();
        std::lock_guard<std::mutex> lock(pst.mu);
        auto ov = pst.overrides.find(cap);
        if (ov != pst.overrides.end()) {
            st.state = ov->second;
            st.finalizeFlags();
            return st;
        }
    }

    if (!MemePermissions::isKnownCapability(cap) || !MemePermissions::isSupportedOnPlatform(cap, plat)) {
        st.state = "unavailable";
        st.error = "Capability '" + rawCap + "' is not available on platform '" + plat + "'";
        st.finalizeFlags();
        return st;
    }

    if (!MemePermissions::isPermissionDeclared(cap)) {
        st.state = "denied";
        st.canAskAgain = false;
        st.error = "Capability '" + cap + "' is not declared in tapri.json permissions for '" + plat + "'";
        st.finalizeFlags();
        return st;
    }

    if (std::getenv("MEME_HEADLESS") != nullptr) {
        if (cap == "clipboard" || cap == "storage" || cap == "sensors") {
            st.state = "granted";
        } else {
            st.state = "prompt";
        }
        st.finalizeFlags();
        return st;
    }

    if (hooks().checkPermission) {
        st = hooks().checkPermission(cap);
        if (st.permission.empty()) st.permission = cap;
        if (st.platform.empty()) st.platform = plat;
        if (st.reason.empty()) st.reason = MemePermissions::getPermissionDescription(cap, state().appName);
        st.finalizeFlags();
        return st;
    }

#if defined(__EMSCRIPTEN__)
    char *rawSt = detail::js_perm_sync_status(cap.c_str());
    st.state = rawSt ? std::string(rawSt) : "prompt";
    if (rawSt) std::free(rawSt);
#elif defined(__APPLE__) && !(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
    if (cap == "clipboard" || cap == "storage" || cap == "sensors") {
        st.state = "granted";
    } else if (cap == "notifications" || cap == "camera" || cap == "microphone" || cap == "photos" ||
               cap == "location" || cap == "locationAlways" || cap == "contacts" || cap == "calendar" || cap == "biometrics") {
        st.state = "prompt";
    } else {
        st.state = "unavailable";
    }
#elif defined(__linux__) && !defined(__ANDROID__)
    if (cap == "clipboard" || cap == "storage" || cap == "notifications" || cap == "sensors") st.state = "granted";
    else st.state = MemePermissions::isSupportedOnPlatform(cap, plat) ? "prompt" : "unavailable";
#elif defined(_WIN32)
    if (cap == "clipboard" || cap == "storage" || cap == "notifications" || cap == "sensors") st.state = "granted";
    else st.state = MemePermissions::isSupportedOnPlatform(cap, plat) ? "prompt" : "unavailable";
#else
    st.state = "prompt";
#endif
    st.finalizeFlags();
    return st;
}

using PermissionCallback = std::function<void(const MemePermissions::PermissionStatus &)>;

inline void requestPermissionAsync(const std::string &rawCap, PermissionCallback cb) {
    std::string cap = MemePermissions::normalizeCapability(rawCap);
    std::string plat = MemePlatform::currentPlatform();
    std::string desc = MemePermissions::getPermissionDescription(cap, state().appName);

    auto invokeWith = [cb](MemePermissions::PermissionStatus st) {
        st.finalizeFlags();
        if (cb) cb(st);
        if (hooks().requestRerender) hooks().requestRerender();
    };

    // Check headless override first
    {
        auto &pst = MemePermissions::permStore();
        std::lock_guard<std::mutex> lock(pst.mu);
        auto ov = pst.overrides.find(cap);
        if (ov != pst.overrides.end()) {
            MemePermissions::PermissionStatus st;
            st.permission = cap;
            st.platform = plat;
            st.reason = desc;
            st.state = ov->second;
            invokeWith(st);
            return;
        }
    }

    if (!MemePermissions::isKnownCapability(cap) || !MemePermissions::isSupportedOnPlatform(cap, plat)) {
        MemePermissions::PermissionStatus st;
        st.permission = cap;
        st.platform = plat;
        st.reason = desc;
        st.state = "unavailable";
        st.error = "Capability '" + cap + "' unsupported on platform '" + plat + "'";
        invokeWith(st);
        return;
    }

    if (!MemePermissions::isPermissionDeclared(cap)) {
        MemePermissions::PermissionStatus st;
        st.permission = cap;
        st.platform = plat;
        st.reason = desc;
        st.state = "denied";
        st.canAskAgain = false;
        st.error = "Capability '" + cap + "' not declared in tapri.json permissions for '" + plat + "'";
        invokeWith(st);
        return;
    }

    if (std::getenv("MEME_HEADLESS") != nullptr) {
        MemePermissions::PermissionStatus st;
        st.permission = cap;
        st.platform = plat;
        st.reason = desc;
        st.state = state().notifyGranted ? "granted" : "denied";
        {
            auto &pst = MemePermissions::permStore();
            std::lock_guard<std::mutex> lock(pst.mu);
            pst.overrides[cap] = st.state;
        }
        invokeWith(st);
        return;
    }

    auto wrapAsyncToPerm = [cap, plat, desc, invokeWith](const AsyncResponse &ar) {
        MemePermissions::PermissionStatus st;
        st.permission = cap;
        st.platform = plat;
        st.reason = desc;
        if (!ar.permissionState.empty()) st.state = ar.permissionState;
        else st.state = ar.accepted ? "granted" : "denied";
        st.limited = ar.limited;
        st.restricted = ar.restricted;
        st.canAskAgain = ar.canAskAgain;
        st.error = ar.error;
        if (cap == "notifications" && (st.state == "granted" || st.state == "limited")) {
            std::lock_guard<std::mutex> lock(state().mu);
            state().notifyGranted = true;
        }
        invokeWith(st);
    };

    if (hooks().requestPermissionAsync) {
        hooks().requestPermissionAsync(cap, wrapAsyncToPerm);
        return;
    }

#if defined(__EMSCRIPTEN__)
    int rid = registerAsyncCallback(wrapAsyncToPerm);
    detail::js_perm_request_async(rid, cap.c_str());
    return;
#elif defined(__APPLE__) && !(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
    if (cap == "notifications" || cap == "camera" || cap == "microphone" || cap == "location" || cap == "clipboard" || cap == "storage") {
        MemePermissions::PermissionStatus st = checkPermission(cap);
        if (st.state == "prompt" && (cap == "clipboard" || cap == "storage" || cap == "notifications")) {
            st.state = "granted";
        }
        invokeWith(st);
        return;
    }
    MemePermissions::PermissionStatus st = checkPermission(cap);
    invokeWith(st);
#else
    MemePermissions::PermissionStatus st = checkPermission(cap);
    if (cap == "clipboard" || cap == "storage" || cap == "notifications") st.state = "granted";
    invokeWith(st);
#endif
}

inline bool openAppSettings(const std::string &rawCap = "") {
    std::string cap = rawCap.empty() ? "" : MemePermissions::normalizeCapability(rawCap);
    if (std::getenv("MEME_HEADLESS") != nullptr) return true;
    if (hooks().openAppSettings) return hooks().openAppSettings(cap);
#if defined(__EMSCRIPTEN__)
    return false;
#elif defined(__APPLE__) && !(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
    return detail::execArgv({"/usr/bin/open", MemePermissions::macSettingsUriFor(cap)}, nullptr, nullptr, nullptr, true);
#elif defined(_WIN32)
    return openUrl(MemePermissions::windowsSettingsUriFor(cap));
#elif defined(__linux__) && !defined(__ANDROID__)
    return detail::execArgv({"gnome-control-center", "privacy"}, nullptr, nullptr, nullptr, true) ||
           detail::execArgv({"xdg-open", "settings://privacy"}, nullptr, nullptr, nullptr, true);
#else
    return false;
#endif
}

inline bool setBadgeCount(int count) {
    {
        std::lock_guard<std::mutex> lock(state().mu);
        state().badgeCount = std::max(0, count);
    }
    if (hooks().setBadgeCount) return hooks().setBadgeCount(std::max(0, count));
    return true;
}

inline void alertAsync(const std::string &title, const std::string &msg, AsyncCallback cb) {
    if (hooks().alertAsync) { hooks().alertAsync(title, msg, std::move(cb)); return; }
#if defined(__EMSCRIPTEN__)
    detail::js_alert(title.c_str(), msg.c_str());
#elif defined(_WIN32)
    typedef int(WINAPI *MessageBoxAFn)(HWND, LPCSTR, LPCSTR, UINT);
    if (HMODULE u32 = LoadLibraryA("user32.dll")) {
        if (auto pBox = reinterpret_cast<MessageBoxAFn>(GetProcAddress(u32, "MessageBoxA"))) {
            pBox(nullptr, msg.c_str(), title.c_str(), 0 /*MB_OK*/);
        }
    }
#elif defined(__linux__) && !defined(__ANDROID__)
    if (std::getenv("DISPLAY") || std::getenv("WAYLAND_DISPLAY")) {
        detail::execArgv({"zenity", "--info", "--title=" + title, "--text=" + msg}, nullptr, nullptr);
    }
#endif
    AsyncResponse r;
    r.ok = true; r.accepted = true;
    dispatchCallbackOnUiThread(cb, r);
}

inline void confirmAsync(const std::string &title, const std::string &msg, AsyncCallback cb) {
    if (hooks().confirmAsync) { hooks().confirmAsync(title, msg, std::move(cb)); return; }
    bool accepted = false;
#if defined(__EMSCRIPTEN__)
    accepted = (detail::js_confirm(title.c_str(), msg.c_str()) != 0);
#elif defined(_WIN32)
    typedef int(WINAPI *MessageBoxAFn)(HWND, LPCSTR, LPCSTR, UINT);
    if (HMODULE u32 = LoadLibraryA("user32.dll")) {
        if (auto pBox = reinterpret_cast<MessageBoxAFn>(GetProcAddress(u32, "MessageBoxA"))) {
            accepted = (pBox(nullptr, msg.c_str(), title.c_str(), 4 /*MB_YESNO*/) == 6 /*IDYES*/);
        }
    }
#elif defined(__linux__) && !defined(__ANDROID__)
    if (std::getenv("DISPLAY") || std::getenv("WAYLAND_DISPLAY")) {
        int code = 1;
        detail::execArgv({"zenity", "--question", "--title=" + title, "--text=" + msg}, nullptr, nullptr, &code);
        accepted = (code == 0);
    }
#endif
    AsyncResponse r;
    r.ok = true; r.accepted = accepted; r.cancelled = !accepted;
    dispatchCallbackOnUiThread(cb, r);
}

inline void promptAsync(const std::string &title, const std::string &msg, const std::string &defVal, AsyncCallback cb) {
    if (hooks().promptAsync) { hooks().promptAsync(title, msg, defVal, std::move(cb)); return; }
    AsyncResponse r;
    r.value = defVal;
#if defined(__EMSCRIPTEN__)
    char *raw = detail::js_prompt(title.c_str(), msg.c_str(), defVal.c_str());
    if (raw) { r.value = raw; r.accepted = true; std::free(raw); }
    else { r.accepted = false; r.cancelled = true; r.value = ""; }
#elif defined(__linux__) && !defined(__ANDROID__)
    if (std::getenv("DISPLAY") || std::getenv("WAYLAND_DISPLAY")) {
        std::string out;
        int code = 1;
        detail::execArgv({"zenity", "--entry", "--title=" + title, "--text=" + msg, "--entry-text=" + defVal}, nullptr, &out, &code);
        if (code == 0) {
            while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
            r.value = out;
            r.accepted = true;
        } else {
            r.accepted = false; r.cancelled = true;
        }
    }
#endif
    dispatchCallbackOnUiThread(cb, r);
}

inline void fileDialogAsync(const std::string &mode, const std::string &title, const std::string &defaultName, AsyncCallback cb) {
    if (hooks().fileDialogAsync) { hooks().fileDialogAsync(mode, title, defaultName, std::move(cb)); return; }
#if defined(__EMSCRIPTEN__)
    if (mode != "save") {
        int id = registerAsyncCallback(std::move(cb));
        detail::js_pick_file_async(id, (mode == "photo" || mode == "camera") ? 1 : 0);
        return;
    }
#endif
    AsyncResponse r;
    r.ok = false; r.accepted = false; r.cancelled = true;
#if defined(__linux__) && !defined(__ANDROID__)
    if (std::getenv("DISPLAY") || std::getenv("WAYLAND_DISPLAY")) {
        std::vector<std::string> args = {"zenity", "--file-selection", "--title=" + title};
        if (mode == "save") args.push_back("--save");
        if (mode == "folder") args.push_back("--directory");
        if (!defaultName.empty()) args.push_back("--filename=" + defaultName);
        std::string out;
        int code = 1;
        detail::execArgv(args, nullptr, &out, &code);
        if (code == 0) {
            while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
            r.ok = !out.empty();
            r.accepted = r.ok;
            r.cancelled = !r.ok;
            r.path = out;
            r.value = out;
            if (!out.empty()) r.paths.push_back(out);
        }
    }
#endif
    dispatchCallbackOnUiThread(cb, r);
}

inline std::string kvStorageFilePath() {
    return getAppPaths().appData + "/meme_storage.json";
}

inline std::string keychainStorageFilePath() {
    return getAppPaths().appData + "/meme_keychain.json";
}

inline std::map<std::string, std::string> loadKVStoreFile(const std::string &path) {
    std::ifstream ifs(path);
    if (!ifs) return {};
    std::ostringstream ss;
    ss << ifs.rdbuf();
    return detail::parseFlatJsonMap(ss.str());
}

inline bool saveKVStoreFile(const std::string &path, const std::map<std::string, std::string> &m, bool restricted = false) {
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
    std::ofstream ofs(path, std::ios::trunc);
    if (!ofs) return false;
    ofs << detail::serializeFlatJsonMap(m);
    ofs.close();
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
    if (restricted) ::chmod(path.c_str(), 0600);
#else
    (void)restricted;
#endif
    return true;
}

inline std::string storageGet(const std::string &key, const std::string &defVal = "") {
#if defined(__EMSCRIPTEN__)
    char *raw = detail::js_storage_get(key.c_str());
    std::string val = raw ? std::string(raw) : "";
    if (raw) std::free(raw);
    return val.empty() ? defVal : val;
#else
    std::string p = kvStorageFilePath();
    std::lock_guard<std::mutex> lock(state().storageMu);
    auto m = loadKVStoreFile(p);
    auto it = m.find(key);
    return (it != m.end()) ? it->second : defVal;
#endif
}

inline bool storageSet(const std::string &key, const std::string &val) {
#if defined(__EMSCRIPTEN__)
    detail::js_storage_set(key.c_str(), val.c_str());
    return true;
#else
    std::string p = kvStorageFilePath();
    std::lock_guard<std::mutex> lock(state().storageMu);
    auto m = loadKVStoreFile(p);
    m[key] = val;
    return saveKVStoreFile(p, m, false);
#endif
}

inline bool storageRemove(const std::string &key) {
#if defined(__EMSCRIPTEN__)
    detail::js_storage_remove(key.c_str());
    return true;
#else
    std::string p = kvStorageFilePath();
    std::lock_guard<std::mutex> lock(state().storageMu);
    auto m = loadKVStoreFile(p);
    m.erase(key);
    return saveKVStoreFile(p, m, false);
#endif
}

inline std::vector<std::string> storageKeys() {
    std::vector<std::string> out;
#if defined(__EMSCRIPTEN__)
    char *raw = detail::js_storage_keys();
    if (raw) {
        std::stringstream ss(raw);
        std::string line;
        while (std::getline(ss, line)) {
            if (!line.empty()) out.push_back(line);
        }
        std::free(raw);
    }
#else
    std::string p = kvStorageFilePath();
    std::lock_guard<std::mutex> lock(state().storageMu);
    auto m = loadKVStoreFile(p);
    for (const auto &kv : m) out.push_back(kv.first);
#endif
    return out;
}

inline std::string keychainGet(const std::string &service, const std::string &key) {
    if (hooks().keychainGet) return hooks().keychainGet(service, key);
#if defined(__EMSCRIPTEN__)
    return storageGet("__kc_" + service + "::" + key);
#else
    std::string p = keychainStorageFilePath();
    std::lock_guard<std::mutex> lock(state().storageMu);
    auto m = loadKVStoreFile(p);
    auto it = m.find(service + "::" + key);
    return (it != m.end()) ? it->second : "";
#endif
}

inline bool keychainSet(const std::string &service, const std::string &key, const std::string &val) {
    if (hooks().keychainSet) return hooks().keychainSet(service, key, val);
#if defined(__EMSCRIPTEN__)
    return storageSet("__kc_" + service + "::" + key, val);
#else
    std::string p = keychainStorageFilePath();
    std::lock_guard<std::mutex> lock(state().storageMu);
    auto m = loadKVStoreFile(p);
    m[service + "::" + key] = val;
    return saveKVStoreFile(p, m, true);
#endif
}

inline bool keychainRemove(const std::string &service, const std::string &key) {
    if (hooks().keychainRemove) return hooks().keychainRemove(service, key);
#if defined(__EMSCRIPTEN__)
    return storageRemove("__kc_" + service + "::" + key);
#else
    std::string p = keychainStorageFilePath();
    std::lock_guard<std::mutex> lock(state().storageMu);
    auto m = loadKVStoreFile(p);
    m.erase(service + "::" + key);
    return saveKVStoreFile(p, m, true);
#endif
}

inline bool biometricsAvailable() {
    if (hooks().biometricsAvailable) return hooks().biometricsAvailable();
    return false;
}

inline void biometricsAuthAsync(const std::string &reason, AsyncCallback cb) {
    if (hooks().biometricsAuthAsync) { hooks().biometricsAuthAsync(reason, std::move(cb)); return; }
    AsyncResponse r;
    r.ok = false; r.accepted = false; r.error = "Biometrics unavailable on this platform/session";
    dispatchCallbackOnUiThread(cb, r);
}

inline void getLocationAsync(AsyncCallback cb) {
    if (hooks().getLocationAsync) { hooks().getLocationAsync(std::move(cb)); return; }
#if defined(__EMSCRIPTEN__)
    int id = registerAsyncCallback(std::move(cb));
    detail::js_geo_async(id);
    return;
#else
    AsyncResponse r;
    r.ok = false; r.accepted = false; r.error = "Location service unavailable";
    dispatchCallbackOnUiThread(cb, r);
#endif
}

inline bool beep(double freqHz = 440.0, int durationMs = 150) {
    if (hooks().beep) return hooks().beep(freqHz, durationMs);
#if defined(__EMSCRIPTEN__)
    detail::js_beep(freqHz, durationMs);
    return true;
#elif defined(_WIN32)
    return ::Beep(static_cast<DWORD>(std::clamp(freqHz, 37.0, 32767.0)), static_cast<DWORD>(std::max(10, durationMs))) != 0;
#else
    std::fputc('\a', stdout);
    std::fflush(stdout);
    return true;
#endif
}

inline bool speak(const std::string &text, double rate = 1.0) {
    if (hooks().speak) return hooks().speak(text, rate);
#if defined(__EMSCRIPTEN__)
    detail::js_speak(text.c_str(), rate);
    return true;
#elif defined(__APPLE__) && !(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
    return detail::execArgv({"/usr/bin/say", text}, nullptr, nullptr, nullptr, true);
#elif defined(__linux__) && !defined(__ANDROID__)
    return detail::execArgv({"espeak", text}, nullptr, nullptr, nullptr, true) ||
           detail::execArgv({"spd-say", text}, nullptr, nullptr, nullptr, true);
#else
    return false;
#endif
}

inline bool stopSpeaking() {
    if (hooks().stopSpeaking) return hooks().stopSpeaking();
#if defined(__EMSCRIPTEN__)
    detail::js_stop_speaking();
    return true;
#else
    return false;
#endif
}

inline bool playSound(const std::string &pathOrId) {
    if (hooks().playSound) return hooks().playSound(pathOrId);
#if defined(__APPLE__) && !(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
    if (std::filesystem::exists(pathOrId)) {
        return detail::execArgv({"/usr/bin/afplay", pathOrId}, nullptr, nullptr, nullptr, true);
    }
#elif defined(__linux__) && !defined(__ANDROID__)
    if (std::filesystem::exists(pathOrId)) {
        return detail::execArgv({"paplay", pathOrId}, nullptr, nullptr, nullptr, true) ||
               detail::execArgv({"aplay", pathOrId}, nullptr, nullptr, nullptr, true);
    }
#elif defined(_WIN32)
    typedef BOOL(WINAPI *PlaySoundAFn)(LPCSTR, HMODULE, DWORD);
    if (HMODULE wm = LoadLibraryA("winmm.dll")) {
        if (auto pPlay = reinterpret_cast<PlaySoundAFn>(GetProcAddress(wm, "PlaySoundA"))) {
            return pPlay(pathOrId.c_str(), nullptr, 0x00020001 /*SND_FILENAME | SND_ASYNC*/) != 0;
        }
    }
#endif
    return beep(520.0, 120);
}

inline bool setWindowTitle(const std::string &title) {
    {
        std::lock_guard<std::mutex> lock(state().mu);
        state().windowTitle = title;
    }
    if (hooks().setWindowTitle) return hooks().setWindowTitle(title);
#if defined(__EMSCRIPTEN__)
    detail::js_set_title(title.c_str());
    return true;
#else
    return true;
#endif
}

inline bool setWindowSize(int w, int h) {
    if (hooks().setWindowSize) return hooks().setWindowSize(w, h);
    return false;
}

inline bool setFullscreen(bool on) {
    if (hooks().setFullscreen) return hooks().setFullscreen(on);
#if defined(__EMSCRIPTEN__)
    detail::js_fullscreen(on ? 1 : 0);
    return true;
#else
    return false;
#endif
}

inline bool minimizeWindow() {
    if (hooks().minimizeWindow) return hooks().minimizeWindow();
    return false;
}

inline bool focusWindow() {
    if (hooks().focusWindow) return hooks().focusWindow();
    return false;
}

inline bool hideKeyboard() {
    if (hooks().hideKeyboard) return hooks().hideKeyboard();
    return false;
}

} // namespace MemeNative
