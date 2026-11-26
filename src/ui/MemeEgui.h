#pragma once

#include "MemeUiTypes.h"
#include "MemeUiTextEngine.h"
#include "MemeUiLayout.h"
#include "MemeUiPrimitives.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <cmath>

namespace MemeUI {

struct EguiResponse {
    std::string id;
    UiRect rect;
    bool clicked = false;
    bool doubleClicked = false;
    bool hovered = false;
    bool active = false;
    bool hasFocus = false;
    bool changed = false;
    bool dragged = false;
    bool boolVal = false;
    double numVal = 0.0;
    std::string strVal;
};

struct EguiContext;

class EguiUi {
public:
    EguiContext *ctx = nullptr;
    UiRect maxRect;
    float cursorX = 0.0f;
    float cursorY = 0.0f;
    float maxRight = 0.0f;
    float maxBottom = 0.0f;
    bool isHorizontal = false;
    float spacingX = 8.0f;
    float spacingY = 6.0f;
    float rowHeight = 0.0f;
    int zIndex = 0;

    EguiUi(EguiContext *c, const UiRect &bounds, bool horiz = false, int z = 0)
        : ctx(c), maxRect(bounds), cursorX(bounds.x), cursorY(bounds.y),
          maxRight(bounds.x), maxBottom(bounds.y), isHorizontal(horiz), zIndex(z) {}

    float availableWidth() const {
        return std::max(0.0f, (maxRect.x + maxRect.width) - cursorX);
    }

    float availableHeight() const {
        return std::max(0.0f, (maxRect.y + maxRect.height) - cursorY);
    }

    UiRect allocateSize(float w, float h);
    void advanceAfter(const UiRect &r);

    UiPainter painter() const;

    void addSpace(float px) {
        if (isHorizontal) cursorX += px;
        else cursorY += px;
    }

    void separator();
    EguiResponse heading(const std::string &text);
    EguiResponse label(const std::string &text);
    EguiResponse coloredLabel(const std::string &hexColor, const std::string &text);
    EguiResponse monospace(const std::string &text);
    EguiResponse small(const std::string &text);
    EguiResponse strong(const std::string &text);
    EguiResponse button(const std::string &labelText, float minW = 0.0f, float minH = 28.0f);
    EguiResponse smallButton(const std::string &labelText);
    EguiResponse selectableLabel(bool selected, const std::string &labelText);
    EguiResponse checkbox(const std::string &id, bool &checked, const std::string &labelText);
    EguiResponse toggleSwitch(const std::string &id, bool &checked, const std::string &labelText);
    EguiResponse radioValue(const std::string &id, bool selected, const std::string &labelText);
    EguiResponse slider(const std::string &id, double &val, double minV, double maxV, const std::string &labelText = "", double step = 0.0);
    EguiResponse dragValue(const std::string &id, double &val, double minV, double maxV, double speed = 0.5);
    EguiResponse progressBar(double fraction, const std::string &overlayText = "");
    EguiResponse textEditSingleline(const std::string &id, std::string &text, const std::string &placeholder = "", bool password = false, float width = 0.0f);
    EguiResponse textEditMultiline(const std::string &id, std::string &text, const std::string &placeholder = "", float width = 0.0f, float height = 96.0f);
    EguiResponse comboBox(const std::string &id, std::string &selected, const std::vector<std::string> &options);
    void spinner(float size = 18.0f);

    void horizontal(const std::function<void(EguiUi &)> &fn);
    void vertical(const std::function<void(EguiUi &)> &fn);
    void group(const std::function<void(EguiUi &)> &fn);
    void collapsing(const std::string &id, const std::string &header, bool defaultOpen, const std::function<void(EguiUi &)> &fn);
    void scrollArea(const std::string &id, float maxH, const std::function<void(EguiUi &)> &fn);
    void columns(int numCols, const std::function<void(std::vector<EguiUi> &)> &fn);
};

struct EguiContext {
    UiInputState input;
    UiInteractionState interaction;
    std::vector<GpuDrawCmd> cmds;
    std::vector<UiHitRegion> hitRegions;
    UiRect remainingRect;
    int nextAutoId = 1;

    // Immediate-mode persistent states
    std::unordered_map<std::string, bool> collapsingOpen;
    std::unordered_map<std::string, std::string> openComboIdMap;
    std::string activeComboId;
    std::unordered_map<std::string, UiPoint> windowPositions;
    std::unordered_map<std::string, bool> draggingWindow;
    std::unordered_map<std::string, bool> draggingSlider;
    std::string activeTextId;

    // Deferred popups (combo boxes, tooltips) rendered at top z-order
    std::vector<std::function<void()>> deferredPopups;
    bool didScroll = false;

    void beginFrame(const UiInputState &inState) {
        input = inState;
        didScroll = false;
        cmds.clear();
        hitRegions.clear();
        deferredPopups.clear();
        nextAutoId = 1;
        remainingRect = {input.safeLeft, input.safeTop, std::max(0.0f, input.viewportW - input.safeLeft - input.safeRight), std::max(0.0f, input.viewportH - input.safeTop - input.safeBottom)};

        // Tick blinking caret
        if (!interaction.focusedKey.empty()) {
            auto it = interaction.textFields.find(interaction.focusedKey);
            if (it != interaction.textFields.end()) {
                it->second.tickBlink(input.deltaMs);
            }
        }
        if (input.mouseReleased[0]) {
            for (auto &kv : draggingSlider) kv.second = false;
            for (auto &kv : draggingWindow) kv.second = false;
            interaction.draggingScrollbar = false;
            interaction.activeKey.clear();
            for (auto &kv : interaction.textFields) kv.second.handlePointerUp();
        }

        // Background clear
        GpuDrawCmd bg;
        bg.type = "FILL_ROUND_RECT";
        bg.x = 0; bg.y = 0; bg.width = input.viewportW; bg.height = input.viewportH;
        bg.color = "#020617"; bg.alpha = 1.0f;
        cmds.push_back(bg);
    }

    void endFrame() {
        for (auto &fn : deferredPopups) fn();
        input.wheelDeltaX = 0.0f;
        input.wheelDeltaY = 0.0f;
    }

    std::string makeWidgetId(const std::string &labelOrKey) {
        if (!labelOrKey.empty()) return labelOrKey;
        return "egui_w_" + std::to_string(nextAutoId++);
    }

    int registerHitRegion(const std::string &wKey, const UiRect &rect, const std::string &tag, const std::string &role, const std::string &name, bool clickable = false, bool editable = false, int z = 0) {
        int id = nextAutoId++;
        UiHitRegion hr;
        hr.id = id;
        hr.nodeKey = wKey;
        hr.x = rect.x; hr.y = rect.y; hr.width = rect.width; hr.height = rect.height;
        hr.zIndex = z;
        hr.tag = tag;
        hr.role = role;
        hr.accessibleName = name;
        hr.hasClick = clickable;
        hr.hasChange = editable;
        hr.focusable = clickable || editable;
        hr.focused = (interaction.focusedKey == wKey);
        hr.hovered = rect.contains(input.pointerX, input.pointerY);
        hr.cursor = clickable ? "pointer" : (editable ? "text" : "default");
        hitRegions.push_back(hr);
        return id;
    }

    void topPanel(const std::string &id, float minHeight, const std::function<void(EguiUi &)> &fn) {
        (void)id;
        float h = std::min(remainingRect.height, std::max(36.0f, minHeight));
        UiRect panelRect = {remainingRect.x, remainingRect.y, remainingRect.width, h};
        GpuDrawCmd bg;
        bg.type = "FILL_ROUND_RECT";
        bg.x = panelRect.x; bg.y = panelRect.y; bg.width = panelRect.width; bg.height = panelRect.height;
        bg.color = "#0f172a"; bg.alpha = 1.0f;
        size_t bgIdx = cmds.size();
        cmds.push_back(bg);

        EguiUi ui(this, {panelRect.x + 12.0f, panelRect.y + 8.0f, std::max(0.0f, panelRect.width - 24.0f), std::max(0.0f, panelRect.height - 16.0f)});
        if (fn) fn(ui);
        float usedH = std::max(h, (ui.maxBottom - panelRect.y) + 8.0f);
        cmds[bgIdx].height = usedH;

        GpuDrawCmd border;
        border.type = "DRAW_LINE";
        border.x = panelRect.x; border.y = panelRect.y + usedH; border.width = panelRect.width; border.height = 0;
        border.lineWidth = 1.0f; border.color = "#1e293b"; border.alpha = 1.0f;
        cmds.push_back(border);

        remainingRect.y += usedH;
        remainingRect.height = std::max(0.0f, remainingRect.height - usedH);
    }

    void bottomPanel(const std::string &id, float height, const std::function<void(EguiUi &)> &fn) {
        (void)id;
        float h = std::min(remainingRect.height, std::max(36.0f, height));
        UiRect panelRect = {remainingRect.x, remainingRect.y + remainingRect.height - h, remainingRect.width, h};
        GpuDrawCmd bg;
        bg.type = "FILL_ROUND_RECT";
        bg.x = panelRect.x; bg.y = panelRect.y; bg.width = panelRect.width; bg.height = panelRect.height;
        bg.color = "#0f172a"; bg.alpha = 1.0f;
        cmds.push_back(bg);

        GpuDrawCmd border;
        border.type = "DRAW_LINE";
        border.x = panelRect.x; border.y = panelRect.y; border.width = panelRect.width; border.height = 0;
        border.lineWidth = 1.0f; border.color = "#1e293b"; border.alpha = 1.0f;
        cmds.push_back(border);

        EguiUi ui(this, {panelRect.x + 12.0f, panelRect.y + 8.0f, std::max(0.0f, panelRect.width - 24.0f), std::max(0.0f, panelRect.height - 16.0f)});
        if (fn) fn(ui);

        remainingRect.height = std::max(0.0f, remainingRect.height - h);
    }

    void leftPanel(const std::string &id, float width, const std::function<void(EguiUi &)> &fn) {
        (void)id;
        float w = std::min(remainingRect.width, std::max(120.0f, width));
        UiRect panelRect = {remainingRect.x, remainingRect.y, w, remainingRect.height};
        GpuDrawCmd bg;
        bg.type = "FILL_ROUND_RECT";
        bg.x = panelRect.x; bg.y = panelRect.y; bg.width = panelRect.width; bg.height = panelRect.height;
        bg.color = "#0f172a"; bg.alpha = 0.96f;
        cmds.push_back(bg);

        GpuDrawCmd border;
        border.type = "DRAW_LINE";
        border.x = panelRect.x + w; border.y = panelRect.y; border.width = 0; border.height = panelRect.height;
        border.lineWidth = 1.0f; border.color = "#1e293b"; border.alpha = 1.0f;
        cmds.push_back(border);

        EguiUi ui(this, {panelRect.x + 12.0f, panelRect.y + 10.0f, std::max(0.0f, panelRect.width - 24.0f), std::max(0.0f, panelRect.height - 20.0f)});
        if (fn) fn(ui);

        remainingRect.x += w;
        remainingRect.width = std::max(0.0f, remainingRect.width - w);
    }

    void rightPanel(const std::string &id, float width, const std::function<void(EguiUi &)> &fn) {
        (void)id;
        float w = std::min(remainingRect.width, std::max(120.0f, width));
        UiRect panelRect = {remainingRect.x + remainingRect.width - w, remainingRect.y, w, remainingRect.height};
        GpuDrawCmd bg;
        bg.type = "FILL_ROUND_RECT";
        bg.x = panelRect.x; bg.y = panelRect.y; bg.width = panelRect.width; bg.height = panelRect.height;
        bg.color = "#0f172a"; bg.alpha = 0.96f;
        cmds.push_back(bg);

        GpuDrawCmd border;
        border.type = "DRAW_LINE";
        border.x = panelRect.x; border.y = panelRect.y; border.width = 0; border.height = panelRect.height;
        border.lineWidth = 1.0f; border.color = "#1e293b"; border.alpha = 1.0f;
        cmds.push_back(border);

        EguiUi ui(this, {panelRect.x + 12.0f, panelRect.y + 10.0f, std::max(0.0f, panelRect.width - 24.0f), std::max(0.0f, panelRect.height - 20.0f)});
        if (fn) fn(ui);

        remainingRect.width = std::max(0.0f, remainingRect.width - w);
    }

    void centralPanel(const std::function<void(EguiUi &)> &fn) {
        EguiUi ui(this, {remainingRect.x + 14.0f, remainingRect.y + 12.0f, std::max(0.0f, remainingRect.width - 28.0f), std::max(0.0f, remainingRect.height - 24.0f)});
        if (fn) fn(ui);
    }

    void window(const std::string &id, const std::string &title, float defaultX, float defaultY, float width, const std::function<void(EguiUi &)> &fn) {
        if (windowPositions.find(id) == windowPositions.end()) {
            windowPositions[id] = {defaultX, defaultY};
        }
        UiPoint &pos = windowPositions[id];
        UiRect titleBar = {pos.x, pos.y, width, 28.0f};
        if (input.mousePressed[0] && titleBar.contains(input.pointerX, input.pointerY)) {
            draggingWindow[id] = true;
        }
        if (draggingWindow[id] && input.mouseDown[0]) {
            pos.x = std::clamp(pos.x + (input.pointerX - input.prevPointerX), 0.0f, std::max(0.0f, input.viewportW - width));
            pos.y = std::clamp(pos.y + (input.pointerY - input.prevPointerY), 0.0f, std::max(0.0f, input.viewportH - 40.0f));
        }

        size_t shIdx = cmds.size();
        GpuDrawCmd shadow;
        shadow.type = "SHADOW_ROUND_RECT";
        shadow.x = pos.x; shadow.y = pos.y; shadow.width = width; shadow.height = 100.0f;
        shadow.radius = 10.0f; shadow.blur = 14.0f; shadow.lineWidth = 4.0f; shadow.alpha = 0.4f; shadow.zIndex = 50;
        cmds.push_back(shadow);

        size_t bgIdx = cmds.size();
        GpuDrawCmd bg;
        bg.type = "FILL_ROUND_RECT";
        bg.x = pos.x; bg.y = pos.y; bg.width = width; bg.height = 100.0f;
        bg.radius = 10.0f; bg.color = "#0f172a"; bg.alpha = 0.96f; bg.zIndex = 50;
        cmds.push_back(bg);

        GpuDrawCmd tb;
        tb.type = "FILL_ROUND_RECT";
        tb.x = pos.x; tb.y = pos.y; tb.width = width; tb.height = 28.0f;
        tb.radius = 10.0f; tb.color = "#1e293b"; tb.alpha = 1.0f; tb.zIndex = 50;
        cmds.push_back(tb);

        GpuDrawCmd tt;
        tt.type = "DRAW_TEXT";
        tt.x = pos.x + 10.0f; tt.y = pos.y + 19.0f;
        tt.text = title; tt.fontSize = 13.0f; tt.fontWeight = 700; tt.color = "#f8fafc"; tt.zIndex = 50;
        cmds.push_back(tt);

        EguiUi ui(this, {pos.x + 12.0f, pos.y + 36.0f, std::max(20.0f, width - 24.0f), 400.0f}, false, 50);
        if (fn) fn(ui);
        float totalH = std::max(64.0f, (ui.maxBottom - pos.y) + 12.0f);
        cmds[shIdx].height = totalH;
        cmds[bgIdx].height = totalH;

        GpuDrawCmd border;
        border.type = "STROKE_ROUND_RECT";
        border.x = pos.x; border.y = pos.y; border.width = width; border.height = totalH;
        border.radius = 10.0f; border.lineWidth = 1.0f; border.color = "#334155"; border.zIndex = 50;
        cmds.push_back(border);
    }
};

inline UiRect EguiUi::allocateSize(float w, float h) {
    UiRect r = {cursorX, cursorY, w, h};
    advanceAfter(r);
    return r;
}

inline void EguiUi::advanceAfter(const UiRect &r) {
    maxRight = std::max(maxRight, r.x + r.width);
    maxBottom = std::max(maxBottom, r.y + r.height);
    if (isHorizontal) {
        cursorX += r.width + spacingX;
        rowHeight = std::max(rowHeight, r.height);
    } else {
        cursorY += r.height + spacingY;
    }
}

inline UiPainter EguiUi::painter() const {
    return UiPainter{&ctx->cmds, zIndex};
}

inline void EguiUi::separator() {
    if (isHorizontal) {
        UiRect r = allocateSize(1.0f, 20.0f);
        painter().line(r.x, r.y, r.x, r.y + r.height, 1.0f, "#334155");
    } else {
        UiRect r = allocateSize(availableWidth(), 1.0f);
        painter().line(r.x, r.y, r.x + r.width, r.y, 1.0f, "#334155");
    }
}

inline EguiResponse EguiUi::coloredLabel(const std::string &hexColor, const std::string &text) {
    float fs = 14.0f;
    float w = measureTextWidth(text, fs, false);
    float h = fs * 1.35f;
    UiRect r = allocateSize(w, h);
    painter().text(r.x, r.y + 1.0f, text, fs, hexColor, 400, false);
    EguiResponse res;
    res.rect = r;
    res.hovered = r.contains(ctx->input.pointerX, ctx->input.pointerY);
    return res;
}

inline EguiResponse EguiUi::label(const std::string &text) {
    return coloredLabel("#e2e8f0", text);
}

inline EguiResponse EguiUi::heading(const std::string &text) {
    float fs = 20.0f;
    float w = measureTextWidth(text, fs, false);
    float h = fs * 1.35f;
    UiRect r = allocateSize(w, h);
    painter().text(r.x, r.y + 1.0f, text, fs, "#ffffff", 700, false);
    EguiResponse res; res.rect = r; return res;
}

inline EguiResponse EguiUi::monospace(const std::string &text) {
    float fs = 13.0f;
    float w = measureTextWidth(text, fs, true);
    float h = fs * 1.35f;
    UiRect r = allocateSize(w, h);
    painter().text(r.x, r.y + 1.0f, text, fs, "#cbd5e1", 400, true);
    EguiResponse res; res.rect = r; return res;
}

inline EguiResponse EguiUi::small(const std::string &text) {
    float fs = 12.0f;
    float w = measureTextWidth(text, fs, false);
    float h = fs * 1.35f;
    UiRect r = allocateSize(w, h);
    painter().text(r.x, r.y + 1.0f, text, fs, "#94a3b8", 400, false);
    EguiResponse res; res.rect = r; return res;
}

inline EguiResponse EguiUi::strong(const std::string &text) {
    float fs = 14.0f;
    float w = measureTextWidth(text, fs, false);
    float h = fs * 1.35f;
    UiRect r = allocateSize(w, h);
    painter().text(r.x, r.y + 1.0f, text, fs, "#ffffff", 700, false);
    EguiResponse res; res.rect = r; return res;
}

inline EguiResponse EguiUi::button(const std::string &labelText, float minW, float minH) {
    float fs = 13.0f;
    float textW = measureTextWidth(labelText, fs, false);
    float w = std::max(minW, textW + 24.0f);
    float h = std::max(minH, fs + 14.0f);
    UiRect r = allocateSize(w, h);
    std::string wid = "egui_btn_" + labelText + "_" + std::to_string(ctx->nextAutoId);
    ctx->registerHitRegion(wid, r, "button", "button", labelText, true, false, zIndex);

    bool hov = r.contains(ctx->input.pointerX, ctx->input.pointerY);
    bool act = hov && ctx->input.mouseDown[0];
    bool clk = hov && ctx->input.mouseReleased[0];

    std::string bgCol = act ? "#4338ca" : (hov ? "#6366f1" : "#4f46e5");
    painter().rectFilled(r.x, r.y, r.width, r.height, 8.0f, bgCol, 1.0f);
    painter().rectStroke(r.x, r.y, r.width, r.height, 8.0f, 1.0f, "#818cf8", hov ? 0.8f : 0.35f);
    painter().text(r.x + (r.width - textW) * 0.5f, r.y + (r.height - fs) * 0.42f, labelText, fs, "#ffffff", 600);

    EguiResponse res;
    res.id = wid; res.rect = r; res.hovered = hov; res.active = act; res.clicked = clk;
    return res;
}

inline EguiResponse EguiUi::smallButton(const std::string &labelText) {
    return button(labelText, 0.0f, 22.0f);
}

inline EguiResponse EguiUi::selectableLabel(bool selected, const std::string &labelText) {
    float fs = 13.0f;
    float textW = measureTextWidth(labelText, fs, false);
    UiRect r = allocateSize(textW + 18.0f, 26.0f);
    std::string wid = "egui_sel_" + labelText + "_" + std::to_string(ctx->nextAutoId);
    ctx->registerHitRegion(wid, r, "button", "option", labelText, true, false, zIndex);

    bool hov = r.contains(ctx->input.pointerX, ctx->input.pointerY);
    bool clk = hov && ctx->input.mouseReleased[0];
    if (selected || hov) {
        painter().rectFilled(r.x, r.y, r.width, r.height, 6.0f, selected ? "#4f46e5" : "#1e293b", selected ? 0.9f : 0.8f);
    }
    painter().text(r.x + 9.0f, r.y + 5.0f, labelText, fs, selected ? "#ffffff" : "#cbd5e1", selected ? 600 : 400);

    EguiResponse res;
    res.id = wid; res.rect = r; res.hovered = hov; res.clicked = clk; res.boolVal = selected;
    return res;
}

inline EguiResponse EguiUi::checkbox(const std::string &id, bool &checked, const std::string &labelText) {
    float fs = 14.0f;
    float textW = measureTextWidth(labelText, fs, false);
    UiRect r = allocateSize(26.0f + textW, 22.0f);
    std::string wid = ctx->makeWidgetId(id.empty() ? ("chk_" + labelText) : id);
    ctx->registerHitRegion(wid, r, "checkbox", "checkbox", labelText, true, true, zIndex);

    bool hov = r.contains(ctx->input.pointerX, ctx->input.pointerY);
    bool clk = hov && ctx->input.mouseReleased[0];
    bool changed = false;
    if (clk) { checked = !checked; changed = true; }

    painter().rectFilled(r.x, r.y + 2.0f, 18.0f, 18.0f, 4.0f, checked ? "#6366f1" : "#1e293b");
    painter().rectStroke(r.x, r.y + 2.0f, 18.0f, 18.0f, 4.0f, 1.5f, hov ? "#a5b4fc" : (checked ? "#818cf8" : "#475569"));
    if (checked) {
        painter().polyline({{r.x + 4.0f, r.y + 11.0f}, {r.x + 7.5f, r.y + 14.5f}, {r.x + 14.0f, r.y + 7.5f}}, 2.0f, "#ffffff");
    }
    painter().text(r.x + 25.0f, r.y + 3.0f, labelText, fs, "#e2e8f0", 400);

    EguiResponse res;
    res.id = wid; res.rect = r; res.hovered = hov; res.clicked = clk; res.changed = changed; res.boolVal = checked;
    return res;
}

inline EguiResponse EguiUi::toggleSwitch(const std::string &id, bool &checked, const std::string &labelText) {
    float fs = 14.0f;
    float textW = measureTextWidth(labelText, fs, false);
    UiRect r = allocateSize(48.0f + textW, 24.0f);
    std::string wid = ctx->makeWidgetId(id.empty() ? ("sw_" + labelText) : id);
    ctx->registerHitRegion(wid, r, "switch", "switch", labelText, true, true, zIndex);

    bool hov = r.contains(ctx->input.pointerX, ctx->input.pointerY);
    bool clk = hov && ctx->input.mouseReleased[0];
    bool changed = false;
    if (clk) { checked = !checked; changed = true; }

    painter().rectFilled(r.x, r.y + 1.0f, 40.0f, 22.0f, 11.0f, checked ? "#10b981" : "#334155");
    float tx = checked ? (r.x + 20.0f) : (r.x + 2.0f);
    painter().circleFilled(tx + 9.0f, r.y + 12.0f, 9.0f, "#ffffff");
    if (!labelText.empty()) painter().text(r.x + 48.0f, r.y + 4.0f, labelText, fs, "#e2e8f0");

    EguiResponse res;
    res.id = wid; res.rect = r; res.hovered = hov; res.clicked = clk; res.changed = changed; res.boolVal = checked;
    return res;
}

inline EguiResponse EguiUi::radioValue(const std::string &id, bool selected, const std::string &labelText) {
    float fs = 14.0f;
    float textW = measureTextWidth(labelText, fs, false);
    UiRect r = allocateSize(26.0f + textW, 22.0f);
    std::string wid = ctx->makeWidgetId(id.empty() ? ("rad_" + labelText) : id);
    ctx->registerHitRegion(wid, r, "radio", "radio", labelText, true, false, zIndex);

    bool hov = r.contains(ctx->input.pointerX, ctx->input.pointerY);
    bool clk = hov && ctx->input.mouseReleased[0];

    painter().circleStroke(r.x + 9.0f, r.y + 11.0f, 9.0f, 1.8f, selected ? "#6366f1" : "#64748b");
    if (selected) painter().circleFilled(r.x + 9.0f, r.y + 11.0f, 4.5f, "#818cf8");
    painter().text(r.x + 25.0f, r.y + 3.0f, labelText, fs, "#e2e8f0");

    EguiResponse res;
    res.id = wid; res.rect = r; res.hovered = hov; res.clicked = clk; res.changed = clk; res.boolVal = selected;
    return res;
}

inline EguiResponse EguiUi::slider(const std::string &id, double &val, double minV, double maxV, const std::string &labelText, double step) {
    float trackW = std::min(availableWidth(), 220.0f);
    float fs = 13.0f;
    std::string valStr = std::to_string(static_cast<long long>(std::round(val)));
    if (step > 0.0 && step < 1.0) {
        std::ostringstream oss; oss.precision(2); oss << std::fixed << val; valStr = oss.str();
    }
    std::string fullLbl = labelText.empty() ? valStr : (labelText + ": " + valStr);
    float lblW = measureTextWidth(fullLbl, fs, false);
    UiRect r = allocateSize(trackW + lblW + 12.0f, 24.0f);
    std::string wid = ctx->makeWidgetId(id.empty() ? ("sld_" + labelText) : id);
    ctx->registerHitRegion(wid, r, "slider", "slider", fullLbl, true, true, zIndex);

    UiRect trackRect = {r.x, r.y + 8.0f, trackW, 8.0f};
    bool hov = r.contains(ctx->input.pointerX, ctx->input.pointerY);
    if (hov && ctx->input.mousePressed[0]) ctx->draggingSlider[wid] = true;
    bool drag = ctx->draggingSlider[wid] && ctx->input.mouseDown[0];
    bool changed = false;

    if (drag && trackW > 0.0f) {
        double ratio = std::clamp(static_cast<double>(ctx->input.pointerX - trackRect.x) / trackW, 0.0, 1.0);
        double next = minV + ratio * (maxV - minV);
        if (step > 0.0) next = minV + std::round((next - minV) / step) * step;
        next = std::clamp(next, minV, maxV);
        if (std::abs(next - val) > 1e-9) { val = next; changed = true; }
    }

    double ratio = (maxV > minV) ? std::clamp((val - minV) / (maxV - minV), 0.0, 1.0) : 0.0;
    painter().rectFilled(trackRect.x, trackRect.y, trackW, 6.0f, 3.0f, "#1e293b");
    painter().rectFilled(trackRect.x, trackRect.y, static_cast<float>(ratio) * trackW, 6.0f, 3.0f, "#6366f1");
    painter().circleFilled(trackRect.x + static_cast<float>(ratio) * trackW, trackRect.y + 3.0f, drag || hov ? 8.5f : 7.0f, "#f8fafc");
    painter().text(trackRect.x + trackW + 12.0f, r.y + 4.0f, fullLbl, fs, "#cbd5e1");

    EguiResponse res;
    res.id = wid; res.rect = r; res.hovered = hov; res.dragged = drag; res.changed = changed; res.numVal = val;
    return res;
}

inline EguiResponse EguiUi::dragValue(const std::string &id, double &val, double minV, double maxV, double speed) {
    UiRect r = allocateSize(76.0f, 24.0f);
    std::string wid = ctx->makeWidgetId(id);
    ctx->registerHitRegion(wid, r, "slider", "spinbutton", wid, true, true, zIndex);

    bool hov = r.contains(ctx->input.pointerX, ctx->input.pointerY);
    if (hov && ctx->input.mousePressed[0]) ctx->draggingSlider[wid] = true;
    bool drag = ctx->draggingSlider[wid] && ctx->input.mouseDown[0];
    bool changed = false;
    if (drag) {
        double delta = (ctx->input.pointerX - ctx->input.prevPointerX) * speed;
        double next = std::clamp(val + delta, minV, maxV);
        if (std::abs(next - val) > 1e-9) { val = next; changed = true; }
    }
    painter().rectFilled(r.x, r.y, r.width, r.height, 6.0f, "#1e293b");
    painter().rectStroke(r.x, r.y, r.width, r.height, 6.0f, 1.0f, hov || drag ? "#6366f1" : "#334155");
    std::ostringstream oss; oss.precision(1); oss << std::fixed << val;
    painter().text(r.x + 10.0f, r.y + 4.0f, oss.str(), 13.0f, "#f8fafc", 500, true);

    EguiResponse res;
    res.id = wid; res.rect = r; res.hovered = hov; res.dragged = drag; res.changed = changed; res.numVal = val;
    return res;
}

inline EguiResponse EguiUi::progressBar(double fraction, const std::string &overlayText) {
    float w = std::max(120.0f, availableWidth());
    UiRect r = allocateSize(w, 18.0f);
    double f = std::clamp(fraction, 0.0, 1.0);
    painter().rectFilled(r.x, r.y, r.width, r.height, 9.0f, "#1e293b");
    painter().rectFilled(r.x, r.y, r.width * static_cast<float>(f), r.height, 9.0f, "#10b981");
    if (!overlayText.empty()) {
        float tw = measureTextWidth(overlayText, 11.0f, false);
        painter().text(r.x + (r.width - tw) * 0.5f, r.y + 2.0f, overlayText, 11.0f, "#ffffff", 600);
    }
    EguiResponse res; res.rect = r; res.numVal = f; return res;
}

inline EguiResponse EguiUi::textEditSingleline(const std::string &id, std::string &text, const std::string &placeholder, bool password, float width) {
    float w = (width > 0.0f) ? width : std::max(140.0f, availableWidth());
    UiRect r = allocateSize(w, 32.0f);
    std::string wid = ctx->makeWidgetId(id.empty() ? "egui_input" : id);
    ctx->registerHitRegion(wid, r, "input", "textbox", placeholder.empty() ? "text input" : placeholder, true, true, zIndex);

    auto &tf = ctx->interaction.getOrCreateTextField(wid, text, false, password, false, false);
    bool hov = r.contains(ctx->input.pointerX, ctx->input.pointerY);
    if (ctx->input.mousePressed[0]) {
        if (hov) {
            ctx->interaction.focusedKey = wid;
            tf.handlePointerDown(ctx->input.pointerX - (r.x + 10.0f), ctx->input.pointerY - (r.y + 8.0f), r.width - 20.0f, 14.0f, false, ctx->input.clickCount);
        } else if (ctx->interaction.focusedKey == wid) {
            ctx->interaction.focusedKey.clear();
        }
    }
    if (ctx->input.mouseDown[0] && tf.dragging) {
        tf.handlePointerDrag(ctx->input.pointerX - (r.x + 10.0f), ctx->input.pointerY - (r.y + 8.0f), r.width - 20.0f, 14.0f, false);
    }
    bool isFocused = (ctx->interaction.focusedKey == wid);
    bool changed = false;
    bool enterSubmit = false;
    if (isFocused) {
        for (const auto &ke : ctx->input.keyEvents) {
            auto kr = tf.handleKeyEvent(ke, r.width - 20.0f, 20.0f, 14.0f, false, ctx->input.clipboardText);
            if (kr.textChanged) { text = tf.text; changed = true; }
            if (kr.submitEnter) enterSubmit = true;
            if (kr.requestClipboardCopy) ctx->input.clipboardText = kr.clipboardSet;
        }
    }

    painter().rectFilled(r.x, r.y, r.width, r.height, 8.0f, "#0f172a");
    painter().rectStroke(r.x, r.y, r.width, r.height, 8.0f, isFocused ? 1.5f : 1.0f, isFocused ? "#6366f1" : "#334155");
    tf.emitDrawCommands(ctx->cmds, r.x + 10.0f, r.y + 8.0f, r.width - 20.0f, 18.0f, 14.0f, 400, hexToRgba("#f8fafc"), placeholder, isFocused, false, 0, zIndex);

    EguiResponse res;
    res.id = wid; res.rect = r; res.hovered = hov; res.hasFocus = isFocused;
    res.changed = changed; res.clicked = enterSubmit; res.strVal = text;
    return res;
}

inline EguiResponse EguiUi::textEditMultiline(const std::string &id, std::string &text, const std::string &placeholder, float width, float height) {
    float w = (width > 0.0f) ? width : std::max(160.0f, availableWidth());
    UiRect r = allocateSize(w, height);
    std::string wid = ctx->makeWidgetId(id.empty() ? "egui_textarea" : id);
    ctx->registerHitRegion(wid, r, "textarea", "textbox", placeholder.empty() ? "multiline input" : placeholder, true, true, zIndex);

    auto &tf = ctx->interaction.getOrCreateTextField(wid, text, true, false, false, false);
    bool hov = r.contains(ctx->input.pointerX, ctx->input.pointerY);
    if (ctx->input.mousePressed[0] && hov) {
        ctx->interaction.focusedKey = wid;
        tf.handlePointerDown(ctx->input.pointerX - (r.x + 10.0f), ctx->input.pointerY - (r.y + 8.0f), r.width - 20.0f, 14.0f, false, ctx->input.clickCount);
    }
    bool isFocused = (ctx->interaction.focusedKey == wid);
    bool changed = false;
    if (isFocused) {
        for (const auto &ke : ctx->input.keyEvents) {
            auto kr = tf.handleKeyEvent(ke, r.width - 20.0f, r.height - 16.0f, 14.0f, false, ctx->input.clipboardText);
            if (kr.textChanged) { text = tf.text; changed = true; }
        }
    }

    painter().rectFilled(r.x, r.y, r.width, r.height, 8.0f, "#0f172a");
    painter().rectStroke(r.x, r.y, r.width, r.height, 8.0f, isFocused ? 1.5f : 1.0f, isFocused ? "#6366f1" : "#334155");
    tf.emitDrawCommands(ctx->cmds, r.x + 10.0f, r.y + 8.0f, r.width - 20.0f, r.height - 16.0f, 14.0f, 400, hexToRgba("#f8fafc"), placeholder, isFocused, false, 0, zIndex);

    EguiResponse res;
    res.id = wid; res.rect = r; res.hovered = hov; res.hasFocus = isFocused; res.changed = changed; res.strVal = text;
    return res;
}

inline EguiResponse EguiUi::comboBox(const std::string &id, std::string &selected, const std::vector<std::string> &options) {
    UiRect r = allocateSize(180.0f, 30.0f);
    std::string wid = ctx->makeWidgetId(id.empty() ? "egui_combo" : id);
    ctx->registerHitRegion(wid, r, "combobox", "combobox", selected, true, true, zIndex);

    bool hov = r.contains(ctx->input.pointerX, ctx->input.pointerY);
    if (hov && ctx->input.mouseReleased[0]) {
        ctx->activeComboId = (ctx->activeComboId == wid) ? "" : wid;
    }

    painter().rectFilled(r.x, r.y, r.width, r.height, 8.0f, "#1e293b");
    painter().rectStroke(r.x, r.y, r.width, r.height, 8.0f, 1.0f, ctx->activeComboId == wid ? "#6366f1" : "#334155");
    painter().text(r.x + 10.0f, r.y + 7.0f, selected, 13.0f, "#f8fafc");
    painter().svgPath(r.x + r.width - 22.0f, r.y + 7.0f, 16.0f, 16.0f, getLucideSvgPath("chevron-down"), "#94a3b8");

    EguiResponse res;
    res.id = wid; res.rect = r; res.hovered = hov; res.strVal = selected;

    if (ctx->activeComboId == wid && !options.empty()) {
        float popH = static_cast<float>(options.size()) * 28.0f + 8.0f;
        PopupPlacementOptions popts;
        popts.side = PopupSide::BOTTOM;
        auto placed = computePopupPlacement(r, r.width, popH, {0, 0, ctx->input.viewportW, ctx->input.viewportH}, popts);

        // Check immediate selection inside popup
        for (size_t i = 0; i < options.size(); ++i) {
            UiRect optR = {placed.x + 4.0f, placed.y + 4.0f + static_cast<float>(i) * 28.0f, r.width - 8.0f, 26.0f};
            if (optR.contains(ctx->input.pointerX, ctx->input.pointerY) && ctx->input.mouseReleased[0]) {
                selected = options[i];
                res.changed = true;
                res.strVal = selected;
                ctx->activeComboId.clear();
                break;
            }
        }

        ctx->deferredPopups.push_back([this, placed, r, options, selected]() {
            UiPainter p{&ctx->cmds, 100};
            p.rectFilled(placed.x, placed.y, r.width, static_cast<float>(options.size()) * 28.0f + 8.0f, 8.0f, "#0f172a");
            p.rectStroke(placed.x, placed.y, r.width, static_cast<float>(options.size()) * 28.0f + 8.0f, 8.0f, 1.0f, "#334155");
            for (size_t i = 0; i < options.size(); ++i) {
                UiRect optR = {placed.x + 4.0f, placed.y + 4.0f + static_cast<float>(i) * 28.0f, r.width - 8.0f, 26.0f};
                bool oh = optR.contains(ctx->input.pointerX, ctx->input.pointerY);
                bool sel = (options[i] == selected);
                if (oh || sel) p.rectFilled(optR.x, optR.y, optR.width, optR.height, 6.0f, sel ? "#4f46e5" : "#1e293b");
                p.text(optR.x + 8.0f, optR.y + 5.0f, options[i], 13.0f, "#f8fafc");
            }
        });
    }
    return res;
}

inline void EguiUi::spinner(float size) {
    UiRect r = allocateSize(size, size);
    float cx = r.x + size * 0.5f, cy = r.y + size * 0.5f;
    float rad = size * 0.42f;
    float a = static_cast<float>(ctx->input.elapsedMs * 0.006);
    painter().circleStroke(cx, cy, rad, 2.0f, "#334155");
    painter().line(cx, cy, cx + std::cos(a) * rad, cy + std::sin(a) * rad, 2.2f, "#6366f1");
}

inline void EguiUi::horizontal(const std::function<void(EguiUi &)> &fn) {
    EguiUi sub(ctx, {cursorX, cursorY, availableWidth(), availableHeight()}, true, zIndex);
    if (fn) fn(sub);
    float h = std::max(24.0f, sub.rowHeight);
    float w = std::max(0.0f, sub.maxRight - cursorX);
    allocateSize(w, h);
}

inline void EguiUi::vertical(const std::function<void(EguiUi &)> &fn) {
    EguiUi sub(ctx, {cursorX, cursorY, availableWidth(), availableHeight()}, false, zIndex);
    if (fn) fn(sub);
    float h = std::max(0.0f, sub.maxBottom - cursorY);
    float w = std::max(0.0f, sub.maxRight - cursorX);
    allocateSize(w, h);
}

inline void EguiUi::group(const std::function<void(EguiUi &)> &fn) {
    float startX = cursorX, startY = cursorY;
    float w = availableWidth();
    size_t bgIdx = ctx->cmds.size();
    painter().rectFilled(startX, startY, w, 40.0f, 10.0f, "#0f172a", 0.85f);
    EguiUi sub(ctx, {startX + 12.0f, startY + 10.0f, std::max(20.0f, w - 24.0f), availableHeight()}, false, zIndex);
    if (fn) fn(sub);
    float totalH = std::max(32.0f, (sub.maxBottom - startY) + 12.0f);
    ctx->cmds[bgIdx].height = totalH;
    painter().rectStroke(startX, startY, w, totalH, 10.0f, 1.0f, "#1e293b");
    allocateSize(w, totalH);
}

inline void EguiUi::collapsing(const std::string &id, const std::string &header, bool defaultOpen, const std::function<void(EguiUi &)> &fn) {
    std::string wid = id.empty() ? ("col_" + header) : id;
    if (ctx->collapsingOpen.find(wid) == ctx->collapsingOpen.end()) {
        ctx->collapsingOpen[wid] = defaultOpen;
    }
    bool &isOpen = ctx->collapsingOpen[wid];
    UiRect hr = allocateSize(availableWidth(), 26.0f);
    bool hov = hr.contains(ctx->input.pointerX, ctx->input.pointerY);
    if (hov && ctx->input.mouseReleased[0]) isOpen = !isOpen;
    if (hov) painter().rectFilled(hr.x, hr.y, hr.width, hr.height, 6.0f, "#1e293b", 0.6f);
    painter().svgPath(hr.x + 4.0f, hr.y + 5.0f, 16.0f, 16.0f, getLucideSvgPath(isOpen ? "chevron-down" : "chevron-right"), "#94a3b8");
    painter().text(hr.x + 24.0f, hr.y + 5.0f, header, 14.0f, "#f8fafc", 600);

    if (isOpen && fn) {
        EguiUi sub(ctx, {hr.x + 18.0f, cursorY, std::max(20.0f, hr.width - 18.0f), availableHeight()}, false, zIndex);
        fn(sub);
        allocateSize(hr.width, std::max(0.0f, sub.maxBottom - cursorY));
    }
}

inline void EguiUi::scrollArea(const std::string &id, float maxH, const std::function<void(EguiUi &)> &fn) {
    float w = availableWidth();
    float h = std::min(maxH, std::max(60.0f, availableHeight()));
    UiRect area = allocateSize(w, h);
    float &offY = ctx->interaction.scrollOffsetsY[id];
    float prevY = offY;
    if (area.contains(ctx->input.pointerX, ctx->input.pointerY) && std::abs(ctx->input.wheelDeltaY) > 0.01f) {
        offY = std::max(0.0f, offY - ctx->input.wheelDeltaY * 24.0f);
    }

    painter().pushClip(area.x, area.y, area.width, area.height);
    EguiUi sub(ctx, {area.x, area.y - offY, std::max(20.0f, area.width - 10.0f), 100000.0f}, false, zIndex);
    if (fn) fn(sub);
    float contentH = std::max(h, sub.maxBottom - (area.y - offY));
    float maxScroll = std::max(0.0f, contentH - h);

    if (maxScroll > 1.0f) {
        float thumbH = std::max(18.0f, h * (h / contentH));
        if (ctx->input.mousePressed[0] &&
            ctx->input.pointerX >= area.x + area.width - 12.0f &&
            ctx->input.pointerX <= area.x + area.width &&
            ctx->input.pointerY >= area.y &&
            ctx->input.pointerY <= area.y + h) {
            ctx->interaction.draggingScrollbar = true;
            ctx->interaction.activeKey = id;
            float thumbY = area.y + (std::clamp(offY, 0.0f, maxScroll) / maxScroll) * (h - thumbH);
            if (ctx->input.pointerY >= thumbY && ctx->input.pointerY <= thumbY + thumbH) {
                ctx->interaction.scrollbarDragOffset = ctx->input.pointerY - thumbY;
            } else {
                ctx->interaction.scrollbarDragOffset = thumbH * 0.5f;
            }
        }
        if (ctx->input.mouseDown[0] && ctx->interaction.draggingScrollbar && ctx->interaction.activeKey == id) {
            float denom = std::max(1.0f, h - thumbH);
            float ratio = std::clamp((ctx->input.pointerY - area.y - ctx->interaction.scrollbarDragOffset) / denom, 0.0f, 1.0f);
            offY = ratio * maxScroll;
        }
    }

    offY = std::clamp(offY, 0.0f, maxScroll);
    if (std::abs(offY - prevY) > 0.01f) ctx->didScroll = true;
    painter().popClip();

    if (maxScroll > 1.0f) {
        float thumbH = std::max(18.0f, h * (h / contentH));
        float thumbY = area.y + (offY / maxScroll) * (h - thumbH);
        painter().rectFilled(area.x + area.width - 5.0f, thumbY, 4.0f, thumbH, 2.0f, "#64748b", 0.7f);
    }

    int sid = ctx->registerHitRegion("egui_sa_" + id, area, "scrollarea", "scrollarea", id, false, false, zIndex);
    if (!ctx->hitRegions.empty() && ctx->hitRegions.back().id == sid) {
        auto &hr = ctx->hitRegions.back();
        hr.hasScroll = true;
        hr.isScrollContainer = true;
        hr.scrollY = offY;
        hr.maxScrollY = maxScroll;
        hr.contentW = area.width;
        hr.contentH = contentH;
    }
}

inline void EguiUi::columns(int numCols, const std::function<void(std::vector<EguiUi> &)> &fn) {
    int n = std::max(1, numCols);
    float gap = spacingX;
    float totalW = availableWidth();
    float colW = (totalW - (n - 1) * gap) / n;
    std::vector<EguiUi> cols;
    cols.reserve(n);
    for (int i = 0; i < n; ++i) {
        cols.emplace_back(ctx, UiRect{cursorX + i * (colW + gap), cursorY, colW, availableHeight()}, false, zIndex);
    }
    if (fn) fn(cols);
    float maxH = 0.0f;
    for (const auto &c : cols) maxH = std::max(maxH, c.maxBottom - cursorY);
    allocateSize(totalW, maxH);
}

} // namespace MemeUI
