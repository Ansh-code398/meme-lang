#pragma once

#if defined(_WIN32)

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <windowsx.h>
#ifdef RELATIVE
#undef RELATIVE
#endif
#ifdef ABSOLUTE
#undef ABSOLUTE
#endif
#ifdef DrawText
#undef DrawText
#endif
#include <d2d1.h>
#include <dwrite.h>
#include <commdlg.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

#include "../../ui/MemeUiTypes.h"
#include "../../ui/MemeUiTessellator.h"
#include "../../ui/MemeUiRuntime.h"
#include "../../platform/MemeNativeApis.h"

namespace MemeUI {

namespace WinHostDetail {

inline std::wstring utf8ToWide(const std::string &str) {
    if (str.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), nullptr, 0);
    if (n <= 0) return L"";
    std::wstring w(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), &w[0], n);
    return w;
}

inline std::string wideToUtf8(const std::wstring &w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    if (n <= 0) return "";
    std::string s(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()), &s[0], n, nullptr, nullptr);
    return s;
}

inline D2D1_COLOR_F d2dColor(const std::string &hexStr, float alpha) {
    ColorRgba c = hexToRgba(hexStr, alpha);
    return D2D1_COLOR_F{c.r, c.g, c.b, c.a};
}

struct WinHostContext {
    UiSession *session = nullptr;
    std::function<void()> onNeedRender = nullptr;
    ID2D1Factory *d2dFactory = nullptr;
    IDWriteFactory *dwFactory = nullptr;
    ID2D1HwndRenderTarget *rt = nullptr;
    float dpiScale = 1.0f;
    bool open = true;
};

inline void recreateRenderTarget(WinHostContext *ctx, HWND hwnd) {
    if (!ctx || !ctx->d2dFactory) return;
    if (ctx->rt) { ctx->rt->Release(); ctx->rt = nullptr; }
    RECT rc{};
    GetClientRect(hwnd, &rc);
    D2D1_SIZE_U sz = D2D1_SIZE_U{
        static_cast<UINT32>(std::max<LONG>(1, rc.right - rc.left)),
        static_cast<UINT32>(std::max<LONG>(1, rc.bottom - rc.top))
    };
    D2D1_RENDER_TARGET_PROPERTIES rtProps{};
    rtProps.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    rtProps.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
    rtProps.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    rtProps.dpiX = 96.0f * ctx->dpiScale;
    rtProps.dpiY = 96.0f * ctx->dpiScale;
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps{};
    hwndProps.hwnd = hwnd;
    hwndProps.pixelSize = sz;
    hwndProps.presentOptions = D2D1_PRESENT_OPTIONS_NONE;
    ctx->d2dFactory->CreateHwndRenderTarget(&rtProps, &hwndProps, &ctx->rt);
}

inline void paintDirect2D(WinHostContext *ctx, HWND hwnd) {
    if (!ctx || !ctx->session) return;
    if (!ctx->rt) recreateRenderTarget(ctx, hwnd);
    if (!ctx->rt) return;

    RECT rc{};
    GetClientRect(hwnd, &rc);
    float vw = std::max(1.0f, static_cast<float>(rc.right - rc.left) / ctx->dpiScale);
    float vh = std::max(1.0f, static_cast<float>(rc.bottom - rc.top) / ctx->dpiScale);
    if (ctx->session->needsRender || ctx->session->lastCmds.empty()) {
        if (ctx->onNeedRender) ctx->onNeedRender();
        else ctx->session->renderFrame(vw, vh);
    }

    ctx->rt->BeginDraw();
    ctx->rt->Clear(D2D1_COLOR_F{0.008f, 0.024f, 0.09f, 1.0f});

    ID2D1SolidColorBrush *brush = nullptr;
    ctx->rt->CreateSolidColorBrush(D2D1_COLOR_F{1, 1, 1, 1}, &brush);

    int clipDepth = 0;
    for (const auto &c : ctx->session->lastCmds) {
        if (!brush) break;
        if (c.type == "PUSH_CLIP") {
            ctx->rt->PushAxisAlignedClip(D2D1_RECT_F{c.x, c.y, c.x + c.width, c.y + c.height}, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            clipDepth++;
        } else if (c.type == "POP_CLIP") {
            if (clipDepth > 0) {
                ctx->rt->PopAxisAlignedClip();
                clipDepth--;
            }
        } else if (c.type == "FILL_ROUND_RECT" || c.type == "SHADOW_ROUND_RECT" || c.type == "DRAW_SELECTION_RECT" || c.type == "DRAW_CARET") {
            float a = (c.type == "SHADOW_ROUND_RECT") ? c.alpha * 0.2f : c.alpha;
            brush->SetColor(d2dColor(c.color, a));
            D2D1_ROUNDED_RECT rr{D2D1_RECT_F{c.x, c.y, c.x + std::max(0.0f, c.width), c.y + std::max(0.0f, c.height)}, c.radius, c.radius};
            ctx->rt->FillRoundedRectangle(&rr, brush);
        } else if (c.type == "FILL_GRADIENT_RECT") {
            D2D1_GRADIENT_STOP stops[2] = {
                {0.0f, d2dColor(c.color, c.alpha)},
                {1.0f, d2dColor(c.color2, c.alpha2)}
            };
            ID2D1GradientStopCollection *gsc = nullptr;
            if (SUCCEEDED(ctx->rt->CreateGradientStopCollection(stops, 2, D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &gsc)) && gsc) {
                ID2D1LinearGradientBrush *lgb = nullptr;
                D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES lgp{D2D1_POINT_2F{c.x, c.y}, D2D1_POINT_2F{c.x + c.width, c.y + c.height}};
                D2D1_BRUSH_PROPERTIES bp{};
                bp.opacity = 1.0f;
                bp.transform._11 = 1.0f; bp.transform._22 = 1.0f;
                if (SUCCEEDED(ctx->rt->CreateLinearGradientBrush(&lgp, &bp, gsc, &lgb)) && lgb) {
                    D2D1_ROUNDED_RECT rr{D2D1_RECT_F{c.x, c.y, c.x + std::max(0.0f, c.width), c.y + std::max(0.0f, c.height)}, c.radius, c.radius};
                    ctx->rt->FillRoundedRectangle(&rr, lgb);
                    lgb->Release();
                }
                gsc->Release();
            }
        } else if (c.type == "STROKE_ROUND_RECT") {
            brush->SetColor(d2dColor(c.color, c.alpha));
            D2D1_ROUNDED_RECT rr{D2D1_RECT_F{c.x, c.y, c.x + std::max(0.0f, c.width), c.y + std::max(0.0f, c.height)}, c.radius, c.radius};
            ctx->rt->DrawRoundedRectangle(&rr, brush, std::max(0.5f, c.lineWidth));
        } else if (c.type == "DRAW_LINE") {
            brush->SetColor(d2dColor(c.color, c.alpha));
            ctx->rt->DrawLine(D2D1_POINT_2F{c.x, c.y}, D2D1_POINT_2F{c.x + c.width, c.y + c.height}, brush, std::max(0.5f, c.lineWidth));
        } else if (c.type == "DRAW_CIRCLE" || c.type == "STROKE_CIRCLE") {
            float cx = c.x + c.width * 0.5f, cy = c.y + c.height * 0.5f;
            float rx = c.width * 0.5f, ry = c.height * 0.5f;
            D2D1_ELLIPSE el{D2D1_POINT_2F{cx, cy}, rx, ry};
            brush->SetColor(d2dColor(c.color, c.alpha));
            if (c.type == "DRAW_CIRCLE" && c.filled) ctx->rt->FillEllipse(&el, brush);
            else ctx->rt->DrawEllipse(&el, brush, std::max(0.5f, c.lineWidth));
        } else if ((c.type == "DRAW_POLYLINE" || c.type == "FILL_POLYGON") && c.points.size() >= 2) {
            ID2D1PathGeometry *pg = nullptr;
            if (SUCCEEDED(ctx->d2dFactory->CreatePathGeometry(&pg)) && pg) {
                ID2D1GeometrySink *sink = nullptr;
                if (SUCCEEDED(pg->Open(&sink)) && sink) {
                    bool closed = (c.type == "FILL_POLYGON");
                    sink->BeginFigure(D2D1_POINT_2F{c.points[0].x, c.points[0].y}, closed ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW);
                    for (size_t i = 1; i < c.points.size(); ++i) sink->AddLine(D2D1_POINT_2F{c.points[i].x, c.points[i].y});
                    sink->EndFigure(closed ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
                    sink->Close();
                    sink->Release();
                    brush->SetColor(d2dColor(c.color, c.alpha));
                    if (closed) ctx->rt->FillGeometry(pg, brush);
                    else ctx->rt->DrawGeometry(pg, brush, std::max(0.5f, c.lineWidth));
                }
                pg->Release();
            }
        } else if (c.type == "DRAW_TEXT" && !c.text.empty() && ctx->dwFactory) {
            std::wstring wtxt = utf8ToWide(c.text);
            IDWriteTextFormat *fmt = nullptr;
            const wchar_t *fam = c.isMono ? L"Consolas" : L"Segoe UI";
            DWRITE_FONT_WEIGHT fw = c.fontWeight >= 600 ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
            if (SUCCEEDED(ctx->dwFactory->CreateTextFormat(fam, nullptr, fw, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, std::max(6.0f, c.fontSize), L"en-us", &fmt)) && fmt) {
                brush->SetColor(d2dColor(c.color, c.alpha));
                float top = c.y - c.fontSize * 0.9f;
                ctx->rt->DrawText(wtxt.c_str(), static_cast<UINT32>(wtxt.size()), fmt,
                                  D2D1_RECT_F{c.x, top, c.x + 10000.0f, top + c.fontSize * 2.0f}, brush);
                fmt->Release();
            }
        }
    }
    while (clipDepth-- > 0) ctx->rt->PopAxisAlignedClip();
    if (brush) brush->Release();

    HRESULT hr = ctx->rt->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        ctx->rt->Release();
        ctx->rt = nullptr;
    }
}

inline std::string mapVirtualKey(WPARAM vk) {
    switch (vk) {
        case VK_LEFT:   return "ArrowLeft";
        case VK_RIGHT:  return "ArrowRight";
        case VK_UP:     return "ArrowUp";
        case VK_DOWN:   return "ArrowDown";
        case VK_BACK:   return "Backspace";
        case VK_DELETE: return "Delete";
        case VK_RETURN: return "Enter";
        case VK_TAB:    return "Tab";
        case VK_ESCAPE: return "Escape";
        case VK_HOME:   return "Home";
        case VK_END:    return "End";
        case VK_PRIOR:  return "PageUp";
        case VK_NEXT:   return "PageDown";
        case VK_SPACE:  return " ";
        default: break;
    }
    if (vk >= 'A' && vk <= 'Z') {
        bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        char ch = static_cast<char>(shift ? vk : (vk - 'A' + 'a'));
        return std::string(1, ch);
    }
    if (vk >= '0' && vk <= '9') {
        return std::string(1, static_cast<char>(vk));
    }
    return "";
}

inline LRESULT CALLBACK memeWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto *ctx = reinterpret_cast<WinHostContext *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
        case WM_CREATE: {
            auto *cs = reinterpret_cast<CREATESTRUCTW *>(lp);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            SetTimer(hwnd, 1, 16, nullptr);
            return 0;
        }
        case WM_SIZE:
            if (ctx && ctx->session) {
                UINT w = LOWORD(lp), h = HIWORD(lp);
                if (ctx->rt) ctx->rt->Resize(D2D1_SIZE_U{std::max<UINT>(1, w), std::max<UINT>(1, h)});
                ctx->session->input.viewportW = static_cast<float>(w) / ctx->dpiScale;
                ctx->session->input.viewportH = static_cast<float>(h) / ctx->dpiScale;
                ctx->session->needsRender = true;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps{};
            BeginPaint(hwnd, &ps);
            paintDirect2D(ctx, hwnd);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_MOUSEMOVE:
            if (ctx && ctx->session) {
                float px = static_cast<float>(GET_X_LPARAM(lp)) / ctx->dpiScale;
                float py = static_cast<float>(GET_Y_LPARAM(lp)) / ctx->dpiScale;
                if (ctx->session->handlePointerMove(px, py)) {
                    if (ctx->onNeedRender) ctx->onNeedRender();
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
            }
            return 0;
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
            if (ctx && ctx->session) {
                SetCapture(hwnd);
                float px = static_cast<float>(GET_X_LPARAM(lp)) / ctx->dpiScale;
                float py = static_cast<float>(GET_Y_LPARAM(lp)) / ctx->dpiScale;
                int btn = (msg == WM_RBUTTONDOWN) ? 2 : 0;
                int clicks = (msg == WM_LBUTTONDBLCLK) ? 2 : 1;
                ctx->session->handlePointerDown(px, py, btn, clicks);
                if (ctx->onNeedRender) ctx->onNeedRender();
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
            if (ctx && ctx->session) {
                ReleaseCapture();
                float px = static_cast<float>(GET_X_LPARAM(lp)) / ctx->dpiScale;
                float py = static_cast<float>(GET_Y_LPARAM(lp)) / ctx->dpiScale;
                int btn = (msg == WM_RBUTTONUP) ? 2 : 0;
                ctx->session->handlePointerUp(px, py, btn);
                if (ctx->onNeedRender) ctx->onNeedRender();
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        case WM_MOUSEWHEEL:
            if (ctx && ctx->session) {
                POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
                ScreenToClient(hwnd, &pt);
                float dy = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wp)) / 40.0f;
                if (ctx->session->handleScroll(static_cast<float>(pt.x) / ctx->dpiScale, static_cast<float>(pt.y) / ctx->dpiScale, 0.0f, dy)) {
                    if (ctx->onNeedRender) ctx->onNeedRender();
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
            }
            return 0;
        case WM_KEYDOWN:
            if (ctx && ctx->session) {
                UiKeyEvent kev;
                kev.shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                kev.ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                kev.alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
                kev.key = mapVirtualKey(wp);
                if (!kev.key.empty() && ctx->session->handleKeyEvent(kev)) {
                    if (ctx->onNeedRender) ctx->onNeedRender();
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
            }
            return 0;
        case WM_CHAR:
            if (ctx && ctx->session && wp >= 32 && wp != 127) {
                if ((GetKeyState(VK_CONTROL) & 0x8000) == 0) {
                    wchar_t wch = static_cast<wchar_t>(wp);
                    std::wstring ws(1, wch);
                    UiKeyEvent kev;
                    kev.text = wideToUtf8(ws);
                    kev.key = kev.text;
                    if (ctx->session->handleKeyEvent(kev)) {
                        if (ctx->onNeedRender) ctx->onNeedRender();
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                }
            }
            return 0;
        case WM_TIMER:
            if (ctx && ctx->session) {
                if (ctx->session->tick(16.0) || ctx->session->isImmediateMode || ctx->session->needsRender) {
                    if (ctx->onNeedRender) ctx->onNeedRender();
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
            }
            return 0;
        case WM_DESTROY:
            if (ctx) ctx->open = false;
            KillTimer(hwnd, 1);
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(hwnd, msg, wp, lp);
    }
}

} // namespace WinHostDetail

inline int runWindowsNativeApp(const std::string &title, UiSession &session, int width = 1040, int height = 720, std::function<void()> onRender = nullptr) {
    if (std::getenv("MEME_HEADLESS") != nullptr) {
        if (onRender) onRender();
        else session.renderFrame(static_cast<float>(width), static_cast<float>(height));
        return 0;
    }

    SetProcessDPIAware();
    session.clipboardReadFn = []() -> std::string {
        if (!OpenClipboard(nullptr)) return "";
        HANDLE h = GetClipboardData(CF_UNICODETEXT);
        std::string out;
        if (h) {
            const wchar_t *p = static_cast<const wchar_t *>(GlobalLock(h));
            if (p) { out = WinHostDetail::wideToUtf8(p); GlobalUnlock(h); }
        }
        CloseClipboard();
        return out;
    };
    session.clipboardWriteFn = [](const std::string &txt) {
        std::wstring ws = WinHostDetail::utf8ToWide(txt);
        if (!OpenClipboard(nullptr)) return;
        EmptyClipboard();
        HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, (ws.size() + 1) * sizeof(wchar_t));
        if (h) {
            wchar_t *p = static_cast<wchar_t *>(GlobalLock(h));
            if (p) { memcpy(p, ws.c_str(), (ws.size() + 1) * sizeof(wchar_t)); GlobalUnlock(h); }
            SetClipboardData(CF_UNICODETEXT, h);
        }
        CloseClipboard();
    };

    WinHostDetail::WinHostContext ctx;
    ctx.session = &session;
    ctx.onNeedRender = onRender ? onRender : [&session, width, height]() {
        float vw = session.input.viewportW > 0 ? session.input.viewportW : static_cast<float>(width);
        float vh = session.input.viewportH > 0 ? session.input.viewportH : static_cast<float>(height);
        session.renderFrame(vw, vh);
    };
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &ctx.d2dFactory);
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown **>(&ctx.dwFactory));

    HDC hdc = GetDC(nullptr);
    if (hdc) { ctx.dpiScale = std::max(1.0f, static_cast<float>(GetDeviceCaps(hdc, LOGPIXELSX)) / 96.0f); ReleaseDC(nullptr, hdc); }

    HINSTANCE hInst = GetModuleHandleW(nullptr);
    const wchar_t *clsName = L"MemeUiWin32Window";
    WNDCLASSW wc{};
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = WinHostDetail::memeWndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    wc.lpszClassName = clsName;
    RegisterClassW(&wc);

    std::wstring wtitle = WinHostDetail::utf8ToWide(title);
    int physW = static_cast<int>(width * ctx.dpiScale);
    int physH = static_cast<int>(height * ctx.dpiScale);
    HWND hwnd = CreateWindowExW(0, clsName, wtitle.c_str(), WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                CW_USEDEFAULT, CW_USEDEFAULT, physW, physH, nullptr, nullptr, hInst, &ctx);
    if (!hwnd) return 1;

    // Apply embedded or default squircle window/taskbar icon
    HICON hAppIcon = nullptr;
    {
        std::vector<uint8_t> rgba;
        int iw = 0, ih = 0;
        MemeNative::ensureDefaultAppIconRgba(32, rgba, iw, ih);
        if (!rgba.empty() && iw > 0 && ih > 0) {
            std::vector<uint8_t> bgra(rgba.size(), 0);
            for (size_t i = 0; i + 3 < rgba.size(); i += 4) {
                bgra[i + 0] = rgba[i + 2];
                bgra[i + 1] = rgba[i + 1];
                bgra[i + 2] = rgba[i + 0];
                bgra[i + 3] = rgba[i + 3];
            }
            HBITMAP hColor = CreateBitmap(iw, ih, 1, 32, bgra.data());
            HBITMAP hMask = CreateBitmap(iw, ih, 1, 1, nullptr);
            if (hColor && hMask) {
                ICONINFO ii{};
                ii.fIcon = TRUE;
                ii.hbmMask = hMask;
                ii.hbmColor = hColor;
                hAppIcon = CreateIconIndirect(&ii);
                if (hAppIcon) {
                    SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hAppIcon));
                    SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hAppIcon));
                }
            }
            if (hColor) DeleteObject(hColor);
            if (hMask) DeleteObject(hMask);
        }
    }

    auto &nh = MemeNative::hooks();
    nh.requestRerender = [hwnd, &session]() {
        session.needsRender = true;
        PostMessageW(hwnd, WM_TIMER, 1, 0);
    };
    nh.clipboardGet = session.clipboardReadFn;
    nh.clipboardSet = [&session](const std::string &t) -> bool {
        if (session.clipboardWriteFn) session.clipboardWriteFn(t);
        return true;
    };
    nh.getScreenInfo = [&session, &ctx]() -> MemeNative::ScreenInfo {
        MemeNative::ScreenInfo s;
        s.width = session.input.viewportW;
        s.height = session.input.viewportH;
        s.scale = ctx.dpiScale;
        return s;
    };
    nh.getDeviceInfo = []() -> MemeNative::DeviceInfo {
        MemeNative::DeviceInfo d;
        d.os = "windows";
        d.arch = MemeNative::defaultArchName();
        d.model = "Windows PC";
        d.isDesktop = true; d.isMobile = false; d.isWeb = false;
        d.isDark = true;
        if (HMODULE adv = LoadLibraryW(L"advapi32.dll")) {
            typedef LONG(WINAPI *RegOpenKeyExWFn)(HKEY, LPCWSTR, DWORD, REGSAM, PHKEY);
            typedef LONG(WINAPI *RegQueryValueExWFn)(HKEY, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
            typedef LONG(WINAPI *RegCloseKeyFn)(HKEY);
            auto pOpen = reinterpret_cast<RegOpenKeyExWFn>(GetProcAddress(adv, "RegOpenKeyExW"));
            auto pQuery = reinterpret_cast<RegQueryValueExWFn>(GetProcAddress(adv, "RegQueryValueExW"));
            auto pClose = reinterpret_cast<RegCloseKeyFn>(GetProcAddress(adv, "RegCloseKey"));
            if (pOpen && pQuery && pClose) {
                HKEY hk = nullptr;
                if (pOpen(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &hk) == 0 && hk) {
                    DWORD val = 0, sz = sizeof(val), tp = 0;
                    if (pQuery(hk, L"AppsUseLightTheme", nullptr, &tp, reinterpret_cast<LPBYTE>(&val), &sz) == 0) {
                        d.isDark = (val == 0);
                    }
                    pClose(hk);
                }
            }
        }
        d.theme = d.isDark ? "dark" : "light";
        return d;
    };
    nh.fileDialogAsync = [hwnd](const std::string &mode, const std::string &dlgTitle, const std::string &defName, MemeNative::AsyncCallback cb) {
        MemeNative::AsyncResponse r; r.ok = false; r.accepted = false; r.cancelled = true;
        if (HMODULE cd = LoadLibraryW(L"comdlg32.dll")) {
            typedef BOOL(WINAPI *DlgFn)(LPOPENFILENAMEW);
            auto fn = reinterpret_cast<DlgFn>(GetProcAddress(cd, mode == "save" ? "GetSaveFileNameW" : "GetOpenFileNameW"));
            if (fn) {
                wchar_t buf[2048] = {0};
                if (!defName.empty()) {
                    std::wstring wdef = WinHostDetail::utf8ToWide(defName);
                    wcsncpy(buf, wdef.c_str(), 2040);
                }
                std::wstring wt = WinHostDetail::utf8ToWide(dlgTitle);
                OPENFILENAMEW ofn{};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hwnd;
                ofn.lpstrFile = buf;
                ofn.nMaxFile = 2048;
                ofn.lpstrTitle = wt.empty() ? nullptr : wt.c_str();
                ofn.lpstrFilter = (mode == "photo") ? L"Images\0*.png;*.jpg;*.jpeg;*.bmp;*.webp\0All Files\0*.*\0" : L"All Files\0*.*\0";
                ofn.Flags = OFN_PATHMUSTEXIST | (mode == "save" ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST) | OFN_NOCHANGEDIR;
                if (fn(&ofn)) {
                    r.ok = true; r.accepted = true; r.cancelled = false;
                    r.path = WinHostDetail::wideToUtf8(buf);
                    r.value = r.path;
                    r.paths.push_back(r.path);
                }
            }
        }
        MemeNative::dispatchCallbackOnUiThread(cb, r);
    };
    nh.setWindowTitle = [hwnd](const std::string &t) -> bool {
        return SetWindowTextW(hwnd, WinHostDetail::utf8ToWide(t).c_str()) != 0;
    };
    nh.setWindowSize = [hwnd, &ctx](int w, int hSz) -> bool {
        return SetWindowPos(hwnd, nullptr, 0, 0,
                            static_cast<int>(std::max(160, w) * ctx.dpiScale),
                            static_cast<int>(std::max(120, hSz) * ctx.dpiScale),
                            SWP_NOMOVE | SWP_NOZORDER) != 0;
    };
    nh.minimizeWindow = [hwnd]() -> bool { return ShowWindow(hwnd, SW_MINIMIZE) != 0; };
    nh.focusWindow = [hwnd]() -> bool { return SetForegroundWindow(hwnd) != 0; };

    session.input.viewportW = static_cast<float>(width);
    session.input.viewportH = static_cast<float>(height);
    session.input.dpiScale = ctx.dpiScale;
    ctx.onNeedRender();

    MSG msg{};
    while (ctx.open && GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (hAppIcon) DestroyIcon(hAppIcon);
    if (ctx.rt) ctx.rt->Release();
    if (ctx.dwFactory) ctx.dwFactory->Release();
    if (ctx.d2dFactory) ctx.d2dFactory->Release();
    return 0;
}

} // namespace MemeUI

#endif // _WIN32
