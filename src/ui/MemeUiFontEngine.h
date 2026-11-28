#pragma once

#include "MemeUiTypes.h"
#include "MemeUiTextEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wsign-conversion"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "../../third_party/stb/stb_truetype.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace MemeUI {

struct GlyphMetrics {
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 0.0f;
    float v1 = 0.0f;
    float x0Em = 0.0f;
    float y0Em = 0.0f;
    float x1Em = 0.0f;
    float y1Em = 0.0f;
    float advEm = 0.5f;
    bool visible = false;
    bool synthBold = false;
};

namespace FontEngineDetail {

constexpr int ATLAS_W = 1024;
constexpr int ATLAS_H = 1024;
constexpr float RASTER_EM_PX = 36.0f;
constexpr int UV_PAD = 1;

struct LoadedFontFace {
    std::string familyKey;
    std::vector<uint8_t> ttfBytes;
    stbtt_fontinfo info{};
    bool valid = false;
    bool isMono = false;
    bool isBoldFile = false;
    float scaleForEm = 0.0f;
    float ascentEm = 0.78f;
    float descentEm = -0.22f;
};

struct AtlasState {
    std::recursive_mutex mtx;
    std::vector<uint8_t> alpha = std::vector<uint8_t>(ATLAS_W * ATLAS_H, 0);
    std::vector<LoadedFontFace> faces;
    std::unordered_map<std::string, int> familySlotRegular;
    std::unordered_map<std::string, int> familySlotBold;
    std::unordered_map<uint64_t, GlyphMetrics> glyphCache;
    int packX = 18;
    int packY = 2;
    int shelfH = 18;
    uint64_t version = 1;
    bool initialized = false;
};

inline AtlasState &state() {
    static AtlasState s;
    return s;
}

inline std::string normalizeFamilyKey(const std::string &raw) {
    std::string out;
    for (unsigned char c : raw) {
        if (std::isalnum(c) || c == '-' || c == '_') {
            out.push_back(static_cast<char>(std::tolower(c)));
        }
    }
    if (out.empty()) out = "sans";
    if (out == "ui-sans-serif" || out == "system-ui" || out == "inter" || out == "sfpro" || out == "sf-pro") return "sans";
    if (out == "ui-monospace" || out == "monospace" || out == "sfmono" || out == "sf-mono" || out == "jetbrainsmono" || out == "jetbrains-mono") return "mono";
    if (out == "ui-serif" || out == "newyork") return "serif";
    return out;
}

inline bool loadFaceFromBytes(LoadedFontFace &face, const std::string &famKey, std::vector<uint8_t> &&bytes, bool monoHint, bool boldHint, int fontIndex = 0) {
    if (bytes.size() < 64) return false;
    face.familyKey = famKey;
    face.ttfBytes = std::move(bytes);
    face.isMono = monoHint;
    face.isBoldFile = boldHint;
    int offset = stbtt_GetFontOffsetForIndex(face.ttfBytes.data(), fontIndex);
    if (offset < 0) offset = 0;
    if (!stbtt_InitFont(&face.info, face.ttfBytes.data(), offset)) {
        face.valid = false;
        return false;
    }
    int asc = 0, desc = 0, lineGap = 0;
    stbtt_GetFontVMetrics(&face.info, &asc, &desc, &lineGap);
    face.scaleForEm = stbtt_ScaleForPixelHeight(&face.info, RASTER_EM_PX);
    if (asc - desc > 0) {
        face.ascentEm = static_cast<float>(asc) / static_cast<float>(asc - desc);
        face.descentEm = static_cast<float>(desc) / static_cast<float>(asc - desc);
    }
    face.valid = true;
    return true;
}

inline bool tryLoadFileFace(const std::string &famKey, const std::string &path, bool monoHint, bool boldHint, int fontIndex = 0) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || !std::filesystem::is_regular_file(path, ec)) return false;
    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (!ifs.is_open()) return false;
    std::streamsize sz = ifs.tellg();
    if (sz <= 64 || sz > 64 * 1024 * 1024) return false;
    ifs.seekg(0, std::ios::beg);
    std::vector<uint8_t> buf(static_cast<size_t>(sz));
    if (!ifs.read(reinterpret_cast<char *>(buf.data()), sz)) return false;

    LoadedFontFace face;
    if (!loadFaceFromBytes(face, famKey, std::move(buf), monoHint, boldHint, fontIndex)) return false;

    auto &st = state();
    int slot = static_cast<int>(st.faces.size());
    st.faces.push_back(std::move(face));
    if (boldHint) st.familySlotBold[famKey] = slot;
    else st.familySlotRegular[famKey] = slot;
    st.glyphCache.clear();
    st.version++;
    return true;
}

// Vector glyph fallback rasterizer (used when headless Wasm/Linux has no TTF files present)
struct VecSeg { float x1, y1, x2, y2, rMul; };
struct VecArc { float cx, cy, rx, ry, deg0, deg1, rMul; };
struct VecDot { float cx, cy, rMul; };
struct VecGlyph {
    std::vector<VecSeg> segs;
    std::vector<VecArc> arcs;
    std::vector<VecDot> dots;
    void seg(float x1, float y1, float x2, float y2, float rm = 1.0f) { segs.push_back({x1, y1, x2, y2, rm}); }
    void arc(float cx, float cy, float rx, float ry, float a0, float a1, float rm = 1.0f) { arcs.push_back({cx, cy, rx, ry, a0, a1, rm}); }
    void dot(float cx, float cy, float rm = 1.35f) { dots.push_back({cx, cy, rm}); }
};

inline float vecDistSeg(float px, float py, float x1, float y1, float x2, float y2) {
    float vx = x2 - x1, vy = y2 - y1;
    float wx = px - x1, wy = py - y1;
    float len2 = vx * vx + vy * vy;
    float t = (len2 > 1e-6f) ? std::clamp((wx * vx + wy * vy) / len2, 0.0f, 1.0f) : 0.0f;
    float dx = px - (x1 + t * vx), dy = py - (y1 + t * vy);
    return std::sqrt(dx * dx + dy * dy);
}

inline float vecDistArc(float px, float py, float cx, float cy, float rx, float ry, float deg0, float deg1) {
    constexpr int STEPS = 18;
    constexpr float D2R = 3.1415926535f / 180.0f;
    float best = 1e6f;
    float prevX = cx + rx * std::cos(deg0 * D2R);
    float prevY = cy + ry * std::sin(deg0 * D2R);
    for (int s = 1; s <= STEPS; ++s) {
        float t = static_cast<float>(s) / static_cast<float>(STEPS);
        float ang = (deg0 + (deg1 - deg0) * t) * D2R;
        float nx = cx + rx * std::cos(ang);
        float ny = cy + ry * std::sin(ang);
        best = std::min(best, vecDistSeg(px, py, prevX, prevY, nx, ny));
        prevX = nx; prevY = ny;
    }
    return best;
}

inline VecGlyph makeFallbackVectorGlyph(uint32_t cp, float &outW) {
    float advEm = getCharAdvanceFactor(cp, false);
    float w = advEm * (28.0f / 21.0f);
    outW = w;
    VecGlyph g;
    float lx = std::min(0.085f, w * 0.14f);
    float rx = std::max(lx + 0.05f, w - lx);
    float cx = w * 0.5f;
    float xt = 0.28f, xm = 0.64f;
    switch (cp) {
    case '!': g.seg(cx, 0.02f, cx, 0.68f); g.dot(cx, 0.95f, 1.25f); break;
    case '"': g.seg(cx - 0.09f, 0.02f, cx - 0.09f, 0.25f, 0.9f); g.seg(cx + 0.09f, 0.02f, cx + 0.09f, 0.25f, 0.9f); break;
    case '#': {
        float dx = (rx - lx) * 0.14f;
        g.seg(lx + dx * 1.4f, 0.08f, lx + dx * 0.4f, 0.94f, 0.9f);
        g.seg(rx - dx * 0.4f, 0.08f, rx - dx * 1.4f, 0.94f, 0.9f);
        g.seg(lx, 0.38f, rx, 0.38f, 0.9f); g.seg(lx - 0.02f, 0.66f, rx - 0.02f, 0.66f, 0.9f);
        break;
    }
    case '$': {
        float rh = 0.21f, rw = (rx - lx) * 0.46f;
        g.seg(cx, -0.05f, cx, 1.05f, 0.85f);
        g.arc(cx, 0.31f, rw, rh, 165.0f, 350.0f); g.seg(cx - rw * 0.92f, 0.37f, cx + rw * 0.92f, 0.65f);
        g.arc(cx, 0.71f, rw, rh, -15.0f, 165.0f);
        break;
    }
    case '%': {
        float r = (rx - lx) * 0.14f;
        g.arc(lx + r + 0.01f, 0.20f, r, 0.14f, 0.0f, 360.0f, 0.8f);
        g.arc(rx - r - 0.01f, 0.82f, r, 0.14f, 0.0f, 360.0f, 0.8f);
        g.seg(rx - 0.04f, 0.05f, lx + 0.04f, 0.97f, 0.88f);
        break;
    }
    case '&': {
        float rw = (rx - lx) * 0.36f;
        g.arc(cx - 0.04f, 0.24f, rw * 0.75f, 0.18f, 40.0f, 360.0f, 0.95f);
        g.seg(cx - 0.04f + rw * 0.55f, 0.35f, lx + 0.04f, 0.62f, 0.95f);
        g.arc(cx - 0.02f, 0.74f, rw * 0.95f, 0.24f, 35.0f, 220.0f, 0.95f);
        g.seg(cx - 0.04f - rw * 0.4f, 0.35f, rx - 0.02f, 0.98f, 0.95f);
        break;
    }
    case '\'': g.seg(cx, 0.02f, cx, 0.26f, 0.9f); break;
    case '(': g.arc(rx + 0.10f, 0.52f, (rx - lx) * 0.85f, 0.56f, 125.0f, 235.0f, 0.95f); break;
    case ')': g.arc(lx - 0.10f, 0.52f, (rx - lx) * 0.85f, 0.56f, -55.0f, 55.0f, 0.95f); break;
    case '+': g.seg(cx, 0.30f, cx, 0.82f, 0.9f); g.seg(lx, 0.56f, rx, 0.56f, 0.9f); break;
    case ',': g.dot(cx, 0.93f, 1.15f); g.seg(cx + 0.02f, 0.93f, cx - 0.06f, 1.18f, 0.85f); break;
    case '-': g.seg(lx * 0.35f, 0.58f, w - lx * 0.35f, 0.58f, 0.95f); break;
    case '.': g.dot(cx, 0.94f, 1.25f); break;
    case '/': g.seg(rx, 0.04f, lx, 1.02f, 0.9f); break;
    case ':': g.dot(cx, 0.42f, 1.2f); g.dot(cx, 0.94f, 1.2f); break;
    case ';': g.dot(cx, 0.42f, 1.2f); g.dot(cx, 0.93f, 1.15f); g.seg(cx + 0.01f, 0.94f, cx - 0.05f, 1.13f, 0.85f); break;
    case '<': g.seg(rx, 0.26f, lx, 0.56f, 0.9f); g.seg(lx, 0.56f, rx, 0.86f, 0.9f); break;
    case '=': g.seg(lx, 0.43f, rx, 0.43f, 0.9f); g.seg(lx, 0.69f, rx, 0.69f, 0.9f); break;
    case '>': g.seg(lx, 0.26f, rx, 0.56f, 0.9f); g.seg(rx, 0.56f, lx, 0.86f, 0.9f); break;
    case '?': g.arc(cx, 0.24f, (rx - lx) * 0.42f, 0.21f, 185.0f, 415.0f); g.seg(cx + 0.10f, 0.42f, cx, 0.71f); g.dot(cx, 0.95f, 1.25f); break;
    case '0': g.arc(cx, 0.52f, (rx - lx) * 0.48f, 0.48f, 0.0f, 360.0f); break;
    case '1': g.seg(cx - 0.12f, 0.18f, cx + 0.02f, 0.04f); g.seg(cx + 0.02f, 0.04f, cx + 0.02f, 0.98f); g.seg(lx + 0.04f, 0.98f, rx - 0.04f, 0.98f, 0.9f); break;
    case '2': g.arc(cx, 0.27f, (rx - lx) * 0.45f, 0.23f, 185.0f, 365.0f); g.seg(rx - 0.02f, 0.30f, lx, 0.98f); g.seg(lx, 0.98f, rx, 0.98f); break;
    case '3': g.arc(cx, 0.27f, (rx - lx) * 0.44f, 0.23f, 195.0f, 450.0f); g.arc(cx, 0.74f, (rx - lx) * 0.46f, 0.25f, -90.0f, 165.0f); break;
    case '4': g.seg(rx - 0.08f, 0.98f, rx - 0.08f, 0.04f); g.seg(rx - 0.08f, 0.04f, lx, 0.70f); g.seg(lx, 0.70f, rx, 0.70f); break;
    case '5': g.seg(rx - 0.02f, 0.05f, lx + 0.04f, 0.05f); g.seg(lx + 0.04f, 0.05f, lx + 0.02f, 0.46f); g.arc(cx, 0.72f, (rx - lx) * 0.46f, 0.27f, 225.0f, 520.0f); break;
    case '6': g.arc(cx + 0.04f, 0.50f, (rx - lx) * 0.44f, 0.46f, 110.0f, 325.0f); g.arc(cx, 0.73f, (rx - lx) * 0.46f, 0.26f, 0.0f, 360.0f); break;
    case '7': g.seg(lx, 0.05f, rx, 0.05f); g.seg(rx, 0.05f, cx - 0.06f, 0.98f); break;
    case '8': g.arc(cx, 0.27f, (rx - lx) * 0.40f, 0.22f, 0.0f, 360.0f); g.arc(cx, 0.74f, (rx - lx) * 0.46f, 0.25f, 0.0f, 360.0f); break;
    case '9': g.arc(cx, 0.30f, (rx - lx) * 0.46f, 0.25f, 0.0f, 360.0f); g.arc(cx - 0.04f, 0.52f, (rx - lx) * 0.44f, 0.46f, -70.0f, 145.0f); break;
    case 'A': g.seg(lx, 0.98f, cx, 0.04f); g.seg(cx, 0.04f, rx, 0.98f); g.seg(lx + (rx - lx) * 0.19f, 0.66f, rx - (rx - lx) * 0.19f, 0.66f); break;
    case 'B': g.seg(lx, 0.04f, lx, 0.98f); g.seg(lx, 0.04f, cx + 0.04f, 0.04f); g.seg(lx, 0.49f, cx + 0.06f, 0.49f); g.seg(lx, 0.98f, cx + 0.06f, 0.98f); g.arc(cx + 0.04f, 0.265f, (rx - cx) * 0.82f, 0.225f, -90.0f, 90.0f); g.arc(cx + 0.06f, 0.735f, (rx - cx) * 0.88f, 0.245f, -90.0f, 90.0f); break;
    case 'C': g.arc(cx + 0.04f, 0.51f, (rx - lx) * 0.48f, 0.47f, 45.0f, 315.0f); break;
    case 'D': g.seg(lx, 0.04f, lx, 0.98f); g.seg(lx, 0.04f, cx - 0.02f, 0.04f); g.seg(lx, 0.98f, cx - 0.02f, 0.98f); g.arc(cx - 0.02f, 0.51f, rx - cx + 0.02f, 0.47f, -90.0f, 90.0f); break;
    case 'E': g.seg(lx, 0.04f, lx, 0.98f); g.seg(lx, 0.04f, rx, 0.04f); g.seg(lx, 0.49f, rx - 0.06f, 0.49f); g.seg(lx, 0.98f, rx, 0.98f); break;
    case 'F': g.seg(lx, 0.04f, lx, 0.98f); g.seg(lx, 0.04f, rx, 0.04f); g.seg(lx, 0.49f, rx - 0.06f, 0.49f); break;
    case 'G': g.arc(cx + 0.02f, 0.51f, (rx - lx) * 0.48f, 0.47f, 15.0f, 318.0f); g.seg(cx + 0.02f, 0.56f, rx, 0.56f); g.seg(rx, 0.56f, rx, 0.98f); break;
    case 'H': g.seg(lx, 0.04f, lx, 0.98f); g.seg(rx, 0.04f, rx, 0.98f); g.seg(lx, 0.49f, rx, 0.49f); break;
    case 'I': g.seg(cx, 0.04f, cx, 0.98f); g.seg(cx - 0.10f, 0.04f, cx + 0.10f, 0.04f, 0.9f); g.seg(cx - 0.10f, 0.98f, cx + 0.10f, 0.98f, 0.9f); break;
    case 'J': g.seg(rx - 0.04f, 0.04f, rx - 0.04f, 0.74f); g.arc(cx - 0.02f, 0.74f, (rx - lx) * 0.44f, 0.24f, 0.0f, 160.0f); break;
    case 'K': g.seg(lx, 0.04f, lx, 0.98f); g.seg(rx, 0.04f, lx + 0.04f, 0.55f); g.seg(lx + 0.14f, 0.44f, rx, 0.98f); break;
    case 'L': g.seg(lx, 0.04f, lx, 0.98f); g.seg(lx, 0.98f, rx, 0.98f); break;
    case 'M': g.seg(lx, 0.98f, lx, 0.04f); g.seg(lx, 0.04f, cx, 0.76f); g.seg(cx, 0.76f, rx, 0.04f); g.seg(rx, 0.04f, rx, 0.98f); break;
    case 'N': g.seg(lx, 0.98f, lx, 0.04f); g.seg(lx, 0.04f, rx, 0.98f); g.seg(rx, 0.98f, rx, 0.04f); break;
    case 'O': g.arc(cx, 0.51f, (rx - lx) * 0.48f, 0.47f, 0.0f, 360.0f); break;
    case 'P': g.seg(lx, 0.04f, lx, 0.98f); g.seg(lx, 0.04f, cx + 0.04f, 0.04f); g.seg(lx, 0.54f, cx + 0.04f, 0.54f); g.arc(cx + 0.04f, 0.29f, rx - cx - 0.04f, 0.25f, -90.0f, 90.0f); break;
    case 'Q': g.arc(cx, 0.51f, (rx - lx) * 0.48f, 0.47f, 0.0f, 360.0f); g.seg(cx + 0.04f, 0.74f, rx + 0.02f, 1.06f); break;
    case 'R': g.seg(lx, 0.04f, lx, 0.98f); g.seg(lx, 0.04f, cx + 0.04f, 0.04f); g.seg(lx, 0.52f, cx + 0.04f, 0.52f); g.arc(cx + 0.04f, 0.28f, rx - cx - 0.04f, 0.24f, -90.0f, 90.0f); g.seg(cx + 0.02f, 0.52f, rx, 0.98f); break;
    case 'S': { float rh = 0.23f, rw = (rx - lx) * 0.46f; g.arc(cx, 0.27f, rw, rh, 165.0f, 345.0f); g.seg(cx - rw * 0.92f, 0.34f, cx + rw * 0.92f, 0.67f); g.arc(cx, 0.74f, rw, rh, -15.0f, 165.0f); break; }
    case 'T': g.seg(lx, 0.04f, rx, 0.04f); g.seg(cx, 0.04f, cx, 0.98f); break;
    case 'U': g.seg(lx, 0.04f, lx, 0.66f); g.seg(rx, 0.04f, rx, 0.66f); g.arc(cx, 0.66f, (rx - lx) * 0.5f, 0.32f, 0.0f, 180.0f); break;
    case 'V': g.seg(lx, 0.04f, cx, 0.98f); g.seg(cx, 0.98f, rx, 0.04f); break;
    case 'W': { float q1 = lx + (rx - lx) * 0.24f, q3 = rx - (rx - lx) * 0.24f; g.seg(lx, 0.04f, q1, 0.98f); g.seg(q1, 0.98f, cx, 0.30f); g.seg(cx, 0.30f, q3, 0.98f); g.seg(q3, 0.98f, rx, 0.04f); break; }
    case 'X': g.seg(lx, 0.04f, rx, 0.98f); g.seg(rx, 0.04f, lx, 0.98f); break;
    case 'Y': g.seg(lx, 0.04f, cx, 0.52f); g.seg(rx, 0.04f, cx, 0.52f); g.seg(cx, 0.52f, cx, 0.98f); break;
    case 'Z': g.seg(lx, 0.04f, rx, 0.04f); g.seg(rx, 0.04f, lx, 0.98f); g.seg(lx, 0.98f, rx, 0.98f); break;
    case '[': g.seg(cx + 0.06f, 0.00f, cx - 0.06f, 0.00f, 0.9f); g.seg(cx - 0.06f, 0.00f, cx - 0.06f, 1.05f, 0.9f); g.seg(cx - 0.06f, 1.05f, cx + 0.06f, 1.05f, 0.9f); break;
    case ']': g.seg(cx - 0.06f, 0.00f, cx + 0.06f, 0.00f, 0.9f); g.seg(cx + 0.06f, 0.00f, cx + 0.06f, 1.05f, 0.9f); g.seg(cx + 0.06f, 1.05f, cx - 0.06f, 1.05f, 0.9f); break;
    case '_': g.seg(lx * 0.3f, 1.04f, w - lx * 0.3f, 1.04f, 0.9f); break;
    case '`': g.seg(cx - 0.05f, 0.02f, cx + 0.07f, 0.22f, 0.9f); break;
    case 'a': g.arc(cx - 0.02f, 0.72f, (rx - lx) * 0.42f, 0.26f, 0.0f, 360.0f); g.arc(cx - 0.02f, 0.45f, (rx - lx) * 0.40f, 0.16f, 200.0f, 355.0f); g.seg(rx - 0.02f, 0.45f, rx - 0.02f, 0.98f); break;
    case 'b': g.seg(lx, 0.02f, lx, 0.98f); g.arc(cx + 0.02f, xm, (rx - lx) * 0.46f, 0.35f, 0.0f, 360.0f); break;
    case 'c': g.arc(cx + 0.04f, xm, (rx - lx) * 0.46f, 0.35f, 42.0f, 318.0f); break;
    case 'd': g.seg(rx, 0.02f, rx, 0.98f); g.arc(cx - 0.02f, xm, (rx - lx) * 0.46f, 0.35f, 0.0f, 360.0f); break;
    case 'e': g.arc(cx, xm, (rx - lx) * 0.47f, 0.35f, 32.0f, 358.0f); g.seg(lx + 0.02f, xm, rx, xm); break;
    case 'f': g.arc(cx + 0.12f, 0.18f, 0.14f, 0.15f, 180.0f, 345.0f); g.seg(cx - 0.02f, 0.18f, cx - 0.02f, 0.98f); g.seg(lx, 0.36f, rx, 0.36f); break;
    case 'g': g.arc(cx - 0.02f, xm, (rx - lx) * 0.45f, 0.34f, 0.0f, 360.0f); g.seg(rx, xt, rx, 1.00f); g.arc(cx, 1.00f, (rx - lx) * 0.44f, 0.22f, 10.0f, 170.0f); break;
    case 'h': g.seg(lx, 0.02f, lx, 0.98f); g.arc(cx, 0.53f, (rx - lx) * 0.46f, 0.24f, 180.0f, 360.0f); g.seg(rx, 0.53f, rx, 0.98f); break;
    case 'i': g.dot(cx, 0.10f, 1.15f); g.seg(cx, xt + 0.03f, cx, 0.98f); break;
    case 'j': g.dot(cx + 0.02f, 0.10f, 1.15f); g.seg(cx + 0.02f, xt + 0.03f, cx + 0.02f, 1.02f); g.arc(cx - 0.06f, 1.02f, 0.10f, 0.14f, 0.0f, 165.0f); break;
    case 'k': g.seg(lx, 0.02f, lx, 0.98f); g.seg(rx, xt, lx + 0.04f, 0.67f); g.seg(lx + 0.12f, 0.59f, rx, 0.98f); break;
    case 'l': g.seg(cx, 0.02f, cx, 0.96f); g.arc(cx + 0.05f, 0.93f, 0.05f, 0.05f, 90.0f, 180.0f); break;
    case 'm': { float mid = cx, r1 = (mid - lx) * 0.49f; g.seg(lx, xt, lx, 0.98f); g.arc(lx + r1, 0.52f, r1, 0.23f, 180.0f, 360.0f); g.seg(mid, 0.52f, mid, 0.98f); g.arc(mid + r1, 0.52f, r1, 0.23f, 180.0f, 360.0f); g.seg(rx, 0.52f, rx, 0.98f); break; }
    case 'n': g.seg(lx, xt, lx, 0.98f); g.arc(cx, 0.53f, (rx - lx) * 0.46f, 0.24f, 180.0f, 360.0f); g.seg(rx, 0.53f, rx, 0.98f); break;
    case 'o': g.arc(cx, xm, (rx - lx) * 0.47f, 0.35f, 0.0f, 360.0f); break;
    case 'p': g.seg(lx, xt, lx, 1.24f); g.arc(cx + 0.02f, xm, (rx - lx) * 0.46f, 0.35f, 0.0f, 360.0f); break;
    case 'q': g.seg(rx, xt, rx, 1.24f); g.arc(cx - 0.02f, xm, (rx - lx) * 0.46f, 0.35f, 0.0f, 360.0f); break;
    case 'r': g.seg(lx, xt, lx, 0.98f); g.arc(cx + 0.08f, 0.50f, (rx - lx) * 0.46f, 0.21f, 180.0f, 335.0f); break;
    case 's': { float rh = 0.17f, rw = (rx - lx) * 0.43f; g.arc(cx, 0.46f, rw, rh, 165.0f, 345.0f); g.seg(cx - rw * 0.90f, 0.52f, cx + rw * 0.90f, 0.74f); g.arc(cx, 0.81f, rw, rh, -15.0f, 165.0f); break; }
    case 't': g.seg(cx - 0.02f, 0.08f, cx - 0.02f, 0.86f); g.arc(cx + 0.08f, 0.84f, 0.10f, 0.14f, 25.0f, 180.0f); g.seg(lx, xt + 0.04f, rx, xt + 0.04f); break;
    case 'u': g.seg(lx, xt, lx, 0.74f); g.arc(cx, 0.74f, (rx - lx) * 0.46f, 0.24f, 0.0f, 180.0f); g.seg(rx, xt, rx, 0.98f); break;
    case 'v': g.seg(lx, xt, cx, 0.98f); g.seg(cx, 0.98f, rx, xt); break;
    case 'w': { float q1 = lx + (rx - lx) * 0.25f, q3 = rx - (rx - lx) * 0.25f; g.seg(lx, xt, q1, 0.98f); g.seg(q1, 0.98f, cx, 0.48f); g.seg(cx, 0.48f, q3, 0.98f); g.seg(q3, 0.98f, rx, xt); break; }
    case 'x': g.seg(lx, xt, rx, 0.98f); g.seg(rx, xt, lx, 0.98f); break;
    case 'y': g.seg(lx, xt, cx, 0.94f); g.seg(rx, xt, cx - 0.12f, 1.24f); break;
    case 'z': g.seg(lx, xt, rx, xt); g.seg(rx, xt, lx, 0.98f); g.seg(lx, 0.98f, rx, 0.98f); break;
    case '{': g.arc(cx + 0.12f, 0.14f, 0.12f, 0.14f, 180.0f, 270.0f, 0.9f); g.seg(cx, 0.14f, cx, 0.42f, 0.9f); g.seg(cx, 0.42f, lx, 0.52f, 0.9f); g.seg(lx, 0.52f, cx, 0.62f, 0.9f); g.seg(cx, 0.62f, cx, 0.92f, 0.9f); g.arc(cx + 0.12f, 0.92f, 0.12f, 0.14f, 90.0f, 180.0f, 0.9f); break;
    case '}': g.arc(cx - 0.12f, 0.14f, 0.12f, 0.14f, 270.0f, 360.0f, 0.9f); g.seg(cx, 0.14f, cx, 0.42f, 0.9f); g.seg(cx, 0.42f, rx, 0.52f, 0.9f); g.seg(rx, 0.52f, cx, 0.62f, 0.9f); g.seg(cx, 0.62f, cx, 0.92f, 0.9f); g.arc(cx - 0.12f, 0.92f, 0.12f, 0.14f, 0.0f, 90.0f, 0.9f); break;
    case 0x2022: g.dot(cx, 0.60f, 2.6f); break;
    default: g.arc(cx, 0.52f, (rx - lx) * 0.44f, 0.44f, 0.0f, 360.0f, 0.85f); break;
    }
    return g;
}

inline void ensureInitializedLocked() {
    auto &st = state();
    if (st.initialized) return;
    st.initialized = true;
    // Solid white 16x16 block at top-left (0..15, 0..15) for untextured SDF/shape primitives
    for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < 16; ++x) {
            st.alpha[static_cast<size_t>(y) * ATLAS_W + x] = 255;
        }
    }

#if defined(__APPLE__) && !defined(__EMSCRIPTEN__)
    tryLoadFileFace("sans", "/System/Library/Fonts/SFNS.ttf", false, false, 0) ||
        tryLoadFileFace("sans", "/System/Library/Fonts/HelveticaNeue.ttc", false, false, 0);
    tryLoadFileFace("sans", "/System/Library/Fonts/HelveticaNeue.ttc", false, true, 10);
    tryLoadFileFace("mono", "/System/Library/Fonts/SFNSMono.ttf", true, false, 0) ||
        tryLoadFileFace("mono", "/System/Library/Fonts/Menlo.ttc", true, false, 0);
    tryLoadFileFace("mono", "/System/Library/Fonts/Menlo.ttc", true, true, 1);
    tryLoadFileFace("serif", "/System/Library/Fonts/NewYork.ttf", false, false, 0) ||
        tryLoadFileFace("serif", "/System/Library/Fonts/Times.ttc", false, false, 0);
    tryLoadFileFace("rounded", "/System/Library/Fonts/SFNSRounded.ttf", false, false, 0);
#elif defined(_WIN32)
    tryLoadFileFace("sans", "C:\\Windows\\Fonts\\segoeui.ttf", false, false);
    tryLoadFileFace("sans", "C:\\Windows\\Fonts\\segoeuib.ttf", false, true);
    tryLoadFileFace("mono", "C:\\Windows\\Fonts\\consola.ttf", true, false);
    tryLoadFileFace("mono", "C:\\Windows\\Fonts\\consolab.ttf", true, true);
    tryLoadFileFace("serif", "C:\\Windows\\Fonts\\georgia.ttf", false, false);
#elif !defined(__EMSCRIPTEN__)
    // Linux & Android standard TTF paths
    tryLoadFileFace("sans", "/system/fonts/Roboto-Regular.ttf", false, false) ||
        tryLoadFileFace("sans", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", false, false) ||
        tryLoadFileFace("sans", "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", false, false);
    tryLoadFileFace("sans", "/system/fonts/Roboto-Bold.ttf", false, true) ||
        tryLoadFileFace("sans", "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", false, true);
    tryLoadFileFace("mono", "/system/fonts/DroidSansMono.ttf", true, false) ||
        tryLoadFileFace("mono", "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", true, false);
    tryLoadFileFace("serif", "/system/fonts/NotoSerif-Regular.ttf", false, false) ||
        tryLoadFileFace("serif", "/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf", false, false);
#endif
}

inline int resolveFaceSlotLocked(FontFamily family, const std::string &fontName, int fontWeight, bool &outNeedSynthBold) {
    auto &st = state();
    ensureInitializedLocked();
    outNeedSynthBold = false;
    bool wantsBold = (fontWeight >= 600);
    std::string famKey;
    if (!fontName.empty()) {
        famKey = normalizeFamilyKey(fontName);
    } else if (family == FontFamily::MONO) {
        famKey = "mono";
    } else if (family == FontFamily::SERIF) {
        famKey = "serif";
    } else {
        famKey = "sans";
    }

    if (wantsBold) {
        auto bit = st.familySlotBold.find(famKey);
        if (bit != st.familySlotBold.end()) return bit->second;
    }
    auto rit = st.familySlotRegular.find(famKey);
    if (rit != st.familySlotRegular.end()) {
        outNeedSynthBold = wantsBold;
        return rit->second;
    }
    if (famKey != "sans") {
        if (wantsBold) {
            auto bit = st.familySlotBold.find("sans");
            if (bit != st.familySlotBold.end()) return bit->second;
        }
        auto sit = st.familySlotRegular.find("sans");
        if (sit != st.familySlotRegular.end()) {
            outNeedSynthBold = wantsBold;
            return sit->second;
        }
    }
    outNeedSynthBold = wantsBold;
    return -1; // vector fallback
}

inline GlyphMetrics packFallbackVectorGlyphLocked(uint32_t cp, bool isMono, bool synthBold) {
    auto &st = state();
    constexpr float CAP_PX = 27.0f;
    constexpr int CELL_H = 44;
    constexpr float BASE_Y = 33.0f;
    float gwNorm = 0.0f;
    VecGlyph vg = makeFallbackVectorGlyph(cp, gwNorm);
    float spanPx = std::max(4.0f, gwNorm * CAP_PX);
    int gw = std::clamp(static_cast<int>(std::ceil(spanPx)) + 6, 8, 44);
    int gh = CELL_H;
    if (st.packX + gw + 2 > ATLAS_W) {
        st.packX = 2;
        st.packY += st.shelfH + 2;
        st.shelfH = gh;
    }
    if (st.packY + gh + 2 > ATLAS_H) {
        return {};
    }
    int ox = st.packX + 2;
    int oy = st.packY + 2;
    st.packX += gw + 2;
    st.shelfH = std::max(st.shelfH, gh);

    float strokeHalf = 1.38f;
    for (int py = 0; py < gh - 4; ++py) {
        for (int px = 0; px < gw - 4; ++px) {
            float accum = 0.0f;
            for (int sy = 0; sy < 3; ++sy) {
                for (int sx = 0; sx < 3; ++sx) {
                    float fx = ((px + (sx + 0.5f) / 3.0f) - 1.0f) / CAP_PX;
                    float fy = ((py + (sy + 0.5f) / 3.0f) - (BASE_Y - CAP_PX)) / CAP_PX;
                    float minEdge = 1e6f;
                    for (const auto &s : vg.segs) {
                        minEdge = std::min(minEdge, vecDistSeg(fx, fy, s.x1, s.y1, s.x2, s.y2) * CAP_PX - strokeHalf * s.rMul);
                    }
                    for (const auto &a : vg.arcs) {
                        minEdge = std::min(minEdge, vecDistArc(fx, fy, a.cx, a.cy, a.rx, a.ry, a.deg0, a.deg1) * CAP_PX - strokeHalf * a.rMul);
                    }
                    for (const auto &d : vg.dots) {
                        float dd = std::hypot(fx - d.cx, fy - d.cy) * CAP_PX;
                        minEdge = std::min(minEdge, dd - strokeHalf * d.rMul);
                    }
                    accum += std::clamp(0.55f - minEdge, 0.0f, 1.0f);
                }
            }
            uint8_t a8 = static_cast<uint8_t>(std::clamp((accum / 9.0f) * 255.0f, 0.0f, 255.0f));
            st.alpha[static_cast<size_t>(oy + py) * ATLAS_W + (ox + px)] = a8;
        }
    }
    st.version++;

    GlyphMetrics m;
    m.visible = (cp != ' ');
    m.advEm = getCharAdvanceFactor(cp, isMono);
    m.x0Em = -2.0f / RASTER_EM_PX;
    m.x1Em = (gw - 4.0f) / RASTER_EM_PX;
    m.y0Em = -(BASE_Y - 2.0f) / RASTER_EM_PX;
    m.y1Em = (gh - 4.0f - BASE_Y + 2.0f) / RASTER_EM_PX;
    m.u0 = static_cast<float>(ox) / ATLAS_W;
    m.v0 = static_cast<float>(oy) / ATLAS_H;
    m.u1 = static_cast<float>(ox + gw - 4) / ATLAS_W;
    m.v1 = static_cast<float>(oy + gh - 4) / ATLAS_H;
    m.synthBold = synthBold;
    return m;
}

inline GlyphMetrics getOrPackGlyphLocked(int faceSlot, uint32_t cp, bool isMono, bool synthBold) {
    auto &st = state();
    int effectiveSlot = (faceSlot >= 0 && faceSlot < static_cast<int>(st.faces.size()) && st.faces[faceSlot].valid) ? faceSlot : -1;
    uint64_t cacheKey = (static_cast<uint64_t>(effectiveSlot + 1) << 40) |
                        (static_cast<uint64_t>(isMono ? 1 : 0) << 39) |
                        (static_cast<uint64_t>(synthBold ? 1 : 0) << 38) |
                        static_cast<uint64_t>(cp);
    auto cit = st.glyphCache.find(cacheKey);
    if (cit != st.glyphCache.end()) return cit->second;

    if (effectiveSlot < 0) {
        GlyphMetrics gm = packFallbackVectorGlyphLocked(cp, isMono, synthBold);
        st.glyphCache[cacheKey] = gm;
        return gm;
    }

    const auto &face = st.faces[effectiveSlot];
    int glyphIndex = stbtt_FindGlyphIndex(&face.info, static_cast<int>(cp));
    if (glyphIndex == 0 && cp != ' ') {
        glyphIndex = stbtt_FindGlyphIndex(&face.info, '?');
    }

    int advW = 0, lsb = 0;
    stbtt_GetGlyphHMetrics(&face.info, glyphIndex, &advW, &lsb);
    float advEm = (static_cast<float>(advW) * face.scaleForEm) / RASTER_EM_PX;
    if (isMono && advEm <= 0.01f) advEm = 0.56f;

    GlyphMetrics gm;
    gm.advEm = advEm;
    gm.synthBold = synthBold;
    if (cp == ' ' || glyphIndex == 0) {
        gm.visible = false;
        st.glyphCache[cacheKey] = gm;
        return gm;
    }

    int ix0 = 0, iy0 = 0, ix1 = 0, iy1 = 0;
    stbtt_GetGlyphBitmapBox(&face.info, glyphIndex, face.scaleForEm, face.scaleForEm, &ix0, &iy0, &ix1, &iy1);
    int bw = ix1 - ix0;
    int bh = iy1 - iy0;
    if (bw <= 0 || bh <= 0) {
        gm.visible = false;
        st.glyphCache[cacheKey] = gm;
        return gm;
    }

    std::vector<uint8_t> bmp(static_cast<size_t>(bw) * bh, 0);
    stbtt_MakeGlyphBitmap(&face.info, bmp.data(), bw, bh, bw, face.scaleForEm, face.scaleForEm, glyphIndex);

    int cellW = bw + UV_PAD * 2;
    int cellH = bh + UV_PAD * 2;
    if (st.packX + cellW + 2 > ATLAS_W) {
        st.packX = 2;
        st.packY += st.shelfH + 2;
        st.shelfH = cellH;
    }
    if (st.packY + cellH + 2 > ATLAS_H) {
        // Atlas full: gracefully skip overflowing glyph without crashing
        gm.visible = false;
        st.glyphCache[cacheKey] = gm;
        return gm;
    }
    int ox = st.packX + UV_PAD;
    int oy = st.packY + UV_PAD;
    st.packX += cellW + 1;
    st.shelfH = std::max(st.shelfH, cellH);

    for (int py = 0; py < bh; ++py) {
        for (int px = 0; px < bw; ++px) {
            st.alpha[static_cast<size_t>(oy + py) * ATLAS_W + (ox + px)] = bmp[static_cast<size_t>(py) * bw + px];
        }
    }
    st.version++;

    gm.visible = true;
    gm.x0Em = static_cast<float>(ix0) / RASTER_EM_PX;
    gm.y0Em = static_cast<float>(iy0) / RASTER_EM_PX;
    gm.x1Em = static_cast<float>(ix1) / RASTER_EM_PX;
    gm.y1Em = static_cast<float>(iy1) / RASTER_EM_PX;
    gm.u0 = static_cast<float>(ox) / static_cast<float>(ATLAS_W);
    gm.v0 = static_cast<float>(oy) / static_cast<float>(ATLAS_H);
    gm.u1 = static_cast<float>(ox + bw) / static_cast<float>(ATLAS_W);
    gm.v1 = static_cast<float>(oy + bh) / static_cast<float>(ATLAS_H);
    st.glyphCache[cacheKey] = gm;
    return gm;
}

} // namespace FontEngineDetail

namespace MemeUiFonts {

constexpr int ATLAS_W = FontEngineDetail::ATLAS_W;
constexpr int ATLAS_H = FontEngineDetail::ATLAS_H;

inline uint64_t getAtlasVersion() {
    auto &st = FontEngineDetail::state();
    std::lock_guard<std::recursive_mutex> lk(st.mtx);
    FontEngineDetail::ensureInitializedLocked();
    return st.version;
}

inline std::vector<uint8_t> getAlphaAtlas() {
    auto &st = FontEngineDetail::state();
    std::lock_guard<std::recursive_mutex> lk(st.mtx);
    FontEngineDetail::ensureInitializedLocked();
    return st.alpha;
}

inline std::vector<uint8_t> getRgbaAtlas() {
    auto &st = FontEngineDetail::state();
    std::lock_guard<std::recursive_mutex> lk(st.mtx);
    FontEngineDetail::ensureInitializedLocked();
    std::vector<uint8_t> rgba(ATLAS_W * ATLAS_H * 4, 255);
    for (size_t i = 0; i < st.alpha.size(); ++i) {
        rgba[i * 4 + 3] = st.alpha[i];
    }
    return rgba;
}

inline bool registerFontFile(const std::string &familyName, const std::string &filePath, const std::string &weight = "400") {
    auto &st = FontEngineDetail::state();
    std::lock_guard<std::recursive_mutex> lk(st.mtx);
    FontEngineDetail::ensureInitializedLocked();
    std::string famKey = FontEngineDetail::normalizeFamilyKey(familyName);
    bool isBold = (weight == "bold" || weight == "700" || weight == "800" || weight == "600");
    bool isMono = (famKey == "mono");
    return FontEngineDetail::tryLoadFileFace(famKey, filePath, isMono, isBold);
}

inline bool registerFontBytes(const std::string &familyName, const std::vector<uint8_t> &bytes, bool isBold = false) {
    auto &st = FontEngineDetail::state();
    std::lock_guard<std::recursive_mutex> lk(st.mtx);
    FontEngineDetail::ensureInitializedLocked();
    std::string famKey = FontEngineDetail::normalizeFamilyKey(familyName);
    FontEngineDetail::LoadedFontFace face;
    std::vector<uint8_t> copy = bytes;
    if (!FontEngineDetail::loadFaceFromBytes(face, famKey, std::move(copy), famKey == "mono", isBold)) return false;
    int slot = static_cast<int>(st.faces.size());
    st.faces.push_back(std::move(face));
    if (isBold) st.familySlotBold[famKey] = slot;
    else st.familySlotRegular[famKey] = slot;
    st.glyphCache.clear();
    st.version++;
    return true;
}

inline GlyphMetrics acquireGlyph(uint32_t codepoint, FontFamily family, const std::string &fontName = "", int fontWeight = 400) {
    auto &st = FontEngineDetail::state();
    std::lock_guard<std::recursive_mutex> lk(st.mtx);
    bool needSynthBold = false;
    int slot = FontEngineDetail::resolveFaceSlotLocked(family, fontName, fontWeight, needSynthBold);
    return FontEngineDetail::getOrPackGlyphLocked(slot, codepoint, family == FontFamily::MONO, needSynthBold);
}

inline float measureCharAdvance(uint32_t cp, uint32_t prevCp, float fontSize, FontFamily family, const std::string &fontName = "", int fontWeight = 400) {
    auto &st = FontEngineDetail::state();
    std::lock_guard<std::recursive_mutex> lk(st.mtx);
    bool needSynthBold = false;
    int slot = FontEngineDetail::resolveFaceSlotLocked(family, fontName, fontWeight, needSynthBold);
    GlyphMetrics gm = FontEngineDetail::getOrPackGlyphLocked(slot, cp, family == FontFamily::MONO, needSynthBold);
    float kern = 0.0f;
    if (slot >= 0 && prevCp != 0 && cp != 0 && slot < static_cast<int>(st.faces.size()) && st.faces[slot].valid) {
        const auto &face = st.faces[slot];
        int k = stbtt_GetCodepointKernAdvance(&face.info, static_cast<int>(prevCp), static_cast<int>(cp));
        if (k != 0) {
            kern = ((static_cast<float>(k) * face.scaleForEm) / FontEngineDetail::RASTER_EM_PX) * fontSize;
        }
    }
    return gm.advEm * fontSize + kern;
}

inline float measureTextWidth(const std::string &text, float fontSize, FontFamily family = FontFamily::SANS, const std::string &fontName = "", int fontWeight = 400) {
    if (text.empty()) return 0.0f;
    auto &st = FontEngineDetail::state();
    std::lock_guard<std::recursive_mutex> lk(st.mtx);
    bool needSynthBold = false;
    int slot = FontEngineDetail::resolveFaceSlotLocked(family, fontName, fontWeight, needSynthBold);
    float w = 0.0f;
    uint32_t prev = 0;
    for (size_t i = 0; i < text.size();) {
        size_t next = i;
        uint32_t cp = decodeGlyphCodepoint(text, i, next);
        if (cp != 0) {
            GlyphMetrics gm = FontEngineDetail::getOrPackGlyphLocked(slot, cp, family == FontFamily::MONO, needSynthBold);
            w += gm.advEm * fontSize;
            if (prev != 0 && slot >= 0 && slot < static_cast<int>(st.faces.size()) && st.faces[slot].valid) {
                const auto &face = st.faces[slot];
                int k = stbtt_GetCodepointKernAdvance(&face.info, static_cast<int>(prev), static_cast<int>(cp));
                if (k != 0) {
                    w += ((static_cast<float>(k) * face.scaleForEm) / FontEngineDetail::RASTER_EM_PX) * fontSize;
                }
            }
            prev = cp;
        }
        i = next;
    }
    return w;
}

inline float getBaselineOffsetRatio(FontFamily family = FontFamily::SANS, const std::string &fontName = "") {
    auto &st = FontEngineDetail::state();
    std::lock_guard<std::recursive_mutex> lk(st.mtx);
    bool sb = false;
    int slot = FontEngineDetail::resolveFaceSlotLocked(family, fontName, 400, sb);
    if (slot >= 0 && slot < static_cast<int>(st.faces.size()) && st.faces[slot].valid) {
        return st.faces[slot].ascentEm;
    }
    return 0.78f;
}

inline float measureTextHookImpl(const std::string &text, float fontSize, bool isMono, const std::string &fontName, int fontWeight) {
    return measureTextWidth(text, fontSize, isMono ? FontFamily::MONO : FontFamily::SANS, fontName, fontWeight);
}

struct FontHookRegistrar {
    FontHookRegistrar() {
        customMeasureTextHook() = &measureTextHookImpl;
    }
};

inline FontHookRegistrar g_fontHookRegistrar;

} // namespace MemeUiFonts

} // namespace MemeUI
