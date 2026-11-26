#if defined(__ANDROID__)

#include <android/native_activity.h>
#include <android/native_window.h>
#include <android/input.h>
#include <android/keycodes.h>
#include <android/looper.h>
#include <android/configuration.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
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

void *androidMainLoop(void *arg) {
    auto *st = static_cast<AndroidAppState *>(arg);
    ALooper *looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);

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
