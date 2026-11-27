#pragma once

#include "../../ui/MemeUiTypes.h"
#include "../../ui/MemeUiTessellator.h"
#include "../../ui/MemeUiRuntime.h"
#include "../../platform/MemeNativeApis.h"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define MEME_WEB_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define MEME_WEB_EXPORT
#endif

namespace MemeWebHost {

inline MemeUI::UiSession *g_activeSession = nullptr;
inline std::function<void()> g_rerenderFn = nullptr;
inline std::function<bool(const std::string &)> g_setRouteFn = nullptr;
inline std::string g_lastBatchesJson = "[]";
inline std::string g_lastCommandsJson = "[]";
inline std::string g_lastHitsJson = "[]";
inline std::string g_lastAgentJson = "{}";
inline std::string g_clipboardBuf = "";
inline std::vector<uint8_t> g_atlasRgba;

inline void bindSession(MemeUI::UiSession *sess, std::function<void()> rerender = nullptr) {
    g_activeSession = sess;
    g_rerenderFn = std::move(rerender);
    if (sess) {
        sess->clipboardReadFn = []() -> std::string { return MemeNative::clipboardGet(); };
        sess->clipboardWriteFn = [](const std::string &s) { g_clipboardBuf = s; MemeNative::clipboardSet(s); };
        auto &nh = MemeNative::hooks();
        nh.requestRerender = []() {
            if (g_activeSession) g_activeSession->needsRender = true;
            if (g_rerenderFn) g_rerenderFn();
        };
        nh.getScreenInfo = []() -> MemeNative::ScreenInfo {
            MemeNative::ScreenInfo s;
            if (g_activeSession) {
                s.width = g_activeSession->input.viewportW;
                s.height = g_activeSession->input.viewportH;
                s.scale = g_activeSession->input.dpiScale > 0 ? g_activeSession->input.dpiScale : 1.0f;
            }
            return s;
        };
    }
}

inline std::string buildBatchesJson(const MemeUI::UiMesh &mesh) {
    std::string out = "[";
    for (size_t i = 0; i < mesh.batches.size(); ++i) {
        if (i > 0) out += ",";
        const auto &b = mesh.batches[i];
        out += "{\"x\":" + std::to_string(b.clipRect.x) +
               ",\"y\":" + std::to_string(b.clipRect.y) +
               ",\"width\":" + std::to_string(b.clipRect.width) +
               ",\"height\":" + std::to_string(b.clipRect.height) +
               ",\"offset\":" + std::to_string(b.indexOffset) +
               ",\"count\":" + std::to_string(b.indexCount) + "}";
    }
    out += "]";
    return out;
}

inline std::string generateZeroDomHtmlShell(const std::string &jsFileName, const std::string &title = "MEME-UI Web Native", const std::string &themeColor = "#020617", bool includePwaMeta = false, const std::string &permissionsPolicy = "") {
    std::string pwaHead;
    if (includePwaMeta) {
        pwaHead =
            "<link rel=\"manifest\" href=\"manifest.webmanifest\" />\n"
            "<link rel=\"icon\" href=\"favicon.ico\" />\n"
            "<link rel=\"apple-touch-icon\" href=\"apple-touch-icon.png\" />\n"
            "<meta name=\"theme-color\" content=\"" + themeColor + "\" />\n"
            "<meta name=\"apple-mobile-web-app-capable\" content=\"yes\" />\n"
            "<meta name=\"apple-mobile-web-app-status-bar-style\" content=\"black-translucent\" />\n";
    }
    if (!permissionsPolicy.empty()) {
        pwaHead += "<meta http-equiv=\"Permissions-Policy\" content=\"" + permissionsPolicy + "\" />\n";
    }
    return std::string(R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8" />
<base href="/" />
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no,viewport-fit=cover" />
)HTML") + pwaHead + R"HTML(<title>)HTML" + title + R"HTML(</title>
<style>
  html, body { margin: 0; padding: 0; width: 100%; height: 100%; overflow: hidden; background: )HTML" + themeColor + R"HTML(; }
  canvas#meme-canvas { display: block; width: 100vw; height: 100vh; outline: none; touch-action: none; }
  #meme-splash { position: fixed; inset: 0; display: flex; flex-direction: column; align-items: center; justify-content: center; gap: 14px; background: )HTML" + themeColor + R"HTML(; color: #e2e8f0; font-family: system-ui, -apple-system, sans-serif; z-index: 9999; pointer-events: none; }
  #meme-splash .meme-spin { width: 34px; height: 34px; border-radius: 9999px; border: 3px solid #1e293b; border-top-color: #6366f1; animation: memeSpin 0.8s linear infinite; }
  @keyframes memeSpin { to { transform: rotate(360deg); } }
</style>
</head>
<body>
<div id="meme-splash"><div class="meme-spin"></div><div>)HTML" + title + R"HTML(</div></div>
<canvas id="meme-canvas" tabindex="0"></canvas>
<script>
(async () => {
  const canvas = document.getElementById("meme-canvas");
  let Mod = null;
  let gl = canvas.getContext("webgl2", { alpha: false, antialias: true });
  let ctx2d = gl ? null : canvas.getContext("2d");
  let glProg = null, atlasTex = null, vbo = null, ibo = null;
  let uVp = null, uAtl = null, aPos = -1, aUv = -1, aCol = -1;

  function initGl() {
    if (!gl) return false;
    const vsSrc = `#version 300 es
      in vec2 aPos; in vec2 aUv; in vec4 aCol;
      uniform vec2 uVp;
      out vec2 vUv; out vec4 vCol;
      void main() {
        gl_Position = vec4((aPos.x / max(1.0, uVp.x)) * 2.0 - 1.0, 1.0 - (aPos.y / max(1.0, uVp.y)) * 2.0, 0.0, 1.0);
        vUv = aUv; vCol = aCol;
      }`;
    const fsSrc = `#version 300 es
      precision mediump float;
      uniform sampler2D uAtl;
      in vec2 vUv; in vec4 vCol;
      out vec4 fragColor;
      void main() {
        vec4 tex = texture(uAtl, vUv);
        fragColor = vec4(vCol.rgb * tex.rgb, vCol.a * tex.a);
      }`;
    const comp = (t, s) => { const sh = gl.createShader(t); gl.shaderSource(sh, s); gl.compileShader(sh); return sh; };
    glProg = gl.createProgram();
    gl.attachShader(glProg, comp(gl.VERTEX_SHADER, vsSrc));
    gl.attachShader(glProg, comp(gl.FRAGMENT_SHADER, fsSrc));
    gl.linkProgram(glProg);
    aPos = gl.getAttribLocation(glProg, "aPos");
    aUv = gl.getAttribLocation(glProg, "aUv");
    aCol = gl.getAttribLocation(glProg, "aCol");
    uVp = gl.getUniformLocation(glProg, "uVp");
    uAtl = gl.getUniformLocation(glProg, "uAtl");
    vbo = gl.createBuffer();
    ibo = gl.createBuffer();

    const aw = Mod._meme_web_get_atlas_width ? Mod._meme_web_get_atlas_width() : 512;
    const ah = Mod._meme_web_get_atlas_height ? Mod._meme_web_get_atlas_height() : 512;
    const atlasPtr = Mod._meme_web_get_atlas_rgba();
    const atlasBytes = Mod.HEAPU8.subarray(atlasPtr, atlasPtr + aw * ah * 4);
    atlasTex = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, atlasTex);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, aw, ah, 0, gl.RGBA, gl.UNSIGNED_BYTE, atlasBytes);
    return true;
  }

  function renderFrame() {
    if (!Mod) return;
    const dpr = window.devicePixelRatio || 1;
    const vw = canvas.clientWidth || window.innerWidth || 960;
    const vh = canvas.clientHeight || window.innerHeight || 720;
    if (canvas.width !== Math.floor(vw * dpr) || canvas.height !== Math.floor(vh * dpr)) {
      canvas.width = Math.floor(vw * dpr);
      canvas.height = Math.floor(vh * dpr);
    }
    Mod._meme_web_render(vw, vh);

    if (gl && glProg) {
      gl.viewport(0, 0, canvas.width, canvas.height);
      gl.clearColor(0.008, 0.024, 0.09, 1.0);
      gl.clear(gl.COLOR_BUFFER_BIT);
      gl.enable(gl.BLEND);
      gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
      gl.enable(gl.SCISSOR_TEST);

      const vCount = Mod._meme_web_get_mesh_vertex_count();
      const iCount = Mod._meme_web_get_mesh_index_count();
      if (vCount > 0 && iCount > 0) {
        const vPtr = Mod._meme_web_get_mesh_vertices();
        const iPtr = Mod._meme_web_get_mesh_indices();
        const batches = JSON.parse(Mod.UTF8ToString(Mod._meme_web_get_batches_json()));
        gl.useProgram(glProg);
        gl.uniform2f(uVp, vw, vh);
        gl.activeTexture(gl.TEXTURE0);
        gl.bindTexture(gl.TEXTURE_2D, atlasTex);
        gl.uniform1i(uAtl, 0);

        gl.bindBuffer(gl.ARRAY_BUFFER, vbo);
        gl.bufferData(gl.ARRAY_BUFFER, Mod.HEAPU8.subarray(vPtr, vPtr + vCount * 20), gl.STREAM_DRAW);
        gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, ibo);
        gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, Mod.HEAPU32.subarray(iPtr >> 2, (iPtr >> 2) + iCount), gl.STREAM_DRAW);

        gl.enableVertexAttribArray(aPos);
        gl.vertexAttribPointer(aPos, 2, gl.FLOAT, false, 20, 0);
        gl.enableVertexAttribArray(aUv);
        gl.vertexAttribPointer(aUv, 2, gl.FLOAT, false, 20, 8);
        gl.enableVertexAttribArray(aCol);
        gl.vertexAttribPointer(aCol, 4, gl.UNSIGNED_BYTE, true, 20, 16);

        for (const b of batches) {
          if (b.count <= 0) continue;
          const sx = Math.max(0, Math.floor(b.x * dpr));
          const ex = Math.min(canvas.width, Math.ceil((b.x + b.width) * dpr));
          const sy = Math.max(0, Math.floor(b.y * dpr));
          const ey = Math.min(canvas.height, Math.ceil((b.y + b.height) * dpr));
          if (ex <= sx || ey <= sy) continue;
          gl.scissor(sx, canvas.height - ey, ex - sx, ey - sy);
          gl.drawElements(gl.TRIANGLES, b.count, gl.UNSIGNED_INT, b.offset * 4);
        }
      }
      gl.disable(gl.SCISSOR_TEST);
    } else if (ctx2d) {
      ctx2d.save();
      ctx2d.setTransform(dpr, 0, 0, dpr, 0, 0);
      ctx2d.fillStyle = "#020617";
      ctx2d.fillRect(0, 0, vw, vh);
      const cmds = JSON.parse(Mod.UTF8ToString(Mod._meme_web_get_commands_json()));
      for (const c of cmds) {
        if (c.type === "PUSH_CLIP") { ctx2d.save(); ctx2d.beginPath(); ctx2d.rect(c.x, c.y, c.width, c.height); ctx2d.clip(); }
        else if (c.type === "POP_CLIP") { ctx2d.restore(); }
        else if (c.type === "FILL_ROUND_RECT" || c.type === "DRAW_SELECTION_RECT" || c.type === "DRAW_CARET") {
          ctx2d.globalAlpha = c.alpha ?? 1; ctx2d.fillStyle = c.color;
          ctx2d.beginPath(); ctx2d.roundRect(c.x, c.y, Math.max(0, c.width), Math.max(0, c.height), c.radius || 0); ctx2d.fill();
        } else if (c.type === "STROKE_ROUND_RECT") {
          ctx2d.globalAlpha = c.alpha ?? 1; ctx2d.strokeStyle = c.color; ctx2d.lineWidth = c.lineWidth || 1;
          ctx2d.beginPath(); ctx2d.roundRect(c.x, c.y, Math.max(0, c.width), Math.max(0, c.height), c.radius || 0); ctx2d.stroke();
        } else if (c.type === "DRAW_TEXT" && c.text) {
          ctx2d.globalAlpha = c.alpha ?? 1; ctx2d.fillStyle = c.color;
          ctx2d.font = `${c.fontWeight || 400} ${c.fontSize || 14}px ${c.isMono ? "monospace" : "system-ui,sans-serif"}`;
          ctx2d.fillText(c.text, c.x, c.y);
        } else if (c.type === "DRAW_LINE") {
          ctx2d.globalAlpha = c.alpha ?? 1; ctx2d.strokeStyle = c.color; ctx2d.lineWidth = c.lineWidth || 1;
          ctx2d.beginPath(); ctx2d.moveTo(c.x, c.y); ctx2d.lineTo(c.x + c.width, c.y + c.height); ctx2d.stroke();
        } else if (c.type === "DRAW_CIRCLE" || c.type === "STROKE_CIRCLE") {
          ctx2d.globalAlpha = c.alpha ?? 1;
          ctx2d.beginPath(); ctx2d.arc(c.x + c.width / 2, c.y + c.height / 2, c.radius || c.width / 2, 0, Math.PI * 2);
          if (c.type === "DRAW_CIRCLE") { ctx2d.fillStyle = c.color; ctx2d.fill(); } else { ctx2d.strokeStyle = c.color; ctx2d.stroke(); }
        }
      }
      ctx2d.restore();
    }
  }

  const getBrowserRoute = () => {
    const h = window.location.hash || "";
    if (h.startsWith("#/")) return h.slice(1);
    const p = (window.location.pathname || "/") + (window.location.search || "");
    if (!p || p.endsWith(".html")) return "/";
    return p;
  };
  window.__MEME_PUSH_ROUTE__ = (target) => {
    try {
      const nextRoute = target && target.startsWith("/") ? target : ("/" + String(target || ""));
      if (window.location.protocol === "file:") {
        if (window.location.hash !== "#" + nextRoute) window.location.hash = "#" + nextRoute;
      } else {
        const cur = (window.location.pathname || "/") + (window.location.search || "");
        if (cur !== nextRoute) window.history.pushState(null, "", nextRoute);
      }
    } catch (_) {}
  };
  window.addEventListener("popstate", () => {
    if (Mod && Mod._meme_web_set_route) {
      Mod.ccall("meme_web_set_route", "number", ["string"], [getBrowserRoute()]);
      renderFrame();
    }
  });
  window.addEventListener("hashchange", () => {
    if (Mod && Mod._meme_web_set_route) {
      Mod.ccall("meme_web_set_route", "number", ["string"], [getBrowserRoute()]);
      renderFrame();
    }
  });

  window.__MEME_AGENT__ = {
    snapshot: () => Mod ? JSON.parse(Mod.UTF8ToString(Mod._meme_web_get_agent_snapshot())) : null,
    act: (target, action, val = "") => {
      if (!Mod) return 0;
      const res = Mod.ccall("meme_web_agent_act", "number", ["string", "string", "string"], [String(target), action, String(val)]);
      if (Mod._meme_web_tick) Mod._meme_web_tick(0);
      renderFrame();
      return res;
    },
    tick: (ms) => {
      if (!Mod) return 0;
      const dirty = Mod._meme_web_tick(ms);
      if (dirty) renderFrame();
      return dirty;
    }
  };

  window.Module = {
    onRuntimeInitialized() {
      Mod = window.Module;
      const splash = document.getElementById("meme-splash");
      if (splash) splash.remove();
      initGl();
      if (Mod._meme_web_set_route) Mod.ccall("meme_web_set_route", "number", ["string"], [getBrowserRoute()]);
      if (Mod._meme_web_start) Mod._meme_web_start();
      if (Mod._meme_web_set_route) Mod.ccall("meme_web_set_route", "number", ["string"], [getBrowserRoute()]);
      renderFrame();
      const loop = () => {
        if (Mod._meme_web_tick(16.67)) renderFrame();
        requestAnimationFrame(loop);
      };
      requestAnimationFrame(loop);
    }
  };

  const s = document.createElement("script");
  s.src = ")HTML" + jsFileName + R"HTML(";
  document.body.appendChild(s);

  const pt = (e) => {
    const r = canvas.getBoundingClientRect();
    return [e.clientX - r.left, e.clientY - r.top];
  };
  const touchDrag = { active: false, id: -1, startX: 0, startY: 0, lastX: 0, lastY: 0, scrolled: false, vx: 0, vy: 0, raf: 0 };
  const stopInertia = () => {
    if (touchDrag.raf) { cancelAnimationFrame(touchDrag.raf); touchDrag.raf = 0; }
    touchDrag.vx = 0; touchDrag.vy = 0;
  };
  const stepInertia = () => {
    touchDrag.raf = 0;
    if (!Mod || touchDrag.active) return;
    if (Math.abs(touchDrag.vx) <= 0.25 && Math.abs(touchDrag.vy) <= 0.25) return;
    if (Mod._meme_web_scroll(touchDrag.lastX, touchDrag.lastY, touchDrag.vx / 28.0, touchDrag.vy / 28.0)) {
      renderFrame();
      touchDrag.vx *= 0.92;
      touchDrag.vy *= 0.92;
      touchDrag.raf = requestAnimationFrame(stepInertia);
    } else {
      touchDrag.vx = 0; touchDrag.vy = 0;
    }
  };
  canvas.addEventListener("pointermove", (e) => {
    if (!Mod) return;
    const [px, py] = pt(e);
    if (touchDrag.active && e.pointerId === touchDrag.id) {
      const dx = px - touchDrag.lastX;
      const dy = py - touchDrag.lastY;
      touchDrag.lastX = px;
      touchDrag.lastY = py;
      if (Math.hypot(px - touchDrag.startX, py - touchDrag.startY) > 6) {
        if (Mod._meme_web_scroll(px, py, dx / 28.0, dy / 28.0)) {
          touchDrag.scrolled = true;
          touchDrag.vx = touchDrag.vx * 0.55 + dx * 0.45;
          touchDrag.vy = touchDrag.vy * 0.55 + dy * 0.45;
          renderFrame();
          return;
        }
      }
      if (touchDrag.scrolled) return;
    }
    if (Mod._meme_web_pointer_move(px, py)) renderFrame();
  });
  canvas.addEventListener("pointerdown", (e) => {
    if (!Mod) return;
    stopInertia();
    canvas.focus();
    if (canvas.setPointerCapture) { try { canvas.setPointerCapture(e.pointerId); } catch (_) {} }
    const [px, py] = pt(e);
    if (e.pointerType !== "mouse") {
      touchDrag.active = true;
      touchDrag.id = e.pointerId;
      touchDrag.startX = touchDrag.lastX = px;
      touchDrag.startY = touchDrag.lastY = py;
      touchDrag.scrolled = false;
    }
    Mod._meme_web_pointer_down(px, py, e.button, e.detail || 1);
    renderFrame();
  });
  canvas.addEventListener("pointerup", (e) => {
    if (!Mod) return;
    if (canvas.releasePointerCapture) { try { canvas.releasePointerCapture(e.pointerId); } catch (_) {} }
    const [px, py] = pt(e);
    if (touchDrag.active && e.pointerId === touchDrag.id) {
      touchDrag.active = false;
      if (touchDrag.scrolled) {
        Mod._meme_web_pointer_up(-9999, -9999, e.button);
        touchDrag.scrolled = false;
        renderFrame();
        if (Math.hypot(touchDrag.vx, touchDrag.vy) > 1.2) {
          touchDrag.raf = requestAnimationFrame(stepInertia);
        }
        return;
      }
    }
    Mod._meme_web_pointer_up(px, py, e.button);
    renderFrame();
  });
  canvas.addEventListener("pointercancel", (e) => {
    if (!Mod) return;
    if (touchDrag.active && e.pointerId === touchDrag.id) {
      touchDrag.active = false;
      touchDrag.scrolled = false;
      stopInertia();
      Mod._meme_web_pointer_up(-9999, -9999, e.button);
      renderFrame();
      return;
    }
    const [px, py] = pt(e);
    Mod._meme_web_pointer_up(px, py, e.button);
    renderFrame();
  });
  canvas.addEventListener("wheel", (e) => {
    if (!Mod) return;
    e.preventDefault();
    stopInertia();
    const [px, py] = pt(e);
    const unitScale = e.deltaMode === 1 ? 28.0 : (e.deltaMode === 2 ? Math.max(120, canvas.clientHeight) : 1.0);
    if (Mod._meme_web_scroll(px, py, (-e.deltaX * unitScale) / 28.0, (-e.deltaY * unitScale) / 28.0)) renderFrame();
  }, { passive: false });
  canvas.addEventListener("keydown", async (e) => {
    if (!Mod) return;
    if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "v" && navigator.clipboard) {
      try {
        const clip = await navigator.clipboard.readText();
        Mod.ccall("meme_web_set_clipboard", null, ["string"], [clip]);
      } catch (_) {}
    }
    const text = (e.key.length === 1 && !e.ctrlKey && !e.metaKey) ? e.key : "";
    if (e.key === "Tab" || e.key === "Backspace" || e.key === " " || e.key === "Enter" || e.key === "ArrowUp" || e.key === "ArrowDown") e.preventDefault();
    if (Mod.ccall("meme_web_key_event", "number", ["string", "string", "number", "number", "number", "number"],
        [e.key, text, e.shiftKey ? 1 : 0, e.ctrlKey ? 1 : 0, e.altKey ? 1 : 0, e.metaKey ? 1 : 0])) {
      renderFrame();
    }
    if ((e.ctrlKey || e.metaKey) && (e.key.toLowerCase() === "c" || e.key.toLowerCase() === "x") && navigator.clipboard) {
      try {
        const out = Mod.UTF8ToString(Mod._meme_web_get_clipboard());
        if (out) await navigator.clipboard.writeText(out);
      } catch (_) {}
    }
  });
  window.addEventListener("resize", renderFrame);
})();
</script>
</body>
</html>
)HTML";
}

} // namespace MemeWebHost

extern "C" {

MEME_WEB_EXPORT inline void meme_web_render(float vw, float vh) {
    auto *s = MemeWebHost::g_activeSession;
    if (!s) return;
    s->input.viewportW = vw;
    s->input.viewportH = vh;
    if (MemeWebHost::g_rerenderFn && !s->isImmediateMode) {
        MemeWebHost::g_rerenderFn();
    } else {
        s->renderFrame(vw, vh);
    }
}

MEME_WEB_EXPORT inline const void *meme_web_get_mesh_vertices() {
    auto *s = MemeWebHost::g_activeSession;
    return (s && !s->lastMesh.vertices.empty()) ? static_cast<const void *>(s->lastMesh.vertices.data()) : nullptr;
}

MEME_WEB_EXPORT inline int meme_web_get_mesh_vertex_count() {
    auto *s = MemeWebHost::g_activeSession;
    return s ? static_cast<int>(s->lastMesh.vertices.size()) : 0;
}

MEME_WEB_EXPORT inline const uint32_t *meme_web_get_mesh_indices() {
    auto *s = MemeWebHost::g_activeSession;
    return (s && !s->lastMesh.indices.empty()) ? s->lastMesh.indices.data() : nullptr;
}

MEME_WEB_EXPORT inline int meme_web_get_mesh_index_count() {
    auto *s = MemeWebHost::g_activeSession;
    return s ? static_cast<int>(s->lastMesh.indices.size()) : 0;
}

MEME_WEB_EXPORT inline const char *meme_web_get_batches_json() {
    auto *s = MemeWebHost::g_activeSession;
    MemeWebHost::g_lastBatchesJson = s ? MemeWebHost::buildBatchesJson(s->lastMesh) : "[]";
    return MemeWebHost::g_lastBatchesJson.c_str();
}

MEME_WEB_EXPORT inline const uint8_t *meme_web_get_atlas_rgba() {
    if (MemeWebHost::g_atlasRgba.empty()) MemeWebHost::g_atlasRgba = MemeUI::FontAtlas::getRgbaAtlas();
    return MemeWebHost::g_atlasRgba.data();
}

MEME_WEB_EXPORT inline int meme_web_get_atlas_width() {
    return MemeUI::FontAtlas::ATLAS_W;
}

MEME_WEB_EXPORT inline int meme_web_get_atlas_height() {
    return MemeUI::FontAtlas::ATLAS_H;
}

MEME_WEB_EXPORT inline const char *meme_web_get_commands_json() {
    auto *s = MemeWebHost::g_activeSession;
    MemeWebHost::g_lastCommandsJson = s ? MemeUI::gpuDrawCmdsToJson(s->lastCmds) : "[]";
    return MemeWebHost::g_lastCommandsJson.c_str();
}

MEME_WEB_EXPORT inline const char *meme_web_get_hit_regions_json() {
    auto *s = MemeWebHost::g_activeSession;
    MemeWebHost::g_lastHitsJson = s ? MemeUI::hitRegionsToJsonStr(s->lastHits) : "[]";
    return MemeWebHost::g_lastHitsJson.c_str();
}

MEME_WEB_EXPORT inline const char *meme_web_get_agent_snapshot() {
    auto *s = MemeWebHost::g_activeSession;
    MemeWebHost::g_lastAgentJson = s ? s->agentSnapshotJson() : "{}";
    return MemeWebHost::g_lastAgentJson.c_str();
}

MEME_WEB_EXPORT inline int meme_web_pointer_move(float x, float y) {
    auto *s = MemeWebHost::g_activeSession;
    if (!s) return 0;
    bool ch = s->handlePointerMove(x, y);
    if (ch && MemeWebHost::g_rerenderFn) MemeWebHost::g_rerenderFn();
    return ch ? 1 : 0;
}

MEME_WEB_EXPORT inline int meme_web_pointer_down(float x, float y, int btn, int clicks) {
    auto *s = MemeWebHost::g_activeSession;
    if (!s) return 0;
    bool ch = s->handlePointerDown(x, y, btn, clicks);
    if (MemeWebHost::g_rerenderFn) MemeWebHost::g_rerenderFn();
    return ch ? 1 : 0;
}

MEME_WEB_EXPORT inline int meme_web_pointer_up(float x, float y, int btn) {
    auto *s = MemeWebHost::g_activeSession;
    if (!s) return 0;
    bool ch = s->handlePointerUp(x, y, btn);
    if (MemeWebHost::g_rerenderFn) MemeWebHost::g_rerenderFn();
    return ch ? 1 : 0;
}

MEME_WEB_EXPORT inline int meme_web_scroll(float x, float y, float dx, float dy) {
    auto *s = MemeWebHost::g_activeSession;
    if (!s) return 0;
    bool ch = s->handleScroll(x, y, dx, dy);
    if (ch && MemeWebHost::g_rerenderFn) MemeWebHost::g_rerenderFn();
    return ch ? 1 : 0;
}

MEME_WEB_EXPORT inline int meme_web_key_event(const char *key, const char *text, int shift, int ctrl, int alt, int meta) {
    auto *s = MemeWebHost::g_activeSession;
    if (!s) return 0;
    MemeUI::UiKeyEvent kev;
    kev.key = key ? key : "";
    kev.text = text ? text : "";
    kev.shift = shift != 0;
    kev.ctrl = ctrl != 0;
    kev.alt = alt != 0;
    kev.meta = meta != 0;
    bool ch = s->handleKeyEvent(kev);
    if (ch && MemeWebHost::g_rerenderFn) MemeWebHost::g_rerenderFn();
    return ch ? 1 : 0;
}

MEME_WEB_EXPORT inline int meme_web_tick(double deltaMs) {
    auto *s = MemeWebHost::g_activeSession;
    if (!s) return 0;
    bool ch = s->tick(deltaMs) || s->isImmediateMode || s->needsRender;
    if (ch && MemeWebHost::g_rerenderFn) MemeWebHost::g_rerenderFn();
    return ch ? 1 : 0;
}

MEME_WEB_EXPORT inline void meme_web_set_clipboard(const char *txt) {
    MemeWebHost::g_clipboardBuf = txt ? txt : "";
}

MEME_WEB_EXPORT inline const char *meme_web_get_clipboard() {
    return MemeWebHost::g_clipboardBuf.c_str();
}

MEME_WEB_EXPORT inline int meme_web_agent_act(const char *target, const char *action, const char *val) {
    auto *s = MemeWebHost::g_activeSession;
    if (!s || !action) return 0;
    std::string tgt = target ? target : "";
    std::string act = action;
    std::string v = val ? val : "";
    int tid = 0; std::string tkey;
    try { tid = std::stoi(tgt); } catch (...) {}
    if (tid <= 0) {
        for (const auto &hr : s->lastHits) {
            if (hr.nodeKey == tgt || hr.testId == tgt || hr.accessibleName == tgt) { tid = hr.id; tkey = hr.nodeKey; break; }
        }
    } else if (const auto *hr = s->findRegionById(tid)) {
        tkey = hr->nodeKey;
    }
    bool ok = false;
    if (act == "focus") {
        s->interaction.focusedId = tid; s->interaction.focusedKey = tkey; s->interaction.focusVisible = true;
        ok = !tkey.empty();
    } else if (act == "type") {
        if (!tkey.empty()) { s->interaction.focusedId = tid; s->interaction.focusedKey = tkey; }
        for (char c : v) { MemeUI::UiKeyEvent kev; kev.text = kev.key = std::string(1, c); ok = s->handleKeyEvent(kev) || ok; }
    } else if (act == "keydown" || act == "key") {
        if (!tkey.empty()) { s->interaction.focusedId = tid; s->interaction.focusedKey = tkey; }
        MemeUI::UiKeyEvent kev; kev.key = v; if (v.size() == 1) kev.text = v;
        ok = s->handleKeyEvent(kev);
    } else if (tid > 0) {
        double nv = 0.0; try { nv = std::stod(v); } catch (...) {}
        ok = s->dispatchEventOnNode(tid, act, v, nv);
    }
    if (MemeWebHost::g_rerenderFn) MemeWebHost::g_rerenderFn();
    return ok ? 1 : 0;
}

MEME_WEB_EXPORT inline int meme_web_set_route(const char *path) {
    std::string route = path ? path : "/";
    if (MemeWebHost::g_setRouteFn) {
        bool changed = MemeWebHost::g_setRouteFn(route);
        if (changed && MemeWebHost::g_rerenderFn) MemeWebHost::g_rerenderFn();
        return changed ? 1 : 0;
    }
    return 0;
}

} // extern "C"
