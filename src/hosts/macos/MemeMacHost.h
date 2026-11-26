#pragma once

#if defined(__APPLE__) && defined(__OBJC__)

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <CoreGraphics/CoreGraphics.h>
#include "../../ui/MemeUiTypes.h"
#include "../../ui/MemeUiTessellator.h"
#include "../../ui/MemeUiRuntime.h"
#include "../../platform/MemeNativeApis.h"
#include <dlfcn.h>
#import <objc/message.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#if __has_feature(objc_arc)
#define MEME_OBJC_RELEASE(x) ((void)0)
#define MEME_OBJC_RETAIN(x) (x)
#else
#define MEME_OBJC_RELEASE(x) [(x) release]
#define MEME_OBJC_RETAIN(x) [(x) retain]
#endif

static_assert(sizeof(MemeUI::UiVertex) == 20, "UiVertex must be tightly packed 20 bytes");

namespace MemeMacHostInternal {

inline NSColor *nsColorFromHex(const std::string &hexStr, float alpha) {
    MemeUI::ColorRgba c = MemeUI::hexToRgba(hexStr, alpha);
    return [NSColor colorWithSRGBRed:c.r green:c.g blue:c.b alpha:c.a];
}

inline std::string mapCocoaKey(NSEvent *ev) {
    unsigned short kc = [ev keyCode];
    switch (kc) {
        case 123: return "ArrowLeft";
        case 124: return "ArrowRight";
        case 125: return "ArrowDown";
        case 126: return "ArrowUp";
        case 51:  return "Backspace";
        case 117: return "Delete";
        case 36:
        case 76:  return "Enter";
        case 48:  return "Tab";
        case 53:  return "Escape";
        case 115: return "Home";
        case 119: return "End";
        case 116: return "PageUp";
        case 121: return "PageDown";
        case 49:  return " ";
        default: break;
    }
    NSString *chars = [ev charactersIgnoringModifiers];
    if (chars && [chars length] > 0) {
        unichar uc = [chars characterAtIndex:0];
        if (uc >= 32 && uc != 127 && !(uc >= 0xF700 && uc <= 0xF8FF)) {
            return std::string([chars UTF8String]);
        }
    }
    return "";
}

inline std::string mapCocoaText(NSEvent *ev) {
    NSEventModifierFlags mods = [ev modifierFlags];
    if ((mods & NSEventModifierFlagCommand) || (mods & NSEventModifierFlagControl)) {
        return "";
    }
    NSString *chars = [ev characters];
    if (!chars || [chars length] == 0) return "";
    std::string out;
    for (NSUInteger i = 0; i < [chars length]; ++i) {
        unichar uc = [chars characterAtIndex:i];
        if (uc < 0x20 || uc == 0x7F || (uc >= 0xF700 && uc <= 0xF8FF)) continue;
        NSString *sub = [chars substringWithRange:NSMakeRange(i, 1)];
        out += [sub UTF8String];
    }
    return out;
}

inline NSString *const kMemeMetalShaderSource = @R"(
#include <metal_stdlib>
using namespace metal;

struct MemeRawVert {
    float x;
    float y;
    float u;
    float v;
    uint rgba;
};

struct MemeUniforms {
    float viewportW;
    float viewportH;
};

struct MemeFragIn {
    float4 position [[position]];
    float2 uv;
    float4 color;
};

vertex MemeFragIn meme_ui_vertex(
    uint vid [[vertex_id]],
    const device MemeRawVert *verts [[buffer(0)]],
    constant MemeUniforms &uni [[buffer(1)]])
{
    MemeRawVert v = verts[vid];
    float nx = (v.x / max(1.0f, uni.viewportW)) * 2.0f - 1.0f;
    float ny = 1.0f - (v.y / max(1.0f, uni.viewportH)) * 2.0f;
    MemeFragIn out;
    out.position = float4(nx, ny, 0.0f, 1.0f);
    out.uv = float2(v.u, v.v);
    float r = float(v.rgba & 0xFFu) / 255.0f;
    float g = float((v.rgba >> 8) & 0xFFu) / 255.0f;
    float b = float((v.rgba >> 16) & 0xFFu) / 255.0f;
    float a = float((v.rgba >> 24) & 0xFFu) / 255.0f;
    out.color = float4(r, g, b, a);
    return out;
}

fragment float4 meme_ui_fragment(
    MemeFragIn in [[stage_in]],
    texture2d<float> atlas [[texture(0)]])
{
    constexpr sampler s(coord::normalized, address::clamp_to_edge, filter::linear);
    float4 tex = atlas.sample(s, in.uv);
    return float4(in.color.rgb * tex.rgb, in.color.a * tex.a);
}
)";

} // namespace MemeMacHostInternal

@interface MemeNativeGpuView : NSView
@property (nonatomic, assign) MemeUI::UiSession *session;
@property (nonatomic, assign) std::mutex *sessionMutex;
@property (nonatomic, assign) std::function<void()> *onNeedRender;
@property (nonatomic, assign) std::string fallbackCommandsJson;
@property (nonatomic, assign) BOOL useMetal;
- (void)setupRenderer;
- (void)renderMetalFrame;
- (void)refreshCursorAt:(NSPoint)pt;
@end

@implementation MemeNativeGpuView {
    id<MTLDevice> _device;
    id<MTLCommandQueue> _queue;
    id<MTLRenderPipelineState> _pipeline;
    id<MTLTexture> _atlasTex;
    CAMetalLayer *_metalLayer;
    NSTrackingArea *_trackingArea;
    BOOL _atlasUploaded;
}

- (BOOL)isFlipped { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)wantsUpdateLayer { return _useMetal; }

- (void)setupRenderer {
    _useMetal = NO;
    _atlasUploaded = NO;
    _device = MTLCreateSystemDefaultDevice();
    if (!_device) return;

    _queue = [_device newCommandQueue];
    NSError *err = nil;
    id<MTLLibrary> lib = [_device newLibraryWithSource:MemeMacHostInternal::kMemeMetalShaderSource options:nil error:&err];
    if (!lib || err) return;

    id<MTLFunction> vfn = [lib newFunctionWithName:@"meme_ui_vertex"];
    id<MTLFunction> ffn = [lib newFunctionWithName:@"meme_ui_fragment"];
    MTLRenderPipelineDescriptor *desc = [[MTLRenderPipelineDescriptor alloc] init];
    desc.vertexFunction = vfn;
    desc.fragmentFunction = ffn;
    desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    desc.colorAttachments[0].blendingEnabled = YES;
    desc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
    desc.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
    desc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    desc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    desc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
    desc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

    _pipeline = [_device newRenderPipelineStateWithDescriptor:desc error:&err];
    MEME_OBJC_RELEASE(desc);
    MEME_OBJC_RELEASE(vfn);
    MEME_OBJC_RELEASE(ffn);
    MEME_OBJC_RELEASE(lib);
    if (!_pipeline || err) return;

    MTLTextureDescriptor *td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                  width:MemeUI::FontAtlas::ATLAS_W
                                                                                 height:MemeUI::FontAtlas::ATLAS_H
                                                                              mipmapped:NO];
    _atlasTex = [_device newTextureWithDescriptor:td];
    auto rgba = MemeUI::FontAtlas::getRgbaAtlas();
    [_atlasTex replaceRegion:MTLRegionMake2D(0, 0, MemeUI::FontAtlas::ATLAS_W, MemeUI::FontAtlas::ATLAS_H)
                 mipmapLevel:0
                   withBytes:rgba.data()
                 bytesPerRow:MemeUI::FontAtlas::ATLAS_W * 4];
    _atlasUploaded = YES;

    _metalLayer = [CAMetalLayer layer];
    _metalLayer.device = _device;
    _metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    _metalLayer.framebufferOnly = YES;
    CGFloat scale = (self.window ? self.window.backingScaleFactor : 2.0);
    _metalLayer.contentsScale = scale;
    _metalLayer.drawableSize = CGSizeMake(std::max(1.0, self.bounds.size.width * scale), std::max(1.0, self.bounds.size.height * scale));
    self.wantsLayer = YES;
    self.layer = _metalLayer;
    _useMetal = YES;
}

- (void)updateTrackingAreas {
    [super updateTrackingAreas];
    if (_trackingArea) {
        [self removeTrackingArea:_trackingArea];
        MEME_OBJC_RELEASE(_trackingArea);
        _trackingArea = nil;
    }
    NSTrackingAreaOptions opts = NSTrackingMouseMoved | NSTrackingActiveInKeyWindow | NSTrackingInVisibleRect;
    _trackingArea = [[NSTrackingArea alloc] initWithRect:self.bounds options:opts owner:self userInfo:nil];
    [self addTrackingArea:_trackingArea];
}

- (void)setFrameSize:(NSSize)newSize {
    [super setFrameSize:newSize];
    if (_metalLayer) {
        CGFloat scale = (self.window ? self.window.backingScaleFactor : 2.0);
        _metalLayer.contentsScale = scale;
        _metalLayer.drawableSize = CGSizeMake(std::max(1.0, newSize.width * scale), std::max(1.0, newSize.height * scale));
    }
    if (_session && newSize.width > 1 && newSize.height > 1) {
        if (_sessionMutex) _sessionMutex->lock();
        _session->input.viewportW = static_cast<float>(newSize.width);
        _session->input.viewportH = static_cast<float>(newSize.height);
        _session->needsRender = true;
        if (_sessionMutex) _sessionMutex->unlock();
        if (_onNeedRender && *_onNeedRender) (*_onNeedRender)();
    }
    [self setNeedsDisplay:YES];
}

- (void)viewDidChangeBackingProperties {
    [super viewDidChangeBackingProperties];
    if (_metalLayer && self.window) {
        CGFloat scale = self.window.backingScaleFactor;
        _metalLayer.contentsScale = scale;
        _metalLayer.drawableSize = CGSizeMake(std::max(1.0, self.bounds.size.width * scale), std::max(1.0, self.bounds.size.height * scale));
    }
}

- (void)renderMetalFrame {
    if (!_useMetal || !_metalLayer || !_pipeline || !_session) return;
    CGFloat scale = _metalLayer.contentsScale > 0 ? _metalLayer.contentsScale : 1.0;
    float vw = static_cast<float>(self.bounds.size.width);
    float vh = static_cast<float>(self.bounds.size.height);
    if (vw <= 0.0f || vh <= 0.0f) return;

    if (static_cast<NSUInteger>(_metalLayer.drawableSize.width) != static_cast<NSUInteger>(vw * scale) ||
        static_cast<NSUInteger>(_metalLayer.drawableSize.height) != static_cast<NSUInteger>(vh * scale)) {
        _metalLayer.drawableSize = CGSizeMake(std::max(1.0, vw * scale), std::max(1.0, vh * scale));
    }

    MemeUI::UiMesh meshCopy;
    if (_sessionMutex) _sessionMutex->lock();
    if (_session->needsRender || _session->lastMesh.vertices.empty()) {
        _session->renderFrame(vw, vh);
    }
    meshCopy = _session->lastMesh;
    if (_sessionMutex) _sessionMutex->unlock();

    @autoreleasepool {
        id<CAMetalDrawable> drawable = [_metalLayer nextDrawable];
        if (!drawable) return;

        MTLRenderPassDescriptor *pass = [MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].texture = drawable.texture;
        pass.colorAttachments[0].loadAction = MTLLoadActionClear;
        pass.colorAttachments[0].clearColor = MTLClearColorMake(0.008, 0.024, 0.09, 1.0);
        pass.colorAttachments[0].storeAction = MTLStoreActionStore;

        id<MTLCommandBuffer> cmdBuf = [_queue commandBuffer];
        id<MTLRenderCommandEncoder> enc = [cmdBuf renderCommandEncoderWithDescriptor:pass];

        if (!meshCopy.vertices.empty() && !meshCopy.indices.empty()) {
            [enc setRenderPipelineState:_pipeline];
            id<MTLBuffer> vbuf = [_device newBufferWithBytes:meshCopy.vertices.data()
                                                      length:meshCopy.vertices.size() * sizeof(MemeUI::UiVertex)
                                                     options:MTLResourceStorageModeShared];
            id<MTLBuffer> ibuf = [_device newBufferWithBytes:meshCopy.indices.data()
                                                      length:meshCopy.indices.size() * sizeof(uint32_t)
                                                     options:MTLResourceStorageModeShared];
            struct { float vw; float vh; } uni{vw, vh};
            [enc setVertexBuffer:vbuf offset:0 atIndex:0];
            [enc setVertexBytes:&uni length:sizeof(uni) atIndex:1];
            [enc setFragmentTexture:_atlasTex atIndex:0];

            NSUInteger dw = static_cast<NSUInteger>(drawable.texture.width);
            NSUInteger dh = static_cast<NSUInteger>(drawable.texture.height);
            for (const auto &batch : meshCopy.batches) {
                if (batch.indexCount == 0) continue;
                NSUInteger sx = static_cast<NSUInteger>(std::clamp(batch.clipRect.x * static_cast<float>(scale), 0.0f, static_cast<float>(dw)));
                NSUInteger sy = static_cast<NSUInteger>(std::clamp(batch.clipRect.y * static_cast<float>(scale), 0.0f, static_cast<float>(dh)));
                NSUInteger ex = static_cast<NSUInteger>(std::clamp((batch.clipRect.x + batch.clipRect.width) * static_cast<float>(scale), 0.0f, static_cast<float>(dw)));
                NSUInteger ey = static_cast<NSUInteger>(std::clamp((batch.clipRect.y + batch.clipRect.height) * static_cast<float>(scale), 0.0f, static_cast<float>(dh)));
                if (ex <= sx || ey <= sy) continue;
                MTLScissorRect sc{sx, sy, ex - sx, ey - sy};
                [enc setScissorRect:sc];
                [enc drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                indexCount:batch.indexCount
                                 indexType:MTLIndexTypeUInt32
                               indexBuffer:ibuf
                         indexBufferOffset:batch.indexOffset * sizeof(uint32_t)];
            }
            MEME_OBJC_RELEASE(vbuf);
            MEME_OBJC_RELEASE(ibuf);
        }

        [enc endEncoding];
        [cmdBuf presentDrawable:drawable];
        [cmdBuf commit];
    }
}

- (void)displayLayer:(CALayer *)layer {
    (void)layer;
    [self renderMetalFrame];
}

- (void)drawRect:(NSRect)dirtyRect {
    if (_useMetal) {
        [self renderMetalFrame];
        return;
    }
    // Pure CoreGraphics fallback (for VMs/headless environments without Metal GPU)
    NSRect bounds = [self bounds];
    [[MemeMacHostInternal::nsColorFromHex("#020617", 1.0f) copy] setFill];
    NSRectFill(bounds);

    if (!_session) return;
    std::vector<MemeUI::GpuDrawCmd> cmds;
    if (_sessionMutex) _sessionMutex->lock();
    if (_session->needsRender || _session->lastCmds.empty()) {
        _session->renderFrame(static_cast<float>(bounds.size.width), static_cast<float>(bounds.size.height));
    }
    cmds = _session->lastCmds;
    if (_sessionMutex) _sessionMutex->unlock();

    CGContextRef ctx = [[NSGraphicsContext currentContext] CGContext];
    int clipDepth = 0;
    for (const auto &c : cmds) {
        if (c.type == "PUSH_CLIP") {
            if (ctx) {
                CGContextSaveGState(ctx);
                CGContextClipToRect(ctx, CGRectMake(c.x, c.y, c.width, c.height));
                clipDepth++;
            }
        } else if (c.type == "POP_CLIP") {
            if (ctx && clipDepth > 0) {
                CGContextRestoreGState(ctx);
                clipDepth--;
            }
        } else if (c.type == "FILL_ROUND_RECT" || c.type == "SHADOW_ROUND_RECT" || c.type == "DRAW_SELECTION_RECT" || c.type == "DRAW_CARET") {
            float a = (c.type == "SHADOW_ROUND_RECT") ? c.alpha * 0.2f : c.alpha;
            [MemeMacHostInternal::nsColorFromHex(c.color, a) setFill];
            NSBezierPath *p = [NSBezierPath bezierPathWithRoundedRect:NSMakeRect(c.x, c.y, std::max(0.0f, c.width), std::max(0.0f, c.height)) xRadius:c.radius yRadius:c.radius];
            [p fill];
        } else if (c.type == "FILL_GRADIENT_RECT") {
            NSGradient *grad = [[NSGradient alloc] initWithStartingColor:MemeMacHostInternal::nsColorFromHex(c.color, c.alpha)
                                                             endingColor:MemeMacHostInternal::nsColorFromHex(c.color2, c.alpha2)];
            NSBezierPath *p = [NSBezierPath bezierPathWithRoundedRect:NSMakeRect(c.x, c.y, std::max(0.0f, c.width), std::max(0.0f, c.height)) xRadius:c.radius yRadius:c.radius];
            [grad drawInBezierPath:p angle:c.angle];
            MEME_OBJC_RELEASE(grad);
        } else if (c.type == "STROKE_ROUND_RECT") {
            [MemeMacHostInternal::nsColorFromHex(c.color, c.alpha) setStroke];
            NSBezierPath *p = [NSBezierPath bezierPathWithRoundedRect:NSMakeRect(c.x, c.y, std::max(0.0f, c.width), std::max(0.0f, c.height)) xRadius:c.radius yRadius:c.radius];
            [p setLineWidth:std::max(0.5f, c.lineWidth)];
            [p stroke];
        } else if (c.type == "DRAW_LINE") {
            [MemeMacHostInternal::nsColorFromHex(c.color, c.alpha) setStroke];
            NSBezierPath *p = [NSBezierPath bezierPath];
            [p moveToPoint:NSMakePoint(c.x, c.y)];
            [p lineToPoint:NSMakePoint(c.x + c.width, c.y + c.height)];
            [p setLineWidth:std::max(0.5f, c.lineWidth)];
            [p stroke];
        } else if (c.type == "DRAW_CIRCLE" || c.type == "STROKE_CIRCLE") {
            NSBezierPath *p = [NSBezierPath bezierPathWithOvalInRect:NSMakeRect(c.x, c.y, c.width, c.height)];
            if (c.type == "DRAW_CIRCLE" && c.filled) {
                [MemeMacHostInternal::nsColorFromHex(c.color, c.alpha) setFill];
                [p fill];
            } else {
                [MemeMacHostInternal::nsColorFromHex(c.color, c.alpha) setStroke];
                [p setLineWidth:std::max(0.5f, c.lineWidth)];
                [p stroke];
            }
        } else if (c.type == "DRAW_POLYLINE" && c.points.size() >= 2) {
            [MemeMacHostInternal::nsColorFromHex(c.color, c.alpha) setStroke];
            NSBezierPath *p = [NSBezierPath bezierPath];
            [p moveToPoint:NSMakePoint(c.points[0].x, c.points[0].y)];
            for (size_t i = 1; i < c.points.size(); ++i) [p lineToPoint:NSMakePoint(c.points[i].x, c.points[i].y)];
            [p setLineWidth:std::max(0.5f, c.lineWidth)];
            [p stroke];
        } else if (c.type == "FILL_POLYGON" && c.points.size() >= 3) {
            [MemeMacHostInternal::nsColorFromHex(c.color, c.alpha) setFill];
            NSBezierPath *p = [NSBezierPath bezierPath];
            [p moveToPoint:NSMakePoint(c.points[0].x, c.points[0].y)];
            for (size_t i = 1; i < c.points.size(); ++i) [p lineToPoint:NSMakePoint(c.points[i].x, c.points[i].y)];
            [p closePath];
            [p fill];
        } else if (c.type == "DRAW_TEXT" && !c.text.empty()) {
            NSFont *font = c.isMono
                ? [NSFont monospacedSystemFontOfSize:c.fontSize weight:NSFontWeightRegular]
                : (c.fontWeight >= 600 ? [NSFont boldSystemFontOfSize:c.fontSize] : [NSFont systemFontOfSize:c.fontSize]);
            NSDictionary *attrs = @{
                NSFontAttributeName: font,
                NSForegroundColorAttributeName: MemeMacHostInternal::nsColorFromHex(c.color, c.alpha)
            };
            NSString *nsTxt = [NSString stringWithUTF8String:c.text.c_str()];
            if (nsTxt) [nsTxt drawAtPoint:NSMakePoint(c.x, c.y - c.fontSize * 0.9f) withAttributes:attrs];
        }
    }
    while (ctx && clipDepth-- > 0) CGContextRestoreGState(ctx);
    (void)dirtyRect;
}

- (void)refreshCursorAt:(NSPoint)pt {
    if (!_session) return;
    const auto *hr = _session->hitTestTopmost(static_cast<float>(pt.x), static_cast<float>(pt.y));
    std::string cur = hr ? hr->cursor : "default";
    if (cur == "pointer") [[NSCursor pointingHandCursor] set];
    else if (cur == "text") [[NSCursor IBeamCursor] set];
    else if (cur == "col-resize") [[NSCursor resizeLeftRightCursor] set];
    else if (cur == "row-resize") [[NSCursor resizeUpDownCursor] set];
    else [[NSCursor arrowCursor] set];
}

- (void)mouseMoved:(NSEvent *)event {
    if (!_session) return;
    NSPoint pt = [self convertPoint:[event locationInWindow] fromView:nil];
    if (_sessionMutex) _sessionMutex->lock();
    bool ch = _session->handlePointerMove(static_cast<float>(pt.x), static_cast<float>(pt.y));
    if (_sessionMutex) _sessionMutex->unlock();
    [self refreshCursorAt:pt];
    if (ch) {
        if (_onNeedRender && *_onNeedRender) (*_onNeedRender)();
        [self setNeedsDisplay:YES];
    }
}

- (void)mouseDown:(NSEvent *)event {
    if (!_session) return;
    [[self window] makeFirstResponder:self];
    NSPoint pt = [self convertPoint:[event locationInWindow] fromView:nil];
    if (_sessionMutex) _sessionMutex->lock();
    _session->handlePointerDown(static_cast<float>(pt.x), static_cast<float>(pt.y), 0, static_cast<int>([event clickCount]));
    if (_sessionMutex) _sessionMutex->unlock();
    if (_onNeedRender && *_onNeedRender) (*_onNeedRender)();
    [self setNeedsDisplay:YES];
}

- (void)mouseDragged:(NSEvent *)event {
    if (!_session) return;
    NSPoint pt = [self convertPoint:[event locationInWindow] fromView:nil];
    if (_sessionMutex) _sessionMutex->lock();
    bool ch = _session->handlePointerMove(static_cast<float>(pt.x), static_cast<float>(pt.y));
    if (_sessionMutex) _sessionMutex->unlock();
    if (ch) {
        if (_onNeedRender && *_onNeedRender) (*_onNeedRender)();
        [self setNeedsDisplay:YES];
    }
}

- (void)mouseUp:(NSEvent *)event {
    if (!_session) return;
    NSPoint pt = [self convertPoint:[event locationInWindow] fromView:nil];
    if (_sessionMutex) _sessionMutex->lock();
    _session->handlePointerUp(static_cast<float>(pt.x), static_cast<float>(pt.y), 0);
    if (_sessionMutex) _sessionMutex->unlock();
    if (_onNeedRender && *_onNeedRender) (*_onNeedRender)();
    [self setNeedsDisplay:YES];
}

- (void)rightMouseDown:(NSEvent *)event {
    if (!_session) return;
    NSPoint pt = [self convertPoint:[event locationInWindow] fromView:nil];
    if (_sessionMutex) _sessionMutex->lock();
    _session->handlePointerDown(static_cast<float>(pt.x), static_cast<float>(pt.y), 2, 1);
    if (_sessionMutex) _sessionMutex->unlock();
    if (_onNeedRender && *_onNeedRender) (*_onNeedRender)();
    [self setNeedsDisplay:YES];
}

- (void)rightMouseUp:(NSEvent *)event {
    if (!_session) return;
    NSPoint pt = [self convertPoint:[event locationInWindow] fromView:nil];
    if (_sessionMutex) _sessionMutex->lock();
    _session->handlePointerUp(static_cast<float>(pt.x), static_cast<float>(pt.y), 2);
    if (_sessionMutex) _sessionMutex->unlock();
    if (_onNeedRender && *_onNeedRender) (*_onNeedRender)();
    [self setNeedsDisplay:YES];
}

- (void)scrollWheel:(NSEvent *)event {
    if (!_session) return;
    NSPoint pt = [self convertPoint:[event locationInWindow] fromView:nil];
    float dx = static_cast<float>([event scrollingDeltaX]);
    float dy = static_cast<float>([event scrollingDeltaY]);
    float scaleDiv = [event hasPreciseScrollingDeltas] ? (1.0f / 28.0f) : 0.5f;
    if (_sessionMutex) _sessionMutex->lock();
    bool ch = _session->handleScroll(static_cast<float>(pt.x), static_cast<float>(pt.y), dx * scaleDiv, dy * scaleDiv);
    if (_sessionMutex) _sessionMutex->unlock();
    if (ch) {
        if (_onNeedRender && *_onNeedRender) (*_onNeedRender)();
        [self setNeedsDisplay:YES];
    }
}

- (void)keyDown:(NSEvent *)event {
    if (!_session) return;
    MemeUI::UiKeyEvent kev;
    NSEventModifierFlags mods = [event modifierFlags];
    kev.shift = (mods & NSEventModifierFlagShift) != 0;
    kev.ctrl = (mods & NSEventModifierFlagControl) != 0;
    kev.alt = (mods & NSEventModifierFlagOption) != 0;
    kev.meta = (mods & NSEventModifierFlagCommand) != 0;
    kev.key = MemeMacHostInternal::mapCocoaKey(event);
    kev.text = MemeMacHostInternal::mapCocoaText(event);

    if (_sessionMutex) _sessionMutex->lock();
    bool ch = _session->handleKeyEvent(kev);
    if (_sessionMutex) _sessionMutex->unlock();
    if (ch) {
        if (_onNeedRender && *_onNeedRender) (*_onNeedRender)();
        [self setNeedsDisplay:YES];
    }
}

- (void)dealloc {
    if (_trackingArea) { MEME_OBJC_RELEASE(_trackingArea); _trackingArea = nil; }
    MEME_OBJC_RELEASE(_atlasTex);
    MEME_OBJC_RELEASE(_pipeline);
    MEME_OBJC_RELEASE(_queue);
    MEME_OBJC_RELEASE(_device);
#if !__has_feature(objc_arc)
    [super dealloc];
#endif
}

@end

@interface MemeMacWindowDelegate : NSObject <NSWindowDelegate>
@property (nonatomic, assign) bool *openFlag;
@end

@implementation MemeMacWindowDelegate
- (void)windowWillClose:(NSNotification *)notification {
    (void)notification;
    if (_openFlag) *_openFlag = false;
}
@end

namespace MemeUI {

inline void bindMacClipboard(UiSession &session) {
    session.clipboardReadFn = []() -> std::string {
        @autoreleasepool {
            NSPasteboard *pb = [NSPasteboard generalPasteboard];
            NSString *s = [pb stringForType:NSPasteboardTypeString];
            return s ? std::string([s UTF8String]) : "";
        }
    };
    session.clipboardWriteFn = [](const std::string &txt) {
        @autoreleasepool {
            NSPasteboard *pb = [NSPasteboard generalPasteboard];
            [pb clearContents];
            NSString *ns = [NSString stringWithUTF8String:txt.c_str()];
            if (ns) [pb setString:ns forType:NSPasteboardTypeString];
        }
    };
}

inline void bindMacNativeHooks(UiSession &session, NSWindow *win, NSView *view) {
    auto &h = MemeNative::hooks();
    h.dispatchMainThread = [](std::function<void()> fn) {
        if (std::getenv("MEME_HEADLESS") != nullptr) { if (fn) fn(); return; }
        dispatch_async(dispatch_get_main_queue(), ^{ if (fn) fn(); });
    };
    h.requestRerender = [&session, view]() {
        session.needsRender = true;
        if (view) dispatch_async(dispatch_get_main_queue(), ^{ [view setNeedsDisplay:YES]; });
    };
    h.clipboardGet = []() -> std::string {
        @autoreleasepool {
            NSString *s = [[NSPasteboard generalPasteboard] stringForType:NSPasteboardTypeString];
            return s ? std::string([s UTF8String]) : "";
        }
    };
    h.clipboardSet = [](const std::string &txt) -> bool {
        @autoreleasepool {
            NSPasteboard *pb = [NSPasteboard generalPasteboard];
            [pb clearContents];
            NSString *ns = [NSString stringWithUTF8String:txt.c_str()];
            return ns ? ([pb setString:ns forType:NSPasteboardTypeString] != NO) : false;
        }
    };
    h.hapticImpact = [](const std::string &style) -> bool {
        @autoreleasepool {
            NSHapticFeedbackPattern pat = NSHapticFeedbackPatternGeneric;
            if (style == "light") pat = NSHapticFeedbackPatternAlignment;
            else if (style == "heavy") pat = NSHapticFeedbackPatternLevelChange;
            [[NSHapticFeedbackManager defaultPerformer] performFeedbackPattern:pat performanceTime:NSHapticFeedbackPerformanceTimeNow];
            return true;
        }
    };
    h.hapticSelection = []() -> bool {
        @autoreleasepool {
            [[NSHapticFeedbackManager defaultPerformer] performFeedbackPattern:NSHapticFeedbackPatternAlignment performanceTime:NSHapticFeedbackPerformanceTimeNow];
            return true;
        }
    };
    h.canOpenUrl = [](const std::string &url) -> bool {
        @autoreleasepool {
            NSURL *u = [NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]];
            return u && ([[NSWorkspace sharedWorkspace] URLForApplicationToOpenURL:u] != nil);
        }
    };
    h.openUrl = [](const std::string &url) -> bool {
        @autoreleasepool {
            NSURL *u = [NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]];
            return u && ([[NSWorkspace sharedWorkspace] openURL:u] != NO);
        }
    };
    h.shareAsync = [view](const std::string &text, const std::string &title, const std::string &url, MemeNative::AsyncCallback cb) {
        int rid = MemeNative::registerAsyncCallback(std::move(cb));
        if (std::getenv("MEME_HEADLESS") != nullptr || !view) {
            MemeNative::AsyncResponse r; r.ok = true; r.value = text;
            MemeNative::resolveAsyncCallback(rid, r);
            return;
        }
        NSMutableArray *items = [NSMutableArray array];
        if (!text.empty()) [items addObject:[NSString stringWithUTF8String:text.c_str()]];
        else if (!title.empty()) [items addObject:[NSString stringWithUTF8String:title.c_str()]];
        if (!url.empty()) {
            NSURL *u = [NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]];
            if (u) [items addObject:u];
        }
        dispatch_async(dispatch_get_main_queue(), ^{
            NSSharingServicePicker *picker = [[NSSharingServicePicker alloc] initWithItems:items];
            [picker showRelativeToRect:NSMakeRect(view.bounds.size.width * 0.5, view.bounds.size.height * 0.5, 2, 2) ofView:view preferredEdge:NSRectEdgeMinY];
            MEME_OBJC_RELEASE(picker);
            MemeNative::AsyncResponse r; r.ok = true; r.accepted = true; r.value = text;
            MemeNative::resolveAsyncCallback(rid, r);
        });
    };
    h.alertAsync = [](const std::string &title, const std::string &msg, MemeNative::AsyncCallback cb) {
        int rid = MemeNative::registerAsyncCallback(std::move(cb));
        if (std::getenv("MEME_HEADLESS") != nullptr) { MemeNative::AsyncResponse r; MemeNative::resolveAsyncCallback(rid, r); return; }
        NSString *t = [NSString stringWithUTF8String:title.c_str()];
        NSString *m = [NSString stringWithUTF8String:msg.c_str()];
        dispatch_async(dispatch_get_main_queue(), ^{
            NSAlert *a = [[NSAlert alloc] init];
            [a setMessageText:(t ?: @"")];
            [a setInformativeText:(m ?: @"")];
            [a runModal];
            MEME_OBJC_RELEASE(a);
            MemeNative::AsyncResponse r; r.ok = true; r.accepted = true;
            MemeNative::resolveAsyncCallback(rid, r);
        });
    };
    h.confirmAsync = [](const std::string &title, const std::string &msg, MemeNative::AsyncCallback cb) {
        int rid = MemeNative::registerAsyncCallback(std::move(cb));
        if (std::getenv("MEME_HEADLESS") != nullptr) { MemeNative::AsyncResponse r; r.accepted = true; MemeNative::resolveAsyncCallback(rid, r); return; }
        NSString *t = [NSString stringWithUTF8String:title.c_str()];
        NSString *m = [NSString stringWithUTF8String:msg.c_str()];
        dispatch_async(dispatch_get_main_queue(), ^{
            NSAlert *a = [[NSAlert alloc] init];
            [a setMessageText:(t ?: @"")];
            [a setInformativeText:(m ?: @"")];
            [a addButtonWithTitle:@"OK"];
            [a addButtonWithTitle:@"Cancel"];
            BOOL yes = ([a runModal] == NSAlertFirstButtonReturn);
            MEME_OBJC_RELEASE(a);
            MemeNative::AsyncResponse r; r.ok = true; r.accepted = yes != NO; r.cancelled = !r.accepted;
            MemeNative::resolveAsyncCallback(rid, r);
        });
    };
    h.promptAsync = [](const std::string &title, const std::string &msg, const std::string &defVal, MemeNative::AsyncCallback cb) {
        int rid = MemeNative::registerAsyncCallback(std::move(cb));
        if (std::getenv("MEME_HEADLESS") != nullptr) { MemeNative::AsyncResponse r; r.value = defVal; MemeNative::resolveAsyncCallback(rid, r); return; }
        NSString *t = [NSString stringWithUTF8String:title.c_str()];
        NSString *m = [NSString stringWithUTF8String:msg.c_str()];
        NSString *d = [NSString stringWithUTF8String:defVal.c_str()];
        dispatch_async(dispatch_get_main_queue(), ^{
            NSAlert *a = [[NSAlert alloc] init];
            [a setMessageText:(t ?: @"")];
            [a setInformativeText:(m ?: @"")];
            [a addButtonWithTitle:@"OK"];
            [a addButtonWithTitle:@"Cancel"];
            NSTextField *tf = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 260, 24)];
            [tf setStringValue:(d ?: @"")];
            [a setAccessoryView:tf];
            BOOL yes = ([a runModal] == NSAlertFirstButtonReturn);
            NSString *val = [tf stringValue] ?: @"";
            MEME_OBJC_RELEASE(tf);
            MEME_OBJC_RELEASE(a);
            MemeNative::AsyncResponse r; r.ok = true; r.accepted = yes != NO; r.cancelled = !r.accepted;
            if (r.accepted) r.value = [val UTF8String];
            MemeNative::resolveAsyncCallback(rid, r);
        });
    };
    h.fileDialogAsync = [](const std::string &mode, const std::string &title, const std::string &defName, MemeNative::AsyncCallback cb) {
        int rid = MemeNative::registerAsyncCallback(std::move(cb));
        if (std::getenv("MEME_HEADLESS") != nullptr) { MemeNative::AsyncResponse r; r.ok = false; r.cancelled = true; MemeNative::resolveAsyncCallback(rid, r); return; }
        NSString *t = [NSString stringWithUTF8String:title.c_str()];
        NSString *d = [NSString stringWithUTF8String:defName.c_str()];
        dispatch_async(dispatch_get_main_queue(), ^{
            MemeNative::AsyncResponse r; r.ok = false; r.accepted = false; r.cancelled = true;
            if (mode == "save") {
                NSSavePanel *sp = [NSSavePanel savePanel];
                if (t.length > 0) [sp setTitle:t];
                if (d.length > 0) [sp setNameFieldStringValue:d];
                if ([sp runModal] == NSModalResponseOK && sp.URL.path) {
                    r.ok = true; r.accepted = true; r.cancelled = false;
                    r.path = [sp.URL.path UTF8String]; r.value = r.path; r.paths.push_back(r.path);
                }
            } else {
                NSOpenPanel *op = [NSOpenPanel openPanel];
                if (t.length > 0) [op setTitle:t];
                [op setCanChooseDirectories:(mode == "folder")];
                [op setCanChooseFiles:(mode != "folder")];
                if ([op runModal] == NSModalResponseOK) {
                    for (NSURL *u in op.URLs) {
                        if (u.path) r.paths.push_back([u.path UTF8String]);
                    }
                    if (!r.paths.empty()) {
                        r.ok = true; r.accepted = true; r.cancelled = false;
                        r.path = r.paths.front(); r.value = r.path;
                    }
                }
            }
            MemeNative::resolveAsyncCallback(rid, r);
        });
    };
    h.getDeviceInfo = []() -> MemeNative::DeviceInfo {
        MemeNative::DeviceInfo d;
        @autoreleasepool {
            d.os = "mac";
            d.arch = MemeNative::defaultArchName();
            d.model = "Mac";
            NSOperatingSystemVersion v = [[NSProcessInfo processInfo] operatingSystemVersion];
            d.osVersion = std::to_string(v.majorVersion) + "." + std::to_string(v.minorVersion) + "." + std::to_string(v.patchVersion);
            d.locale = [[[NSLocale currentLocale] localeIdentifier] UTF8String] ?: "en_US";
            d.timezone = [[[NSTimeZone localTimeZone] name] UTF8String] ?: "UTC";
            NSString *apName = NSApp ? NSApp.effectiveAppearance.name : @"NSAppearanceNameDarkAqua";
            d.isDark = ([apName rangeOfString:@"Dark" options:NSCaseInsensitiveSearch].location != NSNotFound);
            d.theme = d.isDark ? "dark" : "light";
            d.isMobile = false; d.isDesktop = true; d.isWeb = false;
        }
        return d;
    };
    h.getScreenInfo = [&session, win]() -> MemeNative::ScreenInfo {
        MemeNative::ScreenInfo s;
        s.width = session.input.viewportW > 0 ? session.input.viewportW : 1040.0f;
        s.height = session.input.viewportH > 0 ? session.input.viewportH : 720.0f;
        s.scale = win ? static_cast<float>([win backingScaleFactor]) : static_cast<float>([[NSScreen mainScreen] backingScaleFactor]);
        return s;
    };
    h.biometricsAvailable = []() -> bool {
        void *la = dlopen("/System/Library/Frameworks/LocalAuthentication.framework/LocalAuthentication", RTLD_LAZY);
        Class laCls = la ? NSClassFromString(@"LAContext") : nil;
        if (!laCls) return false;
        id ctx = [[laCls alloc] init];
        NSError *err = nil;
        BOOL ok = ((BOOL (*)(id, SEL, NSInteger, NSError **))objc_msgSend)(ctx, NSSelectorFromString(@"canEvaluatePolicy:error:"), 2, &err);
        MEME_OBJC_RELEASE(ctx);
        return ok != NO;
    };
    h.biometricsAuthAsync = [](const std::string &reason, MemeNative::AsyncCallback cb) {
        int rid = MemeNative::registerAsyncCallback(std::move(cb));
        void *la = dlopen("/System/Library/Frameworks/LocalAuthentication.framework/LocalAuthentication", RTLD_LAZY);
        Class laCls = la ? NSClassFromString(@"LAContext") : nil;
        if (!laCls) { MemeNative::AsyncResponse r; r.ok = false; r.error = "LAContext unavailable"; MemeNative::resolveAsyncCallback(rid, r); return; }
        id ctx = [[laCls alloc] init];
        NSString *nsR = [NSString stringWithUTF8String:(reason.empty() ? "Authenticate" : reason.c_str())];
        void (^reply)(BOOL, NSError *) = ^(BOOL ok, NSError *err) {
            MemeNative::AsyncResponse r; r.ok = (ok != NO); r.accepted = r.ok; r.cancelled = !r.ok;
            if (err && err.localizedDescription) r.error = [err.localizedDescription UTF8String];
            MemeNative::resolveAsyncCallback(rid, r);
        };
        ((void (*)(id, SEL, NSInteger, id, id))objc_msgSend)(ctx, NSSelectorFromString(@"evaluatePolicy:localizedReason:reply:"), 2, nsR, reply);
        MEME_OBJC_RELEASE(ctx);
    };
    h.beep = [](double, int) -> bool { NSBeep(); return true; };
    h.setWindowTitle = [win](const std::string &title) -> bool {
        if (!win) return false;
        NSString *ns = [NSString stringWithUTF8String:title.c_str()];
        dispatch_async(dispatch_get_main_queue(), ^{ [win setTitle:(ns ?: @"")]; });
        return true;
    };
    h.setWindowSize = [win](int w, int hSz) -> bool {
        if (!win) return false;
        dispatch_async(dispatch_get_main_queue(), ^{
            [win setContentSize:NSMakeSize(std::max(160, w), std::max(120, hSz))];
        });
        return true;
    };
    h.setFullscreen = [win](bool on) -> bool {
        if (!win) return false;
        dispatch_async(dispatch_get_main_queue(), ^{
            bool isFs = (([win styleMask] & NSWindowStyleMaskFullScreen) != 0);
            if (on != isFs) [win toggleFullScreen:nil];
        });
        return true;
    };
    h.minimizeWindow = [win]() -> bool {
        if (!win) return false;
        dispatch_async(dispatch_get_main_queue(), ^{ [win miniaturize:nil]; });
        return true;
    };
    h.focusWindow = [win]() -> bool {
        if (!win) return false;
        dispatch_async(dispatch_get_main_queue(), ^{
            [NSApp activateIgnoringOtherApps:YES];
            [win makeKeyAndOrderFront:nil];
        });
        return true;
    };
}

inline void applyMacAppIcon() {
    @autoreleasepool {
        std::vector<uint8_t> rgba;
        int iw = 0, ih = 0;
        MemeNative::ensureDefaultAppIconRgba(128, rgba, iw, ih);
        if (rgba.empty() || iw <= 0 || ih <= 0) return;
        NSBitmapImageRep *rep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:nil
                                                                        pixelsWide:iw
                                                                        pixelsHigh:ih
                                                                     bitsPerSample:8
                                                                   samplesPerPixel:4
                                                                          hasAlpha:YES
                                                                          isPlanar:NO
                                                                    colorSpaceName:NSCalibratedRGBColorSpace
                                                                       bytesPerRow:iw * 4
                                                                      bitsPerPixel:32];
        if (!rep) return;
        std::memcpy([rep bitmapData], rgba.data(), rgba.size());
        NSImage *img = [[NSImage alloc] initWithSize:NSMakeSize(iw, ih)];
        [img addRepresentation:rep];
        [NSApp setApplicationIconImage:img];
        MEME_OBJC_RELEASE(img);
        MEME_OBJC_RELEASE(rep);
    }
}

inline int runMacNativeApp(const std::string &title, UiSession &session, int width = 1040, int height = 720, std::function<void()> customRenderFn = nullptr) {
    if (std::getenv("MEME_HEADLESS") != nullptr) {
        bindMacNativeHooks(session, nil, nil);
        session.renderFrame(static_cast<float>(width), static_cast<float>(height));
        return 0;
    }
    bindMacClipboard(session);
    @autoreleasepool {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        applyMacAppIcon();

        NSRect frame = NSMakeRect(120, 120, width, height);
        NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                                  NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
        NSWindow *win = [[NSWindow alloc] initWithContentRect:frame styleMask:style backing:NSBackingStoreBuffered defer:NO];
        NSString *nsTitle = [NSString stringWithUTF8String:title.c_str()];
        [win setTitle:(nsTitle ? nsTitle : @"MEME-UI")];
        [win setAcceptsMouseMovedEvents:YES];

        bool isOpen = true;
        MemeMacWindowDelegate *del = [[MemeMacWindowDelegate alloc] init];
        del.openFlag = &isOpen;
        [win setDelegate:del];

        std::function<void()> onRender = [&session, width, height, customRenderFn]() {
            if (customRenderFn) {
                customRenderFn();
                return;
            }
            float vw = session.input.viewportW > 0 ? session.input.viewportW : static_cast<float>(width);
            float vh = session.input.viewportH > 0 ? session.input.viewportH : static_cast<float>(height);
            session.renderFrame(vw, vh);
        };

        MemeNativeGpuView *view = [[MemeNativeGpuView alloc] initWithFrame:NSMakeRect(0, 0, width, height)];
        view.session = &session;
        view.onNeedRender = &onRender;
        [view setupRenderer];
        bindMacNativeHooks(session, win, view);
        [win setContentView:view];
        [win makeFirstResponder:view];
        [win center];
        [win makeKeyAndOrderFront:nil];
        [NSApp activateIgnoringOtherApps:YES];

        session.renderFrame(static_cast<float>(width), static_cast<float>(height));
        [view setNeedsDisplay:YES];

        while (isOpen) {
            @autoreleasepool {
                NSEvent *ev = [NSApp nextEventMatchingMask:NSEventMaskAny
                                                 untilDate:[NSDate dateWithTimeIntervalSinceNow:0.016]
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
                if (session.tick(16.0) || session.isImmediateMode || session.needsRender) {
                    onRender();
                    [view setNeedsDisplay:YES];
                }
            }
        }
        MEME_OBJC_RELEASE(del);
        MEME_OBJC_RELEASE(view);
        MEME_OBJC_RELEASE(win);
    }
    return 0;
}

} // namespace MemeUI

#endif // __APPLE__ && __OBJC__
