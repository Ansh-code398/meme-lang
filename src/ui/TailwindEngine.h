#ifndef TAILWIND_ENGINE_H
#define TAILWIND_ENGINE_H

#include "../interpreter/Value.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <iomanip>

namespace TailwindEngine
{

// ---------------------------------------------------------------------------
// 22 Official Tailwind Color Families × 11 Shades (50-950) + Special Colors
// ---------------------------------------------------------------------------

struct ColorRgba
{
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;
    std::string hex = "#000000";

    ColorRgba() = default;
    ColorRgba(float r, float g, float b, float a = 1.0f, std::string hex = "")
        : r(r), g(g), b(b), a(a), hex(std::move(hex)) {}
};

inline ColorRgba hexToRgba(const std::string &hexStr, float alpha = 1.0f)
{
    std::string h = hexStr;
    if (!h.empty() && h[0] == '#') h = h.substr(1);
    if (h.size() == 3) {
        std::string expanded;
        for (char c : h) { expanded += c; expanded += c; }
        h = expanded;
    }
    if (h.size() == 6) {
        unsigned int val = 0;
        std::stringstream ss;
        ss << std::hex << h;
        ss >> val;
        float r = ((val >> 16) & 0xFF) / 255.0f;
        float g = ((val >> 8) & 0xFF) / 255.0f;
        float b = (val & 0xFF) / 255.0f;
        return ColorRgba(r, g, b, alpha, "#" + h);
    }
    if (h.size() == 8) {
        unsigned long long val = 0;
        std::stringstream ss;
        ss << std::hex << h;
        ss >> val;
        float r = ((val >> 24) & 0xFF) / 255.0f;
        float g = ((val >> 16) & 0xFF) / 255.0f;
        float b = ((val >> 8) & 0xFF) / 255.0f;
        float a = (val & 0xFF) / 255.0f;
        return ColorRgba(r, g, b, alpha * a, "#" + h.substr(0, 6));
    }
    return ColorRgba(0, 0, 0, alpha, "#000000");
}

static const std::unordered_map<std::string, std::string> TAILWIND_COLORS = {
    // Slate
    {"slate-50", "#f8fafc"}, {"slate-100", "#f1f5f9"}, {"slate-200", "#e2e8f0"},
    {"slate-300", "#cbd5e1"}, {"slate-400", "#94a3b8"}, {"slate-500", "#64748b"},
    {"slate-600", "#475569"}, {"slate-700", "#334155"}, {"slate-800", "#1e293b"},
    {"slate-900", "#0f172a"}, {"slate-950", "#020617"},

    // Gray
    {"gray-50", "#f9fafb"}, {"gray-100", "#f3f4f6"}, {"gray-200", "#e5e7eb"},
    {"gray-300", "#d1d5db"}, {"gray-400", "#9ca3af"}, {"gray-500", "#6b7280"},
    {"gray-600", "#4b5563"}, {"gray-700", "#374151"}, {"gray-800", "#1f2937"},
    {"gray-900", "#111827"}, {"gray-950", "#030712"},

    // Zinc
    {"zinc-50", "#fafafa"}, {"zinc-100", "#f4f4f5"}, {"zinc-200", "#e4e4e7"},
    {"zinc-300", "#d4d4d8"}, {"zinc-400", "#a1a1aa"}, {"zinc-500", "#71717a"},
    {"zinc-600", "#52525b"}, {"zinc-700", "#3f3f46"}, {"zinc-800", "#27272a"},
    {"zinc-900", "#18181b"}, {"zinc-950", "#09090b"},

    // Neutral
    {"neutral-50", "#fafafa"}, {"neutral-100", "#f5f5f5"}, {"neutral-200", "#e5e5e5"},
    {"neutral-300", "#d4d4d4"}, {"neutral-400", "#a3a3a3"}, {"neutral-500", "#737373"},
    {"neutral-600", "#525252"}, {"neutral-700", "#404040"}, {"neutral-800", "#262626"},
    {"neutral-900", "#171717"}, {"neutral-950", "#0a0a0a"},

    // Stone
    {"stone-50", "#fafaf9"}, {"stone-100", "#f5f5f4"}, {"stone-200", "#e7e5e4"},
    {"stone-300", "#d6d3d1"}, {"stone-400", "#a8a29e"}, {"stone-500", "#78716c"},
    {"stone-600", "#57534e"}, {"stone-700", "#44403c"}, {"stone-800", "#292524"},
    {"stone-900", "#1c1917"}, {"stone-950", "#0c0a09"},

    // Red
    {"red-50", "#fef2f2"}, {"red-100", "#fee2e2"}, {"red-200", "#fecaca"},
    {"red-300", "#fca5a5"}, {"red-400", "#f87171"}, {"red-500", "#ef4444"},
    {"red-600", "#dc2626"}, {"red-700", "#b91c1c"}, {"red-800", "#991b1b"},
    {"red-900", "#7f1d1d"}, {"red-950", "#450a0a"},

    // Orange
    {"orange-50", "#fff7ed"}, {"orange-100", "#ffedd5"}, {"orange-200", "#fed7aa"},
    {"orange-300", "#fdba74"}, {"orange-400", "#fb923c"}, {"orange-500", "#f97316"},
    {"orange-600", "#ea580c"}, {"orange-700", "#c2410c"}, {"orange-800", "#9a3412"},
    {"orange-900", "#7c2d12"}, {"orange-950", "#431407"},

    // Amber
    {"amber-50", "#fffbeb"}, {"amber-100", "#fef3c7"}, {"amber-200", "#fde68a"},
    {"amber-300", "#fcd34d"}, {"amber-400", "#fbbf24"}, {"amber-500", "#f59e0b"},
    {"amber-600", "#d97706"}, {"amber-700", "#b45309"}, {"amber-800", "#92400e"},
    {"amber-900", "#78350f"}, {"amber-950", "#451a03"},

    // Yellow
    {"yellow-50", "#fefce8"}, {"yellow-100", "#fef9c3"}, {"yellow-200", "#fef08a"},
    {"yellow-300", "#fde047"}, {"yellow-400", "#facc15"}, {"yellow-500", "#eab308"},
    {"yellow-600", "#ca8a04"}, {"yellow-700", "#a16207"}, {"yellow-800", "#854d0e"},
    {"yellow-900", "#713f12"}, {"yellow-950", "#422006"},

    // Lime
    {"lime-50", "#f7fee7"}, {"lime-100", "#ecfccb"}, {"lime-200", "#d9f99d"},
    {"lime-300", "#bef264"}, {"lime-400", "#a3e635"}, {"lime-500", "#84cc16"},
    {"lime-600", "#65a30d"}, {"lime-700", "#4d7c0f"}, {"lime-800", "#3f6212"},
    {"lime-900", "#365314"}, {"lime-950", "#1a2e05"},

    // Green
    {"green-50", "#f0fdf4"}, {"green-100", "#dcfce7"}, {"green-200", "#bbf7d0"},
    {"green-300", "#86efac"}, {"green-400", "#4ade80"}, {"green-500", "#22c55e"},
    {"green-600", "#16a34a"}, {"green-700", "#15803d"}, {"green-800", "#166534"},
    {"green-900", "#14532d"}, {"green-950", "#052e16"},

    // Emerald
    {"emerald-50", "#ecfdf5"}, {"emerald-100", "#d1fae5"}, {"emerald-200", "#a7f3d0"},
    {"emerald-300", "#6ee7b7"}, {"emerald-400", "#34d399"}, {"emerald-500", "#10b981"},
    {"emerald-600", "#059669"}, {"emerald-700", "#047857"}, {"emerald-800", "#065f46"},
    {"emerald-900", "#064e3b"}, {"emerald-950", "#022c22"},

    // Teal
    {"teal-50", "#f0fdfa"}, {"teal-100", "#ccfbf1"}, {"teal-200", "#99f6e4"},
    {"teal-300", "#5eead4"}, {"teal-400", "#2dd4bf"}, {"teal-500", "#14b8a6"},
    {"teal-600", "#0d9488"}, {"teal-700", "#0f766e"}, {"teal-800", "#115e59"},
    {"teal-900", "#134e4a"}, {"teal-950", "#042f2e"},

    // Cyan
    {"cyan-50", "#ecfeff"}, {"cyan-100", "#cffafe"}, {"cyan-200", "#a5f3fc"},
    {"cyan-300", "#67e8f9"}, {"cyan-400", "#22d3ee"}, {"cyan-500", "#06b6d4"},
    {"cyan-600", "#0891b2"}, {"cyan-700", "#0e7490"}, {"cyan-800", "#155e75"},
    {"cyan-900", "#164e63"}, {"cyan-950", "#083344"},

    // Sky
    {"sky-50", "#f0f9ff"}, {"sky-100", "#e0f2fe"}, {"sky-200", "#bae6fd"},
    {"sky-300", "#7dd3fc"}, {"sky-400", "#38bdf8"}, {"sky-500", "#0ea5e9"},
    {"sky-600", "#0284c7"}, {"sky-700", "#0369a1"}, {"sky-800", "#075985"},
    {"sky-900", "#0c4a6e"}, {"sky-950", "#082f49"},

    // Blue
    {"blue-50", "#eff6ff"}, {"blue-100", "#dbeafe"}, {"blue-200", "#bfdbfe"},
    {"blue-300", "#93c5fd"}, {"blue-400", "#60a5fa"}, {"blue-500", "#3b82f6"},
    {"blue-600", "#2563eb"}, {"blue-700", "#1d4ed8"}, {"blue-800", "#1e40af"},
    {"blue-900", "#1e3a8a"}, {"blue-950", "#172554"},

    // Indigo
    {"indigo-50", "#eef2ff"}, {"indigo-100", "#e0e7ff"}, {"indigo-200", "#c7d2fe"},
    {"indigo-300", "#a5b4fc"}, {"indigo-400", "#818cf8"}, {"indigo-500", "#6366f1"},
    {"indigo-600", "#4f46e5"}, {"indigo-700", "#4338ca"}, {"indigo-800", "#3730a3"},
    {"indigo-900", "#312e81"}, {"indigo-950", "#1e1b4b"},

    // Violet
    {"violet-50", "#f5f3ff"}, {"violet-100", "#ede9fe"}, {"violet-200", "#ddd6fe"},
    {"violet-300", "#c4b5fd"}, {"violet-400", "#a78bfa"}, {"violet-500", "#8b5cf6"},
    {"violet-600", "#7c3aed"}, {"violet-700", "#6d28d9"}, {"violet-800", "#5b21b6"},
    {"violet-900", "#4c1d95"}, {"violet-950", "#2e1065"},

    // Purple
    {"purple-50", "#faf5ff"}, {"purple-100", "#f3e8ff"}, {"purple-200", "#e9d5ff"},
    {"purple-300", "#d8b4fe"}, {"purple-400", "#c084fc"}, {"purple-500", "#a855f7"},
    {"purple-600", "#9333ea"}, {"purple-700", "#7e22ce"}, {"purple-800", "#6b21a8"},
    {"purple-900", "#581c87"}, {"purple-950", "#3b0764"},

    // Fuchsia
    {"fuchsia-50", "#fdf4ff"}, {"fuchsia-100", "#fae8ff"}, {"fuchsia-200", "#f5d0fe"},
    {"fuchsia-300", "#f0abfc"}, {"fuchsia-400", "#e879f9"}, {"fuchsia-500", "#d946ef"},
    {"fuchsia-600", "#c026d3"}, {"fuchsia-700", "#a21caf"}, {"fuchsia-800", "#86198f"},
    {"fuchsia-900", "#701a75"}, {"fuchsia-950", "#4a044e"},

    // Pink
    {"pink-50", "#fdf2f8"}, {"pink-100", "#fce7f3"}, {"pink-200", "#fbcfe8"},
    {"pink-300", "#f472b6"}, {"pink-400", "#f472b6"}, {"pink-500", "#ec4899"},
    {"pink-600", "#db2777"}, {"pink-700", "#be185d"}, {"pink-800", "#9d174d"},
    {"pink-900", "#831843"}, {"pink-950", "#500724"},

    // Rose
    {"rose-50", "#fff1f2"}, {"rose-100", "#ffe4e6"}, {"rose-200", "#fecdd3"},
    {"rose-300", "#fda4af"}, {"rose-400", "#fb7185"}, {"rose-500", "#f43f5e"},
    {"rose-600", "#e11d48"}, {"rose-700", "#be123c"}, {"rose-800", "#9f1239"},
    {"rose-900", "#881337"}, {"rose-950", "#4c0519"},

    // Basics
    {"black", "#000000"},
    {"white", "#ffffff"},
    {"transparent", "#00000000"}
};

inline bool resolveColorToken(const std::string &raw, ColorRgba &out)
{
    std::string token = raw;
    float alpha = 1.0f;

    size_t slash = token.find('/');
    if (slash != std::string::npos)
    {
        std::string opStr = token.substr(slash + 1);
        token = token.substr(0, slash);
        if (!opStr.empty())
        {
            if (opStr.front() == '[' && opStr.back() == ']') {
                opStr = opStr.substr(1, opStr.size() - 2);
            }
            try {
                float v = std::stof(opStr);
                if (v > 1.0f) v /= 100.0f; // /50 -> 0.50
                alpha = std::clamp(v, 0.0f, 1.0f);
            } catch (...) {}
        }
    }

    if (!token.empty() && token.front() == '[' && token.back() == ']')
    {
        std::string arb = token.substr(1, token.size() - 2);
        if (!arb.empty() && arb[0] == '#') {
            out = hexToRgba(arb, alpha);
            return true;
        }
    }

    auto it = TAILWIND_COLORS.find(token);
    if (it != TAILWIND_COLORS.end())
    {
        out = hexToRgba(it->second, alpha);
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// ComputedStyle: Node styling specification
// ---------------------------------------------------------------------------

enum class DisplayMode { BLOCK, FLEX, GRID, INLINE };
enum class FlexDir { ROW, COLUMN };
enum class AlignItems { START, CENTER, END, STRETCH };
enum class JustifyContent { START, CENTER, END, BETWEEN, AROUND, EVENLY };

struct ComputedStyle
{
    DisplayMode display = DisplayMode::BLOCK;
    FlexDir flexDirection = FlexDir::COLUMN;
    AlignItems alignItems = AlignItems::STRETCH;
    JustifyContent justifyContent = JustifyContent::START;
    bool flexWrap = false;
    float flexGrow = 0.0f;

    int gridCols = 1;
    int colSpan = 1;

    float padding[4] = {0, 0, 0, 0}; // top, right, bottom, left
    float margin[4] = {0, 0, 0, 0};
    float gapX = 0;
    float gapY = 0;

    float width = -1; // -1 = auto
    float height = -1;
    float widthPercent = -1; // e.g. 100 for w-full, 50 for w-1/2
    float heightPercent = -1;
    float maxWidth = -1;
    float minHeight = -1;

    float borderRadius = 0;
    float borderWidth = 0;
    ColorRgba borderColor = ColorRgba(0, 0, 0, 0, "#000000");

    ColorRgba backgroundColor = ColorRgba(0, 0, 0, 0, "#000000");
    bool hasBackground = false;

    ColorRgba textColor = ColorRgba(1, 1, 1, 1, "#ffffff");
    bool hasExplicitTextColor = false;

    float fontSize = 16.0f;
    bool hasExplicitFontSize = false;

    int fontWeight = 400; // 400 = normal, 600 = semibold, 700 = bold, 800 = extrabold
    bool hasExplicitFontWeight = false;

    bool isUppercase = false;
    float letterSpacing = 0;
    bool isLineThrough = false;

    bool hasShadow = false;
};

// ---------------------------------------------------------------------------
// Universal Rule-Based Tailwind Parser
// ---------------------------------------------------------------------------

inline float parseNumericOrArbitrary(const std::string &valStr, float multiplier = 4.0f)
{
    if (valStr.empty()) return 0;
    if (valStr.front() == '[' && valStr.back() == ']') {
        std::string inner = valStr.substr(1, valStr.size() - 2);
        if (inner.find("px") != std::string::npos) {
            return std::stof(inner.substr(0, inner.find("px")));
        }
        if (inner.find("rem") != std::string::npos) {
            return std::stof(inner.substr(0, inner.find("rem"))) * 16.0f;
        }
        try {
            return std::stof(inner);
        } catch (...) {
            return 0;
        }
    }
    try {
        return std::stof(valStr) * multiplier;
    } catch (...) {
        return 0;
    }
}

inline void applyTailwindClass(const std::string &cls, ComputedStyle &style, float viewportW = 1000.0f)
{
    std::string token = cls;

    // Check responsive prefixes: sm: (640), md: (768), lg: (1024), xl: (1280)
    if (token.rfind("sm:", 0) == 0) {
        if (viewportW < 640.0f) return;
        token = token.substr(3);
    } else if (token.rfind("md:", 0) == 0) {
        if (viewportW < 768.0f) return;
        token = token.substr(3);
    } else if (token.rfind("lg:", 0) == 0) {
        if (viewportW < 1024.0f) return;
        token = token.substr(3);
    } else if (token.rfind("xl:", 0) == 0) {
        if (viewportW < 1280.0f) return;
        token = token.substr(3);
    }

    // Skip hover/active/dark prefixes for base style (can be stored on interactive state)
    if (token.rfind("hover:", 0) == 0 || token.rfind("active:", 0) == 0 || token.rfind("focus:", 0) == 0 || token.rfind("dark:", 0) == 0) {
        return;
    }

    // Display
    if (token == "flex") { style.display = DisplayMode::FLEX; style.flexDirection = FlexDir::ROW; return; }
    if (token == "grid") { style.display = DisplayMode::GRID; return; }
    if (token == "block") { style.display = DisplayMode::BLOCK; return; }
    if (token == "inline") { style.display = DisplayMode::INLINE; return; }
    if (token == "flex-row") { style.display = DisplayMode::FLEX; style.flexDirection = FlexDir::ROW; return; }
    if (token == "flex-col") { style.display = DisplayMode::FLEX; style.flexDirection = FlexDir::COLUMN; return; }
    if (token == "flex-wrap") { style.flexWrap = true; return; }
    if (token == "flex-1") { style.flexGrow = 1.0f; return; }

    // Flex Alignment
    if (token == "items-center") { style.alignItems = AlignItems::CENTER; return; }
    if (token == "items-start") { style.alignItems = AlignItems::START; return; }
    if (token == "items-end") { style.alignItems = AlignItems::END; return; }
    if (token == "justify-between") { style.justifyContent = JustifyContent::BETWEEN; return; }
    if (token == "justify-center") { style.justifyContent = JustifyContent::CENTER; return; }
    if (token == "justify-start") { style.justifyContent = JustifyContent::START; return; }
    if (token == "justify-end") { style.justifyContent = JustifyContent::END; return; }
    if (token == "justify-around") { style.justifyContent = JustifyContent::AROUND; return; }

    // CSS Grid
    if (token.rfind("grid-cols-", 0) == 0) {
        style.display = DisplayMode::GRID;
        try { style.gridCols = std::stoi(token.substr(10)); } catch (...) {}
        return;
    }
    if (token.rfind("col-span-", 0) == 0) {
        try { style.colSpan = std::stoi(token.substr(9)); } catch (...) {}
        return;
    }

    // Gap
    if (token.rfind("gap-", 0) == 0) {
        float g = parseNumericOrArbitrary(token.substr(4));
        style.gapX = g;
        style.gapY = g;
        return;
    }
    if (token.rfind("gap-x-", 0) == 0) {
        style.gapX = parseNumericOrArbitrary(token.substr(6));
        return;
    }
    if (token.rfind("gap-y-", 0) == 0) {
        style.gapY = parseNumericOrArbitrary(token.substr(6));
        return;
    }
    if (token.rfind("space-x-", 0) == 0) {
        style.gapX = parseNumericOrArbitrary(token.substr(8));
        return;
    }
    if (token.rfind("space-y-", 0) == 0) {
        style.gapY = parseNumericOrArbitrary(token.substr(8));
        return;
    }

    // Padding
    if (token.rfind("p-", 0) == 0) {
        float p = parseNumericOrArbitrary(token.substr(2));
        for (int i = 0; i < 4; ++i) style.padding[i] = p;
        return;
    }
    if (token.rfind("px-", 0) == 0) {
        float p = parseNumericOrArbitrary(token.substr(3));
        style.padding[1] = p; style.padding[3] = p;
        return;
    }
    if (token.rfind("py-", 0) == 0) {
        float p = parseNumericOrArbitrary(token.substr(3));
        style.padding[0] = p; style.padding[2] = p;
        return;
    }
    if (token.rfind("pt-", 0) == 0) { style.padding[0] = parseNumericOrArbitrary(token.substr(3)); return; }
    if (token.rfind("pr-", 0) == 0) { style.padding[1] = parseNumericOrArbitrary(token.substr(3)); return; }
    if (token.rfind("pb-", 0) == 0) { style.padding[2] = parseNumericOrArbitrary(token.substr(3)); return; }
    if (token.rfind("pl-", 0) == 0) { style.padding[3] = parseNumericOrArbitrary(token.substr(3)); return; }

    // Margin
    if (token.rfind("m-", 0) == 0) {
        float m = parseNumericOrArbitrary(token.substr(2));
        for (int i = 0; i < 4; ++i) style.margin[i] = m;
        return;
    }
    if (token.rfind("mx-", 0) == 0) {
        float m = parseNumericOrArbitrary(token.substr(3));
        style.margin[1] = m; style.margin[3] = m;
        return;
    }
    if (token.rfind("my-", 0) == 0) {
        float m = parseNumericOrArbitrary(token.substr(3));
        style.margin[0] = m; style.margin[2] = m;
        return;
    }
    if (token.rfind("mt-", 0) == 0) { style.margin[0] = parseNumericOrArbitrary(token.substr(3)); return; }
    if (token.rfind("mr-", 0) == 0) { style.margin[1] = parseNumericOrArbitrary(token.substr(3)); return; }
    if (token.rfind("mb-", 0) == 0) { style.margin[2] = parseNumericOrArbitrary(token.substr(3)); return; }
    if (token.rfind("ml-", 0) == 0) { style.margin[3] = parseNumericOrArbitrary(token.substr(3)); return; }

    // Width & Max Width
    if (token == "w-full") { style.widthPercent = 100.0f; return; }
    if (token == "w-1/2") { style.widthPercent = 50.0f; return; }
    if (token == "w-1/3") { style.widthPercent = 33.33f; return; }
    if (token == "w-2/3") { style.widthPercent = 66.67f; return; }
    if (token == "w-1/4") { style.widthPercent = 25.0f; return; }
    if (token == "w-3/4") { style.widthPercent = 75.0f; return; }
    if (token.rfind("w-", 0) == 0) {
        style.width = parseNumericOrArbitrary(token.substr(2));
        return;
    }
    if (token == "max-w-xs") { style.maxWidth = 320.0f; return; }
    if (token == "max-w-sm") { style.maxWidth = 384.0f; return; }
    if (token == "max-w-md") { style.maxWidth = 448.0f; return; }
    if (token == "max-w-lg") { style.maxWidth = 512.0f; return; }
    if (token == "max-w-xl") { style.maxWidth = 576.0f; return; }
    if (token == "max-w-2xl") { style.maxWidth = 672.0f; return; }
    if (token.rfind("max-w-", 0) == 0) {
        style.maxWidth = parseNumericOrArbitrary(token.substr(6));
        return;
    }

    // Height & Min Height
    if (token == "h-full") { style.heightPercent = 100.0f; return; }
    if (token == "min-h-screen") { style.minHeight = 800.0f; return; }
    if (token.rfind("h-", 0) == 0) {
        style.height = parseNumericOrArbitrary(token.substr(2));
        return;
    }

    // Border Radius
    if (token == "rounded-none") { style.borderRadius = 0; return; }
    if (token == "rounded-sm") { style.borderRadius = 2; return; }
    if (token == "rounded") { style.borderRadius = 4; return; }
    if (token == "rounded-md") { style.borderRadius = 6; return; }
    if (token == "rounded-lg") { style.borderRadius = 8; return; }
    if (token == "rounded-xl") { style.borderRadius = 12; return; }
    if (token == "rounded-2xl") { style.borderRadius = 16; return; }
    if (token == "rounded-3xl") { style.borderRadius = 24; return; }
    if (token == "rounded-full") { style.borderRadius = 9999; return; }
    if (token.rfind("rounded-[", 0) == 0) {
        style.borderRadius = parseNumericOrArbitrary(token.substr(8), 1.0f);
        return;
    }

    // Border Width & Color
    if (token == "border") { style.borderWidth = 1.0f; return; }
    if (token == "border-2") { style.borderWidth = 2.0f; return; }
    if (token == "border-4") { style.borderWidth = 4.0f; return; }
    if (token == "border-b") { style.borderWidth = 1.0f; return; }
    if (token.rfind("border-", 0) == 0) {
        ColorRgba c;
        if (resolveColorToken(token.substr(7), c)) {
            style.borderColor = c;
            if (style.borderWidth == 0) style.borderWidth = 1.0f;
        }
        return;
    }

    // Background Color
    if (token.rfind("bg-", 0) == 0) {
        ColorRgba c;
        if (resolveColorToken(token.substr(3), c)) {
            style.backgroundColor = c;
            style.hasBackground = true;
        }
        return;
    }

    // Text Color
    if (token.rfind("text-", 0) == 0) {
        std::string sub = token.substr(5);
        if (sub == "xs") { style.fontSize = 12.0f; style.hasExplicitFontSize = true; return; }
        if (sub == "sm") { style.fontSize = 14.0f; style.hasExplicitFontSize = true; return; }
        if (sub == "base") { style.fontSize = 16.0f; style.hasExplicitFontSize = true; return; }
        if (sub == "lg") { style.fontSize = 18.0f; style.hasExplicitFontSize = true; return; }
        if (sub == "xl") { style.fontSize = 20.0f; style.hasExplicitFontSize = true; return; }
        if (sub == "2xl") { style.fontSize = 24.0f; style.hasExplicitFontSize = true; return; }
        if (sub == "3xl") { style.fontSize = 30.0f; style.hasExplicitFontSize = true; return; }
        if (sub == "4xl") { style.fontSize = 36.0f; style.hasExplicitFontSize = true; return; }
        if (sub == "5xl") { style.fontSize = 48.0f; style.hasExplicitFontSize = true; return; }
        if (sub.front() == '[' && sub.back() == ']') {
            style.fontSize = parseNumericOrArbitrary(sub, 1.0f);
            style.hasExplicitFontSize = true;
            return;
        }
        ColorRgba c;
        if (resolveColorToken(sub, c)) {
            style.textColor = c;
            style.hasExplicitTextColor = true;
            return;
        }
    }

    // Typography & Weight
    if (token == "font-normal") { style.fontWeight = 400; style.hasExplicitFontWeight = true; return; }
    if (token == "font-medium") { style.fontWeight = 500; style.hasExplicitFontWeight = true; return; }
    if (token == "font-semibold") { style.fontWeight = 600; style.hasExplicitFontWeight = true; return; }
    if (token == "font-bold") { style.fontWeight = 700; style.hasExplicitFontWeight = true; return; }
    if (token == "font-extrabold") { style.fontWeight = 800; style.hasExplicitFontWeight = true; return; }
    if (token == "uppercase") { style.isUppercase = true; return; }
    if (token == "line-through") { style.isLineThrough = true; return; }
    if (token == "tracking-wider") { style.letterSpacing = 1.0f; return; }
    if (token == "tracking-tight") { style.letterSpacing = -0.5f; return; }

    // Shadow
    if (token.rfind("shadow", 0) == 0) { style.hasShadow = true; return; }
}

inline ComputedStyle parseTailwindStyle(const std::string &twClasses, float viewportW = 1000.0f)
{
    ComputedStyle style;
    std::stringstream ss(twClasses);
    std::string token;
    while (ss >> token)
    {
        applyTailwindClass(token, style, viewportW);
    }
    return style;
}

// ---------------------------------------------------------------------------
// Direct GPU Draw Command Buffer & Layout Solver
// ---------------------------------------------------------------------------

struct GpuDrawCmd
{
    std::string type; // "FILL_ROUND_RECT", "STROKE_ROUND_RECT", "DRAW_TEXT", "CLEAR_RECT"
    float x = 0;
    float y = 0;
    float width = 0;
    float height = 0;
    float radius = 0;
    std::string color = "#000000";
    float alpha = 1.0f;
    float lineWidth = 1.0f;
    std::string text = "";
    float fontSize = 16.0f;
    int fontWeight = 400;
    int nodeId = 0;
};

struct HitRegion
{
    int id = 0;
    float x = 0;
    float y = 0;
    float width = 0;
    float height = 0;
    std::string tag = "";
    std::string cursor = "default";
    bool hasClick = false;
    bool hasChange = false;
    Value onClick = Value(std::monostate{});
    Value onChange = Value(std::monostate{});
    std::string inputValue = "";
};

inline float computeNaturalContentWidth(const Value &nodeVal, const ComputedStyle &inheritedStyle, float viewportW = 1000.0f) {
    Value d = nodeVal.deref();
    if (d.isNil() || (std::holds_alternative<bool>(d.data) && !std::get<bool>(d.data))) return 0.0f;
    if (d.isList()) {
        float maxW = 0.0f;
        for (const auto &child : d.getList()->elements) {
            maxW = std::max(maxW, computeNaturalContentWidth(child, inheritedStyle, viewportW));
        }
        return maxW;
    }
    if (!d.isMap()) {
        std::string txt = d.toString();
        return txt.size() * (inheritedStyle.fontSize * 0.58f);
    }
    auto map = d.getMap();
    std::string tag = map->has("tag") ? map->entries["tag"].deref().toString() : "";
    std::string twClass = map->has("class") ? map->entries["class"].deref().toString() : "";
    if (twClass.empty() && map->has("props") && map->entries["props"].deref().isMap()) {
        auto pm = map->entries["props"].deref().getMap();
        if (pm->has("class")) twClass = pm->entries["class"].deref().toString();
        else if (pm->has("className")) twClass = pm->entries["className"].deref().toString();
    }
    ComputedStyle style = parseTailwindStyle(twClass, viewportW);
    if (!style.hasExplicitFontSize) style.fontSize = inheritedStyle.fontSize;
    if (tag == "h1" && !style.hasExplicitFontSize) style.fontSize = 30.0f;
    if (tag == "h2" && !style.hasExplicitFontSize) style.fontSize = 24.0f;
    if (tag == "h3" && !style.hasExplicitFontSize) style.fontSize = 20.0f;
    if (tag == "p" && !style.hasExplicitFontSize) style.fontSize = 14.0f;

    if (style.width > 0) return style.width;

    std::vector<Value> children;
    if (map->has("children")) {
        Value chVal = map->entries["children"].deref();
        if (chVal.isList()) children = chVal.getList()->elements;
        else if (!chVal.isNil()) children.push_back(chVal);
    }

    if (tag == "input" && children.empty()) {
        return 160.0f + style.padding[1] + style.padding[3];
    }

    float contentW = 0.0f;
    if (style.display == DisplayMode::FLEX && style.flexDirection == FlexDir::ROW) {
        for (const auto &c : children) {
            contentW += computeNaturalContentWidth(c, style, viewportW) + style.gapX;
        }
    } else {
        for (const auto &c : children) {
            contentW = std::max(contentW, computeNaturalContentWidth(c, style, viewportW));
        }
    }
    float totalW = contentW + style.padding[1] + style.padding[3];
    if (style.maxWidth > 0 && totalW > style.maxWidth) totalW = style.maxWidth;
    return totalW;
}

inline void solveNodeLayout(
    const Value &nodeVal,
    float parentX,
    float parentY,
    float parentW,
    const ComputedStyle &inheritedStyle,
    std::vector<GpuDrawCmd> &cmds,
    float &outW,
    float &outH,
    float viewportW = 1000.0f,
    std::vector<HitRegion> *outHitRegions = nullptr,
    int *pNodeIdCounter = nullptr)
{
    Value d = nodeVal.deref();
    if (d.isNil() || (std::holds_alternative<bool>(d.data) && !std::get<bool>(d.data))) {
        outW = 0;
        outH = 0;
        return;
    }
    if (d.isList()) {
        float curY = parentY;
        float maxW = 0.0f;
        for (const auto &child : d.getList()->elements) {
            float cW = 0, cH = 0;
            solveNodeLayout(child, parentX, curY, parentW, inheritedStyle, cmds, cW, cH, viewportW, outHitRegions, pNodeIdCounter);
            curY += cH;
            maxW = std::max(maxW, cW);
        }
        outW = maxW;
        outH = curY - parentY;
        return;
    }
    if (!d.isMap()) {
        // Plain text node
        std::string txt = d.toString();
        if (txt.empty()) {
            outW = 0;
            outH = 0;
            return;
        }
        float txtW = txt.size() * (inheritedStyle.fontSize * 0.55f);
        float txtH = inheritedStyle.fontSize * 1.3f;
        GpuDrawCmd cmd;
        cmd.type = "DRAW_TEXT";
        cmd.x = parentX;
        cmd.y = parentY + inheritedStyle.fontSize;
        cmd.text = txt;
        cmd.fontSize = inheritedStyle.fontSize;
        cmd.fontWeight = inheritedStyle.fontWeight;
        cmd.color = inheritedStyle.textColor.hex;
        cmd.alpha = inheritedStyle.textColor.a;
        cmds.push_back(cmd);

        if (inheritedStyle.isLineThrough) {
            GpuDrawCmd strikeCmd;
            strikeCmd.type = "DRAW_LINE";
            strikeCmd.nodeId = 0;
            strikeCmd.x = parentX;
            strikeCmd.y = parentY + inheritedStyle.fontSize * 0.65f;
            strikeCmd.width = txtW;
            strikeCmd.height = 0;
            strikeCmd.color = inheritedStyle.textColor.hex;
            strikeCmd.alpha = 0.85f;
            strikeCmd.lineWidth = 1.5f;
            cmds.push_back(strikeCmd);
        }

        outW = txtW;
        outH = txtH;
        return;
    }

    auto map = d.getMap();
    std::string tag = map->has("tag") ? map->entries["tag"].deref().toString() : "";
    std::string twClass = map->has("class") ? map->entries["class"].deref().toString() : "";
    if (twClass.empty() && map->has("props") && map->entries["props"].deref().isMap()) {
        auto pm = map->entries["props"].deref().getMap();
        if (pm->has("class")) twClass = pm->entries["class"].deref().toString();
        else if (pm->has("className")) twClass = pm->entries["className"].deref().toString();
    }

    ComputedStyle style = parseTailwindStyle(twClass, viewportW);

    // Cascading text inheritance from parent
    if (!style.hasExplicitTextColor) style.textColor = inheritedStyle.textColor;
    if (!style.hasExplicitFontSize) style.fontSize = inheritedStyle.fontSize;
    if (!style.hasExplicitFontWeight) style.fontWeight = inheritedStyle.fontWeight;
    if (inheritedStyle.isLineThrough) style.isLineThrough = true;

    // Apply default tag styles
    if (tag == "h1" && !style.hasExplicitFontSize) { style.fontSize = 30.0f; style.fontWeight = 700; }
    if (tag == "h2" && !style.hasExplicitFontSize) { style.fontSize = 24.0f; style.fontWeight = 700; }
    if (tag == "h3" && !style.hasExplicitFontSize) { style.fontSize = 20.0f; style.fontWeight = 600; }
    if (tag == "p" && !style.hasExplicitFontSize) { style.fontSize = 14.0f; }

    float nodeX = parentX + style.margin[3];
    float nodeY = parentY + style.margin[0];

    // Compute node width
    float nodeW = parentW - style.margin[1] - style.margin[3];
    if (style.widthPercent > 0) {
        nodeW = parentW * (style.widthPercent / 100.0f);
    } else if (style.width > 0) {
        nodeW = style.width;
    }
    if (style.maxWidth > 0 && nodeW > style.maxWidth) {
        nodeW = style.maxWidth;
    }

    // Children resolution
    std::vector<Value> children;
    if (map->has("children")) {
        Value chVal = map->entries["children"].deref();
        if (chVal.isList()) {
            children = chVal.getList()->elements;
        } else if (!chVal.isNil()) {
            children.push_back(chVal);
        }
    }

    // Compute natural width for inline / button / span / flex row items
    bool isFlexChild = (inheritedStyle.display == DisplayMode::FLEX && inheritedStyle.flexDirection == FlexDir::ROW);
    bool isNaturalInline = (tag == "button" || tag == "span" || tag == "a" || tag == "label" || tag == "b" || tag == "i" || tag == "strong" || style.display == DisplayMode::INLINE);

    if (style.width <= 0 && style.widthPercent <= 0 && style.flexGrow == 0.0f && (isNaturalInline || isFlexChild)) {
        float naturalW = computeNaturalContentWidth(nodeVal, inheritedStyle, viewportW);
        if (naturalW > 0.0f && naturalW < nodeW) {
            nodeW = naturalW;
        }
    }

    std::shared_ptr<MemeMap> pm = nullptr;
    if (map->has("props") && map->entries["props"].deref().isMap()) {
        pm = map->entries["props"].deref().getMap();
    }

    // Resolve event handlers in pre-order before children layout
    Value clickHandler = Value(std::monostate{});
    Value changeHandler = Value(std::monostate{});
    if (pm) {
        if (pm->has("onClick")) clickHandler = pm->entries["onClick"];
        else if (pm->has("onclick")) clickHandler = pm->entries["onclick"];
        if (pm->has("onChange")) changeHandler = pm->entries["onChange"];
        else if (pm->has("onchange")) changeHandler = pm->entries["onchange"];
        else if (pm->has("onInput")) changeHandler = pm->entries["onInput"];
    }
    if (map->has("onClick")) clickHandler = map->entries["onClick"];
    else if (map->has("onclick")) clickHandler = map->entries["onclick"];
    if (map->has("onChange")) changeHandler = map->entries["onChange"];
    else if (map->has("onchange")) changeHandler = map->entries["onchange"];

    bool hasClick = (tag == "button" || tag == "a" || !clickHandler.deref().isNil());
    bool hasChange = (tag == "input" || tag == "textarea" || tag == "select" || !changeHandler.deref().isNil());

    int nodeId = 0;
    if (outHitRegions && (hasClick || hasChange)) {
        nodeId = pNodeIdCounter ? ++(*pNodeIdCounter) : 1;
    }

    float innerX = nodeX + style.padding[3];
    float innerY = nodeY + style.padding[0];
    float innerW = std::max(0.0f, nodeW - style.padding[1] - style.padding[3]);

    float contentH = 0.0f;
    size_t bgIndex = cmds.size();

    // Layout children based on display mode
    if (style.display == DisplayMode::GRID && style.gridCols > 0) {
        int cols = style.gridCols;
        float colW = (innerW - (cols - 1) * style.gapX) / cols;
        float curRowY = innerY;
        float maxRowH = 0.0f;
        int colIdx = 0;

        for (const auto &child : children) {
            float cW = 0, cH = 0;
            float childAllocW = colW * style.colSpan;
            float childX = innerX + colIdx * (colW + style.gapX);

            solveNodeLayout(child, childX, curRowY, childAllocW, style, cmds, cW, cH, viewportW, outHitRegions, pNodeIdCounter);
            maxRowH = std::max(maxRowH, cH);
            colIdx += style.colSpan;
            if (colIdx >= cols) {
                curRowY += maxRowH + style.gapY;
                contentH += maxRowH + style.gapY;
                maxRowH = 0;
                colIdx = 0;
            }
        }
        contentH += maxRowH;
    } else if (style.display == DisplayMode::FLEX && style.flexDirection == FlexDir::ROW) {
        float curX = innerX;
        float maxChildH = 0.0f;
        float totalFixedW = 0.0f;
        float totalFlexGrow = 0.0f;

        // First pass: measure child natural widths for flex/justify (no hit regions, no draw cmds)
        std::vector<std::pair<float, float>> sizes;
        std::vector<float> flexGrows;
        for (const auto &child : children) {
            float cW = 0, cH = 0;
            std::vector<GpuDrawCmd> dummy;
            solveNodeLayout(child, 0, 0, innerW, style, dummy, cW, cH, viewportW, nullptr, nullptr);
            sizes.push_back({cW, cH});

            float fg = 0.0f;
            Value cd = child.deref();
            if (cd.isMap()) {
                auto cm = cd.getMap();
                std::string cls = "";
                if (cm->has("class")) cls = cm->entries["class"].deref().toString();
                else if (cm->has("className")) cls = cm->entries["className"].deref().toString();
                else if (cm->has("props") && cm->entries["props"].deref().isMap()) {
                    auto pm = cm->entries["props"].deref().getMap();
                    if (pm->has("class")) cls = pm->entries["class"].deref().toString();
                    else if (pm->has("className")) cls = pm->entries["className"].deref().toString();
                }
                if (cls.find("flex-1") != std::string::npos || cls.find("grow") != std::string::npos) fg = 1.0f;
            }
            flexGrows.push_back(fg);
            if (fg > 0.0f) {
                totalFlexGrow += fg;
            } else {
                totalFixedW += cW;
            }
            maxChildH = std::max(maxChildH, cH);
        }

        float totalGaps = (children.size() > 1) ? (style.gapX * (children.size() - 1)) : 0.0f;
        float availableFlexSpace = std::max(0.0f, innerW - totalFixedW - totalGaps);

        // Distribute flexGrow space to flex-1 children
        for (size_t i = 0; i < children.size(); ++i) {
            if (flexGrows[i] > 0.0f && totalFlexGrow > 0.0f) {
                sizes[i].first = availableFlexSpace * (flexGrows[i] / totalFlexGrow);
            }
        }

        float totalAllocatedW = 0.0f;
        for (const auto &s : sizes) totalAllocatedW += s.first;
        float leftoverSpace = std::max(0.0f, innerW - totalAllocatedW);
        float extraBetween = (children.size() > 1 && style.justifyContent == JustifyContent::BETWEEN && totalFlexGrow == 0.0f)
            ? (leftoverSpace / (children.size() - 1)) : style.gapX;

        for (size_t i = 0; i < children.size(); ++i) {
            float cW = 0, cH = 0;
            float childY = innerY;
            if (style.alignItems == AlignItems::CENTER) {
                childY += (maxChildH - sizes[i].second) / 2.0f;
            }
            solveNodeLayout(children[i], curX, childY, sizes[i].first, style, cmds, cW, cH, viewportW, outHitRegions, pNodeIdCounter);
            curX += sizes[i].first + extraBetween;
        }
        contentH = maxChildH;
    } else {
        // Flex Column or Block stacking
        float curY = innerY;
        for (size_t i = 0; i < children.size(); ++i) {
            if (i > 0) curY += style.gapY;
            float cW = 0, cH = 0;
            float childX = innerX;
            if (style.alignItems == AlignItems::CENTER) {
                std::vector<GpuDrawCmd> dummy;
                solveNodeLayout(children[i], 0, 0, innerW, style, dummy, cW, cH, viewportW, nullptr, nullptr);
                childX = innerX + std::max(0.0f, (innerW - cW) / 2.0f);
            }
            solveNodeLayout(children[i], childX, curY, innerW, style, cmds, cW, cH, viewportW, outHitRegions, pNodeIdCounter);
            curY += cH;
        }
        contentH = curY - innerY;
    }

    if (tag == "input") {
        std::string val = "";
        std::string placeholder = "";
        if (pm) {
            if (pm->has("value")) val = pm->entries["value"].deref().toString();
            if (pm->has("placeholder")) placeholder = pm->entries["placeholder"].deref().toString();
        }
        std::string displayText = !val.empty() ? val : placeholder;
        bool isPlaceholder = val.empty() && !placeholder.empty();

        float inputFontH = style.fontSize * 1.3f;
        if (contentH < inputFontH) contentH = inputFontH;

        if (!displayText.empty()) {
            GpuDrawCmd textCmd;
            textCmd.type = "DRAW_TEXT";
            textCmd.nodeId = 0;
            textCmd.x = innerX;
            textCmd.y = innerY + style.fontSize;
            textCmd.text = displayText;
            textCmd.fontSize = style.fontSize;
            textCmd.fontWeight = style.fontWeight;
            textCmd.color = isPlaceholder ? "#64748b" : style.textColor.hex;
            textCmd.alpha = isPlaceholder ? 0.7f : style.textColor.a;
            cmds.push_back(textCmd);
        }
    }

    float finalH = contentH + style.padding[0] + style.padding[2];
    if (style.height > 0) finalH = style.height;
    if (style.minHeight > 0 && finalH < style.minHeight) finalH = style.minHeight;

    if (outHitRegions && nodeId > 0) {
        HitRegion hr;
        hr.id = nodeId;
        hr.x = nodeX;
        hr.y = nodeY;
        hr.width = nodeW;
        hr.height = finalH;
        hr.tag = tag;
        hr.cursor = hasClick ? "pointer" : "default";
        hr.hasClick = hasClick;
        hr.hasChange = hasChange;
        hr.onClick = clickHandler;
        hr.onChange = changeHandler;
        if (pm && pm->has("value")) hr.inputValue = pm->entries["value"].deref().toString();
        outHitRegions->push_back(hr);
    }

    // Insert Background and Border draw calls at node box
    if (style.hasBackground) {
        GpuDrawCmd bgCmd;
        bgCmd.type = "FILL_ROUND_RECT";
        bgCmd.nodeId = nodeId;
        bgCmd.x = nodeX;
        bgCmd.y = nodeY;
        bgCmd.width = nodeW;
        bgCmd.height = finalH;
        bgCmd.radius = style.borderRadius;
        bgCmd.color = style.backgroundColor.hex;
        bgCmd.alpha = style.backgroundColor.a;
        // Insert background before child commands
        cmds.insert(cmds.begin() + bgIndex, bgCmd);
    }

    if (style.borderWidth > 0) {
        GpuDrawCmd bCmd;
        bCmd.type = "STROKE_ROUND_RECT";
        bCmd.nodeId = nodeId;
        bCmd.x = nodeX;
        bCmd.y = nodeY;
        bCmd.width = nodeW;
        bCmd.height = finalH;
        bCmd.radius = style.borderRadius;
        bCmd.lineWidth = style.borderWidth;
        bCmd.color = style.borderColor.hex;
        bCmd.alpha = style.borderColor.a;
        cmds.push_back(bCmd);
    }

    outW = nodeW;
    outH = finalH + style.margin[0] + style.margin[2];
}

inline std::vector<GpuDrawCmd> computeNativeLayout(
    const Value &jsxTree,
    float viewportW = 1000.0f,
    float viewportH = 800.0f,
    std::vector<HitRegion> *outHitRegions = nullptr)
{
    (void)viewportH;
    std::vector<GpuDrawCmd> cmds;
    ComputedStyle rootStyle;
    float outW = 0, outH = 0;
    int nodeIdCounter = 0;
    solveNodeLayout(jsxTree, 0, 0, viewportW, rootStyle, cmds, outW, outH, viewportW, outHitRegions, &nodeIdCounter);
    return cmds;
}

inline std::string escapeJsonStr(const std::string &input)
{
    std::string out;
    for (char c : input)
    {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

inline std::string hitRegionsToJson(const std::vector<HitRegion> &regions)
{
    std::string json = "[\n";
    for (size_t i = 0; i < regions.size(); ++i)
    {
        const auto &r = regions[i];
        json += "  {\"id\": " + std::to_string(r.id) + ", ";
        json += "\"x\": " + std::to_string(r.x) + ", ";
        json += "\"y\": " + std::to_string(r.y) + ", ";
        json += "\"width\": " + std::to_string(r.width) + ", ";
        json += "\"height\": " + std::to_string(r.height) + ", ";
        json += "\"tag\": \"" + r.tag + "\", ";
        json += "\"nodeType\": \"" + r.tag + "\", ";
        json += "\"cursor\": \"" + r.cursor + "\", ";
        json += "\"hasClick\": " + std::string(r.hasClick ? "true" : "false") + ", ";
        json += "\"hasChange\": " + std::string(r.hasChange ? "true" : "false") + ", ";
        json += "\"hasOnClick\": " + std::string(r.hasClick ? "true" : "false") + ", ";
        json += "\"hasOnChange\": " + std::string(r.hasChange ? "true" : "false") + ", ";
        json += "\"value\": \"" + escapeJsonStr(r.inputValue) + "\"}";
        if (i + 1 < regions.size()) json += ",\n";
        else json += "\n";
    }
    json += "]";
    return json;
}

inline std::string gpuCommandsToJson(const std::vector<GpuDrawCmd> &cmds)
{
    std::string json = "[\n";
    for (size_t i = 0; i < cmds.size(); ++i)
    {
        const auto &c = cmds[i];
        json += "  {\"type\": \"" + c.type + "\", ";
        json += "\"nodeId\": " + std::to_string(c.nodeId) + ", ";
        json += "\"x\": " + std::to_string(c.x) + ", ";
        json += "\"y\": " + std::to_string(c.y) + ", ";
        json += "\"width\": " + std::to_string(c.width) + ", ";
        json += "\"height\": " + std::to_string(c.height) + ", ";
        json += "\"radius\": " + std::to_string(c.radius) + ", ";
        json += "\"lineWidth\": " + std::to_string(c.lineWidth) + ", ";
        json += "\"color\": \"" + c.color + "\", ";
        json += "\"alpha\": " + std::to_string(c.alpha) + ", ";
        json += "\"fontSize\": " + std::to_string(c.fontSize) + ", ";
        json += "\"fontWeight\": " + std::to_string(c.fontWeight) + ", ";

        std::string escText = "";
        for (char ch : c.text)
        {
            if (ch == '"') escText += "\\\"";
            else if (ch == '\\') escText += "\\\\";
            else if (ch == '\n') escText += "\\n";
            else escText += ch;
        }
        json += "\"text\": \"" + escText + "\"}";
        if (i + 1 < cmds.size()) json += ",\n";
        else json += "\n";
    }
    json += "]";
    return json;
}

} // namespace TailwindEngine

#endif // TAILWIND_ENGINE_H
