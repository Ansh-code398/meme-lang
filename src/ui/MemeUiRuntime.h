#pragma once

#include "MemeUiTypes.h"
#include "MemeUiTextEngine.h"
#include "MemeUiTessellator.h"
#include "MemeUiPrimitives.h"
#include "MemeUiLayout.h"
#include "MemeEgui.h"
#include <functional>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>

namespace MemeUI {

inline std::string escapeJson(const std::string &input) {
    std::string out;
    out.reserve(input.size() + 8);
    for (unsigned char c : input) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (c < 32) {
            char buf[8];
            std::snprintf(buf, sizeof(buf), "\\u%04x", c);
            out += buf;
        } else {
            out += static_cast<char>(c);
        }
    }
    return out;
}

inline std::string gpuDrawCmdsToJson(const std::vector<GpuDrawCmd> &cmds) {
    std::string json = "[\n";
    for (size_t i = 0; i < cmds.size(); ++i) {
        const auto &c = cmds[i];
        json += "  {\"type\": \"" + escapeJson(c.type) + "\", ";
        json += "\"nodeId\": " + std::to_string(c.nodeId) + ", ";
        json += "\"x\": " + std::to_string(c.x) + ", ";
        json += "\"y\": " + std::to_string(c.y) + ", ";
        json += "\"width\": " + std::to_string(c.width) + ", ";
        json += "\"height\": " + std::to_string(c.height) + ", ";
        json += "\"radius\": " + std::to_string(c.radius) + ", ";
        json += "\"lineWidth\": " + std::to_string(c.lineWidth) + ", ";
        json += "\"color\": \"" + escapeJson(c.color) + "\", ";
        json += "\"alpha\": " + std::to_string(c.alpha) + ", ";
        json += "\"fontSize\": " + std::to_string(c.fontSize) + ", ";
        json += "\"fontWeight\": " + std::to_string(c.fontWeight) + ", ";
        json += "\"zIndex\": " + std::to_string(c.zIndex) + ", ";
        json += "\"text\": \"" + escapeJson(c.text) + "\"}";
        json += (i + 1 < cmds.size()) ? ",\n" : "\n";
    }
    json += "]";
    return json;
}

inline std::string hitRegionsToJsonStr(const std::vector<UiHitRegion> &regions) {
    std::string json = "[\n";
    for (size_t i = 0; i < regions.size(); ++i) {
        const auto &r = regions[i];
        json += "  {\"id\": " + std::to_string(r.id) + ", ";
        json += "\"key\": \"" + escapeJson(r.nodeKey) + "\", ";
        json += "\"x\": " + std::to_string(r.x) + ", ";
        json += "\"y\": " + std::to_string(r.y) + ", ";
        json += "\"width\": " + std::to_string(r.width) + ", ";
        json += "\"height\": " + std::to_string(r.height) + ", ";
        json += "\"tag\": \"" + escapeJson(r.tag) + "\", ";
        json += "\"nodeType\": \"" + escapeJson(r.tag) + "\", ";
        json += "\"role\": \"" + escapeJson(r.role) + "\", ";
        json += "\"name\": \"" + escapeJson(r.accessibleName) + "\", ";
        json += "\"accessibleName\": \"" + escapeJson(r.accessibleName) + "\", ";
        json += "\"testId\": \"" + escapeJson(r.testId) + "\", ";
        json += "\"cursor\": \"" + escapeJson(r.cursor) + "\", ";
        json += "\"hasClick\": " + std::string(r.hasClick ? "true" : "false") + ", ";
        json += "\"hasChange\": " + std::string(r.hasChange ? "true" : "false") + ", ";
        json += "\"hasOnClick\": " + std::string(r.hasClick ? "true" : "false") + ", ";
        json += "\"hasOnChange\": " + std::string(r.hasChange ? "true" : "false") + ", ";
        json += "\"focused\": " + std::string(r.focused ? "true" : "false") + ", ";
        json += "\"hovered\": " + std::string(r.hovered ? "true" : "false") + ", ";
        json += "\"disabled\": " + std::string(r.disabled ? "true" : "false") + ", ";
        json += "\"checked\": " + std::string(r.checked ? "true" : "false") + ", ";
        json += "\"indeterminate\": " + std::string(r.indeterminate ? "true" : "false") + ", ";
        json += "\"selected\": " + std::string(r.selected ? "true" : "false") + ", ";
        json += "\"expanded\": " + std::string(r.expanded ? "true" : "false") + ", ";
        json += "\"placeholder\": \"" + escapeJson(r.placeholder) + "\", ";
        json += "\"isPassword\": " + std::string(r.isPassword ? "true" : "false") + ", ";
        json += "\"isMultiline\": " + std::string(r.isMultiline ? "true" : "false") + ", ";
        json += "\"numValue\": " + std::to_string(r.numValue) + ", ";
        json += "\"numMin\": " + std::to_string(r.numMin) + ", ";
        json += "\"numMax\": " + std::to_string(r.numMax) + ", ";
        json += "\"numStep\": " + std::to_string(r.numStep) + ", ";
        json += "\"isScrollContainer\": " + std::string(r.isScrollContainer ? "true" : "false") + ", ";
        json += "\"overscrollContain\": " + std::string(r.overscrollContain ? "true" : "false") + ", ";
        json += "\"scrollX\": " + std::to_string(r.scrollX) + ", ";
        json += "\"maxScrollX\": " + std::to_string(r.maxScrollX) + ", ";
        json += "\"scrollY\": " + std::to_string(r.scrollY) + ", ";
        json += "\"maxScrollY\": " + std::to_string(r.maxScrollY) + ", ";
        json += "\"isRtl\": " + std::string(r.isRtl ? "true" : "false") + ", ";
        json += "\"value\": \"" + escapeJson(r.inputValue) + "\"}";
        json += (i + 1 < regions.size()) ? ",\n" : "\n";
    }
    json += "]";
    return json;
}

class UiSession {
public:
    UiInteractionState interaction;
    PopupStack popups;
    FocusScopeStack focusScopes;
    ToastManager toasts;
    TooltipDelayGroup tooltips;
    EguiContext egui;

    UiInputState input;
    std::vector<GpuDrawCmd> lastCmds;
    std::vector<UiHitRegion> lastHits;
    UiMesh lastMesh;
    std::unordered_map<int, UiNode> nodeById;
    std::function<UiNode()> buildRootFn = nullptr;
    std::function<void(EguiContext &)> eguiUpdateFn = nullptr;
    std::function<void(const std::string &)> clipboardWriteFn = nullptr;
    std::function<std::string()> clipboardReadFn = nullptr;

    UiNode cachedRoot;
    bool isImmediateMode = false;
    bool needsRender = true;
    int version = 1;

    void indexNodes(const UiNode &node, const std::string &keyPrefix, int idx, int &idCounter, std::unordered_map<int, const UiNode *> &outMap) {
        if (node.isText) return;
        std::string nKey = resolveNodeKey(node, keyPrefix, idx);
        bool hasClick = (node.tag == "button" || node.tag == "a" || node.tag == "checkbox" || node.tag == "switch" ||
                         node.tag == "radio" || node.tag == "tab" || node.tag == "select" || node.tag == "combobox" ||
                         node.handlers.count("click") > 0);
        bool hasChange = (node.tag == "input" || node.tag == "textarea" || node.tag == "select" || node.tag == "combobox" ||
                          node.tag == "slider" || node.tag == "checkbox" || node.tag == "switch" || node.tag == "radio" ||
                          node.handlers.count("change") > 0 || node.handlers.count("input") > 0);
        bool isScroll = (node.tag == "scrollarea" ||
                         node.classStr.find("overflow-auto") != std::string::npos ||
                         node.classStr.find("overflow-scroll") != std::string::npos ||
                         node.classStr.find("overflow-y-auto") != std::string::npos ||
                         node.classStr.find("overflow-y-scroll") != std::string::npos ||
                         node.classStr.find("overflow-x-auto") != std::string::npos ||
                         node.classStr.find("overflow-x-scroll") != std::string::npos ||
                         (nKey == "root" && node.classStr.find("overflow-hidden") == std::string::npos));
        if (hasClick || hasChange || isScroll || node.handlers.count("wheel") > 0 || node.handlers.count("scroll") > 0 || node.handlers.count("keydown") > 0 || node.handlers.count("pointerdown") > 0 || !node.testId.empty()) {
            int id = ++idCounter;
            outMap[id] = &node;
        }
        for (size_t i = 0; i < node.children.size(); ++i) {
            indexNodes(node.children[i], nKey, static_cast<int>(i), idCounter, outMap);
        }
    }

    const UiHitRegion *hitTestTopmost(float px, float py) const {
        const UiHitRegion *best = nullptr;
        auto priorityOf = [](const UiHitRegion &hr) -> int {
            if (hr.hasClick || hr.hasChange || hr.hasPointerDown || hr.hasKeyDown || hr.focusable) return 3;
            if (hr.tag == "backdrop") return 2;
            if (hr.isScrollContainer) return 1;
            return 0;
        };
        for (const auto &hr : lastHits) {
            if (!hr.disabled && hr.clipRect.contains(px, py) &&
                px >= hr.x && px <= hr.x + hr.width &&
                py >= hr.y && py <= hr.y + hr.height) {
                if (!best || hr.zIndex > best->zIndex ||
                    (hr.zIndex == best->zIndex && priorityOf(hr) >= priorityOf(*best))) {
                    best = &hr;
                }
            }
        }
        return best;
    }

    const UiHitRegion *findRegionById(int id) const {
        for (const auto &hr : lastHits) {
            if (hr.id == id) return &hr;
        }
        return nullptr;
    }

    const UiHitRegion *findRegionByKey(const std::string &key) const {
        for (const auto &hr : lastHits) {
            if (hr.nodeKey == key) return &hr;
        }
        return nullptr;
    }

    void renderFrame(float vw, float vh) {
        input.viewportW = vw;
        input.viewportH = vh;

        if (isImmediateMode && eguiUpdateFn) {
            egui.interaction.scrollOffsetsY = interaction.scrollOffsetsY;
            egui.interaction.scrollOffsetsX = interaction.scrollOffsetsX;
            egui.beginFrame(input);
            eguiUpdateFn(egui);
            egui.endFrame();
            interaction.scrollOffsetsY = egui.interaction.scrollOffsetsY;
            interaction.scrollOffsetsX = egui.interaction.scrollOffsetsX;
            lastCmds = egui.cmds;
            lastHits = egui.hitRegions;
            lastMesh = Tessellator::tessellate(lastCmds, vw, vh);
            needsRender = false;
            return;
        }

        if (buildRootFn) {
            cachedRoot = buildRootFn();
        }
        lastHits.clear();
        lastCmds = computeUiLayout(cachedRoot, vw, vh, &lastHits, &interaction, &input);
        bool clampedScroll = false;
        for (const auto &hr : lastHits) {
            if (!hr.isScrollContainer || hr.nodeKey.empty()) continue;
            auto ity = interaction.scrollOffsetsY.find(hr.nodeKey);
            if (ity != interaction.scrollOffsetsY.end()) {
                float c = std::clamp(ity->second, 0.0f, std::max(0.0f, hr.maxScrollY));
                if (std::abs(c - ity->second) > 0.5f) {
                    ity->second = c;
                    clampedScroll = true;
                }
            }
            auto itx = interaction.scrollOffsetsX.find(hr.nodeKey);
            if (itx != interaction.scrollOffsetsX.end()) {
                float c = std::clamp(itx->second, 0.0f, std::max(0.0f, hr.maxScrollX));
                if (std::abs(c - itx->second) > 0.5f) {
                    itx->second = c;
                    clampedScroll = true;
                }
            }
        }
        if (clampedScroll) {
            lastHits.clear();
            lastCmds = computeUiLayout(cachedRoot, vw, vh, &lastHits, &interaction, &input);
        }
        lastMesh = Tessellator::tessellate(lastCmds, vw, vh);
        needsRender = false;
    }

    bool tick(double deltaMs) {
        input.deltaMs = deltaMs;
        input.elapsedMs += deltaMs;
        bool changed = false;

        if (!interaction.focusedKey.empty()) {
            auto it = interaction.textFields.find(interaction.focusedKey);
            if (it != interaction.textFields.end()) {
                bool before = it->second.caretVisible;
                it->second.tickBlink(deltaMs);
                if (it->second.caretVisible != before) changed = true;
            }
        }
        if (toasts.tick(deltaMs)) changed = true;
        if (tooltips.tick(deltaMs)) changed = true;
        if (interaction.animStore.tickAll(deltaMs)) changed = true;
        if (changed) {
            needsRender = true;
            version++;
        }
        return changed;
    }

    bool handlePointerMove(float x, float y) {
        input.prevPointerX = input.pointerX;
        input.prevPointerY = input.pointerY;
        input.pointerX = x;
        input.pointerY = y;

        if (isImmediateMode) {
            needsRender = true;
            return true;
        }

        const UiHitRegion *hr = hitTestTopmost(x, y);
        std::string nextHovKey = hr ? hr->nodeKey : "";
        int nextHovId = hr ? hr->id : 0;
        bool changed = (nextHovKey != interaction.hoveredKey);

        if (interaction.hoveredKey != nextHovKey) {
            if (!interaction.hoveredKey.empty()) tooltips.onTriggerHoverEnd(interaction.hoveredKey);
            if (!nextHovKey.empty()) tooltips.onTriggerHoverStart(nextHovKey);
            interaction.hoveredKey = nextHovKey;
            interaction.hoveredId = nextHovId;
        }

        // Active text field drag selection
        if (input.mouseDown[0] && !interaction.focusedKey.empty()) {
            auto tfIt = interaction.textFields.find(interaction.focusedKey);
            const UiHitRegion *fhr = findRegionByKey(interaction.focusedKey);
            if (tfIt != interaction.textFields.end() && tfIt->second.dragging && fhr) {
                tfIt->second.handlePointerDrag(x - (fhr->x + 10.0f), y - (fhr->y + 8.0f), std::max(40.0f, fhr->width - 20.0f), 14.0f, false);
                changed = true;
            }
        }

        // Active slider drag
        if (input.mouseDown[0] && interaction.draggingSlider && !interaction.activeKey.empty()) {
            const UiHitRegion *shr = findRegionByKey(interaction.activeKey);
            if (shr && shr->width > 0) {
                double ratio = std::clamp(static_cast<double>(x - shr->x) / shr->width, 0.0, 1.0);
                if (shr->isRtl) ratio = 1.0 - ratio;
                double nextV = shr->numMin + ratio * (shr->numMax - shr->numMin);
                if (shr->numStep > 0) nextV = shr->numMin + std::round((nextV - shr->numMin) / shr->numStep) * shr->numStep;
                nextV = std::clamp(nextV, shr->numMin, shr->numMax);
                dispatchEventOnNode(shr->id, "change", std::to_string(nextV), nextV);
                changed = true;
            }
        }

        // Active scrollbar thumb drag
        if (input.mouseDown[0] && interaction.draggingScrollbar && !interaction.activeKey.empty()) {
            const UiHitRegion *shr = findRegionByKey(interaction.activeKey);
            if (shr) {
                if (interaction.draggingScrollbarHorizontal && shr->maxScrollX > 0.5f && shr->width > 16.0f) {
                    float thumbW = std::max(20.0f, shr->width * (shr->width / std::max(1.0f, shr->contentW)));
                    float denom = std::max(1.0f, shr->width - thumbW);
                    float ratio = std::clamp((x - shr->x - interaction.scrollbarDragOffset) / denom, 0.0f, 1.0f);
                    interaction.scrollOffsetsX[shr->nodeKey] = ratio * shr->maxScrollX;
                    dispatchEventOnNode(shr->id, "scroll", "", static_cast<double>(interaction.scrollOffsetsX[shr->nodeKey]), x, y, 0.0f, 0.0f);
                    changed = true;
                } else if (!interaction.draggingScrollbarHorizontal && shr->maxScrollY > 0.5f && shr->height > 16.0f) {
                    float thumbH = std::max(20.0f, shr->height * (shr->height / std::max(1.0f, shr->contentH)));
                    float denom = std::max(1.0f, shr->height - thumbH);
                    float ratio = std::clamp((y - shr->y - interaction.scrollbarDragOffset) / denom, 0.0f, 1.0f);
                    interaction.scrollOffsetsY[shr->nodeKey] = ratio * shr->maxScrollY;
                    dispatchEventOnNode(shr->id, "scroll", "", static_cast<double>(interaction.scrollOffsetsY[shr->nodeKey]), x, y, 0.0f, 0.0f);
                    changed = true;
                }
            }
        }

        if (changed) {
            needsRender = true;
            version++;
        }
        return changed;
    }

    bool handlePointerDown(float x, float y, int button = 0, int clickCount = 1) {
        input.pointerX = x;
        input.pointerY = y;
        if (button >= 0 && button < 3) {
            input.mouseDown[button] = true;
            input.mousePressed[button] = true;
        }
        input.clickCount = clickCount;

        if (isImmediateMode) {
            renderFrame(input.viewportW, input.viewportH);
            input.mousePressed[button] = false;
            return true;
        }

        std::string restoreFocus;
        if (popups.handlePointerDown(x, y, restoreFocus)) {
            if (!restoreFocus.empty()) interaction.focusedKey = restoreFocus;
            needsRender = true;
            version++;
        }

        const UiHitRegion *hr = hitTestTopmost(x, y);

        // Check vertical / horizontal scrollbar track hits on overlapping scroll containers
        if (button == 0) {
            const UiHitRegion *sbHit = nullptr;
            bool sbHoriz = false;
            int modalBlockZ = -999999;
            for (const auto &r : lastHits) {
                if (r.tag == "backdrop" && r.clipRect.contains(x, y) &&
                    x >= r.x && x <= r.x + r.width && y >= r.y && y <= r.y + r.height) {
                    modalBlockZ = std::max(modalBlockZ, r.zIndex);
                }
            }
            for (auto it = lastHits.rbegin(); it != lastHits.rend(); ++it) {
                const auto &r = *it;
                if (!r.isScrollContainer || r.disabled || r.zIndex < modalBlockZ) continue;
                bool inside = (r.nodeKey == "root")
                    ? (x >= 0.0f && x <= input.viewportW && y >= 0.0f && y <= input.viewportH)
                    : (r.clipRect.contains(x, y) && x >= r.x && x <= r.x + r.width && y >= r.y && y <= r.y + r.height);
                if (!inside) continue;
                if (r.maxScrollY > 2.0f && r.height > 16.0f && x >= r.x + r.width - 12.0f && x <= r.x + r.width) {
                    if (!hr || !(hr->hasClick || hr->hasChange) || hr->zIndex <= r.zIndex) {
                        sbHit = &r; sbHoriz = false; break;
                    }
                }
                if (r.maxScrollX > 2.0f && r.width > 16.0f && y >= r.y + r.height - 12.0f && y <= r.y + r.height) {
                    if (!hr || !(hr->hasClick || hr->hasChange) || hr->zIndex <= r.zIndex) {
                        sbHit = &r; sbHoriz = true; break;
                    }
                }
            }
            if (sbHit) {
                interaction.activeId = sbHit->id;
                interaction.activeKey = sbHit->nodeKey;
                interaction.draggingScrollbar = true;
                interaction.draggingScrollbarHorizontal = sbHoriz;
                if (sbHoriz) {
                    float thumbW = std::max(20.0f, sbHit->width * (sbHit->width / std::max(1.0f, sbHit->contentW)));
                    float thumbX = sbHit->x + (sbHit->scrollX / sbHit->maxScrollX) * (sbHit->width - thumbW);
                    if (x >= thumbX && x <= thumbX + thumbW) {
                        interaction.scrollbarDragOffset = x - thumbX;
                    } else {
                        interaction.scrollbarDragOffset = thumbW * 0.5f;
                        float denom = std::max(1.0f, sbHit->width - thumbW);
                        float ratio = std::clamp((x - sbHit->x - interaction.scrollbarDragOffset) / denom, 0.0f, 1.0f);
                        interaction.scrollOffsetsX[sbHit->nodeKey] = ratio * sbHit->maxScrollX;
                    }
                } else {
                    float thumbH = std::max(20.0f, sbHit->height * (sbHit->height / std::max(1.0f, sbHit->contentH)));
                    float thumbY = sbHit->y + (sbHit->scrollY / sbHit->maxScrollY) * (sbHit->height - thumbH);
                    if (y >= thumbY && y <= thumbY + thumbH) {
                        interaction.scrollbarDragOffset = y - thumbY;
                    } else {
                        interaction.scrollbarDragOffset = thumbH * 0.5f;
                        float denom = std::max(1.0f, sbHit->height - thumbH);
                        float ratio = std::clamp((y - sbHit->y - interaction.scrollbarDragOffset) / denom, 0.0f, 1.0f);
                        interaction.scrollOffsetsY[sbHit->nodeKey] = ratio * sbHit->maxScrollY;
                    }
                }
                needsRender = true;
                version++;
                return true;
            }
        }

        if (!hr) {
            interaction.focusedKey.clear();
            interaction.focusedId = 0;
            needsRender = true;
            return false;
        }

        interaction.activeId = hr->id;
        interaction.activeKey = hr->nodeKey;
        interaction.focusVisible = false;

        if (hr->focusable) {
            interaction.focusedId = hr->id;
            interaction.focusedKey = hr->nodeKey;
        }

        if (hr->tag == "input" || hr->tag == "textarea") {
            auto &tf = interaction.getOrCreateTextField(hr->nodeKey, hr->inputValue, hr->isMultiline, hr->isPassword, hr->readOnly, hr->disabled);
            tf.handlePointerDown(x - (hr->x + 10.0f), y - (hr->y + 8.0f), std::max(40.0f, hr->width - 20.0f), 14.0f, false, clickCount);
            needsRender = true;
        } else if (hr->tag == "slider") {
            interaction.draggingSlider = true;
            double ratio = (hr->width > 0) ? std::clamp(static_cast<double>(x - hr->x) / hr->width, 0.0, 1.0) : 0.0;
            if (hr->isRtl) ratio = 1.0 - ratio;
            double nextV = hr->numMin + ratio * (hr->numMax - hr->numMin);
            if (hr->numStep > 0) nextV = hr->numMin + std::round((nextV - hr->numMin) / hr->numStep) * hr->numStep;
            nextV = std::clamp(nextV, hr->numMin, hr->numMax);
            dispatchEventOnNode(hr->id, "change", std::to_string(nextV), nextV);
        }

        needsRender = true;
        return true;
    }

    bool handlePointerUp(float x, float y, int button = 0) {
        input.pointerX = x;
        input.pointerY = y;
        if (button >= 0 && button < 3) {
            input.mouseDown[button] = false;
            input.mouseReleased[button] = true;
        }
        bool wasScrollbarDrag = interaction.draggingScrollbar;
        interaction.draggingSlider = false;
        interaction.draggingScrollbar = false;
        interaction.draggingScrollbarHorizontal = false;
        for (auto &kv : interaction.textFields) kv.second.handlePointerUp();

        if (isImmediateMode) {
            renderFrame(input.viewportW, input.viewportH);
            input.mouseReleased[button] = false;
            return true;
        }

        const UiHitRegion *hr = hitTestTopmost(x, y);
        bool dispatched = false;
        if (!wasScrollbarDrag && hr && hr->nodeKey == interaction.activeKey && (hr->hasClick || hr->tag == "checkbox" || hr->tag == "switch" || hr->tag == "radio")) {
            std::string extraVal;
            if (!interaction.focusedKey.empty()) {
                auto it = interaction.textFields.find(interaction.focusedKey);
                if (it != interaction.textFields.end()) extraVal = it->second.text;
            }
            dispatched = dispatchEventOnNode(hr->id, "click", extraVal);
        }
        interaction.activeId = 0;
        interaction.activeKey.clear();
        needsRender = true;
        return dispatched;
    }

    bool handleScroll(float x, float y, float dx, float dy) {
        input.pointerX = x;
        input.pointerY = y;
        input.wheelDeltaX = dx;
        input.wheelDeltaY = dy;
        if (isImmediateMode) {
            renderFrame(input.viewportW, input.viewportH);
            input.wheelDeltaX = 0; input.wheelDeltaY = 0;
            return egui.didScroll;
        }
        if (interaction.draggingSlider || interaction.draggingScrollbar) return false;

        int modalBlockZ = -999999;
        for (const auto &hr : lastHits) {
            if (hr.tag == "backdrop" && hr.clipRect.contains(x, y) &&
                x >= hr.x && x <= hr.x + hr.width && y >= hr.y && y <= hr.y + hr.height) {
                modalBlockZ = std::max(modalBlockZ, hr.zIndex);
            }
        }

        std::vector<const UiHitRegion *> candidates;
        for (const auto &hr : lastHits) {
            if (hr.disabled || hr.zIndex < modalBlockZ) continue;
            if (!hr.isScrollContainer && !hr.hasWheelHandler) continue;
            bool inside = (hr.nodeKey == "root")
                ? (x >= 0.0f && x <= input.viewportW && y >= 0.0f && y <= input.viewportH)
                : (hr.clipRect.contains(x, y) &&
                   x >= hr.x && x <= hr.x + hr.width && y >= hr.y && y <= hr.y + hr.height);
            if (!inside) continue;
            if (hr.hasWheelHandler || hr.overscrollContain || hr.maxScrollY > 0.5f || hr.maxScrollX > 0.5f) {
                candidates.push_back(&hr);
            }
        }
        std::stable_sort(candidates.begin(), candidates.end(), [](const UiHitRegion *a, const UiHitRegion *b) {
            return a->zIndex < b->zIndex;
        });
        for (auto it = candidates.rbegin(); it != candidates.rend(); ++it) {
            const UiHitRegion *hr = *it;
            if (hr->hasWheelHandler) {
                if (dispatchEventOnNode(hr->id, "wheel", "", 0.0, x, y, dx, dy)) {
                    needsRender = true;
                    version++;
                    if (!hr->isScrollContainer || hr->overscrollContain) return true;
                }
            }
            bool moved = false;
            if (hr->isScrollContainer && hr->maxScrollY > 0.5f && std::abs(dy) > 1e-5f) {
                float &sy = interaction.scrollOffsetsY[hr->nodeKey];
                float nextY = std::clamp(sy - dy * 28.0f, 0.0f, hr->maxScrollY);
                if (std::abs(nextY - sy) > 1e-4f) {
                    sy = nextY;
                    moved = true;
                }
            }
            if (hr->isScrollContainer && hr->maxScrollX > 0.5f && std::abs(dx) > 1e-5f) {
                float &sx = interaction.scrollOffsetsX[hr->nodeKey];
                float nextX = std::clamp(sx - dx * 28.0f, 0.0f, hr->maxScrollX);
                if (std::abs(nextX - sx) > 1e-4f) {
                    sx = nextX;
                    moved = true;
                }
            }
            if (moved) {
                dispatchEventOnNode(hr->id, "scroll", "",
                    static_cast<double>(interaction.scrollOffsetsY[hr->nodeKey]),
                    x, y, dx, dy);
                needsRender = true;
                version++;
                return true;
            }
            bool horizDom = std::abs(dx) > std::abs(dy);
            bool blocksChain = hr->overscrollContain && (
                (horizDom && hr->maxScrollX > 0.5f) ||
                (!horizDom && (hr->maxScrollY > 0.5f || hr->hasWheelHandler)));
            if (blocksChain) {
                break;
            }
        }
        return false;
    }

    bool handleKeyEvent(const UiKeyEvent &ev) {
        if (isImmediateMode) {
            input.keyEvents.push_back(ev);
            renderFrame(input.viewportW, input.viewportH);
            input.keyEvents.clear();
            return true;
        }

        if (ev.key == "Escape") {
            if (tooltips.handleEscape()) {
                needsRender = true; version++;
                return true;
            }
            std::string restoreFocus;
            if (popups.handleEscape(restoreFocus)) {
                if (!restoreFocus.empty()) interaction.focusedKey = restoreFocus;
                needsRender = true; version++;
                return true;
            }
        }

        if (ev.key == "Tab") {
            std::string nextTrapId;
            if (focusScopes.handleTab(ev.shift, interaction.focusedKey, nextTrapId)) {
                interaction.focusedKey = nextTrapId;
                interaction.focusVisible = true;
                needsRender = true; version++;
                return true;
            }
            // Linear focus traversal across visible focusable HitRegions
            std::vector<const UiHitRegion *> focusables;
            for (const auto &hr : lastHits) {
                if (hr.focusable && !hr.disabled && hr.tabIndex >= 0) focusables.push_back(&hr);
            }
            if (!focusables.empty()) {
                int cur = -1;
                for (int i = 0; i < static_cast<int>(focusables.size()); ++i) {
                    if (focusables[i]->nodeKey == interaction.focusedKey) { cur = i; break; }
                }
                int nextIdx = (cur < 0)
                    ? (ev.shift ? static_cast<int>(focusables.size()) - 1 : 0)
                    : (ev.shift ? (cur - 1 + static_cast<int>(focusables.size())) % static_cast<int>(focusables.size())
                                : (cur + 1) % static_cast<int>(focusables.size()));
                interaction.focusedKey = focusables[nextIdx]->nodeKey;
                interaction.focusedId = focusables[nextIdx]->id;
                interaction.focusVisible = true;
                needsRender = true; version++;
                return true;
            }
        }

        const UiHitRegion *fhr = findRegionByKey(interaction.focusedKey);
        if (!fhr) return false;

        if (fhr->tag == "input" || fhr->tag == "textarea") {
            if (clipboardReadFn && (ev.ctrl || ev.meta) && (ev.key == "v" || ev.key == "V")) {
                input.clipboardText = clipboardReadFn();
            }
            auto &tf = interaction.getOrCreateTextField(fhr->nodeKey, fhr->inputValue, fhr->isMultiline, fhr->isPassword, fhr->readOnly, fhr->disabled);
            auto kr = tf.handleKeyEvent(ev, std::max(40.0f, fhr->width - 20.0f), std::max(20.0f, fhr->height - 16.0f), 14.0f, false, input.clipboardText);
            if (kr.requestClipboardCopy) {
                input.clipboardText = kr.clipboardSet;
                if (clipboardWriteFn) clipboardWriteFn(kr.clipboardSet);
            }
            if (kr.textChanged) {
                dispatchEventOnNode(fhr->id, "change", tf.text);
            }
            if (kr.submitEnter) {
                dispatchEventOnNode(fhr->id, "enter", tf.text);
            }
            needsRender = true;
            return kr.handled;
        }

        if (fhr->tag == "slider") {
            SliderState ss;
            ss.values = {fhr->numValue};
            ss.min = fhr->numMin;
            ss.max = fhr->numMax;
            ss.step = fhr->numStep;
            ss.rtl = fhr->isRtl;
            if (ss.handleKey(0, ev.key, ev.shift)) {
                dispatchEventOnNode(fhr->id, "change", std::to_string(ss.values[0]), ss.values[0]);
                return true;
            }
        }

        if ((fhr->tag == "button" || fhr->tag == "checkbox" || fhr->tag == "switch" || fhr->tag == "radio") &&
            (ev.key == "Enter" || ev.key == " " || ev.key == "Space")) {
            return dispatchEventOnNode(fhr->id, "click", "");
        }

        return dispatchEventOnNode(fhr->id, "keydown", ev.key);
    }

    const UiNode *findNodeByKey(const std::string &targetKey, const UiNode &node, const std::string &pathPrefix = "root", int childIdx = 0, bool isRoot = true) const {
        if (targetKey.empty()) return nullptr;
        std::string nKey = isRoot ? pathPrefix : resolveNodeKey(node, pathPrefix, childIdx);
        if (nKey == targetKey) return &node;
        for (size_t i = 0; i < node.children.size(); ++i) {
            if (const UiNode *res = findNodeByKey(targetKey, node.children[i], nKey, static_cast<int>(i), false)) {
                return res;
            }
        }
        return nullptr;
    }

    const UiNode *findNodeForHitId(int targetId, const UiNode &root) const {
        if (const UiHitRegion *hr = findRegionById(targetId)) {
            if (!hr->nodeKey.empty()) {
                if (const UiNode *byKey = findNodeByKey(hr->nodeKey, root, "root", 0)) {
                    return byKey;
                }
            }
        }
        // Fallback pre-order walk matching computeUiLayout ID counter
        struct Walker {
            int counter = 0;
            const UiNode *found = nullptr;
            void walk(const UiNode &n, int target, bool isRoot = true) {
                if (found || n.isText) return;
                bool hasClick = (n.tag == "button" || n.tag == "a" || n.tag == "checkbox" || n.tag == "switch" ||
                                 n.tag == "radio" || n.tag == "tab" || n.tag == "select" || n.tag == "combobox" ||
                                 n.handlers.count("click") > 0);
                bool hasChange = (n.tag == "input" || n.tag == "textarea" || n.tag == "select" || n.tag == "combobox" ||
                                  n.tag == "slider" || n.tag == "checkbox" || n.tag == "switch" || n.tag == "radio" ||
                                  n.handlers.count("change") > 0 || n.handlers.count("input") > 0);
                bool isScroll = (n.tag == "scrollarea" ||
                                 n.classStr.find("overflow-auto") != std::string::npos ||
                                 n.classStr.find("overflow-scroll") != std::string::npos ||
                                 n.classStr.find("overflow-y-auto") != std::string::npos ||
                                 n.classStr.find("overflow-y-scroll") != std::string::npos ||
                                 n.classStr.find("overflow-x-auto") != std::string::npos ||
                                 n.classStr.find("overflow-x-scroll") != std::string::npos ||
                                 (isRoot && n.classStr.find("overflow-hidden") == std::string::npos));
                if (hasClick || hasChange || isScroll || n.handlers.count("wheel") > 0 || n.handlers.count("scroll") > 0 || n.handlers.count("keydown") > 0 || n.handlers.count("pointerdown") > 0 || !n.testId.empty()) {
                    ++counter;
                    if (counter == target) { found = &n; return; }
                }
                for (const auto &c : n.children) walk(c, target, false);
            }
        } w;
        w.walk(root, targetId);
        return w.found;
    }

    bool dispatchEventOnNode(int nodeId, const std::string &eventType, const std::string &val = "", double numVal = 0.0, float evX = 0.0f, float evY = 0.0f, float evDx = 0.0f, float evDy = 0.0f) {
        const UiNode *n = findNodeForHitId(nodeId, cachedRoot);
        if (!n) return false;

        UiEvent ev;
        ev.type = eventType;
        ev.targetId = nodeId;
        ev.value = val;
        ev.numValue = numVal;
        ev.x = evX;
        ev.y = evY;
        ev.deltaX = evDx;
        ev.deltaY = evDy;
        ev.boolValue = (n->tag == "checkbox" || n->tag == "switch") ? !n->checked : n->checked;

        if (eventType == "click") {
            auto it = n->handlers.find("click");
            if (it != n->handlers.end() && it->second) {
                auto cb = it->second;
                cb(ev);
                version++;
                needsRender = true;
                return true;
            }
            if (n->tag == "checkbox" || n->tag == "switch" || n->tag == "radio") {
                auto cit = n->handlers.find("change");
                if (cit != n->handlers.end() && cit->second) {
                    auto cb = cit->second;
                    cb(ev);
                    version++;
                    needsRender = true;
                    return true;
                }
            }
        } else if (eventType == "change" || eventType == "input") {
            auto it = n->handlers.find("change");
            if (it == n->handlers.end()) it = n->handlers.find("input");
            if (it != n->handlers.end() && it->second) {
                auto cb = it->second;
                cb(ev);
                version++;
                needsRender = true;
                return true;
            }
        } else if (eventType == "enter") {
            if (!val.empty()) dispatchEventOnNode(nodeId, "change", val);
            // Submit to next button or onSubmit handler
            auto sit = n->handlers.find("submit");
            if (sit != n->handlers.end() && sit->second) {
                auto cb = sit->second;
                cb(ev);
                version++;
                needsRender = true;
                return true;
            }
            for (const auto &hr : lastHits) {
                if (hr.id > nodeId && hr.hasClick && hr.tag == "button") {
                    return dispatchEventOnNode(hr.id, "click", val);
                }
            }
        } else {
            auto it = n->handlers.find(eventType);
            if (it != n->handlers.end() && it->second) {
                auto cb = it->second;
                cb(ev);
                version++;
                needsRender = true;
                return true;
            }
        }
        return false;
    }

    // Agent Bridge APIs
    std::string agentSnapshotJson() const {
        std::string json = "{\n";
        json += "  \"version\": " + std::to_string(version) + ",\n";
        json += "  \"viewport\": {\"width\": " + std::to_string(input.viewportW) + ", \"height\": " + std::to_string(input.viewportH) + "},\n";
        json += "  \"focusedKey\": \"" + escapeJson(interaction.focusedKey) + "\",\n";
        json += "  \"openPopups\": " + std::to_string(popups.stack.size()) + ",\n";
        json += "  \"nodes\": " + hitRegionsToJsonStr(lastHits) + "\n";
        json += "}";
        return json;
    }

    std::vector<UiHitRegion> agentQuery(const std::string &role = "", const std::string &nameContains = "", const std::string &testId = "") const {
        std::vector<UiHitRegion> matches;
        for (const auto &hr : lastHits) {
            if (!role.empty() && hr.role != role && hr.tag != role) continue;
            if (!testId.empty() && hr.testId != testId) continue;
            if (!nameContains.empty() && hr.accessibleName.find(nameContains) == std::string::npos && hr.inputValue.find(nameContains) == std::string::npos) continue;
            matches.push_back(hr);
        }
        return matches;
    }

    AgentAuditResult agentAssertAccessible() const {
        return auditHitRegionsAccessibility(lastHits, popups, focusScopes, interaction.focusedKey, input.viewportW, input.viewportH);
    }
};

} // namespace MemeUI
