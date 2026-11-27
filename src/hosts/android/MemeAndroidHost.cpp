#if defined(__ANDROID__)

#include <android/native_activity.h>
#include <android/native_window.h>
#include <android/input.h>
#include <android/keycodes.h>
#include <android/looper.h>
#include <android/configuration.h>
#include <android/sensor.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <jni.h>
#include <pthread.h>
#include <unistd.h>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <string>
#include <vector>

#include "../../ui/MemeUiTypes.h"
#include "../../ui/MemeUiTessellator.h"
#include "../../ui/MemeUiRuntime.h"
#include "../../platform/MemeNativeApis.h"

extern "C" int meme_user_entrypoint(int argc, char *argv[]);
extern "C" MemeUI::UiSession *meme_host_get_session();
extern "C" void meme_host_rerender();

namespace {

const char *kAndroidVertSrc = R"(
attribute vec2 aPos;
attribute vec2 aUv;
attribute vec4 aCol;
uniform vec2 uViewport;
varying vec2 vUv;
varying vec4 vCol;
void main() {
    float nx = (aPos.x / max(1.0, uViewport.x)) * 2.0 - 1.0;
    float ny = 1.0 - (aPos.y / max(1.0, uViewport.y)) * 2.0;
    gl_Position = vec4(nx, ny, 0.0, 1.0);
    vUv = aUv;
    vCol = aCol;
}
)";

const char *kAndroidFragSrc = R"(
precision mediump float;
uniform sampler2D uAtlas;
varying vec2 vUv;
varying vec4 vCol;
void main() {
    vec4 tex = texture2D(uAtlas, vUv);
    gl_FragColor = vec4(vCol.rgb * tex.rgb, vCol.a * tex.a);
}
)";

struct AndroidAppState {
    ANativeActivity *activity = nullptr;
    ANativeWindow *window = nullptr;
    ANativeWindow *pendingWindow = nullptr;
    AInputQueue *inputQueue = nullptr;
    AInputQueue *pendingInputQueue = nullptr;
    pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
    pthread_t thread{};
    std::atomic<bool> running{false};
    std::atomic<bool> initialized{false};
    float densityScale = 2.5f;

    EGLDisplay display = EGL_NO_DISPLAY;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;
    EGLint width = 0;
    EGLint height = 0;

    GLuint program = 0;
    GLuint atlasTex = 0;
    GLuint vbo = 0;
    GLuint ibo = 0;
    GLint locPos = -1, locUv = -1, locCol = -1, locVp = -1, locAtlas = -1;

    bool touchActive = false;
    bool touchScrolled = false;
    float touchStartX = 0.0f;
    float touchStartY = 0.0f;
    float lastTouchX = 0.0f;
    float lastTouchY = 0.0f;
    float scrollVelX = 0.0f;
    float scrollVelY = 0.0f;
};

GLuint compileShader(GLenum type, const char *src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    return s;
}

void initGles(AndroidAppState *st) {
    st->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(st->display, nullptr, nullptr);
    const EGLint attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8, EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };
    EGLConfig cfg;
    EGLint numCfg = 0;
    eglChooseConfig(st->display, attribs, &cfg, 1, &numCfg);
    st->surface = eglCreateWindowSurface(st->display, cfg, st->window, nullptr);
    const EGLint ctxAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    st->context = eglCreateContext(st->display, cfg, EGL_NO_CONTEXT, ctxAttribs);
    eglMakeCurrent(st->display, st->surface, st->surface, st->context);
    eglQuerySurface(st->display, st->surface, EGL_WIDTH, &st->width);
    eglQuerySurface(st->display, st->surface, EGL_HEIGHT, &st->height);

    GLuint vs = compileShader(GL_VERTEX_SHADER, kAndroidVertSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, kAndroidFragSrc);
    st->program = glCreateProgram();
    glAttachShader(st->program, vs);
    glAttachShader(st->program, fs);
    glLinkProgram(st->program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    st->locPos = glGetAttribLocation(st->program, "aPos");
    st->locUv = glGetAttribLocation(st->program, "aUv");
    st->locCol = glGetAttribLocation(st->program, "aCol");
    st->locVp = glGetUniformLocation(st->program, "uViewport");
    st->locAtlas = glGetUniformLocation(st->program, "uAtlas");

    glGenTextures(1, &st->atlasTex);
    glBindTexture(GL_TEXTURE_2D, st->atlasTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    auto rgba = MemeUI::FontAtlas::getRgbaAtlas();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MemeUI::FontAtlas::ATLAS_W, MemeUI::FontAtlas::ATLAS_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

    glGenBuffers(1, &st->vbo);
    glGenBuffers(1, &st->ibo);
}

void termGles(AndroidAppState *st) {
    if (st->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(st->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (st->context != EGL_NO_CONTEXT) eglDestroyContext(st->display, st->context);
        if (st->surface != EGL_NO_SURFACE) eglDestroySurface(st->display, st->surface);
        eglTerminate(st->display);
    }
    st->display = EGL_NO_DISPLAY;
    st->context = EGL_NO_CONTEXT;
    st->surface = EGL_NO_SURFACE;
}

void drawGlesFrame(AndroidAppState *st, MemeUI::UiSession *sess) {
    if (st->display == EGL_NO_DISPLAY || !sess) return;
    eglQuerySurface(st->display, st->surface, EGL_WIDTH, &st->width);
    eglQuerySurface(st->display, st->surface, EGL_HEIGHT, &st->height);
    float scale = std::max(1.0f, st->densityScale);
    float vw = static_cast<float>(st->width) / scale;
    float vh = static_cast<float>(st->height) / scale;
    if (vw <= 1.0f || vh <= 1.0f) return;

    sess->input.dpiScale = scale;
    sess->input.safeTop = 24.0f;
    sess->input.safeBottom = 16.0f;
    if (sess->needsRender || sess->lastMesh.vertices.empty()) {
        sess->renderFrame(vw, vh);
    }

    glViewport(0, 0, st->width, st->height);
    glClearColor(0.008f, 0.024f, 0.09f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_SCISSOR_TEST);

    const auto &mesh = sess->lastMesh;
    if (!mesh.vertices.empty() && !mesh.indices.empty()) {
        glUseProgram(st->program);
        glUniform2f(st->locVp, vw, vh);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, st->atlasTex);
        glUniform1i(st->locAtlas, 0);

        glBindBuffer(GL_ARRAY_BUFFER, st->vbo);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(MemeUI::UiVertex)), mesh.vertices.data(), GL_STREAM_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, st->ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(uint32_t)), mesh.indices.data(), GL_STREAM_DRAW);

        constexpr GLsizei stride = sizeof(MemeUI::UiVertex);
        glEnableVertexAttribArray(static_cast<GLuint>(st->locPos));
        glVertexAttribPointer(static_cast<GLuint>(st->locPos), 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void *>(0));
        glEnableVertexAttribArray(static_cast<GLuint>(st->locUv));
        glVertexAttribPointer(static_cast<GLuint>(st->locUv), 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void *>(8));
        glEnableVertexAttribArray(static_cast<GLuint>(st->locCol));
        glVertexAttribPointer(static_cast<GLuint>(st->locCol), 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, reinterpret_cast<const void *>(16));

        for (const auto &batch : mesh.batches) {
            if (batch.indexCount == 0) continue;
            GLint sx = static_cast<GLint>(std::clamp(batch.clipRect.x * scale, 0.0f, static_cast<float>(st->width)));
            GLint ex = static_cast<GLint>(std::clamp((batch.clipRect.x + batch.clipRect.width) * scale, 0.0f, static_cast<float>(st->width)));
            GLint syTop = static_cast<GLint>(std::clamp(batch.clipRect.y * scale, 0.0f, static_cast<float>(st->height)));
            GLint eyBot = static_cast<GLint>(std::clamp((batch.clipRect.y + batch.clipRect.height) * scale, 0.0f, static_cast<float>(st->height)));
            if (ex <= sx || eyBot <= syTop) continue;
            glScissor(sx, st->height - eyBot, ex - sx, eyBot - syTop);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(batch.indexCount), GL_UNSIGNED_INT,
                           reinterpret_cast<const void *>(static_cast<uintptr_t>(batch.indexOffset * sizeof(uint32_t))));
        }
    }
    glDisable(GL_SCISSOR_TEST);
    eglSwapBuffers(st->display, st->surface);
}

void processAndroidInput(AndroidAppState *st, AInputEvent *ev, MemeUI::UiSession *sess) {
    if (!sess) return;
    int type = AInputEvent_getType(ev);
    float scale = std::max(1.0f, st->densityScale);
    if (type == AINPUT_EVENT_TYPE_MOTION) {
        int action = AMotionEvent_getAction(ev) & AMOTION_EVENT_ACTION_MASK;
        float px = AMotionEvent_getX(ev, 0) / scale;
        float py = AMotionEvent_getY(ev, 0) / scale;
        if (action == AMOTION_EVENT_ACTION_DOWN) {
            st->touchActive = true;
            st->touchScrolled = false;
            st->touchStartX = st->lastTouchX = px;
            st->touchStartY = st->lastTouchY = py;
            st->scrollVelX = 0.0f;
            st->scrollVelY = 0.0f;
            sess->handlePointerDown(px, py, 0, 1);
            meme_host_rerender();
        } else if (action == AMOTION_EVENT_ACTION_MOVE) {
            float dx = px - st->lastTouchX;
            float dy = py - st->lastTouchY;
            st->lastTouchX = px;
            st->lastTouchY = py;
            float totalDist = std::hypot(px - st->touchStartX, py - st->touchStartY);
            bool scrolled = false;
            if (totalDist > 6.0f && !sess->interaction.draggingSlider) {
                st->touchScrolled = true;
                sess->interaction.activeId = 0;
                sess->interaction.activeKey.clear();
                for (auto &kv : sess->interaction.textFields) kv.second.dragging = false;
                if (dx != 0.0f || dy != 0.0f) {
                    scrolled = sess->handleScroll(px, py, dx / 28.0f, dy / 28.0f);
                    if (scrolled) {
                        st->scrollVelX = st->scrollVelX * 0.55f + dx * 0.45f;
                        st->scrollVelY = st->scrollVelY * 0.55f + dy * 0.45f;
                    }
                }
            }
            bool moved = !st->touchScrolled && sess->handlePointerMove(px, py);
            if (scrolled || moved) meme_host_rerender();
        } else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
            st->touchActive = false;
            if (st->touchScrolled) {
                sess->handlePointerUp(-9999.0f, -9999.0f, 0);
                if (action == AMOTION_EVENT_ACTION_CANCEL || std::hypot(st->scrollVelX, st->scrollVelY) <= 1.2f) {
                    st->scrollVelX = 0.0f;
                    st->scrollVelY = 0.0f;
                }
            } else {
                st->scrollVelX = 0.0f;
                st->scrollVelY = 0.0f;
                sess->handlePointerUp(px, py, 0);
            }
            st->touchScrolled = false;
            meme_host_rerender();
        }
    } else if (type == AINPUT_EVENT_TYPE_KEY) {
        if (AKeyEvent_getAction(ev) == AKEY_EVENT_ACTION_DOWN) {
            int kc = AKeyEvent_getKeyCode(ev);
            int meta = AKeyEvent_getMetaState(ev);
            MemeUI::UiKeyEvent kev;
            kev.shift = (meta & AMETA_SHIFT_ON) != 0;
            kev.ctrl = (meta & AMETA_CTRL_ON) != 0;
            kev.alt = (meta & AMETA_ALT_ON) != 0;
            if (kc == AKEYCODE_DEL) kev.key = "Backspace";
            else if (kc == AKEYCODE_FORWARD_DEL) kev.key = "Delete";
            else if (kc == AKEYCODE_ENTER) kev.key = "Enter";
            else if (kc == AKEYCODE_TAB) kev.key = "Tab";
            else if (kc == AKEYCODE_ESCAPE || kc == AKEYCODE_BACK) kev.key = "Escape";
            else if (kc == AKEYCODE_DPAD_LEFT) kev.key = "ArrowLeft";
            else if (kc == AKEYCODE_DPAD_RIGHT) kev.key = "ArrowRight";
            else if (kc == AKEYCODE_DPAD_UP) kev.key = "ArrowUp";
            else if (kc == AKEYCODE_DPAD_DOWN) kev.key = "ArrowDown";
            else if (kc == AKEYCODE_SPACE) { kev.key = " "; kev.text = " "; }
            else if (kc >= AKEYCODE_A && kc <= AKEYCODE_Z) {
                char c = static_cast<char>((kev.shift ? 'A' : 'a') + (kc - AKEYCODE_A));
                kev.key = std::string(1, c);
                if (!kev.ctrl) kev.text = kev.key;
            } else if (kc >= AKEYCODE_0 && kc <= AKEYCODE_9) {
                char c = static_cast<char>('0' + (kc - AKEYCODE_0));
                kev.key = kev.text = std::string(1, c);
            }
            if (!kev.key.empty()) {
                sess->handleKeyEvent(kev);
                meme_host_rerender();
            }
        }
    }
}

void bindAndroidNativeHooks(AndroidAppState *st) {
    if (!st || !st->activity) return;
    static std::mutex sSensorMu;
    static MemeNative::SensorSnapshot sLatestSensors;
    static ASensorManager *sSensorMgr = nullptr;
    static ASensorEventQueue *sSensorQueue = nullptr;
    static const ASensor *sAccel = nullptr;
    static const ASensor *sGyro = nullptr;

    auto withJni = [st](const std::function<void(JNIEnv *, jobject)> &fn) {
        if (!st->activity || !st->activity->vm || !st->activity->clazz) return;
        JNIEnv *env = nullptr;
        bool attached = false;
        jint r = st->activity->vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
        if (r == JNI_EDETACHED) {
            if (st->activity->vm->AttachCurrentThread(&env, nullptr) == JNI_OK) attached = true;
        }
        if (env) {
            fn(env, st->activity->clazz);
            if (env->ExceptionCheck()) env->ExceptionClear();
        }
        if (attached) st->activity->vm->DetachCurrentThread();
    };

    auto &h = MemeNative::hooks();
    h.requestRerender = []() {
        meme_host_rerender();
        if (auto *s = meme_host_get_session()) s->needsRender = true;
    };
    h.getAppPaths = [st]() -> MemeNative::PathsInfo {
        MemeNative::PathsInfo p;
        if (st->activity->internalDataPath) {
            std::string base = st->activity->internalDataPath;
            p.appData = base + "/data";
            p.cache = base + "/cache";
            p.documents = base + "/docs";
            p.temp = base + "/tmp";
        } else {
            p.appData = "/data/local/tmp/meme";
            p.cache = "/data/local/tmp/meme_cache";
            p.documents = p.appData;
            p.temp = "/data/local/tmp";
        }
        std::error_code ec;
        std::filesystem::create_directories(p.appData, ec);
        std::filesystem::create_directories(p.cache, ec);
        return p;
    };
    h.getDeviceInfo = [st, withJni]() -> MemeNative::DeviceInfo {
        MemeNative::DeviceInfo d;
        d.os = "android";
        d.arch = MemeNative::defaultArchName();
        d.isMobile = true; d.isDesktop = false; d.isWeb = false;
        d.model = "Android Device";
        if (st->activity && st->activity->assetManager) {
            AConfiguration *cfg = AConfiguration_new();
            AConfiguration_fromAssetManager(cfg, st->activity->assetManager);
            char lang[3] = {0, 0, 0}, ctry[3] = {0, 0, 0};
            AConfiguration_getLanguage(cfg, lang);
            AConfiguration_getCountry(cfg, ctry);
            if (lang[0]) {
                d.locale = std::string(lang) + (ctry[0] ? ("-" + std::string(ctry)) : "");
            }
            int night = AConfiguration_getUiModeNight(cfg);
            d.isDark = (night != ACONFIGURATION_UI_MODE_NIGHT_NO);
            d.theme = d.isDark ? "dark" : "light";
            AConfiguration_delete(cfg);
        }
        withJni([&d](JNIEnv *env, jobject) {
            jclass bCls = env->FindClass("android/os/Build");
            if (bCls) {
                jfieldID mf = env->GetStaticFieldID(bCls, "MODEL", "Ljava/lang/String;");
                if (mf) {
                    jstring jm = static_cast<jstring>(env->GetStaticObjectField(bCls, mf));
                    if (jm) {
                        const char *p = env->GetStringUTFChars(jm, nullptr);
                        if (p) { d.model = p; env->ReleaseStringUTFChars(jm, p); }
                        env->DeleteLocalRef(jm);
                    }
                }
                env->DeleteLocalRef(bCls);
            }
            jclass vCls = env->FindClass("android/os/Build$VERSION");
            if (vCls) {
                jfieldID rf = env->GetStaticFieldID(vCls, "RELEASE", "Ljava/lang/String;");
                if (rf) {
                    jstring jv = static_cast<jstring>(env->GetStaticObjectField(vCls, rf));
                    if (jv) {
                        const char *p = env->GetStringUTFChars(jv, nullptr);
                        if (p) { d.osVersion = p; env->ReleaseStringUTFChars(jv, p); }
                        env->DeleteLocalRef(jv);
                    }
                }
                env->DeleteLocalRef(vCls);
            }
        });
        return d;
    };
    h.getScreenInfo = [st]() -> MemeNative::ScreenInfo {
        MemeNative::ScreenInfo s;
        float sc = st->densityScale > 0.0f ? st->densityScale : 1.0f;
        s.scale = sc;
        if (auto *sess = meme_host_get_session()) {
            s.width = sess->input.viewportW;
            s.height = sess->input.viewportH;
            s.safeTop = sess->input.safeTop;
            s.safeBottom = sess->input.safeBottom;
        }
        return s;
    };
    h.hapticVibrate = [withJni](int ms) -> bool {
        bool ok = false;
        withJni([ms, &ok](JNIEnv *env, jobject act) {
            jclass actCls = env->GetObjectClass(act);
            jmethodID gss = env->GetMethodID(actCls, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
            jstring svcName = env->NewStringUTF("vibrator");
            jobject vib = env->CallObjectMethod(act, gss, svcName);
            env->DeleteLocalRef(svcName);
            if (vib && !env->ExceptionCheck()) {
                jclass vCls = env->GetObjectClass(vib);
                jmethodID vm = env->GetMethodID(vCls, "vibrate", "(J)V");
                if (vm && !env->ExceptionCheck()) {
                    env->CallVoidMethod(vib, vm, static_cast<jlong>(std::max(1, ms)));
                    ok = !env->ExceptionCheck();
                }
                env->DeleteLocalRef(vCls);
                env->DeleteLocalRef(vib);
            }
            env->DeleteLocalRef(actCls);
        });
        return ok;
    };
    h.hapticImpact = [](const std::string &style) -> bool {
        return MemeNative::hapticVibrate(style == "heavy" ? 35 : (style == "light" ? 12 : 20));
    };
    h.hapticNotify = [](const std::string &type) -> bool {
        return MemeNative::hapticVibrate(type == "error" ? 50 : 25);
    };
    h.hapticSelection = []() -> bool { return MemeNative::hapticVibrate(10); };
    h.getBatteryInfo = [withJni]() -> MemeNative::BatteryInfo {
        MemeNative::BatteryInfo b;
        withJni([&b](JNIEnv *env, jobject act) {
            jclass actCls = env->GetObjectClass(act);
            jmethodID gss = env->GetMethodID(actCls, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
            jstring svc = env->NewStringUTF("batterymanager");
            jobject bm = env->CallObjectMethod(act, gss, svc);
            env->DeleteLocalRef(svc);
            if (bm && !env->ExceptionCheck()) {
                jclass bCls = env->GetObjectClass(bm);
                jmethodID gip = env->GetMethodID(bCls, "getIntProperty", "(I)I");
                jmethodID ic = env->GetMethodID(bCls, "isCharging", "()Z");
                if (gip) {
                    int lvl = env->CallIntMethod(bm, gip, 4 /*BATTERY_PROPERTY_CAPACITY*/);
                    if (lvl >= 0 && lvl <= 100) { b.level = lvl; b.supported = true; }
                }
                if (ic && !env->ExceptionCheck()) b.charging = (env->CallBooleanMethod(bm, ic) != JNI_FALSE);
                b.state = b.charging ? "charging" : (b.supported ? "discharging" : "unknown");
                env->DeleteLocalRef(bCls);
                env->DeleteLocalRef(bm);
            }
            env->DeleteLocalRef(actCls);
        });
        return b;
    };
    h.openUrl = [withJni](const std::string &url) -> bool {
        bool ok = false;
        withJni([&url, &ok](JNIEnv *env, jobject act) {
            jclass uriCls = env->FindClass("android/net/Uri");
            jclass intCls = env->FindClass("android/content/Intent");
            jclass actCls = env->GetObjectClass(act);
            if (uriCls && intCls) {
                jmethodID parseFn = env->GetStaticMethodID(uriCls, "parse", "(Ljava/lang/String;)Landroid/net/Uri;");
                jstring jsUrl = env->NewStringUTF(url.c_str());
                jobject uri = env->CallStaticObjectMethod(uriCls, parseFn, jsUrl);
                jstring jsAct = env->NewStringUTF("android.intent.action.VIEW");
                jmethodID ctor = env->GetMethodID(intCls, "<init>", "(Ljava/lang/String;Landroid/net/Uri;)V");
                jobject intent = env->NewObject(intCls, ctor, jsAct, uri);
                jmethodID sa = env->GetMethodID(actCls, "startActivity", "(Landroid/content/Intent;)V");
                if (sa && intent) {
                    env->CallVoidMethod(act, sa, intent);
                    ok = !env->ExceptionCheck();
                }
                env->DeleteLocalRef(jsUrl);
                env->DeleteLocalRef(jsAct);
                if (uri) env->DeleteLocalRef(uri);
                if (intent) env->DeleteLocalRef(intent);
            }
            if (uriCls) env->DeleteLocalRef(uriCls);
            if (intCls) env->DeleteLocalRef(intCls);
            env->DeleteLocalRef(actCls);
        });
        return ok;
    };
    h.getSensorSnapshot = [st]() -> MemeNative::SensorSnapshot {
        std::lock_guard<std::mutex> lock(sSensorMu);
        if (!sSensorMgr) {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
            sSensorMgr = ASensorManager_getInstance();
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
            if (sSensorMgr) {
                ALooper *lp = ALooper_forThread();
                if (!lp) lp = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
                if (lp) {
                    sSensorQueue = ASensorManager_createEventQueue(sSensorMgr, lp, 2, nullptr, nullptr);
                    sAccel = ASensorManager_getDefaultSensor(sSensorMgr, ASENSOR_TYPE_ACCELEROMETER);
                    sGyro = ASensorManager_getDefaultSensor(sSensorMgr, ASENSOR_TYPE_GYROSCOPE);
                    if (sSensorQueue && sAccel) ASensorEventQueue_enableSensor(sSensorQueue, sAccel);
                    if (sSensorQueue && sGyro) ASensorEventQueue_enableSensor(sSensorQueue, sGyro);
                }
            }
        }
        if (sSensorQueue) {
            ASensorEvent ev{};
            while (ASensorEventQueue_getEvents(sSensorQueue, &ev, 1) > 0) {
                sLatestSensors.available = true;
                if (ev.type == ASENSOR_TYPE_ACCELEROMETER) {
                    sLatestSensors.accelX = ev.acceleration.x;
                    sLatestSensors.accelY = ev.acceleration.y;
                    sLatestSensors.accelZ = ev.acceleration.z;
                } else if (ev.type == ASENSOR_TYPE_GYROSCOPE) {
                    sLatestSensors.gyroX = ev.vector.x;
                    sLatestSensors.gyroY = ev.vector.y;
                    sLatestSensors.gyroZ = ev.vector.z;
                }
            }
        }
        (void)st;
        return sLatestSensors;
    };
    h.hideKeyboard = [withJni]() -> bool {
        bool ok = false;
        withJni([&ok](JNIEnv *env, jobject act) {
            jclass actCls = env->GetObjectClass(act);
            jmethodID gss = env->GetMethodID(actCls, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
            jstring svc = env->NewStringUTF("input_method");
            jobject imm = env->CallObjectMethod(act, gss, svc);
            jmethodID gw = env->GetMethodID(actCls, "getWindow", "()Landroid/view/Window;");
            jobject win = env->CallObjectMethod(act, gw);
            if (imm && win && !env->ExceptionCheck()) {
                jclass wCls = env->GetObjectClass(win);
                jmethodID gd = env->GetMethodID(wCls, "getDecorView", "()Landroid/view/View;");
                jobject dv = env->CallObjectMethod(win, gd);
                if (dv) {
                    jclass vCls = env->GetObjectClass(dv);
                    jmethodID gwt = env->GetMethodID(vCls, "getWindowToken", "()Landroid/os/IBinder;");
                    jobject tok = env->CallObjectMethod(dv, gwt);
                    if (tok) {
                        jclass immCls = env->GetObjectClass(imm);
                        jmethodID hk = env->GetMethodID(immCls, "hideSoftInputFromWindow", "(Landroid/os/IBinder;I)Z");
                        if (hk) ok = (env->CallBooleanMethod(imm, hk, tok, 0) != JNI_FALSE);
                        env->DeleteLocalRef(immCls);
                        env->DeleteLocalRef(tok);
                    }
                    env->DeleteLocalRef(vCls);
                    env->DeleteLocalRef(dv);
                }
                env->DeleteLocalRef(wCls);
            }
            env->DeleteLocalRef(svc);
            if (win) env->DeleteLocalRef(win);
            if (imm) env->DeleteLocalRef(imm);
            env->DeleteLocalRef(actCls);
        });
        return ok;
    };
    auto androidPermName = [](const std::string &rawCap, int sdkInt) -> std::string {
        std::string c = MemePermissions::normalizeCapability(rawCap);
        if (c == "notifications") return (sdkInt >= 33) ? "android.permission.POST_NOTIFICATIONS" : "";
        if (c == "camera") return "android.permission.CAMERA";
        if (c == "microphone") return "android.permission.RECORD_AUDIO";
        if (c == "photos" || c == "storage") return (sdkInt >= 33) ? "android.permission.READ_MEDIA_IMAGES" : "android.permission.READ_EXTERNAL_STORAGE";
        if (c == "location") return "android.permission.ACCESS_FINE_LOCATION";
        if (c == "locationAlways") return (sdkInt >= 29) ? "android.permission.ACCESS_BACKGROUND_LOCATION" : "android.permission.ACCESS_FINE_LOCATION";
        if (c == "contacts") return "android.permission.READ_CONTACTS";
        if (c == "calendar") return "android.permission.READ_CALENDAR";
        if (c == "bluetooth") return (sdkInt >= 31) ? "android.permission.BLUETOOTH_SCAN" : "android.permission.BLUETOOTH";
        if (c == "sensors") return (sdkInt >= 29) ? "android.permission.ACTIVITY_RECOGNITION" : "";
        if (c == "biometrics") return (sdkInt >= 28) ? "android.permission.USE_BIOMETRIC" : "";
        return "";
    };
    auto getSdkInt = [withJni]() -> int {
        int sdk = 30;
        withJni([&sdk](JNIEnv *env, jobject) {
            jclass vCls = env->FindClass("android/os/Build$VERSION");
            if (vCls) {
                jfieldID sf = env->GetStaticFieldID(vCls, "SDK_INT", "I");
                if (sf) sdk = static_cast<int>(env->GetStaticIntField(vCls, sf));
                env->DeleteLocalRef(vCls);
            }
        });
        return sdk;
    };
    h.checkPermission = [withJni, androidPermName, getSdkInt](const std::string &rawCap) -> MemePermissions::PermissionStatus {
        std::string cap = MemePermissions::normalizeCapability(rawCap);
        MemePermissions::PermissionStatus out;
        out.permission = cap;
        out.platform = "android";
        int sdk = getSdkInt();
        std::string perm = androidPermName(cap, sdk);
        if (perm.empty()) {
            out.state = MemePermissions::isSupportedOnPlatform(cap, "android") ? "granted" : "unavailable";
            out.finalizeFlags();
            return out;
        }
        withJni([&](JNIEnv *env, jobject act) {
            jclass actCls = env->GetObjectClass(act);
            jmethodID csp = env->GetMethodID(actCls, "checkSelfPermission", "(Ljava/lang/String;)I");
            jmethodID rationale = env->GetMethodID(actCls, "shouldShowRequestPermissionRationale", "(Ljava/lang/String;)Z");
            jstring jp = env->NewStringUTF(perm.c_str());
            jint gr = csp ? env->CallIntMethod(act, csp, jp) : 0;
            if (gr == 0 /*PERMISSION_GRANTED*/) {
                out.state = "granted";
            } else {
                bool showRat = rationale ? (env->CallBooleanMethod(act, rationale, jp) != JNI_FALSE) : false;
                out.state = showRat ? "prompt-with-rationale" : "prompt";
            }
            env->DeleteLocalRef(jp);
            env->DeleteLocalRef(actCls);
        });
        out.finalizeFlags();
        return out;
    };
    h.requestPermissionAsync = [withJni, androidPermName, getSdkInt](const std::string &rawCap, MemeNative::AsyncCallback cb) {
        std::string cap = MemePermissions::normalizeCapability(rawCap);
        int rid = MemeNative::registerAsyncCallback(std::move(cb));
        int sdk = getSdkInt();
        std::string perm = androidPermName(cap, sdk);
        if (perm.empty() || sdk < 23) {
            MemeNative::AsyncResponse r;
            r.permissionState = "granted";
            r.ok = true; r.accepted = true; r.value = "granted";
            MemeNative::resolveAsyncCallback(rid, r);
            return;
        }
        bool requested = false;
        bool alreadyGranted = false;
        withJni([&](JNIEnv *env, jobject act) {
            jclass actCls = env->GetObjectClass(act);
            jmethodID csp = env->GetMethodID(actCls, "checkSelfPermission", "(Ljava/lang/String;)I");
            jstring jp = env->NewStringUTF(perm.c_str());
            if (csp && env->CallIntMethod(act, csp, jp) == 0) {
                alreadyGranted = true;
            } else {
                jmethodID rp = env->GetMethodID(actCls, "requestPermissions", "([Ljava/lang/String;I)V");
                jclass strCls = env->FindClass("java/lang/String");
                if (rp && strCls) {
                    jobjectArray arr = env->NewObjectArray(1, strCls, jp);
                    env->CallVoidMethod(act, rp, arr, static_cast<jint>(rid));
                    requested = !env->ExceptionCheck();
                    env->DeleteLocalRef(arr);
                }
                if (strCls) env->DeleteLocalRef(strCls);
            }
            env->DeleteLocalRef(jp);
            env->DeleteLocalRef(actCls);
        });
        if (alreadyGranted) {
            MemeNative::AsyncResponse r;
            r.permissionState = "granted";
            r.ok = true; r.accepted = true; r.canAskAgain = false; r.value = "granted";
            MemeNative::resolveAsyncCallback(rid, r);
            return;
        }
        if (!requested) {
            MemeNative::AsyncResponse r;
            r.permissionState = "prompt";
            r.ok = false; r.accepted = false; r.error = "Unable to invoke Activity.requestPermissions";
            MemeNative::resolveAsyncCallback(rid, r);
        }
    };
    h.openAppSettings = [withJni](const std::string &) -> bool {
        bool ok = false;
        withJni([&ok](JNIEnv *env, jobject act) {
            jclass actCls = env->GetObjectClass(act);
            jmethodID gpn = env->GetMethodID(actCls, "getPackageName", "()Ljava/lang/String;");
            jstring pkg = gpn ? static_cast<jstring>(env->CallObjectMethod(act, gpn)) : nullptr;
            jclass uriCls = env->FindClass("android/net/Uri");
            jclass intCls = env->FindClass("android/content/Intent");
            if (pkg && uriCls && intCls) {
                jmethodID fp = env->GetStaticMethodID(uriCls, "fromParts", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Landroid/net/Uri;");
                jstring jsScheme = env->NewStringUTF("package");
                jobject uri = fp ? env->CallStaticObjectMethod(uriCls, fp, jsScheme, pkg, nullptr) : nullptr;
                jstring jsAct = env->NewStringUTF("android.settings.APPLICATION_DETAILS_SETTINGS");
                jmethodID ctor = env->GetMethodID(intCls, "<init>", "(Ljava/lang/String;Landroid/net/Uri;)V");
                jobject intent = (uri && ctor) ? env->NewObject(intCls, ctor, jsAct, uri) : nullptr;
                jmethodID sa = env->GetMethodID(actCls, "startActivity", "(Landroid/content/Intent;)V");
                if (sa && intent) {
                    env->CallVoidMethod(act, sa, intent);
                    ok = !env->ExceptionCheck();
                }
                env->DeleteLocalRef(jsScheme);
                env->DeleteLocalRef(jsAct);
                if (uri) env->DeleteLocalRef(uri);
                if (intent) env->DeleteLocalRef(intent);
            }
            if (pkg) env->DeleteLocalRef(pkg);
            if (uriCls) env->DeleteLocalRef(uriCls);
            if (intCls) env->DeleteLocalRef(intCls);
            env->DeleteLocalRef(actCls);
        });
        return ok;
    };
}

} // namespace

extern "C" JNIEXPORT void JNICALL
Java_dev_memelang_host_MemeNativeActivity_nativeOnPermissionResult(
    JNIEnv *, jclass, jint reqId, jboolean granted, jboolean canAskAgain)
{
    MemeNative::AsyncResponse r;
    r.ok = (granted != JNI_FALSE);
    r.accepted = r.ok;
    r.canAskAgain = r.ok ? false : (canAskAgain != JNI_FALSE);
    r.permissionState = r.ok ? "granted" : (r.canAskAgain ? "prompt-with-rationale" : "denied");
    r.value = r.permissionState;
    MemeNative::resolveAsyncCallback(static_cast<int>(reqId), r);
}

namespace {

void *androidMainLoop(void *arg) {
    auto *st = static_cast<AndroidAppState *>(arg);
    ALooper *looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);

    bindAndroidNativeHooks(st);
    char arg0[] = "meme_android";
    char *argv[] = {arg0};
    meme_user_entrypoint(1, argv);
    MemeUI::UiSession *sess = meme_host_get_session();

    while (st->running) {
        pthread_mutex_lock(&st->mtx);
        if (st->pendingWindow != st->window) {
            if (st->window) termGles(st);
            st->window = st->pendingWindow;
            if (st->window) initGles(st);
        }
        if (st->pendingInputQueue != st->inputQueue) {
            if (st->inputQueue) AInputQueue_detachLooper(st->inputQueue);
            st->inputQueue = st->pendingInputQueue;
            if (st->inputQueue) AInputQueue_attachLooper(st->inputQueue, looper, 1, nullptr, nullptr);
        }
        pthread_mutex_unlock(&st->mtx);

        ALooper_pollOnce(st->window ? 8 : 16, nullptr, nullptr, nullptr);

        if (st->inputQueue) {
            AInputEvent *ev = nullptr;
            while (AInputQueue_getEvent(st->inputQueue, &ev) >= 0) {
                if (AInputQueue_preDispatchEvent(st->inputQueue, ev)) continue;
                processAndroidInput(st, ev, sess);
                AInputQueue_finishEvent(st->inputQueue, ev, 1);
            }
        }

        if (st->window && sess) {
            bool inertiaMoved = false;
            if (!st->touchActive && (std::abs(st->scrollVelX) > 0.25f || std::abs(st->scrollVelY) > 0.25f)) {
                if (sess->handleScroll(st->lastTouchX, st->lastTouchY, st->scrollVelX / 28.0f, st->scrollVelY / 28.0f)) {
                    st->scrollVelX *= 0.92f;
                    st->scrollVelY *= 0.92f;
                    inertiaMoved = true;
                } else {
                    st->scrollVelX = 0.0f;
                    st->scrollVelY = 0.0f;
                }
            }
            if (inertiaMoved || sess->tick(16.0) || sess->isImmediateMode || sess->needsRender) {
                meme_host_rerender();
            }
            drawGlesFrame(st, sess);
        }
    }
    termGles(st);
    return nullptr;
}

void onWinCreated(ANativeActivity *act, ANativeWindow *w) {
    auto *st = static_cast<AndroidAppState *>(act->instance);
    pthread_mutex_lock(&st->mtx);
    st->pendingWindow = w;
    pthread_mutex_unlock(&st->mtx);
}
void onWinDestroyed(ANativeActivity *act, ANativeWindow *) {
    auto *st = static_cast<AndroidAppState *>(act->instance);
    pthread_mutex_lock(&st->mtx);
    st->pendingWindow = nullptr;
    pthread_mutex_unlock(&st->mtx);
}
void onQueueCreated(ANativeActivity *act, AInputQueue *q) {
    auto *st = static_cast<AndroidAppState *>(act->instance);
    pthread_mutex_lock(&st->mtx);
    st->pendingInputQueue = q;
    pthread_mutex_unlock(&st->mtx);
}
void onQueueDestroyed(ANativeActivity *act, AInputQueue *) {
    auto *st = static_cast<AndroidAppState *>(act->instance);
    pthread_mutex_lock(&st->mtx);
    st->pendingInputQueue = nullptr;
    pthread_mutex_unlock(&st->mtx);
}
void onActDestroy(ANativeActivity *act) {
    auto *st = static_cast<AndroidAppState *>(act->instance);
    st->running = false;
    pthread_join(st->thread, nullptr);
    delete st;
}

} // namespace

extern "C" __attribute__((visibility("default")))
void ANativeActivity_onCreate(ANativeActivity *activity, void *savedState, size_t savedStateSize) {
    (void)savedState; (void)savedStateSize;
    auto *st = new AndroidAppState();
    st->activity = activity;
    AConfiguration *cfg = AConfiguration_new();
    AConfiguration_fromAssetManager(cfg, activity->assetManager);
    int32_t dpi = AConfiguration_getDensity(cfg);
    AConfiguration_delete(cfg);
    st->densityScale = (dpi > 0 && dpi != ACONFIGURATION_DENSITY_NONE) ? std::max(1.0f, static_cast<float>(dpi) / 160.0f) : 2.5f;

    activity->instance = st;
    activity->callbacks->onNativeWindowCreated = onWinCreated;
    activity->callbacks->onNativeWindowDestroyed = onWinDestroyed;
    activity->callbacks->onInputQueueCreated = onQueueCreated;
    activity->callbacks->onInputQueueDestroyed = onQueueDestroyed;
    activity->callbacks->onDestroy = onActDestroy;

    st->running = true;
    pthread_create(&st->thread, nullptr, androidMainLoop, st);
}

#endif // __ANDROID__
