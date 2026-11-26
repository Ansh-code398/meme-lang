#if defined(__APPLE__) && defined(__OBJC__)
#import <TargetConditionals.h>
#if TARGET_OS_IPHONE

#import <UIKit/UIKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <QuartzCore/CADisplayLink.h>
#import <CoreGraphics/CoreGraphics.h>
#include "../../ui/MemeUiTypes.h"
#include "../../ui/MemeUiTessellator.h"
#include "../../ui/MemeUiRuntime.h"
#include <algorithm>
#include <string>
#include <vector>

#if __has_feature(objc_arc)
#define MEME_IOS_RELEASE(x) ((void)0)
#else
#define MEME_IOS_RELEASE(x) [(x) release]
#endif

extern "C" int meme_user_entrypoint(int argc, char *argv[]);
extern "C" MemeUI::UiSession *meme_host_get_session();
extern "C" void meme_host_rerender();

namespace {

NSString *const kMemeIosMetalShader = @R"(
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

vertex MemeFragIn meme_ios_vertex(
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
    out.color = float4(
        float(v.rgba & 0xFFu) / 255.0f,
        float((v.rgba >> 8) & 0xFFu) / 255.0f,
        float((v.rgba >> 16) & 0xFFu) / 255.0f,
        float((v.rgba >> 24) & 0xFFu) / 255.0f);
    return out;
}

fragment float4 meme_ios_fragment(
    MemeFragIn in [[stage_in]],
    texture2d<float> atlas [[texture(0)]])
{
    constexpr sampler s(coord::normalized, address::clamp_to_edge, filter::linear);
    float4 tex = atlas.sample(s, in.uv);
    return float4(in.color.rgb * tex.rgb, in.color.a * tex.a);
}
)";

UIColor *uiColorFromHex(const std::string &hexStr, float alpha) {
    MemeUI::ColorRgba c = MemeUI::hexToRgba(hexStr, alpha);
    return [UIColor colorWithRed:c.r green:c.g blue:c.b alpha:c.a];
}

void bindIosClipboard(MemeUI::UiSession *sess) {
    if (!sess) return;
    sess->clipboardReadFn = []() -> std::string {
        @autoreleasepool {
            NSString *s = [UIPasteboard generalPasteboard].string;
            return s ? std::string([s UTF8String]) : "";
        }
    };
    sess->clipboardWriteFn = [](const std::string &txt) {
        @autoreleasepool {
            NSString *ns = [NSString stringWithUTF8String:txt.c_str()];
            if (ns) [UIPasteboard generalPasteboard].string = ns;
        }
    };
}

} // namespace

@interface MemeIosRenderView : UIView <UIKeyInput>
@property (nonatomic, assign) MemeUI::UiSession *session;
@property (nonatomic, assign) BOOL useMetal;
- (void)setupRenderer;
- (void)renderFrameNow;
@end

@implementation MemeIosRenderView {
    id<MTLDevice> _device;
    id<MTLCommandQueue> _queue;
    id<MTLRenderPipelineState> _pipeline;
    id<MTLTexture> _atlasTex;
    CAMetalLayer *_metalLayer;
    UITouch *_activeTouch;
    CGPoint _touchStartPt;
    CGPoint _lastTouchPt;
    BOOL _touchScrolled;
    float _scrollVelX;
    float _scrollVelY;
}

+ (Class)layerClass {
    return [CAMetalLayer class];
}

- (BOOL)canBecomeFirstResponder { return YES; }

- (BOOL)hasText {
    if (!_session) return NO;
    const auto &fk = _session->interaction.focusedKey;
    auto it = _session->interaction.textFields.find(fk);
    return (it != _session->interaction.textFields.end() && !it->second.text.empty());
}

- (void)insertText:(NSString *)text {
    if (!_session || !text) return;
    std::string utf8 = [text UTF8String];
    MemeUI::UiKeyEvent kev;
    if (utf8 == "\n" || utf8 == "\r") {
        kev.key = "Enter";
    } else {
        kev.key = utf8;
        kev.text = utf8;
    }
    if (_session->handleKeyEvent(kev)) {
        meme_host_rerender();
        [self renderFrameNow];
    }
}

- (void)deleteBackward {
    if (!_session) return;
    MemeUI::UiKeyEvent kev;
    kev.key = "Backspace";
    if (_session->handleKeyEvent(kev)) {
        meme_host_rerender();
        [self renderFrameNow];
    }
}

- (void)setupRenderer {
    _useMetal = NO;
    self.multipleTouchEnabled = NO;
    self.backgroundColor = uiColorFromHex("#020617", 1.0f);

    _device = MTLCreateSystemDefaultDevice();
    if (!_device) return;
    _queue = [_device newCommandQueue];
    NSError *err = nil;
    id<MTLLibrary> lib = [_device newLibraryWithSource:kMemeIosMetalShader options:nil error:&err];
    if (!lib || err) return;

    id<MTLFunction> vfn = [lib newFunctionWithName:@"meme_ios_vertex"];
    id<MTLFunction> ffn = [lib newFunctionWithName:@"meme_ios_fragment"];
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
    MEME_IOS_RELEASE(desc);
    MEME_IOS_RELEASE(vfn);
    MEME_IOS_RELEASE(ffn);
    MEME_IOS_RELEASE(lib);
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

    _metalLayer = (CAMetalLayer *)self.layer;
    _metalLayer.device = _device;
    _metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    CGFloat initScale = self.traitCollection.displayScale > 0 ? self.traitCollection.displayScale : 2.0;
    _metalLayer.contentsScale = initScale;
    _useMetal = YES;
}

- (void)layoutSubviews {
    [super layoutSubviews];
    CGFloat scale = self.window ? self.window.screen.scale : (self.traitCollection.displayScale > 0 ? self.traitCollection.displayScale : 2.0);
    if (_metalLayer) {
        _metalLayer.contentsScale = scale;
        _metalLayer.drawableSize = CGSizeMake(std::max(1.0, self.bounds.size.width * scale),
                                              std::max(1.0, self.bounds.size.height * scale));
    }
    if (_session) {
        _session->input.viewportW = static_cast<float>(self.bounds.size.width);
        _session->input.viewportH = static_cast<float>(self.bounds.size.height);
        _session->needsRender = true;
        meme_host_rerender();
    }
    [self renderFrameNow];
}

- (void)renderFrameNow {
    if (!_session) return;
    float vw = static_cast<float>(self.bounds.size.width);
    float vh = static_cast<float>(self.bounds.size.height);
    if (vw <= 1.0f || vh <= 1.0f) return;

    if (_session->needsRender || _session->lastMesh.vertices.empty()) {
        _session->renderFrame(vw, vh);
    }

    if (!_useMetal || !_metalLayer || !_pipeline) {
        [self setNeedsDisplay];
        return;
    }

    CGFloat scale = _metalLayer.contentsScale > 0 ? _metalLayer.contentsScale : 1.0;
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

        const auto &mesh = _session->lastMesh;
        if (!mesh.vertices.empty() && !mesh.indices.empty()) {
            [enc setRenderPipelineState:_pipeline];
            id<MTLBuffer> vbuf = [_device newBufferWithBytes:mesh.vertices.data()
                                                      length:mesh.vertices.size() * sizeof(MemeUI::UiVertex)
                                                     options:MTLResourceStorageModeShared];
            id<MTLBuffer> ibuf = [_device newBufferWithBytes:mesh.indices.data()
                                                      length:mesh.indices.size() * sizeof(uint32_t)
                                                     options:MTLResourceStorageModeShared];
            struct { float vw; float vh; } uni{vw, vh};
            [enc setVertexBuffer:vbuf offset:0 atIndex:0];
            [enc setVertexBytes:&uni length:sizeof(uni) atIndex:1];
            [enc setFragmentTexture:_atlasTex atIndex:0];

            NSUInteger dw = drawable.texture.width;
            NSUInteger dh = drawable.texture.height;
            for (const auto &batch : mesh.batches) {
                if (batch.indexCount == 0) continue;
                NSUInteger sx = static_cast<NSUInteger>(std::clamp(batch.clipRect.x * static_cast<float>(scale), 0.0f, static_cast<float>(dw)));
                NSUInteger sy = static_cast<NSUInteger>(std::clamp(batch.clipRect.y * static_cast<float>(scale), 0.0f, static_cast<float>(dh)));
                NSUInteger ex = static_cast<NSUInteger>(std::clamp((batch.clipRect.x + batch.clipRect.width) * static_cast<float>(scale), 0.0f, static_cast<float>(dw)));
                NSUInteger ey = static_cast<NSUInteger>(std::clamp((batch.clipRect.y + batch.clipRect.height) * static_cast<float>(scale), 0.0f, static_cast<float>(dh)));
                if (ex <= sx || ey <= sy) continue;
                [enc setScissorRect:MTLScissorRect{sx, sy, ex - sx, ey - sy}];
                [enc drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                indexCount:batch.indexCount
                                 indexType:MTLIndexTypeUInt32
                               indexBuffer:ibuf
                         indexBufferOffset:batch.indexOffset * sizeof(uint32_t)];
            }
            MEME_IOS_RELEASE(vbuf);
            MEME_IOS_RELEASE(ibuf);
        }
        [enc endEncoding];
        [cmdBuf presentDrawable:drawable];
        [cmdBuf commit];
    }
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    (void)event;
    if (!_session) return;
    UITouch *t = [touches anyObject];
    if (!t) return;
    _activeTouch = t;
    CGPoint pt = [t locationInView:self];
    _touchStartPt = pt;
    _lastTouchPt = pt;
    _touchScrolled = NO;
    _scrollVelX = 0.0f;
    _scrollVelY = 0.0f;
    _session->handlePointerDown(static_cast<float>(pt.x), static_cast<float>(pt.y), 0, static_cast<int>(t.tapCount));
    const auto *hr = _session->hitTestTopmost(static_cast<float>(pt.x), static_cast<float>(pt.y));
    if (hr && (hr->tag == "input" || hr->tag == "textarea")) [self becomeFirstResponder];
    else [self resignFirstResponder];
    meme_host_rerender();
    [self renderFrameNow];
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    (void)event;
    if (!_session) return;
    UITouch *t = (_activeTouch && [touches containsObject:_activeTouch]) ? _activeTouch : [touches anyObject];
    if (!t) return;
    CGPoint pt = [t locationInView:self];
    float dy = static_cast<float>(pt.y - _lastTouchPt.y);
    float dx = static_cast<float>(pt.x - _lastTouchPt.x);
    _lastTouchPt = pt;
    float totalDist = std::hypot(static_cast<float>(pt.x - _touchStartPt.x), static_cast<float>(pt.y - _touchStartPt.y));
    bool scrolled = false;
    if (totalDist > 6.0f && !_session->interaction.draggingSlider) {
        _touchScrolled = YES;
        _session->interaction.activeId = 0;
        _session->interaction.activeKey.clear();
        for (auto &kv : _session->interaction.textFields) kv.second.dragging = false;
        if (dx != 0.0f || dy != 0.0f) {
            scrolled = _session->handleScroll(static_cast<float>(pt.x), static_cast<float>(pt.y), dx / 28.0f, dy / 28.0f);
            if (scrolled) {
                _scrollVelX = _scrollVelX * 0.55f + dx * 0.45f;
                _scrollVelY = _scrollVelY * 0.55f + dy * 0.45f;
            }
        }
    }
    bool moved = !_touchScrolled && _session->handlePointerMove(static_cast<float>(pt.x), static_cast<float>(pt.y));
    if (scrolled || moved) {
        meme_host_rerender();
        [self renderFrameNow];
    }
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    (void)event;
    if (!_session) return;
    UITouch *t = (_activeTouch && [touches containsObject:_activeTouch]) ? _activeTouch : [touches anyObject];
    if (!t) return;
    _activeTouch = nil;
    CGPoint pt = [t locationInView:self];
    if (_touchScrolled) {
        _session->handlePointerUp(-9999.0f, -9999.0f, 0);
        if (std::hypot(_scrollVelX, _scrollVelY) <= 1.2f) {
            _scrollVelX = 0.0f;
            _scrollVelY = 0.0f;
        }
    } else {
        _scrollVelX = 0.0f;
        _scrollVelY = 0.0f;
        _session->handlePointerUp(static_cast<float>(pt.x), static_cast<float>(pt.y), 0);
    }
    _touchScrolled = NO;
    meme_host_rerender();
    [self renderFrameNow];
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    _scrollVelX = 0.0f;
    _scrollVelY = 0.0f;
    _touchScrolled = YES;
    [self touchesEnded:touches withEvent:event];
}

- (BOOL)stepScrollInertia {
    if (!_session || _activeTouch) return NO;
    if (std::abs(_scrollVelX) <= 0.25f && std::abs(_scrollVelY) <= 0.25f) {
        _scrollVelX = 0.0f;
        _scrollVelY = 0.0f;
        return NO;
    }
    bool scrolled = _session->handleScroll(static_cast<float>(_lastTouchPt.x), static_cast<float>(_lastTouchPt.y), _scrollVelX / 28.0f, _scrollVelY / 28.0f);
    if (scrolled) {
        _scrollVelX *= 0.92f;
        _scrollVelY *= 0.92f;
        return YES;
    }
    _scrollVelX = 0.0f;
    _scrollVelY = 0.0f;
    return NO;
}

@end

@interface MemeIosViewController : UIViewController
@property (nonatomic, strong) MemeIosRenderView *renderView;
@property (nonatomic, strong) CADisplayLink *displayLink;
@end

@implementation MemeIosViewController
- (UIStatusBarStyle)preferredStatusBarStyle {
    return UIStatusBarStyleLightContent;
}

- (void)loadView {
    MemeIosRenderView *rv = [[MemeIosRenderView alloc] initWithFrame:CGRectMake(0, 0, 393, 852)];
    rv.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    rv.session = meme_host_get_session();
    bindIosClipboard(rv.session);
    [rv setupRenderer];
    self.renderView = rv;
    self.view = rv;
    MEME_IOS_RELEASE(rv);
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    if (!self.displayLink) {
        self.displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(onTick:)];
        [self.displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
    }
}

- (void)viewSafeAreaInsetsDidChange {
    [super viewSafeAreaInsetsDidChange];
    if (auto *s = meme_host_get_session()) {
        UIEdgeInsets ins = self.view.safeAreaInsets;
        s->input.safeTop = static_cast<float>(ins.top);
        s->input.safeBottom = static_cast<float>(ins.bottom);
        s->input.safeLeft = static_cast<float>(ins.left);
        s->input.safeRight = static_cast<float>(ins.right);
        s->needsRender = true;
        meme_host_rerender();
        [self.renderView renderFrameNow];
    }
}

- (void)onTick:(CADisplayLink *)link {
    (void)link;
    if (auto *s = meme_host_get_session()) {
        bool inertiaMoved = [self.renderView stepScrollInertia] ? true : false;
        if (inertiaMoved || s->tick(16.0) || s->isImmediateMode || s->needsRender) {
            meme_host_rerender();
            [self.renderView renderFrameNow];
        }
    }
}
@end

@interface MemeIosSceneDelegate : UIResponder <UIWindowSceneDelegate>
@property (nonatomic, strong) UIWindow *window;
@end

@implementation MemeIosSceneDelegate
- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connectionOptions {
    (void)session; (void)connectionOptions;
    if (![scene isKindOfClass:[UIWindowScene class]]) return;
    UIWindowScene *ws = (UIWindowScene *)scene;
    self.window = [[UIWindow alloc] initWithWindowScene:ws];
    MemeIosViewController *vc = [[MemeIosViewController alloc] init];
    self.window.rootViewController = vc;
    [self.window makeKeyAndVisible];
    MEME_IOS_RELEASE(vc);
}
@end

@interface MemeIosAppDelegate : UIResponder <UIApplicationDelegate>
@property (nonatomic, strong) UIWindow *window;
@end

@implementation MemeIosAppDelegate
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    (void)application; (void)launchOptions;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        char arg0[] = "meme_ios";
        char *argv[] = {arg0};
        meme_user_entrypoint(1, argv);
    });
    return YES;
}

- (UISceneConfiguration *)application:(UIApplication *)application
    configurationForConnectingSceneSession:(UISceneSession *)connectingSceneSession
                                   options:(UISceneConnectionOptions *)options {
    (void)application; (void)options;
    UISceneConfiguration *cfg = [[UISceneConfiguration alloc] initWithName:@"Default Configuration"
                                                               sessionRole:connectingSceneSession.role];
    cfg.delegateClass = [MemeIosSceneDelegate class];
    return cfg;
}
@end

int main(int argc, char *argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([MemeIosAppDelegate class]));
    }
}

#endif // TARGET_OS_IPHONE
#endif // __APPLE__ && __OBJC__
