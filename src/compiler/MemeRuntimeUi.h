#pragma once

#include "MemeRuntime.h"
#include "../ui/MemeUiTypes.h"
#include "../ui/MemeUiTextEngine.h"
#include "../ui/MemeUiTessellator.h"
#include "../ui/MemeUiPrimitives.h"
#include "../ui/MemeUiLayout.h"
#include "../ui/MemeEgui.h"
#include "../ui/MemeUiRuntime.h"

#if defined(__APPLE__)
#include <TargetConditionals.h>
#if defined(__OBJC__) && !(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
#include "../hosts/macos/MemeMacHost.h"
#endif
#elif defined(_WIN32)
#include "../hosts/windows/MemeWindowsHost.h"
#elif defined(__linux__) && !defined(__ANDROID__)
#include "../hosts/linux/MemeLinuxHost.h"
#elif defined(__EMSCRIPTEN__)
#include "../hosts/web/MemeWebCanvasHost.h"
#endif

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace MemeAotUI {

inline bool hookValuesEqual(const MemeValue &a, const MemeValue &b) {
    if (a.type != b.type) {
        if ((a.type == MemeValue::TYPE_INT || a.type == MemeValue::TYPE_FLOAT) &&
            (b.type == MemeValue::TYPE_INT || b.type == MemeValue::TYPE_FLOAT)) {
            return a.as_double() == b.as_double();
        }
        return false;
    }
    if (a.type == MemeValue::TYPE_NALLA) return true;
    if (a.type == MemeValue::TYPE_BOOL) return a.b_val == b.b_val;
    if (a.type == MemeValue::TYPE_INT) return a.i_val == b.i_val;
    if (a.type == MemeValue::TYPE_FLOAT) return a.f_val == b.f_val;
    if (a.type == MemeValue::TYPE_STRING) return a.s_val == b.s_val;
    if (a.type == MemeValue::TYPE_LIST) return a.list_data.get() == b.list_data.get();
    if (a.type == MemeValue::TYPE_MAP) return a.map_data.get() == b.map_data.get();
    return a == b;
}

inline bool hookDepsChanged(const std::vector<MemeValue> &prev, const MemeValue &depsList) {
    if (depsList.type != MemeValue::TYPE_LIST || !depsList.list_data) return true;
    const auto &cur = *depsList.list_data;
    if (prev.size() != cur.size()) return true;
    for (size_t i = 0; i < cur.size(); ++i) {
        if (!hookValuesEqual(prev[i], cur[i])) return true;
    }
    return false;
}

class AotUiRuntime {
public:
    MemeUI::UiSession session;
    MemeValue rootCallable;
    MemeValue rootTree;
    std::string rootComponentName = "App";
    std::string lastGpuCommandsJson = "[]";
    std::string lastHitRegionsJson = "[]";

    // Hooks & Contexts
    std::unordered_map<std::string, MemeValue> hookSlots;
    std::vector<std::string> componentPathStack;
    std::unordered_map<std::string, int> componentInstanceCounters;
    std::unordered_map<std::string, int> componentHookCursors;
    std::unordered_map<std::string, std::vector<MemeValue>> effectDepsSlots;
    std::unordered_map<std::string, bool> effectHasDeps;
    std::unordered_map<std::string, MemeValue> effectCleanupSlots;
    std::vector<std::function<void()>> pendingEffects;
    std::unordered_map<std::string, std::pair<MemeValue, std::vector<MemeValue>>> memoSlots;

    std::unordered_map<std::string, MemeValue> contextDefaults;
    std::unordered_map<std::string, std::vector<MemeValue>> contextStacks;
    int nextContextId = 0;

    std::string routerPath = "/";
    std::unordered_map<std::string, std::string> routerQuery;

    // Platform runner hook injected by OS hosts (Win/Linux/Android/iOS/Web)
    std::function<int(const std::string &, MemeUI::UiSession &, int, int)> customHostRunner = nullptr;

    void pushComponentCall(const std::string &name) {
        std::string parentPath = componentPathStack.empty() ? "root" : componentPathStack.back();
        std::string countKey = parentPath + "/" + name;
        int idx = componentInstanceCounters[countKey]++;
        std::string fullPath = countKey + "#" + std::to_string(idx);
        componentPathStack.push_back(fullPath);
        componentHookCursors[fullPath] = 0;
    }

    void popComponentCall() {
        if (!componentPathStack.empty()) componentPathStack.pop_back();
    }

    std::string getCurrentComponentPath() const {
        return componentPathStack.empty() ? "global" : componentPathStack.back();
    }

    int getNextHookCursor(const std::string &path) {
        return componentHookCursors[path]++;
    }

    void pushContext(const std::string &ctxId, const MemeValue &val) {
        contextStacks[ctxId].push_back(val);
    }

    void popContext(const std::string &ctxId) {
        auto it = contextStacks.find(ctxId);
        if (it != contextStacks.end() && !it->second.empty()) {
            it->second.pop_back();
        }
    }

    MemeValue readContext(const std::string &ctxId) const {
        auto it = contextStacks.find(ctxId);
        if (it != contextStacks.end() && !it->second.empty()) return it->second.back();
        auto dit = contextDefaults.find(ctxId);
        if (dit != contextDefaults.end()) return dit->second;
        return MemeValue();
    }

    static MemeValue wrapUiPainter(MemeUI::UiPainter p, float ox = 0.0f, float oy = 0.0f, float w = 0.0f, float h = 0.0f) {
        MemeValue m = make_map({});
        m["width"] = MemeValue(static_cast<double>(w));
        m["height"] = MemeValue(static_cast<double>(h));

        auto rectFilledFn = meme_fn([p, ox, oy](const std::vector<MemeValue> &a) mutable -> MemeValue {
            if (a.size() < 5) return MemeValue();
            float x = ox + static_cast<float>(a[0].as_double());
            float y = oy + static_cast<float>(a[1].as_double());
            float rw = static_cast<float>(a[2].as_double());
            float rh = static_cast<float>(a[3].as_double());
            float rad = (a.size() > 4 && (a[4].type == MemeValue::TYPE_INT || a[4].type == MemeValue::TYPE_FLOAT)) ? static_cast<float>(a[4].as_double()) : 0.0f;
            std::string col = (a.size() > 5) ? a[5].to_string() : (a[4].type == MemeValue::TYPE_STRING ? a[4].to_string() : "#ffffff");
            float al = (a.size() > 6) ? static_cast<float>(a[6].as_double()) : 1.0f;
            p.rectFilled(x, y, rw, rh, rad, col, al);
            return MemeValue();
        });
        m["rect_filled"] = rectFilledFn;
        m["rectFilled"] = rectFilledFn;

        auto rectStrokeFn = meme_fn([p, ox, oy](const std::vector<MemeValue> &a) mutable -> MemeValue {
            if (a.size() < 7) return MemeValue();
            p.rectStroke(ox + static_cast<float>(a[0].as_double()), oy + static_cast<float>(a[1].as_double()),
                         static_cast<float>(a[2].as_double()), static_cast<float>(a[3].as_double()),
                         static_cast<float>(a[4].as_double()), static_cast<float>(a[5].as_double()),
                         a[6].to_string(), a.size() > 7 ? static_cast<float>(a[7].as_double()) : 1.0f);
            return MemeValue();
        });
        m["rect_stroke"] = rectStrokeFn;
        m["rectStroke"] = rectStrokeFn;

        auto circleFilledFn = meme_fn([p, ox, oy](const std::vector<MemeValue> &a) mutable -> MemeValue {
            if (a.size() < 4) return MemeValue();
            p.circleFilled(ox + static_cast<float>(a[0].as_double()), oy + static_cast<float>(a[1].as_double()),
                           static_cast<float>(a[2].as_double()), a[3].to_string(), a.size() > 4 ? static_cast<float>(a[4].as_double()) : 1.0f);
            return MemeValue();
        });
        m["circle_filled"] = circleFilledFn;
        m["circleFilled"] = circleFilledFn;

        auto circleStrokeFn = meme_fn([p, ox, oy](const std::vector<MemeValue> &a) mutable -> MemeValue {
            if (a.size() < 5) return MemeValue();
            p.circleStroke(ox + static_cast<float>(a[0].as_double()), oy + static_cast<float>(a[1].as_double()),
                           static_cast<float>(a[2].as_double()), static_cast<float>(a[3].as_double()),
                           a[4].to_string(), a.size() > 5 ? static_cast<float>(a[5].as_double()) : 1.0f);
            return MemeValue();
        });
        m["circle_stroke"] = circleStrokeFn;
        m["circleStroke"] = circleStrokeFn;

        m["line"] = meme_fn([p, ox, oy](const std::vector<MemeValue> &a) mutable -> MemeValue {
            if (a.size() < 5) return MemeValue();
            float lw = (a.size() >= 6 && (a[4].type == MemeValue::TYPE_INT || a[4].type == MemeValue::TYPE_FLOAT)) ? static_cast<float>(a[4].as_double()) : 1.5f;
            std::string col = (a.size() >= 6 && a[5].type == MemeValue::TYPE_STRING) ? a[5].to_string() : a[4].to_string();
            float al = (a.size() > 6) ? static_cast<float>(a[6].as_double()) : 1.0f;
            p.line(ox + static_cast<float>(a[0].as_double()), oy + static_cast<float>(a[1].as_double()),
                   ox + static_cast<float>(a[2].as_double()), oy + static_cast<float>(a[3].as_double()), lw, col, al);
            return MemeValue();
        });

        auto parsePts = [ox, oy](const MemeValue &v) {
            std::vector<MemeUI::UiPoint> pts;
            if (v.type != MemeValue::TYPE_LIST || !v.list_data) return pts;
            for (const auto &el : *v.list_data) {
                if (el.type == MemeValue::TYPE_LIST && el.list_data && el.list_data->size() >= 2) {
                    pts.push_back({ox + static_cast<float>((*el.list_data)[0].as_double()),
                                   oy + static_cast<float>((*el.list_data)[1].as_double())});
                } else if (el.type == MemeValue::TYPE_MAP) {
                    pts.push_back({ox + static_cast<float>(el.get("x", MemeValue(0.0)).as_double()),
                                   oy + static_cast<float>(el.get("y", MemeValue(0.0)).as_double())});
                }
            }
            return pts;
        };

        m["polyline"] = meme_fn([p, parsePts](const std::vector<MemeValue> &a) mutable -> MemeValue {
            if (a.size() < 2) return MemeValue();
            auto pts = parsePts(a[0]);
            float lw = (a[1].type == MemeValue::TYPE_INT || a[1].type == MemeValue::TYPE_FLOAT) ? static_cast<float>(a[1].as_double()) : 2.0f;
            std::string col = (a.size() > 2 && a[2].type == MemeValue::TYPE_STRING) ? a[2].to_string() : (a[1].type == MemeValue::TYPE_STRING ? a[1].to_string() : "#ffffff");
            p.polyline(pts, lw, col, a.size() > 3 ? static_cast<float>(a[3].as_double()) : 1.0f);
            return MemeValue();
        });

        m["polygon"] = meme_fn([p, parsePts](const std::vector<MemeValue> &a) mutable -> MemeValue {
            if (a.size() < 2) return MemeValue();
            p.polygon(parsePts(a[0]), a[1].to_string(), a.size() > 2 ? static_cast<float>(a[2].as_double()) : 1.0f);
            return MemeValue();
        });

        m["text"] = meme_fn([p, ox, oy](const std::vector<MemeValue> &a) mutable -> MemeValue {
            if (a.size() < 4) return MemeValue();
            float fs = (a[3].type == MemeValue::TYPE_INT || a[3].type == MemeValue::TYPE_FLOAT) ? static_cast<float>(a[3].as_double()) : 14.0f;
            std::string col = (a.size() > 4 && a[4].type == MemeValue::TYPE_STRING) ? a[4].to_string() : (a[3].type == MemeValue::TYPE_STRING ? a[3].to_string() : "#ffffff");
            int fw = (a.size() > 5) ? static_cast<int>(a[5].as_int()) : 400;
            bool mono = (a.size() > 6) ? a[6].is_truthy() : false;
            float al = (a.size() > 7) ? static_cast<float>(a[7].as_double()) : 1.0f;
            p.text(ox + static_cast<float>(a[0].as_double()), oy + static_cast<float>(a[1].as_double()),
                   a[2].to_string(), fs, col, fw, mono, al);
            return MemeValue();
        });

        auto svgFn = meme_fn([p, ox, oy](const std::vector<MemeValue> &a) mutable -> MemeValue {
            if (a.size() < 6) return MemeValue();
            p.svgPath(ox + static_cast<float>(a[0].as_double()), oy + static_cast<float>(a[1].as_double()),
                      static_cast<float>(a[2].as_double()), static_cast<float>(a[3].as_double()),
                      a[4].to_string(), a[5].to_string(),
                      a.size() > 6 ? a[6].is_truthy() : false,
                      a.size() > 7 ? static_cast<float>(a[7].as_double()) : 1.5f,
                      a.size() > 8 ? static_cast<float>(a[8].as_double()) : 1.0f);
            return MemeValue();
        });
        m["svg_path"] = svgFn;
        m["svgPath"] = svgFn;

        auto pushClipFn = meme_fn([p, ox, oy](const std::vector<MemeValue> &a) mutable -> MemeValue {
            if (a.size() >= 4) p.pushClip(ox + static_cast<float>(a[0].as_double()), oy + static_cast<float>(a[1].as_double()), static_cast<float>(a[2].as_double()), static_cast<float>(a[3].as_double()));
            return MemeValue();
        });
        auto popClipFn = meme_fn([p](const std::vector<MemeValue> &) mutable -> MemeValue {
            p.popClip();
            return MemeValue();
        });
        m["push_clip"] = pushClipFn;
        m["pushClip"] = pushClipFn;
        m["pop_clip"] = popClipFn;
        m["popClip"] = popClipFn;
        return m;
    }

    MemeUI::UiNode valueToUiNode(const MemeValue &v) {
        if (v.type == MemeValue::TYPE_NALLA || v.type == MemeValue::TYPE_BOOL) {
            MemeUI::UiNode empty;
            empty.isText = true;
            empty.text = "";
            return empty;
        }
        if (v.type == MemeValue::TYPE_LIST && v.list_data) {
            MemeUI::UiNode frag;
            frag.tag = "fragment";
            for (const auto &ch : *v.list_data) {
                frag.children.push_back(valueToUiNode(ch));
            }
            return frag;
        }
        if (v.type != MemeValue::TYPE_MAP || !v.map_data) {
            MemeUI::UiNode txt;
            txt.isText = true;
            txt.text = v.to_string();
            return txt;
        }

        MemeUI::UiNode node;
        std::string typeStr = v.get("type", MemeValue("")).to_string();
        node.tag = v.get("tag", MemeValue("")).to_string();
        if (node.tag.empty()) {
            if (typeStr == "likh") {
                node.tag = "p";
                MemeUI::UiNode t; t.isText = true; t.text = v.get("text", MemeValue("")).to_string();
                node.children.push_back(std::move(t));
            } else if (typeStr == "button") {
                node.tag = "button";
                node.classStr = "px-4 py-2 bg-indigo-600 text-white rounded-xl text-sm font-medium";
                MemeUI::UiNode t; t.isText = true; t.text = v.get("label", MemeValue("Button")).to_string();
                node.children.push_back(std::move(t));
            } else if (typeStr == "puch") {
                node.tag = "input";
                node.classStr = "px-3 py-2 bg-slate-900 border border-slate-700 rounded-xl text-sm text-white";
            } else if (typeStr == "badge") {
                node.tag = "span";
                node.classStr = "px-2 py-0.5 rounded-full text-xs font-semibold bg-emerald-500/20 text-emerald-400";
                MemeUI::UiNode t; t.isText = true; t.text = v.get("label", MemeValue("")).to_string();
                node.children.push_back(std::move(t));
            } else if (typeStr == "faasla") {
                node.tag = "div"; node.classStr = "h-4";
            } else if (typeStr == "katar") {
                node.tag = "div"; node.classStr = "flex flex-row gap-4";
            } else if (typeStr == "khamba" || typeStr == "dabba") {
                node.tag = "div";
                node.classStr = (typeStr == "dabba") ? "flex flex-col gap-2 p-4 rounded-xl border border-slate-800 bg-slate-900" : "flex flex-col gap-3";
            } else {
                node.tag = "div";
            }
        }

        node.classStr = v.get("class", MemeValue(node.classStr)).to_string();
        MemeValue props = v.get("props", MemeValue());
        auto readProp = [&](const std::string &k) -> MemeValue {
            if (props.type == MemeValue::TYPE_MAP && props.has(k)) return props.get(k);
            if (v.has(k)) return v.get(k);
            return MemeValue();
        };

        if (node.classStr.empty() && props.type == MemeValue::TYPE_MAP) {
            if (props.has("class")) node.classStr = props.get("class").to_string();
            else if (props.has("className")) node.classStr = props.get("className").to_string();
        }
        if (props.type == MemeValue::TYPE_MAP && props.map_data) {
            for (const auto &k : props.map_data->map_keys) {
                if (k == "children" || k == "onClick" || k == "onclick" || k == "onChange" ||
                    k == "onchange" || k == "onInput" || k == "onKeyDown" || k == "onPaint" || k == "onDraw") {
                    continue;
                }
                const MemeValue &pv = props.map_data->map_val[k];
                if (pv.type == MemeValue::TYPE_INT || pv.type == MemeValue::TYPE_FLOAT) {
                    node.numProps[k] = pv.as_double();
                    node.props[k] = pv.to_string();
                } else if (pv.type == MemeValue::TYPE_BOOL) {
                    node.boolProps[k] = pv.b_val;
                    node.props[k] = pv.b_val ? "true" : "false";
                } else if (pv.type != MemeValue::TYPE_NALLA && !pv.is_callable()) {
                    node.props[k] = pv.to_string();
                }
                if (k.rfind("aria-", 0) == 0 && pv.type != MemeValue::TYPE_NALLA) {
                    node.aria[k] = pv.to_string();
                }
            }
        }

        MemeValue kv = readProp("key"); if (kv.type != MemeValue::TYPE_NALLA) node.key = kv.to_string();
        MemeValue idv = readProp("id"); if (idv.type != MemeValue::TYPE_NALLA) node.idAttr = idv.to_string();
        MemeValue tid = readProp("data-testid"); if (tid.type == MemeValue::TYPE_NALLA) tid = readProp("testId");
        if (tid.type != MemeValue::TYPE_NALLA) node.testId = tid.to_string();
        MemeValue rv = readProp("role"); if (rv.type != MemeValue::TYPE_NALLA) node.role = rv.to_string();
        MemeValue alv = readProp("aria-label");
        if (alv.type == MemeValue::TYPE_NALLA) alv = readProp("accessibleName");
        if (alv.type != MemeValue::TYPE_NALLA) node.ariaLabel = alv.to_string();
        MemeValue dv = readProp("dir");
        if (dv.type != MemeValue::TYPE_NALLA) node.dir = dv.to_string();
        else if (readProp("rtl").is_truthy()) node.dir = "rtl";

        node.disabled = readProp("disabled").is_truthy();
        node.readOnly = readProp("readOnly").is_truthy();
        if (readProp("hidden").is_truthy()) node.classStr += " hidden";
        node.checked = readProp("checked").is_truthy();
        node.indeterminate = readProp("indeterminate").is_truthy();
        node.selected = readProp("selected").is_truthy();
        node.expanded = readProp("expanded").is_truthy() || readProp("aria-expanded").is_truthy();
        node.open = readProp("open").is_truthy();
        node.isPortal = readProp("portal").is_truthy() || node.tag == "dialog" || node.tag == "popover" || node.tag == "tooltip" || node.tag == "toast" || node.tag == "contextmenu";
        MemeValue tabV = readProp("tabIndex");
        if (tabV.type == MemeValue::TYPE_INT || tabV.type == MemeValue::TYPE_FLOAT) node.tabIndex = static_cast<int>(tabV.as_int());
        MemeValue zV = readProp("zIndex");
        if (zV.type == MemeValue::TYPE_INT || zV.type == MemeValue::TYPE_FLOAT) node.zIndex = static_cast<int>(zV.as_int());

        auto attachEv = [&](const std::string &evName, const MemeValue &fn) {
            if (!fn.is_callable()) return;
            MemeValue fnCopy = fn;
            node.handlers[evName] = [fnCopy](const MemeUI::UiEvent &ev) {
                MemeValue evMap = make_map({
                    {"type", MemeValue(ev.type)},
                    {"targetId", MemeValue(static_cast<int64_t>(ev.targetId))},
                    {"value", MemeValue(ev.value)},
                    {"checked", MemeValue(ev.boolValue)},
                    {"numValue", MemeValue(ev.numValue)},
                    {"x", MemeValue(static_cast<double>(ev.x))},
                    {"y", MemeValue(static_cast<double>(ev.y))},
                    {"deltaX", MemeValue(static_cast<double>(ev.deltaX))},
                    {"deltaY", MemeValue(static_cast<double>(ev.deltaY))},
                    {"key", MemeValue(ev.key)},
                    {"target", make_map({{"value", MemeValue(ev.value)}, {"checked", MemeValue(ev.boolValue)}})}
                });
                fnCopy.call_args({evMap});
            };
        };

        MemeValue clk = readProp("onClick"); if (!clk.is_callable()) clk = readProp("onclick");
        MemeValue chg = readProp("onChange"); if (!chg.is_callable()) chg = readProp("onchange"); if (!chg.is_callable()) chg = readProp("onInput");
        MemeValue kd = readProp("onKeyDown"); if (!kd.is_callable()) kd = readProp("onkeydown");
        MemeValue sub = readProp("onSubmit"); if (!sub.is_callable()) sub = readProp("onsubmit");
        MemeValue whl = readProp("onWheel"); if (!whl.is_callable()) whl = readProp("onwheel");
        MemeValue scr = readProp("onScroll"); if (!scr.is_callable()) scr = readProp("onscroll");
        attachEv("click", clk);
        attachEv("change", chg);
        attachEv("keydown", kd);
        attachEv("submit", sub);
        attachEv("wheel", whl);
        attachEv("scroll", scr);

        MemeValue pfn = readProp("onPaint"); if (!pfn.is_callable()) pfn = readProp("onDraw");
        if (pfn.is_callable()) {
            MemeValue pCopy = pfn;
            node.onPaint = [pCopy](MemeUI::UiPainter &p, MemeUI::UiRect r, const MemeUI::UiInputState &) {
                MemeValue pv = wrapUiPainter(p, r.x, r.y, r.width, r.height);
                MemeValue bv = make_map({{"x", MemeValue(static_cast<double>(r.x))}, {"y", MemeValue(static_cast<double>(r.y))},
                                         {"width", MemeValue(static_cast<double>(r.width))}, {"height", MemeValue(static_cast<double>(r.height))}});
                pCopy.call_args({pv, bv});
            };
        }

        MemeValue ch = v.get("children", MemeValue());
        if (ch.type == MemeValue::TYPE_LIST && ch.list_data) {
            for (const auto &c : *ch.list_data) node.children.push_back(valueToUiNode(c));
        } else if (ch.type != MemeValue::TYPE_NALLA) {
            node.children.push_back(valueToUiNode(ch));
        }
        return node;
    }

    void triggerUiRerender() {
        if (!MemeNative::hooks().requestRerender) {
            MemeNative::hooks().requestRerender = [this]() { session.needsRender = true; };
        }
        if (!MemeNative::hooks().getScreenInfo) {
            MemeNative::hooks().getScreenInfo = [this]() {
                MemeNative::ScreenInfo s;
                s.width = session.input.viewportW > 0.0f ? session.input.viewportW : 800.0f;
                s.height = session.input.viewportH > 0.0f ? session.input.viewportH : 600.0f;
                s.scale = session.input.dpiScale > 0.0f ? session.input.dpiScale : 1.0f;
                s.safeTop = session.input.safeTop;
                s.safeBottom = session.input.safeBottom;
                s.safeLeft = session.input.safeLeft;
                s.safeRight = session.input.safeRight;
                return s;
            };
        }
        if (session.isImmediateMode && session.eguiUpdateFn) {
            float vw = session.input.viewportW > 0.0f ? session.input.viewportW : 800.0f;
            float vh = session.input.viewportH > 0.0f ? session.input.viewportH : 600.0f;
            session.renderFrame(vw, vh);
            lastGpuCommandsJson = MemeUI::gpuDrawCmdsToJson(session.lastCmds);
            lastHitRegionsJson = MemeUI::hitRegionsToJsonStr(session.lastHits);
            return;
        }
        if (rootCallable.type == MemeValue::TYPE_NALLA && rootTree.type == MemeValue::TYPE_NALLA) return;

        componentInstanceCounters.clear();
        componentHookCursors.clear();
        componentPathStack.clear();

        MemeValue rendered;
        if (rootCallable.is_callable()) {
            pushComponentCall(rootComponentName.empty() ? "App" : rootComponentName);
            try {
                rendered = rootCallable.call_args({});
                popComponentCall();
            } catch (...) {
                popComponentCall();
                throw;
            }
        } else {
            rendered = rootTree;
        }
        rootTree = rendered;

        session.isImmediateMode = false;
        session.cachedRoot = valueToUiNode(rendered);
        float vw = session.input.viewportW > 0.0f ? session.input.viewportW : 800.0f;
        float vh = session.input.viewportH > 0.0f ? session.input.viewportH : 600.0f;
        session.renderFrame(vw, vh);
        lastGpuCommandsJson = MemeUI::gpuDrawCmdsToJson(session.lastCmds);
        lastHitRegionsJson = MemeUI::hitRegionsToJsonStr(session.lastHits);

        if (!pendingEffects.empty()) {
            static thread_local int tl_fxDepth = 0;
            if (tl_fxDepth < 8) {
                tl_fxDepth++;
                auto effects = std::move(pendingEffects);
                pendingEffects.clear();
                try {
                    for (auto &ef : effects) ef();
                } catch (...) {
                    tl_fxDepth--;
                    throw;
                }
                tl_fxDepth--;
            } else {
                pendingEffects.clear();
            }
        }
    }

    int runNativeWindow(const std::string &title, int width = 1040, int height = 720) {
        if (std::getenv("MEME_HEADLESS") != nullptr) {
            triggerUiRerender();
            return 0;
        }
        if (customHostRunner) {
            return customHostRunner(title, session, width, height);
        }
#if defined(__APPLE__) && defined(__OBJC__) && !(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
        return MemeUI::runMacNativeApp(title, session, width, height, [this]() { triggerUiRerender(); });
#elif defined(_WIN32)
        return MemeUI::runWindowsNativeApp(title, session, width, height, [this]() { triggerUiRerender(); });
#elif defined(__linux__) && !defined(__ANDROID__)
        return MemeUI::runLinuxNativeApp(title, session, width, height, [this]() { triggerUiRerender(); });
#elif defined(__EMSCRIPTEN__)
        MemeWebHost::bindSession(&session, [this]() { triggerUiRerender(); });
        triggerUiRerender();
        return 0;
#else
        triggerUiRerender();
        return 0;
#endif
    }
};

inline AotUiRuntime &aot_ui() {
    static AotUiRuntime instance;
    return instance;
}

inline MemeValue make_jsx_element(
    const std::string &tag,
    bool isFragment,
    const std::vector<std::pair<std::string, MemeValue>> &propsPairs,
    const std::vector<MemeValue> &rawChildren)
{
    MemeValue propsMap = make_map({});
    for (const auto &p : propsPairs) {
        if (p.first == "...") {
            if (p.second.type == MemeValue::TYPE_MAP && p.second.map_data) {
                for (const auto &k : p.second.map_data->map_keys) {
                    MemeValue pv = p.second.get(k);
                    propsMap[k] = pv;
                    if (k == "className" && !propsMap.has("class")) propsMap["class"] = pv;
                }
            }
        } else {
            propsMap[p.first] = p.second;
            if (p.first == "className" && !propsMap.has("class")) propsMap["class"] = p.second;
        }
    }

    auto childrenVec = std::make_shared<std::vector<MemeValue>>();
    auto shouldIgnore = [](const MemeValue &c) -> bool {
        return c.type == MemeValue::TYPE_NALLA || c.type == MemeValue::TYPE_BOOL;
    };
    for (const auto &c : rawChildren) {
        if (c.type == MemeValue::TYPE_LIST && c.list_data) {
            for (const auto &item : *c.list_data) {
                if (!shouldIgnore(item)) childrenVec->push_back(item);
            }
        } else if (!shouldIgnore(c)) {
            childrenVec->push_back(c);
        }
    }

    std::string cls = propsMap.get("class", propsMap.get("className", MemeValue(""))).to_string();
    return make_map({
        {"type", MemeValue("jsx_element")},
        {"tag", MemeValue(isFragment ? "fragment" : tag)},
        {"class", MemeValue(cls)},
        {"props", propsMap},
        {"children", MemeValue(childrenVec)}
    });
}

inline MemeValue make_jsx_component(
    MemeValue comp,
    const std::string &tagName,
    const std::vector<std::pair<std::string, MemeValue>> &propsPairs,
    const std::vector<MemeValue> &rawChildren)
{
    MemeValue propsMap = make_map({});
    for (const auto &p : propsPairs) {
        if (p.first == "...") {
            if (p.second.type == MemeValue::TYPE_MAP && p.second.map_data) {
                for (const auto &k : p.second.map_data->map_keys) {
                    MemeValue pv = p.second.get(k);
                    propsMap[k] = pv;
                    if (k == "className" && !propsMap.has("class")) propsMap["class"] = pv;
                }
            }
        } else {
            propsMap[p.first] = p.second;
            if (p.first == "className" && !propsMap.has("class")) propsMap["class"] = p.second;
        }
    }

    auto childrenVec = std::make_shared<std::vector<MemeValue>>();
    auto shouldIgnore = [](const MemeValue &c) -> bool {
        return c.type == MemeValue::TYPE_NALLA || c.type == MemeValue::TYPE_BOOL;
    };
    for (const auto &c : rawChildren) {
        if (c.type == MemeValue::TYPE_LIST && c.list_data) {
            for (const auto &item : *c.list_data) {
                if (!shouldIgnore(item)) childrenVec->push_back(item);
            }
        } else if (!shouldIgnore(c)) {
            childrenVec->push_back(c);
        }
    }
    propsMap["children"] = MemeValue(childrenVec);

    if (comp.type == MemeValue::TYPE_MAP) {
        if (comp.has(tagName)) comp = comp.get(tagName);
        else if (comp.has("default")) comp = comp.get("default");
    }

    auto &rt = aot_ui();
    rt.pushComponentCall(tagName);
    MemeValue out;
    try {
        out = comp.call_args({propsMap});
        rt.popComponentCall();
    } catch (...) {
        rt.popComponentCall();
        throw;
    }
    if (out.type == MemeValue::TYPE_MAP && comp.is_callable()) {
        out["__rootCallable"] = meme_fn([comp, propsMap](const std::vector<MemeValue> &) -> MemeValue {
            return comp.call_args({propsMap});
        });
        out["__componentName"] = MemeValue(tagName);
    }
    return out;
}

inline void meme_push_context(const MemeValue &ctxObj, const MemeValue &val) {
    std::string ctxId = ctxObj.get("id", MemeValue("ctx_0")).to_string();
    aot_ui().pushContext(ctxId, val);
}

inline void meme_pop_context(const MemeValue &ctxObj) {
    std::string ctxId = ctxObj.get("id", MemeValue("ctx_0")).to_string();
    aot_ui().popContext(ctxId);
}

// ---------------------------------------------------------------------------
// React Hooks Globals (AOT parity)
// ---------------------------------------------------------------------------
inline MemeValue useState(const MemeValue &init = MemeValue()) {
    auto &rt = aot_ui();
    std::string compPath = rt.getCurrentComponentPath();
    int cursor = rt.getNextHookCursor(compPath);
    std::string slotKey = compPath + "@state@" + std::to_string(cursor);

    if (rt.hookSlots.find(slotKey) == rt.hookSlots.end()) {
        rt.hookSlots[slotKey] = init;
    }

    auto setter = meme_fn([slotKey](const std::vector<MemeValue> &args) -> MemeValue {
        auto &rt2 = aot_ui();
        if (!args.empty()) {
            MemeValue nextVal = args[0];
            if (nextVal.is_callable()) {
                nextVal = nextVal.call_args({rt2.hookSlots[slotKey]});
            }
            rt2.hookSlots[slotKey] = nextVal;
            rt2.triggerUiRerender();
        }
        return rt2.hookSlots[slotKey];
    });
    return MemeValue(std::vector<MemeValue>{rt.hookSlots[slotKey], setter});
}
inline MemeValue state_bana(const MemeValue &init = MemeValue()) { return useState(init); }

inline MemeValue useEffect(const MemeValue &fn, const MemeValue &deps = MemeValue()) {
    if (!fn.is_callable()) return MemeValue();
    auto &rt = aot_ui();
    std::string compPath = rt.getCurrentComponentPath();
    int cursor = rt.getNextHookCursor(compPath);
    std::string slotKey = compPath + "@effect@" + std::to_string(cursor);

    bool hasDeps = (deps.type == MemeValue::TYPE_LIST);
    bool shouldRun = false;
    if (rt.effectDepsSlots.find(slotKey) == rt.effectDepsSlots.end() &&
        rt.effectHasDeps.find(slotKey) == rt.effectHasDeps.end()) {
        shouldRun = true;
    } else if (!hasDeps) {
        shouldRun = true;
    } else {
        shouldRun = hookDepsChanged(rt.effectDepsSlots[slotKey], deps);
    }

    if (shouldRun) {
        rt.effectHasDeps[slotKey] = hasDeps;
        if (hasDeps && deps.list_data) rt.effectDepsSlots[slotKey] = *deps.list_data;
        auto runEffect = [fn, slotKey]() {
            auto &rt2 = aot_ui();
            auto cit = rt2.effectCleanupSlots.find(slotKey);
            if (cit != rt2.effectCleanupSlots.end() && cit->second.is_callable()) {
                cit->second.call_args({});
            }
            MemeValue cleanup = fn.call_args({});
            if (cleanup.is_callable()) rt2.effectCleanupSlots[slotKey] = cleanup;
            else rt2.effectCleanupSlots.erase(slotKey);
        };
        if (!rt.componentPathStack.empty()) rt.pendingEffects.push_back(std::move(runEffect));
        else runEffect();
    }
    return MemeValue();
}

inline MemeValue useMemo(const MemeValue &fn, const MemeValue &deps = MemeValue()) {
    auto &rt = aot_ui();
    std::string compPath = rt.getCurrentComponentPath();
    int cursor = rt.getNextHookCursor(compPath);
    std::string slotKey = compPath + "@memo@" + std::to_string(cursor);

    bool hasDeps = (deps.type == MemeValue::TYPE_LIST);
    bool needCompute = (rt.memoSlots.find(slotKey) == rt.memoSlots.end());
    if (!needCompute && hasDeps) needCompute = hookDepsChanged(rt.memoSlots[slotKey].second, deps);
    if (needCompute) {
        MemeValue val = fn.is_callable() ? fn.call_args({}) : fn;
        std::vector<MemeValue> dc = (hasDeps && deps.list_data) ? *deps.list_data : std::vector<MemeValue>{};
        rt.memoSlots[slotKey] = {val, std::move(dc)};
        rt.hookSlots[slotKey] = val;
    }
    return rt.memoSlots[slotKey].first;
}

inline MemeValue useCallback(const MemeValue &fn, const MemeValue &deps = MemeValue()) {
    auto &rt = aot_ui();
    std::string compPath = rt.getCurrentComponentPath();
    int cursor = rt.getNextHookCursor(compPath);
    std::string slotKey = compPath + "@cb@" + std::to_string(cursor);

    bool hasDeps = (deps.type == MemeValue::TYPE_LIST);
    bool needUpdate = (rt.memoSlots.find(slotKey) == rt.memoSlots.end());
    if (!needUpdate && hasDeps) needUpdate = hookDepsChanged(rt.memoSlots[slotKey].second, deps);
    if (needUpdate) {
        std::vector<MemeValue> dc = (hasDeps && deps.list_data) ? *deps.list_data : std::vector<MemeValue>{};
        rt.memoSlots[slotKey] = {fn, std::move(dc)};
        rt.hookSlots[slotKey] = fn;
    }
    return rt.memoSlots[slotKey].first;
}

inline MemeValue useReducer(const MemeValue &reducer, const MemeValue &init, const MemeValue &initFn = MemeValue()) {
    auto &rt = aot_ui();
    std::string compPath = rt.getCurrentComponentPath();
    int cursor = rt.getNextHookCursor(compPath);
    std::string slotKey = compPath + "@reducer@" + std::to_string(cursor);

    if (rt.hookSlots.find(slotKey) == rt.hookSlots.end()) {
        rt.hookSlots[slotKey] = initFn.is_callable() ? initFn.call_args({init}) : init;
    }
    auto dispatch = meme_fn([slotKey, reducer](const std::vector<MemeValue> &a) -> MemeValue {
        auto &rt2 = aot_ui();
        MemeValue act = a.empty() ? MemeValue() : a[0];
        MemeValue nextVal = reducer.is_callable() ? reducer.call_args({rt2.hookSlots[slotKey], act}) : act;
        rt2.hookSlots[slotKey] = nextVal;
        rt2.triggerUiRerender();
        return nextVal;
    });
    return MemeValue(std::vector<MemeValue>{rt.hookSlots[slotKey], dispatch});
}

inline MemeValue useRef(const MemeValue &init = MemeValue()) {
    auto &rt = aot_ui();
    std::string compPath = rt.getCurrentComponentPath();
    int cursor = rt.getNextHookCursor(compPath);
    std::string slotKey = compPath + "@ref@" + std::to_string(cursor);

    if (rt.hookSlots.find(slotKey) == rt.hookSlots.end()) {
        rt.hookSlots[slotKey] = make_map({{"current", init}});
    }
    return rt.hookSlots[slotKey];
}

inline MemeValue createContext(const MemeValue &defaultVal = MemeValue()) {
    auto &rt = aot_ui();
    std::string ctxId = "ctx_" + std::to_string(++rt.nextContextId);
    rt.contextDefaults[ctxId] = defaultVal;
    return make_map({{"id", MemeValue(ctxId)}, {"defaultValue", defaultVal}});
}

inline MemeValue useContext(const MemeValue &ctxObj) {
    std::string ctxId = ctxObj.get("id", MemeValue("")).to_string();
    return aot_ui().readContext(ctxId);
}

inline MemeValue useRouter() {
    auto &rt = aot_ui();
    auto navFn = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue {
        std::string target = a.empty() ? "/" : a[0].to_string();
        auto &rt2 = aot_ui();
        rt2.routerQuery.clear();
        size_t q = target.find('?');
        if (q != std::string::npos) {
            rt2.routerPath = target.substr(0, q);
            std::istringstream ss(target.substr(q + 1));
            std::string pair;
            while (std::getline(ss, pair, '&')) {
                size_t eq = pair.find('=');
                if (eq != std::string::npos) rt2.routerQuery[pair.substr(0, eq)] = pair.substr(eq + 1);
            }
        } else {
            rt2.routerPath = target;
        }
#ifdef __EMSCRIPTEN__
        EM_ASM({
            if (typeof window !== 'undefined' && typeof window.__MEME_PUSH_ROUTE__ === 'function') {
                window.__MEME_PUSH_ROUTE__(UTF8ToString($0));
            }
        }, target.c_str());
#endif
        rt2.triggerUiRerender();
        return MemeValue(rt2.routerPath);
    });
    MemeValue qMap = make_map({});
    for (const auto &kv : rt.routerQuery) qMap[kv.first] = MemeValue(kv.second);
    return make_map({
        {"path", MemeValue(rt.routerPath)},
        {"query", qMap},
        {"navigate", navFn},
        {"push", navFn}
    });
}

inline MemeValue Router(const MemeValue &props = MemeValue()) {
    MemeValue ch = props.get("children", MemeValue(std::vector<MemeValue>{}));
    return make_jsx_element("fragment", true, {}, ch.type == MemeValue::TYPE_LIST && ch.list_data ? *ch.list_data : std::vector<MemeValue>{ch});
}

inline MemeValue Route(const MemeValue &props = MemeValue()) {
    std::string p = props.get("path", MemeValue("")).to_string();
    if (p == "*" || p == aot_ui().routerPath) {
        MemeValue comp = props.get("component", props.get("element", MemeValue()));
        if (comp.is_callable()) return comp.call_args({props});
        MemeValue ch = props.get("children", MemeValue(std::vector<MemeValue>{}));
        return make_jsx_element("fragment", true, {}, ch.type == MemeValue::TYPE_LIST && ch.list_data ? *ch.list_data : std::vector<MemeValue>{ch});
    }
    return MemeValue();
}

inline MemeValue Link(const MemeValue &props = MemeValue()) {
    std::string to = props.get("to", props.get("href", MemeValue("/"))).to_string();
    MemeValue onClick = meme_fn([to](const std::vector<MemeValue> &) -> MemeValue {
        auto &rt2 = aot_ui();
        rt2.routerQuery.clear();
        size_t q = to.find('?');
        if (q != std::string::npos) {
            rt2.routerPath = to.substr(0, q);
            std::istringstream ss(to.substr(q + 1));
            std::string pair;
            while (std::getline(ss, pair, '&')) {
                size_t eq = pair.find('=');
                if (eq != std::string::npos) rt2.routerQuery[pair.substr(0, eq)] = pair.substr(eq + 1);
            }
        } else {
            rt2.routerPath = to;
        }
#ifdef __EMSCRIPTEN__
        EM_ASM({
            if (typeof window !== 'undefined' && typeof window.__MEME_PUSH_ROUTE__ === 'function') {
                window.__MEME_PUSH_ROUTE__(UTF8ToString($0));
            }
        }, to.c_str());
#endif
        rt2.triggerUiRerender();
        return MemeValue(true);
    });
    MemeValue ch = props.get("children", MemeValue(std::vector<MemeValue>{}));
    std::string cls = props.get("class", props.get("className", MemeValue("text-indigo-400 cursor-pointer"))).to_string();
    return make_jsx_element("a", false, {{"class", MemeValue(cls)}, {"onClick", onClick}}, ch.type == MemeValue::TYPE_LIST && ch.list_data ? *ch.list_data : std::vector<MemeValue>{ch});
}

inline MemeValue Platform(const MemeValue &props = MemeValue()) {
    auto matchesVal = [](const MemeValue &v) -> bool {
        if (v.type == MemeValue::TYPE_LIST && v.list_data) {
            for (const auto &it : *v.list_data) {
                if (MemePlatform::matchPlatformSpec(it.to_string())) return true;
            }
            return false;
        }
        return MemePlatform::matchPlatformSpec(v.to_string());
    };
    bool matched = true;
    if (props.type == MemeValue::TYPE_MAP) {
        if (props.has("only")) matched = matched && matchesVal(props.get("only"));
        if (props.has("os")) matched = matched && matchesVal(props.get("os"));
        if (props.has("target")) matched = matched && matchesVal(props.get("target"));
        if (props.has("is")) matched = matched && matchesVal(props.get("is"));

        static const char *kFlags[] = {
            "ios", "android", "mac", "macos", "windows", "win",
            "linux", "web", "wasm", "mobile", "desktop", "apple", "native"
        };
        bool hasPosFlag = false;
        bool posFlagHit = false;
        for (const char *fk : kFlags) {
            if (props.has(fk)) {
                if (props.get(fk).is_truthy()) {
                    hasPosFlag = true;
                    if (MemePlatform::matchPlatformSpec(fk)) posFlagHit = true;
                } else {
                    if (MemePlatform::matchPlatformSpec(fk)) matched = false;
                }
            }
        }
        if (hasPosFlag) matched = matched && posFlagHit;

        if (props.has("exclude") && matchesVal(props.get("exclude"))) matched = false;
        if (props.has("not") && matchesVal(props.get("not"))) matched = false;
    }

    if (matched) {
        MemeValue ch = props.get("children", MemeValue(std::vector<MemeValue>{}));
        return make_jsx_element("fragment", true, {}, ch.type == MemeValue::TYPE_LIST && ch.list_data ? *ch.list_data : std::vector<MemeValue>{ch});
    }
    if (props.has("fallback")) {
        MemeValue fb = props.get("fallback");
        if (fb.is_callable()) return fb.call_args({});
        return fb;
    }
    return MemeValue();
}

// ---------------------------------------------------------------------------
// ui_shadcn & ui_primitives & egui Builders
// ---------------------------------------------------------------------------
inline MemeValue build_shadcn_module() {
    auto comp = [](const std::string &tag, const std::string &baseCls) {
        return meme_fn([tag, baseCls](const std::vector<MemeValue> &args) -> MemeValue {
            MemeValue props = (!args.empty() && args[0].type == MemeValue::TYPE_MAP) ? args[0] : make_map({});
            MemeValue ch = props.get("children", MemeValue(std::vector<MemeValue>{}));
            std::string uc = props.get("class", props.get("className", MemeValue(""))).to_string();
            std::string merged = uc.empty() ? baseCls : (baseCls + " " + uc);
            props["class"] = MemeValue(merged);
            props["className"] = MemeValue(merged);
            std::vector<MemeValue> chVec;
            if (ch.type == MemeValue::TYPE_LIST && ch.list_data) chVec = *ch.list_data;
            else if (ch.type != MemeValue::TYPE_NALLA) chVec.push_back(ch);
            std::vector<std::pair<std::string, MemeValue>> pairs;
            if (props.map_data) {
                for (const auto &k : props.map_data->map_keys) {
                    if (k != "children") pairs.emplace_back(k, props.get(k));
                }
            }
            return make_jsx_element(tag, false, pairs, chVec);
        });
    };

    MemeValue m = make_map({});
    m["Card"] = comp("div", "rounded-2xl border border-slate-800 bg-slate-900/90 text-slate-100");
    m["CardHeader"] = comp("div", "flex flex-col gap-1.5 p-6");
    m["CardTitle"] = comp("h3", "text-2xl font-bold text-white");
    m["CardDescription"] = comp("p", "text-sm text-slate-400");
    m["CardContent"] = comp("div", "p-6 pt-0");
    m["CardFooter"] = comp("div", "flex items-center p-6 pt-0 gap-2");

    m["Button"] = meme_fn([](const std::vector<MemeValue> &args) -> MemeValue {
        MemeValue props = (!args.empty() && args[0].type == MemeValue::TYPE_MAP) ? args[0] : make_map({});
        std::string var = props.get("variant", MemeValue("default")).to_string();
        std::string vCls = "bg-indigo-600 text-white hover:bg-indigo-500";
        if (var == "destructive") vCls = "bg-rose-600 text-white hover:bg-rose-500";
        else if (var == "outline") vCls = "border border-slate-700 bg-transparent hover:bg-slate-800 text-slate-100";
        else if (var == "secondary") vCls = "bg-slate-800 text-slate-100 hover:bg-slate-700";
        else if (var == "ghost") vCls = "hover:bg-slate-800 text-slate-100";
        std::string base = "inline-flex items-center justify-center rounded-xl font-medium px-4 py-2 text-sm " + vCls;
        std::string uc = props.get("class", props.get("className", MemeValue(""))).to_string();
        std::string merged = uc.empty() ? base : (base + " " + uc);
        props["class"] = MemeValue(merged);
        std::vector<MemeValue> chVec;
        MemeValue ch = props.get("children", MemeValue());
        if (ch.type == MemeValue::TYPE_LIST && ch.list_data) chVec = *ch.list_data;
        else if (ch.type != MemeValue::TYPE_NALLA) chVec.push_back(ch);
        std::vector<std::pair<std::string, MemeValue>> pairs;
        if (props.map_data) for (const auto &k : props.map_data->map_keys) if (k != "children") pairs.emplace_back(k, props.get(k));
        return make_jsx_element("button", false, pairs, chVec);
    });

    m["Badge"] = comp("span", "inline-flex items-center rounded-full px-2.5 py-0.5 text-xs font-semibold");
    m["Input"] = comp("input", "flex h-10 w-full rounded-xl border border-slate-800 bg-slate-950 px-3 py-2 text-sm text-white");
    m["Textarea"] = comp("textarea", "flex min-h-[80px] w-full rounded-xl border border-slate-800 bg-slate-950 px-3 py-2 text-sm text-slate-100");
    auto labeledControl = [](const std::string &tag, const std::string &ctrlCls) {
        return meme_fn([tag, ctrlCls](const std::vector<MemeValue> &args) -> MemeValue {
            MemeValue props = (!args.empty() && args[0].type == MemeValue::TYPE_MAP) ? args[0] : make_map({});
            MemeValue ch = props.get("children", MemeValue(std::vector<MemeValue>{}));
            bool hasLabel = props.has("label") && props.get("label").type != MemeValue::TYPE_NALLA;
            bool hasCh = (tag == "checkbox" && ch.type == MemeValue::TYPE_LIST && ch.list_data && !ch.list_data->empty());
            auto toPairs = [](const MemeValue &pm, bool skipClass, bool skipChildren) {
                std::vector<std::pair<std::string, MemeValue>> pairs;
                if (pm.map_data) {
                    for (const auto &k : pm.map_data->map_keys) {
                        if (skipChildren && k == "children") continue;
                        if (skipClass && (k == "class" || k == "className")) continue;
                        pairs.emplace_back(k, pm.get(k));
                    }
                }
                return pairs;
            };
            if (hasLabel || hasCh) {
                MemeValue boxProps = make_map({});
                if (props.map_data) for (const auto &k : props.map_data->map_keys) boxProps[k] = props.get(k);
                if (!boxProps.has("name") && hasLabel) boxProps["name"] = props.get("label");
                boxProps["class"] = MemeValue(ctrlCls);
                std::vector<MemeValue> rowCh;
                rowCh.push_back(make_jsx_element(tag, false, toPairs(boxProps, false, true), {}));
                std::vector<std::pair<std::string, MemeValue>> spanProps = {{"class", MemeValue("text-sm text-slate-200 cursor-pointer")}};
                MemeValue cb = props.get("onChange", props.get("onchange", props.get("onClick", props.get("onclick", MemeValue()))));
                bool isDisabled = props.get("disabled", MemeValue(false)).is_truthy();
                if (cb.is_callable() && !isDisabled) {
                    bool curChecked = props.get("checked", MemeValue(false)).is_truthy();
                    MemeValue toggleFn = meme_fn([cb, curChecked](const std::vector<MemeValue> &) -> MemeValue {
                        MemeValue ev = make_map({
                            {"type", MemeValue("click")},
                            {"checked", MemeValue(!curChecked)},
                            {"target", make_map({{"checked", MemeValue(!curChecked)}})}
                        });
                        cb.call_args({ev});
                        return MemeValue();
                    });
                    spanProps.emplace_back("onClick", toggleFn);
                }
                if (hasLabel) {
                    rowCh.push_back(make_jsx_element("span", false, spanProps, {props.get("label")}));
                } else {
                    rowCh.push_back(make_jsx_element("span", false, spanProps, *ch.list_data));
                }
                std::string wrapCls = "inline-flex items-center gap-2";
                std::string uc = props.get("class", props.get("className", MemeValue(""))).to_string();
                if (!uc.empty()) wrapCls += " " + uc;
                return make_jsx_element("div", false, {{"class", MemeValue(wrapCls)}}, rowCh);
            }
            return make_jsx_element(tag, false, toPairs(props, false, true), {});
        });
    };
    m["Checkbox"] = labeledControl("checkbox", "shrink-0");
    m["Switch"] = labeledControl("switch", "shrink-0");
    m["RadioGroup"] = meme_fn([](const std::vector<MemeValue> &args) -> MemeValue {
        MemeValue props = (!args.empty() && args[0].type == MemeValue::TYPE_MAP) ? args[0] : make_map({});
        if (!props.has("role")) props["role"] = MemeValue("radiogroup");
        std::string uc = props.get("class", props.get("className", MemeValue(""))).to_string();
        props["class"] = MemeValue(uc.empty() ? "flex flex-col gap-2" : ("flex flex-col gap-2 " + uc));
        MemeValue ch = props.get("children", MemeValue(std::vector<MemeValue>{}));
        std::vector<MemeValue> chVec = (ch.type == MemeValue::TYPE_LIST && ch.list_data) ? *ch.list_data : std::vector<MemeValue>{};
        std::vector<std::pair<std::string, MemeValue>> pairs;
        if (props.map_data) for (const auto &k : props.map_data->map_keys) if (k != "children") pairs.emplace_back(k, props.get(k));
        return make_jsx_element("div", false, pairs, chVec);
    });
    m["RadioItem"] = labeledControl("radio", "shrink-0");
    m["Slider"] = comp("slider", "w-full h-6");
    m["Progress"] = comp("progress", "w-full h-2.5 rounded-full bg-slate-800");
    m["Select"] = comp("select", "inline-flex items-center justify-between rounded-xl border border-slate-700 bg-slate-900 px-3 py-2 text-sm text-slate-100");
    m["Combobox"] = comp("combobox", "inline-flex items-center justify-between rounded-xl border border-slate-700 bg-slate-900 px-3 py-2 text-sm text-slate-100");
    m["Accordion"] = comp("div", "flex flex-col divide-y divide-slate-800 rounded-xl border border-slate-800 bg-slate-900/60");
    m["Tabs"] = comp("div", "flex flex-col gap-2");
    m["TabsList"] = meme_fn([](const std::vector<MemeValue> &args) -> MemeValue {
        MemeValue props = (!args.empty() && args[0].type == MemeValue::TYPE_MAP) ? args[0] : make_map({});
        if (!props.has("role")) props["role"] = MemeValue("tablist");
        std::string uc = props.get("class", props.get("className", MemeValue(""))).to_string();
        std::string base = "inline-flex items-center gap-1 rounded-xl bg-slate-900 border border-slate-800 p-1";
        props["class"] = MemeValue(uc.empty() ? base : (base + " " + uc));
        MemeValue ch = props.get("children", MemeValue(std::vector<MemeValue>{}));
        std::vector<MemeValue> chVec = (ch.type == MemeValue::TYPE_LIST && ch.list_data) ? *ch.list_data : std::vector<MemeValue>{};
        std::vector<std::pair<std::string, MemeValue>> pairs;
        if (props.map_data) for (const auto &k : props.map_data->map_keys) if (k != "children") pairs.emplace_back(k, props.get(k));
        return make_jsx_element("div", false, pairs, chVec);
    });
    m["TabsTrigger"] = meme_fn([](const std::vector<MemeValue> &args) -> MemeValue {
        MemeValue props = (!args.empty() && args[0].type == MemeValue::TYPE_MAP) ? args[0] : make_map({});
        if (!props.has("role")) props["role"] = MemeValue("tab");
        bool active = props.get("active", props.get("selected", MemeValue(false))).is_truthy();
        std::string base = active
            ? "inline-flex items-center justify-center rounded-lg px-3 py-1.5 text-sm font-medium bg-indigo-600 text-white"
            : "inline-flex items-center justify-center rounded-lg px-3 py-1.5 text-sm font-medium text-slate-400 hover:bg-slate-800 hover:text-slate-100";
        std::string uc = props.get("class", props.get("className", MemeValue(""))).to_string();
        props["class"] = MemeValue(uc.empty() ? base : (base + " " + uc));
        MemeValue ch = props.get("children", MemeValue(std::vector<MemeValue>{}));
        std::vector<MemeValue> chVec = (ch.type == MemeValue::TYPE_LIST && ch.list_data) ? *ch.list_data : std::vector<MemeValue>{};
        std::vector<std::pair<std::string, MemeValue>> pairs;
        if (props.map_data) for (const auto &k : props.map_data->map_keys) if (k != "children") pairs.emplace_back(k, props.get(k));
        return make_jsx_element("tab", false, pairs, chVec);
    });
    m["TabsContent"] = meme_fn([](const std::vector<MemeValue> &args) -> MemeValue {
        MemeValue props = (!args.empty() && args[0].type == MemeValue::TYPE_MAP) ? args[0] : make_map({});
        if (!props.has("role")) props["role"] = MemeValue("tabpanel");
        std::string uc = props.get("class", props.get("className", MemeValue(""))).to_string();
        std::string base = "mt-2 rounded-xl border border-slate-800 bg-slate-950/60 p-4";
        props["class"] = MemeValue(uc.empty() ? base : (base + " " + uc));
        MemeValue ch = props.get("children", MemeValue(std::vector<MemeValue>{}));
        std::vector<MemeValue> chVec = (ch.type == MemeValue::TYPE_LIST && ch.list_data) ? *ch.list_data : std::vector<MemeValue>{};
        std::vector<std::pair<std::string, MemeValue>> pairs;
        if (props.map_data) for (const auto &k : props.map_data->map_keys) if (k != "children") pairs.emplace_back(k, props.get(k));
        return make_jsx_element("div", false, pairs, chVec);
    });
    m["Dialog"] = meme_fn([](const std::vector<MemeValue> &args) -> MemeValue {
        MemeValue props = (!args.empty() && args[0].type == MemeValue::TYPE_MAP) ? args[0] : make_map({});
        if (!props.has("role")) props["role"] = MemeValue("dialog");
        std::string uc = props.get("class", props.get("className", MemeValue(""))).to_string();
        std::string base = "rounded-2xl border border-slate-700 bg-slate-900 p-6 shadow-2xl text-slate-100";
        props["class"] = MemeValue(uc.empty() ? base : (base + " " + uc));
        MemeValue ch = props.get("children", MemeValue(std::vector<MemeValue>{}));
        std::vector<MemeValue> chVec = (ch.type == MemeValue::TYPE_LIST && ch.list_data) ? *ch.list_data : std::vector<MemeValue>{};
        std::vector<std::pair<std::string, MemeValue>> pairs;
        if (props.map_data) for (const auto &k : props.map_data->map_keys) if (k != "children") pairs.emplace_back(k, props.get(k));
        return make_jsx_element("dialog", false, pairs, chVec);
    });
    m["Popover"] = comp("popover", "rounded-xl border border-slate-700 bg-slate-900 p-3 shadow-xl text-slate-100");
    m["Tooltip"] = comp("tooltip", "rounded-lg border border-slate-700 bg-slate-950 px-2.5 py-1.5 text-xs text-slate-100 shadow-lg");
    m["ContextMenu"] = comp("contextmenu", "min-w-[160px] rounded-xl border border-slate-700 bg-slate-900 p-1 shadow-xl text-slate-100");
    m["Toast"] = comp("toast", "rounded-xl border border-slate-700 bg-slate-900 px-4 py-3 text-sm text-slate-100 shadow-xl");
    m["Table"] = comp("div", "w-full rounded-xl border border-slate-800 bg-slate-950/60 overflow-hidden");
    m["ResizablePanelGroup"] = meme_fn([](const std::vector<MemeValue> &args) -> MemeValue {
        MemeValue props = (!args.empty() && args[0].type == MemeValue::TYPE_MAP) ? args[0] : make_map({});
        std::string dir = props.get("direction", MemeValue("horizontal")).to_string();
        std::string base = (dir == "vertical") ? "flex flex-col w-full h-full" : "flex flex-row w-full h-full";
        std::string uc = props.get("class", props.get("className", MemeValue(""))).to_string();
        props["class"] = MemeValue(uc.empty() ? base : (base + " " + uc));
        MemeValue ch = props.get("children", MemeValue(std::vector<MemeValue>{}));
        std::vector<MemeValue> chVec = (ch.type == MemeValue::TYPE_LIST && ch.list_data) ? *ch.list_data : std::vector<MemeValue>{};
        std::vector<std::pair<std::string, MemeValue>> pairs;
        if (props.map_data) for (const auto &k : props.map_data->map_keys) if (k != "children") pairs.emplace_back(k, props.get(k));
        return make_jsx_element("div", false, pairs, chVec);
    });
    m["ResizablePanel"] = comp("div", "flex-1 min-w-0 min-h-0 overflow-hidden");
    m["ResizableHandle"] = meme_fn([](const std::vector<MemeValue> &args) -> MemeValue {
        MemeValue props = (!args.empty() && args[0].type == MemeValue::TYPE_MAP) ? args[0] : make_map({});
        if (!props.has("role")) props["role"] = MemeValue("separator");
        if (!props.has("aria-label") && !props.has("accessibleName")) props["aria-label"] = MemeValue("Resize panel handle");
        std::string uc = props.get("class", props.get("className", MemeValue(""))).to_string();
        std::string base = "w-1.5 min-h-[24px] bg-slate-800 hover:bg-indigo-500 cursor-col-resize shrink-0";
        props["class"] = MemeValue(uc.empty() ? base : (base + " " + uc));
        std::vector<std::pair<std::string, MemeValue>> pairs;
        if (props.map_data) for (const auto &k : props.map_data->map_keys) if (k != "children") pairs.emplace_back(k, props.get(k));
        return make_jsx_element("div", false, pairs, {});
    });
    m["ScrollArea"] = comp("scrollarea", "overflow-auto flex flex-col");
    m["Avatar"] = comp("div", "inline-flex items-center justify-center w-10 h-10 rounded-full bg-indigo-600 text-white text-sm font-semibold");
    m["Skeleton"] = comp("div", "rounded-xl bg-slate-800/70 h-4 w-full");
    m["Canvas"] = comp("canvas", "w-full h-40 rounded-xl bg-slate-950 border border-slate-800");
    m["Icon"] = comp("icon", "w-4 h-4 text-slate-200");
    m["Separator"] = comp("div", "h-[1px] w-full bg-slate-800 my-2");
    m["Alert"] = comp("div", "relative w-full rounded-xl border border-slate-800 p-4 bg-slate-950");
    m["Platform"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue { return Platform(a.empty() ? MemeValue() : a[0]); });
    return m;
}

inline MemeValue merge_prop_maps(const std::vector<MemeValue> &maps) {
    MemeValue out = make_map({});
    std::unordered_map<std::string, std::vector<MemeValue>> handlers;
    std::vector<std::string> clsParts;
    for (const auto &v : maps) {
        if (v.type != MemeValue::TYPE_MAP || !v.map_data) continue;
        for (const auto &k : v.map_data->map_keys) {
            MemeValue pv = v.get(k);
            if (k == "class" || k == "className") {
                std::string s = pv.to_string();
                if (!s.empty()) clsParts.push_back(s);
            } else if (k.size() > 2 && k[0] == 'o' && k[1] == 'n' && std::isupper(static_cast<unsigned char>(k[2])) && pv.is_callable()) {
                handlers[k].push_back(pv);
            } else {
                out[k] = pv;
            }
        }
    }
    if (!clsParts.empty()) {
        std::string merged;
        for (size_t i = 0; i < clsParts.size(); ++i) {
            if (i > 0) merged += " ";
            merged += clsParts[i];
        }
        out["class"] = MemeValue(merged);
        out["className"] = MemeValue(merged);
    }
    for (const auto &[hk, fns] : handlers) {
        if (fns.size() == 1) out[hk] = fns[0];
        else {
            auto fnsCopy = fns;
            out[hk] = meme_fn([fnsCopy](const std::vector<MemeValue> &a) -> MemeValue {
                MemeValue last;
                for (const auto &fn : fnsCopy) last = fn.call_args(a);
                return last;
            });
        }
    }
    return out;
}

inline MemeValue hit_region_to_meme(const MemeUI::UiHitRegion &hr) {
    return make_map({
        {"id", MemeValue(static_cast<int64_t>(hr.id))},
        {"key", MemeValue(hr.nodeKey)},
        {"tag", MemeValue(hr.tag)},
        {"role", MemeValue(hr.role)},
        {"name", MemeValue(hr.accessibleName)},
        {"accessibleName", MemeValue(hr.accessibleName)},
        {"testId", MemeValue(hr.testId)},
        {"x", MemeValue(static_cast<double>(hr.x))},
        {"y", MemeValue(static_cast<double>(hr.y))},
        {"width", MemeValue(static_cast<double>(hr.width))},
        {"height", MemeValue(static_cast<double>(hr.height))},
        {"disabled", MemeValue(hr.disabled)},
        {"focused", MemeValue(hr.focused)},
        {"hovered", MemeValue(hr.hovered)},
        {"checked", MemeValue(hr.checked)},
        {"indeterminate", MemeValue(hr.indeterminate)},
        {"selected", MemeValue(hr.selected)},
        {"expanded", MemeValue(hr.expanded)},
        {"value", MemeValue(hr.inputValue)},
        {"numValue", MemeValue(hr.numValue)},
        {"placeholder", MemeValue(hr.placeholder)},
        {"isPassword", MemeValue(hr.isPassword)},
        {"isMultiline", MemeValue(hr.isMultiline)},
        {"scrollX", MemeValue(static_cast<double>(hr.scrollX))},
        {"maxScrollX", MemeValue(static_cast<double>(hr.maxScrollX))},
        {"scrollY", MemeValue(static_cast<double>(hr.scrollY))},
        {"maxScrollY", MemeValue(static_cast<double>(hr.maxScrollY))},
        {"isScrollContainer", MemeValue(hr.isScrollContainer)},
        {"isRtl", MemeValue(hr.isRtl)}
    });
}

inline MemeValue build_primitives_module() {
    auto parseStrList = [](const MemeValue &v) {
        std::vector<std::string> out;
        if (v.type == MemeValue::TYPE_LIST && v.list_data) {
            for (const auto &e : *v.list_data) out.push_back(e.to_string());
        }
        return out;
    };
    auto strVecToVal = [](const std::vector<std::string> &v) {
        auto l = std::make_shared<std::vector<MemeValue>>();
        for (const auto &s : v) l->push_back(MemeValue(s));
        return MemeValue(l);
    };
    auto parseRoving = [](const MemeValue &v) {
        std::vector<MemeUI::RovingItem> out;
        if (v.type == MemeValue::TYPE_LIST && v.list_data) {
            for (const auto &el : *v.list_data) {
                MemeUI::RovingItem ri;
                if (el.type == MemeValue::TYPE_MAP) {
                    ri.id = el.has("id") ? el.get("id").to_string() : el.get("value", MemeValue("")).to_string();
                    ri.disabled = el.get("disabled", MemeValue(false)).is_truthy();
                } else {
                    ri.id = el.to_string();
                }
                out.push_back(ri);
            }
        }
        return out;
    };
    auto parseTypeahead = [](const MemeValue &v) {
        std::vector<MemeUI::TypeaheadItem> out;
        if (v.type == MemeValue::TYPE_LIST && v.list_data) {
            for (const auto &el : *v.list_data) {
                MemeUI::TypeaheadItem ti;
                if (el.type == MemeValue::TYPE_MAP) {
                    ti.id = el.get("id", MemeValue("")).to_string();
                    ti.text = el.has("text") ? el.get("text").to_string() : (el.has("label") ? el.get("label").to_string() : ti.id);
                    if (ti.id.empty()) ti.id = ti.text;
                    ti.disabled = el.get("disabled", MemeValue(false)).is_truthy();
                } else {
                    ti.id = ti.text = el.to_string();
                }
                out.push_back(ti);
            }
        }
        return out;
    };

    MemeValue m = make_map({});

    m["mergeProps"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue {
        return merge_prop_maps(a);
    });

    m["createParts"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue {
        MemeValue out = make_map({});
        if (a.empty() || a[0].type != MemeValue::TYPE_MAP || !a[0].map_data) return out;
        for (const auto &k : a[0].map_data->map_keys) {
            MemeValue spec = a[0].get(k);
            out[k] = meme_fn([spec](const std::vector<MemeValue> &pa) -> MemeValue {
                MemeValue up = (!pa.empty()) ? pa[0] : make_map({});
                MemeValue bp = spec.is_callable() ? spec.call_args({up}) : spec;
                return merge_prop_maps({bp, up});
            });
        }
        return out;
    });

    m["definePrimitive"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue {
        MemeValue spec = a.empty() ? make_map({}) : a[0];
        return meme_fn([spec](const std::vector<MemeValue> &fa) -> MemeValue {
            MemeValue inst = make_map({});
            MemeValue uo = !fa.empty() ? fa[0] : make_map({});
            MemeValue merged = merge_prop_maps({spec.get("defaults", make_map({})), uo});
            inst["options"] = merged;
            MemeValue parts = spec.get("parts");
            if (parts.type == MemeValue::TYPE_MAP && parts.map_data) {
                MemeValue pMap = make_map({});
                for (const auto &pk : parts.map_data->map_keys) {
                    MemeValue pv = parts.get(pk);
                    pMap[pk] = meme_fn([pv, merged](const std::vector<MemeValue> &pa) -> MemeValue {
                        MemeValue up = !pa.empty() ? pa[0] : make_map({});
                        MemeValue bp = pv.is_callable() ? pv.call_args({merged, up}) : pv;
                        return merge_prop_maps({bp, up});
                    });
                }
                inst["parts"] = pMap;
            }
            MemeValue setup = spec.get("setup");
            if (setup.is_callable()) {
                MemeValue extra = setup.call_args({merged, inst});
                if (extra.type == MemeValue::TYPE_MAP && extra.map_data) {
                    for (const auto &ek : extra.map_data->map_keys) inst[ek] = extra.get(ek);
                }
            }
            return inst;
        });
    });

    m["createPopup"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue {
        struct Ctrl {
            std::string id = "popup";
            bool modal = false, closeOnEsc = true, closeOnOut = true;
            std::string restoreId = "";
            MemeValue onOpenChange;
            MemeUI::UiRect triggerBounds{0, 0, 0, 0}, bounds{0, 0, 200, 120};
        };
        auto c = std::make_shared<Ctrl>();
        bool initOpen = false;
        if (!a.empty() && a[0].type == MemeValue::TYPE_MAP) {
            auto &om = a[0];
            if (om.has("id")) c->id = om.get("id").to_string();
            if (om.has("modal")) c->modal = om.get("modal").is_truthy();
            if (om.has("closeOnEscape")) c->closeOnEsc = om.get("closeOnEscape").is_truthy();
            if (om.has("closeOnOutsideClick")) c->closeOnOut = om.get("closeOnOutsideClick").is_truthy();
            if (om.has("restoreFocusToId")) c->restoreId = om.get("restoreFocusToId").to_string();
            if (om.has("onOpenChange")) c->onOpenChange = om.get("onOpenChange");
            if (om.has("defaultOpen") || om.has("open")) initOpen = om.get("defaultOpen", om.get("open", MemeValue(false))).is_truthy();
        }
        auto syncOpen = [c](bool nextOpen) {
            auto &rt = aot_ui();
            bool cur = rt.session.popups.isOpen(c->id);
            if (nextOpen && !cur) {
                MemeUI::PopupEntry pe;
                pe.id = c->id; pe.modal = c->modal; pe.closeOnEscape = c->closeOnEsc;
                pe.closeOnOutsideClick = c->closeOnOut;
                pe.restoreFocusId = c->restoreId.empty() ? rt.session.interaction.focusedKey : c->restoreId;
                pe.triggerBounds = c->triggerBounds; pe.bounds = c->bounds;
                pe.onDismiss = [c]() { if (c->onOpenChange.is_callable()) c->onOpenChange.call_args({MemeValue(false)}); };
                rt.session.popups.open(pe);
                if (c->onOpenChange.is_callable()) c->onOpenChange.call_args({MemeValue(true)});
            } else if (!nextOpen && cur) {
                std::string r = rt.session.popups.closeById(c->id);
                if (!r.empty()) rt.session.interaction.focusedKey = r;
            }
            rt.session.needsRender = true;
            rt.session.version++;
        };
        if (initOpen) syncOpen(true);

        MemeValue obj = make_map({});
        obj["id"] = MemeValue(c->id);
        obj["isOpen"] = meme_fn([c](const std::vector<MemeValue> &) -> MemeValue { return MemeValue(aot_ui().session.popups.isOpen(c->id)); });
        obj["open"] = meme_fn([c, syncOpen](const std::vector<MemeValue> &oa) -> MemeValue {
            if (!oa.empty() && oa[0].type == MemeValue::TYPE_MAP) {
                if (oa[0].has("triggerBounds")) {
                    auto tb = oa[0].get("triggerBounds");
                    c->triggerBounds = {static_cast<float>(tb.get("x", 0.0).as_double()), static_cast<float>(tb.get("y", 0.0).as_double()),
                                        static_cast<float>(tb.get("width", 0.0).as_double()), static_cast<float>(tb.get("height", 0.0).as_double())};
                }
                if (oa[0].has("bounds")) {
                    auto b = oa[0].get("bounds");
                    c->bounds = {static_cast<float>(b.get("x", 0.0).as_double()), static_cast<float>(b.get("y", 0.0).as_double()),
                                 static_cast<float>(b.get("width", 200.0).as_double()), static_cast<float>(b.get("height", 120.0).as_double())};
                }
            }
            syncOpen(true); return MemeValue(true);
        });
        obj["close"] = meme_fn([syncOpen](const std::vector<MemeValue> &) -> MemeValue { syncOpen(false); return MemeValue(false); });
        obj["toggle"] = meme_fn([c, syncOpen](const std::vector<MemeValue> &) -> MemeValue {
            bool n = !aot_ui().session.popups.isOpen(c->id); syncOpen(n); return MemeValue(n);
        });
        obj["handleEscape"] = meme_fn([](const std::vector<MemeValue> &) -> MemeValue {
            std::string r; bool closed = aot_ui().session.popups.handleEscape(r);
            if (!r.empty()) aot_ui().session.interaction.focusedKey = r;
            return MemeValue(closed);
        });
        obj["handlePointerDown"] = meme_fn([](const std::vector<MemeValue> &pa) -> MemeValue {
            if (pa.size() < 2) return MemeValue(false);
            std::string r; bool closed = aot_ui().session.popups.handlePointerDown(static_cast<float>(pa[0].as_double()), static_cast<float>(pa[1].as_double()), r);
            if (!r.empty()) aot_ui().session.interaction.focusedKey = r;
            return MemeValue(closed);
        });
        obj["computePlacement"] = meme_fn([](const std::vector<MemeValue> &pa) -> MemeValue {
            if (pa.empty() || pa[0].type != MemeValue::TYPE_MAP) return make_map({});
            auto &om = pa[0];
            auto rr = [&](const std::string &k, MemeUI::UiRect def) -> MemeUI::UiRect {
                if (!om.has(k)) return def;
                auto r = om.get(k);
                return {static_cast<float>(r.get("x", MemeValue(def.x)).as_double()), static_cast<float>(r.get("y", MemeValue(def.y)).as_double()),
                        static_cast<float>(r.get("width", MemeValue(def.width)).as_double()), static_cast<float>(r.get("height", MemeValue(def.height)).as_double())};
            };
            MemeUI::PopupPlacementOptions po;
            std::string s = om.get("side", MemeValue("bottom")).to_string();
            if (s == "top") po.side = MemeUI::PopupSide::TOP; else if (s == "left") po.side = MemeUI::PopupSide::LEFT; else if (s == "right") po.side = MemeUI::PopupSide::RIGHT;
            std::string al = om.get("align", MemeValue("start")).to_string();
            if (al == "center") po.align = MemeUI::PopupAlign::CENTER; else if (al == "end") po.align = MemeUI::PopupAlign::END;
            if (om.has("sideOffset")) po.sideOffset = static_cast<float>(om.get("sideOffset").as_double());
            if (om.has("alignOffset")) po.alignOffset = static_cast<float>(om.get("alignOffset").as_double());
            if (om.has("padding")) po.viewportPadding = static_cast<float>(om.get("padding").as_double());
            if (om.has("avoidCollisions")) po.avoidCollisions = om.get("avoidCollisions").is_truthy();
            if (om.has("flip")) po.flip = om.get("flip").is_truthy();
            if (om.has("shift")) po.shift = om.get("shift").is_truthy();
            if (om.has("rtl")) po.rtl = om.get("rtl").is_truthy();
            auto res = MemeUI::computePopupPlacement(rr("anchor", {0, 0, 100, 32}),
                                                     static_cast<float>(om.get("popupWidth", MemeValue(200.0)).as_double()),
                                                     static_cast<float>(om.get("popupHeight", MemeValue(120.0)).as_double()),
                                                     rr("viewport", {0, 0, aot_ui().session.input.viewportW, aot_ui().session.input.viewportH}), po);
            return make_map({
                {"x", MemeValue(static_cast<double>(res.x))}, {"y", MemeValue(static_cast<double>(res.y))},
                {"side", MemeValue(res.actualSide == MemeUI::PopupSide::TOP ? "top" : (res.actualSide == MemeUI::PopupSide::LEFT ? "left" : (res.actualSide == MemeUI::PopupSide::RIGHT ? "right" : "bottom")))},
                {"align", MemeValue(res.actualAlign == MemeUI::PopupAlign::CENTER ? "center" : (res.actualAlign == MemeUI::PopupAlign::END ? "end" : "start"))},
                {"flipped", MemeValue(res.flipped)}, {"shifted", MemeValue(res.shifted)}
            });
        });
        obj["getTriggerProps"] = meme_fn([c, syncOpen](const std::vector<MemeValue> &pa) -> MemeValue {
            MemeValue base = make_map({
                {"aria-expanded", MemeValue(aot_ui().session.popups.isOpen(c->id))},
                {"aria-haspopup", MemeValue(c->modal ? "dialog" : "menu")},
                {"onClick", meme_fn([c, syncOpen](const std::vector<MemeValue> &) -> MemeValue {
                    bool n = !aot_ui().session.popups.isOpen(c->id); syncOpen(n); return MemeValue(n);
                })}
            });
            return merge_prop_maps({base, !pa.empty() ? pa[0] : MemeValue()});
        });
        obj["getContentProps"] = meme_fn([c](const std::vector<MemeValue> &pa) -> MemeValue {
            MemeValue base = make_map({
                {"role", MemeValue(c->modal ? "dialog" : "menu")},
                {"open", MemeValue(aot_ui().session.popups.isOpen(c->id))},
                {"portal", MemeValue(true)}
            });
            return merge_prop_maps({base, !pa.empty() ? pa[0] : MemeValue()});
        });
        return obj;
    });

    m["createRovingFocus"] = meme_fn([parseRoving](const std::vector<MemeValue> &a) -> MemeValue {
        auto st = std::make_shared<MemeUI::RovingFocusState>();
        if (!a.empty() && a[0].type == MemeValue::TYPE_MAP) {
            if (a[0].has("items")) st->items = parseRoving(a[0].get("items"));
            if (a[0].has("activeId")) st->activeId = a[0].get("activeId").to_string();
            if (a[0].has("orientation")) st->orientation = a[0].get("orientation").to_string();
            if (a[0].has("columns")) st->columns = static_cast<int>(a[0].get("columns").as_int());
            if (a[0].has("loop")) st->loop = a[0].get("loop").is_truthy();
            if (a[0].has("rtl")) st->rtl = a[0].get("rtl").is_truthy();
        }
        st->ensureValidActive();
        MemeValue obj = make_map({});
        obj["setItems"] = meme_fn([st, parseRoving](const std::vector<MemeValue> &x) -> MemeValue { st->items = parseRoving(x[0]); st->ensureValidActive(); return MemeValue(st->activeId); });
        obj["handleKey"] = meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue { std::string n; return st->handleKey(x[0].to_string(), n) ? MemeValue(n) : MemeValue(); });
        obj["getActiveId"] = meme_fn([st](const std::vector<MemeValue> &) -> MemeValue { return MemeValue(st->activeId); });
        obj["setActiveId"] = meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue { st->activeId = x[0].to_string(); st->ensureValidActive(); return MemeValue(st->activeId); });
        obj["getTabIndex"] = meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue { return MemeValue(static_cast<int64_t>(st->getTabIndex(x[0].to_string()))); });
        obj["getItemProps"] = meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue {
            std::string id = x[0].to_string();
            MemeValue p = make_map({{"tabIndex", MemeValue(static_cast<int64_t>(st->getTabIndex(id)))}});
            return merge_prop_maps({p, x.size() > 1 ? x[1] : MemeValue()});
        });
        return obj;
    });

    auto makeScope = [parseStrList](bool trapped) {
        return meme_fn([parseStrList, trapped](const std::vector<MemeValue> &a) -> MemeValue {
            auto sc = std::make_shared<MemeUI::FocusScope>();
            sc->trapped = trapped;
            sc->id = trapped ? "focus-trap" : "focus-scope";
            if (!a.empty() && a[0].type == MemeValue::TYPE_MAP) {
                if (a[0].has("id")) sc->id = a[0].get("id").to_string();
                if (a[0].has("containerId")) sc->containerId = a[0].get("containerId").to_string();
                if (a[0].has("focusableIds")) sc->focusableIds = parseStrList(a[0].get("focusableIds"));
                if (a[0].has("initialFocusId")) sc->initialFocusId = a[0].get("initialFocusId").to_string();
                if (a[0].has("restoreFocusToId")) sc->restoreFocusToId = a[0].get("restoreFocusToId").to_string();
                if (a[0].has("trapped")) sc->trapped = a[0].get("trapped").is_truthy();
            }
            return make_map({
                {"mount", meme_fn([sc](const std::vector<MemeValue> &x) -> MemeValue {
                    auto &rt = aot_ui();
                    std::string cur = !x.empty() ? x[0].to_string() : rt.session.interaction.focusedKey;
                    std::string tgt = rt.session.focusScopes.mountScope(*sc, cur);
                    rt.session.interaction.focusedKey = tgt;
                    return MemeValue(tgt);
                })},
                {"unmount", meme_fn([sc](const std::vector<MemeValue> &) -> MemeValue {
                    auto &rt = aot_ui();
                    std::string r = rt.session.focusScopes.unmountScope(sc->id);
                    if (!r.empty()) rt.session.interaction.focusedKey = r;
                    return MemeValue(r);
                })},
                {"handleTab", meme_fn([](const std::vector<MemeValue> &x) -> MemeValue {
                    auto &rt = aot_ui();
                    bool shift = !x.empty() && x[0].is_truthy();
                    std::string cur = (x.size() > 1) ? x[1].to_string() : rt.session.interaction.focusedKey;
                    std::string next;
                    if (rt.session.focusScopes.handleTab(shift, cur, next)) {
                        rt.session.interaction.focusedKey = next;
                        return MemeValue(next);
                    }
                    return MemeValue();
                })},
                {"isActive", meme_fn([sc](const std::vector<MemeValue> &) -> MemeValue {
                    const auto *t = aot_ui().session.focusScopes.activeTrap();
                    return MemeValue(t && t->id == sc->id);
                })}
            });
        });
    };
    m["createFocusTrap"] = makeScope(true);
    m["createFocusScope"] = makeScope(false);

    m["createSelection"] = meme_fn([parseStrList, strVecToVal](const std::vector<MemeValue> &a) -> MemeValue {
        auto st = std::make_shared<MemeUI::SelectionState>();
        if (!a.empty() && a[0].type == MemeValue::TYPE_MAP) {
            if (a[0].has("mode")) st->mode = a[0].get("mode").to_string();
            if (a[0].has("disallowEmpty")) st->disallowEmpty = a[0].get("disallowEmpty").is_truthy();
            if (a[0].has("selectedIds")) st->selectedIds = parseStrList(a[0].get("selectedIds"));
            if (a[0].has("disabledIds")) for (const auto &d : parseStrList(a[0].get("disabledIds"))) st->disabledIds.insert(d);
        }
        return make_map({
            {"isSelected", meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue { return MemeValue(st->isSelected(x[0].to_string())); })},
            {"select", meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue { return MemeValue(st->select(x[0].to_string())); })},
            {"deselect", meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue { return MemeValue(st->deselect(x[0].to_string())); })},
            {"toggle", meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue { return MemeValue(st->toggle(x[0].to_string())); })},
            {"selectRange", meme_fn([st, parseStrList](const std::vector<MemeValue> &x) -> MemeValue { return MemeValue(st->selectRange(x[0].to_string(), parseStrList(x[1]))); })},
            {"selectAll", meme_fn([st, parseStrList](const std::vector<MemeValue> &x) -> MemeValue { return MemeValue(st->selectAll(parseStrList(x[0]))); })},
            {"clear", meme_fn([st](const std::vector<MemeValue> &) -> MemeValue { return MemeValue(st->clear()); })},
            {"getSelected", meme_fn([st, strVecToVal](const std::vector<MemeValue> &) -> MemeValue { return strVecToVal(st->selectedIds); })}
        });
    });

    m["createTypeahead"] = meme_fn([parseTypeahead](const std::vector<MemeValue> &a) -> MemeValue {
        auto st = std::make_shared<MemeUI::TypeaheadState>();
        auto items = std::make_shared<std::vector<MemeUI::TypeaheadItem>>();
        if (!a.empty() && a[0].type == MemeValue::TYPE_MAP) {
            if (a[0].has("items")) *items = parseTypeahead(a[0].get("items"));
            if (a[0].has("timeoutMs")) st->timeoutMs = a[0].get("timeoutMs").as_double();
        }
        return make_map({
            {"setItems", meme_fn([items, parseTypeahead](const std::vector<MemeValue> &x) -> MemeValue { *items = parseTypeahead(x[0]); return MemeValue(true); })},
            {"handleChar", meme_fn([st, items](const std::vector<MemeValue> &x) -> MemeValue {
                return MemeValue(st->handleChar(x[0].to_string(), x.size() > 1 ? x[1].as_double() : 0.0, *items, x.size() > 2 ? x[2].to_string() : ""));
            })},
            {"matchQuery", meme_fn([st, items](const std::vector<MemeValue> &x) -> MemeValue {
                return MemeValue(st->matchQuery(x.empty() ? "" : x[0].to_string(), *items, x.size() > 1 ? x[1].to_string() : ""));
            })},
            {"reset", meme_fn([st](const std::vector<MemeValue> &) -> MemeValue { st->reset(); return MemeValue(true); })},
            {"getBuffer", meme_fn([st](const std::vector<MemeValue> &) -> MemeValue { return MemeValue(st->buffer); })}
        });
    });

    m["createVirtualizer"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue {
        auto st = std::make_shared<MemeUI::VirtualizerState>();
        int cnt = 0; float est = 44.0f;
        if (!a.empty() && a[0].type == MemeValue::TYPE_MAP) {
            if (a[0].has("count")) cnt = static_cast<int>(a[0].get("count").as_int());
            else if (a[0].has("itemCount")) cnt = static_cast<int>(a[0].get("itemCount").as_int());
            if (a[0].has("estimateSize")) est = static_cast<float>(a[0].get("estimateSize").as_double());
            if (a[0].has("overscan")) st->overscan = static_cast<int>(a[0].get("overscan").as_int());
            if (a[0].has("viewportSize")) st->viewportSize = static_cast<float>(a[0].get("viewportSize").as_double());
            MemeValue si = a[0].get("stickyIndices");
            if (si.type == MemeValue::TYPE_LIST && si.list_data) for (const auto &v : *si.list_data) st->stickyIndices.push_back(static_cast<int>(v.as_int()));
        }
        st->setCount(cnt, est);
        return make_map({
            {"setCount", meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue { st->setCount(static_cast<int>(x[0].as_int()), x.size() > 1 ? static_cast<float>(x[1].as_double()) : st->estimateSize); return MemeValue(static_cast<double>(st->totalSize())); })},
            {"measureItem", meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue { st->measureItem(static_cast<int>(x[0].as_int()), static_cast<float>(x[1].as_double())); return MemeValue(static_cast<double>(st->totalSize())); })},
            {"setScrollOffset", meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue { st->scrollOffset = std::max(0.0f, static_cast<float>(x[0].as_double())); return MemeValue(static_cast<double>(st->scrollOffset)); })},
            {"setViewportSize", meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue { st->viewportSize = std::max(1.0f, static_cast<float>(x[0].as_double())); return MemeValue(static_cast<double>(st->viewportSize)); })},
            {"scrollToIndex", meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue { return MemeValue(static_cast<double>(st->scrollToIndex(static_cast<int>(x[0].as_int()), x.size() > 1 ? x[1].to_string() : "auto"))); })},
            {"getTotalSize", meme_fn([st](const std::vector<MemeValue> &) -> MemeValue { return MemeValue(static_cast<double>(st->totalSize())); })},
            {"offsetOf", meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue { return MemeValue(static_cast<double>(st->offsetOf(static_cast<int>(x[0].as_int())))); })},
            {"getStickyIndex", meme_fn([st](const std::vector<MemeValue> &) -> MemeValue { return MemeValue(static_cast<int64_t>(st->computeSnapshot().stickyIndex)); })},
            {"getVirtualItems", meme_fn([st](const std::vector<MemeValue> &) -> MemeValue {
                auto snap = st->computeSnapshot();
                auto list = std::make_shared<std::vector<MemeValue>>();
                for (const auto &vi : snap.visibleItems) {
                    list->push_back(make_map({{"index", MemeValue(static_cast<int64_t>(vi.index))}, {"offset", MemeValue(static_cast<double>(vi.offset))}, {"size", MemeValue(static_cast<double>(vi.size))}}));
                }
                return MemeValue(list);
            })}
        });
    });

    m["createReorder"] = meme_fn([parseStrList, strVecToVal](const std::vector<MemeValue> &a) -> MemeValue {
        auto st = std::make_shared<MemeUI::ReorderState>();
        if (!a.empty() && a[0].type == MemeValue::TYPE_MAP && a[0].has("items")) st->items = parseStrList(a[0].get("items"));
        return make_map({
            {"setItems", meme_fn([st, parseStrList](const std::vector<MemeValue> &x) -> MemeValue { st->items = parseStrList(x[0]); return MemeValue(true); })},
            {"getItems", meme_fn([st, strVecToVal](const std::vector<MemeValue> &) -> MemeValue { return strVecToVal(st->items); })},
            {"startDrag", meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue { return MemeValue(st->startDrag(static_cast<int>(x[0].as_int()), static_cast<float>(x[1].as_double()))); })},
            {"updateDrag", meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue {
                std::vector<float> mids;
                if (x[1].type == MemeValue::TYPE_LIST && x[1].list_data) for (const auto &e : *x[1].list_data) mids.push_back(static_cast<float>(e.as_double()));
                return MemeValue(st->updateDrag(static_cast<float>(x[0].as_double()), mids));
            })},
            {"endDrag", meme_fn([st, strVecToVal](const std::vector<MemeValue> &) -> MemeValue { st->endDrag(); return strVecToVal(st->items); })},
            {"handleKey", meme_fn([st, strVecToVal](const std::vector<MemeValue> &x) -> MemeValue {
                int nf = 0; bool h = st->handleKey(x[0].to_string(), static_cast<int>(x[1].as_int()), nf);
                return make_map({{"handled", MemeValue(h)}, {"focusedIndex", MemeValue(static_cast<int64_t>(nf))}, {"grabbedIndex", MemeValue(static_cast<int64_t>(st->grabbedIndex))}, {"announcement", MemeValue(st->announcement)}, {"items", strVecToVal(st->items)}});
            })},
            {"cancel", meme_fn([st](const std::vector<MemeValue> &) -> MemeValue { return MemeValue(st->cancel()); })},
            {"getGrabbedIndex", meme_fn([st](const std::vector<MemeValue> &) -> MemeValue { return MemeValue(static_cast<int64_t>(st->grabbedIndex)); })},
            {"getAnnouncement", meme_fn([st](const std::vector<MemeValue> &) -> MemeValue { return MemeValue(st->announcement); })}
        });
    });

    m["createResize"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue {
        auto st = std::make_shared<MemeUI::ResizeState>();
        auto pf = [](const MemeValue &v) {
            std::vector<float> out;
            if (v.type == MemeValue::TYPE_LIST && v.list_data) for (const auto &e : *v.list_data) out.push_back(static_cast<float>(e.as_double()));
            return out;
        };
        if (!a.empty() && a[0].type == MemeValue::TYPE_MAP) {
            if (a[0].has("sizes")) st->sizes = pf(a[0].get("sizes"));
            if (a[0].has("minSizes")) st->minSizes = pf(a[0].get("minSizes"));
            if (a[0].has("maxSizes")) st->maxSizes = pf(a[0].get("maxSizes"));
            if (a[0].has("collapsible")) {
                st->collapsible.clear();
                MemeValue c = a[0].get("collapsible");
                if (c.type == MemeValue::TYPE_LIST && c.list_data) for (const auto &e : *c.list_data) st->collapsible.push_back(e.is_truthy());
            }
            if (a[0].has("step")) st->step = static_cast<float>(a[0].get("step").as_double());
            if (a[0].has("coarseStep")) st->coarseStep = static_cast<float>(a[0].get("coarseStep").as_double());
            if (a[0].has("vertical")) st->vertical = a[0].get("vertical").is_truthy();
        }
        return make_map({
            {"getSizes", meme_fn([st](const std::vector<MemeValue> &) -> MemeValue {
                auto l = std::make_shared<std::vector<MemeValue>>();
                for (float s : st->sizes) l->push_back(MemeValue(static_cast<double>(s)));
                return MemeValue(l);
            })},
            {"resizeHandle", meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue { return MemeValue(st->resizeHandle(static_cast<int>(x[0].as_int()), static_cast<float>(x[1].as_double()))); })},
            {"handleKey", meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue { return MemeValue(st->handleKey(static_cast<int>(x[0].as_int()), x[1].to_string(), x.size() > 2 && x[2].is_truthy())); })}
        });
    });

    m["createDisclosure"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue {
        struct DS { bool open = false; MemeValue onOpenChange; };
        auto st = std::make_shared<DS>();
        if (!a.empty() && a[0].type == MemeValue::TYPE_MAP) {
            st->open = a[0].get("defaultOpen", a[0].get("open", MemeValue(false))).is_truthy();
            st->onOpenChange = a[0].get("onOpenChange");
        }
        auto setO = [st](bool v) { if (st->open == v) return v; st->open = v; if (st->onOpenChange.is_callable()) st->onOpenChange.call_args({MemeValue(v)}); return v; };
        return make_map({
            {"isOpen", meme_fn([st](const std::vector<MemeValue> &) -> MemeValue { return MemeValue(st->open); })},
            {"open", meme_fn([setO](const std::vector<MemeValue> &) -> MemeValue { return MemeValue(setO(true)); })},
            {"close", meme_fn([setO](const std::vector<MemeValue> &) -> MemeValue { return MemeValue(setO(false)); })},
            {"toggle", meme_fn([st, setO](const std::vector<MemeValue> &) -> MemeValue { return MemeValue(setO(!st->open)); })},
            {"getTriggerProps", meme_fn([st, setO](const std::vector<MemeValue> &pa) -> MemeValue {
                MemeValue base = make_map({{"aria-expanded", MemeValue(st->open)}, {"onClick", meme_fn([st, setO](const std::vector<MemeValue> &) -> MemeValue { return MemeValue(setO(!st->open)); })}});
                return merge_prop_maps({base, !pa.empty() ? pa[0] : MemeValue()});
            })},
            {"getContentProps", meme_fn([st](const std::vector<MemeValue> &pa) -> MemeValue {
                MemeValue base = make_map({{"hidden", MemeValue(!st->open)}, {"open", MemeValue(st->open)}});
                return merge_prop_maps({base, !pa.empty() ? pa[0] : MemeValue()});
            })}
        });
    });

    m["createTabs"] = meme_fn([parseRoving](const std::vector<MemeValue> &a) -> MemeValue {
        auto st = std::make_shared<MemeUI::TabsState>();
        if (!a.empty() && a[0].type == MemeValue::TYPE_MAP) {
            if (a[0].has("orientation")) st->roving.orientation = a[0].get("orientation").to_string();
            if (a[0].has("activationMode")) st->activationMode = a[0].get("activationMode").to_string();
            if (a[0].has("rtl")) st->roving.rtl = a[0].get("rtl").is_truthy();
            if (a[0].has("loop")) st->roving.loop = a[0].get("loop").is_truthy();
            if (a[0].has("items")) st->syncItems(parseRoving(a[0].get("items")));
            std::string dv = a[0].get("value", a[0].get("defaultValue", MemeValue(""))).to_string();
            if (!dv.empty()) st->clickTab(dv);
        }
        return make_map({
            {"setItems", meme_fn([st, parseRoving](const std::vector<MemeValue> &x) -> MemeValue { st->syncItems(parseRoving(x[0])); return MemeValue(st->activeTab()); })},
            {"activeTab", meme_fn([st](const std::vector<MemeValue> &) -> MemeValue { return MemeValue(st->activeTab()); })},
            {"clickTab", meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue { return MemeValue(st->clickTab(x[0].to_string())); })},
            {"handleKey", meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue {
                std::string f; st->handleKey(x[0].to_string(), f);
                return make_map({{"activeTab", MemeValue(st->activeTab())}, {"focusedTab", MemeValue(f)}});
            })},
            {"getTabProps", meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue {
                std::string id = x[0].to_string();
                MemeValue base = make_map({
                    {"role", MemeValue("tab")},
                    {"selected", MemeValue(st->activeTab() == id)},
                    {"active", MemeValue(st->activeTab() == id)},
                    {"tabIndex", MemeValue(static_cast<int64_t>(st->roving.getTabIndex(id)))},
                    {"onClick", meme_fn([st, id](const std::vector<MemeValue> &) -> MemeValue { return MemeValue(st->clickTab(id)); })}
                });
                return merge_prop_maps({base, x.size() > 1 ? x[1] : MemeValue()});
            })},
            {"getPanelProps", meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue {
                std::string id = x[0].to_string();
                MemeValue base = make_map({{"role", MemeValue("tabpanel")}, {"hidden", MemeValue(st->activeTab() != id)}});
                return merge_prop_maps({base, x.size() > 1 ? x[1] : MemeValue()});
            })}
        });
    });

    m["createSlider"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue {
        auto st = std::make_shared<MemeUI::SliderState>();
        if (!a.empty() && a[0].type == MemeValue::TYPE_MAP) {
            if (a[0].has("min")) st->min = a[0].get("min").as_double();
            if (a[0].has("max")) st->max = a[0].get("max").as_double();
            if (a[0].has("step")) st->step = a[0].get("step").as_double();
            if (a[0].has("minStepsBetweenThumbs")) st->minStepsBetweenThumbs = static_cast<int>(a[0].get("minStepsBetweenThumbs").as_int());
            if (a[0].has("vertical")) st->vertical = a[0].get("vertical").is_truthy();
            if (a[0].has("rtl")) st->rtl = a[0].get("rtl").is_truthy();
            if (a[0].has("disabled")) st->disabled = a[0].get("disabled").is_truthy();
            if (a[0].has("values") && a[0].get("values").type == MemeValue::TYPE_LIST && a[0].get("values").list_data) {
                st->values.clear();
                for (const auto &v : *a[0].get("values").list_data) st->values.push_back(v.as_double());
            } else if (a[0].has("value") || a[0].has("defaultValue")) {
                st->values = {a[0].get("value", a[0].get("defaultValue", MemeValue(50.0))).as_double()};
            }
        }
        return make_map({
            {"getValues", meme_fn([st](const std::vector<MemeValue> &) -> MemeValue {
                auto l = std::make_shared<std::vector<MemeValue>>();
                for (double v : st->values) l->push_back(MemeValue(v));
                return MemeValue(l);
            })},
            {"setValue", meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue {
                int idx = (x.size() == 2) ? static_cast<int>(x[0].as_int()) : 0;
                double val = (x.size() == 2) ? x[1].as_double() : x[0].as_double();
                return MemeValue(st->setThumbValue(idx, val));
            })},
            {"setFromRatio", meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue { return MemeValue(st->setFromRatio(static_cast<int>(x[0].as_int()), x[1].as_double())); })},
            {"handleKey", meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue { return MemeValue(st->handleKey(static_cast<int>(x[0].as_int()), x[1].to_string(), x.size() > 2 && x[2].is_truthy())); })}
        });
    });

    m["createToast"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue {
        auto local = std::make_shared<MemeUI::ToastManager>();
        bool iso = !a.empty() && a[0].type == MemeValue::TYPE_MAP && a[0].get("isolated", MemeValue(false)).is_truthy();
        if (!a.empty() && a[0].type == MemeValue::TYPE_MAP && a[0].has("maxVisible")) local->maxVisible = static_cast<int>(a[0].get("maxVisible").as_int());
        auto gm = [local, iso]() -> MemeUI::ToastManager & { return iso ? *local : aot_ui().session.toasts; };
        return make_map({
            {"add", meme_fn([gm](const std::vector<MemeValue> &x) -> MemeValue {
                return MemeValue(gm().add(x[0].to_string(), x.size() > 1 ? x[1].to_string() : "", x.size() > 2 ? x[2].to_string() : "default", x.size() > 3 ? x[3].as_double() : 4000.0));
            })},
            {"dismiss", meme_fn([gm](const std::vector<MemeValue> &x) -> MemeValue { gm().dismiss(x[0].to_string()); return MemeValue(true); })},
            {"dismissAll", meme_fn([gm](const std::vector<MemeValue> &) -> MemeValue { gm().dismissAll(); return MemeValue(true); })},
            {"setPaused", meme_fn([gm](const std::vector<MemeValue> &x) -> MemeValue { gm().setPaused(x[0].to_string(), x[1].is_truthy()); return MemeValue(true); })},
            {"pauseAll", meme_fn([gm](const std::vector<MemeValue> &x) -> MemeValue { gm().pauseAll(x[0].is_truthy()); return MemeValue(true); })},
            {"tick", meme_fn([gm](const std::vector<MemeValue> &x) -> MemeValue { return MemeValue(gm().tick(x[0].as_double())); })},
            {"list", meme_fn([gm](const std::vector<MemeValue> &) -> MemeValue {
                auto l = std::make_shared<std::vector<MemeValue>>();
                for (const auto &t : gm().toasts) l->push_back(make_map({{"id", MemeValue(t.id)}, {"title", MemeValue(t.title)}, {"description", MemeValue(t.description)}, {"variant", MemeValue(t.variant)}, {"remainingMs", MemeValue(t.remainingMs)}, {"paused", MemeValue(t.paused)}}));
                return MemeValue(l);
            })}
        });
    });

    m["createTooltip"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue {
        auto st = std::make_shared<MemeUI::TooltipDelayGroup>();
        if (!a.empty() && a[0].type == MemeValue::TYPE_MAP) {
            if (a[0].has("openDelayMs")) st->openDelayMs = a[0].get("openDelayMs").as_double();
            if (a[0].has("skipDelayMs")) st->skipDelayMs = a[0].get("skipDelayMs").as_double();
        }
        return make_map({
            {"onTriggerHoverStart", meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue { st->onTriggerHoverStart(x[0].to_string()); return MemeValue(st->openTriggerId); })},
            {"onTriggerHoverEnd", meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue { st->onTriggerHoverEnd(x[0].to_string()); return MemeValue(st->openTriggerId); })},
            {"onTriggerFocus", meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue { st->onTriggerFocus(x[0].to_string()); return MemeValue(st->openTriggerId); })},
            {"onTooltipHoverChange", meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue { st->onTooltipHoverChange(x[0].is_truthy()); return MemeValue(st->openTriggerId); })},
            {"handleEscape", meme_fn([st](const std::vector<MemeValue> &) -> MemeValue { return MemeValue(st->handleEscape()); })},
            {"tick", meme_fn([st](const std::vector<MemeValue> &x) -> MemeValue { return MemeValue(st->tick(x[0].as_double())); })},
            {"getOpenTriggerId", meme_fn([st](const std::vector<MemeValue> &) -> MemeValue { return MemeValue(st->openTriggerId); })}
        });
    });

    m["createAgent"] = meme_fn([strVecToVal](const std::vector<MemeValue> &) -> MemeValue {
        return make_map({
            {"snapshotJson", meme_fn([](const std::vector<MemeValue> &) -> MemeValue { return MemeValue(aot_ui().session.agentSnapshotJson()); })},
            {"snapshot", meme_fn([](const std::vector<MemeValue> &) -> MemeValue {
                auto &s = aot_ui().session;
                auto nl = std::make_shared<std::vector<MemeValue>>();
                for (const auto &hr : s.lastHits) nl->push_back(hit_region_to_meme(hr));
                return make_map({{"version", MemeValue(static_cast<int64_t>(s.version))}, {"focusedKey", MemeValue(s.interaction.focusedKey)}, {"openPopups", MemeValue(static_cast<int64_t>(s.popups.stack.size()))}, {"nodes", MemeValue(nl)}});
            })},
            {"query", meme_fn([](const std::vector<MemeValue> &x) -> MemeValue {
                std::string role, name, testId;
                if (!x.empty() && x[0].type == MemeValue::TYPE_MAP) {
                    role = x[0].get("role", MemeValue("")).to_string();
                    name = x[0].get("name", MemeValue("")).to_string();
                    testId = x[0].get("testId", MemeValue("")).to_string();
                } else if (!x.empty()) name = x[0].to_string();
                auto nl = std::make_shared<std::vector<MemeValue>>();
                for (const auto &hr : aot_ui().session.agentQuery(role, name, testId)) nl->push_back(hit_region_to_meme(hr));
                return MemeValue(nl);
            })},
            {"getByRole", meme_fn([](const std::vector<MemeValue> &x) -> MemeValue {
                auto ms = aot_ui().session.agentQuery(x[0].to_string(), x.size() > 1 ? x[1].to_string() : "", "");
                return ms.empty() ? MemeValue() : hit_region_to_meme(ms.front());
            })},
            {"getByText", meme_fn([](const std::vector<MemeValue> &x) -> MemeValue {
                auto ms = aot_ui().session.agentQuery("", x[0].to_string(), "");
                return ms.empty() ? MemeValue() : hit_region_to_meme(ms.front());
            })},
            {"getByTestId", meme_fn([](const std::vector<MemeValue> &x) -> MemeValue {
                auto ms = aot_ui().session.agentQuery("", "", x[0].to_string());
                return ms.empty() ? MemeValue() : hit_region_to_meme(ms.front());
            })},
            {"act", meme_fn([](const std::vector<MemeValue> &x) -> MemeValue {
                auto &rt = aot_ui();
                int targetId = 0; std::string targetKey;
                const auto &tgt = x[0];
                if (tgt.type == MemeValue::TYPE_INT || tgt.type == MemeValue::TYPE_FLOAT) {
                    targetId = static_cast<int>(tgt.as_int());
                } else if (tgt.type == MemeValue::TYPE_MAP && tgt.has("id")) {
                    targetId = static_cast<int>(tgt.get("id").as_int());
                    targetKey = tgt.get("key", MemeValue("")).to_string();
                } else {
                    std::string s = tgt.to_string();
                    for (const auto &hr : rt.session.lastHits) {
                        if (hr.nodeKey == s || hr.testId == s || hr.accessibleName == s) { targetId = hr.id; targetKey = hr.nodeKey; break; }
                    }
                }
                if (targetKey.empty() && targetId > 0) {
                    if (const auto *hr = rt.session.findRegionById(targetId)) targetKey = hr->nodeKey;
                }
                std::string action = x[1].to_string();
                std::string val = x.size() > 2 ? x[2].to_string() : "";
                double numVal = (x.size() > 2 && (x[2].type == MemeValue::TYPE_INT || x[2].type == MemeValue::TYPE_FLOAT)) ? x[2].as_double() : 0.0;
                bool ok = false;
                if (action == "focus") {
                    rt.session.interaction.focusedId = targetId; rt.session.interaction.focusedKey = targetKey; rt.session.interaction.focusVisible = true;
                    ok = !targetKey.empty();
                } else if (action == "keydown" || action == "key") {
                    if (!targetKey.empty()) { rt.session.interaction.focusedId = targetId; rt.session.interaction.focusedKey = targetKey; }
                    MemeUI::UiKeyEvent kev; kev.key = val; if (val.size() == 1) kev.text = val;
                    ok = rt.session.handleKeyEvent(kev);
                } else if (action == "type") {
                    if (!targetKey.empty()) { rt.session.interaction.focusedId = targetId; rt.session.interaction.focusedKey = targetKey; }
                    for (char c : val) { MemeUI::UiKeyEvent kev; kev.text = kev.key = std::string(1, c); ok = rt.session.handleKeyEvent(kev) || ok; }
                } else if (action == "scroll") {
                    if (const auto *hr = rt.session.findRegionById(targetId)) {
                        ok = rt.session.handleScroll(hr->x + 4.0f, hr->y + 4.0f, 0.0f, static_cast<float>(numVal != 0.0 ? numVal : -5.0));
                    }
                } else if (targetId > 0) {
                    ok = rt.session.dispatchEventOnNode(targetId, action, val, numVal);
                }
                rt.triggerUiRerender();
                return MemeValue(ok);
            })},
            {"tick", meme_fn([](const std::vector<MemeValue> &x) -> MemeValue {
                auto &rt = aot_ui();
                bool ch = rt.session.tick(x[0].as_double());
                if (ch || rt.session.needsRender) rt.triggerUiRerender();
                return MemeValue(ch);
            })},
            {"audit", meme_fn([strVecToVal](const std::vector<MemeValue> &) -> MemeValue {
                auto res = aot_ui().session.agentAssertAccessible();
                return make_map({{"valid", MemeValue(res.ok)}, {"violations", strVecToVal(res.violations)}});
            })},
            {"assertAccessible", meme_fn([](const std::vector<MemeValue> &) -> MemeValue {
                auto res = aot_ui().session.agentAssertAccessible();
                if (!res.ok) {
                    std::string msg = "Accessibility audit failed:";
                    for (const auto &v : res.violations) msg += " " + v + ";";
                    throw std::runtime_error(msg);
                }
                return MemeValue(true);
            })}
        });
    });

    return m;
}

inline MemeValue wrap_egui_response(const MemeUI::EguiResponse &r, const MemeValue &val) {
    return make_map({
        {"id", MemeValue(r.id)},
        {"clicked", MemeValue(r.clicked)},
        {"double_clicked", MemeValue(r.doubleClicked)},
        {"doubleClicked", MemeValue(r.doubleClicked)},
        {"hovered", MemeValue(r.hovered)},
        {"active", MemeValue(r.active)},
        {"has_focus", MemeValue(r.hasFocus)},
        {"hasFocus", MemeValue(r.hasFocus)},
        {"changed", MemeValue(r.changed)},
        {"dragged", MemeValue(r.dragged)},
        {"value", val},
        {"rect", make_map({{"x", MemeValue(static_cast<double>(r.rect.x))}, {"y", MemeValue(static_cast<double>(r.rect.y))},
                           {"width", MemeValue(static_cast<double>(r.rect.width))}, {"height", MemeValue(static_cast<double>(r.rect.height))}})}
    });
}

inline MemeValue wrap_egui_ui(MemeUI::EguiUi *ui) {
    MemeValue um = make_map({});
    um["available_width"] = um["availableWidth"] = meme_fn([ui](const std::vector<MemeValue> &) -> MemeValue { return MemeValue(static_cast<double>(ui->availableWidth())); });
    um["available_height"] = um["availableHeight"] = meme_fn([ui](const std::vector<MemeValue> &) -> MemeValue { return MemeValue(static_cast<double>(ui->availableHeight())); });
    um["add_space"] = um["addSpace"] = meme_fn([ui](const std::vector<MemeValue> &a) -> MemeValue { if (!a.empty()) ui->addSpace(static_cast<float>(a[0].as_double())); return MemeValue(); });
    um["separator"] = meme_fn([ui](const std::vector<MemeValue> &) -> MemeValue { ui->separator(); return MemeValue(); });
    um["painter"] = meme_fn([ui](const std::vector<MemeValue> &) -> MemeValue { return AotUiRuntime::wrapUiPainter(ui->painter(), 0, 0, ui->availableWidth(), ui->availableHeight()); });

    um["heading"] = meme_fn([ui](const std::vector<MemeValue> &a) -> MemeValue { auto r = ui->heading(a[0].to_string()); return wrap_egui_response(r, MemeValue(r.clicked)); });
    um["label"] = meme_fn([ui](const std::vector<MemeValue> &a) -> MemeValue { auto r = ui->label(a[0].to_string()); return wrap_egui_response(r, MemeValue(r.clicked)); });
    um["colored_label"] = um["coloredLabel"] = meme_fn([ui](const std::vector<MemeValue> &a) -> MemeValue { auto r = ui->coloredLabel(a[0].to_string(), a[1].to_string()); return wrap_egui_response(r, MemeValue(r.clicked)); });
    um["monospace"] = meme_fn([ui](const std::vector<MemeValue> &a) -> MemeValue { return wrap_egui_response(ui->monospace(a[0].to_string()), MemeValue(false)); });
    um["small"] = meme_fn([ui](const std::vector<MemeValue> &a) -> MemeValue { return wrap_egui_response(ui->small(a[0].to_string()), MemeValue(false)); });
    um["strong"] = meme_fn([ui](const std::vector<MemeValue> &a) -> MemeValue { return wrap_egui_response(ui->strong(a[0].to_string()), MemeValue(false)); });
    um["button"] = meme_fn([ui](const std::vector<MemeValue> &a) -> MemeValue {
        auto r = ui->button(a[0].to_string(), a.size() > 1 ? static_cast<float>(a[1].as_double()) : 0.0f, a.size() > 2 ? static_cast<float>(a[2].as_double()) : 28.0f);
        return wrap_egui_response(r, MemeValue(r.clicked));
    });
    um["small_button"] = um["smallButton"] = meme_fn([ui](const std::vector<MemeValue> &a) -> MemeValue { auto r = ui->smallButton(a[0].to_string()); return wrap_egui_response(r, MemeValue(r.clicked)); });
    um["selectable_label"] = um["selectableLabel"] = meme_fn([ui](const std::vector<MemeValue> &a) -> MemeValue { auto r = ui->selectableLabel(a[0].is_truthy(), a[1].to_string()); return wrap_egui_response(r, MemeValue(r.clicked)); });

    auto readBoolRef = [](MemeValue v, bool &isRef, MemeValue &refM, std::string &rk) {
        if (v.type == MemeValue::TYPE_MAP) {
            refM = v;
            if (v.has("current")) { isRef = true; rk = "current"; return v.get("current").is_truthy(); }
            if (v.has("value")) { isRef = true; rk = "value"; return v.get("value").is_truthy(); }
        }
        return v.is_truthy();
    };
    auto readNumRef = [](MemeValue v, bool &isRef, MemeValue &refM, std::string &rk) {
        if (v.type == MemeValue::TYPE_MAP) {
            refM = v;
            if (v.has("current")) { isRef = true; rk = "current"; return v.get("current").as_double(); }
            if (v.has("value")) { isRef = true; rk = "value"; return v.get("value").as_double(); }
        }
        return v.as_double();
    };
    auto readStrRef = [](MemeValue v, bool &isRef, MemeValue &refM, std::string &rk) {
        if (v.type == MemeValue::TYPE_MAP) {
            refM = v;
            if (v.has("current")) { isRef = true; rk = "current"; return v.get("current").to_string(); }
            if (v.has("value")) { isRef = true; rk = "value"; return v.get("value").to_string(); }
        }
        return v.to_string();
    };

    um["checkbox"] = meme_fn([ui, readBoolRef](const std::vector<MemeValue> &a) -> MemeValue {
        std::string id = (a.size() >= 3) ? a[0].to_string() : a[1].to_string();
        bool ir = false; MemeValue rm; std::string rk;
        bool val = readBoolRef((a.size() >= 3) ? a[1] : a[0], ir, rm, rk);
        std::string lbl = (a.size() >= 3) ? a[2].to_string() : a[1].to_string();
        auto r = ui->checkbox(id, val, lbl);
        if (ir && r.changed) rm[rk] = MemeValue(val);
        return wrap_egui_response(r, MemeValue(val));
    });
    um["toggle"] = um["toggle_switch"] = um["toggleSwitch"] = meme_fn([ui, readBoolRef](const std::vector<MemeValue> &a) -> MemeValue {
        std::string id = (a.size() >= 3) ? a[0].to_string() : a[1].to_string();
        bool ir = false; MemeValue rm; std::string rk;
        bool val = readBoolRef((a.size() >= 3) ? a[1] : a[0], ir, rm, rk);
        std::string lbl = (a.size() >= 3) ? a[2].to_string() : a[1].to_string();
        auto r = ui->toggleSwitch(id, val, lbl);
        if (ir && r.changed) rm[rk] = MemeValue(val);
        return wrap_egui_response(r, MemeValue(val));
    });
    um["radio"] = um["radio_value"] = um["radioValue"] = meme_fn([ui](const std::vector<MemeValue> &a) -> MemeValue {
        std::string id = (a.size() >= 3) ? a[0].to_string() : a[1].to_string();
        bool sel = (a.size() >= 3) ? a[1].is_truthy() : a[0].is_truthy();
        std::string lbl = (a.size() >= 3) ? a[2].to_string() : a[1].to_string();
        auto r = ui->radioValue(id, sel, lbl);
        return wrap_egui_response(r, MemeValue(r.clicked));
    });
    um["slider"] = meme_fn([ui, readNumRef](const std::vector<MemeValue> &a) -> MemeValue {
        bool ir = false; MemeValue rm; std::string rk;
        double val = readNumRef(a[1], ir, rm, rk);
        auto r = ui->slider(a[0].to_string(), val, a[2].as_double(), a[3].as_double(), a.size() > 4 ? a[4].to_string() : "", a.size() > 5 ? a[5].as_double() : 0.0);
        if (ir && r.changed) rm[rk] = MemeValue(val);
        return wrap_egui_response(r, MemeValue(val));
    });
    um["drag_value"] = um["dragValue"] = meme_fn([ui, readNumRef](const std::vector<MemeValue> &a) -> MemeValue {
        bool ir = false; MemeValue rm; std::string rk;
        double val = readNumRef(a[1], ir, rm, rk);
        auto r = ui->dragValue(a[0].to_string(), val, a.size() > 2 ? a[2].as_double() : -1e9, a.size() > 3 ? a[3].as_double() : 1e9, a.size() > 4 ? a[4].as_double() : 0.5);
        if (ir && r.changed) rm[rk] = MemeValue(val);
        return wrap_egui_response(r, MemeValue(val));
    });
    um["progress_bar"] = um["progressBar"] = meme_fn([ui](const std::vector<MemeValue> &a) -> MemeValue {
        double f = a[0].as_double();
        return wrap_egui_response(ui->progressBar(f, a.size() > 1 ? a[1].to_string() : ""), MemeValue(f));
    });
    um["text_edit"] = um["text_edit_singleline"] = um["textEditSingleline"] = meme_fn([ui, readStrRef](const std::vector<MemeValue> &a) -> MemeValue {
        bool ir = false; MemeValue rm; std::string rk;
        std::string txt = readStrRef(a[1], ir, rm, rk);
        auto r = ui->textEditSingleline(a[0].to_string(), txt, a.size() > 2 ? a[2].to_string() : "", a.size() > 3 && a[3].is_truthy(), a.size() > 4 ? static_cast<float>(a[4].as_double()) : 0.0f);
        if (ir && r.changed) rm[rk] = MemeValue(txt);
        return wrap_egui_response(r, MemeValue(txt));
    });
    um["text_edit_multiline"] = um["textEditMultiline"] = meme_fn([ui, readStrRef](const std::vector<MemeValue> &a) -> MemeValue {
        bool ir = false; MemeValue rm; std::string rk;
        std::string txt = readStrRef(a[1], ir, rm, rk);
        auto r = ui->textEditMultiline(a[0].to_string(), txt, a.size() > 2 ? a[2].to_string() : "", a.size() > 3 ? static_cast<float>(a[3].as_double()) : 0.0f, a.size() > 4 ? static_cast<float>(a[4].as_double()) : 96.0f);
        if (ir && r.changed) rm[rk] = MemeValue(txt);
        return wrap_egui_response(r, MemeValue(txt));
    });
    um["combo_box"] = um["comboBox"] = meme_fn([ui, readStrRef](const std::vector<MemeValue> &a) -> MemeValue {
        bool ir = false; MemeValue rm; std::string rk;
        std::string sel = readStrRef(a[1], ir, rm, rk);
        std::vector<std::string> opts;
        if (a[2].type == MemeValue::TYPE_LIST && a[2].list_data) for (const auto &e : *a[2].list_data) opts.push_back(e.to_string());
        auto r = ui->comboBox(a[0].to_string(), sel, opts);
        if (ir && r.changed) rm[rk] = MemeValue(sel);
        return wrap_egui_response(r, MemeValue(sel));
    });
    um["spinner"] = meme_fn([ui](const std::vector<MemeValue> &a) -> MemeValue { ui->spinner(!a.empty() ? static_cast<float>(a[0].as_double()) : 18.0f); return MemeValue(); });
    um["horizontal"] = meme_fn([ui](const std::vector<MemeValue> &a) -> MemeValue { ui->horizontal([&](MemeUI::EguiUi &s) { a[0].call_args({wrap_egui_ui(&s)}); }); return MemeValue(); });
    um["vertical"] = meme_fn([ui](const std::vector<MemeValue> &a) -> MemeValue { ui->vertical([&](MemeUI::EguiUi &s) { a[0].call_args({wrap_egui_ui(&s)}); }); return MemeValue(); });
    um["group"] = meme_fn([ui](const std::vector<MemeValue> &a) -> MemeValue { ui->group([&](MemeUI::EguiUi &s) { a[0].call_args({wrap_egui_ui(&s)}); }); return MemeValue(); });
    um["collapsing"] = meme_fn([ui](const std::vector<MemeValue> &a) -> MemeValue {
        std::string id = a[0].to_string(); std::string hdr = id; bool defO = false;
        if (a.size() == 3) { if (a[1].type == MemeValue::TYPE_BOOL) defO = a[1].is_truthy(); else hdr = a[1].to_string(); }
        else if (a.size() >= 4) { hdr = a[1].to_string(); defO = a[2].is_truthy(); }
        ui->collapsing(id, hdr, defO, [&](MemeUI::EguiUi &s) { a.back().call_args({wrap_egui_ui(&s)}); });
        return MemeValue();
    });
    um["scroll_area"] = um["scrollArea"] = meme_fn([ui](const std::vector<MemeValue> &a) -> MemeValue {
        std::string id = (a.size() == 3) ? a[0].to_string() : "scroll";
        float mh = (a.size() == 3) ? static_cast<float>(a[1].as_double()) : ((a.size() == 2 && (a[0].type == MemeValue::TYPE_INT || a[0].type == MemeValue::TYPE_FLOAT)) ? static_cast<float>(a[0].as_double()) : 240.0f);
        ui->scrollArea(id, mh, [&](MemeUI::EguiUi &s) { a.back().call_args({wrap_egui_ui(&s)}); });
        return MemeValue();
    });
    um["columns"] = meme_fn([ui](const std::vector<MemeValue> &a) -> MemeValue {
        ui->columns(static_cast<int>(a[0].as_int()), [&](std::vector<MemeUI::EguiUi> &cols) {
            auto l = std::make_shared<std::vector<MemeValue>>();
            for (auto &c : cols) l->push_back(wrap_egui_ui(&c));
            a[1].call_args({MemeValue(l)});
        });
        return MemeValue();
    });
    return um;
}

inline MemeValue wrap_egui_context(MemeUI::EguiContext *ctx) {
    MemeValue cm = make_map({});
    cm["top_panel"] = cm["topPanel"] = meme_fn([ctx](const std::vector<MemeValue> &a) -> MemeValue {
        std::string id = (a.size() >= 2 && a[0].type == MemeValue::TYPE_STRING) ? a[0].to_string() : "top_panel";
        float h = (a.size() >= 3) ? static_cast<float>(a[1].as_double()) : ((a.size() == 2 && a[0].type != MemeValue::TYPE_STRING) ? static_cast<float>(a[0].as_double()) : 44.0f);
        ctx->topPanel(id, h, [&](MemeUI::EguiUi &u) { a.back().call_args({wrap_egui_ui(&u)}); });
        return MemeValue();
    });
    cm["bottom_panel"] = cm["bottomPanel"] = meme_fn([ctx](const std::vector<MemeValue> &a) -> MemeValue {
        std::string id = (a.size() >= 2 && a[0].type == MemeValue::TYPE_STRING) ? a[0].to_string() : "bottom_panel";
        float h = (a.size() >= 3) ? static_cast<float>(a[1].as_double()) : 48.0f;
        ctx->bottomPanel(id, h, [&](MemeUI::EguiUi &u) { a.back().call_args({wrap_egui_ui(&u)}); });
        return MemeValue();
    });
    cm["left_panel"] = cm["leftPanel"] = meme_fn([ctx](const std::vector<MemeValue> &a) -> MemeValue {
        std::string id = (a.size() >= 2 && a[0].type == MemeValue::TYPE_STRING) ? a[0].to_string() : "left_panel";
        float w = (a.size() >= 3) ? static_cast<float>(a[1].as_double()) : 200.0f;
        ctx->leftPanel(id, w, [&](MemeUI::EguiUi &u) { a.back().call_args({wrap_egui_ui(&u)}); });
        return MemeValue();
    });
    cm["right_panel"] = cm["rightPanel"] = meme_fn([ctx](const std::vector<MemeValue> &a) -> MemeValue {
        std::string id = (a.size() >= 2 && a[0].type == MemeValue::TYPE_STRING) ? a[0].to_string() : "right_panel";
        float w = (a.size() >= 3) ? static_cast<float>(a[1].as_double()) : 200.0f;
        ctx->rightPanel(id, w, [&](MemeUI::EguiUi &u) { a.back().call_args({wrap_egui_ui(&u)}); });
        return MemeValue();
    });
    cm["central_panel"] = cm["centralPanel"] = meme_fn([ctx](const std::vector<MemeValue> &a) -> MemeValue {
        ctx->centralPanel([&](MemeUI::EguiUi &u) { a[0].call_args({wrap_egui_ui(&u)}); });
        return MemeValue();
    });
    cm["window"] = meme_fn([ctx](const std::vector<MemeValue> &a) -> MemeValue {
        std::string title = a[0].to_string(), id = "win_" + title;
        float dx = 60, dy = 60, w = 280;
        if (a.size() == 3) w = static_cast<float>(a[1].as_double());
        else if (a.size() == 4) { dx = static_cast<float>(a[1].as_double()); dy = static_cast<float>(a[2].as_double()); }
        else if (a.size() == 5) { dx = static_cast<float>(a[1].as_double()); dy = static_cast<float>(a[2].as_double()); w = static_cast<float>(a[3].as_double()); }
        else if (a.size() >= 6) { id = a[0].to_string(); title = a[1].to_string(); dx = static_cast<float>(a[2].as_double()); dy = static_cast<float>(a[3].as_double()); w = static_cast<float>(a[4].as_double()); }
        ctx->window(id, title, dx, dy, w, [&](MemeUI::EguiUi &u) { a.back().call_args({wrap_egui_ui(&u)}); });
        return MemeValue();
    });
    cm["elapsed_ms"] = meme_fn([ctx](const std::vector<MemeValue> &) -> MemeValue { return MemeValue(ctx->input.elapsedMs); });
    cm["delta_ms"] = meme_fn([ctx](const std::vector<MemeValue> &) -> MemeValue { return MemeValue(ctx->input.deltaMs); });
    return cm;
}

inline MemeValue build_ui_module() {
    MemeValue prims = build_primitives_module();
    MemeValue m = make_map({});

    m["mount"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue {
        auto &rt = aot_ui();
        if (!a.empty()) {
            bool wasImmediate = rt.session.isImmediateMode;
            rt.session.isImmediateMode = false;
            rt.session.eguiUpdateFn = nullptr;
            if (wasImmediate) {
                rt.session.interaction = MemeUI::UiInteractionState();
                rt.session.popups.stack.clear();
                rt.session.focusScopes.scopes.clear();
            }
            if (a[0].is_callable()) {
                rt.session.interaction = MemeUI::UiInteractionState();
                rt.session.popups.stack.clear();
                rt.session.focusScopes.scopes.clear();
                rt.rootCallable = a[0];
                rt.rootComponentName = "App";
                rt.rootTree = MemeValue();
            } else if (a[0].type == MemeValue::TYPE_MAP && a[0].has("__rootCallable") && a[0].get("__rootCallable").is_callable()) {
                std::string nextName = a[0].has("__componentName") ? a[0].get("__componentName").to_string() : "App";
                if (nextName != rt.rootComponentName) {
                    rt.session.interaction = MemeUI::UiInteractionState();
                    rt.session.popups.stack.clear();
                    rt.session.focusScopes.scopes.clear();
                }
                rt.rootCallable = a[0].get("__rootCallable");
                rt.rootComponentName = nextName;
                rt.rootTree = MemeValue();
            } else {
                rt.rootCallable = MemeValue();
                rt.rootComponentName = "App";
                rt.rootTree = a[0];
            }
            rt.triggerUiRerender();
        }
        return MemeValue(rt.lastGpuCommandsJson);
    });

    auto showFn = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue {
        std::string title = !a.empty() ? a[0].to_string() : "MEME-UI Native Desktop";
        int w = a.size() > 1 ? static_cast<int>(a[1].as_int()) : 1040;
        int h = a.size() > 2 ? static_cast<int>(a[2].as_int()) : 720;
        aot_ui().runNativeWindow(title, w, h);
        return MemeValue(true);
    });
    m["show"] = m["chala"] = m["run"] = showFn;

    m["dispatch"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue {
        auto &rt = aot_ui();
        int id = !a.empty() ? static_cast<int>(a[0].as_int()) : 0;
        std::string evType = a.size() > 1 ? a[1].to_string() : "click";
        std::string val = a.size() > 2 ? a[2].to_string() : "";
        double numVal = (a.size() > 2 && (a[2].type == MemeValue::TYPE_INT || a[2].type == MemeValue::TYPE_FLOAT)) ? a[2].as_double() : 0.0;

        if (evType == "click" && !val.empty()) {
            int inId = 0;
            for (const auto &hr : rt.session.lastHits) {
                if (hr.hasChange && hr.tag == "input" && hr.id < id) inId = hr.id;
            }
            if (inId > 0) rt.session.dispatchEventOnNode(inId, "change", val);
        }
        bool ok = rt.session.dispatchEventOnNode(id, evType, val, numVal);
        if (ok) rt.triggerUiRerender();
        return MemeValue(ok);
    });

    m["dispatch_key"] = m["dispatchKey"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue {
        MemeUI::UiKeyEvent kev;
        kev.key = a[0].to_string();
        if (a.size() > 1 && a[1].type == MemeValue::TYPE_MAP) {
            kev.ctrl = a[1].get("ctrl", MemeValue(false)).is_truthy();
            kev.shift = a[1].get("shift", MemeValue(false)).is_truthy();
            kev.alt = a[1].get("alt", MemeValue(false)).is_truthy();
            kev.meta = a[1].get("meta", MemeValue(false)).is_truthy();
            kev.text = a[1].get("text", MemeValue("")).to_string();
        }
        if (kev.text.empty() && kev.key.size() == 1 && !kev.ctrl && !kev.meta) kev.text = kev.key;
        bool ok = aot_ui().session.handleKeyEvent(kev);
        if (ok) aot_ui().triggerUiRerender();
        return MemeValue(ok);
    });

    m["tick"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue {
        bool ch = aot_ui().session.tick(a[0].as_double());
        if (ch || aot_ui().session.needsRender) aot_ui().triggerUiRerender();
        return MemeValue(ch);
    });

    m["gpu_commands"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue {
        auto &rt = aot_ui();
        MemeUI::UiNode node = rt.valueToUiNode(a.empty() ? MemeValue() : a[0]);
        auto cmds = MemeUI::computeUiLayout(node, 800.0f, 600.0f);
        rt.lastGpuCommandsJson = MemeUI::gpuDrawCmdsToJson(cmds);
        auto l = std::make_shared<std::vector<MemeValue>>();
        for (const auto &c : cmds) {
            l->push_back(make_map({
                {"type", MemeValue(c.type)}, {"x", MemeValue(static_cast<double>(c.x))}, {"y", MemeValue(static_cast<double>(c.y))},
                {"width", MemeValue(static_cast<double>(c.width))}, {"height", MemeValue(static_cast<double>(c.height))},
                {"radius", MemeValue(static_cast<double>(c.radius))}, {"color", MemeValue(c.color)},
                {"alpha", MemeValue(static_cast<double>(c.alpha))}, {"text", MemeValue(c.text)}
            }));
        }
        return MemeValue(l);
    });

    m["agent"] = m["createAgent"] = prims.get("createAgent");
    m["primitives"] = prims;

    m["likh"] = m["text"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue {
        return make_map({{"type", MemeValue("likh")}, {"text", !a.empty() ? a[0] : MemeValue("")}, {"class", a.size() > 1 ? a[1] : MemeValue("")}});
    });
    m["button"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue {
        return make_map({{"type", MemeValue("button")}, {"label", !a.empty() ? a[0] : MemeValue("Button")}, {"class", a.size() > 1 ? a[1] : MemeValue("")}});
    });
    m["dabba"] = m["card"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue {
        return make_map({{"type", MemeValue("dabba")}, {"children", !a.empty() ? a[0] : MemeValue(std::vector<MemeValue>{})}, {"class", a.size() > 1 ? a[1] : MemeValue("")}});
    });
    m["katar"] = m["row"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue {
        return make_map({{"type", MemeValue("katar")}, {"children", !a.empty() ? a[0] : MemeValue(std::vector<MemeValue>{})}, {"class", a.size() > 1 ? a[1] : MemeValue("")}});
    });
    m["khamba"] = m["col"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue {
        return make_map({{"type", MemeValue("khamba")}, {"children", !a.empty() ? a[0] : MemeValue(std::vector<MemeValue>{})}, {"class", a.size() > 1 ? a[1] : MemeValue("")}});
    });

    m["useState"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue { return useState(a.empty() ? MemeValue() : a[0]); });
    m["useEffect"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue { return useEffect(a[0], a.size() > 1 ? a[1] : MemeValue()); });
    m["useMemo"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue { return useMemo(a[0], a.size() > 1 ? a[1] : MemeValue()); });
    m["useCallback"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue { return useCallback(a[0], a.size() > 1 ? a[1] : MemeValue()); });
    m["useReducer"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue { return useReducer(a[0], a.size() > 1 ? a[1] : MemeValue(), a.size() > 2 ? a[2] : MemeValue()); });
    m["useRef"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue { return useRef(a.empty() ? MemeValue() : a[0]); });
    m["createContext"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue { return createContext(a.empty() ? MemeValue() : a[0]); });
    m["useContext"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue { return useContext(a[0]); });
    m["useRouter"] = meme_fn([](const std::vector<MemeValue> &) -> MemeValue { return useRouter(); });
    m["Router"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue { return Router(a.empty() ? MemeValue() : a[0]); });
    m["Route"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue { return Route(a.empty() ? MemeValue() : a[0]); });
    m["Link"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue { return Link(a.empty() ? MemeValue() : a[0]); });
    m["Platform"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue { return Platform(a.empty() ? MemeValue() : a[0]); });

    return m;
}

inline MemeValue build_egui_module(const MemeValue &baseUi) {
    MemeValue em = make_map({});
    if (baseUi.type == MemeValue::TYPE_MAP && baseUi.map_data) {
        for (const auto &k : baseUi.map_data->map_keys) em[k] = baseUi.get(k);
    }

    auto setupImm = [](const MemeValue &fn, float vw, float vh) {
        auto &rt = aot_ui();
        rt.session.isImmediateMode = true;
        rt.session.eguiUpdateFn = [fn](MemeUI::EguiContext &ctx) {
            fn.call_args({wrap_egui_context(&ctx)});
        };
        rt.session.renderFrame(vw, vh);
        rt.lastGpuCommandsJson = MemeUI::gpuDrawCmdsToJson(rt.session.lastCmds);
        rt.lastHitRegionsJson = MemeUI::hitRegionsToJsonStr(rt.session.lastHits);
    };

    em["frame"] = meme_fn([setupImm](const std::vector<MemeValue> &a) -> MemeValue {
        float vw = a.size() > 1 ? static_cast<float>(a[1].as_double()) : aot_ui().session.input.viewportW;
        float vh = a.size() > 2 ? static_cast<float>(a[2].as_double()) : aot_ui().session.input.viewportH;
        setupImm(a[0], vw, vh);
        auto l = std::make_shared<std::vector<MemeValue>>();
        for (const auto &c : aot_ui().session.lastCmds) {
            l->push_back(make_map({{"type", MemeValue(c.type)}, {"x", MemeValue(static_cast<double>(c.x))}, {"y", MemeValue(static_cast<double>(c.y))},
                                   {"width", MemeValue(static_cast<double>(c.width))}, {"height", MemeValue(static_cast<double>(c.height))},
                                   {"color", MemeValue(c.color)}, {"text", MemeValue(c.text)}}));
        }
        return MemeValue(l);
    });

    em["step"] = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue {
        auto &rt = aot_ui();
        double dt = !a.empty() ? a[0].as_double() : 16.0;
        if (a.size() > 2) {
            rt.session.input.prevPointerX = rt.session.input.pointerX;
            rt.session.input.prevPointerY = rt.session.input.pointerY;
            rt.session.input.pointerX = static_cast<float>(a[1].as_double());
            rt.session.input.pointerY = static_cast<float>(a[2].as_double());
        }
        if (a.size() > 3 && a[3].is_truthy()) {
            rt.session.input.mousePressed[0] = true;
            rt.session.input.mouseDown[0] = true;
            rt.session.input.mouseReleased[0] = false;
        }
        rt.session.tick(dt);
        rt.triggerUiRerender();
        if (a.size() > 3 && a[3].is_truthy()) {
            rt.session.input.mousePressed[0] = false;
            rt.session.input.mouseDown[0] = false;
            rt.session.input.mouseReleased[0] = true;
            rt.triggerUiRerender();
            rt.session.input.mouseReleased[0] = false;
        }
        return MemeValue(static_cast<int64_t>(rt.session.lastCmds.size()));
    });

    MemeValue legacyShow = baseUi.get("show");
    auto runImm = meme_fn([setupImm, legacyShow](const std::vector<MemeValue> &a) -> MemeValue {
        if (a.empty() || (a.size() == 1 && !a[0].is_callable())) {
            return legacyShow.call_args(a);
        }
        std::string title = "MEME-UI Immediate Mode";
        float vw = 1040.0f, vh = 720.0f;
        MemeValue fn = a[0];
        if (a.size() >= 2 && a[1].is_callable()) {
            title = a[0].to_string();
            fn = a[1];
            if (a.size() > 2) vw = static_cast<float>(a[2].as_double());
            if (a.size() > 3) vh = static_cast<float>(a[3].as_double());
        }
        setupImm(fn, vw, vh);
        aot_ui().runNativeWindow(title, static_cast<int>(vw), static_cast<int>(vh));
        return MemeValue(true);
    });
    em["run"] = em["app"] = em["show"] = runImm;
    return em;
}

} // namespace MemeAotUI

// Top-level AOT symbols available directly in compiled MEMElang programs
using MemeAotUI::useState;
using MemeAotUI::state_bana;
using MemeAotUI::useEffect;
using MemeAotUI::useMemo;
using MemeAotUI::useCallback;
using MemeAotUI::useReducer;
using MemeAotUI::useRef;
using MemeAotUI::createContext;
using MemeAotUI::useContext;
using MemeAotUI::useRouter;
using MemeAotUI::Router;
using MemeAotUI::Route;
using MemeAotUI::Link;
using MemeAotUI::make_jsx_element;
using MemeAotUI::make_jsx_component;
using MemeAotUI::meme_push_context;
using MemeAotUI::meme_pop_context;

inline MemeValue ui = MemeAotUI::build_ui_module();
inline MemeValue view = ui;
inline MemeValue gui = ui;
inline MemeValue egui = MemeAotUI::build_egui_module(ui);
inline MemeValue ui_shadcn = MemeAotUI::build_shadcn_module();
inline MemeValue ui_primitives = MemeAotUI::build_primitives_module();
inline MemeValue Router_fn = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue { return Router(a.empty() ? MemeValue() : a[0]); });
inline MemeValue Route_fn = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue { return Route(a.empty() ? MemeValue() : a[0]); });
inline MemeValue Link_fn = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue { return Link(a.empty() ? MemeValue() : a[0]); });
inline MemeValue Platform_fn = meme_fn([](const std::vector<MemeValue> &a) -> MemeValue { return MemeAotUI::Platform(a.empty() ? MemeValue() : a[0]); });

extern "C" {
#ifdef MEME_HOST_PROVIDES_MAIN
__attribute__((visibility("default"))) __attribute__((used))
MemeUI::UiSession *meme_host_get_session() {
    return &MemeAotUI::aot_ui().session;
}
__attribute__((visibility("default"))) __attribute__((used))
void meme_host_rerender() {
    MemeAotUI::aot_ui().triggerUiRerender();
}
#else
inline MemeUI::UiSession *meme_host_get_session() {
    return &MemeAotUI::aot_ui().session;
}
inline void meme_host_rerender() {
    MemeAotUI::aot_ui().triggerUiRerender();
}
#endif
#ifdef __EMSCRIPTEN__
#ifdef MEME_HOST_PROVIDES_MAIN
int meme_user_entrypoint(int argc, char *argv[]);
#endif
EMSCRIPTEN_KEEPALIVE inline int meme_web_start() {
    MemeWebHost::g_setRouteFn = [](const std::string &raw) -> bool {
        auto &rt2 = MemeAotUI::aot_ui();
        std::string target = raw.empty() ? "/" : raw;
        std::string nextPath = target;
        std::unordered_map<std::string, std::string> nextQuery;
        size_t q = target.find('?');
        if (q != std::string::npos) {
            nextPath = target.substr(0, q);
            std::istringstream ss(target.substr(q + 1));
            std::string pair;
            while (std::getline(ss, pair, '&')) {
                size_t eq = pair.find('=');
                if (eq != std::string::npos) nextQuery[pair.substr(0, eq)] = pair.substr(eq + 1);
            }
        }
        bool changed = (rt2.routerPath != nextPath || rt2.routerQuery != nextQuery);
        rt2.routerPath = nextPath;
        rt2.routerQuery = nextQuery;
        return changed;
    };
#ifdef MEME_HOST_PROVIDES_MAIN
    char arg0[] = "meme_web";
    char *argv[] = {arg0};
    meme_user_entrypoint(1, argv);
#endif
    MemeWebHost::bindSession(&MemeAotUI::aot_ui().session, []() { MemeAotUI::aot_ui().triggerUiRerender(); });
    MemeAotUI::aot_ui().triggerUiRerender();
    return 0;
}
#endif
}
