#include "NativeDesktopWindow.h"

#if defined(__APPLE__)
#import <Cocoa/Cocoa.h>
#include "src/ui/TailwindEngine.h"
#include "src/interpreter/Interpreter.h"
#include <vector>
#include <mutex>
#include <iostream>
#include <algorithm>

@interface MemeNativeCanvasView : NSView <NSTextFieldDelegate> {
    DevServerState *statePtr;
    NSTextField *nativeInput;
    int activeInputId;
    std::vector<TailwindEngine::GpuDrawCmd> cachedCmds;
    std::vector<TailwindEngine::HitRegion> cachedHitRegions;
    NSTrackingArea *trackingArea;
}
- (instancetype)initWithFrame:(NSRect)frameRect state:(DevServerState *)st;
@end

@interface MemeWindowDelegate : NSObject <NSWindowDelegate> {
    DevServerState *statePtr;
}
- (instancetype)initWithState:(DevServerState *)st;
@end

static NSWindow *g_nativeWindow = nil;
static MemeNativeCanvasView *g_nativeCanvasView = nil;
static MemeWindowDelegate *g_windowDelegate = nil;

@implementation MemeWindowDelegate
- (instancetype)initWithState:(DevServerState *)st {
    self = [super init];
    if (self) {
        statePtr = st;
    }
    return self;
}

- (void)windowWillClose:(NSNotification *)notification {
    (void)notification;
    if (statePtr) {
        std::lock_guard<std::mutex> lock(statePtr->mtx);
        statePtr->platform = "terminal";
    }
    g_nativeWindow = nil;
    g_nativeCanvasView = nil;
    g_windowDelegate = nil;
}
@end

@implementation MemeNativeCanvasView

- (instancetype)initWithFrame:(NSRect)frameRect state:(DevServerState *)st {
    self = [super initWithFrame:frameRect];
    if (self) {
        statePtr = st;
        activeInputId = 0;

        nativeInput = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 100, 30)];
        [nativeInput setHidden:YES];
        [nativeInput setDelegate:self];
        [nativeInput setFocusRingType:NSFocusRingTypeNone];
        [nativeInput setBordered:NO];
        [nativeInput setDrawsBackground:NO];
        [nativeInput setTextColor:[NSColor whiteColor]];
        [nativeInput setFont:[NSFont systemFontOfSize:14.0 weight:NSFontWeightRegular]];

        NSMutableAttributedString *ph = [[NSMutableAttributedString alloc] initWithString:@"Naya task likho..."];
        [ph addAttribute:NSForegroundColorAttributeName
                   value:[NSColor colorWithRed:100.0/255.0 green:116.0/255.0 blue:139.0/255.0 alpha:1.0]
                   range:NSMakeRange(0, [ph length])];
        [[nativeInput cell] setPlaceholderAttributedString:ph];

        [self addSubview:nativeInput];
    }
    return self;
}

- (BOOL)isFlipped {
    return YES;
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)updateTrackingAreas {
    [super updateTrackingAreas];
    if (trackingArea) {
        [self removeTrackingArea:trackingArea];
    }
    trackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                                options:(NSTrackingMouseMoved | NSTrackingActiveAlways)
                                                  owner:self
                                               userInfo:nil];
    [self addTrackingArea:trackingArea];
}

- (void)mouseMoved:(NSEvent *)event {
    NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
    bool isOverClickable = false;
    for (const auto &hr : cachedHitRegions) {
        if (hr.hasClick &&
            loc.x >= hr.x && loc.x <= hr.x + hr.width &&
            loc.y >= hr.y && loc.y <= hr.y + hr.height) {
            isOverClickable = true;
            break;
        }
    }
    if (isOverClickable) {
        [[NSCursor pointingHandCursor] set];
    } else {
        [[NSCursor arrowCursor] set];
    }
}

- (void)mouseDown:(NSEvent *)event {
    if (!statePtr || !statePtr->interpreter) return;

    NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
    for (const auto &hr : cachedHitRegions) {
        if (loc.x >= hr.x && loc.x <= hr.x + hr.width &&
            loc.y >= hr.y && loc.y <= hr.y + hr.height) {
            if (hr.hasClick) {
                // If native input has text, pass it along with the click
                std::string inVal = "";
                if (![nativeInput isHidden]) {
                    inVal = [[nativeInput stringValue] UTF8String];
                }
                bool ok = false;
                {
                    std::lock_guard<std::mutex> lock(statePtr->mtx);
                    ok = statePtr->interpreter->dispatchUiEvent(hr.id, "click", inVal);
                    if (ok) {
                        statePtr->version++;
                        statePtr->gpuCommands = statePtr->interpreter->getLastGpuCommandsJson();
                        statePtr->hitRegions = statePtr->interpreter->getLastHitRegionsJson();
                        statePtr->terminalUi = statePtr->interpreter->getLastTerminalUi();
                        statePtr->html = statePtr->interpreter->getLastHtml();
                    }
                }
                if (ok) {
                    [self setNeedsDisplay:YES];
                }
                return;
            } else if (hr.tag == "input" || hr.hasChange) {
                [[self window] makeFirstResponder:nativeInput];
                return;
            }
        }
    }
}

- (void)controlTextDidChange:(NSNotification *)obj {
    (void)obj;
    if (activeInputId > 0 && statePtr && statePtr->interpreter) {
        std::string val = [[nativeInput stringValue] UTF8String];
        statePtr->interpreter->dispatchUiEvent(activeInputId, "change", val);
        std::lock_guard<std::mutex> lock(statePtr->mtx);
        statePtr->version++;
        statePtr->gpuCommands = statePtr->interpreter->getLastGpuCommandsJson();
        statePtr->hitRegions = statePtr->interpreter->getLastHitRegionsJson();
        statePtr->terminalUi = statePtr->interpreter->getLastTerminalUi();
        statePtr->html = statePtr->interpreter->getLastHtml();
    }
}

- (BOOL)control:(NSControl *)control textView:(NSTextView *)textView doCommandBySelector:(SEL)commandSelector {
    (void)control;
    (void)textView;
    if (commandSelector == @selector(insertNewline:)) {
        if (activeInputId > 0 && statePtr && statePtr->interpreter) {
            std::string val = [[nativeInput stringValue] UTF8String];
            statePtr->interpreter->dispatchUiEvent(activeInputId, "enter", val);
            {
                std::lock_guard<std::mutex> lock(statePtr->mtx);
                statePtr->version++;
                statePtr->gpuCommands = statePtr->interpreter->getLastGpuCommandsJson();
                statePtr->hitRegions = statePtr->interpreter->getLastHitRegionsJson();
                statePtr->terminalUi = statePtr->interpreter->getLastTerminalUi();
                statePtr->html = statePtr->interpreter->getLastHtml();
            }
            [nativeInput setStringValue:@""];
            [self setNeedsDisplay:YES];
        }
        return YES;
    }
    return NO;
}

- (void)drawRect:(NSRect)dirtyRect {
    (void)dirtyRect;
    if (!statePtr || !statePtr->interpreter) return;

    NSRect b = [self bounds];
    cachedHitRegions.clear();

    {
        std::lock_guard<std::mutex> lock(statePtr->mtx);
        Value rootVal = statePtr->interpreter->getRootUiComponent();
        cachedCmds = TailwindEngine::computeNativeLayout(rootVal, b.size.width, b.size.height, &cachedHitRegions);
        statePtr->gpuCommands = TailwindEngine::gpuCommandsToJson(cachedCmds);
        statePtr->hitRegions = TailwindEngine::hitRegionsToJson(cachedHitRegions);
    }

    // Window background: Slate 950 (#020617)
    [[NSColor colorWithRed:2.0/255.0 green:6.0/255.0 blue:23.0/255.0 alpha:1.0] setFill];
    NSRectFill(b);

    // Locate active input hit region
    const TailwindEngine::HitRegion *inputHr = nullptr;
    for (const auto &hr : cachedHitRegions) {
        if (hr.tag == "input" || hr.hasChange) {
            inputHr = &hr;
            break;
        }
    }

    if (inputHr) {
        activeInputId = inputHr->id;
        NSRect inFrame = NSMakeRect(inputHr->x + 16, inputHr->y + 8,
                                    std::max(60.0f, inputHr->width - 32),
                                    std::max(20.0f, inputHr->height - 16));
        [nativeInput setFrame:inFrame];
        [nativeInput setHidden:NO];
        if (![nativeInput currentEditor] || inputHr->inputValue.empty()) {
            [nativeInput setStringValue:[NSString stringWithUTF8String:inputHr->inputValue.c_str()]];
        }
    } else {
        [nativeInput setHidden:YES];
    }

    // Direct Native CoreGraphics Painting
    for (const auto &cmd : cachedCmds) {
        // Skip drawing placeholder or text inside the input box since nativeInput handles it natively
        if (inputHr && cmd.type == "DRAW_TEXT" &&
            cmd.x >= inputHr->x && cmd.x <= inputHr->x + inputHr->width &&
            cmd.y >= inputHr->y && cmd.y <= inputHr->y + inputHr->height) {
            continue;
        }

        TailwindEngine::ColorRgba c = TailwindEngine::hexToRgba(cmd.color, cmd.alpha);
        NSColor *col = [NSColor colorWithRed:c.r green:c.g blue:c.b alpha:c.a];

        if (cmd.type == "FILL_ROUND_RECT") {
            NSRect r = NSMakeRect(cmd.x, cmd.y, cmd.width, cmd.height);
            NSBezierPath *bp = [NSBezierPath bezierPathWithRoundedRect:r xRadius:cmd.radius yRadius:cmd.radius];
            [col setFill];
            [bp fill];
        } else if (cmd.type == "STROKE_ROUND_RECT") {
            NSRect r = NSMakeRect(cmd.x, cmd.y, cmd.width, cmd.height);
            NSBezierPath *bp = [NSBezierPath bezierPathWithRoundedRect:r xRadius:cmd.radius yRadius:cmd.radius];
            [bp setLineWidth:cmd.lineWidth];
            [col setStroke];
            [bp stroke];
        } else if (cmd.type == "DRAW_LINE") {
            NSBezierPath *bp = [NSBezierPath bezierPath];
            [bp moveToPoint:NSMakePoint(cmd.x, cmd.y)];
            [bp lineToPoint:NSMakePoint(cmd.x + cmd.width, cmd.y + cmd.height)];
            [bp setLineWidth:cmd.lineWidth];
            [col setStroke];
            [bp stroke];
        } else if (cmd.type == "DRAW_TEXT") {
            NSFontWeight w = (cmd.fontWeight >= 800) ? NSFontWeightBlack :
                             (cmd.fontWeight >= 700) ? NSFontWeightBold :
                             (cmd.fontWeight >= 600) ? NSFontWeightSemibold :
                             (cmd.fontWeight >= 500) ? NSFontWeightMedium : NSFontWeightRegular;
            NSFont *font = [NSFont systemFontOfSize:cmd.fontSize weight:w];
            NSDictionary *attrs = @{
                NSFontAttributeName: font,
                NSForegroundColorAttributeName: col
            };
            NSString *nsText = [NSString stringWithUTF8String:cmd.text.c_str()];
            // In flipped coordinates, baseline is cmd.y. The glyph top-left is at cmd.y - fontSize * 0.88.
            NSPoint pt = NSMakePoint(cmd.x, cmd.y - cmd.fontSize * 0.88f);
            [nsText drawAtPoint:pt withAttributes:attrs];
        }
    }
}

@end

namespace MemeUI {

void initNativeDesktopApp() {
    @autoreleasepool {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    }
}

void showNativeDesktopWindow(DevServerState &state) {
    @autoreleasepool {
        if (!g_nativeWindow) {
            initNativeDesktopApp();

            NSRect frame = NSMakeRect(0, 0, 960, 720);
            g_nativeWindow = [[NSWindow alloc] initWithContentRect:frame
                                                         styleMask:(NSWindowStyleMaskTitled |
                                                                    NSWindowStyleMaskClosable |
                                                                    NSWindowStyleMaskMiniaturizable |
                                                                    NSWindowStyleMaskResizable)
                                                           backing:NSBackingStoreBuffered
                                                             defer:NO];
            std::string winTitle = state.appName + " — MEMElang Native Desktop";
            [g_nativeWindow setTitle:[NSString stringWithUTF8String:winTitle.c_str()]];
            [g_nativeWindow setReleasedWhenClosed:NO];
            [g_nativeWindow center];
            [g_nativeWindow setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameDarkAqua]];

            g_windowDelegate = [[MemeWindowDelegate alloc] initWithState:&state];
            [g_nativeWindow setDelegate:g_windowDelegate];

            g_nativeCanvasView = [[MemeNativeCanvasView alloc] initWithFrame:frame state:&state];
            [g_nativeWindow setContentView:g_nativeCanvasView];
            [g_nativeWindow makeKeyAndOrderFront:nil];
            [NSApp activateIgnoringOtherApps:YES];
        } else {
            [g_nativeWindow makeKeyAndOrderFront:nil];
            [g_nativeCanvasView setNeedsDisplay:YES];
        }
    }
}

void hideNativeDesktopWindow() {
    @autoreleasepool {
        if (g_nativeWindow) {
            [g_nativeWindow orderOut:nil];
        }
    }
}

bool isNativeDesktopWindowOpen() {
    return (g_nativeWindow != nil && [g_nativeWindow isVisible]);
}

void pumpNativeDesktopEvents(double timeoutSeconds) {
    @autoreleasepool {
        NSEvent *event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                            untilDate:[NSDate dateWithTimeIntervalSinceNow:timeoutSeconds]
                                               inMode:NSDefaultRunLoopMode
                                              dequeue:YES];
        if (event) {
            [NSApp sendEvent:event];
            [NSApp updateWindows];
        }
    }
}

void triggerNativeDesktopRepaint() {
    @autoreleasepool {
        if (g_nativeCanvasView) {
            [g_nativeCanvasView setNeedsDisplay:YES];
        }
    }
}

} // namespace MemeUI

#else // Non-Apple stub

namespace MemeUI {
void initNativeDesktopApp() {}
void showNativeDesktopWindow(DevServerState &) {}
void hideNativeDesktopWindow() {}
bool isNativeDesktopWindowOpen() { return false; }
void pumpNativeDesktopEvents(double) {}
void triggerNativeDesktopRepaint() {}
}

#endif
