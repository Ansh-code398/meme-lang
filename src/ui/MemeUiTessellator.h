#pragma once

#include "MemeUiTypes.h"
#include "MemeUiTextEngine.h"
#include <vector>
#include <array>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <sstream>

namespace MemeUI {

struct UiVertex {
    float x = 0.0f;
    float y = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
    uint32_t rgba = 0xFFFFFFFFu;
};

struct UiClipBatch {
    UiRect clipRect = {0, 0, 1e6f, 1e6f};
    uint32_t indexOffset = 0;
    uint32_t indexCount = 0;
};

struct UiMesh {
    std::vector<UiVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<UiClipBatch> batches;

    void clear() {
        vertices.clear();
        indices.clear();
        batches.clear();
    }
};

// High-resolution 512x512 4x-oversampled analytical anti-aliased vector glyph atlas.
// Pixel (0..15, 0..15) in Slot 0 is solid 255 for untextured solid geometry under LINEAR filtering.
namespace FontAtlas {
    constexpr int ATLAS_W = 512;
    constexpr int ATLAS_H = 512;
    constexpr int CELL_W = 32;
    constexpr int CELL_H = 44;
    constexpr int COLS = 16;

    constexpr float EM_PX = 28.0f;
    constexpr float CAP_PX = 21.0f;
    constexpr float PAD_X = 4.0f;
    constexpr float CAP_TOP_PX = 10.0f;
    constexpr float BASE_PX = CAP_TOP_PX + CAP_PX; // 31.0f inside 44px cell
    constexpr float UV_PAD_PX = 2.0f;

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

    inline float distToSegment(float px, float py, float x1, float y1, float x2, float y2) {
        float vx = x2 - x1, vy = y2 - y1;
        float wx = px - x1, wy = py - y1;
        float len2 = vx * vx + vy * vy;
        float t = (len2 > 1e-6f) ? std::clamp((wx * vx + wy * vy) / len2, 0.0f, 1.0f) : 0.0f;
        float dx = px - (x1 + t * vx), dy = py - (y1 + t * vy);
        return std::sqrt(dx * dx + dy * dy);
    }

    inline float distToArc(float px, float py, float cx, float cy, float rx, float ry, float deg0, float deg1) {
        constexpr int STEPS = 20;
        constexpr float DEG2RAD = 3.1415926535f / 180.0f;
        float best = 1e6f;
        float prevX = cx + rx * std::cos(deg0 * DEG2RAD);
        float prevY = cy + ry * std::sin(deg0 * DEG2RAD);
        for (int s = 1; s <= STEPS; ++s) {
            float t = static_cast<float>(s) / static_cast<float>(STEPS);
            float ang = (deg0 + (deg1 - deg0) * t) * DEG2RAD;
            float nx = cx + rx * std::cos(ang);
            float ny = cy + ry * std::sin(ang);
            best = std::min(best, distToSegment(px, py, prevX, prevY, nx, ny));
            prevX = nx;
            prevY = ny;
        }
        return best;
    }

    inline VecGlyph buildVectorGlyph(uint32_t cp, float &outW) {
        float advEm = getCharAdvanceFactor(cp, false);
        float w = advEm * (EM_PX / CAP_PX);
        outW = w;
        VecGlyph g;
        float lx = std::min(0.085f, w * 0.14f);
        float rx = std::max(lx + 0.05f, w - lx);
        float cx = w * 0.5f;
        float xt = 0.28f;  // x-height top
        float xm = 0.64f;  // x-height mid

        switch (cp) {
        case '!': g.seg(cx, 0.02f, cx, 0.68f); g.dot(cx, 0.95f, 1.25f); break;
        case '"': g.seg(cx - 0.09f, 0.02f, cx - 0.09f, 0.25f, 0.9f); g.seg(cx + 0.09f, 0.02f, cx + 0.09f, 0.25f, 0.9f); break;
        case '#': {
            float dx = (rx - lx) * 0.14f;
            g.seg(lx + dx * 1.4f, 0.08f, lx + dx * 0.4f, 0.94f, 0.9f);
            g.seg(rx - dx * 0.4f, 0.08f, rx - dx * 1.4f, 0.94f, 0.9f);
            g.seg(lx, 0.38f, rx, 0.38f, 0.9f);
            g.seg(lx - 0.02f, 0.66f, rx - 0.02f, 0.66f, 0.9f);
            break;
        }
        case '$': {
            float rh = 0.21f;
            float rw = (rx - lx) * 0.46f;
            g.seg(cx, -0.05f, cx, 1.05f, 0.85f);
            g.arc(cx, 0.31f, rw, rh, 165.0f, 350.0f);
            g.seg(cx - rw * 0.92f, 0.37f, cx + rw * 0.92f, 0.65f);
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
        case '(':  g.arc(rx + 0.10f, 0.52f, (rx - lx) * 0.85f, 0.56f, 125.0f, 235.0f, 0.95f); break;
        case ')':  g.arc(lx - 0.10f, 0.52f, (rx - lx) * 0.85f, 0.56f, -55.0f, 55.0f, 0.95f); break;
        case '*': {
            float yc = 0.32f, rr = (rx - lx) * 0.44f;
            g.seg(cx, yc - rr, cx, yc + rr, 0.85f);
            g.seg(cx - rr * 0.87f, yc - rr * 0.5f, cx + rr * 0.87f, yc + rr * 0.5f, 0.85f);
            g.seg(cx - rr * 0.87f, yc + rr * 0.5f, cx + rr * 0.87f, yc - rr * 0.5f, 0.85f);
            break;
        }
        case '+':  g.seg(cx, 0.30f, cx, 0.82f, 0.9f); g.seg(lx, 0.56f, rx, 0.56f, 0.9f); break;
        case ',':  g.dot(cx, 0.93f, 1.15f); g.seg(cx + 0.02f, 0.93f, cx - 0.06f, 1.18f, 0.85f); break;
        case '-':  g.seg(lx * 0.35f, 0.58f, w - lx * 0.35f, 0.58f, 0.95f); break;
        case '.':  g.dot(cx, 0.94f, 1.25f); break;
        case '/':  g.seg(rx, 0.04f, lx, 1.02f, 0.9f); break;
        case '0':  g.arc(cx, 0.52f, (rx - lx) * 0.48f, 0.48f, 0.0f, 360.0f); break;
        case '1':  g.seg(cx - 0.12f, 0.18f, cx + 0.02f, 0.04f); g.seg(cx + 0.02f, 0.04f, cx + 0.02f, 0.98f); g.seg(lx + 0.04f, 0.98f, rx - 0.04f, 0.98f, 0.9f); break;
        case '2':  g.arc(cx, 0.27f, (rx - lx) * 0.45f, 0.23f, 185.0f, 365.0f); g.seg(rx - 0.02f, 0.30f, lx, 0.98f); g.seg(lx, 0.98f, rx, 0.98f); break;
        case '3':  g.arc(cx, 0.27f, (rx - lx) * 0.44f, 0.23f, 195.0f, 450.0f); g.arc(cx, 0.74f, (rx - lx) * 0.46f, 0.25f, -90.0f, 165.0f); break;
        case '4':  g.seg(rx - 0.08f, 0.98f, rx - 0.08f, 0.04f); g.seg(rx - 0.08f, 0.04f, lx, 0.70f); g.seg(lx, 0.70f, rx, 0.70f); break;
        case '5':  g.seg(rx - 0.02f, 0.05f, lx + 0.04f, 0.05f); g.seg(lx + 0.04f, 0.05f, lx + 0.02f, 0.46f); g.arc(cx, 0.72f, (rx - lx) * 0.46f, 0.27f, 225.0f, 520.0f); break;
        case '6':  g.arc(cx + 0.04f, 0.50f, (rx - lx) * 0.44f, 0.46f, 110.0f, 325.0f); g.arc(cx, 0.73f, (rx - lx) * 0.46f, 0.26f, 0.0f, 360.0f); break;
        case '7':  g.seg(lx, 0.05f, rx, 0.05f); g.seg(rx, 0.05f, cx - 0.06f, 0.98f); break;
        case '8':  g.arc(cx, 0.27f, (rx - lx) * 0.40f, 0.22f, 0.0f, 360.0f); g.arc(cx, 0.74f, (rx - lx) * 0.46f, 0.25f, 0.0f, 360.0f); break;
        case '9':  g.arc(cx, 0.30f, (rx - lx) * 0.46f, 0.25f, 0.0f, 360.0f); g.arc(cx - 0.04f, 0.52f, (rx - lx) * 0.44f, 0.46f, -70.0f, 145.0f); break;
        case ':':  g.dot(cx, 0.42f, 1.2f); g.dot(cx, 0.94f, 1.2f); break;
        case ';':  g.dot(cx, 0.42f, 1.2f); g.dot(cx, 0.93f, 1.15f); g.seg(cx + 0.01f, 0.94f, cx - 0.05f, 1.13f, 0.85f); break;
        case '<':  g.seg(rx, 0.26f, lx, 0.56f, 0.9f); g.seg(lx, 0.56f, rx, 0.86f, 0.9f); break;
        case '=':  g.seg(lx, 0.43f, rx, 0.43f, 0.9f); g.seg(lx, 0.69f, rx, 0.69f, 0.9f); break;
        case '>':  g.seg(lx, 0.26f, rx, 0.56f, 0.9f); g.seg(rx, 0.56f, lx, 0.86f, 0.9f); break;
        case '?':  g.arc(cx, 0.24f, (rx - lx) * 0.42f, 0.21f, 185.0f, 415.0f); g.seg(cx + 0.10f, 0.42f, cx, 0.56f); g.seg(cx, 0.56f, cx, 0.71f); g.dot(cx, 0.95f, 1.25f); break;
        case '@': {
            float rIn = (rx - lx) * 0.22f;
            g.arc(cx, 0.56f, rIn, 0.21f, 0.0f, 360.0f, 0.85f);
            g.seg(cx + rIn, 0.35f, cx + rIn, 0.72f, 0.85f);
            g.arc(cx, 0.54f, (rx - lx) * 0.48f, 0.46f, 55.0f, 385.0f, 0.88f);
            break;
        }
        case 'A': g.seg(lx, 0.98f, cx, 0.04f); g.seg(cx, 0.04f, rx, 0.98f); g.seg(lx + (rx-lx)*0.19f, 0.66f, rx - (rx-lx)*0.19f, 0.66f); break;
        case 'B': g.seg(lx, 0.04f, lx, 0.98f); g.seg(lx, 0.04f, cx + 0.04f, 0.04f); g.seg(lx, 0.49f, cx + 0.06f, 0.49f); g.seg(lx, 0.98f, cx + 0.06f, 0.98f); g.arc(cx + 0.04f, 0.265f, (rx-cx)*0.82f, 0.225f, -90.0f, 90.0f); g.arc(cx + 0.06f, 0.735f, (rx-cx)*0.88f, 0.245f, -90.0f, 90.0f); break;
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
        case 'W': { float q = (rx - lx) * 0.25f; g.seg(lx, 0.04f, lx + q, 0.98f); g.seg(lx + q, 0.98f, cx, 0.28f); g.seg(cx, 0.28f, rx - q, 0.98f); g.seg(rx - q, 0.98f, rx, 0.04f); break; }
        case 'X': g.seg(lx, 0.04f, rx, 0.98f); g.seg(rx, 0.04f, lx, 0.98f); break;
        case 'Y': g.seg(lx, 0.04f, cx, 0.54f); g.seg(rx, 0.04f, cx, 0.54f); g.seg(cx, 0.54f, cx, 0.98f); break;
        case 'Z': g.seg(lx, 0.04f, rx, 0.04f); g.seg(rx, 0.04f, lx, 0.98f); g.seg(lx, 0.98f, rx, 0.98f); break;
        case '[': g.seg(cx + 0.08f, 0.02f, cx - 0.06f, 0.02f, 0.9f); g.seg(cx - 0.06f, 0.02f, cx - 0.06f, 1.06f, 0.9f); g.seg(cx - 0.06f, 1.06f, cx + 0.08f, 1.06f, 0.9f); break;
        case '\\':g.seg(lx, 0.04f, rx, 1.02f, 0.9f); break;
        case ']': g.seg(cx - 0.08f, 0.02f, cx + 0.06f, 0.02f, 0.9f); g.seg(cx + 0.06f, 0.02f, cx + 0.06f, 1.06f, 0.9f); g.seg(cx + 0.06f, 1.06f, cx - 0.08f, 1.06f, 0.9f); break;
        case '^': g.seg(lx, 0.36f, cx, 0.08f, 0.9f); g.seg(cx, 0.08f, rx, 0.36f, 0.9f); break;
        case '_': g.seg(lx - 0.02f, 1.08f, rx + 0.02f, 1.08f, 0.9f); break;
        case '`': g.seg(cx - 0.06f, 0.04f, cx + 0.06f, 0.22f, 0.85f); break;
        case 'a': g.arc(cx - 0.02f, xm, (rx - lx) * 0.44f, 0.34f, 0.0f, 360.0f); g.seg(rx - 0.01f, xt + 0.02f, rx - 0.01f, 0.98f); break;
        case 'b': g.seg(lx, 0.02f, lx, 0.98f); g.arc(cx + 0.02f, xm, (rx - lx) * 0.45f, 0.34f, 0.0f, 360.0f); break;
        case 'c': g.arc(cx + 0.04f, xm, (rx - lx) * 0.46f, 0.35f, 45.0f, 315.0f); break;
        case 'd': g.seg(rx, 0.02f, rx, 0.98f); g.arc(cx - 0.02f, xm, (rx - lx) * 0.45f, 0.34f, 0.0f, 360.0f); break;
        case 'e': g.arc(cx, xm, (rx - lx) * 0.47f, 0.35f, 38.0f, 360.0f); g.seg(lx + 0.02f, xm, rx, xm); break;
        case 'f': g.seg(cx - 0.02f, 0.22f, cx - 0.02f, 0.98f); g.arc(cx + 0.07f, 0.22f, 0.09f, 0.18f, 180.0f, 330.0f); g.seg(lx, 0.38f, rx, 0.38f); break;
        case 'g': g.arc(cx - 0.02f, xm, (rx - lx) * 0.44f, 0.34f, 0.0f, 360.0f); g.seg(rx - 0.01f, xt + 0.02f, rx - 0.01f, 1.06f); g.arc(cx - 0.02f, 1.06f, (rx - lx) * 0.44f, 0.22f, 0.0f, 155.0f); break;
        case 'h': g.seg(lx, 0.02f, lx, 0.98f); g.arc(cx, 0.54f, (rx - lx) * 0.48f, 0.24f, 180.0f, 360.0f); g.seg(rx, 0.54f, rx, 0.98f); break;
        case 'i': g.dot(cx, 0.06f, 1.25f); g.seg(cx, 0.30f, cx, 0.98f); break;
        case 'j': g.dot(cx + 0.03f, 0.06f, 1.25f); g.seg(cx + 0.03f, 0.30f, cx + 0.03f, 1.06f); g.arc(cx - 0.08f, 1.06f, 0.11f, 0.20f, 0.0f, 150.0f); break;
        case 'k': g.seg(lx, 0.02f, lx, 0.98f); g.seg(rx, 0.30f, lx + 0.03f, 0.68f); g.seg(lx + 0.14f, 0.56f, rx, 0.98f); break;
        case 'l': g.seg(cx, 0.02f, cx, 0.98f); break;
        case 'm': { float mx = (rx - lx) * 0.25f; g.seg(lx, 0.30f, lx, 0.98f); g.arc(lx + mx, 0.52f, mx, 0.22f, 180.0f, 360.0f); g.seg(cx, 0.52f, cx, 0.98f); g.arc(rx - mx, 0.52f, mx, 0.22f, 180.0f, 360.0f); g.seg(rx, 0.52f, rx, 0.98f); break; }
        case 'n': g.seg(lx, 0.30f, lx, 0.98f); g.arc(cx, 0.54f, (rx - lx) * 0.48f, 0.24f, 180.0f, 360.0f); g.seg(rx, 0.54f, rx, 0.98f); break;
        case 'o': g.arc(cx, xm, (rx - lx) * 0.47f, 0.35f, 0.0f, 360.0f); break;
        case 'p': g.seg(lx, 0.30f, lx, 1.26f); g.arc(cx + 0.02f, xm, (rx - lx) * 0.45f, 0.34f, 0.0f, 360.0f); break;
        case 'q': g.seg(rx, 0.30f, rx, 1.26f); g.arc(cx - 0.02f, xm, (rx - lx) * 0.45f, 0.34f, 0.0f, 360.0f); break;
        case 'r': g.seg(lx + 0.02f, 0.30f, lx + 0.02f, 0.98f); g.arc(cx + 0.06f, 0.48f, (rx - lx) * 0.42f, 0.18f, 180.0f, 330.0f); break;
        case 's': { float rw = (rx - lx) * 0.44f, rh = 0.16f; g.arc(cx, 0.45f, rw, rh, 165.0f, 345.0f); g.seg(cx - rw * 0.92f, 0.51f, cx + rw * 0.92f, 0.74f); g.arc(cx, 0.82f, rw, rh, -15.0f, 165.0f); break; }
        case 't': g.seg(cx - 0.02f, 0.08f, cx - 0.02f, 0.84f); g.arc(cx + 0.06f, 0.84f, 0.08f, 0.14f, 30.0f, 180.0f); g.seg(lx, 0.32f, rx, 0.32f); break;
        case 'u': g.seg(lx, 0.30f, lx, 0.74f); g.seg(rx, 0.30f, rx, 0.98f); g.arc(cx, 0.74f, (rx - lx) * 0.48f, 0.24f, 0.0f, 180.0f); break;
        case 'v': g.seg(lx, 0.30f, cx, 0.98f); g.seg(cx, 0.98f, rx, 0.30f); break;
        case 'w': { float q = (rx - lx) * 0.25f; g.seg(lx, 0.30f, lx + q, 0.98f); g.seg(lx + q, 0.98f, cx, 0.44f); g.seg(cx, 0.44f, rx - q, 0.98f); g.seg(rx - q, 0.98f, rx, 0.30f); break; }
        case 'x': g.seg(lx, 0.30f, rx, 0.98f); g.seg(rx, 0.30f, lx, 0.98f); break;
        case 'y': g.seg(lx, 0.30f, cx, 0.96f); g.seg(rx, 0.30f, cx - 0.06f, 1.24f); g.arc(cx - 0.14f, 1.18f, 0.08f, 0.10f, 20.0f, 120.0f); break;
        case 'z': g.seg(lx, 0.30f, rx, 0.30f); g.seg(rx, 0.30f, lx, 0.98f); g.seg(lx, 0.98f, rx, 0.98f); break;
        case '{': g.arc(rx, 0.26f, (rx - lx) * 0.45f, 0.24f, 180.0f, 270.0f, 0.9f); g.seg(cx, 0.26f, cx, 0.45f, 0.9f); g.seg(cx, 0.45f, lx, 0.54f, 0.9f); g.seg(lx, 0.54f, cx, 0.63f, 0.9f); g.seg(cx, 0.63f, cx, 0.82f, 0.9f); g.arc(rx, 0.82f, (rx - lx) * 0.45f, 0.24f, 90.0f, 180.0f, 0.9f); break;
        case '|': g.seg(cx, 0.00f, cx, 1.08f, 0.9f); break;
        case '}': g.arc(lx, 0.26f, (rx - lx) * 0.45f, 0.24f, 270.0f, 360.0f, 0.9f); g.seg(cx, 0.26f, cx, 0.45f, 0.9f); g.seg(cx, 0.45f, rx, 0.54f, 0.9f); g.seg(rx, 0.54f, cx, 0.63f, 0.9f); g.seg(cx, 0.63f, cx, 0.82f, 0.9f); g.arc(lx, 0.82f, (rx - lx) * 0.45f, 0.24f, 0.0f, 90.0f, 0.9f); break;
        case '~': g.arc(lx + (rx - lx) * 0.25f, 0.52f, (rx - lx) * 0.25f, 0.14f, 180.0f, 360.0f, 0.9f); g.arc(rx - (rx - lx) * 0.25f, 0.52f, (rx - lx) * 0.25f, 0.14f, 0.0f, 180.0f, 0.9f); break;
        case 0x2022: g.dot(cx, 0.58f, 2.35f); break; // solid circular bullet
        default:  g.seg(lx, 0.06f, rx, 0.06f, 0.8f); g.seg(rx, 0.06f, rx, 0.96f, 0.8f); g.seg(rx, 0.96f, lx, 0.96f, 0.8f); g.seg(lx, 0.96f, lx, 0.06f, 0.8f); break;
        }
        return g;
    }

    inline const std::vector<uint8_t> &getAlphaAtlas() {
        static const std::vector<uint8_t> atlas = []() {
            std::vector<uint8_t> buf(ATLAS_W * ATLAS_H, 0);
            // Slot 0: solid 16x16 white patch for untextured solid geometry (safe under LINEAR filtering)
            for (int y = 0; y < 16; ++y) {
                for (int x = 0; x < 16; ++x) {
                    buf[y * ATLAS_W + x] = 255;
                }
            }
            const float strokeR = 0.068f;
            const float aaPx = 0.95f;
            auto rasterSlot = [&](int idx, uint32_t cp) {
                int col = idx % COLS;
                int row = idx / COLS;
                int x0 = col * CELL_W;
                int y0 = row * CELL_H;
                float dummyW = 0.0f;
                VecGlyph g = buildVectorGlyph(cp, dummyW);
                for (int py = 0; py < CELL_H; ++py) {
                    float gy = (static_cast<float>(py) + 0.5f - CAP_TOP_PX) / CAP_PX;
                    for (int px = 0; px < CELL_W; ++px) {
                        float gx = (static_cast<float>(px) + 0.5f - PAD_X) / CAP_PX;
                        float bestAlpha = 0.0f;
                        for (const auto &s : g.segs) {
                            float dPx = (distToSegment(gx, gy, s.x1, s.y1, s.x2, s.y2) - strokeR * s.rMul) * CAP_PX;
                            float a = std::clamp(0.5f - dPx / aaPx, 0.0f, 1.0f);
                            bestAlpha = std::max(bestAlpha, a);
                        }
                        for (const auto &ac : g.arcs) {
                            float dPx = (distToArc(gx, gy, ac.cx, ac.cy, ac.rx, ac.ry, ac.deg0, ac.deg1) - strokeR * ac.rMul) * CAP_PX;
                            float a = std::clamp(0.5f - dPx / aaPx, 0.0f, 1.0f);
                            bestAlpha = std::max(bestAlpha, a);
                        }
                        for (const auto &dt : g.dots) {
                            float dx = gx - dt.cx, dy = gy - dt.cy;
                            float dPx = (std::sqrt(dx * dx + dy * dy) - strokeR * dt.rMul) * CAP_PX;
                            float a = std::clamp(0.5f - dPx / aaPx, 0.0f, 1.0f);
                            bestAlpha = std::max(bestAlpha, a);
                        }
                        if (bestAlpha > 0.0f) {
                            buf[(y0 + py) * ATLAS_W + (x0 + px)] = static_cast<uint8_t>(std::lround(bestAlpha * 255.0f));
                        }
                    }
                }
            };
            for (int code = 33; code <= 126; ++code) {
                rasterSlot(code - 32, static_cast<uint32_t>(code));
            }
            rasterSlot(95, 0x2022);
            return buf;
        }();
        return atlas;
    }

    inline std::vector<uint8_t> getRgbaAtlas() {
        const auto &a = getAlphaAtlas();
        std::vector<uint8_t> rgba(ATLAS_W * ATLAS_H * 4, 255);
        for (size_t i = 0; i < a.size(); ++i) {
            rgba[i * 4 + 0] = 255;
            rgba[i * 4 + 1] = 255;
            rgba[i * 4 + 2] = 255;
            rgba[i * 4 + 3] = a[i];
        }
        return rgba;
    }

    inline void getGlyphUV(uint32_t codepoint, float &u0, float &v0, float &u1, float &v1) {
        int idx = 0;
        if (codepoint >= 33 && codepoint <= 126) idx = static_cast<int>(codepoint - 32);
        else if (codepoint == 0x2022) idx = 95;
        else if (codepoint == ' ') idx = 0;
        else idx = '?' - 32;

        if (idx == 0) {
            u0 = v0 = u1 = v1 = 0.0f;
            return;
        }
        int col = idx % COLS;
        int row = idx / COLS;
        float spanPx = getCharAdvanceFactor(codepoint, false) * EM_PX;
        float px0 = static_cast<float>(col * CELL_W) + PAD_X - UV_PAD_PX;
        float px1 = static_cast<float>(col * CELL_W) + PAD_X + spanPx + UV_PAD_PX;
        float py0 = static_cast<float>(row * CELL_H) + 3.0f;
        float py1 = static_cast<float>(row * CELL_H) + 41.0f;
        u0 = px0 / static_cast<float>(ATLAS_W);
        v0 = py0 / static_cast<float>(ATLAS_H);
        u1 = px1 / static_cast<float>(ATLAS_W);
        v1 = py1 / static_cast<float>(ATLAS_H);
    }
} // namespace FontAtlas

class Tessellator {
public:
    static constexpr float SOLID_UV = 7.5f / static_cast<float>(FontAtlas::ATLAS_W);

    static void buildRoundedPerimeter(
        float x, float y, float w, float h,
        const float rIn[4], std::vector<UiPoint> &out)
    {
        if (w <= 0.0f || h <= 0.0f) return;
        float maxR = 0.5f * std::min(w, h);
        float r[4] = {
            std::clamp(rIn[0], 0.0f, maxR),
            std::clamp(rIn[1], 0.0f, maxR),
            std::clamp(rIn[2], 0.0f, maxR),
            std::clamp(rIn[3], 0.0f, maxR)
        };

        const int segs = 6;
        auto addCorner = [&](float cx, float cy, float cr, float a0, float a1) {
            if (cr <= 0.1f) {
                out.push_back({cx, cy});
                return;
            }
            for (int i = 0; i <= segs; ++i) {
                float a = a0 + (a1 - a0) * (static_cast<float>(i) / static_cast<float>(segs));
                out.push_back({cx + std::cos(a) * cr, cy + std::sin(a) * cr});
            }
        };

        constexpr float PI = 3.14159265358979323846f;
        addCorner(x + r[0],     y + r[0],     r[0], PI,         1.5f * PI);
        addCorner(x + w - r[1], y + r[1],     r[1], 1.5f * PI,  2.0f * PI);
        addCorner(x + w - r[2], y + h - r[2], r[2], 0.0f,       0.5f * PI);
        addCorner(x + r[3],     y + h - r[3], r[3], 0.5f * PI,  PI);
    }

    static void addConvexFilledWithAA(
        UiMesh &mesh,
        const std::vector<UiPoint> &pts,
        const ColorRgba &c1,
        const ColorRgba *c2 = nullptr,
        float gradAngle = 0.0f,
        float x0 = 0, float y0 = 0, float w = 1, float h = 1,
        float aaFeather = 0.75f)
    {
        if (pts.size() < 3) return;
        float cx = 0, cy = 0;
        for (const auto &p : pts) { cx += p.x; cy += p.y; }
        cx /= static_cast<float>(pts.size());
        cy /= static_cast<float>(pts.size());

        float rad = gradAngle * 3.14159265f / 180.0f;
        float dx = std::cos(rad), dy = std::sin(rad);

        auto sampleColor = [&](float px, float py, float alphaScale) -> uint32_t {
            ColorRgba c = c1;
            if (c2 && w > 0 && h > 0) {
                float nx = (px - x0) / w - 0.5f;
                float ny = (py - y0) / h - 0.5f;
                float t = std::clamp(nx * dx + ny * dy + 0.5f, 0.0f, 1.0f);
                c.r = c1.r + (c2->r - c1.r) * t;
                c.g = c1.g + (c2->g - c1.g) * t;
                c.b = c1.b + (c2->b - c1.b) * t;
                c.a = c1.a + (c2->a - c1.a) * t;
            }
            c.a *= alphaScale;
            return c.toUint32();
        };

        uint32_t centerIdx = static_cast<uint32_t>(mesh.vertices.size());
        mesh.vertices.push_back({cx, cy, SOLID_UV, SOLID_UV, sampleColor(cx, cy, 1.0f)});

        uint32_t ringStart = static_cast<uint32_t>(mesh.vertices.size());
        for (const auto &p : pts) {
            mesh.vertices.push_back({p.x, p.y, SOLID_UV, SOLID_UV, sampleColor(p.x, p.y, 1.0f)});
        }

        uint32_t n = static_cast<uint32_t>(pts.size());
        for (uint32_t i = 0; i < n; ++i) {
            uint32_t next = (i + 1) % n;
            mesh.indices.push_back(centerIdx);
            mesh.indices.push_back(ringStart + i);
            mesh.indices.push_back(ringStart + next);
        }

        if (aaFeather > 0.0f) {
            uint32_t outerStart = static_cast<uint32_t>(mesh.vertices.size());
            for (size_t i = 0; i < pts.size(); ++i) {
                const auto &prev = pts[(i + pts.size() - 1) % pts.size()];
                const auto &curr = pts[i];
                const auto &next = pts[(i + 1) % pts.size()];
                float ex1 = curr.x - prev.x, ey1 = curr.y - prev.y;
                float l1 = std::hypot(ex1, ey1);
                float nx1 = (l1 > 1e-4f) ? (ey1 / l1) : 0.0f;
                float ny1 = (l1 > 1e-4f) ? (-ex1 / l1) : 0.0f;

                float ex2 = next.x - curr.x, ey2 = next.y - curr.y;
                float l2 = std::hypot(ex2, ey2);
                float nx2 = (l2 > 1e-4f) ? (ey2 / l2) : 0.0f;
                float ny2 = (l2 > 1e-4f) ? (-ex2 / l2) : 0.0f;

                float nx = nx1 + nx2, ny = ny1 + ny2;
                float nl = std::hypot(nx, ny);
                if (nl > 1e-4f) { nx /= nl; ny /= nl; }
                mesh.vertices.push_back({curr.x + nx * aaFeather, curr.y + ny * aaFeather, SOLID_UV, SOLID_UV, sampleColor(curr.x, curr.y, 0.0f)});
            }
            for (uint32_t i = 0; i < n; ++i) {
                uint32_t next = (i + 1) % n;
                mesh.indices.push_back(ringStart + i);
                mesh.indices.push_back(outerStart + i);
                mesh.indices.push_back(outerStart + next);
                mesh.indices.push_back(ringStart + i);
                mesh.indices.push_back(outerStart + next);
                mesh.indices.push_back(ringStart + next);
            }
        }
    }

    static void addClosedStroke(UiMesh &mesh, const std::vector<UiPoint> &pts, float width, const ColorRgba &col) {
        if (pts.size() < 3 || width <= 0.0f) return;
        float half = width * 0.5f;
        uint32_t innerStart = static_cast<uint32_t>(mesh.vertices.size());
        uint32_t outerStart = innerStart + static_cast<uint32_t>(pts.size());
        uint32_t rgba = col.toUint32();

        std::vector<UiVertex> innerV, outerV;
        innerV.reserve(pts.size());
        outerV.reserve(pts.size());
        for (size_t i = 0; i < pts.size(); ++i) {
            const auto &prev = pts[(i + pts.size() - 1) % pts.size()];
            const auto &curr = pts[i];
            const auto &next = pts[(i + 1) % pts.size()];
            float ex1 = curr.x - prev.x, ey1 = curr.y - prev.y;
            float l1 = std::hypot(ex1, ey1);
            float nx1 = (l1 > 1e-4f) ? (ey1 / l1) : 0.0f;
            float ny1 = (l1 > 1e-4f) ? (-ex1 / l1) : 0.0f;

            float ex2 = next.x - curr.x, ey2 = next.y - curr.y;
            float l2 = std::hypot(ex2, ey2);
            float nx2 = (l2 > 1e-4f) ? (ey2 / l2) : 0.0f;
            float ny2 = (l2 > 1e-4f) ? (-ex2 / l2) : 0.0f;

            float nx = nx1 + nx2, ny = ny1 + ny2;
            float nl = std::hypot(nx, ny);
            if (nl > 1e-4f) { nx /= nl; ny /= nl; }
            innerV.push_back({curr.x - nx * half, curr.y - ny * half, SOLID_UV, SOLID_UV, rgba});
            outerV.push_back({curr.x + nx * half, curr.y + ny * half, SOLID_UV, SOLID_UV, rgba});
        }
        mesh.vertices.insert(mesh.vertices.end(), innerV.begin(), innerV.end());
        mesh.vertices.insert(mesh.vertices.end(), outerV.begin(), outerV.end());

        uint32_t n = static_cast<uint32_t>(pts.size());
        for (uint32_t i = 0; i < n; ++i) {
            uint32_t next = (i + 1) % n;
            mesh.indices.push_back(innerStart + i);
            mesh.indices.push_back(outerStart + i);
            mesh.indices.push_back(outerStart + next);
            mesh.indices.push_back(innerStart + i);
            mesh.indices.push_back(outerStart + next);
            mesh.indices.push_back(innerStart + next);
        }
    }

    static void addPolyline(UiMesh &mesh, const std::vector<UiPoint> &pts, float width, const ColorRgba &col) {
        if (pts.size() < 2 || width <= 0.0f) return;
        float half = std::max(0.5f, width * 0.5f);
        uint32_t rgba = col.toUint32();
        for (size_t i = 0; i + 1 < pts.size(); ++i) {
            const auto &a = pts[i];
            const auto &b = pts[i + 1];
            float dx = b.x - a.x, dy = b.y - a.y;
            float len = std::hypot(dx, dy);
            if (len < 1e-4f) continue;
            float nx = -dy / len * half;
            float ny = dx / len * half;
            uint32_t base = static_cast<uint32_t>(mesh.vertices.size());
            mesh.vertices.push_back({a.x + nx, a.y + ny, SOLID_UV, SOLID_UV, rgba});
            mesh.vertices.push_back({a.x - nx, a.y - ny, SOLID_UV, SOLID_UV, rgba});
            mesh.vertices.push_back({b.x - nx, b.y - ny, SOLID_UV, SOLID_UV, rgba});
            mesh.vertices.push_back({b.x + nx, b.y + ny, SOLID_UV, SOLID_UV, rgba});
            mesh.indices.push_back(base + 0); mesh.indices.push_back(base + 1); mesh.indices.push_back(base + 2);
            mesh.indices.push_back(base + 0); mesh.indices.push_back(base + 2); mesh.indices.push_back(base + 3);
        }
    }

    static std::vector<UiPoint> parseSvgPathPoints(const std::string &d, float ox, float oy, float sx = 1.0f, float sy = 1.0f) {
        std::vector<UiPoint> pts;
        size_t i = 0;
        char cmd = 'M';
        float cx = 0.0f, cy = 0.0f;
        float startX = 0.0f, startY = 0.0f;

        auto skipSep = [&]() {
            while (i < d.size() && (d[i] == ' ' || d[i] == ',' || d[i] == '\t' || d[i] == '\n' || d[i] == '\r')) ++i;
        };
        auto readNum = [&](float &out) -> bool {
            skipSep();
            if (i >= d.size()) return false;
            size_t start = i;
            if (d[i] == '-' || d[i] == '+') ++i;
            bool dot = false;
            while (i < d.size() && (std::isdigit(static_cast<unsigned char>(d[i])) || (!dot && d[i] == '.'))) {
                if (d[i] == '.') dot = true;
                ++i;
            }
            if (i == start || (i == start + 1 && (d[start] == '-' || d[start] == '+'))) {
                i = start;
                return false;
            }
            try { out = std::stof(d.substr(start, i - start)); return true; } catch (...) { return false; }
        };

        while (i < d.size()) {
            skipSep();
            if (i >= d.size()) break;
            if (std::isalpha(static_cast<unsigned char>(d[i]))) {
                cmd = d[i++];
                if (cmd == 'Z' || cmd == 'z') {
                    pts.push_back({ox + startX * sx, oy + startY * sy});
                    cx = startX; cy = startY;
                    continue;
                }
            }
            if (cmd == 'M' || cmd == 'm' || cmd == 'L' || cmd == 'l') {
                float vx, vy;
                if (!readNum(vx) || !readNum(vy)) break;
                if (cmd == 'm' || cmd == 'l') { cx += vx; cy += vy; }
                else { cx = vx; cy = vy; }
                if (cmd == 'M' || cmd == 'm') { startX = cx; startY = cy; cmd = (cmd == 'M') ? 'L' : 'l'; }
                pts.push_back({ox + cx * sx, oy + cy * sy});
            } else if (cmd == 'H' || cmd == 'h') {
                float vx;
                if (!readNum(vx)) break;
                cx = (cmd == 'h') ? (cx + vx) : vx;
                pts.push_back({ox + cx * sx, oy + cy * sy});
            } else if (cmd == 'V' || cmd == 'v') {
                float vy;
                if (!readNum(vy)) break;
                cy = (cmd == 'v') ? (cy + vy) : vy;
                pts.push_back({ox + cx * sx, oy + cy * sy});
            } else if (cmd == 'Q' || cmd == 'q') {
                float x1, y1, x2, y2;
                if (!readNum(x1) || !readNum(y1) || !readNum(x2) || !readNum(y2)) break;
                if (cmd == 'q') { x1 += cx; y1 += cy; x2 += cx; y2 += cy; }
                for (int s = 1; s <= 6; ++s) {
                    float t = s / 6.0f, it = 1.0f - t;
                    float px = it * it * cx + 2 * it * t * x1 + t * t * x2;
                    float py = it * it * cy + 2 * it * t * y1 + t * t * y2;
                    pts.push_back({ox + px * sx, oy + py * sy});
                }
                cx = x2; cy = y2;
            } else if (cmd == 'C' || cmd == 'c') {
                float x1, y1, x2, y2, x3, y3;
                if (!readNum(x1) || !readNum(y1) || !readNum(x2) || !readNum(y2) || !readNum(x3) || !readNum(y3)) break;
                if (cmd == 'c') { x1 += cx; y1 += cy; x2 += cx; y2 += cy; x3 += cx; y3 += cy; }
                for (int s = 1; s <= 8; ++s) {
                    float t = s / 8.0f, it = 1.0f - t;
                    float px = it*it*it*cx + 3*it*it*t*x1 + 3*it*t*t*x2 + t*t*t*x3;
                    float py = it*it*it*cy + 3*it*it*t*y1 + 3*it*t*t*y2 + t*t*t*y3;
                    pts.push_back({ox + px * sx, oy + py * sy});
                }
                cx = x3; cy = y3;
            } else {
                ++i;
            }
        }
        return pts;
    }

    static void addText(UiMesh &mesh, float x, float baselineY, const std::string &text, float fontSize, const ColorRgba &col, bool isMono, int fontWeight = 400) {
        if (text.empty()) return;
        uint32_t rgba = col.toUint32();
        float scale = fontSize / FontAtlas::EM_PX;
        float padScr = FontAtlas::UV_PAD_PX * scale;
        float topOff = (FontAtlas::BASE_PX - 3.0f) * scale;
        float botOff = (41.0f - FontAtlas::BASE_PX) * scale;
        float gy0 = baselineY - topOff;
        float gy1 = baselineY + botOff;
        bool isBold = (fontWeight >= 600);
        uint32_t boldRgba = col.withAlpha(col.a * 0.65f).toUint32();
        float boldOff = isBold ? std::max(0.42f, fontSize * 0.032f) : 0.0f;
        float curX = x;

        auto emitQuad = [&](float qx0, float qy0, float qx1, float qy1, float u0, float v0, float u1, float v1, uint32_t c) {
            uint32_t base = static_cast<uint32_t>(mesh.vertices.size());
            mesh.vertices.push_back({qx0, qy0, u0, v0, c});
            mesh.vertices.push_back({qx1, qy0, u1, v0, c});
            mesh.vertices.push_back({qx1, qy1, u1, v1, c});
            mesh.vertices.push_back({qx0, qy1, u0, v1, c});
            mesh.indices.push_back(base + 0); mesh.indices.push_back(base + 1); mesh.indices.push_back(base + 2);
            mesh.indices.push_back(base + 0); mesh.indices.push_back(base + 2); mesh.indices.push_back(base + 3);
        };

        for (size_t i = 0; i < text.size();) {
            size_t next = i;
            uint32_t cp = decodeGlyphCodepoint(text, i, next);
            if (cp == 0) {
                i = next;
                continue;
            }
            float propAdv = fontSize * getCharAdvanceFactor(cp, false);
            float slotAdv = fontSize * getCharAdvanceFactor(cp, isMono);
            if (cp != ' ') {
                float u0, v0, u1, v1;
                FontAtlas::getGlyphUV(cp, u0, v0, u1, v1);
                float glyphX = isMono ? (curX + (slotAdv - propAdv) * 0.5f) : curX;
                float gx0 = glyphX - padScr;
                float gx1 = glyphX + propAdv + padScr;
                emitQuad(gx0, gy0, gx1, gy1, u0, v0, u1, v1, rgba);
                if (isBold) {
                    emitQuad(gx0 + boldOff, gy0, gx1 + boldOff, gy1, u0, v0, u1, v1, boldRgba);
                }
            }
            curX += slotAdv;
            i = next;
        }
    }

    static UiMesh tessellate(const std::vector<GpuDrawCmd> &cmds, float viewportW, float viewportH) {
        UiMesh mesh;
        std::vector<UiRect> clipStack;
        UiRect rootClip = {0.0f, 0.0f, viewportW, viewportH};
        clipStack.push_back(rootClip);

        uint32_t batchStart = 0;
        auto flushBatch = [&](const UiRect &clip) {
            uint32_t curCount = static_cast<uint32_t>(mesh.indices.size());
            if (curCount > batchStart) {
                mesh.batches.push_back({clip, batchStart, curCount - batchStart});
                batchStart = curCount;
            }
        };

        for (const auto &cmd : cmds) {
            if (cmd.type == "PUSH_CLIP") {
                flushBatch(clipStack.back());
                UiRect next = clipStack.back().intersect({cmd.x, cmd.y, cmd.width, cmd.height});
                clipStack.push_back(next);
                continue;
            }
            if (cmd.type == "POP_CLIP") {
                flushBatch(clipStack.back());
                if (clipStack.size() > 1) clipStack.pop_back();
                continue;
            }

            ColorRgba col = hexToRgba(cmd.color, cmd.alpha);

            if (cmd.type == "SHADOW_ROUND_RECT") {
                float r[4] = {cmd.radius, cmd.radius, cmd.radius, cmd.radius};
                for (int layer = 3; layer >= 1; --layer) {
                    float spread = static_cast<float>(layer) * std::max(1.5f, cmd.blur * 0.35f);
                    ColorRgba sc = col.withAlpha(cmd.alpha * (0.14f / static_cast<float>(layer)));
                    std::vector<UiPoint> pts;
                    float rr[4] = {r[0] + spread, r[1] + spread, r[2] + spread, r[3] + spread};
                    buildRoundedPerimeter(cmd.x - spread, cmd.y + cmd.lineWidth - spread * 0.5f, cmd.width + spread * 2.0f, cmd.height + spread * 2.0f, rr, pts);
                    addConvexFilledWithAA(mesh, pts, sc, nullptr, 0, 0, 0, 1, 1, 1.5f);
                }
            } else if (cmd.type == "FILL_ROUND_RECT" || cmd.type == "DRAW_SELECTION_RECT" || cmd.type == "DRAW_CARET") {
                float r[4] = {
                    cmd.radii[0] > 0 ? cmd.radii[0] : cmd.radius,
                    cmd.radii[1] > 0 ? cmd.radii[1] : cmd.radius,
                    cmd.radii[2] > 0 ? cmd.radii[2] : cmd.radius,
                    cmd.radii[3] > 0 ? cmd.radii[3] : cmd.radius
                };
                std::vector<UiPoint> pts;
                buildRoundedPerimeter(cmd.x, cmd.y, cmd.width, cmd.height, r, pts);
                addConvexFilledWithAA(mesh, pts, col, nullptr, 0, cmd.x, cmd.y, cmd.width, cmd.height, 0.65f);
            } else if (cmd.type == "FILL_GRADIENT_RECT") {
                float r[4] = {
                    cmd.radii[0] > 0 ? cmd.radii[0] : cmd.radius,
                    cmd.radii[1] > 0 ? cmd.radii[1] : cmd.radius,
                    cmd.radii[2] > 0 ? cmd.radii[2] : cmd.radius,
                    cmd.radii[3] > 0 ? cmd.radii[3] : cmd.radius
                };
                ColorRgba col2 = hexToRgba(cmd.color2, cmd.alpha2);
                std::vector<UiPoint> pts;
                buildRoundedPerimeter(cmd.x, cmd.y, cmd.width, cmd.height, r, pts);
                addConvexFilledWithAA(mesh, pts, col, &col2, cmd.angle, cmd.x, cmd.y, cmd.width, cmd.height, 0.65f);
            } else if (cmd.type == "STROKE_ROUND_RECT") {
                float r[4] = {
                    cmd.radii[0] > 0 ? cmd.radii[0] : cmd.radius,
                    cmd.radii[1] > 0 ? cmd.radii[1] : cmd.radius,
                    cmd.radii[2] > 0 ? cmd.radii[2] : cmd.radius,
                    cmd.radii[3] > 0 ? cmd.radii[3] : cmd.radius
                };
                std::vector<UiPoint> pts;
                buildRoundedPerimeter(cmd.x, cmd.y, cmd.width, cmd.height, r, pts);
                addClosedStroke(mesh, pts, cmd.lineWidth, col);
            } else if (cmd.type == "DRAW_LINE") {
                addPolyline(mesh, {{cmd.x, cmd.y}, {cmd.x + cmd.width, cmd.y + cmd.height}}, cmd.lineWidth, col);
            } else if (cmd.type == "DRAW_CIRCLE" || cmd.type == "STROKE_CIRCLE") {
                float cx = cmd.x + cmd.width * 0.5f;
                float cy = cmd.y + cmd.height * 0.5f;
                float rad = cmd.radius > 0 ? cmd.radius : (std::min(cmd.width, cmd.height) * 0.5f);
                std::vector<UiPoint> pts;
                const int N = 24;
                for (int i = 0; i < N; ++i) {
                    float a = static_cast<float>(i) * (2.0f * 3.14159265f / static_cast<float>(N));
                    pts.push_back({cx + std::cos(a) * rad, cy + std::sin(a) * rad});
                }
                if (cmd.type == "DRAW_CIRCLE" && cmd.filled) {
                    addConvexFilledWithAA(mesh, pts, col, nullptr, 0, cmd.x, cmd.y, cmd.width, cmd.height, 0.75f);
                } else {
                    addClosedStroke(mesh, pts, cmd.lineWidth, col);
                }
            } else if (cmd.type == "DRAW_POLYLINE") {
                addPolyline(mesh, cmd.points, cmd.lineWidth, col);
            } else if (cmd.type == "FILL_POLYGON") {
                addConvexFilledWithAA(mesh, cmd.points, col, nullptr, 0, 0, 0, 1, 1, 0.75f);
            } else if (cmd.type == "DRAW_SVG_PATH") {
                float sx = cmd.width > 0 ? (cmd.width / 24.0f) : 1.0f;
                float sy = cmd.height > 0 ? (cmd.height / 24.0f) : 1.0f;
                auto pts = parseSvgPathPoints(cmd.svgPath, cmd.x, cmd.y, sx, sy);
                if (cmd.filled) addConvexFilledWithAA(mesh, pts, col, nullptr, 0, cmd.x, cmd.y, cmd.width, cmd.height, 0.5f);
                else addPolyline(mesh, pts, cmd.lineWidth, col);
            } else if (cmd.type == "DRAW_TEXT") {
                addText(mesh, cmd.x, cmd.y, cmd.text, cmd.fontSize, col, cmd.isMono, cmd.fontWeight);
            }
        }

        flushBatch(clipStack.back());
        return mesh;
    }
};

} // namespace MemeUI
