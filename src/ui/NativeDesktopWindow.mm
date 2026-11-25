#include "NativeDesktopWindow.h"

#if defined(__APPLE__)
#import <Cocoa/Cocoa.h>
#include "../interpreter/Interpreter.h"
#include "../hosts/macos/MemeMacHost.h"
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <functional>
#include <string>

@interface MemeDevWindowDelegate : NSObject <NSWindowDelegate>
@property (nonatomic, assign) DevServerState *devState;
@property (nonatomic, assign) bool *windowOpenPtr;
@end

@implementation MemeDevWindowDelegate
- (void)windowWillClose:(NSNotification *)notification {
    (void)notification;
    if (_windowOpenPtr) *_windowOpenPtr = false;
    if (_devState) _devState->running = false;
}
@end

namespace MemeUI {

static NSWindow *g_window = nil;
static MemeNativeGpuView *g_view = nil;
static MemeDevWindowDelegate *g_delegate = nil;
static bool g_windowOpen = false;
static int g_lastSeenVersion = -1;
static std::function<void()> g_renderCallback;
static UiSession g_fallbackSession;

void initNativeDesktopApp() {
    if (std::getenv("MEME_HEADLESS") != nullptr) return;
    @autoreleasepool {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    }
}

void triggerNativeDesktopRepaint() {
    if (g_view != nil) {
        if (g_renderCallback) g_renderCallback();
        [g_view setNeedsDisplay:YES];
    }
}

void showNativeDesktopWindow(DevServerState &state) {
    if (std::getenv("MEME_HEADLESS") != nullptr) return;
    if (g_windowOpen && g_window != nil) {
        [g_window makeKeyAndOrderFront:nil];
        return;
    }

    UiSession *sess = state.uiSession
        ? state.uiSession
        : (state.interpreter ? &state.interpreter->getUiSession() : &g_fallbackSession);
    bindMacClipboard(*sess);

    g_renderCallback = [&state, sess]() {
        if (state.onRenderFrame) {
            state.onRenderFrame();
        } else if (state.interpreter) {
            state.interpreter->triggerUiRerender();
        } else if (sess) {
            float vw = sess->input.viewportW > 0 ? sess->input.viewportW : static_cast<float>(state.viewportWidth);
            float vh = sess->input.viewportH > 0 ? sess->input.viewportH : static_cast<float>(state.viewportHeight);
            sess->renderFrame(vw, vh);
        }
    };

    @autoreleasepool {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        int w = std::max(320, state.viewportWidth);
        int h = std::max(240, state.viewportHeight);
        NSRect frame = NSMakeRect(120, 120, w, h);
        NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                                  NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;

        g_window = [[NSWindow alloc] initWithContentRect:frame styleMask:style backing:NSBackingStoreBuffered defer:NO];
        std::string title = state.appName + " — MEME-UI Native Desktop";
        NSString *nsTitle = [NSString stringWithUTF8String:title.c_str()];
        [g_window setTitle:(nsTitle ? nsTitle : @"MEME-UI Native Desktop")];
        [g_window setAcceptsMouseMovedEvents:YES];

        g_delegate = [[MemeDevWindowDelegate alloc] init];
        g_delegate.devState = &state;
        g_delegate.windowOpenPtr = &g_windowOpen;
        [g_window setDelegate:g_delegate];

        g_view = [[MemeNativeGpuView alloc] initWithFrame:NSMakeRect(0, 0, w, h)];
        g_view.session = sess;
        g_view.sessionMutex = &state.mtx;
        g_view.onNeedRender = &g_renderCallback;
        [g_view setupRenderer];

        [g_window setContentView:g_view];
        [g_window makeFirstResponder:g_view];
        [g_window center];
        [g_window makeKeyAndOrderFront:nil];
        [NSApp activateIgnoringOtherApps:YES];

        g_windowOpen = true;
        g_lastSeenVersion = state.version;
        if (sess) {
            sess->input.viewportW = static_cast<float>(w);
            sess->input.viewportH = static_cast<float>(h);
            g_renderCallback();
        }
        [g_view setNeedsDisplay:YES];
    }
}

void hideNativeDesktopWindow() {
    g_windowOpen = false;
    @autoreleasepool {
        if (g_window) {
            [g_window close];
            MEME_OBJC_RELEASE(g_window);
            g_window = nil;
        }
        if (g_view) {
            MEME_OBJC_RELEASE(g_view);
            g_view = nil;
        }
        if (g_delegate) {
            MEME_OBJC_RELEASE(g_delegate);
            g_delegate = nil;
        }
    }
}

bool isNativeDesktopWindowOpen() {
    return g_windowOpen;
}

void pumpNativeDesktopEvents(double timeoutSeconds) {
    if (!g_windowOpen) return;
    @autoreleasepool {
        NSEvent *ev = [NSApp nextEventMatchingMask:NSEventMaskAny
                                         untilDate:[NSDate dateWithTimeIntervalSinceNow:timeoutSeconds]
                                            inMode:NSDefaultRunLoopMode
                                           dequeue:YES];
        while (ev) {
            [NSApp sendEvent:ev];
            ev = [NSApp nextEventMatchingMask:NSEventMaskAny
                                    untilDate:[NSDate distantPast]
                                       inMode:NSDefaultRunLoopMode
                                      dequeue:YES];
        }
        [NSApp updateWindows];

        if (g_view && g_view.session) {
            bool redraw = false;
            if (g_view.session->needsRender || g_view.session->isImmediateMode) redraw = true;
            if (g_delegate && g_delegate.devState && g_delegate.devState->version != g_lastSeenVersion) {
                g_lastSeenVersion = g_delegate.devState->version;
                if (g_delegate.devState->uiSession) g_view.session = g_delegate.devState->uiSession;
                else if (g_delegate.devState->interpreter) g_view.session = &g_delegate.devState->interpreter->getUiSession();
                redraw = true;
            }
            if (redraw) {
                if (g_renderCallback) g_renderCallback();
                [g_view setNeedsDisplay:YES];
            }
        }
    }
}

} // namespace MemeUI

#else
namespace MemeUI {
void initNativeDesktopApp() {}
void showNativeDesktopWindow(DevServerState &) {}
void hideNativeDesktopWindow() {}
bool isNativeDesktopWindowOpen() { return false; }
void pumpNativeDesktopEvents(double) {}
void triggerNativeDesktopRepaint() {}
}
#endif
