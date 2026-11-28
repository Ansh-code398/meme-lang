#pragma once

#include "MemeUiTypes.h"
#include "MemeUiTextEngine.h"
#include "MemeUiFontEngine.h"
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

// Dynamic 1024x1024 TrueType / Vector Multi-Font Atlas backed by MemeUiFontEngine.h
namespace FontAtlas {
    constexpr int ATLAS_W = MemeUiFonts::ATLAS_W;
    constexpr int ATLAS_H = MemeUiFonts::ATLAS_H;

    inline uint64_t getAtlasVersion() { return MemeUiFonts::getAtlasVersion(); }
    inline std::vector<uint8_t> getAlphaAtlas() { return MemeUiFonts::getAlphaAtlas(); }
    inline std::vector<uint8_t> getRgbaAtlas() { return MemeUiFonts::getRgbaAtlas(); }
    inline void getGlyphUV(uint32_t ch, float &u0, float &v0, float &u1, float &v1, FontFamily family = FontFamily::SANS) {
        auto gm = MemeUiFonts::acquireGlyph(ch, family);
        u0 = gm.u0; v0 = gm.v0; u1 = gm.u1; v1 = gm.v1;
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

    struct SvgSubpath {
        std::vector<UiPoint> pts;
        bool closed = false;
    };

    static std::vector<SvgSubpath> parseSvgSubpaths(const std::string &d, float ox, float oy, float sx = 1.0f, float sy = 1.0f) {
        std::vector<SvgSubpath> subpaths;
        size_t i = 0;
        char cmd = 'M';
        float cx = 0.0f, cy = 0.0f;
        float startX = 0.0f, startY = 0.0f;
        float lastCtrlX = 0.0f, lastCtrlY = 0.0f;
        char prevCmd = 0;

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
        auto readFlag = [&](int &out) -> bool {
            skipSep();
            if (i >= d.size()) return false;
            if (d[i] == '0' || d[i] == '1') {
                out = (d[i] == '1') ? 1 : 0;
                ++i;
                return true;
            }
            return false;
        };
        auto ensureSubpath = [&]() -> SvgSubpath & {
            if (subpaths.empty()) subpaths.push_back(SvgSubpath{});
            return subpaths.back();
        };

        while (i < d.size()) {
            skipSep();
            if (i >= d.size()) break;
            if (std::isalpha(static_cast<unsigned char>(d[i]))) {
                cmd = d[i++];
                if (cmd == 'Z' || cmd == 'z') {
                    if (!subpaths.empty()) {
                        subpaths.back().closed = true;
                    }
                    cx = startX; cy = startY;
                    prevCmd = cmd;
                    continue;
                }
            }
            if (cmd == 'M' || cmd == 'm') {
                float vx, vy;
                if (!readNum(vx) || !readNum(vy)) break;
                if (cmd == 'm') { cx += vx; cy += vy; }
                else { cx = vx; cy = vy; }
                startX = cx; startY = cy;
                subpaths.push_back(SvgSubpath{});
                subpaths.back().pts.push_back({ox + cx * sx, oy + cy * sy});
                prevCmd = cmd;
                cmd = (cmd == 'M') ? 'L' : 'l';
            } else if (cmd == 'L' || cmd == 'l') {
                float vx, vy;
                if (!readNum(vx) || !readNum(vy)) break;
                if (cmd == 'l') { cx += vx; cy += vy; }
                else { cx = vx; cy = vy; }
                ensureSubpath().pts.push_back({ox + cx * sx, oy + cy * sy});
                prevCmd = cmd;
            } else if (cmd == 'H' || cmd == 'h') {
                float vx;
                if (!readNum(vx)) break;
                cx = (cmd == 'h') ? (cx + vx) : vx;
                ensureSubpath().pts.push_back({ox + cx * sx, oy + cy * sy});
                prevCmd = cmd;
            } else if (cmd == 'V' || cmd == 'v') {
                float vy;
                if (!readNum(vy)) break;
                cy = (cmd == 'v') ? (cy + vy) : vy;
                ensureSubpath().pts.push_back({ox + cx * sx, oy + cy * sy});
                prevCmd = cmd;
            } else if (cmd == 'Q' || cmd == 'q' || cmd == 'T' || cmd == 't') {
                float x1, y1, x2, y2;
                if (cmd == 'T' || cmd == 't') {
                    if (!readNum(x2) || !readNum(y2)) break;
                    if (cmd == 't') { x2 += cx; y2 += cy; }
                    if (prevCmd == 'Q' || prevCmd == 'q' || prevCmd == 'T' || prevCmd == 't') {
                        x1 = 2.0f * cx - lastCtrlX;
                        y1 = 2.0f * cy - lastCtrlY;
                    } else { x1 = cx; y1 = cy; }
                } else {
                    if (!readNum(x1) || !readNum(y1) || !readNum(x2) || !readNum(y2)) break;
                    if (cmd == 'q') { x1 += cx; y1 += cy; x2 += cx; y2 += cy; }
                }
                auto &pts = ensureSubpath().pts;
                for (int s = 1; s <= 8; ++s) {
                    float t = s / 8.0f, it = 1.0f - t;
                    float px = it * it * cx + 2 * it * t * x1 + t * t * x2;
                    float py = it * it * cy + 2 * it * t * y1 + t * t * y2;
                    pts.push_back({ox + px * sx, oy + py * sy});
                }
                lastCtrlX = x1; lastCtrlY = y1;
                cx = x2; cy = y2;
                prevCmd = cmd;
            } else if (cmd == 'C' || cmd == 'c' || cmd == 'S' || cmd == 's') {
                float x1, y1, x2, y2, x3, y3;
                if (cmd == 'S' || cmd == 's') {
                    if (!readNum(x2) || !readNum(y2) || !readNum(x3) || !readNum(y3)) break;
                    if (cmd == 's') { x2 += cx; y2 += cy; x3 += cx; y3 += cy; }
                    if (prevCmd == 'C' || prevCmd == 'c' || prevCmd == 'S' || prevCmd == 's') {
                        x1 = 2.0f * cx - lastCtrlX;
                        y1 = 2.0f * cy - lastCtrlY;
                    } else { x1 = cx; y1 = cy; }
                } else {
                    if (!readNum(x1) || !readNum(y1) || !readNum(x2) || !readNum(y2) || !readNum(x3) || !readNum(y3)) break;
                    if (cmd == 'c') { x1 += cx; y1 += cy; x2 += cx; y2 += cy; x3 += cx; y3 += cy; }
                }
                auto &pts = ensureSubpath().pts;
                for (int s = 1; s <= 10; ++s) {
                    float t = s / 10.0f, it = 1.0f - t;
                    float px = it*it*it*cx + 3*it*it*t*x1 + 3*it*t*t*x2 + t*t*t*x3;
                    float py = it*it*it*cy + 3*it*it*t*y1 + 3*it*t*t*y2 + t*t*t*y3;
                    pts.push_back({ox + px * sx, oy + py * sy});
                }
                lastCtrlX = x2; lastCtrlY = y2;
                cx = x3; cy = y3;
                prevCmd = cmd;
            } else if (cmd == 'A' || cmd == 'a') {
                float rx, ry, xRotDeg, xEnd, yEnd;
                int largeArc = 0, sweep = 0;
                if (!readNum(rx) || !readNum(ry) || !readNum(xRotDeg) ||
                    !readFlag(largeArc) || !readFlag(sweep) ||
                    !readNum(xEnd) || !readNum(yEnd)) break;
                if (cmd == 'a') { xEnd += cx; yEnd += cy; }
                auto &pts = ensureSubpath().pts;
                rx = std::abs(rx); ry = std::abs(ry);
                if (rx < 1e-4f || ry < 1e-4f || (std::abs(xEnd - cx) < 1e-4f && std::abs(yEnd - cy) < 1e-4f)) {
                    pts.push_back({ox + xEnd * sx, oy + yEnd * sy});
                } else {
                    float phi = xRotDeg * (3.14159265f / 180.0f);
                    float cphi = std::cos(phi), sphi = std::sin(phi);
                    float dx2 = (cx - xEnd) * 0.5f, dy2 = (cy - yEnd) * 0.5f;
                    float x1p = cphi * dx2 + sphi * dy2;
                    float y1p = -sphi * dx2 + cphi * dy2;
                    float rx2 = rx * rx, ry2 = ry * ry;
                    float x1p2 = x1p * x1p, y1p2 = y1p * y1p;
                    float lam = x1p2 / rx2 + y1p2 / ry2;
                    if (lam > 1.0f) { float sq = std::sqrt(lam); rx *= sq; ry *= sq; rx2 = rx * rx; ry2 = ry * ry; }
                    float num = std::max(0.0f, rx2 * ry2 - rx2 * y1p2 - ry2 * x1p2);
                    float den = std::max(1e-6f, rx2 * y1p2 + ry2 * x1p2);
                    float factor = std::sqrt(num / den) * ((largeArc == sweep) ? -1.0f : 1.0f);
                    float cxp = factor * (rx * y1p / ry);
                    float cyp = factor * (-ry * x1p / rx);
                    float cxCenter = cphi * cxp - sphi * cyp + (cx + xEnd) * 0.5f;
                    float cyCenter = sphi * cxp + cphi * cyp + (cy + yEnd) * 0.5f;
                    auto vecAng = [](float ux, float uy, float vx, float vy) {
                        float dot = ux * vx + uy * vy;
                        float len = std::max(1e-6f, std::hypot(ux, uy) * std::hypot(vx, vy));
                        float ang = std::acos(std::clamp(dot / len, -1.0f, 1.0f));
                        return (ux * vy - uy * vx < 0.0f) ? -ang : ang;
                    };
                    float theta1 = vecAng(1.0f, 0.0f, (x1p - cxp) / rx, (y1p - cyp) / ry);
                    float dTheta = vecAng((x1p - cxp) / rx, (y1p - cyp) / ry, (-x1p - cxp) / rx, (-y1p - cyp) / ry);
                    constexpr float PI = 3.14159265f;
                    if (sweep == 0 && dTheta > 0.0f) dTheta -= 2.0f * PI;
                    if (sweep == 1 && dTheta < 0.0f) dTheta += 2.0f * PI;
                    int segs = std::max(6, static_cast<int>(std::ceil(std::abs(dTheta) / (PI / 10.0f))));
                    for (int s = 1; s <= segs; ++s) {
                        float a = theta1 + dTheta * (static_cast<float>(s) / static_cast<float>(segs));
                        float px = cxCenter + cphi * rx * std::cos(a) - sphi * ry * std::sin(a);
                        float py = cyCenter + sphi * rx * std::cos(a) + cphi * ry * std::sin(a);
                        pts.push_back({ox + px * sx, oy + py * sy});
                    }
                }
                cx = xEnd; cy = yEnd;
                prevCmd = cmd;
            } else {
                ++i;
            }
        }
        return subpaths;
    }

    static void addText(UiMesh &mesh, float x, float baselineY, const std::string &text, float fontSize, const ColorRgba &col, bool isMono, int fontWeight = 400, FontFamily family = FontFamily::SANS, const std::string &fontName = "") {
        if (text.empty()) return;
        FontFamily effFam = isMono ? FontFamily::MONO : family;
        uint32_t rgba = col.toUint32();
        bool isBold = (fontWeight >= 600);
        uint32_t boldRgba = col.withAlpha(col.a * 0.65f).toUint32();
        float boldOff = isBold ? std::max(0.42f, fontSize * 0.032f) : 0.0f;
        float curX = x;
        uint32_t prevCp = 0;

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
            float advPx = MemeUiFonts::measureCharAdvance(cp, prevCp, fontSize, effFam, fontName, fontWeight);
            GlyphMetrics gm = MemeUiFonts::acquireGlyph(cp, effFam, fontName, fontWeight);
            if (gm.visible) {
                float gx0 = curX + gm.x0Em * fontSize;
                float gy0 = baselineY + gm.y0Em * fontSize;
                float gx1 = curX + gm.x1Em * fontSize;
                float gy1 = baselineY + gm.y1Em * fontSize;
                emitQuad(gx0, gy0, gx1, gy1, gm.u0, gm.v0, gm.u1, gm.v1, rgba);
                if (gm.synthBold) {
                    emitQuad(gx0 + boldOff, gy0, gx1 + boldOff, gy1, gm.u0, gm.v0, gm.u1, gm.v1, boldRgba);
                }
            }
            curX += advPx;
            prevCp = cp;
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
                auto subpaths = parseSvgSubpaths(cmd.svgPath, cmd.x, cmd.y, sx, sy);
                for (const auto &sp : subpaths) {
                    if (sp.pts.empty()) continue;
                    if (cmd.filled && sp.pts.size() >= 3) {
                        addConvexFilledWithAA(mesh, sp.pts, col, nullptr, 0, cmd.x, cmd.y, cmd.width, cmd.height, 0.5f);
                    } else if (sp.closed && sp.pts.size() >= 3) {
                        addClosedStroke(mesh, sp.pts, cmd.lineWidth, col);
                    } else if (sp.pts.size() >= 2) {
                        addPolyline(mesh, sp.pts, cmd.lineWidth, col);
                    }
                }
            } else if (cmd.type == "DRAW_TEXT") {
                FontFamily fam = cmd.isSerif ? FontFamily::SERIF : (cmd.isMono ? FontFamily::MONO : FontFamily::SANS);
                addText(mesh, cmd.x, cmd.y, cmd.text, cmd.fontSize, col, cmd.isMono, cmd.fontWeight, fam, cmd.fontName);
            }
        }

        flushBatch(clipStack.back());
        return mesh;
    }
};

} // namespace MemeUI
