#pragma once

#if defined(__linux__) && !defined(__ANDROID__)

#include <dlfcn.h>
#include <poll.h>
#include <unistd.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

#include "../../ui/MemeUiTypes.h"
#include "../../ui/MemeUiTessellator.h"
#include "../../ui/MemeUiRuntime.h"
#include "../../platform/MemeNativeApis.h"

namespace MemeUI {

namespace LinuxHostDetail {

// Minimal X11 ABI types (binary-compatible with Xlib64)
using XDisplay = void;
using XWindow = unsigned long;
using XVisualID = unsigned long;
using XColormap = unsigned long;
using XAtom = unsigned long;
using XGC = void *;

struct XSetWindowAttrs {
    unsigned long background_pixmap;
    unsigned long background_pixel;
    unsigned long border_pixmap;
    unsigned long border_pixel;
    int bit_gravity;
    int win_gravity;
    int backing_store;
    unsigned long backing_planes;
    unsigned long backing_pixel;
    int save_under;
    long event_mask;
    long do_not_propagate_mask;
    int override_redirect;
    XColormap colormap;
    unsigned long cursor;
};

struct XWinAttributes {
    int x, y, width, height, border_width, depth;
    void *visual;
    XWindow root;
    int c_class;
    int bit_gravity, win_gravity, backing_store;
    unsigned long backing_planes, backing_pixel;
    int save_under;
    XColormap colormap;
    int map_installed, map_state;
    long all_event_masks, your_event_mask, do_not_propagate_mask;
    int override_redirect;
    void *screen;
};

struct XImageRaw {
    int width, height, xoffset, format;
    char *data;
    int byte_order, bitmap_unit, bitmap_bit_order, bitmap_pad;
    int depth, bytes_per_line, bits_per_pixel;
    unsigned long red_mask, green_mask, blue_mask;
    void *obdata;
};

struct XKeyEv {
    int type;
    unsigned long serial;
    int send_event;
    XDisplay *display;
    XWindow window, root, subwindow;
    unsigned long time;
    int x, y, x_root, y_root;
    unsigned int state, keycode, same_screen;
};

struct XButtonEv {
    int type;
    unsigned long serial;
    int send_event;
    XDisplay *display;
    XWindow window, root, subwindow;
    unsigned long time;
    int x, y, x_root, y_root;
    unsigned int state, button, same_screen;
};

struct XMotionEv {
    int type;
    unsigned long serial;
    int send_event;
    XDisplay *display;
    XWindow window, root, subwindow;
    unsigned long time;
    int x, y, x_root, y_root;
    unsigned int state;
    char is_hint;
    int same_screen;
};

struct XConfigureEv {
    int type;
    unsigned long serial;
    int send_event;
    XDisplay *display;
    XWindow event, window;
    int x, y, width, height, border_width;
    XWindow above;
    int override_redirect;
};

struct XClientMsgEv {
    int type;
    unsigned long serial;
    int send_event;
    XDisplay *display;
    XWindow window;
    XAtom message_type;
    int format;
    union {
        char b[20];
        short s[10];
        long l[5];
    } data;
};

union XEvUnion {
    int type;
    XKeyEv xkey;
    XButtonEv xbutton;
    XMotionEv xmotion;
    XConfigureEv xconfigure;
    XClientMsgEv xclient;
    long pad[24];
};

inline void rasterizeMeshSoftware(
    const UiMesh &mesh, int width, int height, std::vector<uint32_t> &fb)
{
    fb.assign(static_cast<size_t>(width) * static_cast<size_t>(height), 0xFF170602u); // #020617 in BGRA
    if (mesh.vertices.empty() || mesh.indices.empty()) return;
    const auto &atlas = FontAtlas::getAlphaAtlas();

    for (const auto &batch : mesh.batches) {
        int cX0 = std::max(0, static_cast<int>(std::floor(batch.clipRect.x)));
        int cY0 = std::max(0, static_cast<int>(std::floor(batch.clipRect.y)));
        int cX1 = std::min(width, static_cast<int>(std::ceil(batch.clipRect.x + batch.clipRect.width)));
        int cY1 = std::min(height, static_cast<int>(std::ceil(batch.clipRect.y + batch.clipRect.height)));
        if (cX1 <= cX0 || cY1 <= cY0) continue;

        for (uint32_t ii = batch.indexOffset; ii + 2 < batch.indexOffset + batch.indexCount; ii += 3) {
            const auto &v0 = mesh.vertices[mesh.indices[ii + 0]];
            const auto &v1 = mesh.vertices[mesh.indices[ii + 1]];
            const auto &v2 = mesh.vertices[mesh.indices[ii + 2]];

            float area = (v1.x - v0.x) * (v2.y - v0.y) - (v2.x - v0.x) * (v1.y - v0.y);
            if (std::abs(area) < 1e-4f) continue;

            int minX = std::max(cX0, static_cast<int>(std::floor(std::min({v0.x, v1.x, v2.x}))));
            int maxX = std::min(cX1 - 1, static_cast<int>(std::ceil(std::max({v0.x, v1.x, v2.x}))));
            int minY = std::max(cY0, static_cast<int>(std::floor(std::min({v0.y, v1.y, v2.y}))));
            int maxY = std::min(cY1 - 1, static_cast<int>(std::ceil(std::max({v0.y, v1.y, v2.y}))));

            for (int py = minY; py <= maxY; ++py) {
                for (int px = minX; px <= maxX; ++px) {
                    float fx = static_cast<float>(px) + 0.5f;
                    float fy = static_cast<float>(py) + 0.5f;
                    float w0 = ((v1.x - fx) * (v2.y - fy) - (v2.x - fx) * (v1.y - fy)) / area;
                    float w1 = ((v2.x - fx) * (v0.y - fy) - (v0.x - fx) * (v2.y - fy)) / area;
                    float w2 = 1.0f - w0 - w1;
                    if (w0 < -0.001f || w1 < -0.001f || w2 < -0.001f) continue;

                    float u = w0 * v0.u + w1 * v1.u + w2 * v2.u;
                    float v = w0 * v0.v + w1 * v1.v + w2 * v2.v;
                    float sxu = std::clamp(u * FontAtlas::ATLAS_W - 0.5f, 0.0f, static_cast<float>(FontAtlas::ATLAS_W - 1));
                    float syv = std::clamp(v * FontAtlas::ATLAS_H - 0.5f, 0.0f, static_cast<float>(FontAtlas::ATLAS_H - 1));
                    int ax0 = static_cast<int>(sxu);
                    int ay0 = static_cast<int>(syv);
                    int ax1 = std::min(FontAtlas::ATLAS_W - 1, ax0 + 1);
                    int ay1 = std::min(FontAtlas::ATLAS_H - 1, ay0 + 1);
                    float fxA = sxu - ax0, fyA = syv - ay0;
                    float a00 = static_cast<float>(atlas[static_cast<size_t>(ay0) * FontAtlas::ATLAS_W + ax0]);
                    float a10 = static_cast<float>(atlas[static_cast<size_t>(ay0) * FontAtlas::ATLAS_W + ax1]);
                    float a01 = static_cast<float>(atlas[static_cast<size_t>(ay1) * FontAtlas::ATLAS_W + ax0]);
                    float a11 = static_cast<float>(atlas[static_cast<size_t>(ay1) * FontAtlas::ATLAS_W + ax1]);
                    float ta = ((a00 * (1.0f - fxA) + a10 * fxA) * (1.0f - fyA) + (a01 * (1.0f - fxA) + a11 * fxA) * fyA) / 255.0f;
                    if (ta <= 0.001f) continue;

                    auto unpack = [](uint32_t c, float ch[4]) {
                        ch[0] = static_cast<float>(c & 0xFF) / 255.0f;
                        ch[1] = static_cast<float>((c >> 8) & 0xFF) / 255.0f;
                        ch[2] = static_cast<float>((c >> 16) & 0xFF) / 255.0f;
                        ch[3] = static_cast<float>((c >> 24) & 0xFF) / 255.0f;
                    };
                    float c0[4], c1[4], c2[4];
                    unpack(v0.rgba, c0); unpack(v1.rgba, c1); unpack(v2.rgba, c2);
                    float sr = w0 * c0[0] + w1 * c1[0] + w2 * c2[0];
                    float sg = w0 * c0[1] + w1 * c1[1] + w2 * c2[1];
                    float sb = w0 * c0[2] + w1 * c1[2] + w2 * c2[2];
                    float sa = (w0 * c0[3] + w1 * c1[3] + w2 * c2[3]) * ta;
                    if (sa <= 0.004f) continue;

                    uint32_t &dst = fb[static_cast<size_t>(py) * width + px];
                    float db = static_cast<float>(dst & 0xFF) / 255.0f;
                    float dg = static_cast<float>((dst >> 8) & 0xFF) / 255.0f;
                    float dr = static_cast<float>((dst >> 16) & 0xFF) / 255.0f;
                    float inv = 1.0f - sa;
                    uint8_t ob = static_cast<uint8_t>(std::clamp((sb * sa + db * inv) * 255.0f, 0.0f, 255.0f));
                    uint8_t og = static_cast<uint8_t>(std::clamp((sg * sa + dg * inv) * 255.0f, 0.0f, 255.0f));
                    uint8_t orr = static_cast<uint8_t>(std::clamp((sr * sa + dr * inv) * 255.0f, 0.0f, 255.0f));
                    dst = (0xFFu << 24) | (static_cast<uint32_t>(orr) << 16) | (static_cast<uint32_t>(og) << 8) | ob;
                }
            }
        }
    }
}

} // namespace LinuxHostDetail

inline int runLinuxNativeApp(const std::string &title, UiSession &session, int width = 1040, int height = 720, std::function<void()> onRender = nullptr) {
    if (std::getenv("MEME_HEADLESS") != nullptr || std::getenv("DISPLAY") == nullptr) {
        if (onRender) onRender();
        else session.renderFrame(static_cast<float>(width), static_cast<float>(height));
        return 0;
    }

    void *x11 = dlopen("libX11.so.6", RTLD_LAZY);
    if (!x11) {
        if (onRender) onRender();
        else session.renderFrame(static_cast<float>(width), static_cast<float>(height));
        return 0;
    }

    using namespace LinuxHostDetail;
    auto pXOpenDisplay = reinterpret_cast<XDisplay *(*)(const char *)>(dlsym(x11, "XOpenDisplay"));
    auto pXCloseDisplay = reinterpret_cast<int (*)(XDisplay *)>(dlsym(x11, "XCloseDisplay"));
    auto pXDefaultScreen = reinterpret_cast<int (*)(XDisplay *)>(dlsym(x11, "XDefaultScreen"));
    auto pXRootWindow = reinterpret_cast<XWindow (*)(XDisplay *, int)>(dlsym(x11, "XRootWindow"));
    auto pXCreateSimpleWindow = reinterpret_cast<XWindow (*)(XDisplay *, XWindow, int, int, unsigned int, unsigned int, unsigned int, unsigned long, unsigned long)>(dlsym(x11, "XCreateSimpleWindow"));
    auto pXStoreName = reinterpret_cast<int (*)(XDisplay *, XWindow, const char *)>(dlsym(x11, "XStoreName"));
    auto pXSelectInput = reinterpret_cast<int (*)(XDisplay *, XWindow, long)>(dlsym(x11, "XSelectInput"));
    auto pXMapWindow = reinterpret_cast<int (*)(XDisplay *, XWindow)>(dlsym(x11, "XMapWindow"));
    auto pXInternAtom = reinterpret_cast<XAtom (*)(XDisplay *, const char *, int)>(dlsym(x11, "XInternAtom"));
    auto pXSetWMProtocols = reinterpret_cast<int (*)(XDisplay *, XWindow, XAtom *, int)>(dlsym(x11, "XSetWMProtocols"));
    auto pXPending = reinterpret_cast<int (*)(XDisplay *)>(dlsym(x11, "XPending"));
    auto pXNextEvent = reinterpret_cast<int (*)(XDisplay *, XEvUnion *)>(dlsym(x11, "XNextEvent"));
    auto pXDefaultGC = reinterpret_cast<XGC (*)(XDisplay *, int)>(dlsym(x11, "XDefaultGC"));
    auto pXDefaultVisual = reinterpret_cast<void *(*)(XDisplay *, int)>(dlsym(x11, "XDefaultVisual"));
    auto pXDefaultDepth = reinterpret_cast<int (*)(XDisplay *, int)>(dlsym(x11, "XDefaultDepth"));
    auto pXCreateImage = reinterpret_cast<XImageRaw *(*)(XDisplay *, void *, unsigned int, int, int, char *, unsigned int, unsigned int, int, int)>(dlsym(x11, "XCreateImage"));
    auto pXDestroyImage = reinterpret_cast<int (*)(XImageRaw *)>(dlsym(x11, "XDestroyImage"));
    auto pXPutImage = reinterpret_cast<int (*)(XDisplay *, XWindow, XGC, XImageRaw *, int, int, int, int, unsigned int, unsigned int)>(dlsym(x11, "XPutImage"));
    auto pXLookupString = reinterpret_cast<int (*)(XKeyEv *, char *, int, unsigned long *, void *)>(dlsym(x11, "XLookupString"));
    auto pXFlush = reinterpret_cast<int (*)(XDisplay *)>(dlsym(x11, "XFlush"));
    auto pXChangeProperty = reinterpret_cast<int (*)(XDisplay *, XWindow, XAtom, XAtom, int, int, const unsigned char *, int)>(dlsym(x11, "XChangeProperty"));
    auto pXResizeWindow = reinterpret_cast<int (*)(XDisplay *, XWindow, unsigned int, unsigned int)>(dlsym(x11, "XResizeWindow"));
    auto pXIconifyWindow = reinterpret_cast<int (*)(XDisplay *, XWindow, int)>(dlsym(x11, "XIconifyWindow"));
    auto pXRaiseWindow = reinterpret_cast<int (*)(XDisplay *, XWindow)>(dlsym(x11, "XRaiseWindow"));

    // Dynamic EGL + GLESv2 probe (Wayland/X11 GPU path with automatic XPutImage software fallback)
    void *eglLib = dlopen("libEGL.so.1", RTLD_LAZY);
    void *glLib = dlopen("libGLESv2.so.2", RTLD_LAZY);
    void *eglDpy = nullptr, *eglSurf = nullptr, *eglCtx = nullptr;
    unsigned int glProg = 0, glTex = 0, glVbo = 0, glIbo = 0;
    int locPos = -1, locUv = -1, locCol = -1, locVp = -1, locAtl = -1;
    bool hasGles = false;

    using EglGetDisplayFn = void *(*)(void *);
    using EglInitFn = int (*)(void *, int *, int *);
    using EglChooseCfgFn = int (*)(void *, const int *, void **, int, int *);
    using EglCreateSurfFn = void *(*)(void *, void *, XWindow, const int *);
    using EglCreateCtxFn = void *(*)(void *, void *, void *, const int *);
    using EglMakeCurFn = int (*)(void *, void *, void *, void *);
    using EglSwapFn = int (*)(void *, void *);
    using GlUintVoidFn = unsigned int (*)(unsigned int);
    using GlShaderSrcFn = void (*)(unsigned int, int, const char **, const int *);
    using GlVoidUintFn = void (*)(unsigned int);
    using GlLinkFn = unsigned int (*)();
    using GlAttachFn = void (*)(unsigned int, unsigned int);
    using GlLocFn = int (*)(unsigned int, const char *);
    using GlGenFn = void (*)(int, unsigned int *);
    using GlBindFn = void (*)(unsigned int, unsigned int);
    using GlTexImgFn = void (*)(unsigned int, int, int, int, int, int, unsigned int, unsigned int, const void *);
    using GlTexParamFn = void (*)(unsigned int, unsigned int, int);
    using GlBufDataFn = void (*)(unsigned int, ptrdiff_t, const void *, unsigned int);
    using GlVpFn = void (*)(int, int, int, int);
    using GlClearColFn = void (*)(float, float, float, float);
    using GlClearFn = void (*)(unsigned int);
    using GlEnableFn = void (*)(unsigned int);
    using GlBlendFn = void (*)(unsigned int, unsigned int);
    using GlUnif2fFn = void (*)(int, float, float);
    using GlUnif1iFn = void (*)(int, int);
    using GlAttribPtrFn = void (*)(unsigned int, int, unsigned int, unsigned char, int, const void *);
    using GlDrawElemFn = void (*)(unsigned int, int, unsigned int, const void *);

    auto pEglSwap = eglLib ? reinterpret_cast<EglSwapFn>(dlsym(eglLib, "eglSwapBuffers")) : nullptr;
    auto pGlVp = glLib ? reinterpret_cast<GlVpFn>(dlsym(glLib, "glViewport")) : nullptr;
    auto pGlClearCol = glLib ? reinterpret_cast<GlClearColFn>(dlsym(glLib, "glClearColor")) : nullptr;
    auto pGlClear = glLib ? reinterpret_cast<GlClearFn>(dlsym(glLib, "glClear")) : nullptr;
    auto pGlEnable = glLib ? reinterpret_cast<GlEnableFn>(dlsym(glLib, "glEnable")) : nullptr;
    auto pGlDisable = glLib ? reinterpret_cast<GlEnableFn>(dlsym(glLib, "glDisable")) : nullptr;
    auto pGlBlend = glLib ? reinterpret_cast<GlBlendFn>(dlsym(glLib, "glBlendFunc")) : nullptr;
    auto pGlUseProg = glLib ? reinterpret_cast<GlVoidUintFn>(dlsym(glLib, "glUseProgram")) : nullptr;
    auto pGlBindBuf = glLib ? reinterpret_cast<GlBindFn>(dlsym(glLib, "glBindBuffer")) : nullptr;
    auto pGlBufData = glLib ? reinterpret_cast<GlBufDataFn>(dlsym(glLib, "glBufferData")) : nullptr;
    auto pGlBindTex = glLib ? reinterpret_cast<GlBindFn>(dlsym(glLib, "glBindTexture")) : nullptr;
    auto pGlUnif2f = glLib ? reinterpret_cast<GlUnif2fFn>(dlsym(glLib, "glUniform2f")) : nullptr;
    auto pGlUnif1i = glLib ? reinterpret_cast<GlUnif1iFn>(dlsym(glLib, "glUniform1i")) : nullptr;
    auto pGlEnableAttrib = glLib ? reinterpret_cast<GlVoidUintFn>(dlsym(glLib, "glEnableVertexAttribArray")) : nullptr;
    auto pGlAttribPtr = glLib ? reinterpret_cast<GlAttribPtrFn>(dlsym(glLib, "glVertexAttribPointer")) : nullptr;
    auto pGlScissor = glLib ? reinterpret_cast<GlVpFn>(dlsym(glLib, "glScissor")) : nullptr;
    auto pGlDrawElem = glLib ? reinterpret_cast<GlDrawElemFn>(dlsym(glLib, "glDrawElements")) : nullptr;

    if (!pXOpenDisplay || !pXCreateSimpleWindow) { dlclose(x11); return 0; }
    XDisplay *dpy = pXOpenDisplay(nullptr);
    if (!dpy) { dlclose(x11); return 0; }

    int scr = pXDefaultScreen(dpy);
    XWindow root = pXRootWindow(dpy, scr);
    XWindow win = pXCreateSimpleWindow(dpy, root, 80, 80, static_cast<unsigned int>(width), static_cast<unsigned int>(height), 1, 0, 0x020617);
    if (pXStoreName) pXStoreName(dpy, win, title.c_str());

    long mask = (1L<<0) | (1L<<1) | (1L<<2) | (1L<<3) | (1L<<6) | (1L<<15) | (1L<<17);
    if (pXSelectInput) pXSelectInput(dpy, win, mask);
    XAtom wmDel = pXInternAtom ? pXInternAtom(dpy, "WM_DELETE_WINDOW", 0) : 0;
    if (wmDel && pXSetWMProtocols) pXSetWMProtocols(dpy, win, &wmDel, 1);

    // Apply _NET_WM_ICON (w, h, ARGB unsigned long pixels)
    if (pXInternAtom && pXChangeProperty) {
        XAtom netWmIcon = pXInternAtom(dpy, "_NET_WM_ICON", 0);
        std::vector<uint8_t> rgba;
        int iw = 0, ih = 0;
        MemeNative::ensureDefaultAppIconRgba(64, rgba, iw, ih);
        if (netWmIcon && !rgba.empty() && iw > 0 && ih > 0) {
            std::vector<unsigned long> data;
            data.reserve(2 + static_cast<size_t>(iw) * static_cast<size_t>(ih));
            data.push_back(static_cast<unsigned long>(iw));
            data.push_back(static_cast<unsigned long>(ih));
            for (size_t i = 0; i + 3 < rgba.size(); i += 4) {
                unsigned long px = (static_cast<unsigned long>(rgba[i + 3]) << 24) |
                                   (static_cast<unsigned long>(rgba[i + 0]) << 16) |
                                   (static_cast<unsigned long>(rgba[i + 1]) << 8) |
                                   static_cast<unsigned long>(rgba[i + 2]);
                data.push_back(px);
            }
            pXChangeProperty(dpy, win, netWmIcon, 6 /*XA_CARDINAL*/, 32, 0 /*PropModeReplace*/,
                             reinterpret_cast<const unsigned char *>(data.data()), static_cast<int>(data.size()));
        }
    }

    int curW = width, curH = height;
    auto &nh = MemeNative::hooks();
    nh.requestRerender = [&session]() { session.needsRender = true; };
    nh.getScreenInfo = [&curW, &curH]() -> MemeNative::ScreenInfo {
        MemeNative::ScreenInfo s;
        s.width = static_cast<float>(curW);
        s.height = static_cast<float>(curH);
        s.scale = 1.0f;
        return s;
    };
    nh.setWindowTitle = [pXStoreName, pXFlush, dpy, win](const std::string &t) -> bool {
        if (!pXStoreName) return false;
        pXStoreName(dpy, win, t.c_str());
        if (pXFlush) pXFlush(dpy);
        return true;
    };
    nh.setWindowSize = [pXResizeWindow, pXFlush, dpy, win](int w, int hSz) -> bool {
        if (!pXResizeWindow) return false;
        pXResizeWindow(dpy, win, static_cast<unsigned int>(std::max(160, w)), static_cast<unsigned int>(std::max(120, hSz)));
        if (pXFlush) pXFlush(dpy);
        return true;
    };
    nh.minimizeWindow = [pXIconifyWindow, pXFlush, dpy, win, scr]() -> bool {
        if (!pXIconifyWindow) return false;
        pXIconifyWindow(dpy, win, scr);
        if (pXFlush) pXFlush(dpy);
        return true;
    };
    nh.focusWindow = [pXRaiseWindow, pXFlush, dpy, win]() -> bool {
        if (!pXRaiseWindow) return false;
        pXRaiseWindow(dpy, win);
        if (pXFlush) pXFlush(dpy);
        return true;
    };
    nh.checkPermission = [](const std::string &rawCap) -> MemePermissions::PermissionStatus {
        std::string cap = MemePermissions::normalizeCapability(rawCap);
        MemePermissions::PermissionStatus out;
        out.permission = cap;
        out.platform = "linux";
        if (cap == "clipboard" || cap == "storage" || cap == "notifications" || cap == "sensors") {
            out.state = "granted";
        } else {
            out.state = MemePermissions::isSupportedOnPlatform(cap, "linux") ? "prompt" : "unavailable";
        }
        out.finalizeFlags();
        return out;
    };
    nh.requestPermissionAsync = [](const std::string &rawCap, MemeNative::AsyncCallback cb) {
        std::string cap = MemePermissions::normalizeCapability(rawCap);
        MemeNative::AsyncResponse r;
        int code = 1;
        std::string msg = MemePermissions::getPermissionDescription(cap, "MEME App") + " Allow '" + cap + "'?";
        if (std::getenv("DISPLAY") || std::getenv("WAYLAND_DISPLAY")) {
            MemeNative::detail::execArgv({"zenity", "--question", "--title=Permission Request", "--text=" + msg}, nullptr, nullptr, &code);
            r.ok = (code == 0);
        } else {
            r.ok = MemePermissions::isSupportedOnPlatform(cap, "linux");
        }
        r.accepted = r.ok;
        r.permissionState = r.ok ? "granted" : "denied";
        r.value = r.permissionState;
        MemeNative::dispatchCallbackOnUiThread(cb, r);
    };
    nh.openAppSettings = [](const std::string &) -> bool {
        return MemeNative::detail::execArgv({"gnome-control-center", "privacy"}, nullptr, nullptr, nullptr, true) ||
               MemeNative::detail::execArgv({"xdg-open", "settings://privacy"}, nullptr, nullptr, nullptr, true);
    };

    if (pXMapWindow) pXMapWindow(dpy, win);

    if (eglLib && glLib && pEglSwap && pGlVp && pGlClear && pGlDrawElem) {
        auto pEglGetDpy = reinterpret_cast<EglGetDisplayFn>(dlsym(eglLib, "eglGetDisplay"));
        auto pEglInit = reinterpret_cast<EglInitFn>(dlsym(eglLib, "eglInitialize"));
        auto pEglChoose = reinterpret_cast<EglChooseCfgFn>(dlsym(eglLib, "eglChooseConfig"));
        auto pEglSurf = reinterpret_cast<EglCreateSurfFn>(dlsym(eglLib, "eglCreateWindowSurface"));
        auto pEglCtx = reinterpret_cast<EglCreateCtxFn>(dlsym(eglLib, "eglCreateContext"));
        auto pEglMake = reinterpret_cast<EglMakeCurFn>(dlsym(eglLib, "eglMakeCurrent"));
        if (pEglGetDpy && pEglInit && pEglChoose && pEglSurf && pEglCtx && pEglMake) {
            eglDpy = pEglGetDpy(dpy);
            if (eglDpy && pEglInit(eglDpy, nullptr, nullptr)) {
                const int att[] = {0x3040, 4 /*GLES2*/, 0x3033, 4 /*WINDOW*/, 0x3024, 8, 0x3023, 8, 0x3022, 8, 0x3038};
                void *cfg = nullptr; int n = 0;
                const int catt[] = {0x3098, 2, 0x3038};
                if (pEglChoose(eglDpy, att, &cfg, 1, &n) && n > 0) {
                    eglSurf = pEglSurf(eglDpy, cfg, win, nullptr);
                    eglCtx = pEglCtx(eglDpy, cfg, nullptr, catt);
                    if (eglSurf && eglCtx && pEglMake(eglDpy, eglSurf, eglSurf, eglCtx)) {
                        auto pCreateShader = reinterpret_cast<GlUintVoidFn>(dlsym(glLib, "glCreateShader"));
                        auto pShaderSrc = reinterpret_cast<GlShaderSrcFn>(dlsym(glLib, "glShaderSource"));
                        auto pCompile = reinterpret_cast<GlVoidUintFn>(dlsym(glLib, "glCompileShader"));
                        auto pCreateProg = reinterpret_cast<GlLinkFn>(dlsym(glLib, "glCreateProgram"));
                        auto pAttach = reinterpret_cast<GlAttachFn>(dlsym(glLib, "glAttachShader"));
                        auto pLink = reinterpret_cast<GlVoidUintFn>(dlsym(glLib, "glLinkProgram"));
                        auto pAttribLoc = reinterpret_cast<GlLocFn>(dlsym(glLib, "glGetAttribLocation"));
                        auto pUnifLoc = reinterpret_cast<GlLocFn>(dlsym(glLib, "glGetUniformLocation"));
                        auto pGenTex = reinterpret_cast<GlGenFn>(dlsym(glLib, "glGenTextures"));
                        auto pTexParam = reinterpret_cast<GlTexParamFn>(dlsym(glLib, "glTexParameteri"));
                        auto pTexImg = reinterpret_cast<GlTexImgFn>(dlsym(glLib, "glTexImage2D"));
                        auto pGenBuf = reinterpret_cast<GlGenFn>(dlsym(glLib, "glGenBuffers"));
                        if (pCreateShader && pCreateProg && pGenTex && pGenBuf) {
                            const char *vs = "attribute vec2 aPos; attribute vec2 aUv; attribute vec4 aCol; uniform vec2 uVp; varying vec2 vUv; varying vec4 vCol; void main(){ gl_Position=vec4((aPos.x/max(1.0,uVp.x))*2.0-1.0, 1.0-(aPos.y/max(1.0,uVp.y))*2.0, 0.0, 1.0); vUv=aUv; vCol=aCol; }";
                            const char *fs = "precision mediump float; uniform sampler2D uAtl; varying vec2 vUv; varying vec4 vCol; void main(){ vec4 t=texture2D(uAtl,vUv); gl_FragColor=vec4(vCol.rgb*t.rgb, vCol.a*t.a); }";
                            unsigned int v = pCreateShader(0x8B31), f = pCreateShader(0x8B30);
                            pShaderSrc(v, 1, &vs, nullptr); pCompile(v);
                            pShaderSrc(f, 1, &fs, nullptr); pCompile(f);
                            glProg = pCreateProg(); pAttach(glProg, v); pAttach(glProg, f); pLink(glProg);
                            locPos = pAttribLoc(glProg, "aPos"); locUv = pAttribLoc(glProg, "aUv"); locCol = pAttribLoc(glProg, "aCol");
                            locVp = pUnifLoc(glProg, "uVp"); locAtl = pUnifLoc(glProg, "uAtl");
                            pGenTex(1, &glTex); pGlBindTex(0x0DE1, glTex);
                            pTexParam(0x0DE1, 0x2801, 0x2601); pTexParam(0x0DE1, 0x2800, 0x2601);
                            auto rgba = FontAtlas::getRgbaAtlas();
                            pTexImg(0x0DE1, 0, 0x1908, FontAtlas::ATLAS_W, FontAtlas::ATLAS_H, 0, 0x1908, 0x1401, rgba.data());
                            pGenBuf(1, &glVbo); pGenBuf(1, &glIbo);
                            hasGles = true;
                        }
                    }
                }
            }
        }
    }

    session.input.viewportW = static_cast<float>(width);
    session.input.viewportH = static_cast<float>(height);
    if (onRender) onRender();
    else session.renderFrame(static_cast<float>(width), static_cast<float>(height));

    std::vector<uint32_t> fb;
    bool open = true;
    while (open) {
        while (pXPending && pXPending(dpy) > 0) {
            XEvUnion ev{};
            pXNextEvent(dpy, &ev);
            if (ev.type == 33 && static_cast<XAtom>(ev.xclient.data.l[0]) == wmDel) {
                open = false;
            } else if (ev.type == 22) { // ConfigureNotify
                if (ev.xconfigure.width > 1 && ev.xconfigure.height > 1) {
                    curW = ev.xconfigure.width; curH = ev.xconfigure.height;
                    session.input.viewportW = static_cast<float>(curW);
                    session.input.viewportH = static_cast<float>(curH);
                    session.needsRender = true;
                }
            } else if (ev.type == 6) { // MotionNotify
                if (session.handlePointerMove(static_cast<float>(ev.xmotion.x), static_cast<float>(ev.xmotion.y))) {
                    if (onRender) onRender();
                }
            } else if (ev.type == 4) { // ButtonPress
                float px = static_cast<float>(ev.xbutton.x), py = static_cast<float>(ev.xbutton.y);
                if (ev.xbutton.button == 4) session.handleScroll(px, py, 0.0f, 3.0f);
                else if (ev.xbutton.button == 5) session.handleScroll(px, py, 0.0f, -3.0f);
                else if (ev.xbutton.button == 6) session.handleScroll(px, py, 3.0f, 0.0f);
                else if (ev.xbutton.button == 7) session.handleScroll(px, py, -3.0f, 0.0f);
                else session.handlePointerDown(px, py, ev.xbutton.button == 3 ? 2 : 0, 1);
                if (onRender) onRender();
            } else if (ev.type == 5) { // ButtonRelease
                if (ev.xbutton.button <= 3) {
                    session.handlePointerUp(static_cast<float>(ev.xbutton.x), static_cast<float>(ev.xbutton.y), ev.xbutton.button == 3 ? 2 : 0);
                    if (onRender) onRender();
                }
            } else if (ev.type == 2) { // KeyPress
                char buf[16] = {0};
                unsigned long ksym = 0;
                int len = pXLookupString ? pXLookupString(&ev.xkey, buf, sizeof(buf) - 1, &ksym, nullptr) : 0;
                UiKeyEvent kev;
                kev.shift = (ev.xkey.state & 1u) != 0;
                kev.ctrl = (ev.xkey.state & 4u) != 0;
                kev.alt = (ev.xkey.state & 8u) != 0;
                if (ksym == 0xFF51) kev.key = "ArrowLeft";
                else if (ksym == 0xFF52) kev.key = "ArrowUp";
                else if (ksym == 0xFF53) kev.key = "ArrowRight";
                else if (ksym == 0xFF54) kev.key = "ArrowDown";
                else if (ksym == 0xFF08) kev.key = "Backspace";
                else if (ksym == 0xFFFF) kev.key = "Delete";
                else if (ksym == 0xFF0D) kev.key = "Enter";
                else if (ksym == 0xFF09) kev.key = "Tab";
                else if (ksym == 0xFF1B) kev.key = "Escape";
                else if (ksym == 0xFF50) kev.key = "Home";
                else if (ksym == 0xFF57) kev.key = "End";
                else if (len > 0) {
                    kev.key = std::string(buf, static_cast<size_t>(len));
                    if (!kev.ctrl && static_cast<unsigned char>(buf[0]) >= 32) kev.text = kev.key;
                }
                if (!kev.key.empty() && session.handleKeyEvent(kev)) {
                    if (onRender) onRender();
                }
            }
        }

        if (session.tick(16.0) || session.isImmediateMode || session.needsRender) {
            if (onRender) onRender();
            else session.renderFrame(static_cast<float>(curW), static_cast<float>(curH));
        }

        if (hasGles) {
            pGlVp(0, 0, curW, curH);
            pGlClearCol(0.008f, 0.024f, 0.09f, 1.0f);
            pGlClear(0x4000u);
            pGlEnable(0x0BE2u);
            pGlBlend(0x0302u, 0x0303u);
            pGlEnable(0x0C11u);
            const auto &mesh = session.lastMesh;
            if (!mesh.vertices.empty() && !mesh.indices.empty()) {
                pGlUseProg(glProg);
                pGlUnif2f(locVp, static_cast<float>(curW), static_cast<float>(curH));
                pGlBindTex(0x0DE1u, glTex);
                pGlUnif1i(locAtl, 0);
                pGlBindBuf(0x8892u, glVbo);
                pGlBufData(0x8892u, static_cast<ptrdiff_t>(mesh.vertices.size() * sizeof(UiVertex)), mesh.vertices.data(), 0x88E0u);
                pGlBindBuf(0x8893u, glIbo);
                pGlBufData(0x8893u, static_cast<ptrdiff_t>(mesh.indices.size() * sizeof(uint32_t)), mesh.indices.data(), 0x88E0u);
                constexpr int st = sizeof(UiVertex);
                pGlEnableAttrib(static_cast<unsigned int>(locPos));
                pGlAttribPtr(static_cast<unsigned int>(locPos), 2, 0x1406u, 0, st, reinterpret_cast<const void *>(0));
                pGlEnableAttrib(static_cast<unsigned int>(locUv));
                pGlAttribPtr(static_cast<unsigned int>(locUv), 2, 0x1406u, 0, st, reinterpret_cast<const void *>(8));
                pGlEnableAttrib(static_cast<unsigned int>(locCol));
                pGlAttribPtr(static_cast<unsigned int>(locCol), 4, 0x1401u, 1, st, reinterpret_cast<const void *>(16));
                for (const auto &b : mesh.batches) {
                    if (b.indexCount == 0) continue;
                    int sx = std::clamp(static_cast<int>(b.clipRect.x), 0, curW);
                    int ex = std::clamp(static_cast<int>(b.clipRect.x + b.clipRect.width), 0, curW);
                    int sy = std::clamp(static_cast<int>(b.clipRect.y), 0, curH);
                    int ey = std::clamp(static_cast<int>(b.clipRect.y + b.clipRect.height), 0, curH);
                    if (ex <= sx || ey <= sy) continue;
                    pGlScissor(sx, curH - ey, ex - sx, ey - sy);
                    pGlDrawElem(4u, static_cast<int>(b.indexCount), 0x1405u, reinterpret_cast<const void *>(static_cast<uintptr_t>(b.indexOffset * sizeof(uint32_t))));
                }
            }
            pGlDisable(0x0C11u);
            pEglSwap(eglDpy, eglSurf);
        } else if (pXCreateImage && pXPutImage && pXDefaultVisual && pXDefaultGC && pXDefaultDepth) {
            rasterizeMeshSoftware(session.lastMesh, curW, curH, fb);
            XImageRaw *img = pXCreateImage(dpy, pXDefaultVisual(dpy, scr), static_cast<unsigned int>(pXDefaultDepth(dpy, scr)),
                                           2 /*ZPixmap*/, 0, reinterpret_cast<char *>(fb.data()),
                                           static_cast<unsigned int>(curW), static_cast<unsigned int>(curH), 32, 0);
            if (img) {
                pXPutImage(dpy, win, pXDefaultGC(dpy, scr), img, 0, 0, 0, 0, static_cast<unsigned int>(curW), static_cast<unsigned int>(curH));
                img->data = nullptr;
                if (pXDestroyImage) pXDestroyImage(img);
                else free(img);
            }
            if (pXFlush) pXFlush(dpy);
        }
        usleep(16000);
    }

    if (glLib) dlclose(glLib);
    if (eglLib) dlclose(eglLib);
    pXCloseDisplay(dpy);
    dlclose(x11);
    return 0;
}

} // namespace MemeUI

#endif // __linux__ && !__ANDROID__
