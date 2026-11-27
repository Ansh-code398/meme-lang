#if defined(__APPLE__) && defined(__OBJC__)
#import <TargetConditionals.h>
#if TARGET_OS_IPHONE

#import <UIKit/UIKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <QuartzCore/CADisplayLink.h>
#import <CoreGraphics/CoreGraphics.h>
#import <UserNotifications/UserNotifications.h>
#include "../../ui/MemeUiTypes.h"
#include "../../ui/MemeUiTessellator.h"
#include "../../ui/MemeUiRuntime.h"
#include "../../platform/MemeNativeApis.h"
#include <dlfcn.h>
#import <objc/message.h>
#include <algorithm>
#include <cmath>
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

@class MemeIosRenderView;
@class MemeIosViewController;

static UIView *g_iosRenderView = nil;
static UIViewController *g_iosRootVc = nil;
static id g_iosMotionMgr = nil;
static id g_iosSpeechSynth = nil;

@interface MemeIosLocHelper : NSObject
@property (nonatomic, strong) id manager;
@property (nonatomic, assign) int reqId;
@property (nonatomic, assign) int permReqId;
- (void)locationManager:(id)manager didUpdateLocations:(NSArray *)locations;
- (void)locationManager:(id)manager didFailWithError:(NSError *)error;
- (void)locationManagerDidChangeAuthorization:(id)manager;
- (void)locationManager:(id)manager didChangeAuthorizationStatus:(int)status;
@end

static MemeIosLocHelper *g_iosLocHelper = nil;

namespace {
inline std::string iosMapCLStatus(int clStatus) {
    if (clStatus == 3 || clStatus == 4) return "granted";
    if (clStatus == 1) return "restricted";
    if (clStatus == 2) return "denied";
    return "prompt";
}
}

@implementation MemeIosLocHelper
- (void)handleAuthChange:(id)mgr status:(int)stVal {
    (void)mgr;
    int prid = self.permReqId;
    if (prid <= 0) return;
    if (stVal == 0) return; // NotDetermined yet
    self.permReqId = 0;
    std::string stateStr = iosMapCLStatus(stVal);
    MemeNative::AsyncResponse res;
    res.permissionState = stateStr;
    res.ok = (stateStr == "granted");
    res.accepted = res.ok;
    res.restricted = (stateStr == "restricted");
    res.canAskAgain = (stateStr == "prompt");
    res.value = stateStr;
    MemeNative::resolveAsyncCallback(prid, res);
}
- (void)locationManagerDidChangeAuthorization:(id)mgr {
    int stVal = 0;
    if ([mgr respondsToSelector:NSSelectorFromString(@"authorizationStatus")]) {
        stVal = ((int (*)(id, SEL))objc_msgSend)(mgr, NSSelectorFromString(@"authorizationStatus"));
    }
    [self handleAuthChange:mgr status:stVal];
}
- (void)locationManager:(id)mgr didChangeAuthorizationStatus:(int)status {
    [self handleAuthChange:mgr status:status];
}
- (void)locationManager:(id)mgr didUpdateLocations:(NSArray *)locations {
    (void)mgr;
    int rid = self.reqId;
    self.reqId = 0;
    SEL stopSel = NSSelectorFromString(@"stopUpdatingLocation");
    if ([self.manager respondsToSelector:stopSel]) {
        ((void (*)(id, SEL))objc_msgSend)(self.manager, stopSel);
    }
    if (rid <= 0) return;
    MemeNative::AsyncResponse res;
    id loc = [locations lastObject];
    if (loc) {
        struct { double lat; double lon; } coord{0.0, 0.0};
        SEL cSel = NSSelectorFromString(@"coordinate");
        NSMethodSignature *sig = [loc methodSignatureForSelector:cSel];
        if (sig && [sig methodReturnLength] == sizeof(coord)) {
            NSInvocation *inv = [NSInvocation invocationWithMethodSignature:sig];
            [inv setTarget:loc];
            [inv setSelector:cSel];
            [inv invoke];
            [inv getReturnValue:&coord];
        }
        double acc = 0.0;
        SEL aSel = NSSelectorFromString(@"horizontalAccuracy");
        NSMethodSignature *asig = [loc methodSignatureForSelector:aSel];
        if (asig && [asig methodReturnLength] == sizeof(double)) {
            NSInvocation *inv = [NSInvocation invocationWithMethodSignature:asig];
            [inv setTarget:loc];
            [inv setSelector:aSel];
            [inv invoke];
            [inv getReturnValue:&acc];
        }
        res.ok = true;
        res.accepted = true;
        res.lat = coord.lat;
        res.lon = coord.lon;
        res.accuracy = acc;
    } else {
        res.ok = false;
        res.accepted = false;
        res.error = "No location fix returned";
    }
    MemeNative::resolveAsyncCallback(rid, res);
}

- (void)locationManager:(id)mgr didFailWithError:(NSError *)error {
    (void)mgr;
    int rid = self.reqId;
    self.reqId = 0;
    if (rid <= 0) return;
    MemeNative::AsyncResponse res;
    res.ok = false;
    res.accepted = false;
    res.error = error && error.localizedDescription ? std::string([error.localizedDescription UTF8String]) : "Location error";
    MemeNative::resolveAsyncCallback(rid, res);
}
@end

@interface MemeIosPickerHelper : NSObject <UIDocumentPickerDelegate, UIImagePickerControllerDelegate, UINavigationControllerDelegate>
@property (nonatomic, assign) int reqId;
@end

static MemeIosPickerHelper *g_iosPickerHelper = nil;

@implementation MemeIosPickerHelper
- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls {
    (void)controller;
    int rid = self.reqId;
    self.reqId = 0;
    MemeNative::AsyncResponse res;
    for (NSURL *u in urls) {
        if (u.path) res.paths.push_back(std::string([u.path UTF8String]));
    }
    if (!res.paths.empty()) {
        res.ok = true; res.accepted = true; res.cancelled = false;
        res.path = res.paths.front();
        res.value = res.path;
    } else {
        res.ok = false; res.accepted = false; res.cancelled = true;
    }
    MemeNative::resolveAsyncCallback(rid, res);
}
- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller {
    (void)controller;
    int rid = self.reqId;
    self.reqId = 0;
    MemeNative::AsyncResponse res;
    res.ok = false; res.accepted = false; res.cancelled = true;
    MemeNative::resolveAsyncCallback(rid, res);
}
- (void)imagePickerController:(UIImagePickerController *)picker didFinishPickingMediaWithInfo:(NSDictionary<UIImagePickerControllerInfoKey, id> *)info {
    [picker dismissViewControllerAnimated:YES completion:nil];
    int rid = self.reqId;
    self.reqId = 0;
    MemeNative::AsyncResponse res;
    NSURL *u = info[UIImagePickerControllerImageURL];
    if (!u) u = info[UIImagePickerControllerMediaURL];
    if (u && u.path) {
        res.ok = true; res.accepted = true; res.cancelled = false;
        res.path = std::string([u.path UTF8String]);
        res.value = res.path;
        res.paths.push_back(res.path);
    } else if (UIImage *img = info[UIImagePickerControllerOriginalImage]) {
        NSString *tmp = [NSTemporaryDirectory() stringByAppendingPathComponent:[NSString stringWithFormat:@"meme_photo_%d.png", rid]];
        NSData *png = UIImagePNGRepresentation(img);
        if (png && [png writeToFile:tmp atomically:YES]) {
            res.ok = true; res.accepted = true; res.cancelled = false;
            res.path = std::string([tmp UTF8String]);
            res.value = res.path;
            res.paths.push_back(res.path);
        } else {
            res.ok = false; res.error = "Failed to export picked image";
        }
    } else {
        res.ok = false; res.accepted = false; res.cancelled = true;
    }
    MemeNative::resolveAsyncCallback(rid, res);
}
- (void)imagePickerControllerDidCancel:(UIImagePickerController *)picker {
    [picker dismissViewControllerAnimated:YES completion:nil];
    int rid = self.reqId;
    self.reqId = 0;
    MemeNative::AsyncResponse res;
    res.ok = false; res.accepted = false; res.cancelled = true;
    MemeNative::resolveAsyncCallback(rid, res);
}
@end

@interface MemeIosNotifDelegate : NSObject <UNUserNotificationCenterDelegate>
@end

@implementation MemeIosNotifDelegate
- (void)userNotificationCenter:(UNUserNotificationCenter *)center
       willPresentNotification:(UNNotification *)notification
         withCompletionHandler:(void (^)(UNNotificationPresentationOptions options))completionHandler
{
    (void)center;
    (void)notification;
    if (completionHandler) {
        completionHandler(UNNotificationPresentationOptionBanner | UNNotificationPresentationOptionList | UNNotificationPresentationOptionSound | UNNotificationPresentationOptionBadge);
    }
}

- (void)userNotificationCenter:(UNUserNotificationCenter *)center
didReceiveNotificationResponse:(UNNotificationResponse *)response
         withCompletionHandler:(void (^)(void))completionHandler
{
    (void)center;
    (void)response;
    if (completionHandler) completionHandler();
}
@end

static MemeIosNotifDelegate *g_iosNotifDelegate = nil;

namespace {

UIViewController *memeIosTopViewController() {
    UIViewController *vc = g_iosRootVc;
    if (!vc) {
        for (UIScene *s in UIApplication.sharedApplication.connectedScenes) {
            if ([s isKindOfClass:[UIWindowScene class]]) {
                for (UIWindow *w in ((UIWindowScene *)s).windows) {
                    if (w.isKeyWindow && w.rootViewController) { vc = w.rootViewController; break; }
                }
            }
            if (vc) break;
        }
    }
    while (vc && vc.presentedViewController) vc = vc.presentedViewController;
    return vc;
}

void bindIosNativeHooks() {
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        auto &h = MemeNative::hooks();
        h.dispatchMainThread = [](std::function<void()> fn) {
            dispatch_async(dispatch_get_main_queue(), ^{
                if (fn) fn();
            });
        };
        h.requestRerender = []() {
            dispatch_async(dispatch_get_main_queue(), ^{
                meme_host_rerender();
                if (g_iosRenderView) [g_iosRenderView setNeedsDisplay];
            });
        };
        h.clipboardGet = []() -> std::string {
            @autoreleasepool {
                NSString *s = [UIPasteboard generalPasteboard].string;
                return s ? std::string([s UTF8String]) : "";
            }
        };
        h.clipboardSet = [](const std::string &txt) -> bool {
            @autoreleasepool {
                NSString *ns = [NSString stringWithUTF8String:txt.c_str()];
                if (ns) [UIPasteboard generalPasteboard].string = ns;
                return ns != nil;
            }
        };
        h.hapticImpact = [](const std::string &style) -> bool {
            dispatch_async(dispatch_get_main_queue(), ^{
                UIImpactFeedbackStyle st = UIImpactFeedbackStyleMedium;
                if (style == "light") st = UIImpactFeedbackStyleLight;
                else if (style == "heavy") st = UIImpactFeedbackStyleHeavy;
                UIImpactFeedbackGenerator *gen = [[UIImpactFeedbackGenerator alloc] initWithStyle:st];
                [gen prepare];
                [gen impactOccurred];
                MEME_IOS_RELEASE(gen);
            });
            return true;
        };
        h.hapticNotify = [](const std::string &tp) -> bool {
            dispatch_async(dispatch_get_main_queue(), ^{
                UINotificationFeedbackType nt = UINotificationFeedbackTypeSuccess;
                if (tp == "warning") nt = UINotificationFeedbackTypeWarning;
                else if (tp == "error") nt = UINotificationFeedbackTypeError;
                UINotificationFeedbackGenerator *gen = [[UINotificationFeedbackGenerator alloc] init];
                [gen prepare];
                [gen notificationOccurred:nt];
                MEME_IOS_RELEASE(gen);
            });
            return true;
        };
        h.hapticSelection = []() -> bool {
            dispatch_async(dispatch_get_main_queue(), ^{
                UISelectionFeedbackGenerator *gen = [[UISelectionFeedbackGenerator alloc] init];
                [gen prepare];
                [gen selectionChanged];
                MEME_IOS_RELEASE(gen);
            });
            return true;
        };
        h.hapticVibrate = [](int) -> bool {
            void *at = dlopen("/System/Library/Frameworks/AudioToolbox.framework/AudioToolbox", RTLD_LAZY);
            using PlaySysFn = void (*)(uint32_t);
            if (at) {
                if (auto fn = reinterpret_cast<PlaySysFn>(dlsym(at, "AudioServicesPlaySystemSound"))) fn(4095);
            }
            return true;
        };
        h.canOpenUrl = [](const std::string &url) -> bool {
            @autoreleasepool {
                NSString *ns = [NSString stringWithUTF8String:url.c_str()];
                NSURL *u = ns ? [NSURL URLWithString:ns] : nil;
                return u && [[UIApplication sharedApplication] canOpenURL:u];
            }
        };
        h.openUrl = [](const std::string &url) -> bool {
            NSString *ns = [NSString stringWithUTF8String:url.c_str()];
            NSURL *u = ns ? [NSURL URLWithString:ns] : nil;
            if (!u) return false;
            dispatch_async(dispatch_get_main_queue(), ^{
                [[UIApplication sharedApplication] openURL:u options:@{} completionHandler:nil];
            });
            return true;
        };
        h.shareAsync = [](const std::string &text, const std::string &title, const std::string &url, MemeNative::AsyncCallback cb) {
            int rid = MemeNative::registerAsyncCallback(std::move(cb));
            NSMutableArray *items = [NSMutableArray array];
            if (!text.empty()) [items addObject:[NSString stringWithUTF8String:text.c_str()]];
            else if (!title.empty()) [items addObject:[NSString stringWithUTF8String:title.c_str()]];
            if (!url.empty()) {
                NSURL *u = [NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]];
                if (u) [items addObject:u];
            }
            if (items.count == 0) [items addObject:@"Shared via MEME-UI"];
            dispatch_async(dispatch_get_main_queue(), ^{
                UIViewController *vc = memeIosTopViewController();
                if (!vc) {
                    MemeNative::AsyncResponse r; r.ok = false; r.error = "No view controller";
                    MemeNative::resolveAsyncCallback(rid, r);
                    return;
                }
                UIActivityViewController *avc = [[UIActivityViewController alloc] initWithActivityItems:items applicationActivities:nil];
                if (avc.popoverPresentationController) {
                    avc.popoverPresentationController.sourceView = vc.view;
                    avc.popoverPresentationController.sourceRect = CGRectMake(vc.view.bounds.size.width * 0.5, vc.view.bounds.size.height * 0.5, 1, 1);
                }
                avc.completionWithItemsHandler = ^(UIActivityType activityType, BOOL completed, NSArray *returnedItems, NSError *activityError) {
                    (void)activityType; (void)returnedItems;
                    MemeNative::AsyncResponse r;
                    r.ok = completed && !activityError;
                    r.accepted = completed;
                    r.cancelled = !completed;
                    if (activityError) r.error = std::string([activityError.localizedDescription UTF8String]);
                    MemeNative::resolveAsyncCallback(rid, r);
                };
                [vc presentViewController:avc animated:YES completion:nil];
                MEME_IOS_RELEASE(avc);
            });
        };
        h.alertAsync = [](const std::string &title, const std::string &msg, MemeNative::AsyncCallback cb) {
            int rid = MemeNative::registerAsyncCallback(std::move(cb));
            NSString *t = [NSString stringWithUTF8String:title.c_str()];
            NSString *m = [NSString stringWithUTF8String:msg.c_str()];
            dispatch_async(dispatch_get_main_queue(), ^{
                UIViewController *vc = memeIosTopViewController();
                if (!vc) { MemeNative::AsyncResponse r; MemeNative::resolveAsyncCallback(rid, r); return; }
                UIAlertController *ac = [UIAlertController alertControllerWithTitle:(t ?: @"") message:(m ?: @"") preferredStyle:UIAlertControllerStyleAlert];
                [ac addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction *a) {
                    (void)a; MemeNative::AsyncResponse r; r.ok = true; r.accepted = true;
                    MemeNative::resolveAsyncCallback(rid, r);
                }]];
                [vc presentViewController:ac animated:YES completion:nil];
            });
        };
        h.confirmAsync = [](const std::string &title, const std::string &msg, MemeNative::AsyncCallback cb) {
            int rid = MemeNative::registerAsyncCallback(std::move(cb));
            NSString *t = [NSString stringWithUTF8String:title.c_str()];
            NSString *m = [NSString stringWithUTF8String:msg.c_str()];
            dispatch_async(dispatch_get_main_queue(), ^{
                UIViewController *vc = memeIosTopViewController();
                if (!vc) { MemeNative::AsyncResponse r; r.accepted = false; MemeNative::resolveAsyncCallback(rid, r); return; }
                UIAlertController *ac = [UIAlertController alertControllerWithTitle:(t ?: @"") message:(m ?: @"") preferredStyle:UIAlertControllerStyleAlert];
                [ac addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:^(UIAlertAction *a) {
                    (void)a; MemeNative::AsyncResponse r; r.ok = true; r.accepted = false; r.cancelled = true;
                    MemeNative::resolveAsyncCallback(rid, r);
                }]];
                [ac addAction:[UIAlertAction actionWithTitle:@"Confirm" style:UIAlertActionStyleDefault handler:^(UIAlertAction *a) {
                    (void)a; MemeNative::AsyncResponse r; r.ok = true; r.accepted = true; r.cancelled = false;
                    MemeNative::resolveAsyncCallback(rid, r);
                }]];
                [vc presentViewController:ac animated:YES completion:nil];
            });
        };
        h.promptAsync = [](const std::string &title, const std::string &msg, const std::string &defVal, MemeNative::AsyncCallback cb) {
            int rid = MemeNative::registerAsyncCallback(std::move(cb));
            NSString *t = [NSString stringWithUTF8String:title.c_str()];
            NSString *m = [NSString stringWithUTF8String:msg.c_str()];
            NSString *d = [NSString stringWithUTF8String:defVal.c_str()];
            dispatch_async(dispatch_get_main_queue(), ^{
                UIViewController *vc = memeIosTopViewController();
                if (!vc) { MemeNative::AsyncResponse r; r.accepted = false; MemeNative::resolveAsyncCallback(rid, r); return; }
                UIAlertController *ac = [UIAlertController alertControllerWithTitle:(t ?: @"") message:(m ?: @"") preferredStyle:UIAlertControllerStyleAlert];
                [ac addTextFieldWithConfigurationHandler:^(UITextField *tf) { tf.text = d ?: @""; }];
                [ac addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:^(UIAlertAction *a) {
                    (void)a; MemeNative::AsyncResponse r; r.ok = true; r.accepted = false; r.cancelled = true;
                    MemeNative::resolveAsyncCallback(rid, r);
                }]];
                [ac addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction *a) {
                    (void)a;
                    NSString *val = ac.textFields.firstObject.text ?: @"";
                    MemeNative::AsyncResponse r; r.ok = true; r.accepted = true; r.value = [val UTF8String];
                    MemeNative::resolveAsyncCallback(rid, r);
                }]];
                [vc presentViewController:ac animated:YES completion:nil];
            });
        };
        h.fileDialogAsync = [](const std::string &mode, const std::string &title, const std::string &, MemeNative::AsyncCallback cb) {
            (void)title;
            int rid = MemeNative::registerAsyncCallback(std::move(cb));
            dispatch_async(dispatch_get_main_queue(), ^{
                UIViewController *vc = memeIosTopViewController();
                if (!vc) {
                    MemeNative::AsyncResponse r; r.ok = false; r.error = "No view controller";
                    MemeNative::resolveAsyncCallback(rid, r);
                    return;
                }
                if (!g_iosPickerHelper) g_iosPickerHelper = [[MemeIosPickerHelper alloc] init];
                g_iosPickerHelper.reqId = rid;
                if (mode == "photo" || mode == "camera") {
                    UIImagePickerControllerSourceType src = (mode == "camera" && [UIImagePickerController isSourceTypeAvailable:UIImagePickerControllerSourceTypeCamera])
                        ? UIImagePickerControllerSourceTypeCamera
                        : UIImagePickerControllerSourceTypePhotoLibrary;
                    if (![UIImagePickerController isSourceTypeAvailable:src]) {
                        MemeNative::AsyncResponse r; r.ok = false; r.accepted = false; r.error = "Media source unavailable on simulator";
                        g_iosPickerHelper.reqId = 0;
                        MemeNative::resolveAsyncCallback(rid, r);
                        return;
                    }
                    UIImagePickerController *ipc = [[UIImagePickerController alloc] init];
                    ipc.sourceType = src;
                    ipc.delegate = g_iosPickerHelper;
                    [vc presentViewController:ipc animated:YES completion:nil];
                    MEME_IOS_RELEASE(ipc);
                    return;
                }
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
                UIDocumentPickerViewController *dp = [[UIDocumentPickerViewController alloc] initWithDocumentTypes:@[@"public.data", @"public.item"] inMode:UIDocumentPickerModeImport];
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
                dp.delegate = g_iosPickerHelper;
                [vc presentViewController:dp animated:YES completion:nil];
                MEME_IOS_RELEASE(dp);
            });
        };
        h.getDeviceInfo = []() -> MemeNative::DeviceInfo {
            MemeNative::DeviceInfo d;
            @autoreleasepool {
                UIDevice *dev = [UIDevice currentDevice];
                d.os = "ios";
                d.arch = MemeNative::defaultArchName();
                d.model = dev.model ? std::string([dev.model UTF8String]) : "iPhone";
                d.osVersion = dev.systemVersion ? std::string([dev.systemVersion UTF8String]) : "18.0";
                NSString *loc = [[NSLocale currentLocale] localeIdentifier];
                d.locale = loc ? std::string([loc UTF8String]) : "en_US";
                NSString *tz = [[NSTimeZone localTimeZone] name];
                d.timezone = tz ? std::string([tz UTF8String]) : "UTC";
                d.isDark = ([UITraitCollection currentTraitCollection].userInterfaceStyle != UIUserInterfaceStyleLight);
                d.theme = d.isDark ? "dark" : "light";
                d.isMobile = true; d.isDesktop = false; d.isWeb = false;
            }
            return d;
        };
        h.getScreenInfo = []() -> MemeNative::ScreenInfo {
            MemeNative::ScreenInfo s;
            if (auto *sess = meme_host_get_session()) {
                s.width = sess->input.viewportW > 0 ? sess->input.viewportW : static_cast<float>([UIScreen mainScreen].bounds.size.width);
                s.height = sess->input.viewportH > 0 ? sess->input.viewportH : static_cast<float>([UIScreen mainScreen].bounds.size.height);
                s.safeTop = sess->input.safeTop;
                s.safeBottom = sess->input.safeBottom;
                s.safeLeft = sess->input.safeLeft;
                s.safeRight = sess->input.safeRight;
            }
            s.scale = static_cast<float>([UIScreen mainScreen].scale);
            return s;
        };
        h.getBatteryInfo = []() -> MemeNative::BatteryInfo {
            MemeNative::BatteryInfo b;
            @autoreleasepool {
                UIDevice *dev = [UIDevice currentDevice];
                if (!dev.isBatteryMonitoringEnabled) dev.batteryMonitoringEnabled = YES;
                float lvl = dev.batteryLevel;
                b.supported = (lvl >= 0.0f);
                b.level = (lvl >= 0.0f) ? static_cast<double>(std::round(lvl * 100.0f)) : -1.0;
                UIDeviceBatteryState st = dev.batteryState;
                b.charging = (st == UIDeviceBatteryStateCharging || st == UIDeviceBatteryStateFull);
                if (st == UIDeviceBatteryStateFull) b.state = "full";
                else if (st == UIDeviceBatteryStateCharging) b.state = "charging";
                else if (st == UIDeviceBatteryStateUnplugged) b.state = "discharging";
                else b.state = "unknown";
            }
            return b;
        };
        if (!g_iosNotifDelegate) {
            g_iosNotifDelegate = [[MemeIosNotifDelegate alloc] init];
        }
        [UNUserNotificationCenter currentNotificationCenter].delegate = g_iosNotifDelegate;

        h.setBadgeCount = [](int count) -> bool {
            dispatch_async(dispatch_get_main_queue(), ^{
                UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
                if (@available(iOS 16.0, *)) {
                    [center setBadgeCount:static_cast<NSInteger>(std::max(0, count)) withCompletionHandler:nil];
                } else {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
                    [UIApplication sharedApplication].applicationIconBadgeNumber = std::max(0, count);
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
                }
            });
            return true;
        };
        h.notifyRequestPermissionAsync = [](MemeNative::AsyncCallback cb) {
            int rid = MemeNative::registerAsyncCallback(std::move(cb));
            dispatch_async(dispatch_get_main_queue(), ^{
                UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
                if (!g_iosNotifDelegate) g_iosNotifDelegate = [[MemeIosNotifDelegate alloc] init];
                center.delegate = g_iosNotifDelegate;
                [center requestAuthorizationWithOptions:(UNAuthorizationOptionAlert | UNAuthorizationOptionSound | UNAuthorizationOptionBadge)
                                      completionHandler:^(BOOL granted, NSError *err) {
                    (void)err;
                    MemeNative::AsyncResponse r;
                    r.ok = true;
                    r.accepted = (granted != NO);
                    MemeNative::resolveAsyncCallback(rid, r);
                }];
            });
        };
        h.notifySend = [](const std::string &title, const std::string &body) -> bool {
            NSString *nsTitle = [NSString stringWithUTF8String:(title.empty() ? "MEME App" : title.c_str())];
            NSString *nsBody = [NSString stringWithUTF8String:body.c_str()];
            if (!nsTitle) nsTitle = @"MEME App";
            if (!nsBody) nsBody = @"";
            dispatch_async(dispatch_get_main_queue(), ^{
                UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
                if (!g_iosNotifDelegate) g_iosNotifDelegate = [[MemeIosNotifDelegate alloc] init];
                center.delegate = g_iosNotifDelegate;

                void (^scheduleNotification)(void) = ^{
                    UNMutableNotificationContent *content = [[UNMutableNotificationContent alloc] init];
                    content.title = nsTitle;
                    content.body = nsBody;
                    content.sound = [UNNotificationSound defaultSound];
                    UNTimeIntervalNotificationTrigger *trigger = [UNTimeIntervalNotificationTrigger triggerWithTimeInterval:0.15 repeats:NO];
                    NSString *reqId = [[NSUUID UUID] UUIDString];
                    UNNotificationRequest *req = [UNNotificationRequest requestWithIdentifier:reqId content:content trigger:trigger];
                    [center addNotificationRequest:req withCompletionHandler:^(NSError *err) {
                        if (err) NSLog(@"[MEME-UI] Notification error: %@", err.localizedDescription);
                    }];
                    MEME_IOS_RELEASE(content);
                };

                [center getNotificationSettingsWithCompletionHandler:^(UNNotificationSettings *settings) {
                    if (settings.authorizationStatus == UNAuthorizationStatusAuthorized ||
                        settings.authorizationStatus == UNAuthorizationStatusProvisional ||
                        settings.authorizationStatus == UNAuthorizationStatusEphemeral) {
                        scheduleNotification();
                    } else if (settings.authorizationStatus == UNAuthorizationStatusNotDetermined) {
                        [center requestAuthorizationWithOptions:(UNAuthorizationOptionAlert | UNAuthorizationOptionSound | UNAuthorizationOptionBadge)
                                              completionHandler:^(BOOL granted, NSError *err) {
                            (void)err;
                            if (granted) scheduleNotification();
                        }];
                    } else {
                        NSLog(@"[MEME-UI] Notifications denied in iOS Settings (status=%ld)", (long)settings.authorizationStatus);
                    }
                }];
            });
            return true;
        };
        h.biometricsAvailable = []() -> bool {
            void *la = dlopen("/System/Library/Frameworks/LocalAuthentication.framework/LocalAuthentication", RTLD_LAZY);
            Class laCls = la ? NSClassFromString(@"LAContext") : nil;
            if (!laCls) return false;
            id ctx = [[laCls alloc] init];
            NSError *err = nil;
            BOOL ok = ((BOOL (*)(id, SEL, NSInteger, NSError **))objc_msgSend)(ctx, NSSelectorFromString(@"canEvaluatePolicy:error:"), 2 /*LAPolicyDeviceOwnerAuthentication*/, &err);
            MEME_IOS_RELEASE(ctx);
            return ok != NO;
        };
        h.biometricsAuthAsync = [](const std::string &reason, MemeNative::AsyncCallback cb) {
            int rid = MemeNative::registerAsyncCallback(std::move(cb));
            void *la = dlopen("/System/Library/Frameworks/LocalAuthentication.framework/LocalAuthentication", RTLD_LAZY);
            Class laCls = la ? NSClassFromString(@"LAContext") : nil;
            if (!laCls) {
                MemeNative::AsyncResponse r; r.ok = false; r.accepted = false; r.error = "LocalAuthentication unavailable";
                MemeNative::resolveAsyncCallback(rid, r);
                return;
            }
            id ctx = [[laCls alloc] init];
            NSString *nsR = [NSString stringWithUTF8String:(reason.empty() ? "Authenticate" : reason.c_str())];
            void (^reply)(BOOL, NSError *) = ^(BOOL ok, NSError *err) {
                MemeNative::AsyncResponse r;
                r.ok = (ok != NO);
                r.accepted = r.ok;
                r.cancelled = !r.ok;
                if (err && err.localizedDescription) r.error = [err.localizedDescription UTF8String];
                MemeNative::resolveAsyncCallback(rid, r);
            };
            ((void (*)(id, SEL, NSInteger, id, id))objc_msgSend)(ctx, NSSelectorFromString(@"evaluatePolicy:localizedReason:reply:"), 2, nsR, reply);
            MEME_IOS_RELEASE(ctx);
        };
        h.getLocationAsync = [](MemeNative::AsyncCallback cb) {
            int rid = MemeNative::registerAsyncCallback(std::move(cb));
            void *cl = dlopen("/System/Library/Frameworks/CoreLocation.framework/CoreLocation", RTLD_LAZY);
            Class clCls = cl ? NSClassFromString(@"CLLocationManager") : nil;
            if (!clCls) {
                MemeNative::AsyncResponse r; r.ok = false; r.error = "CoreLocation unavailable";
                MemeNative::resolveAsyncCallback(rid, r);
                return;
            }
            dispatch_async(dispatch_get_main_queue(), ^{
                if (!g_iosLocHelper) {
                    g_iosLocHelper = [[MemeIosLocHelper alloc] init];
                    g_iosLocHelper.manager = [[clCls alloc] init];
                    [g_iosLocHelper.manager setValue:g_iosLocHelper forKey:@"delegate"];
                }
                g_iosLocHelper.reqId = rid;
                SEL authSel = NSSelectorFromString(@"requestWhenInUseAuthorization");
                if ([g_iosLocHelper.manager respondsToSelector:authSel]) {
                    ((void (*)(id, SEL))objc_msgSend)(g_iosLocHelper.manager, authSel);
                }
                SEL startSel = NSSelectorFromString(@"startUpdatingLocation");
                ((void (*)(id, SEL))objc_msgSend)(g_iosLocHelper.manager, startSel);
            });
        };
        h.getSensorSnapshot = []() -> MemeNative::SensorSnapshot {
            MemeNative::SensorSnapshot ss;
            void *cm = dlopen("/System/Library/Frameworks/CoreMotion.framework/CoreMotion", RTLD_LAZY);
            Class cmCls = cm ? NSClassFromString(@"CMMotionManager") : nil;
            if (!cmCls) return ss;
            if (!g_iosMotionMgr) {
                g_iosMotionMgr = [[cmCls alloc] init];
                SEL sa = NSSelectorFromString(@"startAccelerometerUpdates");
                SEL sg = NSSelectorFromString(@"startGyroUpdates");
                if ([g_iosMotionMgr respondsToSelector:sa]) ((void (*)(id, SEL))objc_msgSend)(g_iosMotionMgr, sa);
                if ([g_iosMotionMgr respondsToSelector:sg]) ((void (*)(id, SEL))objc_msgSend)(g_iosMotionMgr, sg);
            }
            id accData = [g_iosMotionMgr valueForKey:@"accelerometerData"];
            id gyroData = [g_iosMotionMgr valueForKey:@"gyroData"];
            if (accData) {
                ss.available = true;
                struct { double x, y, z; } a{0, 0, 0};
                SEL aSel = NSSelectorFromString(@"acceleration");
                NSMethodSignature *sig = [accData methodSignatureForSelector:aSel];
                if (sig && [sig methodReturnLength] == sizeof(a)) {
                    NSInvocation *inv = [NSInvocation invocationWithMethodSignature:sig];
                    [inv setTarget:accData]; [inv setSelector:aSel]; [inv invoke];
                    [inv getReturnValue:&a];
                    ss.accelX = a.x; ss.accelY = a.y; ss.accelZ = a.z;
                }
            }
            if (gyroData) {
                ss.available = true;
                struct { double x, y, z; } g{0, 0, 0};
                SEL gSel = NSSelectorFromString(@"rotationRate");
                NSMethodSignature *sig = [gyroData methodSignatureForSelector:gSel];
                if (sig && [sig methodReturnLength] == sizeof(g)) {
                    NSInvocation *inv = [NSInvocation invocationWithMethodSignature:sig];
                    [inv setTarget:gyroData]; [inv setSelector:gSel]; [inv invoke];
                    [inv getReturnValue:&g];
                    ss.gyroX = g.x; ss.gyroY = g.y; ss.gyroZ = g.z;
                }
            }
            return ss;
        };
        h.beep = [](double, int) -> bool {
            void *at = dlopen("/System/Library/Frameworks/AudioToolbox.framework/AudioToolbox", RTLD_LAZY);
            using PlaySysFn = void (*)(uint32_t);
            if (at) {
                if (auto fn = reinterpret_cast<PlaySysFn>(dlsym(at, "AudioServicesPlaySystemSound"))) {
                    fn(1057);
                    return true;
                }
            }
            return false;
        };
        h.speak = [](const std::string &text, double rate) -> bool {
            void *av = dlopen("/System/Library/Frameworks/AVFoundation.framework/AVFoundation", RTLD_LAZY);
            Class sCls = av ? NSClassFromString(@"AVSpeechSynthesizer") : nil;
            Class uCls = av ? NSClassFromString(@"AVSpeechUtterance") : nil;
            if (!sCls || !uCls) return false;
            if (!g_iosSpeechSynth) g_iosSpeechSynth = [[sCls alloc] init];
            NSString *ns = [NSString stringWithUTF8String:text.c_str()];
            if (!ns) return false;
            id utt = ((id (*)(Class, SEL, id))objc_msgSend)(uCls, NSSelectorFromString(@"speechUtteranceWithString:"), ns);
            if (rate > 0.0) [utt setValue:@(static_cast<float>(std::clamp(rate * 0.5, 0.1, 1.0))) forKey:@"rate"];
            ((void (*)(id, SEL, id))objc_msgSend)(g_iosSpeechSynth, NSSelectorFromString(@"speakUtterance:"), utt);
            return true;
        };
        h.stopSpeaking = []() -> bool {
            if (!g_iosSpeechSynth) return false;
            return ((BOOL (*)(id, SEL, NSInteger))objc_msgSend)(g_iosSpeechSynth, NSSelectorFromString(@"stopSpeakingAtBoundary:"), 0) != NO;
        };
        h.hideKeyboard = []() -> bool {
            dispatch_async(dispatch_get_main_queue(), ^{
                if (g_iosRenderView) [g_iosRenderView resignFirstResponder];
                [[UIApplication sharedApplication] sendAction:@selector(resignFirstResponder) to:nil from:nil forEvent:nil];
            });
            return true;
        };
        static std::atomic<int> s_iosNotifAuthCache{-1};
        [[UNUserNotificationCenter currentNotificationCenter] getNotificationSettingsWithCompletionHandler:^(UNNotificationSettings *settings) {
            s_iosNotifAuthCache.store(static_cast<int>(settings.authorizationStatus));
        }];
        auto refreshNotifCache = []() {
            [[UNUserNotificationCenter currentNotificationCenter] getNotificationSettingsWithCompletionHandler:^(UNNotificationSettings *settings) {
                s_iosNotifAuthCache.store(static_cast<int>(settings.authorizationStatus));
            }];
        };
        auto mapAppleAuth = [](NSInteger st) -> std::string {
            if (st == 3) return "granted";
            if (st == 1) return "restricted";
            if (st == 2) return "denied";
            return "prompt";
        };
        h.checkPermission = [mapAppleAuth, refreshNotifCache](const std::string &rawCap) -> MemePermissions::PermissionStatus {
            std::string cap = MemePermissions::normalizeCapability(rawCap);
            MemePermissions::PermissionStatus out;
            out.permission = cap;
            out.platform = "ios";
            if (cap == "notifications") {
                int cached = s_iosNotifAuthCache.load();
                refreshNotifCache();
                if (cached == static_cast<int>(UNAuthorizationStatusAuthorized) ||
                    cached == static_cast<int>(UNAuthorizationStatusEphemeral)) out.state = "granted";
                else if (cached == static_cast<int>(UNAuthorizationStatusProvisional)) out.state = "limited";
                else if (cached == static_cast<int>(UNAuthorizationStatusDenied)) out.state = "denied";
                else out.state = "prompt";
            } else if (cap == "camera" || cap == "microphone") {
                void *av = dlopen("/System/Library/Frameworks/AVFoundation.framework/AVFoundation", RTLD_LAZY);
                Class cls = av ? NSClassFromString(@"AVCaptureDevice") : nil;
                if (cls) {
                    NSString *mt = (cap == "camera") ? @"vide" : @"soun";
                    NSInteger st = ((NSInteger (*)(Class, SEL, id))objc_msgSend)(cls, NSSelectorFromString(@"authorizationStatusForMediaType:"), mt);
                    out.state = mapAppleAuth(st);
                } else {
                    out.state = "unavailable";
                }
            } else if (cap == "photos") {
                void *ph = dlopen("/System/Library/Frameworks/Photos.framework/Photos", RTLD_LAZY);
                Class cls = ph ? NSClassFromString(@"PHPhotoLibrary") : nil;
                if (cls) {
                    SEL sel2 = NSSelectorFromString(@"authorizationStatusForAccessLevel:");
                    NSInteger st = [cls respondsToSelector:sel2]
                        ? ((NSInteger (*)(Class, SEL, NSInteger))objc_msgSend)(cls, sel2, 1 /*readWrite*/)
                        : ((NSInteger (*)(Class, SEL))objc_msgSend)(cls, NSSelectorFromString(@"authorizationStatus"));
                    if (st == 4) out.state = "limited";
                    else out.state = mapAppleAuth(st);
                } else {
                    out.state = "unavailable";
                }
            } else if (cap == "location" || cap == "locationAlways") {
                void *cl = dlopen("/System/Library/Frameworks/CoreLocation.framework/CoreLocation", RTLD_LAZY);
                Class cls = cl ? NSClassFromString(@"CLLocationManager") : nil;
                if (cls) {
                    int st = 0;
                    if (!g_iosLocHelper) {
                        g_iosLocHelper = [[MemeIosLocHelper alloc] init];
                        g_iosLocHelper.manager = [[cls alloc] init];
                        [g_iosLocHelper.manager setValue:g_iosLocHelper forKey:@"delegate"];
                    }
                    if ([g_iosLocHelper.manager respondsToSelector:NSSelectorFromString(@"authorizationStatus")]) {
                        st = ((int (*)(id, SEL))objc_msgSend)(g_iosLocHelper.manager, NSSelectorFromString(@"authorizationStatus"));
                    } else if ([cls respondsToSelector:NSSelectorFromString(@"authorizationStatus")]) {
                        st = ((int (*)(Class, SEL))objc_msgSend)(cls, NSSelectorFromString(@"authorizationStatus"));
                    }
                    if (cap == "locationAlways") {
                        out.state = (st == 3) ? "granted" : (st == 4 ? "limited" : (st == 1 ? "restricted" : (st == 2 ? "denied" : "prompt")));
                    } else {
                        out.state = iosMapCLStatus(st);
                    }
                } else {
                    out.state = "unavailable";
                }
            } else if (cap == "contacts") {
                void *cn = dlopen("/System/Library/Frameworks/Contacts.framework/Contacts", RTLD_LAZY);
                Class cls = cn ? NSClassFromString(@"CNContactStore") : nil;
                if (cls) {
                    NSInteger st = ((NSInteger (*)(Class, SEL, NSInteger))objc_msgSend)(cls, NSSelectorFromString(@"authorizationStatusForEntityType:"), 0);
                    out.state = mapAppleAuth(st);
                } else out.state = "unavailable";
            } else if (cap == "calendar") {
                void *ek = dlopen("/System/Library/Frameworks/EventKit.framework/EventKit", RTLD_LAZY);
                Class cls = ek ? NSClassFromString(@"EKEventStore") : nil;
                if (cls) {
                    NSInteger st = ((NSInteger (*)(Class, SEL, NSInteger))objc_msgSend)(cls, NSSelectorFromString(@"authorizationStatusForEntityType:"), 0);
                    out.state = (st == 3 || st == 4) ? "granted" : (st == 1 ? "restricted" : (st == 2 ? "denied" : "prompt"));
                } else out.state = "unavailable";
            } else if (cap == "sensors") {
                void *cm = dlopen("/System/Library/Frameworks/CoreMotion.framework/CoreMotion", RTLD_LAZY);
                Class cls = cm ? NSClassFromString(@"CMMotionActivityManager") : nil;
                if (cls && [cls respondsToSelector:NSSelectorFromString(@"authorizationStatus")]) {
                    NSInteger st = ((NSInteger (*)(Class, SEL))objc_msgSend)(cls, NSSelectorFromString(@"authorizationStatus"));
                    out.state = mapAppleAuth(st);
                } else {
                    out.state = "granted";
                }
            } else if (cap == "bluetooth") {
                void *cb = dlopen("/System/Library/Frameworks/CoreBluetooth.framework/CoreBluetooth", RTLD_LAZY);
                Class cls = cb ? NSClassFromString(@"CBManager") : nil;
                if (cls && [cls respondsToSelector:NSSelectorFromString(@"authorization")]) {
                    NSInteger st = ((NSInteger (*)(Class, SEL))objc_msgSend)(cls, NSSelectorFromString(@"authorization"));
                    out.state = mapAppleAuth(st);
                } else out.state = "unavailable";
            } else if (cap == "biometrics") {
                void *la = dlopen("/System/Library/Frameworks/LocalAuthentication.framework/LocalAuthentication", RTLD_LAZY);
                Class laCls = la ? NSClassFromString(@"LAContext") : nil;
                if (!laCls) out.state = "unavailable";
                else {
                    id ctx = [[laCls alloc] init];
                    NSError *err = nil;
                    BOOL ok = ((BOOL (*)(id, SEL, NSInteger, NSError **))objc_msgSend)(ctx, NSSelectorFromString(@"canEvaluatePolicy:error:"), 2, &err);
                    MEME_IOS_RELEASE(ctx);
                    out.state = ok ? "granted" : "unavailable";
                }
            } else if (cap == "clipboard" || cap == "storage") {
                out.state = "granted";
            } else {
                out.state = "unavailable";
            }
            out.finalizeFlags();
            return out;
        };
        h.requestPermissionAsync = [mapAppleAuth](const std::string &rawCap, MemeNative::AsyncCallback cb) {
            std::string cap = MemePermissions::normalizeCapability(rawCap);
            int rid = MemeNative::registerAsyncCallback(std::move(cb));
            auto finishState = [rid](const std::string &stStr, const std::string &err = "") {
                MemeNative::AsyncResponse r;
                r.permissionState = stStr;
                r.ok = (stStr == "granted" || stStr == "limited");
                r.accepted = r.ok;
                r.limited = (stStr == "limited");
                r.restricted = (stStr == "restricted");
                r.canAskAgain = (stStr == "prompt" || stStr == "prompt-with-rationale");
                r.value = stStr;
                r.error = err;
                MemeNative::resolveAsyncCallback(rid, r);
            };
            dispatch_async(dispatch_get_main_queue(), ^{
                if (cap == "notifications") {
                    UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
                    if (!g_iosNotifDelegate) g_iosNotifDelegate = [[MemeIosNotifDelegate alloc] init];
                    center.delegate = g_iosNotifDelegate;
                    [center requestAuthorizationWithOptions:(UNAuthorizationOptionAlert | UNAuthorizationOptionSound | UNAuthorizationOptionBadge)
                                          completionHandler:^(BOOL granted, NSError *err) {
                        s_iosNotifAuthCache.store(granted ? static_cast<int>(UNAuthorizationStatusAuthorized) : static_cast<int>(UNAuthorizationStatusDenied));
                        finishState(granted ? "granted" : "denied", err && err.localizedDescription ? [err.localizedDescription UTF8String] : "");
                    }];
                    return;
                }
                if (cap == "camera" || cap == "microphone") {
                    void *av = dlopen("/System/Library/Frameworks/AVFoundation.framework/AVFoundation", RTLD_LAZY);
                    Class cls = av ? NSClassFromString(@"AVCaptureDevice") : nil;
                    if (!cls) { finishState("unavailable", "AVFoundation unavailable"); return; }
                    NSString *mt = (cap == "camera") ? @"vide" : @"soun";
                    NSInteger cur = ((NSInteger (*)(Class, SEL, id))objc_msgSend)(cls, NSSelectorFromString(@"authorizationStatusForMediaType:"), mt);
                    if (cur != 0) { finishState(mapAppleAuth(cur)); return; }
                    void (^handler)(BOOL) = ^(BOOL granted) { finishState(granted ? "granted" : "denied"); };
                    ((void (*)(Class, SEL, id, id))objc_msgSend)(cls, NSSelectorFromString(@"requestAccessForMediaType:completionHandler:"), mt, handler);
                    return;
                }
                if (cap == "photos") {
                    void *ph = dlopen("/System/Library/Frameworks/Photos.framework/Photos", RTLD_LAZY);
                    Class cls = ph ? NSClassFromString(@"PHPhotoLibrary") : nil;
                    if (!cls) { finishState("unavailable", "Photos framework unavailable"); return; }
                    SEL req2 = NSSelectorFromString(@"requestAuthorizationForAccessLevel:handler:");
                    void (^handler)(NSInteger) = ^(NSInteger st) {
                        finishState(st == 4 ? "limited" : mapAppleAuth(st));
                    };
                    if ([cls respondsToSelector:req2]) {
                        ((void (*)(Class, SEL, NSInteger, id))objc_msgSend)(cls, req2, 1, handler);
                    } else {
                        ((void (*)(Class, SEL, id))objc_msgSend)(cls, NSSelectorFromString(@"requestAuthorization:"), handler);
                    }
                    return;
                }
                if (cap == "location" || cap == "locationAlways") {
                    void *cl = dlopen("/System/Library/Frameworks/CoreLocation.framework/CoreLocation", RTLD_LAZY);
                    Class cls = cl ? NSClassFromString(@"CLLocationManager") : nil;
                    if (!cls) { finishState("unavailable", "CoreLocation unavailable"); return; }
                    if (!g_iosLocHelper) {
                        g_iosLocHelper = [[MemeIosLocHelper alloc] init];
                        g_iosLocHelper.manager = [[cls alloc] init];
                        [g_iosLocHelper.manager setValue:g_iosLocHelper forKey:@"delegate"];
                    }
                    int cur = 0;
                    if ([g_iosLocHelper.manager respondsToSelector:NSSelectorFromString(@"authorizationStatus")]) {
                        cur = ((int (*)(id, SEL))objc_msgSend)(g_iosLocHelper.manager, NSSelectorFromString(@"authorizationStatus"));
                    }
                    if ((cap == "location" && cur != 0) || (cap == "locationAlways" && cur != 0 && cur != 4)) {
                        finishState(iosMapCLStatus(cur));
                        return;
                    }
                    g_iosLocHelper.permReqId = rid;
                    SEL askSel = NSSelectorFromString(cap == "locationAlways" ? @"requestAlwaysAuthorization" : @"requestWhenInUseAuthorization");
                    if ([g_iosLocHelper.manager respondsToSelector:askSel]) {
                        ((void (*)(id, SEL))objc_msgSend)(g_iosLocHelper.manager, askSel);
                    } else {
                        g_iosLocHelper.permReqId = 0;
                        finishState("unavailable", "Location authorization selector missing");
                    }
                    return;
                }
                if (cap == "contacts") {
                    void *cn = dlopen("/System/Library/Frameworks/Contacts.framework/Contacts", RTLD_LAZY);
                    Class cls = cn ? NSClassFromString(@"CNContactStore") : nil;
                    if (!cls) { finishState("unavailable"); return; }
                    id store = [[cls alloc] init];
                    void (^handler)(BOOL, NSError *) = ^(BOOL granted, NSError *err) {
                        finishState(granted ? "granted" : "denied", err && err.localizedDescription ? [err.localizedDescription UTF8String] : "");
                    };
                    ((void (*)(id, SEL, NSInteger, id))objc_msgSend)(store, NSSelectorFromString(@"requestAccessForEntityType:completionHandler:"), 0, handler);
                    MEME_IOS_RELEASE(store);
                    return;
                }
                if (cap == "calendar") {
                    void *ek = dlopen("/System/Library/Frameworks/EventKit.framework/EventKit", RTLD_LAZY);
                    Class cls = ek ? NSClassFromString(@"EKEventStore") : nil;
                    if (!cls) { finishState("unavailable"); return; }
                    id store = [[cls alloc] init];
                    void (^handler)(BOOL, NSError *) = ^(BOOL granted, NSError *err) {
                        finishState(granted ? "granted" : "denied", err && err.localizedDescription ? [err.localizedDescription UTF8String] : "");
                    };
                    SEL fullSel = NSSelectorFromString(@"requestFullAccessToEventsWithCompletion:");
                    if ([store respondsToSelector:fullSel]) {
                        ((void (*)(id, SEL, id))objc_msgSend)(store, fullSel, handler);
                    } else {
                        ((void (*)(id, SEL, NSInteger, id))objc_msgSend)(store, NSSelectorFromString(@"requestAccessToEntityType:completion:"), 0, handler);
                    }
                    MEME_IOS_RELEASE(store);
                    return;
                }
                if (cap == "clipboard" || cap == "storage" || cap == "sensors") { finishState("granted"); return; }
                finishState("unavailable");
            });
        };
        h.openAppSettings = [](const std::string &) -> bool {
            dispatch_async(dispatch_get_main_queue(), ^{
                NSURL *u = [NSURL URLWithString:UIApplicationOpenSettingsURLString];
                if (u) [[UIApplication sharedApplication] openURL:u options:@{} completionHandler:nil];
            });
            return true;
        };
    });
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
    bindIosNativeHooks();
    g_iosRenderView = rv;
    g_iosRootVc = self;
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
        bindIosNativeHooks();
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
