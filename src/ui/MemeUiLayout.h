#pragma once

#include "MemeUiTypes.h"
#include "MemeUiTextEngine.h"
#include "MemeUiFontEngine.h"
#include "MemeIconsRegistry.h"
#include "MemeUiAnimation.h"
#include "MemeUiPrimitives.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cmath>

namespace MemeUI {

struct UiInteractionState {
    int focusedId = 0;
    std::string focusedKey = "";
    bool focusVisible = false;
    int hoveredId = 0;
    std::string hoveredKey = "";
    int activeId = 0;
    std::string activeKey = "";
    int activeThumbIdx = 0;
    bool draggingSlider = false;
    bool draggingScrollbar = false;
    bool draggingScrollbarHorizontal = false;
    float scrollbarDragOffset = 0.0f;
    int draggingResizeId = 0;

    std::unordered_map<std::string, TextFieldState> textFields;
    std::unordered_map<std::string, float> scrollOffsetsY;
    std::unordered_map<std::string, float> scrollOffsetsX;
    std::unordered_map<std::string, UiRect> nodeBoundsByKey;
    UiAnimationStore animStore;

    TextFieldState &getOrCreateTextField(const std::string &key, const std::string &propVal, bool multiline, bool password, bool readOnly, bool disabled) {
        auto &tf = textFields[key];
        tf.multiline = multiline;
        tf.password = password;
        tf.readOnly = readOnly;
        tf.disabled = disabled;
        if (!tf.dragging) {
            tf.syncFromProp(propVal);
        }
        return tf;
    }
};

inline float parseTwNumericOrArb(const std::string &valStr, float multiplier = 4.0f) {
    if (valStr.empty()) return 0.0f;
    if (valStr.front() == '[' && valStr.back() == ']') {
        std::string inner = valStr.substr(1, valStr.size() - 2);
        size_t px = inner.find("px");
        if (px != std::string::npos) {
            try { return std::stof(inner.substr(0, px)); } catch (...) { return 0.0f; }
        }
        size_t rem = inner.find("rem");
        if (rem != std::string::npos) {
            try { return std::stof(inner.substr(0, rem)) * 16.0f; } catch (...) { return 0.0f; }
        }
        try { return std::stof(inner); } catch (...) { return 0.0f; }
    }
    try {
        size_t end = 0;
        float v = std::stof(valStr, &end);
        if (end != valStr.size()) return 0.0f;
        return v * multiplier;
    } catch (...) { return 0.0f; }
}

inline void applySingleTailwindToken(const std::string &token, ComputedStyle &style) {
    if (token.empty()) return;

    // Display
    if (token == "hidden") { style.display = DisplayMode::NONE; return; }
    if (token == "flex") { style.display = DisplayMode::FLEX; style.flexDirection = FlexDir::ROW; return; }
    if (token == "inline-flex") { style.display = DisplayMode::INLINE_FLEX; style.flexDirection = FlexDir::ROW; return; }
    if (token == "grid") { style.display = DisplayMode::GRID; return; }
    if (token == "block") { style.display = DisplayMode::BLOCK; return; }
    if (token == "inline" || token == "inline-block") { style.display = DisplayMode::INLINE; return; }
    if (token == "flex-row") { style.display = DisplayMode::FLEX; style.flexDirection = FlexDir::ROW; return; }
    if (token == "flex-col") { style.display = DisplayMode::FLEX; style.flexDirection = FlexDir::COLUMN; return; }
    if (token == "flex-row-reverse") { style.display = DisplayMode::FLEX; style.flexDirection = FlexDir::ROW_REVERSE; return; }
    if (token == "flex-col-reverse") { style.display = DisplayMode::FLEX; style.flexDirection = FlexDir::COLUMN_REVERSE; return; }
    if (token == "flex-wrap") { style.flexWrap = true; return; }
    if (token == "flex-nowrap") { style.flexWrap = false; return; }
    if (token == "flex-1") { style.flexGrow = 1.0f; style.flexShrink = 1.0f; style.flexBasis = 0.0f; return; }
    if (token == "flex-auto") { style.flexGrow = 1.0f; style.flexShrink = 1.0f; return; }
    if (token == "flex-none") { style.flexGrow = 0.0f; style.flexShrink = 0.0f; return; }
    if (token == "grow" || token == "flex-grow") { style.flexGrow = 1.0f; return; }
    if (token == "grow-0") { style.flexGrow = 0.0f; return; }
    if (token == "shrink" || token == "flex-shrink") { style.flexShrink = 1.0f; return; }
    if (token == "shrink-0" || token == "flex-shrink-0") { style.flexShrink = 0.0f; return; }
    if (token.rfind("basis-", 0) == 0) { style.flexBasis = parseTwNumericOrArb(token.substr(6)); return; }
    if (token.rfind("order-", 0) == 0) { try { style.order = std::stoi(token.substr(6)); } catch (...) {} return; }

    // Alignment
    if (token == "items-center") { style.alignItems = AlignItems::CENTER; return; }
    if (token == "items-start") { style.alignItems = AlignItems::START; return; }
    if (token == "items-end") { style.alignItems = AlignItems::END; return; }
    if (token == "items-stretch") { style.alignItems = AlignItems::STRETCH; return; }
    if (token == "items-baseline") { style.alignItems = AlignItems::BASELINE; return; }
    if (token == "self-center") { style.alignSelf = AlignItems::CENTER; style.hasAlignSelf = true; return; }
    if (token == "self-start") { style.alignSelf = AlignItems::START; style.hasAlignSelf = true; return; }
    if (token == "self-end") { style.alignSelf = AlignItems::END; style.hasAlignSelf = true; return; }
    if (token == "self-stretch") { style.alignSelf = AlignItems::STRETCH; style.hasAlignSelf = true; return; }

    if (token == "justify-between") { style.justifyContent = JustifyContent::BETWEEN; return; }
    if (token == "justify-center") { style.justifyContent = JustifyContent::CENTER; return; }
    if (token == "justify-start") { style.justifyContent = JustifyContent::START; return; }
    if (token == "justify-end") { style.justifyContent = JustifyContent::END; return; }
    if (token == "justify-around") { style.justifyContent = JustifyContent::AROUND; return; }
    if (token == "justify-evenly") { style.justifyContent = JustifyContent::EVENLY; return; }

    // Grid
    if (token.rfind("grid-cols-", 0) == 0) {
        style.display = DisplayMode::GRID;
        try { style.gridCols = std::max(1, std::stoi(token.substr(10))); } catch (...) {}
        return;
    }
    if (token.rfind("col-span-", 0) == 0) {
        try { style.colSpan = std::max(1, std::stoi(token.substr(9))); } catch (...) {}
        return;
    }

    // Gap / Space
    if (token.rfind("gap-x-", 0) == 0) { style.gapX = parseTwNumericOrArb(token.substr(6)); return; }
    if (token.rfind("gap-y-", 0) == 0) { style.gapY = parseTwNumericOrArb(token.substr(6)); return; }
    if (token.rfind("gap-", 0) == 0) { float g = parseTwNumericOrArb(token.substr(4)); style.gapX = g; style.gapY = g; return; }
    if (token.rfind("space-x-", 0) == 0) { style.gapX = parseTwNumericOrArb(token.substr(8)); return; }
    if (token.rfind("space-y-", 0) == 0) { style.gapY = parseTwNumericOrArb(token.substr(8)); return; }

    // Padding
    if (token.rfind("p-", 0) == 0) { float p = parseTwNumericOrArb(token.substr(2)); for (int i=0;i<4;++i) style.padding[i] = p; return; }
    if (token.rfind("px-", 0) == 0) { float p = parseTwNumericOrArb(token.substr(3)); style.padding[1] = p; style.padding[3] = p; return; }
    if (token.rfind("py-", 0) == 0) { float p = parseTwNumericOrArb(token.substr(3)); style.padding[0] = p; style.padding[2] = p; return; }
    if (token.rfind("pt-", 0) == 0) { style.padding[0] = parseTwNumericOrArb(token.substr(3)); return; }
    if (token.rfind("pr-", 0) == 0) { style.padding[1] = parseTwNumericOrArb(token.substr(3)); return; }
    if (token.rfind("pb-", 0) == 0) { style.padding[2] = parseTwNumericOrArb(token.substr(3)); return; }
    if (token.rfind("pl-", 0) == 0) { style.padding[3] = parseTwNumericOrArb(token.substr(3)); return; }
    if (token.rfind("ps-", 0) == 0) { style.padding[style.isRtl ? 1 : 3] = parseTwNumericOrArb(token.substr(3)); return; }
    if (token.rfind("pe-", 0) == 0) { style.padding[style.isRtl ? 3 : 1] = parseTwNumericOrArb(token.substr(3)); return; }

    // Margin
    if (token == "mx-auto") { style.marginAuto[1] = true; style.marginAuto[3] = true; return; }
    if (token == "ml-auto") { style.marginAuto[3] = true; return; }
    if (token == "mr-auto") { style.marginAuto[1] = true; return; }
    if (token.rfind("m-", 0) == 0) { float m = parseTwNumericOrArb(token.substr(2)); for (int i=0;i<4;++i) style.margin[i] = m; return; }
    if (token.rfind("mx-", 0) == 0) { float m = parseTwNumericOrArb(token.substr(3)); style.margin[1] = m; style.margin[3] = m; return; }
    if (token.rfind("my-", 0) == 0) { float m = parseTwNumericOrArb(token.substr(3)); style.margin[0] = m; style.margin[2] = m; return; }
    if (token.rfind("mt-", 0) == 0) { style.margin[0] = parseTwNumericOrArb(token.substr(3)); return; }
    if (token.rfind("mr-", 0) == 0) { style.margin[1] = parseTwNumericOrArb(token.substr(3)); return; }
    if (token.rfind("mb-", 0) == 0) { style.margin[2] = parseTwNumericOrArb(token.substr(3)); return; }
    if (token.rfind("ml-", 0) == 0) { style.margin[3] = parseTwNumericOrArb(token.substr(3)); return; }

    // Sizing
    if (token == "w-full" || token == "w-screen") { style.widthPercent = 100.0f; style.width = -1.0f; return; }
    if (token == "w-1/2") { style.widthPercent = 50.0f; style.width = -1.0f; return; }
    if (token == "w-1/3") { style.widthPercent = 33.333f; style.width = -1.0f; return; }
    if (token == "w-2/3") { style.widthPercent = 66.667f; style.width = -1.0f; return; }
    if (token == "w-1/4") { style.widthPercent = 25.0f; style.width = -1.0f; return; }
    if (token == "w-3/4") { style.widthPercent = 75.0f; style.width = -1.0f; return; }
    if (token == "w-auto") { style.width = -1.0f; style.widthPercent = -1.0f; return; }
    if (token.rfind("w-[", 0) == 0 && token.find('%') != std::string::npos) {
        try { style.widthPercent = std::stof(token.substr(3, token.find('%') - 3)); style.width = -1.0f; } catch (...) {}
        return;
    }
    if (token.rfind("w-", 0) == 0) { style.width = parseTwNumericOrArb(token.substr(2)); style.widthPercent = -1.0f; return; }
    if (token.rfind("min-w-", 0) == 0) { style.minWidth = parseTwNumericOrArb(token.substr(6)); return; }

    if (token == "max-w-xs") { style.maxWidth = 320.0f; return; }
    if (token == "max-w-sm") { style.maxWidth = 384.0f; return; }
    if (token == "max-w-md") { style.maxWidth = 448.0f; return; }
    if (token == "max-w-lg") { style.maxWidth = 512.0f; return; }
    if (token == "max-w-xl") { style.maxWidth = 576.0f; return; }
    if (token == "max-w-2xl") { style.maxWidth = 672.0f; return; }
    if (token == "max-w-3xl") { style.maxWidth = 768.0f; return; }
    if (token == "max-w-4xl") { style.maxWidth = 896.0f; return; }
    if (token == "max-w-5xl") { style.maxWidth = 1024.0f; return; }
    if (token == "max-w-6xl") { style.maxWidth = 1152.0f; return; }
    if (token == "max-w-7xl") { style.maxWidth = 1280.0f; return; }
    if (token == "max-w-screen-lg") { style.maxWidth = 1024.0f; return; }
    if (token == "max-w-screen-xl") { style.maxWidth = 1280.0f; return; }
    if (token == "max-w-full" || token == "max-w-none") { style.maxWidth = 1e6f; return; }
    if (token.rfind("max-w-", 0) == 0) {
        float parsed = parseTwNumericOrArb(token.substr(6));
        if (parsed > 0.0f) style.maxWidth = parsed;
        return;
    }

    if (token == "h-full") { style.heightPercent = 100.0f; style.height = -1.0f; style.heightVh = -1.0f; return; }
    if (token == "h-screen" || token == "h-dvh" || token == "h-svh") { style.heightVh = 100.0f; style.height = -1.0f; style.heightPercent = -1.0f; return; }
    if (token == "h-1/2") { style.heightPercent = 50.0f; style.height = -1.0f; return; }
    if (token == "h-1/3") { style.heightPercent = 33.333f; style.height = -1.0f; return; }
    if (token == "h-2/3") { style.heightPercent = 66.667f; style.height = -1.0f; return; }
    if (token == "h-1/4") { style.heightPercent = 25.0f; style.height = -1.0f; return; }
    if (token == "h-3/4") { style.heightPercent = 75.0f; style.height = -1.0f; return; }
    if (token == "h-auto") { style.height = -1.0f; style.heightPercent = -1.0f; style.heightVh = -1.0f; return; }
    if (token == "min-h-screen") { style.minHScreen = true; return; }
    if (token.rfind("min-h-", 0) == 0) { style.minHeight = parseTwNumericOrArb(token.substr(6)); return; }
    if (token == "max-h-none") { style.maxHeight = -1.0f; style.maxHeightPercent = -1.0f; style.maxHeightVh = -1.0f; return; }
    if (token == "max-h-full") { style.maxHeightPercent = 100.0f; style.maxHeight = -1.0f; return; }
    if (token == "max-h-screen" || token == "max-h-dvh" || token == "max-h-svh") { style.maxHeightVh = 100.0f; style.maxHeight = -1.0f; return; }
    if (token.rfind("max-h-[", 0) == 0) {
        std::string sub = token.substr(6);
        if (sub.find('%') != std::string::npos) {
            try { style.maxHeightPercent = std::stof(sub.substr(1, sub.find('%') - 1)); style.maxHeight = -1.0f; } catch (...) {}
            return;
        }
        size_t vhPos = sub.find("vh");
        if (vhPos != std::string::npos) {
            try { style.maxHeightVh = std::stof(sub.substr(1, vhPos - 1)); style.maxHeight = -1.0f; } catch (...) {}
            return;
        }
        style.maxHeight = parseTwNumericOrArb(sub);
        return;
    }
    if (token.rfind("max-h-", 0) == 0) { style.maxHeight = parseTwNumericOrArb(token.substr(6)); return; }
    if (token.rfind("h-[", 0) == 0) {
        std::string sub = token.substr(2);
        if (sub.find('%') != std::string::npos) {
            try { style.heightPercent = std::stof(sub.substr(1, sub.find('%') - 1)); style.height = -1.0f; } catch (...) {}
            return;
        }
        size_t vhPos = sub.find("vh");
        if (vhPos != std::string::npos) {
            try { style.heightVh = std::stof(sub.substr(1, vhPos - 1)); style.height = -1.0f; } catch (...) {}
            return;
        }
        style.height = parseTwNumericOrArb(sub);
        style.heightPercent = -1.0f;
        return;
    }
    if (token.rfind("h-", 0) == 0) { style.height = parseTwNumericOrArb(token.substr(2)); style.heightPercent = -1.0f; style.heightVh = -1.0f; return; }

    // Position & Z-Index
    if (token == "static") { style.position = PositionMode::STATIC; return; }
    if (token == "relative") { style.position = PositionMode::RELATIVE; return; }
    if (token == "absolute") { style.position = PositionMode::ABSOLUTE; return; }
    if (token == "fixed") { style.position = PositionMode::FIXED; return; }
    if (token.rfind("inset-", 0) == 0) { float v = parseTwNumericOrArb(token.substr(6)); style.posTop = style.posRight = style.posBottom = style.posLeft = v; return; }
    if (token.rfind("top-", 0) == 0) { style.posTop = parseTwNumericOrArb(token.substr(4)); return; }
    if (token.rfind("right-", 0) == 0) { style.posRight = parseTwNumericOrArb(token.substr(6)); return; }
    if (token.rfind("bottom-", 0) == 0) { style.posBottom = parseTwNumericOrArb(token.substr(7)); return; }
    if (token.rfind("left-", 0) == 0) { style.posLeft = parseTwNumericOrArb(token.substr(5)); return; }
    if (token.rfind("z-", 0) == 0) {
        std::string sub = token.substr(2);
        if (!sub.empty() && sub.front() == '[' && sub.back() == ']') sub = sub.substr(1, sub.size() - 2);
        try { style.zIndex = std::stoi(sub); style.hasExplicitZIndex = true; } catch (...) {}
        return;
    }

    // Overflow & Overscroll
    if (token == "overflow-hidden") { style.overflowX = OverflowMode::HIDDEN; style.overflowY = OverflowMode::HIDDEN; return; }
    if (token == "overflow-x-hidden") { style.overflowX = OverflowMode::HIDDEN; return; }
    if (token == "overflow-y-hidden") { style.overflowY = OverflowMode::HIDDEN; return; }
    if (token == "overflow-auto") { style.overflowX = OverflowMode::AUTO; style.overflowY = OverflowMode::AUTO; return; }
    if (token == "overflow-scroll") { style.overflowX = OverflowMode::SCROLL; style.overflowY = OverflowMode::SCROLL; return; }
    if (token == "overflow-y-auto") { style.overflowY = OverflowMode::AUTO; return; }
    if (token == "overflow-y-scroll") { style.overflowY = OverflowMode::SCROLL; return; }
    if (token == "overflow-x-auto") { style.overflowX = OverflowMode::AUTO; return; }
    if (token == "overflow-x-scroll") { style.overflowX = OverflowMode::SCROLL; return; }
    if (token == "overscroll-contain" || token == "overscroll-y-contain" || token == "overscroll-x-contain" || token == "overscroll-none") {
        style.overscrollContain = true; return;
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
    if (token.rfind("rounded-[", 0) == 0) { style.borderRadius = parseTwNumericOrArb(token.substr(8), 1.0f); return; }

    // Borders
    if (token == "border-none" || token == "border-0") { style.borderWidth = 0; style.hasPerSideBorder = false; return; }
    if (token == "border") { style.borderWidth = 1.0f; style.hasPerSideBorder = false; return; }
    if (token == "border-2") { style.borderWidth = 2.0f; style.hasPerSideBorder = false; return; }
    if (token == "border-4") { style.borderWidth = 4.0f; style.hasPerSideBorder = false; return; }
    if (token == "border-t") { style.borderSides[0] = 1.0f; style.hasPerSideBorder = true; style.borderWidth = std::max(style.borderWidth, 1.0f); return; }
    if (token == "border-r") { style.borderSides[1] = 1.0f; style.hasPerSideBorder = true; style.borderWidth = std::max(style.borderWidth, 1.0f); return; }
    if (token == "border-b") { style.borderSides[2] = 1.0f; style.hasPerSideBorder = true; style.borderWidth = std::max(style.borderWidth, 1.0f); return; }
    if (token == "border-l") { style.borderSides[3] = 1.0f; style.hasPerSideBorder = true; style.borderWidth = std::max(style.borderWidth, 1.0f); return; }
    if (token.rfind("border-", 0) == 0) {
        ColorRgba c;
        if (resolveColorToken(token.substr(7), c)) {
            style.borderColor = c;
            if (style.borderWidth == 0) style.borderWidth = 1.0f;
        }
        return;
    }

    // Gradients & Background
    if (token.rfind("bg-gradient-to-", 0) == 0) {
        std::string dir = token.substr(15);
        style.hasGradient = true;
        if (dir == "r") style.gradientAngle = 0.0f;
        else if (dir == "l") style.gradientAngle = 180.0f;
        else if (dir == "b") style.gradientAngle = 90.0f;
        else if (dir == "t") style.gradientAngle = 270.0f;
        else if (dir == "tr") style.gradientAngle = 315.0f;
        else if (dir == "br") style.gradientAngle = 45.0f;
        else if (dir == "tl") style.gradientAngle = 225.0f;
        else if (dir == "bl") style.gradientAngle = 135.0f;
        return;
    }
    if (token.rfind("from-", 0) == 0) {
        ColorRgba c;
        if (resolveColorToken(token.substr(5), c)) { style.gradientFrom = c; style.hasGradient = true; style.hasBackground = true; }
        return;
    }
    if (token.rfind("via-", 0) == 0) {
        ColorRgba c;
        if (resolveColorToken(token.substr(4), c)) { style.gradientVia = c; style.hasGradientVia = true; }
        return;
    }
    if (token.rfind("to-", 0) == 0) {
        ColorRgba c;
        if (resolveColorToken(token.substr(3), c)) { style.gradientTo = c; style.hasGradient = true; style.hasBackground = true; }
        return;
    }
    if (token.rfind("bg-", 0) == 0) {
        ColorRgba c;
        if (resolveColorToken(token.substr(3), c)) {
            style.backgroundColor = c;
            style.hasBackground = true;
        }
        return;
    }

    // Opacity
    if (token.rfind("opacity-", 0) == 0) {
        std::string s = token.substr(8);
        try {
            float v = std::stof(s);
            if (v > 1.0f) v /= 100.0f;
            style.opacity = std::clamp(v, 0.0f, 1.0f);
        } catch (...) {}
        return;
    }

    // Text Size / Align / Color
    if (token == "text-left") { style.textAlign = TextAlign::LEFT; style.hasExplicitTextAlign = true; return; }
    if (token == "text-center") { style.textAlign = TextAlign::CENTER; style.hasExplicitTextAlign = true; return; }
    if (token == "text-right") { style.textAlign = TextAlign::RIGHT; style.hasExplicitTextAlign = true; return; }
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
        if (!sub.empty() && sub.front() == '[' && sub.back() == ']') {
            style.fontSize = parseTwNumericOrArb(sub, 1.0f);
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

    // Typography
    if (token == "font-normal") { style.fontWeight = 400; style.hasExplicitFontWeight = true; return; }
    if (token == "font-medium") { style.fontWeight = 500; style.hasExplicitFontWeight = true; return; }
    if (token == "font-semibold") { style.fontWeight = 600; style.hasExplicitFontWeight = true; return; }
    if (token == "font-bold") { style.fontWeight = 700; style.hasExplicitFontWeight = true; return; }
    if (token == "font-extrabold") { style.fontWeight = 800; style.hasExplicitFontWeight = true; return; }
    if (token == "font-black") { style.fontWeight = 900; style.hasExplicitFontWeight = true; return; }
    if (token == "font-mono") { style.fontFamily = FontFamily::MONO; style.fontName.clear(); style.hasExplicitFontFamily = true; return; }
    if (token == "font-sans") { style.fontFamily = FontFamily::SANS; style.fontName.clear(); style.hasExplicitFontFamily = true; return; }
    if (token == "font-serif") { style.fontFamily = FontFamily::SERIF; style.fontName.clear(); style.hasExplicitFontFamily = true; return; }
    if (token.rfind("font-[", 0) == 0 && token.back() == ']' && token.size() > 7) {
        std::string fn = token.substr(6, token.size() - 7);
        for (char &ch : fn) if (ch == '_') ch = ' ';
        style.fontName = fn;
        style.hasExplicitFontFamily = true;
        return;
    }
    if (token == "animate-pulse" || token == "animate-spin" || token == "animate-bounce" ||
        token == "animate-fade-in" || token == "animate-slide-up") {
        style.animateName = token.substr(8);
        return;
    }
    if (token.rfind("duration-", 0) == 0) {
        try { style.transitionDurationMs = std::max(0, std::stoi(token.substr(9))); } catch (...) {}
        return;
    }
    if (token == "ease-linear") { style.transitionEase = "linear"; return; }
    if (token == "ease-in") { style.transitionEase = "ease-in"; return; }
    if (token == "ease-out") { style.transitionEase = "ease-out"; return; }
    if (token == "ease-in-out") { style.transitionEase = "ease-in-out"; return; }
    if (token == "transition" || token == "transition-all" || token == "transition-colors" ||
        token == "transition-opacity" || token == "transition-transform") {
        if (style.transitionDurationMs <= 0) style.transitionDurationMs = 180;
        return;
    }
    if (token == "italic") { style.isItalic = true; return; }
    if (token == "uppercase") { style.isUppercase = true; return; }
    if (token == "lowercase") { style.isLowercase = true; return; }
    if (token == "line-through") { style.isLineThrough = true; return; }
    if (token == "underline") { style.isUnderline = true; return; }
    if (token == "truncate") { style.truncate = true; return; }
    if (token == "tracking-wider") { style.letterSpacing = 1.0f; return; }
    if (token == "tracking-tight") { style.letterSpacing = -0.5f; return; }

    // Shadow
    if (token == "shadow-none") { style.hasShadow = false; return; }
    if (token == "shadow-sm") { style.hasShadow = true; style.shadowBlur = 3.0f; style.shadowOffsetY = 1.0f; style.shadowAlpha = 0.2f; return; }
    if (token == "shadow" || token == "shadow-md") { style.hasShadow = true; style.shadowBlur = 6.0f; style.shadowOffsetY = 2.0f; style.shadowAlpha = 0.28f; return; }
    if (token == "shadow-lg") { style.hasShadow = true; style.shadowBlur = 12.0f; style.shadowOffsetY = 4.0f; style.shadowAlpha = 0.35f; return; }
    if (token == "shadow-xl" || token == "shadow-2xl") { style.hasShadow = true; style.shadowBlur = 18.0f; style.shadowOffsetY = 6.0f; style.shadowAlpha = 0.45f; return; }

    // Misc
    if (token == "pointer-events-none") { style.pointerEventsNone = true; return; }
    if (token == "select-none") { style.userSelectNone = true; return; }
    if (token == "sr-only") { style.srOnly = true; return; }
    if (token.rfind("cursor-", 0) == 0) { style.cursor = token.substr(7); return; }
}

struct StyleRuleFilter {
    bool hovered = false;
    bool focused = false;
    bool focusVisible = false;
    bool active = false;
    bool disabled = false;
    bool checked = false;
    bool selected = false;
    bool rtl = false;
    float viewportW = 1000.0f;
};

inline ComputedStyle parseTailwindStyleWithStates(const std::string &twClasses, const StyleRuleFilter &stateFilter) {
    ComputedStyle style;
    style.isRtl = stateFilter.rtl;
    std::stringstream ss(twClasses);
    std::string raw;
    while (ss >> raw) {
        std::string tok = raw;
        bool applies = true;

        // Peel off prefixes like md:, hover:, focus:, active:, disabled:, rtl:
        while (true) {
            size_t colon = tok.find(':');
            if (colon == std::string::npos) break;
            std::string prefix = tok.substr(0, colon);
            // Ignore arbitrary brackets like w-[10px]
            if (prefix.find('[') != std::string::npos && prefix.find("data-") == std::string::npos) break;

            if (prefix == "sm") { if (stateFilter.viewportW < 640.0f) applies = false; }
            else if (prefix == "md") { if (stateFilter.viewportW < 768.0f) applies = false; }
            else if (prefix == "lg") { if (stateFilter.viewportW < 1024.0f) applies = false; }
            else if (prefix == "xl") { if (stateFilter.viewportW < 1280.0f) applies = false; }
            else if (prefix == "hover") { if (!stateFilter.hovered) applies = false; }
            else if (prefix == "focus") { if (!stateFilter.focused) applies = false; }
            else if (prefix == "focus-visible") { if (!stateFilter.focusVisible) applies = false; }
            else if (prefix == "active") { if (!stateFilter.active) applies = false; }
            else if (prefix == "disabled") { if (!stateFilter.disabled) applies = false; }
            else if (prefix == "checked" || prefix == "data-[state=checked]") { if (!stateFilter.checked) applies = false; }
            else if (prefix == "selected" || prefix == "data-[state=selected]") { if (!stateFilter.selected) applies = false; }
            else if (prefix == "rtl") { if (!stateFilter.rtl) applies = false; }
            else if (prefix == "dark") { /* dark theme active by default */ }
            else { applies = false; }

            tok = tok.substr(colon + 1);
            if (!applies) break;
        }
        if (applies) {
            applySingleTailwindToken(tok, style);
        }
    }
    return style;
}

inline std::string getLucideSvgPath(const std::string &iconName) {
    return Icons::lookupIconSvgPath(iconName);
}

struct LayoutEngineContext {
    float viewportW = 960.0f;
    float viewportH = 720.0f;
    bool rootRtl = false;
    UiInteractionState *interaction = nullptr;
    const UiInputState *input = nullptr;
    int nodeIdCounter = 0;

    struct DeferredLayer {
        int zIndex = 0;
        std::vector<GpuDrawCmd> cmds;
        std::vector<UiHitRegion> hits;
    };
    std::vector<DeferredLayer> portalLayers;
};

inline std::string resolveNodeKey(const UiNode &node, const std::string &pathPrefix, int childIdx) {
    if (!node.key.empty()) return pathPrefix + "#" + node.key;
    if (!node.idAttr.empty()) return pathPrefix + "@" + node.idAttr;
    if (!node.testId.empty()) return pathPrefix + "$t:" + node.testId;
    return pathPrefix + "/" + node.tag + "[" + std::to_string(childIdx) + "]";
}

inline ComputedStyle computeNodeStyle(const UiNode &node, const ComputedStyle &inherited, const std::string &nodeKey, LayoutEngineContext &ctx) {
    bool isRtl = inherited.isRtl;
    if (node.dir == "rtl") isRtl = true;
    else if (node.dir == "ltr") isRtl = false;

    StyleRuleFilter sf;
    sf.viewportW = ctx.viewportW;
    sf.rtl = isRtl;
    sf.disabled = node.disabled;
    sf.checked = node.checked;
    sf.selected = node.selected;
    if (ctx.interaction) {
        sf.hovered = (!nodeKey.empty() && ctx.interaction->hoveredKey == nodeKey);
        sf.focused = (!nodeKey.empty() && ctx.interaction->focusedKey == nodeKey);
        sf.focusVisible = sf.focused && ctx.interaction->focusVisible;
        sf.active = (!nodeKey.empty() && ctx.interaction->activeKey == nodeKey);
    }

    ComputedStyle style = parseTailwindStyleWithStates(node.classStr, sf);
    if (node.getBoolProp("hidden", false)) style.display = DisplayMode::NONE;
    style.isRtl = isRtl;

    if (!style.hasExplicitTextColor) style.textColor = inherited.textColor;
    if (!style.hasExplicitFontSize) style.fontSize = inherited.fontSize;
    if (!style.hasExplicitFontWeight) style.fontWeight = inherited.fontWeight;
    if (!style.hasExplicitFontFamily) {
        style.fontFamily = inherited.fontFamily;
        style.fontName = inherited.fontName;
    }
    if (!style.hasExplicitTextAlign) style.textAlign = inherited.textAlign;
    if (inherited.isLineThrough) style.isLineThrough = true;
    if (node.zIndex != 0 && !style.hasExplicitZIndex) style.zIndex = node.zIndex;

    // Default semantic tag styling (preserving original h1/h2/h3/p sizes)
    if (node.tag == "h1" && !style.hasExplicitFontSize) { style.fontSize = 30.0f; style.fontWeight = 700; }
    if (node.tag == "h2" && !style.hasExplicitFontSize) { style.fontSize = 24.0f; style.fontWeight = 700; }
    if (node.tag == "h3" && !style.hasExplicitFontSize) { style.fontSize = 20.0f; style.fontWeight = 600; }
    if (node.tag == "p" && !style.hasExplicitFontSize) { style.fontSize = 14.0f; }

    // Default native widget dimensions if unstyled
    if (node.tag == "checkbox" || node.tag == "radio") {
        if (style.width <= 0) style.width = 18.0f;
        if (style.height <= 0) style.height = 18.0f;
    } else if (node.tag == "switch") {
        if (style.width <= 0) style.width = 40.0f;
        if (style.height <= 0) style.height = 22.0f;
    } else if (node.tag == "slider") {
        if (style.height <= 0) style.height = 24.0f;
    } else if (node.tag == "progress") {
        if (style.height <= 0) style.height = 10.0f;
    } else if (node.tag == "textarea") {
        if (style.height <= 0 && style.minHeight <= 0) style.minHeight = 96.0f;
    } else if (node.tag == "canvas") {
        if (style.height <= 0 && style.minHeight <= 0) style.minHeight = 160.0f;
    }

    return style;
}

inline std::string collectNodeAccessibleText(const UiNode &node) {
    if (!node.ariaLabel.empty()) return node.ariaLabel;
    if (node.hasProp("aria-label")) return node.getProp("aria-label");
    if (node.hasProp("accessibleName")) return node.getProp("accessibleName");
    if (node.hasProp("label")) return node.getProp("label");
    if (node.hasProp("title")) return node.getProp("title");
    if (node.hasProp("placeholder") && node.getProp("value").empty()) return node.getProp("placeholder");
    if (node.hasProp("value") && !node.getProp("value").empty()) return node.getProp("value");
    if (node.isText) return node.text;
    std::string acc;
    for (const auto &c : node.children) {
        std::string sub = collectNodeAccessibleText(c);
        if (!sub.empty()) {
            if (!acc.empty()) acc += " ";
            acc += sub;
        }
    }
    return acc;
}

inline float resolveExplicitBoxHeight(const ComputedStyle &style, float refH, float viewportH) {
    if (style.height > 0.0f) return style.height;
    if (style.heightPercent > 0.0f && refH > 0.0f) return refH * (style.heightPercent / 100.0f);
    if (style.heightVh > 0.0f && viewportH > 0.0f) return viewportH * (style.heightVh / 100.0f);
    return -1.0f;
}

inline float resolveMaxHeightLimit(const ComputedStyle &style, float refH, float viewportH) {
    float mh = style.maxHeight;
    if (style.maxHeightPercent > 0.0f && refH > 0.0f) {
        float pct = refH * (style.maxHeightPercent / 100.0f);
        mh = (mh > 0.0f) ? std::min(mh, pct) : pct;
    }
    if (style.maxHeightVh > 0.0f && viewportH > 0.0f) {
        float vh = viewportH * (style.maxHeightVh / 100.0f);
        mh = (mh > 0.0f) ? std::min(mh, vh) : vh;
    }
    return mh;
}

inline float measureNaturalNodeWidth(const UiNode &node, const ComputedStyle &inherited, const std::string &nodeKey, LayoutEngineContext &ctx) {
    if (node.isText) {
        if (node.text.empty()) return 0.0f;
        std::string txt = node.text;
        if (inherited.isUppercase) for (char &c : txt) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        else if (inherited.isLowercase) for (char &c : txt) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return MemeUiFonts::measureTextWidth(txt, inherited.fontSize, inherited.fontFamily, inherited.fontName, inherited.fontWeight);
    }
    ComputedStyle style = computeNodeStyle(node, inherited, nodeKey, ctx);
    if (style.display == DisplayMode::NONE) return 0.0f;
    if (node.tag == "icon" || node.tag == "svg") {
        double explicitSz = node.getNumProp("size", 0.0);
        float sz = explicitSz > 0.0 ? static_cast<float>(explicitSz) : (style.width > 0 ? style.width : 18.0f);
        return sz + style.padding[1] + style.padding[3];
    }
    if (style.width > 0) return style.width;
    if (node.tag == "input" && node.children.empty()) return 160.0f + style.padding[1] + style.padding[3];
    if (node.tag == "select" || node.tag == "combobox") return std::max(140.0f, measureTextWidth(collectNodeAccessibleText(node), 14.0f, false) + 36.0f);

    float contentW = 0.0f;
    bool rowFlex = (style.display == DisplayMode::FLEX || style.display == DisplayMode::INLINE_FLEX) &&
                   (style.flexDirection == FlexDir::ROW || style.flexDirection == FlexDir::ROW_REVERSE);
    if (rowFlex) {
        bool anyVisible = false;
        for (size_t i = 0; i < node.children.size(); ++i) {
            float cw = measureNaturalNodeWidth(node.children[i], style, resolveNodeKey(node.children[i], nodeKey, static_cast<int>(i)), ctx);
            if (!node.children[i].isText) {
                ComputedStyle cs = computeNodeStyle(node.children[i], style, resolveNodeKey(node.children[i], nodeKey, static_cast<int>(i)), ctx);
                if (cs.display == DisplayMode::NONE) continue;
            }
            if (anyVisible) contentW += style.gapX;
            contentW += cw;
            anyVisible = true;
        }
    } else {
        for (size_t i = 0; i < node.children.size(); ++i) {
            contentW = std::max(contentW, measureNaturalNodeWidth(node.children[i], style, resolveNodeKey(node.children[i], nodeKey, static_cast<int>(i)), ctx));
        }
    }
    float totalW = contentW + style.padding[1] + style.padding[3];
    if (style.maxWidth > 0 && totalW > style.maxWidth) totalW = style.maxWidth;
    if (style.minWidth > 0 && totalW < style.minWidth) totalW = style.minWidth;
    return totalW;
}

// Fast O(N) single-pass size estimator for layout passes (avoids emitting dummy draw cmds!)
inline void estimateNodeSize(const UiNode &node, float availW, const ComputedStyle &inherited, const std::string &nodeKey, LayoutEngineContext &ctx, float &outW, float &outH) {
    if (node.isText) {
        if (node.text.empty()) { outW = 0; outH = 0; return; }
        std::string txt = node.text;
        if (inherited.isUppercase) for (char &c : txt) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        else if (inherited.isLowercase) for (char &c : txt) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        bool mono = (inherited.fontFamily == FontFamily::MONO);
        float singleW = MemeUiFonts::measureTextWidth(txt, inherited.fontSize, inherited.fontFamily, inherited.fontName, inherited.fontWeight);
        float lineH = inherited.fontSize * 1.35f;
        bool hasNewline = (txt.find('\n') != std::string::npos);
        if ((hasNewline || (availW > 24.0f && singleW > availW + 1.5f)) && !inherited.truncate) {
            float wrapW = (availW > 24.0f) ? availW : 1e6f;
            auto lines = wrapTextLines(txt, wrapW, inherited.fontSize, mono);
            float maxW = 0.0f;
            for (const auto &ln : lines) maxW = std::max(maxW, ln.width);
            outW = (availW > 24.0f) ? std::min(availW, maxW) : maxW;
            outH = std::max(1, static_cast<int>(lines.size())) * lineH;
        } else {
            outW = (inherited.truncate && availW > 0) ? std::min(singleW, availW) : singleW;
            outH = lineH;
        }
        return;
    }
    ComputedStyle style = computeNodeStyle(node, inherited, nodeKey, ctx);
    if (style.display == DisplayMode::NONE) { outW = 0; outH = 0; return; }

    float nodeW = availW - style.margin[1] - style.margin[3];
    if (style.widthPercent > 0) nodeW = availW * (style.widthPercent / 100.0f);
    else if (style.width > 0) nodeW = style.width;
    if (style.maxWidth > 0 && nodeW > style.maxWidth) nodeW = style.maxWidth;

    bool isFlexChild = ((inherited.display == DisplayMode::FLEX || inherited.display == DisplayMode::INLINE_FLEX) &&
                        (inherited.flexDirection == FlexDir::ROW || inherited.flexDirection == FlexDir::ROW_REVERSE));
    bool isHorizScrollParent = (inherited.overflowX == OverflowMode::AUTO || inherited.overflowX == OverflowMode::SCROLL);
    bool isNaturalInline = (node.tag == "button" || node.tag == "span" || node.tag == "a" || node.tag == "label" ||
                            node.tag == "b" || node.tag == "i" || node.tag == "strong" ||
                            style.display == DisplayMode::INLINE || style.display == DisplayMode::INLINE_FLEX);
    if (style.width <= 0 && style.widthPercent <= 0 && style.flexGrow == 0.0f && (isNaturalInline || isFlexChild)) {
        float natW = measureNaturalNodeWidth(node, inherited, nodeKey, ctx);
        if (natW > 0.0f && ((isFlexChild && isHorizScrollParent) || natW < nodeW)) nodeW = natW;
    }

    float innerW = std::max(0.0f, nodeW - style.padding[1] - style.padding[3]);
    float contentH = 0.0f;
    bool rowFlex = (style.display == DisplayMode::FLEX || style.display == DisplayMode::INLINE_FLEX) &&
                   (style.flexDirection == FlexDir::ROW || style.flexDirection == FlexDir::ROW_REVERSE);
    if (style.display == DisplayMode::GRID && style.gridCols > 0) {
        float colW = (innerW - (style.gridCols - 1) * style.gapX) / style.gridCols;
        int colIdx = 0;
        float maxRowH = 0.0f;
        for (size_t i = 0; i < node.children.size(); ++i) {
            std::string cKey = resolveNodeKey(node.children[i], nodeKey, static_cast<int>(i));
            if (!node.children[i].isText && computeNodeStyle(node.children[i], style, cKey, ctx).display == DisplayMode::NONE) continue;
            float cw = 0, ch = 0;
            estimateNodeSize(node.children[i], colW, style, cKey, ctx, cw, ch);
            maxRowH = std::max(maxRowH, ch);
            colIdx++;
            if (colIdx >= style.gridCols) {
                contentH += maxRowH + style.gapY;
                maxRowH = 0;
                colIdx = 0;
            }
        }
        contentH += maxRowH;
    } else if (rowFlex && style.flexWrap) {
        float curLineW = 0.0f;
        float curLineH = 0.0f;
        int lineItems = 0;
        bool hasAnyLine = false;
        for (size_t i = 0; i < node.children.size(); ++i) {
            std::string cKey = resolveNodeKey(node.children[i], nodeKey, static_cast<int>(i));
            if (!node.children[i].isText && computeNodeStyle(node.children[i], style, cKey, ctx).display == DisplayMode::NONE) continue;
            float cw = 0, ch = 0;
            estimateNodeSize(node.children[i], innerW, style, cKey, ctx, cw, ch);
            if (lineItems > 0 && curLineW + style.gapX + cw > innerW + 0.5f) {
                contentH += (hasAnyLine ? style.gapY : 0.0f) + curLineH;
                hasAnyLine = true;
                curLineW = cw;
                curLineH = ch;
                lineItems = 1;
            } else {
                curLineW += (lineItems > 0 ? style.gapX : 0.0f) + cw;
                curLineH = std::max(curLineH, ch);
                lineItems++;
            }
        }
        if (lineItems > 0) contentH += (hasAnyLine ? style.gapY : 0.0f) + curLineH;
    } else if (rowFlex) {
        for (size_t i = 0; i < node.children.size(); ++i) {
            std::string cKey = resolveNodeKey(node.children[i], nodeKey, static_cast<int>(i));
            if (!node.children[i].isText && computeNodeStyle(node.children[i], style, cKey, ctx).display == DisplayMode::NONE) continue;
            float cw = 0, ch = 0;
            estimateNodeSize(node.children[i], innerW, style, cKey, ctx, cw, ch);
            contentH = std::max(contentH, ch);
        }
    } else {
        bool anyVisible = false;
        for (size_t i = 0; i < node.children.size(); ++i) {
            std::string cKey = resolveNodeKey(node.children[i], nodeKey, static_cast<int>(i));
            if (!node.children[i].isText && computeNodeStyle(node.children[i], style, cKey, ctx).display == DisplayMode::NONE) continue;
            if (anyVisible) contentH += style.gapY;
            float cw = 0, ch = 0;
            estimateNodeSize(node.children[i], innerW, style, cKey, ctx, cw, ch);
            contentH += ch;
            anyVisible = true;
        }
    }

    if (node.tag == "input") contentH = std::max(contentH, style.fontSize * 1.35f);
    else if (node.tag == "textarea") {
        std::string v = node.getProp("value");
        bool mono = (style.fontFamily == FontFamily::MONO);
        auto lines = wrapTextLines(v, innerW, style.fontSize, mono);
        contentH = std::max(contentH, std::max(3.0f, static_cast<float>(lines.size())) * (style.fontSize * 1.35f) + 6.0f);
    }
    float finalH = contentH + style.padding[0] + style.padding[2];
    if (style.height > 0) finalH = style.height;
    else if (style.heightVh > 0 && ctx.viewportH > 0) finalH = ctx.viewportH * (style.heightVh / 100.0f);
    else if (style.heightPercent > 0 && ctx.viewportH > 0) finalH = std::max(finalH, ctx.viewportH * (style.heightPercent / 100.0f));
    if (style.minHScreen) finalH = std::max(finalH, ctx.viewportH);
    if (style.minHeight > 0 && finalH < style.minHeight) finalH = style.minHeight;
    float effMaxH = resolveMaxHeightLimit(style, ctx.viewportH, ctx.viewportH);
    if (effMaxH > 0 && finalH > effMaxH) finalH = effMaxH;
    outW = nodeW;
    outH = finalH + style.margin[0] + style.margin[2];
}

inline bool nodeHasShrinkableScrollDescendant(
    const UiNode &node,
    const ComputedStyle &inherited,
    const std::string &nodeKey,
    LayoutEngineContext &ctx,
    int depth = 0)
{
    if (node.isText || depth > 12) return false;
    ComputedStyle cs = computeNodeStyle(node, inherited, nodeKey, ctx);
    if (cs.display == DisplayMode::NONE) return false;
    if (node.tag == "scrollarea" || cs.overflowY != OverflowMode::VISIBLE) return true;
    for (size_t i = 0; i < node.children.size(); ++i) {
        const auto &ch = node.children[i];
        if (ch.isText) continue;
        std::string cKey = resolveNodeKey(ch, nodeKey, static_cast<int>(i));
        ComputedStyle ccs = computeNodeStyle(ch, cs, cKey, ctx);
        if (ccs.display == DisplayMode::NONE) continue;
        if (ccs.flexGrow > 0.0f || ccs.heightPercent > 0.0f || ccs.overflowY != OverflowMode::VISIBLE || ch.tag == "scrollarea") {
            if (nodeHasShrinkableScrollDescendant(ch, cs, cKey, ctx, depth + 1)) return true;
        }
    }
    return false;
}

inline void solveUiNode(
    const UiNode &node,
    float parentX,
    float parentY,
    float parentW,
    float parentH,
    const ComputedStyle &inherited,
    const std::string &nodeKey,
    const UiRect &clipRect,
    std::vector<GpuDrawCmd> &cmds,
    std::vector<UiHitRegion> *hitRegions,
    LayoutEngineContext &ctx,
    float &outW,
    float &outH,
    int overrideZ = -999999,
    float stretchTargetH = -1.0f,
    bool isDeferredRoot = false,
    bool parentIsBounded = false)
{
    if (node.isText) {
        std::string txt = node.text;
        if (txt.empty()) { outW = 0; outH = 0; return; }
        if (inherited.isUppercase) for (char &c : txt) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        else if (inherited.isLowercase) for (char &c : txt) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        bool isMono = (inherited.fontFamily == FontFamily::MONO);
        bool isSerif = (inherited.fontFamily == FontFamily::SERIF);
        float singleW = MemeUiFonts::measureTextWidth(txt, inherited.fontSize, inherited.fontFamily, inherited.fontName, inherited.fontWeight);
        float lineH = inherited.fontSize * 1.35f;
        int zUse = (overrideZ != -999999) ? overrideZ : inherited.zIndex;

        auto emitTextLine = [&](const std::string &lineStr, float lineW, float ly) {
            if (lineStr.empty() || inherited.srOnly) return;
            float drawX = parentX;
            if (inherited.textAlign == TextAlign::CENTER && parentW > lineW) drawX += (parentW - lineW) * 0.5f;
            else if ((inherited.textAlign == TextAlign::RIGHT || (inherited.isRtl && !inherited.hasExplicitTextAlign)) && parentW > lineW) drawX += (parentW - lineW);

            GpuDrawCmd cmd;
            cmd.type = "DRAW_TEXT";
            cmd.x = drawX;
            cmd.y = ly + inherited.fontSize * MemeUiFonts::getBaselineOffsetRatio(inherited.fontFamily, inherited.fontName);
            cmd.text = lineStr;
            cmd.fontSize = inherited.fontSize;
            cmd.fontWeight = inherited.fontWeight;
            cmd.color = inherited.textColor.hex;
            cmd.alpha = inherited.textColor.a * inherited.opacity;
            cmd.isMono = isMono;
            cmd.isSerif = isSerif;
            cmd.fontName = inherited.fontName;
            cmd.isItalic = inherited.isItalic;
            cmd.isUnderline = inherited.isUnderline;
            cmd.isStrike = inherited.isLineThrough;
            cmd.zIndex = zUse;
            cmds.push_back(cmd);

            if (inherited.isLineThrough) {
                GpuDrawCmd strike;
                strike.type = "DRAW_LINE";
                strike.x = drawX;
                strike.y = ly + inherited.fontSize * 0.65f;
                strike.width = lineW;
                strike.height = 0;
                strike.color = inherited.textColor.hex;
                strike.alpha = 0.85f * inherited.opacity;
                strike.lineWidth = 1.5f;
                strike.zIndex = zUse;
                cmds.push_back(strike);
            }
        };

        bool hasNewline = (txt.find('\n') != std::string::npos);
        if ((hasNewline || (parentW > 24.0f && singleW > parentW + 1.5f)) && !inherited.truncate) {
            float wrapW = (parentW > 24.0f) ? parentW : 1e6f;
            auto lines = wrapTextLines(txt, wrapW, inherited.fontSize, isMono);
            float maxW = 0.0f;
            for (size_t r = 0; r < lines.size(); ++r) {
                emitTextLine(lines[r].text, lines[r].width, parentY + r * lineH);
                maxW = std::max(maxW, lines[r].width);
            }
            outW = (parentW > 24.0f) ? std::min(parentW, maxW) : maxW;
            outH = std::max(1, static_cast<int>(lines.size())) * lineH;
        } else if (inherited.truncate && parentW > 24.0f && singleW > parentW + 1.5f) {
            std::string trimmed;
            float ellW = measureTextWidth("...", inherited.fontSize, isMono);
            for (size_t i = 0; i < txt.size();) {
                size_t next = utf8NextChar(txt, i);
                std::string cand = txt.substr(0, next);
                if (measureTextWidth(cand, inherited.fontSize, isMono) + ellW > parentW) break;
                trimmed = cand;
                i = next;
            }
            if (!trimmed.empty()) trimmed += "...";
            float tw = measureTextWidth(trimmed, inherited.fontSize, isMono);
            emitTextLine(trimmed, tw, parentY);
            outW = std::min(parentW, tw);
            outH = lineH;
        } else {
            emitTextLine(txt, singleW, parentY);
            outW = singleW;
            outH = lineH;
        }
        return;
    }

    ComputedStyle style = computeNodeStyle(node, inherited, nodeKey, ctx);
    if (style.display == DisplayMode::NONE) {
        outW = 0; outH = 0; return;
    }

    if (ctx.interaction) {
        if (node.hasProp("animateOpacity") || node.tag.rfind("motion.", 0) == 0) {
            float targetOp = static_cast<float>(node.getNumProp("animateOpacity", style.opacity));
            float initOp = static_cast<float>(node.getNumProp("initialOpacity", style.opacity));
            ctx.interaction->animStore.setTarget(nodeKey + ":op", targetOp, initOp, node.hasProp("initialOpacity"));
            style.opacity = std::clamp(ctx.interaction->animStore.getValue(nodeKey + ":op", targetOp), 0.0f, 1.0f);
        }
        if (node.hasProp("animateY")) {
            float targetY = static_cast<float>(node.getNumProp("animateY", 0.0));
            float initY = static_cast<float>(node.getNumProp("initialY", 0.0));
            ctx.interaction->animStore.setTarget(nodeKey + ":ty", targetY, initY, node.hasProp("initialY"));
            style.translateY += ctx.interaction->animStore.getValue(nodeKey + ":ty", targetY);
        }
        if (style.animateName == "pulse") {
            ctx.interaction->animStore.hasLoopingKeyframe = true;
            float phase = 0.5f + 0.5f * std::sin(static_cast<float>(ctx.interaction->animStore.elapsedSec) * 3.14159265f);
            style.opacity *= (0.45f + 0.55f * phase);
        } else if (style.animateName == "fade-in") {
            ctx.interaction->animStore.setTarget(nodeKey + ":fade", 1.0f, 0.0f, true);
            style.opacity *= ctx.interaction->animStore.getValue(nodeKey + ":fade", 1.0f);
        } else if (style.animateName == "slide-up") {
            ctx.interaction->animStore.setTarget(nodeKey + ":fade", 1.0f, 0.0f, true);
            ctx.interaction->animStore.setTarget(nodeKey + ":slideY", 0.0f, 10.0f, true);
            style.opacity *= ctx.interaction->animStore.getValue(nodeKey + ":fade", 1.0f);
            style.translateY += ctx.interaction->animStore.getValue(nodeKey + ":slideY", 0.0f);
        }
    }

    int baseZ = (overrideZ != -999999) ? overrideZ : 0;
    int effZ = std::max(baseZ, style.zIndex);

    // Portal / explicit elevated z-index layers defer to overlay pass so they paint on top of everything
    bool needsDefer = node.isPortal || ((style.hasExplicitZIndex || node.zIndex > 0) && style.zIndex > baseZ);
    if (needsDefer && !isDeferredRoot) {
        if (node.tag == "dialog" && !node.open && node.hasProp("open") && !node.getBoolProp("open", true)) {
            outW = 0; outH = 0; return;
        }
        LayoutEngineContext::DeferredLayer layer;
        int minPortalZ = node.isPortal ? ((node.tag == "toast") ? 60 : ((node.tag == "dialog") ? 50 : 10)) : 1;
        layer.zIndex = std::max(std::max(baseZ + 1, minPortalZ), effZ);
        float pX = parentX, pY = parentY, pW = parentW, pH = parentH;
        if (std::isnan(style.posTop) && std::isnan(style.posBottom) && std::isnan(style.posLeft) && std::isnan(style.posRight)) {
            if (node.tag == "dialog") {
                float availW = (style.width > 0) ? style.width : std::min(440.0f, std::max(240.0f, ctx.viewportW - 48.0f));
                float estW = 0, estH = 0;
                estimateNodeSize(node, availW, inherited, nodeKey, ctx, estW, estH);
                pW = std::min(availW, std::max(220.0f, estW > 0 ? estW : availW));
                float dH = std::max(120.0f, estH);
                pX = std::max(16.0f, (ctx.viewportW - pW) * 0.5f);
                pY = std::max(16.0f, (ctx.viewportH - dH) * 0.5f);
                pH = std::max(dH, ctx.viewportH - pY - 16.0f);
            } else if (node.tag == "toast") {
                float availW = (style.width > 0) ? style.width : std::min(380.0f, std::max(200.0f, ctx.viewportW - 32.0f));
                float estW = 0, estH = 0;
                estimateNodeSize(node, availW, inherited, nodeKey, ctx, estW, estH);
                pW = std::max(220.0f, std::min(availW, estW > 0 ? estW : availW));
                float dH = std::max(44.0f, estH);
                pX = std::max(16.0f, ctx.viewportW - pW - 20.0f);
                pY = std::max(16.0f, ctx.viewportH - dH - 20.0f);
                pH = std::max(dH, ctx.viewportH - pY - 16.0f);
            } else if ((node.tag == "popover" || node.tag == "tooltip") && pY + 120.0f > ctx.viewportH) {
                pY = std::max(16.0f, ctx.viewportH - 140.0f);
            }
        }
        float dw = 0, dh = 0;
        solveUiNode(node, pX, pY, pW, pH, inherited, nodeKey, {0, 0, ctx.viewportW, ctx.viewportH},
                    layer.cmds, hitRegions ? &layer.hits : nullptr, ctx, dw, dh, layer.zIndex, -1.0f, true);
        ctx.portalLayers.push_back(std::move(layer));
        outW = 0; outH = 0; return;
    }

    // Dialog / backdrop handling when mounted as portal layer
    if (node.tag == "dialog" && node.getBoolProp("modal", true)) {
        GpuDrawCmd scrim;
        scrim.type = "FILL_ROUND_RECT";
        scrim.x = 0; scrim.y = 0; scrim.width = ctx.viewportW; scrim.height = ctx.viewportH;
        scrim.color = "#020617"; scrim.alpha = 0.68f; scrim.zIndex = effZ;
        cmds.push_back(scrim);
        if (hitRegions) {
            UiHitRegion bd;
            bd.id = 0;
            bd.nodeKey = nodeKey + "$backdrop";
            bd.x = 0; bd.y = 0; bd.width = ctx.viewportW; bd.height = ctx.viewportH;
            bd.clipRect = {0, 0, ctx.viewportW, ctx.viewportH};
            bd.zIndex = effZ;
            bd.tag = "backdrop";
            bd.role = "presentation";
            bd.accessibleName = "Modal backdrop";
            hitRegions->push_back(std::move(bd));
        }
    }

    float nodeX = parentX + style.margin[3] + style.translateX;
    float nodeY = parentY + style.margin[0] + style.translateY;

    float nodeW = parentW - style.margin[1] - style.margin[3];
    if (style.widthPercent > 0) nodeW = parentW * (style.widthPercent / 100.0f);
    else if (style.width > 0) nodeW = style.width;
    if (style.maxWidth > 0 && nodeW > style.maxWidth) nodeW = style.maxWidth;
    if (style.minWidth > 0 && nodeW < style.minWidth) nodeW = style.minWidth;

    if (style.marginAuto[1] && style.marginAuto[3] && parentW > nodeW) {
        nodeX = parentX + (parentW - nodeW) * 0.5f;
    }

    if (style.position == PositionMode::ABSOLUTE || style.position == PositionMode::FIXED) {
        float baseX = (style.position == PositionMode::FIXED) ? 0.0f : parentX;
        float baseY = (style.position == PositionMode::FIXED) ? 0.0f : parentY;
        float baseW = (style.position == PositionMode::FIXED) ? ctx.viewportW : parentW;
        float baseH = (style.position == PositionMode::FIXED) ? ctx.viewportH : parentH;
        if (!std::isnan(style.posLeft)) nodeX = baseX + style.posLeft;
        else if (!std::isnan(style.posRight)) nodeX = baseX + baseW - nodeW - style.posRight;
        if (!std::isnan(style.posTop)) nodeY = baseY + style.posTop;
        else if (!std::isnan(style.posBottom)) nodeY = baseY + baseH - 40.0f - style.posBottom;
    }

    bool isFlexChild = ((inherited.display == DisplayMode::FLEX || inherited.display == DisplayMode::INLINE_FLEX) &&
                        (inherited.flexDirection == FlexDir::ROW || inherited.flexDirection == FlexDir::ROW_REVERSE));
    bool isHorizScrollParent = (inherited.overflowX == OverflowMode::AUTO || inherited.overflowX == OverflowMode::SCROLL);
    bool isNaturalInline = (node.tag == "button" || node.tag == "span" || node.tag == "a" || node.tag == "label" ||
                            node.tag == "b" || node.tag == "i" || node.tag == "strong" || node.tag == "checkbox" ||
                            node.tag == "switch" || node.tag == "radio" || node.tag == "icon" || node.tag == "svg" ||
                            style.display == DisplayMode::INLINE || style.display == DisplayMode::INLINE_FLEX);
    if (style.width <= 0 && style.widthPercent <= 0 && style.flexGrow == 0.0f && (isNaturalInline || isFlexChild)) {
        float natW = measureNaturalNodeWidth(node, inherited, nodeKey, ctx);
        if (natW > 0.0f && ((isFlexChild && isHorizScrollParent) || natW < nodeW)) nodeW = natW;
    }

    bool hasClick = (node.tag == "button" || node.tag == "a" || node.tag == "checkbox" || node.tag == "switch" ||
                     node.tag == "radio" || node.tag == "tab" || node.tag == "select" || node.tag == "combobox" ||
                     node.handlers.count("click") > 0);
    bool hasChange = (node.tag == "input" || node.tag == "textarea" || node.tag == "select" || node.tag == "combobox" ||
                      node.tag == "slider" || node.tag == "checkbox" || node.tag == "switch" || node.tag == "radio" ||
                      node.handlers.count("change") > 0 || node.handlers.count("input") > 0);
    bool isRoot = (nodeKey == "root");
    bool hasVertScroll = (style.overflowY == OverflowMode::AUTO || style.overflowY == OverflowMode::SCROLL || node.tag == "scrollarea");
    bool hasHorizScroll = (style.overflowX == OverflowMode::AUTO || style.overflowX == OverflowMode::SCROLL);
    bool hasExplicitScroll = (hasVertScroll || hasHorizScroll);
    bool hasWheelOrScrollHandler = (node.handlers.count("wheel") > 0 || node.handlers.count("scroll") > 0);
    bool isScroll = (hasExplicitScroll ||
                     (isRoot && style.overflowY != OverflowMode::HIDDEN && style.height <= 0));
    bool isFocusable = hasClick || hasChange || node.tabIndex >= 0;

    int nodeId = 0;
    if (hitRegions && (hasClick || hasChange || isScroll || hasWheelOrScrollHandler || node.handlers.count("keydown") > 0 || node.handlers.count("pointerdown") > 0 || !node.testId.empty())) {
        nodeId = ++ctx.nodeIdCounter;
    }

    float innerX = nodeX + style.padding[3];
    float innerY = nodeY + style.padding[0];
    float innerW = std::max(0.0f, nodeW - style.padding[1] - style.padding[3]);

    // Scroll offset retrieval
    float scrollY = 0.0f;
    float scrollX = 0.0f;
    if (isScroll && ctx.interaction && !nodeKey.empty()) {
        scrollY = ctx.interaction->scrollOffsetsY[nodeKey];
        scrollX = ctx.interaction->scrollOffsetsX[nodeKey];
    }

    float expBoxH = resolveExplicitBoxHeight(style, parentH, ctx.viewportH);
    float effMaxH = resolveMaxHeightLimit(style, parentH, ctx.viewportH);
    bool hasDefinitePctH = (style.heightPercent > 0.0f && parentH > 0.0f && (parentIsBounded || isRoot));
    bool hasDefiniteOwnH = (style.height > 0.0f || style.heightVh > 0.0f || hasDefinitePctH || stretchTargetH > 0.0f ||
                            (isRoot && parentH > 0.0f && style.overflowY != OverflowMode::HIDDEN));
    bool boundedScrollH = (node.tag == "scrollarea" || style.height > 0.0f || style.heightVh > 0.0f ||
                           effMaxH > 0.0f || hasDefinitePctH || stretchTargetH > 0.0f);
    bool emitOverflowClip = (style.overflowY == OverflowMode::HIDDEN || style.overflowX == OverflowMode::HIDDEN ||
                             (!isRoot && ((hasVertScroll && boundedScrollH) || hasHorizScroll)));

    size_t bgIndex = cmds.size();
    size_t clipCmdIndex = static_cast<size_t>(-1);
    size_t hitBgIndex = hitRegions ? hitRegions->size() : 0;
    UiRect childClip = isRoot && parentH > 0 ? clipRect.intersect({nodeX, nodeY, nodeW, parentH}) : clipRect;
    if (emitOverflowClip) {
        float estClipH = (expBoxH > 0.0f && (style.height > 0.0f || style.heightVh > 0.0f || hasDefinitePctH))
            ? expBoxH
            : (stretchTargetH > 0.0f ? stretchTargetH
                                     : (effMaxH > 0.0f ? effMaxH : (hasHorizScroll && !boundedScrollH ? clipRect.height : parentH)));
        if (effMaxH > 0.0f && estClipH > effMaxH) estClipH = effMaxH;
        if (estClipH <= 0.0f) estClipH = ctx.viewportH;
        childClip = clipRect.intersect({nodeX, nodeY, nodeW, estClipH});
        GpuDrawCmd pc;
        pc.type = "PUSH_CLIP";
        pc.x = nodeX; pc.y = nodeY; pc.width = nodeW; pc.height = estClipH; pc.zIndex = effZ;
        clipCmdIndex = cmds.size();
        cmds.push_back(pc);
    }

    float contentH = 0.0f;
    float contentW = innerW;
    bool rowFlex = (style.display == DisplayMode::FLEX || style.display == DisplayMode::INLINE_FLEX) &&
                   (style.flexDirection == FlexDir::ROW || style.flexDirection == FlexDir::ROW_REVERSE);

    if (style.display == DisplayMode::GRID && style.gridCols > 0) {
        int cols = style.gridCols;
        float colW = (innerW - (cols - 1) * style.gapX) / cols;
        float curRowY = innerY - scrollY;
        float maxRowH = 0.0f;
        float maxGridRightX = 0.0f;
        int colIdx = 0;

        for (size_t ci = 0; ci < node.children.size(); ++ci) {
            const auto &child = node.children[ci];
            std::string cKey = resolveNodeKey(child, nodeKey, static_cast<int>(ci));
            ComputedStyle cs = computeNodeStyle(child, style, cKey, ctx);
            if (!child.isText && cs.display == DisplayMode::NONE) continue;
            int span = std::clamp(cs.colSpan, 1, cols);
            if (colIdx + span > cols) {
                curRowY += maxRowH + style.gapY;
                contentH += maxRowH + style.gapY;
                maxRowH = 0;
                colIdx = 0;
            }
            float allocW = colW * span + (span - 1) * style.gapX;
            int effCol = style.isRtl ? (cols - colIdx - span) : colIdx;
            float relColX = effCol * (colW + style.gapX);
            float childX = innerX + relColX - scrollX;
            float cw = 0, ch = 0;
            solveUiNode(child, childX, curRowY, allocW, parentH, style, cKey, childClip, cmds, hitRegions, ctx, cw, ch, effZ);
            maxRowH = std::max(maxRowH, ch);
            maxGridRightX = std::max(maxGridRightX, relColX + std::max(allocW, cw));
            colIdx += span;
            if (colIdx >= cols) {
                curRowY += maxRowH + style.gapY;
                contentH += maxRowH + style.gapY;
                maxRowH = 0;
                colIdx = 0;
            }
        }
        contentH += maxRowH;
        contentW = std::max(innerW, maxGridRightX);
    } else if (rowFlex) {
        bool reverseRow = (style.flexDirection == FlexDir::ROW_REVERSE) ^ style.isRtl;
        std::vector<size_t> visIdx;
        std::vector<std::pair<float, float>> visSizes;
        std::vector<float> visGrows;
        std::vector<ComputedStyle> visStyles;

        for (size_t i = 0; i < node.children.size(); ++i) {
            std::string cKey = resolveNodeKey(node.children[i], nodeKey, static_cast<int>(i));
            ComputedStyle cs = computeNodeStyle(node.children[i], style, cKey, ctx);
            if (!node.children[i].isText && cs.display == DisplayMode::NONE) continue;
            float cw = 0, ch = 0;
            estimateNodeSize(node.children[i], innerW, style, cKey, ctx, cw, ch);
            visIdx.push_back(i);
            visSizes.push_back({cw, ch});
            visGrows.push_back(cs.flexGrow);
            visStyles.push_back(std::move(cs));
        }

        size_t nV = visIdx.size();
        if (style.flexWrap && nV > 0) {
            struct FlexRow { size_t start; size_t end; float totalW; float maxH; };
            std::vector<FlexRow> rows;
            size_t rStart = 0;
            float rW = 0.0f, rH = 0.0f;
            for (size_t v = 0; v < nV; ++v) {
                float cw = visSizes[v].first;
                float ch = visSizes[v].second;
                if (v > rStart && rW + style.gapX + cw > innerW + 0.5f) {
                    rows.push_back({rStart, v, rW, rH});
                    rStart = v;
                    rW = cw;
                    rH = ch;
                } else {
                    rW += (v > rStart ? style.gapX : 0.0f) + cw;
                    rH = std::max(rH, ch);
                }
            }
            rows.push_back({rStart, nV, rW, rH});

            float curY = innerY - scrollY;
            float startY = curY;
            float maxWrapRightX = 0.0f;
            for (size_t ri = 0; ri < rows.size(); ++ri) {
                if (ri > 0) curY += style.gapY;
                const auto &row = rows[ri];
                size_t count = row.end - row.start;
                float leftover = std::max(0.0f, innerW - row.totalW);
                float curX = innerX - scrollX;
                float gap = style.gapX;
                if (style.justifyContent == JustifyContent::CENTER) curX += leftover * 0.5f;
                else if (style.justifyContent == JustifyContent::END) curX += leftover;
                else if (style.justifyContent == JustifyContent::BETWEEN && count > 1) gap += leftover / (count - 1);
                else if (style.justifyContent == JustifyContent::AROUND && count > 0) {
                    float pad = leftover / count;
                    curX += pad * 0.5f;
                    gap += pad;
                } else if (style.justifyContent == JustifyContent::EVENLY) {
                    float pad = leftover / (count + 1);
                    curX += pad;
                    gap += pad;
                }

                float actualRowH = row.maxH;
                for (size_t k = 0; k < count; ++k) {
                    size_t v = reverseRow ? (row.end - 1 - k) : (row.start + k);
                    size_t origI = visIdx[v];
                    std::string cKey = resolveNodeKey(node.children[origI], nodeKey, static_cast<int>(origI));
                    const ComputedStyle &cs = visStyles[v];
                    AlignItems effA = cs.hasAlignSelf ? cs.alignSelf : style.alignItems;
                    float childY = curY;
                    if (effA == AlignItems::CENTER) childY += (row.maxH - visSizes[v].second) * 0.5f;
                    else if (effA == AlignItems::END) childY += (row.maxH - visSizes[v].second);
                    float childStretch = (effA == AlignItems::STRETCH && row.maxH > 0.0f) ? row.maxH : -1.0f;
                    float cw = 0, ch = 0;
                    solveUiNode(node.children[origI], curX, childY, visSizes[v].first, parentH, style, cKey, childClip, cmds, hitRegions, ctx, cw, ch, effZ, childStretch, false, hasDefiniteOwnH);
                    actualRowH = std::max(actualRowH, ch);
                    maxWrapRightX = std::max(maxWrapRightX, (curX + scrollX - innerX) + std::max(visSizes[v].first, cw));
                    curX += visSizes[v].first + gap;
                }
                curY += actualRowH;
            }
            contentH = curY - startY;
            contentW = std::max(innerW, maxWrapRightX);
        } else {
            float totalFixedW = 0.0f;
            float totalGrow = 0.0f;
            float maxChildH = 0.0f;
            for (size_t v = 0; v < nV; ++v) {
                if (visGrows[v] > 0.0f) totalGrow += visGrows[v];
                else totalFixedW += visSizes[v].first;
                maxChildH = std::max(maxChildH, visSizes[v].second);
            }

            float totalGaps = (nV > 1) ? (style.gapX * (nV - 1)) : 0.0f;
            float availFlex = std::max(0.0f, innerW - totalFixedW - totalGaps);
            for (size_t v = 0; v < nV; ++v) {
                if (visGrows[v] > 0.0f && totalGrow > 0.0f) {
                    visSizes[v].first = availFlex * (visGrows[v] / totalGrow);
                }
            }

            float totalAlloc = 0.0f;
            for (const auto &s : visSizes) totalAlloc += s.first;
            float leftover = std::max(0.0f, innerW - totalAlloc - totalGaps);
            float curX = innerX - scrollX;
            float gap = style.gapX;

            if (totalGrow == 0.0f) {
                if (style.justifyContent == JustifyContent::CENTER) curX += leftover * 0.5f;
                else if (style.justifyContent == JustifyContent::END) curX += leftover;
                else if (style.justifyContent == JustifyContent::BETWEEN && nV > 1) gap += leftover / (nV - 1);
                else if (style.justifyContent == JustifyContent::AROUND && nV > 0) {
                    float pad = leftover / nV;
                    curX += pad * 0.5f;
                    gap += pad;
                } else if (style.justifyContent == JustifyContent::EVENLY) {
                    float pad = leftover / (nV + 1);
                    curX += pad;
                    gap += pad;
                }
            }

            float effBoxH = (expBoxH > 0.0f && (style.height > 0.0f || style.heightVh > 0.0f || hasDefinitePctH))
                ? expBoxH
                : (stretchTargetH > 0.0f ? stretchTargetH : (effMaxH > 0.0f ? effMaxH : 0.0f));
            if (effMaxH > 0.0f && effBoxH > effMaxH) effBoxH = effMaxH;
            float effInnerH = (effBoxH > 0.0f) ? std::max(0.0f, effBoxH - style.padding[0] - style.padding[2]) : 0.0f;
            bool rowHasDefiniteH = (style.height > 0.0f || style.heightVh > 0.0f || hasDefinitePctH || effMaxH > 0.0f);
            float maxActualChildH = 0.0f;
            float maxRightX = 0.0f;
            for (size_t idx = 0; idx < nV; ++idx) {
                size_t v = reverseRow ? (nV - 1 - idx) : idx;
                size_t origI = visIdx[v];
                std::string cKey = resolveNodeKey(node.children[origI], nodeKey, static_cast<int>(origI));
                const ComputedStyle &cs = visStyles[v];
                AlignItems effA = cs.hasAlignSelf ? cs.alignSelf : style.alignItems;
                bool childShrinkScroll = nodeHasShrinkableScrollDescendant(node.children[origI], style, cKey, ctx);
                float childStretch = -1.0f;
                float cAllocH = (effInnerH > 0.0f) ? effInnerH : parentH;
                if (effA == AlignItems::STRETCH) {
                    if (effInnerH > 0.0f && (rowHasDefiniteH || childShrinkScroll || style.overflowY != OverflowMode::VISIBLE)) {
                        childStretch = effInnerH;
                    } else {
                        float baseStretch = std::max(maxChildH, effInnerH);
                        if (baseStretch > 0.0f) childStretch = baseStretch;
                    }
                }
                float refRowH = (childStretch > 0.0f) ? std::min(maxChildH, childStretch) : maxChildH;
                float childY = innerY - scrollY;
                if (effA == AlignItems::CENTER) childY += (refRowH - visSizes[v].second) * 0.5f;
                else if (effA == AlignItems::END) childY += (refRowH - visSizes[v].second);
                float cw = 0, ch = 0;
                bool cBounded = hasDefiniteOwnH || (childStretch > 0.0f && (rowHasDefiniteH || childShrinkScroll));
                solveUiNode(node.children[origI], curX, childY, visSizes[v].first, cAllocH, style, cKey, childClip, cmds, hitRegions, ctx, cw, ch, effZ, childStretch, false, cBounded);
                maxActualChildH = std::max(maxActualChildH, ch);
                maxRightX = std::max(maxRightX, (curX + scrollX - innerX) + std::max(visSizes[v].first, cw));
                curX += visSizes[v].first + gap;
            }
            contentH = (maxActualChildH > 0.0f) ? maxActualChildH : maxChildH;
            contentW = std::max(innerW, maxRightX);
        }
    } else {
        float targetBoxH = 0.0f;
        if (style.height > 0.0f) targetBoxH = style.height;
        else {
            if (style.heightVh > 0.0f && ctx.viewportH > 0.0f) targetBoxH = std::max(targetBoxH, ctx.viewportH * (style.heightVh / 100.0f));
            if (hasDefinitePctH) targetBoxH = std::max(targetBoxH, parentH * (style.heightPercent / 100.0f));
            if (style.minHScreen) targetBoxH = std::max(targetBoxH, (isRoot && parentH > 0.0f) ? parentH : ctx.viewportH);
            if (style.minHeight > 0.0f) targetBoxH = std::max(targetBoxH, style.minHeight);
            if (stretchTargetH > 0.0f) targetBoxH = std::max(targetBoxH, stretchTargetH);
            if (effMaxH > 0.0f && targetBoxH <= 0.0f) targetBoxH = effMaxH;
        }
        if (effMaxH > 0.0f && targetBoxH > effMaxH) targetBoxH = effMaxH;
        float targetInnerH = (targetBoxH > 0.0f) ? std::max(0.0f, targetBoxH - style.padding[0] - style.padding[2]) : 0.0f;

        std::vector<float> childStretchH(node.children.size(), -1.0f);
        if (targetInnerH > 0.0f && (style.display == DisplayMode::FLEX || style.display == DisplayMode::INLINE_FLEX)) {
            std::vector<float> natH(node.children.size(), 0.0f);
            std::vector<float> grows(node.children.size(), 0.0f);
            std::vector<float> childMinH(node.children.size(), 0.0f);
            std::vector<bool> canShrinkToFit(node.children.size(), false);
            std::vector<bool> vis(node.children.size(), false);
            float sumH = 0.0f;
            float fixedUnshrinkableH = 0.0f;
            float totalGrow = 0.0f;
            float totalShrinkGrow = 0.0f;
            int visCount = 0;
            for (size_t i = 0; i < node.children.size(); ++i) {
                std::string cKey = resolveNodeKey(node.children[i], nodeKey, static_cast<int>(i));
                ComputedStyle cs = computeNodeStyle(node.children[i], style, cKey, ctx);
                if (!node.children[i].isText && cs.display == DisplayMode::NONE) continue;
                vis[i] = true;
                float cw = 0, ch = 0;
                estimateNodeSize(node.children[i], innerW, style, cKey, ctx, cw, ch);
                natH[i] = ch;
                grows[i] = cs.flexGrow;
                childMinH[i] = cs.minHeight;
                totalGrow += cs.flexGrow;
                bool childBoundedScroll = nodeHasShrinkableScrollDescendant(node.children[i], style, cKey, ctx);
                canShrinkToFit[i] = (cs.flexGrow > 0.0f && childBoundedScroll);
                if (canShrinkToFit[i]) totalShrinkGrow += cs.flexGrow;
                else fixedUnshrinkableH += ch;
                if (visCount > 0) sumH += style.gapY;
                sumH += ch;
                visCount++;
            }
            float totalGaps = (visCount > 1) ? style.gapY * (visCount - 1) : 0.0f;
            bool canGrowExtra = (style.height > 0.0f || style.heightVh > 0.0f || hasDefinitePctH ||
                                 style.minHScreen || style.minHeight > 0.0f || stretchTargetH > 0.0f);
            if (canGrowExtra && totalGrow > 0.0f && targetInnerH > sumH) {
                float extra = targetInnerH - sumH;
                for (size_t i = 0; i < node.children.size(); ++i) {
                    if (vis[i] && grows[i] > 0.0f) {
                        childStretchH[i] = natH[i] + extra * (grows[i] / totalGrow);
                    }
                }
            } else if (totalShrinkGrow > 0.0f && sumH > targetInnerH) {
                bool strictBound = (style.height > 0.0f || style.heightVh > 0.0f || hasDefinitePctH ||
                                    effMaxH > 0.0f || stretchTargetH > 0.0f || parentIsBounded);
                float availShrink = std::max(0.0f, targetInnerH - fixedUnshrinkableH - totalGaps);
                if (strictBound || availShrink >= 160.0f) {
                    for (size_t i = 0; i < node.children.size(); ++i) {
                        if (vis[i] && canShrinkToFit[i]) {
                            float share = availShrink * (grows[i] / totalShrinkGrow);
                            if (childMinH[i] > 0.0f && share < childMinH[i]) share = childMinH[i];
                            childStretchH[i] = std::max(1.0f, share);
                        }
                    }
                }
            }
        }

        float curY = innerY - scrollY;
        float startY = curY;
        float maxColRightX = 0.0f;
        bool anyVisible = false;
        for (size_t i = 0; i < node.children.size(); ++i) {
            std::string cKey = resolveNodeKey(node.children[i], nodeKey, static_cast<int>(i));
            ComputedStyle cs = computeNodeStyle(node.children[i], style, cKey, ctx);
            if (!node.children[i].isText && cs.display == DisplayMode::NONE) continue;
            if (anyVisible) curY += style.gapY;
            anyVisible = true;
            float childX = innerX - scrollX;
            if (style.alignItems == AlignItems::CENTER) {
                float cw = 0, ch = 0;
                estimateNodeSize(node.children[i], innerW, style, cKey, ctx, cw, ch);
                childX += std::max(0.0f, (innerW - cw) * 0.5f);
            } else if (style.alignItems == AlignItems::END) {
                float cw = 0, ch = 0;
                estimateNodeSize(node.children[i], innerW, style, cKey, ctx, cw, ch);
                childX += std::max(0.0f, innerW - cw);
            }
            float cw = 0, ch = 0;
            float cAllocH = (childStretchH[i] > 0.0f)
                ? childStretchH[i]
                : ((targetInnerH > 0.0f && (hasDefiniteOwnH || effMaxH > 0.0f)) ? targetInnerH : parentH);
            bool cBounded = (childStretchH[i] > 0.0f) || hasDefiniteOwnH || (effMaxH > 0.0f);
            solveUiNode(node.children[i], childX, curY, innerW, cAllocH, style, cKey, childClip, cmds, hitRegions, ctx, cw, ch, effZ, childStretchH[i], false, cBounded);
            maxColRightX = std::max(maxColRightX, (childX + scrollX - innerX) + cw);
            curY += ch;
        }
        contentH = curY - startY;
        contentW = std::max(innerW, maxColRightX);
    }

    // Self-drawn native widgets
    bool isFocused = (ctx.interaction && !nodeKey.empty() && ctx.interaction->focusedKey == nodeKey);
    bool isHovered = (ctx.interaction && !nodeKey.empty() && ctx.interaction->hoveredKey == nodeKey);

    if (node.tag == "input" || node.tag == "textarea") {
        std::string val = node.getProp("value");
        std::string placeholder = node.getProp("placeholder");
        std::string inputType = node.getProp("type", "text");
        bool isPassword = (inputType == "password");
        bool isMultiline = (node.tag == "textarea");
        bool isMono = (style.fontFamily == FontFamily::MONO);

        float inputFontH = style.fontSize * 1.35f;
        if (isMultiline) {
            std::string activeStr = val;
            if (isFocused && ctx.interaction) {
                activeStr = ctx.interaction->getOrCreateTextField(nodeKey, val, true, isPassword, node.readOnly, node.disabled).text;
            }
            auto mLines = wrapTextLines(activeStr, innerW, style.fontSize, isMono);
            contentH = std::max(contentH, std::max(3.0f, static_cast<float>(mLines.size())) * inputFontH + 6.0f);
        }
        if (contentH < inputFontH) contentH = inputFontH;
        float boxH = (style.height > 0) ? (style.height - style.padding[0] - style.padding[2]) : std::max(contentH, (style.minHeight > 0 ? style.minHeight - style.padding[0] - style.padding[2] : contentH));

        if (isFocused && ctx.interaction) {
            auto &tf = ctx.interaction->getOrCreateTextField(nodeKey, val, isMultiline, isPassword, node.readOnly, node.disabled);
            tf.emitDrawCommands(cmds, innerX, innerY, innerW, boxH, style.fontSize, style.fontWeight, style.textColor, placeholder, true, isMono, nodeId, effZ);
        } else {
            std::string displayRaw = !val.empty() ? (isPassword ? maskPasswordString(val) : val) : placeholder;
            bool isPh = val.empty() && !placeholder.empty();
            if (!displayRaw.empty()) {
                GpuDrawCmd cp;
                cp.type = "PUSH_CLIP";
                cp.x = innerX - 2.0f; cp.y = innerY - 2.0f;
                cp.width = innerW + 4.0f; cp.height = boxH + 4.0f;
                cp.zIndex = effZ;
                cmds.push_back(cp);

                if (isMultiline) {
                    float lineH = style.fontSize * 1.35f;
                    auto lines = wrapTextLines(displayRaw, innerW, style.fontSize, isMono);
                    for (size_t r = 0; r < lines.size(); ++r) {
                        float ly = innerY + r * lineH;
                        if (ly > innerY + boxH) break;
                        if (lines[r].text.empty()) continue;
                        GpuDrawCmd textCmd;
                        textCmd.type = "DRAW_TEXT";
                        textCmd.nodeId = 0;
                        textCmd.x = innerX;
                        textCmd.y = ly + style.fontSize;
                        textCmd.text = lines[r].text;
                        textCmd.fontSize = style.fontSize;
                        textCmd.fontWeight = style.fontWeight;
                        textCmd.isMono = isMono;
                        textCmd.isSerif = (style.fontFamily == FontFamily::SERIF);
                        textCmd.fontName = style.fontName;
                        textCmd.color = isPh ? "#64748b" : style.textColor.hex;
                        textCmd.alpha = (isPh ? 0.7f : style.textColor.a) * style.opacity;
                        textCmd.zIndex = effZ;
                        cmds.push_back(textCmd);
                    }
                } else {
                    GpuDrawCmd textCmd;
                    textCmd.type = "DRAW_TEXT";
                    textCmd.nodeId = 0;
                    textCmd.x = innerX;
                    textCmd.y = innerY + style.fontSize;
                    textCmd.text = displayRaw;
                    textCmd.fontSize = style.fontSize;
                    textCmd.fontWeight = style.fontWeight;
                    textCmd.isMono = isMono;
                    textCmd.isSerif = (style.fontFamily == FontFamily::SERIF);
                    textCmd.fontName = style.fontName;
                    textCmd.color = isPh ? "#64748b" : style.textColor.hex;
                    textCmd.alpha = (isPh ? 0.7f : style.textColor.a) * style.opacity;
                    textCmd.zIndex = effZ;
                    cmds.push_back(textCmd);
                }

                GpuDrawCmd cpop;
                cpop.type = "POP_CLIP";
                cpop.zIndex = effZ;
                cmds.push_back(cpop);
            }
        }
    } else if (node.tag == "checkbox") {
        float boxSz = std::min(nodeW, 18.0f);
        contentH = std::max(contentH, boxSz);
        GpuDrawCmd box;
        box.type = "FILL_ROUND_RECT";
        box.x = nodeX; box.y = nodeY; box.width = boxSz; box.height = boxSz; box.radius = 4.0f;
        box.color = (node.checked || node.indeterminate) ? "#6366f1" : "#1e293b";
        box.alpha = node.disabled ? 0.45f : 1.0f; box.zIndex = effZ;
        cmds.push_back(box);

        GpuDrawCmd border;
        border.type = "STROKE_ROUND_RECT";
        border.x = nodeX; border.y = nodeY; border.width = boxSz; border.height = boxSz; border.radius = 4.0f;
        border.lineWidth = 1.5f; border.color = isFocused ? "#a5b4fc" : ((node.checked || node.indeterminate) ? "#818cf8" : "#475569");
        border.alpha = 1.0f; border.zIndex = effZ;
        cmds.push_back(border);

        if (node.indeterminate) {
            GpuDrawCmd dash;
            dash.type = "DRAW_LINE";
            dash.x = nodeX + 4.0f; dash.y = nodeY + boxSz * 0.5f;
            dash.width = boxSz - 8.0f; dash.height = 0.0f;
            dash.lineWidth = 2.0f; dash.color = "#ffffff"; dash.alpha = 1.0f; dash.zIndex = effZ;
            cmds.push_back(dash);
        } else if (node.checked) {
            GpuDrawCmd chk;
            chk.type = "DRAW_POLYLINE";
            chk.points = {{nodeX + 4.0f, nodeY + 9.0f}, {nodeX + 7.5f, nodeY + 12.5f}, {nodeX + 14.0f, nodeY + 5.5f}};
            chk.lineWidth = 2.0f; chk.color = "#ffffff"; chk.alpha = 1.0f; chk.zIndex = effZ;
            cmds.push_back(chk);
        }
    } else if (node.tag == "switch") {
        float w = (style.width > 0) ? style.width : 40.0f;
        float h = (style.height > 0) ? style.height : 22.0f;
        contentH = std::max(contentH, h);
        GpuDrawCmd track;
        track.type = "FILL_ROUND_RECT";
        track.x = nodeX; track.y = nodeY; track.width = w; track.height = h; track.radius = h * 0.5f;
        track.color = node.checked ? "#10b981" : "#334155";
        track.alpha = node.disabled ? 0.45f : 1.0f; track.zIndex = effZ;
        cmds.push_back(track);

        float thumbR = (h - 4.0f) * 0.5f;
        float thumbX = node.checked ? (nodeX + w - 2.0f - thumbR * 2.0f) : (nodeX + 2.0f);
        GpuDrawCmd thumb;
        thumb.type = "DRAW_CIRCLE";
        thumb.x = thumbX; thumb.y = nodeY + 2.0f; thumb.width = thumbR * 2.0f; thumb.height = thumbR * 2.0f;
        thumb.radius = thumbR; thumb.color = "#ffffff"; thumb.alpha = 1.0f; thumb.filled = true; thumb.zIndex = effZ;
        cmds.push_back(thumb);
    } else if (node.tag == "radio") {
        float r = 9.0f;
        contentH = std::max(contentH, r * 2.0f);
        GpuDrawCmd outer;
        outer.type = "STROKE_CIRCLE";
        outer.x = nodeX; outer.y = nodeY; outer.width = r * 2.0f; outer.height = r * 2.0f;
        outer.radius = r; outer.lineWidth = 1.8f; outer.color = node.checked ? "#6366f1" : "#64748b";
        outer.alpha = 1.0f; outer.filled = false; outer.zIndex = effZ;
        cmds.push_back(outer);
        if (node.checked) {
            GpuDrawCmd dot;
            dot.type = "DRAW_CIRCLE";
            dot.x = nodeX + 4.5f; dot.y = nodeY + 4.5f; dot.width = r; dot.height = r;
            dot.radius = r * 0.5f; dot.color = "#818cf8"; dot.alpha = 1.0f; dot.filled = true; dot.zIndex = effZ;
            cmds.push_back(dot);
        }
    } else if (node.tag == "slider") {
        double minV = node.getNumProp("min", 0.0);
        double maxV = node.getNumProp("max", 100.0);
        double curV = node.getNumProp("value", 50.0);
        double ratio = (maxV > minV) ? std::clamp((curV - minV) / (maxV - minV), 0.0, 1.0) : 0.0;
        if (style.isRtl) ratio = 1.0 - ratio;
        float trackH = 6.0f;
        float h = (style.height > 0) ? style.height : 24.0f;
        contentH = std::max(contentH, h);
        float trackY = nodeY + (h - trackH) * 0.5f;

        GpuDrawCmd bg;
        bg.type = "FILL_ROUND_RECT";
        bg.x = innerX; bg.y = trackY; bg.width = innerW; bg.height = trackH; bg.radius = 3.0f;
        bg.color = "#1e293b"; bg.alpha = 1.0f; bg.zIndex = effZ;
        cmds.push_back(bg);

        float fillW = static_cast<float>(ratio) * innerW;
        GpuDrawCmd fill;
        fill.type = "FILL_ROUND_RECT";
        fill.x = style.isRtl ? (innerX + innerW - fillW) : innerX;
        fill.y = trackY; fill.width = fillW; fill.height = trackH; fill.radius = 3.0f;
        fill.color = "#6366f1"; fill.alpha = 1.0f; fill.zIndex = effZ;
        cmds.push_back(fill);

        float thumbX = style.isRtl ? (innerX + innerW - fillW) : (innerX + fillW);
        float thumbR = isHovered || isFocused ? 8.5f : 7.5f;
        GpuDrawCmd thumb;
        thumb.type = "DRAW_CIRCLE";
        thumb.x = thumbX - thumbR; thumb.y = nodeY + h * 0.5f - thumbR;
        thumb.width = thumbR * 2.0f; thumb.height = thumbR * 2.0f;
        thumb.radius = thumbR; thumb.color = "#f8fafc"; thumb.alpha = 1.0f; thumb.filled = true; thumb.zIndex = effZ;
        cmds.push_back(thumb);
    } else if (node.tag == "progress") {
        double maxV = std::max(1.0, node.getNumProp("max", 100.0));
        double curV = std::clamp(node.getNumProp("value", 0.0), 0.0, maxV);
        float h = (style.height > 0) ? style.height : 10.0f;
        contentH = std::max(contentH, h);
        GpuDrawCmd bg;
        bg.type = "FILL_ROUND_RECT";
        bg.x = nodeX; bg.y = nodeY; bg.width = nodeW; bg.height = h; bg.radius = h * 0.5f;
        bg.color = "#1e293b"; bg.alpha = 1.0f; bg.zIndex = effZ;
        cmds.push_back(bg);

        GpuDrawCmd fg;
        fg.type = "FILL_ROUND_RECT";
        fg.x = nodeX; fg.y = nodeY; fg.width = nodeW * static_cast<float>(curV / maxV); fg.height = h; fg.radius = h * 0.5f;
        fg.color = "#10b981"; fg.alpha = 1.0f; fg.zIndex = effZ;
        cmds.push_back(fg);
    } else if (node.tag == "svg" || node.tag == "icon") {
        float iconSz = static_cast<float>(node.getNumProp("size", 0.0));
        float w = iconSz > 0 ? iconSz : (style.width > 0 ? style.width : 16.0f);
        float h = iconSz > 0 ? iconSz : (style.height > 0 ? style.height : 16.0f);
        contentH = std::max(contentH, h);
        contentW = std::max(contentW, w);
        std::string pathD = node.getProp("d");
        if (pathD.empty()) pathD = getLucideSvgPath(node.getProp("name"));
        std::string iconCol = node.hasProp("color") ? node.getProp("color") : style.textColor.hex;
        float strokeW = static_cast<float>(node.getNumProp("strokeWidth", 2.0));
        if (!pathD.empty()) {
            GpuDrawCmd sc;
            sc.type = "DRAW_SVG_PATH";
            sc.x = innerX; sc.y = innerY; sc.width = w; sc.height = h;
            sc.svgPath = pathD; sc.color = iconCol; sc.alpha = style.textColor.a * style.opacity;
            sc.lineWidth = strokeW; sc.filled = false; sc.zIndex = effZ;
            cmds.push_back(sc);
        }
        for (const auto &ch : node.children) {
            if (!ch.isText && ch.tag == "path") {
                std::string cd = ch.getProp("d");
                if (!cd.empty()) {
                    GpuDrawCmd sc;
                    sc.type = "DRAW_SVG_PATH";
                    sc.x = innerX; sc.y = innerY; sc.width = w; sc.height = h;
                    sc.svgPath = cd; sc.color = iconCol; sc.alpha = style.textColor.a * style.opacity;
                    sc.lineWidth = strokeW; sc.filled = false; sc.zIndex = effZ;
                    cmds.push_back(sc);
                }
            }
        }
    } else if (node.tag == "canvas" && node.onPaint) {
        float h = (style.height > 0) ? style.height : (style.minHeight > 0 ? style.minHeight : 160.0f);
        contentH = std::max(contentH, h);
        UiPainter painter{&cmds, effZ};
        UiInputState dummyInput;
        node.onPaint(painter, {innerX, innerY, innerW, h}, ctx.input ? *ctx.input : dummyInput);
    }

    if (emitOverflowClip) {
        GpuDrawCmd pop;
        pop.type = "POP_CLIP"; pop.zIndex = effZ;
        cmds.push_back(pop);
    }

    float rawContentH = contentH + style.padding[0] + style.padding[2];
    float rawContentW = contentW + style.padding[1] + style.padding[3];
    float finalH = rawContentH;
    if (style.height > 0.0f) {
        finalH = style.height;
    } else if (style.heightVh > 0.0f && ctx.viewportH > 0.0f) {
        float vhH = ctx.viewportH * (style.heightVh / 100.0f);
        finalH = (isScroll || style.overflowY == OverflowMode::HIDDEN) ? vhH : std::max(finalH, vhH);
    } else if (stretchTargetH > 0.0f && (isScroll || style.overflowY == OverflowMode::HIDDEN)) {
        finalH = stretchTargetH;
    } else if (style.heightPercent > 0.0f && parentH > 0.0f) {
        float pctH = parentH * (style.heightPercent / 100.0f);
        finalH = (isScroll || style.overflowY == OverflowMode::HIDDEN) ? pctH : std::max(finalH, pctH);
    }
    if (style.minHScreen) {
        float screenMinH = (isRoot && parentH > 0.0f) ? parentH : ctx.viewportH;
        finalH = std::max(finalH, screenMinH);
    }
    if (style.minHeight > 0.0f && finalH < style.minHeight) finalH = style.minHeight;
    if (style.height <= 0.0f && !isScroll && style.overflowY != OverflowMode::HIDDEN && stretchTargetH > finalH) {
        finalH = stretchTargetH;
    }
    if (isRoot && isScroll && parentH > 0.0f && style.height <= 0.0f && finalH > parentH) {
        finalH = parentH;
    }
    if (effMaxH > 0.0f && finalH > effMaxH) finalH = effMaxH;
    if (clipCmdIndex != static_cast<size_t>(-1) && clipCmdIndex < cmds.size()) {
        cmds[clipCmdIndex].height = finalH;
    }
    if (emitOverflowClip && hitRegions) {
        UiRect actualClip = clipRect.intersect({nodeX, nodeY, nodeW, finalH});
        for (size_t hi = hitBgIndex; hi < hitRegions->size(); ++hi) {
            (*hitRegions)[hi].clipRect = (*hitRegions)[hi].clipRect.intersect(actualClip);
        }
    }

    // Draw scrollbar thumbs when scroll container overflows vertically or horizontally
    bool canScrollY = (hasVertScroll || (isRoot && style.overflowY != OverflowMode::HIDDEN && style.height <= 0.0f));
    bool canScrollX = hasHorizScroll;
    float maxScrollY = canScrollY ? std::max(0.0f, rawContentH - finalH) : 0.0f;
    float maxScrollX = canScrollX ? std::max(0.0f, rawContentW - nodeW) : 0.0f;
    if (isRoot && !emitOverflowClip && (maxScrollY > 0.5f || scrollY > 0.5f)) {
        GpuDrawCmd pc;
        pc.type = "PUSH_CLIP";
        pc.x = nodeX; pc.y = nodeY; pc.width = nodeW; pc.height = finalH; pc.zIndex = effZ;
        cmds.insert(cmds.begin() + static_cast<std::ptrdiff_t>(bgIndex), pc);
        GpuDrawCmd pop;
        pop.type = "POP_CLIP"; pop.zIndex = effZ;
        cmds.push_back(pop);
    }
    if (canScrollY && maxScrollY > 2.0f && finalH > 16.0f) {
        float clampedScrollY = std::clamp(scrollY, 0.0f, maxScrollY);
        float thumbH = std::max(20.0f, finalH * (finalH / rawContentH));
        float thumbY = nodeY + (clampedScrollY / maxScrollY) * (finalH - thumbH);
        GpuDrawCmd sb;
        sb.type = "FILL_ROUND_RECT";
        sb.x = nodeX + nodeW - 6.0f; sb.y = thumbY; sb.width = 4.0f; sb.height = thumbH;
        sb.radius = 2.0f; sb.color = "#64748b"; sb.alpha = 0.72f; sb.zIndex = effZ;
        cmds.push_back(sb);
    }
    if (canScrollX && maxScrollX > 2.0f && nodeW > 16.0f) {
        float clampedScrollX = std::clamp(scrollX, 0.0f, maxScrollX);
        float thumbW = std::max(20.0f, nodeW * (nodeW / rawContentW));
        float thumbX = nodeX + (clampedScrollX / maxScrollX) * (nodeW - thumbW);
        GpuDrawCmd sbh;
        sbh.type = "FILL_ROUND_RECT";
        sbh.x = thumbX; sbh.y = nodeY + finalH - 6.0f; sbh.width = thumbW; sbh.height = 4.0f;
        sbh.radius = 2.0f; sbh.color = "#64748b"; sbh.alpha = 0.72f; sbh.zIndex = effZ;
        cmds.push_back(sbh);
    }

    if (ctx.interaction && !nodeKey.empty()) {
        ctx.interaction->nodeBoundsByKey[nodeKey] = {nodeX, nodeY, nodeW, finalH};
    }

    if (hitRegions && nodeId > 0 && !style.pointerEventsNone) {
        UiHitRegion hr;
        hr.id = nodeId;
        hr.nodeKey = nodeKey;
        hr.x = nodeX; hr.y = nodeY; hr.width = nodeW; hr.height = finalH;
        hr.clipRect = clipRect;
        hr.zIndex = effZ;
        hr.tag = node.tag;
        hr.role = node.role.empty() ? node.tag : node.role;
        hr.accessibleName = collectNodeAccessibleText(node);
        hr.testId = node.testId;
        hr.cursor = !style.cursor.empty() ? style.cursor : (hasClick ? "pointer" : ((node.tag == "input" || node.tag == "textarea") ? "text" : "default"));
        hr.disabled = node.disabled;
        hr.readOnly = node.readOnly;
        hr.focused = isFocused;
        hr.hovered = isHovered;
        hr.focusable = isFocusable && !node.disabled;
        hr.tabIndex = (node.tabIndex != -999) ? node.tabIndex : (hr.focusable ? 0 : -1);
        hr.hasClick = hasClick;
        hr.hasChange = hasChange;
        hr.hasKeyDown = node.handlers.count("keydown") > 0;
        hr.hasPointerDown = node.handlers.count("pointerdown") > 0;
        hr.hasScroll = (isScroll || hasWheelOrScrollHandler);
        hr.isScrollContainer = isScroll;
        hr.overscrollContain = style.overscrollContain;
        hr.hasWheelHandler = (node.handlers.count("wheel") > 0);
        hr.scrollX = scrollX;
        hr.scrollY = scrollY;
        hr.maxScrollX = maxScrollX;
        hr.maxScrollY = maxScrollY;
        hr.contentW = rawContentW;
        hr.contentH = rawContentH;
        hr.inputValue = node.getProp("value");
        hr.placeholder = node.getProp("placeholder");
        hr.isPassword = (node.getProp("type") == "password");
        hr.isMultiline = (node.tag == "textarea");
        hr.checked = node.checked;
        hr.indeterminate = node.indeterminate;
        hr.selected = node.selected;
        hr.expanded = node.expanded;
        hr.numValue = node.getNumProp("value", 0.0);
        hr.numMin = node.getNumProp("min", 0.0);
        hr.numMax = node.getNumProp("max", 100.0);
        hr.numStep = node.getNumProp("step", 1.0);
        hr.isRtl = style.isRtl;
        hr.ariaProps = node.aria;
        if (hitBgIndex <= hitRegions->size()) {
            hitRegions->insert(hitRegions->begin() + static_cast<std::ptrdiff_t>(hitBgIndex), std::move(hr));
        } else {
            hitRegions->push_back(std::move(hr));
        }
    }

    // Background / Gradient / Shadow inserted BEFORE children
    if (style.hasShadow && !style.srOnly) {
        GpuDrawCmd sh;
        sh.type = "SHADOW_ROUND_RECT";
        sh.nodeId = nodeId;
        sh.x = nodeX; sh.y = nodeY; sh.width = nodeW; sh.height = finalH;
        sh.radius = style.borderRadius;
        sh.color = "#000000";
        sh.alpha = style.shadowAlpha * style.opacity;
        sh.blur = style.shadowBlur;
        sh.lineWidth = style.shadowOffsetY;
        sh.zIndex = effZ;
        cmds.insert(cmds.begin() + bgIndex, sh);
        bgIndex++;
    }

    if (style.hasBackground && !style.srOnly) {
        GpuDrawCmd bgCmd;
        if (style.hasGradient) {
            bgCmd.type = "FILL_GRADIENT_RECT";
            bgCmd.color = style.gradientFrom.hex;
            bgCmd.alpha = style.gradientFrom.a * style.opacity;
            bgCmd.color2 = style.gradientTo.hex;
            bgCmd.alpha2 = style.gradientTo.a * style.opacity;
            bgCmd.angle = style.gradientAngle;
        } else {
            bgCmd.type = "FILL_ROUND_RECT";
            bgCmd.color = style.backgroundColor.hex;
            bgCmd.alpha = style.backgroundColor.a * style.opacity;
        }
        bgCmd.nodeId = nodeId;
        bgCmd.x = nodeX; bgCmd.y = nodeY; bgCmd.width = nodeW; bgCmd.height = finalH;
        bgCmd.radius = style.borderRadius;
        bgCmd.zIndex = effZ;
        cmds.insert(cmds.begin() + bgIndex, bgCmd);
    }

    if (style.borderWidth > 0 && !style.srOnly) {
        std::string bCol = (style.borderColor.a > 0.0f) ? style.borderColor.hex : "#334155";
        float bAlpha = ((style.borderColor.a > 0.0f) ? style.borderColor.a : 1.0f) * style.opacity;
        if (style.hasPerSideBorder) {
            auto emitEdge = [&](float ex, float ey, float ew, float eh, float lw) {
                GpuDrawCmd edge;
                edge.type = "DRAW_LINE";
                edge.nodeId = nodeId;
                edge.x = ex; edge.y = ey; edge.width = ew; edge.height = eh;
                edge.lineWidth = lw;
                edge.color = bCol;
                edge.alpha = bAlpha;
                edge.zIndex = effZ;
                cmds.push_back(edge);
            };
            if (style.borderSides[0] > 0) emitEdge(nodeX, nodeY, nodeW, 0.0f, style.borderWidth);
            if (style.borderSides[1] > 0) emitEdge(nodeX + nodeW, nodeY, 0.0f, finalH, style.borderWidth);
            if (style.borderSides[2] > 0) emitEdge(nodeX, nodeY + finalH, nodeW, 0.0f, style.borderWidth);
            if (style.borderSides[3] > 0) emitEdge(nodeX, nodeY, 0.0f, finalH, style.borderWidth);
        } else {
            GpuDrawCmd bCmd;
            bCmd.type = "STROKE_ROUND_RECT";
            bCmd.nodeId = nodeId;
            bCmd.x = nodeX; bCmd.y = nodeY; bCmd.width = nodeW; bCmd.height = finalH;
            bCmd.radius = style.borderRadius;
            bCmd.lineWidth = style.borderWidth;
            bCmd.color = bCol;
            bCmd.alpha = bAlpha;
            bCmd.zIndex = effZ;
            cmds.push_back(bCmd);
        }
    }

    outW = nodeW;
    outH = finalH + style.margin[0] + style.margin[2];
}

inline std::vector<GpuDrawCmd> computeUiLayout(
    const UiNode &root,
    float viewportW = 800.0f,
    float viewportH = 600.0f,
    std::vector<UiHitRegion> *outHitRegions = nullptr,
    UiInteractionState *interaction = nullptr,
    const UiInputState *input = nullptr)
{
    LayoutEngineContext ctx;
    ctx.viewportW = viewportW;
    ctx.viewportH = viewportH;
    ctx.rootRtl = input ? input->rtl : false;
    ctx.interaction = interaction;
    ctx.input = input;

    ComputedStyle rootStyle;
    rootStyle.isRtl = ctx.rootRtl;
    std::vector<GpuDrawCmd> cmds;
    float outW = 0.0f, outH = 0.0f;
    float safeL = input ? input->safeLeft : 0.0f;
    float safeT = input ? input->safeTop : 0.0f;
    float safeW = std::max(0.0f, viewportW - safeL - (input ? input->safeRight : 0.0f));
    float safeH = std::max(0.0f, viewportH - safeT - (input ? input->safeBottom : 0.0f));
    solveUiNode(root, safeL, safeT, safeW, safeH, rootStyle, "root", {0, 0, viewportW, viewportH}, cmds, outHitRegions, ctx, outW, outH);

    // Append deferred portal / z-indexed overlay layers in ascending z-order
    if (!ctx.portalLayers.empty()) {
        std::stable_sort(ctx.portalLayers.begin(), ctx.portalLayers.end(), [](const auto &a, const auto &b) {
            return a.zIndex < b.zIndex;
        });
        for (const auto &layer : ctx.portalLayers) {
            cmds.insert(cmds.end(), layer.cmds.begin(), layer.cmds.end());
            if (outHitRegions) {
                outHitRegions->insert(outHitRegions->end(), layer.hits.begin(), layer.hits.end());
            }
        }
    }

    return cmds;
}

} // namespace MemeUI
