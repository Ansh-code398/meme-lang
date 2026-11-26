#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <cstdint>
#include <memory>

namespace MemeUI {

struct ColorRgba {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;
    std::string hex = "#000000";

    ColorRgba() = default;
    ColorRgba(float r, float g, float b, float a = 1.0f, std::string hex = "")
        : r(r), g(g), b(b), a(a), hex(std::move(hex)) {}

    uint32_t toUint32() const {
        uint32_t ri = static_cast<uint32_t>(std::clamp(r, 0.0f, 1.0f) * 255.0f + 0.5f);
        uint32_t gi = static_cast<uint32_t>(std::clamp(g, 0.0f, 1.0f) * 255.0f + 0.5f);
        uint32_t bi = static_cast<uint32_t>(std::clamp(b, 0.0f, 1.0f) * 255.0f + 0.5f);
        uint32_t ai = static_cast<uint32_t>(std::clamp(a, 0.0f, 1.0f) * 255.0f + 0.5f);
        return (ai << 24) | (bi << 16) | (gi << 8) | ri;
    }

    ColorRgba withAlpha(float newAlpha) const {
        return ColorRgba(r, g, b, std::clamp(newAlpha, 0.0f, 1.0f), hex);
    }
};

inline ColorRgba hexToRgba(const std::string &hexStr, float alpha = 1.0f) {
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

inline const std::unordered_map<std::string, std::string> &getTailwindColors() {
    static const std::unordered_map<std::string, std::string> colors = {
        {"slate-50", "#f8fafc"}, {"slate-100", "#f1f5f9"}, {"slate-200", "#e2e8f0"},
        {"slate-300", "#cbd5e1"}, {"slate-400", "#94a3b8"}, {"slate-500", "#64748b"},
        {"slate-600", "#475569"}, {"slate-700", "#334155"}, {"slate-800", "#1e293b"},
        {"slate-900", "#0f172a"}, {"slate-950", "#020617"},

        {"gray-50", "#f9fafb"}, {"gray-100", "#f3f4f6"}, {"gray-200", "#e5e7eb"},
        {"gray-300", "#d1d5db"}, {"gray-400", "#9ca3af"}, {"gray-500", "#6b7280"},
        {"gray-600", "#4b5563"}, {"gray-700", "#374151"}, {"gray-800", "#1f2937"},
        {"gray-900", "#111827"}, {"gray-950", "#030712"},

        {"zinc-50", "#fafafa"}, {"zinc-100", "#f4f4f5"}, {"zinc-200", "#e4e4e7"},
        {"zinc-300", "#d4d4d8"}, {"zinc-400", "#a1a1aa"}, {"zinc-500", "#71717a"},
        {"zinc-600", "#52525b"}, {"zinc-700", "#3f3f46"}, {"zinc-800", "#27272a"},
        {"zinc-900", "#18181b"}, {"zinc-950", "#09090b"},

        {"neutral-50", "#fafafa"}, {"neutral-100", "#f5f5f5"}, {"neutral-200", "#e5e5e5"},
        {"neutral-300", "#d4d4d4"}, {"neutral-400", "#a3a3a3"}, {"neutral-500", "#737373"},
        {"neutral-600", "#525252"}, {"neutral-700", "#404040"}, {"neutral-800", "#262626"},
        {"neutral-900", "#171717"}, {"neutral-950", "#0a0a0a"},

        {"stone-50", "#fafaf9"}, {"stone-100", "#f5f5f4"}, {"stone-200", "#e7e5e4"},
        {"stone-300", "#d6d3d1"}, {"stone-400", "#a8a29e"}, {"stone-500", "#78716c"},
        {"stone-600", "#57534e"}, {"stone-700", "#44403c"}, {"stone-800", "#292524"},
        {"stone-900", "#1c1917"}, {"stone-950", "#0c0a09"},

        {"red-50", "#fef2f2"}, {"red-100", "#fee2e2"}, {"red-200", "#fecaca"},
        {"red-300", "#fca5a5"}, {"red-400", "#f87171"}, {"red-500", "#ef4444"},
        {"red-600", "#dc2626"}, {"red-700", "#b91c1c"}, {"red-800", "#991b1b"},
        {"red-900", "#7f1d1d"}, {"red-950", "#450a0a"},

        {"orange-50", "#fff7ed"}, {"orange-100", "#ffedd5"}, {"orange-200", "#fed7aa"},
        {"orange-300", "#fdba74"}, {"orange-400", "#fb923c"}, {"orange-500", "#f97316"},
        {"orange-600", "#ea580c"}, {"orange-700", "#c2410c"}, {"orange-800", "#9a3412"},
        {"orange-900", "#7c2d12"}, {"orange-950", "#431407"},

        {"amber-50", "#fffbeb"}, {"amber-100", "#fef3c7"}, {"amber-200", "#fde68a"},
        {"amber-300", "#fcd34d"}, {"amber-400", "#fbbf24"}, {"amber-500", "#f59e0b"},
        {"amber-600", "#d97706"}, {"amber-700", "#b45309"}, {"amber-800", "#92400e"},
        {"amber-900", "#78350f"}, {"amber-950", "#451a03"},

        {"yellow-50", "#fefce8"}, {"yellow-100", "#fef9c3"}, {"yellow-200", "#fef08a"},
        {"yellow-300", "#fde047"}, {"yellow-400", "#facc15"}, {"yellow-500", "#eab308"},
        {"yellow-600", "#ca8a04"}, {"yellow-700", "#a16207"}, {"yellow-800", "#854d0e"},
        {"yellow-900", "#713f12"}, {"yellow-950", "#422006"},

        {"lime-50", "#f7fee7"}, {"lime-100", "#ecfccb"}, {"lime-200", "#d9f99d"},
        {"lime-300", "#bef264"}, {"lime-400", "#a3e635"}, {"lime-500", "#84cc16"},
        {"lime-600", "#65a30d"}, {"lime-700", "#4d7c0f"}, {"lime-800", "#3f6212"},
        {"lime-900", "#365314"}, {"lime-950", "#1a2e05"},

        {"green-50", "#f0fdf4"}, {"green-100", "#dcfce7"}, {"green-200", "#bbf7d0"},
        {"green-300", "#86efac"}, {"green-400", "#4ade80"}, {"green-500", "#22c55e"},
        {"green-600", "#16a34a"}, {"green-700", "#15803d"}, {"green-800", "#166534"},
        {"green-900", "#14532d"}, {"green-950", "#052e16"},

        {"emerald-50", "#ecfdf5"}, {"emerald-100", "#d1fae5"}, {"emerald-200", "#a7f3d0"},
        {"emerald-300", "#6ee7b7"}, {"emerald-400", "#34d399"}, {"emerald-500", "#10b981"},
        {"emerald-600", "#059669"}, {"emerald-700", "#047857"}, {"emerald-800", "#065f46"},
        {"emerald-900", "#064e3b"}, {"emerald-950", "#022c22"},

        {"teal-50", "#f0fdfa"}, {"teal-100", "#ccfbf1"}, {"teal-200", "#99f6e4"},
        {"teal-300", "#5eead4"}, {"teal-400", "#2dd4bf"}, {"teal-500", "#14b8a6"},
        {"teal-600", "#0d9488"}, {"teal-700", "#0f766e"}, {"teal-800", "#115e59"},
        {"teal-900", "#134e4a"}, {"teal-950", "#042f2e"},

        {"cyan-50", "#ecfeff"}, {"cyan-100", "#cffafe"}, {"cyan-200", "#a5f3fc"},
        {"cyan-300", "#67e8f9"}, {"cyan-400", "#22d3ee"}, {"cyan-500", "#06b6d4"},
        {"cyan-600", "#0891b2"}, {"cyan-700", "#0e7490"}, {"cyan-800", "#155e75"},
        {"cyan-900", "#164e63"}, {"cyan-950", "#083344"},

        {"sky-50", "#f0f9ff"}, {"sky-100", "#e0f2fe"}, {"sky-200", "#bae6fd"},
        {"sky-300", "#7dd3fc"}, {"sky-400", "#38bdf8"}, {"sky-500", "#0ea5e9"},
        {"sky-600", "#0284c7"}, {"sky-700", "#0369a1"}, {"sky-800", "#075985"},
        {"sky-900", "#0c4a6e"}, {"sky-950", "#082f49"},

        {"blue-50", "#eff6ff"}, {"blue-100", "#dbeafe"}, {"blue-200", "#bfdbfe"},
        {"blue-300", "#93c5fd"}, {"blue-400", "#60a5fa"}, {"blue-500", "#3b82f6"},
        {"blue-600", "#2563eb"}, {"blue-700", "#1d4ed8"}, {"blue-800", "#1e40af"},
        {"blue-900", "#1e3a8a"}, {"blue-950", "#172554"},

        {"indigo-50", "#eef2ff"}, {"indigo-100", "#e0e7ff"}, {"indigo-200", "#c7d2fe"},
        {"indigo-300", "#a5b4fc"}, {"indigo-400", "#818cf8"}, {"indigo-500", "#6366f1"},
        {"indigo-600", "#4f46e5"}, {"indigo-700", "#4338ca"}, {"indigo-800", "#3730a3"},
        {"indigo-900", "#312e81"}, {"indigo-950", "#1e1b4b"},

        {"violet-50", "#f5f3ff"}, {"violet-100", "#ede9fe"}, {"violet-200", "#ddd6fe"},
        {"violet-300", "#c4b5fd"}, {"violet-400", "#a78bfa"}, {"violet-500", "#8b5cf6"},
        {"violet-600", "#7c3aed"}, {"violet-700", "#6d28d9"}, {"violet-800", "#5b21b6"},
        {"violet-900", "#4c1d95"}, {"violet-950", "#2e1065"},

        {"purple-50", "#faf5ff"}, {"purple-100", "#f3e8ff"}, {"purple-200", "#e9d5ff"},
        {"purple-300", "#d8b4fe"}, {"purple-400", "#c084fc"}, {"purple-500", "#a855f7"},
        {"purple-600", "#9333ea"}, {"purple-700", "#7e22ce"}, {"purple-800", "#6b21a8"},
        {"purple-900", "#581c87"}, {"purple-950", "#3b0764"},

        {"fuchsia-50", "#fdf4ff"}, {"fuchsia-100", "#fae8ff"}, {"fuchsia-200", "#f5d0fe"},
        {"fuchsia-300", "#f0abfc"}, {"fuchsia-400", "#e879f9"}, {"fuchsia-500", "#d946ef"},
        {"fuchsia-600", "#c026d3"}, {"fuchsia-700", "#a21caf"}, {"fuchsia-800", "#86198f"},
        {"fuchsia-900", "#701a75"}, {"fuchsia-950", "#4a044e"},

        {"pink-50", "#fdf2f8"}, {"pink-100", "#fce7f3"}, {"pink-200", "#fbcfe8"},
        {"pink-300", "#f9a8d4"}, {"pink-400", "#f472b6"}, {"pink-500", "#ec4899"},
        {"pink-600", "#db2777"}, {"pink-700", "#be185d"}, {"pink-800", "#9d174d"},
        {"pink-900", "#831843"}, {"pink-950", "#500724"},

        {"rose-50", "#fff1f2"}, {"rose-100", "#ffe4e6"}, {"rose-200", "#fecdd3"},
        {"rose-300", "#fda4af"}, {"rose-400", "#fb7185"}, {"rose-500", "#f43f5e"},
        {"rose-600", "#e11d48"}, {"rose-700", "#be123c"}, {"rose-800", "#9f1239"},
        {"rose-900", "#881337"}, {"rose-950", "#4c0519"},

        {"black", "#000000"},
        {"white", "#ffffff"},
        {"transparent", "#00000000"}
    };
    return colors;
}

inline bool resolveColorToken(const std::string &raw, ColorRgba &out) {
    std::string token = raw;
    float alpha = 1.0f;

    size_t slash = token.find('/');
    if (slash != std::string::npos) {
        std::string opStr = token.substr(slash + 1);
        token = token.substr(0, slash);
        if (!opStr.empty()) {
            if (opStr.front() == '[' && opStr.back() == ']') {
                opStr = opStr.substr(1, opStr.size() - 2);
            }
            try {
                float v = std::stof(opStr);
                if (v > 1.0f) v /= 100.0f;
                alpha = std::clamp(v, 0.0f, 1.0f);
            } catch (...) {}
        }
    }

    if (!token.empty() && token.front() == '[' && token.back() == ']') {
        std::string arb = token.substr(1, token.size() - 2);
        if (!arb.empty() && arb[0] == '#') {
            out = hexToRgba(arb, alpha);
            return true;
        }
    }
    if (!token.empty() && token[0] == '#') {
        out = hexToRgba(token, alpha);
        return true;
    }

    const auto &colors = getTailwindColors();
    auto it = colors.find(token);
    if (it != colors.end()) {
        out = hexToRgba(it->second, alpha);
        return true;
    }
    return false;
}

enum class DisplayMode { BLOCK, FLEX, GRID, INLINE, INLINE_FLEX, NONE };
enum class FlexDir { ROW, COLUMN, ROW_REVERSE, COLUMN_REVERSE };
enum class AlignItems { START, CENTER, END, STRETCH, BASELINE };
enum class JustifyContent { START, CENTER, END, BETWEEN, AROUND, EVENLY };
#ifdef RELATIVE
#undef RELATIVE
#endif
#ifdef ABSOLUTE
#undef ABSOLUTE
#endif
enum class PositionMode { STATIC, RELATIVE, ABSOLUTE, FIXED };
enum class OverflowMode { VISIBLE, HIDDEN, SCROLL, AUTO };
enum class TextAlign { LEFT, CENTER, RIGHT };
enum class FontFamily { SANS, MONO, SERIF };

struct ComputedStyle {
    DisplayMode display = DisplayMode::BLOCK;
    FlexDir flexDirection = FlexDir::COLUMN;
    AlignItems alignItems = AlignItems::STRETCH;
    AlignItems alignSelf = AlignItems::STRETCH;
    bool hasAlignSelf = false;
    JustifyContent justifyContent = JustifyContent::START;
    bool flexWrap = false;
    float flexGrow = 0.0f;
    float flexShrink = 1.0f;
    float flexBasis = -1.0f;
    int order = 0;

    int gridCols = 1;
    int gridRows = 0;
    int colSpan = 1;
    int rowSpan = 1;

    float padding[4] = {0, 0, 0, 0}; // top, right, bottom, left
    float margin[4] = {0, 0, 0, 0};
    bool marginAuto[4] = {false, false, false, false};
    float gapX = 0;
    float gapY = 0;

    float width = -1;
    float height = -1;
    float widthPercent = -1;
    float heightPercent = -1;
    float heightVh = -1;
    float minWidth = -1;
    float maxWidth = -1;
    float minHeight = -1;
    float maxHeight = -1;
    float maxHeightPercent = -1;
    float maxHeightVh = -1;
    bool minHScreen = false;

    PositionMode position = PositionMode::STATIC;
    float posTop = NAN;
    float posRight = NAN;
    float posBottom = NAN;
    float posLeft = NAN;
    int zIndex = 0;
    bool hasExplicitZIndex = false;

    OverflowMode overflowX = OverflowMode::VISIBLE;
    OverflowMode overflowY = OverflowMode::VISIBLE;
    bool overscrollContain = false;

    bool isRtl = false;
    bool hasExplicitRtl = false;

    float borderRadius = 0;
    float borderRadii[4] = {0, 0, 0, 0}; // tl, tr, br, bl
    bool hasPerCornerRadius = false;

    float borderWidth = 0;
    float borderSides[4] = {0, 0, 0, 0}; // top, right, bottom, left
    bool hasPerSideBorder = false;
    ColorRgba borderColor = ColorRgba(0, 0, 0, 0, "#000000");

    ColorRgba backgroundColor = ColorRgba(0, 0, 0, 0, "#000000");
    bool hasBackground = false;

    bool hasGradient = false;
    float gradientAngle = 45.0f;
    ColorRgba gradientFrom = ColorRgba(0, 0, 0, 0, "#000000");
    ColorRgba gradientVia = ColorRgba(0, 0, 0, 0, "#000000");
    bool hasGradientVia = false;
    ColorRgba gradientTo = ColorRgba(0, 0, 0, 0, "#000000");

    float opacity = 1.0f;

    ColorRgba textColor = ColorRgba(1, 1, 1, 1, "#ffffff");
    bool hasExplicitTextColor = false;

    float fontSize = 16.0f;
    bool hasExplicitFontSize = false;

    int fontWeight = 400;
    bool hasExplicitFontWeight = false;
    FontFamily fontFamily = FontFamily::SANS;
    bool hasExplicitFontFamily = false;
    TextAlign textAlign = TextAlign::LEFT;
    bool hasExplicitTextAlign = false;

    bool isItalic = false;
    bool isUppercase = false;
    bool isLowercase = false;
    float letterSpacing = 0;
    float lineHeightMult = 1.3f;
    bool isLineThrough = false;
    bool isUnderline = false;
    bool truncate = false;

    bool hasShadow = false;
    float shadowBlur = 0.0f;
    float shadowOffsetY = 0.0f;
    float shadowAlpha = 0.25f;

    std::string cursor = "";
    bool pointerEventsNone = false;
    bool userSelectNone = false;
    bool srOnly = false;
};

struct UiPoint {
    float x = 0;
    float y = 0;
};

struct GpuDrawCmd {
    std::string type;
    float x = 0;
    float y = 0;
    float width = 0;
    float height = 0;
    float radius = 0;
    float radii[4] = {0, 0, 0, 0};
    std::string color = "#000000";
    float alpha = 1.0f;
    std::string color2 = "#000000";
    float alpha2 = 1.0f;
    float angle = 0.0f;
    float lineWidth = 1.0f;
    std::string text = "";
    float fontSize = 16.0f;
    int fontWeight = 400;
    int nodeId = 0;
    int zIndex = 0;
    bool isMono = false;
    bool isItalic = false;
    bool isUnderline = false;
    bool isStrike = false;
    bool filled = true;
    float blur = 0.0f;
    std::string svgPath = "";
    std::string imageId = "";
    std::vector<UiPoint> points;
};

struct UiRect {
    float x = 0;
    float y = 0;
    float width = 0;
    float height = 0;

    bool contains(float px, float py) const {
        return px >= x && px <= x + width && py >= y && py <= y + height;
    }

    UiRect intersect(const UiRect &o) const {
        float x1 = std::max(x, o.x);
        float y1 = std::max(y, o.y);
        float x2 = std::min(x + width, o.x + o.width);
        float y2 = std::min(y + height, o.y + o.height);
        return {x1, y1, std::max(0.0f, x2 - x1), std::max(0.0f, y2 - y1)};
    }
};

struct UiHitRegion {
    int id = 0;
    std::string nodeKey;
    float x = 0;
    float y = 0;
    float width = 0;
    float height = 0;
    UiRect clipRect = {0, 0, 1e6f, 1e6f};
    int zIndex = 0;
    std::string tag = "";
    std::string role = "";
    std::string accessibleName = "";
    std::string testId = "";
    std::string cursor = "default";
    bool disabled = false;
    bool readOnly = false;
    bool focused = false;
    bool hovered = false;
    bool pressed = false;
    bool focusable = false;
    int tabIndex = 0;
    bool hasClick = false;
    bool hasChange = false;
    bool hasKeyDown = false;
    bool hasPointerDown = false;
    bool hasScroll = false;
    bool isScrollContainer = false;
    bool overscrollContain = false;
    bool hasWheelHandler = false;
    float scrollX = 0;
    float scrollY = 0;
    float maxScrollX = 0;
    float maxScrollY = 0;
    float contentW = 0;
    float contentH = 0;
    std::string inputValue = "";
    std::string placeholder = "";
    bool isPassword = false;
    bool isMultiline = false;
    bool checked = false;
    bool indeterminate = false;
    bool selected = false;
    bool expanded = false;
    double numValue = 0.0;
    double numMin = 0.0;
    double numMax = 100.0;
    double numStep = 1.0;
    bool isRtl = false;
    std::unordered_map<std::string, std::string> ariaProps;
};

struct UiEvent {
    std::string type;      // "click", "change", "input", "enter", "keydown", "keyup", "focus", "blur", "pointerdown", "pointerup", "pointermove", "scroll", "reorder", "resize"
    int targetId = 0;
    std::string value;
    double numValue = 0.0;
    bool boolValue = false;
    float x = 0;
    float y = 0;
    float deltaX = 0;
    float deltaY = 0;
    int button = 0;
    int clickCount = 1;
    std::string key;
    bool shiftKey = false;
    bool ctrlKey = false;
    bool altKey = false;
    bool metaKey = false;
};

struct UiKeyEvent {
    std::string key;       // "Enter", "Escape", "Tab", "ArrowUp", "ArrowDown", "ArrowLeft", "ArrowRight", "Home", "End", "PageUp", "PageDown", "Backspace", "Delete", "Space", "a", ...
    std::string text;      // UTF-8 inserted text if printable
    bool shift = false;
    bool ctrl = false;
    bool alt = false;
    bool meta = false;
    bool repeat = false;
    bool isDown = true;
};

struct UiInputState {
    float pointerX = -1.0f;
    float pointerY = -1.0f;
    float prevPointerX = -1.0f;
    float prevPointerY = -1.0f;
    bool mouseDown[3] = {false, false, false};
    bool mousePressed[3] = {false, false, false};
    bool mouseReleased[3] = {false, false, false};
    int clickCount = 1;
    float wheelDeltaX = 0.0f;
    float wheelDeltaY = 0.0f;
    std::vector<UiKeyEvent> keyEvents;
    std::string imePreedit = "";
    std::string clipboardText = "";
    float viewportW = 960.0f;
    float viewportH = 720.0f;
    float dpiScale = 1.0f;
    float safeTop = 0.0f;
    float safeBottom = 0.0f;
    float safeLeft = 0.0f;
    float safeRight = 0.0f;
    bool rtl = false;
    double elapsedMs = 0.0;
    double deltaMs = 16.67;
};

struct UiPainter;

struct UiNode {
    std::string tag = "div";
    std::string text = "";
    bool isText = false;
    std::string key = "";
    std::string idAttr = "";
    std::string testId = "";
    std::string role = "";
    std::string ariaLabel = "";
    std::string classStr = "";
    std::string dir = "";
    bool disabled = false;
    bool readOnly = false;
    bool checked = false;
    bool indeterminate = false;
    bool selected = false;
    bool expanded = false;
    bool open = false;
    bool isPortal = false;
    int tabIndex = -999;
    int zIndex = 0;

    std::unordered_map<std::string, std::string> props;
    std::unordered_map<std::string, double> numProps;
    std::unordered_map<std::string, bool> boolProps;
    std::unordered_map<std::string, std::string> aria;
    std::unordered_map<std::string, std::function<void(const UiEvent &)>> handlers;
    std::function<void(UiPainter &, UiRect, const UiInputState &)> onPaint = nullptr;
    std::vector<UiNode> children;

    bool hasProp(const std::string &k) const {
        return props.find(k) != props.end();
    }
    std::string getProp(const std::string &k, const std::string &def = "") const {
        auto it = props.find(k);
        return it != props.end() ? it->second : def;
    }
    double getNumProp(const std::string &k, double def = 0.0) const {
        auto it = numProps.find(k);
        if (it != numProps.end()) return it->second;
        auto pit = props.find(k);
        if (pit != props.end()) {
            try { return std::stod(pit->second); } catch (...) {}
        }
        return def;
    }
    bool getBoolProp(const std::string &k, bool def = false) const {
        auto it = boolProps.find(k);
        if (it != boolProps.end()) return it->second;
        auto pit = props.find(k);
        if (pit != props.end()) {
            return pit->second == "true" || pit->second == "han" || pit->second == "sach" || pit->second == "1";
        }
        return def;
    }
};

struct UiPainter {
    std::vector<GpuDrawCmd> *cmds = nullptr;
    int baseZ = 0;

    void rectFilled(float x, float y, float w, float h, float radius, const std::string &color, float alpha = 1.0f) {
        if (!cmds) return;
        GpuDrawCmd c;
        c.type = "FILL_ROUND_RECT";
        c.x = x; c.y = y; c.width = w; c.height = h;
        c.radius = radius; c.color = color; c.alpha = alpha; c.zIndex = baseZ;
        cmds->push_back(std::move(c));
    }

    void rectStroke(float x, float y, float w, float h, float radius, float lineWidth, const std::string &color, float alpha = 1.0f) {
        if (!cmds) return;
        GpuDrawCmd c;
        c.type = "STROKE_ROUND_RECT";
        c.x = x; c.y = y; c.width = w; c.height = h;
        c.radius = radius; c.lineWidth = lineWidth; c.color = color; c.alpha = alpha; c.zIndex = baseZ;
        cmds->push_back(std::move(c));
    }

    void circleFilled(float cx, float cy, float r, const std::string &color, float alpha = 1.0f) {
        if (!cmds) return;
        GpuDrawCmd c;
        c.type = "DRAW_CIRCLE";
        c.x = cx - r; c.y = cy - r; c.width = r * 2.0f; c.height = r * 2.0f;
        c.radius = r; c.color = color; c.alpha = alpha; c.filled = true; c.zIndex = baseZ;
        cmds->push_back(std::move(c));
    }

    void circleStroke(float cx, float cy, float r, float lineWidth, const std::string &color, float alpha = 1.0f) {
        if (!cmds) return;
        GpuDrawCmd c;
        c.type = "STROKE_CIRCLE";
        c.x = cx - r; c.y = cy - r; c.width = r * 2.0f; c.height = r * 2.0f;
        c.radius = r; c.lineWidth = lineWidth; c.color = color; c.alpha = alpha; c.filled = false; c.zIndex = baseZ;
        cmds->push_back(std::move(c));
    }

    void line(float x1, float y1, float x2, float y2, float lineWidth, const std::string &color, float alpha = 1.0f) {
        if (!cmds) return;
        GpuDrawCmd c;
        c.type = "DRAW_LINE";
        c.x = x1; c.y = y1; c.width = x2 - x1; c.height = y2 - y1;
        c.lineWidth = lineWidth; c.color = color; c.alpha = alpha; c.zIndex = baseZ;
        cmds->push_back(std::move(c));
    }

    void polyline(const std::vector<UiPoint> &pts, float lineWidth, const std::string &color, float alpha = 1.0f) {
        if (!cmds || pts.size() < 2) return;
        GpuDrawCmd c;
        c.type = "DRAW_POLYLINE";
        c.points = pts;
        c.lineWidth = lineWidth; c.color = color; c.alpha = alpha; c.filled = false; c.zIndex = baseZ;
        cmds->push_back(std::move(c));
    }

    void polygon(const std::vector<UiPoint> &pts, const std::string &color, float alpha = 1.0f) {
        if (!cmds || pts.size() < 3) return;
        GpuDrawCmd c;
        c.type = "FILL_POLYGON";
        c.points = pts;
        c.color = color; c.alpha = alpha; c.filled = true; c.zIndex = baseZ;
        cmds->push_back(std::move(c));
    }

    void text(float x, float y, const std::string &txt, float fontSize, const std::string &color, int fontWeight = 400, bool mono = false, float alpha = 1.0f) {
        if (!cmds || txt.empty()) return;
        GpuDrawCmd c;
        c.type = "DRAW_TEXT";
        c.x = x; c.y = y + fontSize;
        c.text = txt; c.fontSize = fontSize; c.fontWeight = fontWeight;
        c.color = color; c.alpha = alpha; c.isMono = mono; c.zIndex = baseZ;
        cmds->push_back(std::move(c));
    }

    void svgPath(float x, float y, float w, float h, const std::string &d, const std::string &color, bool filled = false, float strokeW = 1.5f, float alpha = 1.0f) {
        if (!cmds || d.empty()) return;
        GpuDrawCmd c;
        c.type = "DRAW_SVG_PATH";
        c.x = x; c.y = y; c.width = w; c.height = h;
        c.svgPath = d; c.color = color; c.filled = filled; c.lineWidth = strokeW; c.alpha = alpha; c.zIndex = baseZ;
        cmds->push_back(std::move(c));
    }

    void pushClip(float x, float y, float w, float h) {
        if (!cmds) return;
        GpuDrawCmd c;
        c.type = "PUSH_CLIP";
        c.x = x; c.y = y; c.width = w; c.height = h; c.zIndex = baseZ;
        cmds->push_back(std::move(c));
    }

    void popClip() {
        if (!cmds) return;
        GpuDrawCmd c;
        c.type = "POP_CLIP";
        c.zIndex = baseZ;
        cmds->push_back(std::move(c));
    }
};

} // namespace MemeUI
