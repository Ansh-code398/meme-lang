#ifndef TAILWIND_ENGINE_H
#define TAILWIND_ENGINE_H

#include "../interpreter/Value.h"
#include "MemeUiTypes.h"
#include "MemeUiLayout.h"
#include "MemeUiTessellator.h"
#include "MemeUiRuntime.h"
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

using ColorRgba = MemeUI::ColorRgba;

inline ColorRgba hexToRgba(const std::string &hexStr, float alpha = 1.0f) {
    return MemeUI::hexToRgba(hexStr, alpha);
}

inline bool resolveColorToken(const std::string &raw, ColorRgba &out) {
    return MemeUI::resolveColorToken(raw, out);
}

using DisplayMode = MemeUI::DisplayMode;
using FlexDir = MemeUI::FlexDir;
using AlignItems = MemeUI::AlignItems;
using JustifyContent = MemeUI::JustifyContent;
using ComputedStyle = MemeUI::ComputedStyle;

inline void applyTailwindClass(const std::string &cls, ComputedStyle &style, float viewportW = 1000.0f) {
    MemeUI::StyleRuleFilter sf;
    sf.viewportW = viewportW;
    // Preserve legacy single-class application semantics
    if (cls.rfind("sm:", 0) == 0) { if (viewportW < 640.0f) return; MemeUI::applySingleTailwindToken(cls.substr(3), style); return; }
    if (cls.rfind("md:", 0) == 0) { if (viewportW < 768.0f) return; MemeUI::applySingleTailwindToken(cls.substr(3), style); return; }
    if (cls.rfind("lg:", 0) == 0) { if (viewportW < 1024.0f) return; MemeUI::applySingleTailwindToken(cls.substr(3), style); return; }
    if (cls.rfind("xl:", 0) == 0) { if (viewportW < 1280.0f) return; MemeUI::applySingleTailwindToken(cls.substr(3), style); return; }
    if (cls.rfind("hover:", 0) == 0 || cls.rfind("active:", 0) == 0 || cls.rfind("focus:", 0) == 0 || cls.rfind("dark:", 0) == 0) return;
    MemeUI::applySingleTailwindToken(cls, style);
}

inline ComputedStyle parseTailwindStyle(const std::string &twClasses, float viewportW = 1000.0f) {
    MemeUI::StyleRuleFilter sf;
    sf.viewportW = viewportW;
    return MemeUI::parseTailwindStyleWithStates(twClasses, sf);
}

using GpuDrawCmd = MemeUI::GpuDrawCmd;

struct HitRegion {
    int id = 0;
    std::string nodeKey = "";
    float x = 0;
    float y = 0;
    float width = 0;
    float height = 0;
    MemeUI::UiRect clipRect = {0, 0, 1e6f, 1e6f};
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
    bool focusable = false;
    int tabIndex = 0;
    bool hasClick = false;
    bool hasChange = false;
    bool hasKeyDown = false;
    bool hasScroll = false;
    bool isScrollContainer = false;
    float scrollY = 0;
    float maxScrollY = 0;
    Value onClick = Value(std::monostate{});
    Value onChange = Value(std::monostate{});
    Value onKeyDown = Value(std::monostate{});
    Value onPaint = Value(std::monostate{});
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
};

struct ValueCallbackRecord {
    Value onClick = Value(std::monostate{});
    Value onChange = Value(std::monostate{});
    Value onKeyDown = Value(std::monostate{});
    Value onPaint = Value(std::monostate{});
};

inline MemeUI::UiNode valueToUiNode(
    const Value &nodeVal,
    std::vector<ValueCallbackRecord> *outCallbacks = nullptr,
    std::function<void(const Value &paintFn, MemeUI::UiPainter &, MemeUI::UiRect, const MemeUI::UiInputState &)> paintInvoker = nullptr,
    std::function<void(const Value &cbFn, const MemeUI::UiEvent &)> eventInvoker = nullptr)
{
    Value d = nodeVal.deref();
    if (d.isNil() || (std::holds_alternative<bool>(d.data) && !std::get<bool>(d.data))) {
        MemeUI::UiNode empty;
        empty.isText = true;
        empty.text = "";
        return empty;
    }
    if (d.isList()) {
        MemeUI::UiNode frag;
        frag.tag = "fragment";
        for (const auto &ch : d.getList()->elements) {
            frag.children.push_back(valueToUiNode(ch, outCallbacks, paintInvoker, eventInvoker));
        }
        return frag;
    }
    if (!d.isMap()) {
        MemeUI::UiNode txt;
        txt.isText = true;
        txt.text = d.toString();
        return txt;
    }

    auto map = d.getMap();
    MemeUI::UiNode node;
    std::string typeStr = map->has("type") ? map->entries["type"].deref().toString() : "";
    node.tag = map->has("tag") ? map->entries["tag"].deref().toString() : "";
    if (node.tag.empty()) {
        if (typeStr == "likh") {
            node.tag = "p";
            MemeUI::UiNode txt; txt.isText = true;
            txt.text = map->has("text") ? map->entries["text"].deref().toString() : "";
            node.children.push_back(std::move(txt));
        } else if (typeStr == "button") {
            node.tag = "button";
            node.classStr = "px-4 py-2 bg-indigo-600 text-white rounded-xl text-sm font-medium";
            MemeUI::UiNode txt; txt.isText = true;
            txt.text = map->has("label") ? map->entries["label"].deref().toString() : "Button";
            node.children.push_back(std::move(txt));
        } else if (typeStr == "puch") {
            node.tag = "input";
            node.classStr = "px-3 py-2 bg-slate-900 border border-slate-700 rounded-xl text-sm text-white";
        } else if (typeStr == "badge") {
            node.tag = "span";
            node.classStr = "px-2 py-0.5 rounded-full text-xs font-semibold bg-emerald-500/20 text-emerald-400";
            MemeUI::UiNode txt; txt.isText = true;
            txt.text = map->has("label") ? map->entries["label"].deref().toString() : "";
            node.children.push_back(std::move(txt));
        } else if (typeStr == "faasla") {
            node.tag = "div";
            node.classStr = "h-4";
        } else if (typeStr == "katar") {
            node.tag = "div";
            node.classStr = "flex flex-row gap-4";
        } else if (typeStr == "khamba" || typeStr == "dabba") {
            node.tag = "div";
            node.classStr = (typeStr == "dabba") ? "flex flex-col gap-2 p-4 rounded-xl border border-slate-800 bg-slate-900" : "flex flex-col gap-3";
        } else {
            node.tag = "div";
        }
    }

    node.classStr = map->has("class") ? map->entries["class"].deref().toString() : node.classStr;
    std::shared_ptr<MemeMap> pm = nullptr;
    if (map->has("props") && map->entries["props"].deref().isMap()) {
        pm = map->entries["props"].deref().getMap();
    }
    if (node.classStr.empty() && pm) {
        if (pm->has("class")) node.classStr = pm->entries["class"].deref().toString();
        else if (pm->has("className")) node.classStr = pm->entries["className"].deref().toString();
    }

    auto readProp = [&](const std::string &k) -> Value {
        if (pm && pm->has(k)) return pm->entries[k];
        if (map->has(k)) return map->entries[k];
        return Value(std::monostate{});
    };

    if (pm) {
        for (const auto &kv : pm->entries) {
            if (kv.first == "children" || kv.first == "onClick" || kv.first == "onclick" ||
                kv.first == "onChange" || kv.first == "onchange" || kv.first == "onInput" ||
                kv.first == "onKeyDown" || kv.first == "onPaint" || kv.first == "onDraw") {
                continue;
            }
            Value pv = kv.second.deref();
            if (pv.isNumeric()) {
                node.numProps[kv.first] = pv.asDouble();
                node.props[kv.first] = pv.toString();
            } else if (std::holds_alternative<bool>(pv.data)) {
                node.boolProps[kv.first] = std::get<bool>(pv.data);
                node.props[kv.first] = std::get<bool>(pv.data) ? "true" : "false";
            } else if (!pv.isNil() && !pv.isCallable()) {
                node.props[kv.first] = pv.toString();
            }
            if (kv.first.rfind("aria-", 0) == 0 && !pv.isNil()) {
                node.aria[kv.first] = pv.toString();
            }
        }
    }

    Value kV = readProp("key"); if (!kV.deref().isNil()) node.key = kV.deref().toString();
    Value idV = readProp("id"); if (!idV.deref().isNil()) node.idAttr = idV.deref().toString();
    Value tidV = readProp("data-testid"); if (!tidV.deref().isNil()) node.testId = tidV.deref().toString();
    if (node.testId.empty()) { Value tid2 = readProp("testId"); if (!tid2.deref().isNil()) node.testId = tid2.deref().toString(); }
    Value roleV = readProp("role"); if (!roleV.deref().isNil()) node.role = roleV.deref().toString();
    Value alV = readProp("aria-label");
    if (alV.deref().isNil()) alV = readProp("accessibleName");
    if (!alV.deref().isNil()) node.ariaLabel = alV.deref().toString();
    Value dirV = readProp("dir");
    if (!dirV.deref().isNil()) node.dir = dirV.deref().toString();
    else if (readProp("rtl").deref().isTruthy()) node.dir = "rtl";

    node.disabled = readProp("disabled").deref().isTruthy();
    node.readOnly = readProp("readOnly").deref().isTruthy();
    if (readProp("hidden").deref().isTruthy()) node.classStr += " hidden";
    node.checked = readProp("checked").deref().isTruthy();
    node.indeterminate = readProp("indeterminate").deref().isTruthy();
    node.selected = readProp("selected").deref().isTruthy();
    node.expanded = readProp("expanded").deref().isTruthy() || readProp("aria-expanded").deref().isTruthy();
    node.open = readProp("open").deref().isTruthy();
    node.isPortal = readProp("portal").deref().isTruthy() || node.tag == "dialog" || node.tag == "popover" || node.tag == "tooltip" || node.tag == "toast" || node.tag == "contextmenu";
    Value tabV = readProp("tabIndex").deref();
    if (tabV.isNumeric()) node.tabIndex = static_cast<int>(tabV.asInt());
    Value zV = readProp("zIndex").deref();
    if (zV.isNumeric()) node.zIndex = static_cast<int>(zV.asInt());

    Value clickHandler = readProp("onClick");
    if (clickHandler.deref().isNil()) clickHandler = readProp("onclick");
    Value changeHandler = readProp("onChange");
    if (changeHandler.deref().isNil()) changeHandler = readProp("onchange");
    if (changeHandler.deref().isNil()) changeHandler = readProp("onInput");
    Value keyHandler = readProp("onKeyDown");
    if (keyHandler.deref().isNil()) keyHandler = readProp("onkeydown");
    Value submitHandler = readProp("onSubmit");
    if (submitHandler.deref().isNil()) submitHandler = readProp("onsubmit");
    Value paintHandler = readProp("onPaint");
    if (paintHandler.deref().isNil()) paintHandler = readProp("onDraw");

    bool hasClick = (node.tag == "button" || node.tag == "a" || node.tag == "checkbox" || node.tag == "switch" ||
                     node.tag == "radio" || node.tag == "tab" || node.tag == "select" || node.tag == "combobox" ||
                     !clickHandler.deref().isNil());
    bool hasChange = (node.tag == "input" || node.tag == "textarea" || node.tag == "select" || node.tag == "combobox" ||
                      node.tag == "slider" || node.tag == "checkbox" || node.tag == "switch" || node.tag == "radio" ||
                      !changeHandler.deref().isNil());
    bool isScroll = (node.tag == "scrollarea" || node.classStr.find("overflow-auto") != std::string::npos || node.classStr.find("overflow-scroll") != std::string::npos);

    auto attachHandler = [&](const std::string &evName, const Value &cbVal) {
        if (cbVal.deref().isNil()) return;
        Value cbCopy = cbVal.deref();
        if (eventInvoker) {
            node.handlers[evName] = [cbCopy, eventInvoker](const MemeUI::UiEvent &ev) {
                eventInvoker(cbCopy, ev);
            };
        } else {
            node.handlers[evName] = [](const MemeUI::UiEvent &) {};
        }
    };
    attachHandler("click", clickHandler);
    attachHandler("change", changeHandler);
    attachHandler("keydown", keyHandler);
    attachHandler("submit", submitHandler);

    if (!paintHandler.deref().isNil() && paintInvoker) {
        Value pCopy = paintHandler.deref();
        node.onPaint = [pCopy, paintInvoker](MemeUI::UiPainter &p, MemeUI::UiRect r, const MemeUI::UiInputState &in) {
            paintInvoker(pCopy, p, r, in);
        };
    }

    if (outCallbacks && (hasClick || hasChange || isScroll || !keyHandler.deref().isNil() || !node.testId.empty())) {
        ValueCallbackRecord rec;
        rec.onClick = clickHandler;
        rec.onChange = changeHandler;
        rec.onKeyDown = keyHandler;
        rec.onPaint = paintHandler;
        outCallbacks->push_back(std::move(rec));
    }

    if (map->has("children")) {
        Value chVal = map->entries["children"].deref();
        if (chVal.isList()) {
            for (const auto &c : chVal.getList()->elements) {
                node.children.push_back(valueToUiNode(c, outCallbacks, paintInvoker, eventInvoker));
            }
        } else if (!chVal.isNil()) {
            node.children.push_back(valueToUiNode(chVal, outCallbacks, paintInvoker, eventInvoker));
        }
    }
    return node;
}

inline std::vector<GpuDrawCmd> computeNativeLayout(
    const Value &jsxTree,
    float viewportW = 1000.0f,
    float viewportH = 800.0f,
    std::vector<HitRegion> *outHitRegions = nullptr,
    MemeUI::UiInteractionState *interaction = nullptr,
    const MemeUI::UiInputState *input = nullptr,
    std::function<void(const Value &, MemeUI::UiPainter &, MemeUI::UiRect, const MemeUI::UiInputState &)> paintInvoker = nullptr)
{
    std::vector<ValueCallbackRecord> callbacks;
    MemeUI::UiNode root = valueToUiNode(jsxTree, outHitRegions ? &callbacks : nullptr, paintInvoker);
    std::vector<MemeUI::UiHitRegion> uiHits;
    auto cmds = MemeUI::computeUiLayout(root, viewportW, viewportH, outHitRegions ? &uiHits : nullptr, interaction, input);

    if (outHitRegions) {
        outHitRegions->clear();
        for (size_t i = 0; i < uiHits.size(); ++i) {
            const auto &uh = uiHits[i];
            HitRegion hr;
            hr.id = uh.id;
            hr.nodeKey = uh.nodeKey;
            hr.x = uh.x; hr.y = uh.y; hr.width = uh.width; hr.height = uh.height;
            hr.clipRect = uh.clipRect;
            hr.zIndex = uh.zIndex;
            hr.tag = uh.tag;
            hr.role = uh.role;
            hr.accessibleName = uh.accessibleName;
            hr.testId = uh.testId;
            hr.cursor = uh.cursor;
            hr.disabled = uh.disabled;
            hr.readOnly = uh.readOnly;
            hr.focused = uh.focused;
            hr.hovered = uh.hovered;
            hr.focusable = uh.focusable;
            hr.tabIndex = uh.tabIndex;
            hr.hasClick = uh.hasClick;
            hr.hasChange = uh.hasChange;
            hr.hasKeyDown = uh.hasKeyDown;
            hr.hasScroll = uh.hasScroll;
            hr.isScrollContainer = uh.isScrollContainer;
            hr.scrollY = uh.scrollY;
            hr.maxScrollY = uh.maxScrollY;
            hr.inputValue = uh.inputValue;
            hr.placeholder = uh.placeholder;
            hr.isPassword = uh.isPassword;
            hr.isMultiline = uh.isMultiline;
            hr.checked = uh.checked;
            hr.indeterminate = uh.indeterminate;
            hr.selected = uh.selected;
            hr.expanded = uh.expanded;
            hr.numValue = uh.numValue;
            hr.numMin = uh.numMin;
            hr.numMax = uh.numMax;
            hr.numStep = uh.numStep;
            hr.isRtl = uh.isRtl;
            if (i < callbacks.size()) {
                hr.onClick = callbacks[i].onClick;
                hr.onChange = callbacks[i].onChange;
                hr.onKeyDown = callbacks[i].onKeyDown;
                hr.onPaint = callbacks[i].onPaint;
            }
            outHitRegions->push_back(std::move(hr));
        }
    }
    return cmds;
}

inline std::string escapeJsonStr(const std::string &input) {
    return MemeUI::escapeJson(input);
}

inline std::string hitRegionsToJson(const std::vector<HitRegion> &regions) {
    std::vector<MemeUI::UiHitRegion> tmp;
    tmp.reserve(regions.size());
    for (const auto &r : regions) {
        MemeUI::UiHitRegion u;
        u.id = r.id; u.nodeKey = r.nodeKey; u.x = r.x; u.y = r.y; u.width = r.width; u.height = r.height;
        u.tag = r.tag; u.role = r.role; u.accessibleName = r.accessibleName; u.testId = r.testId;
        u.cursor = r.cursor; u.hasClick = r.hasClick; u.hasChange = r.hasChange;
        u.focused = r.focused; u.disabled = r.disabled; u.checked = r.checked; u.selected = r.selected;
        u.inputValue = r.inputValue;
        tmp.push_back(std::move(u));
    }
    return MemeUI::hitRegionsToJsonStr(tmp);
}

inline std::string gpuCommandsToJson(const std::vector<GpuDrawCmd> &cmds) {
    return MemeUI::gpuDrawCmdsToJson(cmds);
}

} // namespace TailwindEngine

#endif // TAILWIND_ENGINE_H
