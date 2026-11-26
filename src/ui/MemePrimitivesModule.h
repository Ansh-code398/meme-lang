#pragma once

#include "../interpreter/Value.h"
#include "MemeUiPrimitives.h"
#include "MemeUiRuntime.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <algorithm>
#include <cmath>

namespace MemePrimitivesModule {

inline Value invokeCallableSafe(Interpreter &interp, const Value &fnVal, const std::vector<Value> &args) {
    Value derefed = fnVal.deref();
    if (!derefed.isCallable()) return Value(std::monostate{});
    auto callable = derefed.getCallable();
    if (!callable) return Value(std::monostate{});

    std::vector<Value> callArgs = args;
    if (!callable->isVariadic()) {
        size_t maxA = callable->maxArity();
        size_t minA = callable->minArity();
        if (callArgs.size() > maxA) callArgs.resize(maxA);
        while (callArgs.size() < minA) callArgs.push_back(Value(std::monostate{}));
    }
    return callable->call(interp, callArgs);
}

inline Value mergePropMaps(const std::vector<Value> &maps) {
    auto out = std::make_shared<MemeMap>();
    std::unordered_map<std::string, std::vector<Value>> chainedHandlers;
    std::vector<std::string> classParts;

    for (const auto &v : maps) {
        Value d = v.deref();
        if (!d.isMap()) continue;
        auto m = d.getMap();
        if (!m) continue;
        for (const auto &[k, val] : m->entries) {
            Value vd = val.deref();
            if (k == "class" || k == "className") {
                std::string s = vd.toString();
                if (!s.empty()) classParts.push_back(s);
            } else if (k.size() > 2 && k[0] == 'o' && k[1] == 'n' && std::isupper(static_cast<unsigned char>(k[2])) && vd.isCallable()) {
                chainedHandlers[k].push_back(vd);
            } else {
                out->set(k, val);
            }
        }
    }

    if (!classParts.empty()) {
        std::string mergedCls;
        for (size_t i = 0; i < classParts.size(); ++i) {
            if (i > 0) mergedCls += " ";
            mergedCls += classParts[i];
        }
        out->set("className", Value(mergedCls));
        out->set("class", Value(mergedCls));
    }

    for (const auto &[evKey, fns] : chainedHandlers) {
        if (fns.size() == 1) {
            out->set(evKey, fns[0]);
        } else {
            auto fnsCopy = fns;
            auto composed = std::make_shared<NativeMethodCallable>(evKey, 0, -1, [fnsCopy](Interpreter &interp, const std::vector<Value> &args) -> Value {
                Value last(std::monostate{});
                for (const auto &fn : fnsCopy) {
                    last = invokeCallableSafe(interp, fn, args);
                }
                return last;
            });
            out->set(evKey, Value(composed));
        }
    }
    return Value(out);
}

inline std::vector<std::string> valueToStringList(const Value &v) {
    std::vector<std::string> out;
    Value d = v.deref();
    if (!d.isList()) return out;
    for (const auto &el : d.getList()->elements) {
        out.push_back(el.deref().toString());
    }
    return out;
}

inline Value stringListToValue(const std::vector<std::string> &vec) {
    auto list = std::make_shared<MemeList>();
    list->elements.reserve(vec.size());
    for (const auto &s : vec) list->elements.push_back(Value(s));
    return Value(list);
}

inline std::vector<MemeUI::RovingItem> parseRovingItems(const Value &v) {
    std::vector<MemeUI::RovingItem> out;
    Value d = v.deref();
    if (!d.isList()) return out;
    for (const auto &el : d.getList()->elements) {
        Value ed = el.deref();
        MemeUI::RovingItem ri;
        if (ed.isMap()) {
            auto m = ed.getMap();
            if (m->has("id")) ri.id = m->entries["id"].deref().toString();
            else if (m->has("value")) ri.id = m->entries["value"].deref().toString();
            if (m->has("disabled")) ri.disabled = m->entries["disabled"].deref().isTruthy();
        } else {
            ri.id = ed.toString();
        }
        out.push_back(ri);
    }
    return out;
}

inline std::vector<MemeUI::TypeaheadItem> parseTypeaheadItems(const Value &v) {
    std::vector<MemeUI::TypeaheadItem> out;
    Value d = v.deref();
    if (!d.isList()) return out;
    for (const auto &el : d.getList()->elements) {
        Value ed = el.deref();
        MemeUI::TypeaheadItem ti;
        if (ed.isMap()) {
            auto m = ed.getMap();
            if (m->has("id")) ti.id = m->entries["id"].deref().toString();
            if (m->has("text")) ti.text = m->entries["text"].deref().toString();
            else if (m->has("label")) ti.text = m->entries["label"].deref().toString();
            else ti.text = ti.id;
            if (ti.id.empty()) ti.id = ti.text;
            if (m->has("disabled")) ti.disabled = m->entries["disabled"].deref().isTruthy();
        } else {
            ti.id = ed.toString();
            ti.text = ti.id;
        }
        out.push_back(ti);
    }
    return out;
}

inline Value hitRegionToValue(const MemeUI::UiHitRegion &hr) {
    auto m = std::make_shared<MemeMap>();
    m->set("id", Value(static_cast<int64_t>(hr.id)));
    m->set("key", Value(hr.nodeKey));
    m->set("tag", Value(hr.tag));
    m->set("role", Value(hr.role));
    m->set("name", Value(hr.accessibleName));
    m->set("accessibleName", Value(hr.accessibleName));
    m->set("testId", Value(hr.testId));
    m->set("x", Value(static_cast<double>(hr.x)));
    m->set("y", Value(static_cast<double>(hr.y)));
    m->set("width", Value(static_cast<double>(hr.width)));
    m->set("height", Value(static_cast<double>(hr.height)));
    m->set("disabled", Value(hr.disabled));
    m->set("focused", Value(hr.focused));
    m->set("hovered", Value(hr.hovered));
    m->set("checked", Value(hr.checked));
    m->set("indeterminate", Value(hr.indeterminate));
    m->set("selected", Value(hr.selected));
    m->set("expanded", Value(hr.expanded));
    m->set("value", Value(hr.inputValue));
    m->set("numValue", Value(hr.numValue));
    m->set("placeholder", Value(hr.placeholder));
    m->set("isPassword", Value(hr.isPassword));
    m->set("isMultiline", Value(hr.isMultiline));
    m->set("scrollX", Value(static_cast<double>(hr.scrollX)));
    m->set("maxScrollX", Value(static_cast<double>(hr.maxScrollX)));
    m->set("scrollY", Value(static_cast<double>(hr.scrollY)));
    m->set("maxScrollY", Value(static_cast<double>(hr.maxScrollY)));
    m->set("isScrollContainer", Value(hr.isScrollContainer));
    m->set("isRtl", Value(hr.isRtl));
    return Value(m);
}

inline std::shared_ptr<MemeMap> createPrimitivesModule() {
    auto m = std::make_shared<MemeMap>();

    // 1. mergeProps(...maps)
    m->set("mergeProps", Value(std::make_shared<NativeMethodCallable>("mergeProps", 0, -1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        return mergePropMaps(args);
    })));

    // 2. createParts(specMap)
    m->set("createParts", Value(std::make_shared<NativeMethodCallable>("createParts", 1, 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto out = std::make_shared<MemeMap>();
        Value d = args[0].deref();
        if (!d.isMap()) return Value(out);
        for (const auto &[partName, specVal] : d.getMap()->entries) {
            Value sCopy = specVal.deref();
            auto partFn = std::make_shared<NativeMethodCallable>(partName, 0, 1, [sCopy](Interpreter &interp, const std::vector<Value> &pArgs) -> Value {
                Value userProps = (pArgs.size() > 0 && pArgs[0].deref().isMap()) ? pArgs[0].deref() : Value(std::make_shared<MemeMap>());
                Value baseProps = sCopy.isCallable() ? invokeCallableSafe(interp, sCopy, {userProps}) : sCopy;
                return mergePropMaps({baseProps, userProps});
            });
            out->set(partName, Value(partFn));
        }
        return Value(out);
    })));

    // 3. definePrimitive(specMap)
    m->set("definePrimitive", Value(std::make_shared<NativeMethodCallable>("definePrimitive", 1, 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        Value spec = args[0].deref();
        auto factory = std::make_shared<NativeMethodCallable>("primitive", 0, 1, [spec](Interpreter &interp, const std::vector<Value> &fArgs) -> Value {
            auto inst = std::make_shared<MemeMap>();
            Value userOpts = (fArgs.size() > 0 && fArgs[0].deref().isMap()) ? fArgs[0].deref() : Value(std::make_shared<MemeMap>());
            if (spec.isMap()) {
                auto sm = spec.getMap();
                Value defaults = sm->has("defaults") ? sm->entries["defaults"] : Value(std::make_shared<MemeMap>());
                Value merged = mergePropMaps({defaults, userOpts});
                inst->set("options", merged);
                if (sm->has("parts") && sm->entries["parts"].deref().isMap()) {
                    auto pMap = std::make_shared<MemeMap>();
                    for (const auto &[pk, pv] : sm->entries["parts"].deref().getMap()->entries) {
                        Value pvCopy = pv.deref();
                        pMap->set(pk, Value(std::make_shared<NativeMethodCallable>(pk, 0, 1, [pvCopy, merged](Interpreter &in2, const std::vector<Value> &pa) -> Value {
                            Value up = (pa.size() > 0) ? pa[0].deref() : Value(std::make_shared<MemeMap>());
                            Value bp = pvCopy.isCallable() ? invokeCallableSafe(in2, pvCopy, {merged, up}) : pvCopy;
                            return mergePropMaps({bp, up});
                        })));
                    }
                    inst->set("parts", Value(pMap));
                }
                if (sm->has("setup") && sm->entries["setup"].deref().isCallable()) {
                    Value extra = invokeCallableSafe(interp, sm->entries["setup"], {merged, Value(inst)});
                    if (extra.deref().isMap()) {
                        for (const auto &[ek, ev] : extra.deref().getMap()->entries) inst->set(ek, ev);
                    }
                }
            } else {
                inst->set("options", userOpts);
            }
            return Value(inst);
        });
        return Value(factory);
    })));

    // 4. createPopup(opts)
    m->set("createPopup", Value(std::make_shared<NativeMethodCallable>("createPopup", 0, 1, [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        struct PopupController {
            std::string id = "popup";
            bool modal = false;
            bool closeOnEscape = true;
            bool closeOnOutsideClick = true;
            std::string restoreFocusToId = "";
            Value onOpenChange = Value(std::monostate{});
            MemeUI::UiRect anchorBounds{0, 0, 0, 0};
            MemeUI::UiRect triggerBounds{0, 0, 0, 0};
            MemeUI::UiRect contentBounds{0, 0, 200, 120};
        };
        auto ctrl = std::make_shared<PopupController>();
        bool initOpen = false;
        if (args.size() > 0 && args[0].deref().isMap()) {
            auto om = args[0].deref().getMap();
            if (om->has("id")) ctrl->id = om->entries["id"].deref().toString();
            if (om->has("modal")) ctrl->modal = om->entries["modal"].deref().isTruthy();
            if (om->has("closeOnEscape")) ctrl->closeOnEscape = om->entries["closeOnEscape"].deref().isTruthy();
            if (om->has("closeOnOutsideClick")) ctrl->closeOnOutsideClick = om->entries["closeOnOutsideClick"].deref().isTruthy();
            if (om->has("restoreFocusToId")) ctrl->restoreFocusToId = om->entries["restoreFocusToId"].deref().toString();
            if (om->has("onOpenChange")) ctrl->onOpenChange = om->entries["onOpenChange"].deref();
            if (om->has("defaultOpen")) initOpen = om->entries["defaultOpen"].deref().isTruthy();
            if (om->has("open")) initOpen = om->entries["open"].deref().isTruthy();
        }

        auto obj = std::make_shared<MemeMap>();
        auto syncOpen = [ctrl](Interpreter &interp, bool nextOpen) {
            bool currentlyOpen = interp.uiSession.popups.isOpen(ctrl->id);
            if (nextOpen && !currentlyOpen) {
                MemeUI::PopupEntry pe;
                pe.id = ctrl->id;
                pe.modal = ctrl->modal;
                pe.closeOnEscape = ctrl->closeOnEscape;
                pe.closeOnOutsideClick = ctrl->closeOnOutsideClick;
                pe.restoreFocusId = ctrl->restoreFocusToId.empty() ? interp.uiSession.interaction.focusedKey : ctrl->restoreFocusToId;
                pe.triggerBounds = ctrl->triggerBounds;
                pe.bounds = ctrl->contentBounds;
                pe.onDismiss = [ctrl, &interp]() {
                    if (ctrl->onOpenChange.isCallable()) invokeCallableSafe(interp, ctrl->onOpenChange, {Value(false)});
                };
                interp.uiSession.popups.open(pe);
                if (ctrl->onOpenChange.isCallable()) invokeCallableSafe(interp, ctrl->onOpenChange, {Value(true)});
            } else if (!nextOpen && currentlyOpen) {
                std::string restore = interp.uiSession.popups.closeById(ctrl->id);
                if (!restore.empty()) interp.uiSession.interaction.focusedKey = restore;
            }
            interp.uiSession.needsRender = true;
            interp.uiSession.version++;
        };

        if (initOpen) {
            syncOpen(interp, true);
        }

        obj->set("id", Value(ctrl->id));
        obj->set("isOpen", Value(std::make_shared<NativeMethodCallable>("isOpen", 0, 0, [ctrl](Interpreter &interp, const std::vector<Value> &) -> Value {
            return Value(interp.uiSession.popups.isOpen(ctrl->id));
        })));
        obj->set("open", Value(std::make_shared<NativeMethodCallable>("open", 0, 1, [ctrl, syncOpen](Interpreter &interp, const std::vector<Value> &a) -> Value {
            if (a.size() > 0 && a[0].deref().isMap()) {
                auto m = a[0].deref().getMap();
                if (m->has("triggerBounds") && m->entries["triggerBounds"].deref().isMap()) {
                    auto tb = m->entries["triggerBounds"].deref().getMap();
                    ctrl->triggerBounds = {
                        tb->has("x") ? static_cast<float>(tb->entries["x"].deref().asDouble()) : 0.0f,
                        tb->has("y") ? static_cast<float>(tb->entries["y"].deref().asDouble()) : 0.0f,
                        tb->has("width") ? static_cast<float>(tb->entries["width"].deref().asDouble()) : 100.0f,
                        tb->has("height") ? static_cast<float>(tb->entries["height"].deref().asDouble()) : 36.0f
                    };
                }
                if (m->has("bounds") && m->entries["bounds"].deref().isMap()) {
                    auto cb = m->entries["bounds"].deref().getMap();
                    ctrl->contentBounds = {
                        cb->has("x") ? static_cast<float>(cb->entries["x"].deref().asDouble()) : 0.0f,
                        cb->has("y") ? static_cast<float>(cb->entries["y"].deref().asDouble()) : 0.0f,
                        cb->has("width") ? static_cast<float>(cb->entries["width"].deref().asDouble()) : 200.0f,
                        cb->has("height") ? static_cast<float>(cb->entries["height"].deref().asDouble()) : 120.0f
                    };
                }
            }
            syncOpen(interp, true);
            return Value(true);
        })));
        obj->set("close", Value(std::make_shared<NativeMethodCallable>("close", 0, 0, [syncOpen](Interpreter &interp, const std::vector<Value> &) -> Value {
            syncOpen(interp, false);
            return Value(false);
        })));
        obj->set("toggle", Value(std::make_shared<NativeMethodCallable>("toggle", 0, 0, [ctrl, syncOpen](Interpreter &interp, const std::vector<Value> &) -> Value {
            bool next = !interp.uiSession.popups.isOpen(ctrl->id);
            syncOpen(interp, next);
            return Value(next);
        })));
        obj->set("handleEscape", Value(std::make_shared<NativeMethodCallable>("handleEscape", 0, 0, [](Interpreter &interp, const std::vector<Value> &) -> Value {
            std::string restore;
            bool closed = interp.uiSession.popups.handleEscape(restore);
            if (!restore.empty()) interp.uiSession.interaction.focusedKey = restore;
            return Value(closed);
        })));
        obj->set("handlePointerDown", Value(std::make_shared<NativeMethodCallable>("handlePointerDown", 2, 2, [](Interpreter &interp, const std::vector<Value> &a) -> Value {
            std::string restore;
            float x = static_cast<float>(a[0].deref().asDouble());
            float y = static_cast<float>(a[1].deref().asDouble());
            bool closed = interp.uiSession.popups.handlePointerDown(x, y, restore);
            if (!restore.empty()) interp.uiSession.interaction.focusedKey = restore;
            return Value(closed);
        })));
        obj->set("computePlacement", Value(std::make_shared<NativeMethodCallable>("computePlacement", 1, 1, [](Interpreter &interp, const std::vector<Value> &a) -> Value {
            auto out = std::make_shared<MemeMap>();
            if (!a[0].deref().isMap()) return Value(out);
            auto om = a[0].deref().getMap();
            auto readRect = [&](const std::string &k, MemeUI::UiRect def) -> MemeUI::UiRect {
                if (!om->has(k) || !om->entries[k].deref().isMap()) return def;
                auto r = om->entries[k].deref().getMap();
                return {
                    r->has("x") ? static_cast<float>(r->entries["x"].deref().asDouble()) : def.x,
                    r->has("y") ? static_cast<float>(r->entries["y"].deref().asDouble()) : def.y,
                    r->has("width") ? static_cast<float>(r->entries["width"].deref().asDouble()) : def.width,
                    r->has("height") ? static_cast<float>(r->entries["height"].deref().asDouble()) : def.height
                };
            };
            MemeUI::UiRect anchor = readRect("anchor", {0, 0, 100, 32});
            float pw = om->has("popupWidth") ? static_cast<float>(om->entries["popupWidth"].deref().asDouble()) : 200.0f;
            float ph = om->has("popupHeight") ? static_cast<float>(om->entries["popupHeight"].deref().asDouble()) : 120.0f;
            MemeUI::UiRect vp = readRect("viewport", {0, 0, interp.uiSession.input.viewportW, interp.uiSession.input.viewportH});
            MemeUI::PopupPlacementOptions po;
            if (om->has("side")) {
                std::string s = om->entries["side"].deref().toString();
                if (s == "top") po.side = MemeUI::PopupSide::TOP;
                else if (s == "left") po.side = MemeUI::PopupSide::LEFT;
                else if (s == "right") po.side = MemeUI::PopupSide::RIGHT;
                else po.side = MemeUI::PopupSide::BOTTOM;
            }
            if (om->has("align")) {
                std::string al = om->entries["align"].deref().toString();
                if (al == "center") po.align = MemeUI::PopupAlign::CENTER;
                else if (al == "end") po.align = MemeUI::PopupAlign::END;
                else po.align = MemeUI::PopupAlign::START;
            }
            if (om->has("sideOffset")) po.sideOffset = static_cast<float>(om->entries["sideOffset"].deref().asDouble());
            if (om->has("alignOffset")) po.alignOffset = static_cast<float>(om->entries["alignOffset"].deref().asDouble());
            if (om->has("padding")) po.viewportPadding = static_cast<float>(om->entries["padding"].deref().asDouble());
            if (om->has("viewportPadding")) po.viewportPadding = static_cast<float>(om->entries["viewportPadding"].deref().asDouble());
            if (om->has("avoidCollisions")) po.avoidCollisions = om->entries["avoidCollisions"].deref().isTruthy();
            if (om->has("flip")) po.flip = om->entries["flip"].deref().isTruthy();
            if (om->has("shift")) po.shift = om->entries["shift"].deref().isTruthy();
            if (om->has("rtl")) po.rtl = om->entries["rtl"].deref().isTruthy();

            auto res = MemeUI::computePopupPlacement(anchor, pw, ph, vp, po);
            auto sideStr = [](MemeUI::PopupSide s) -> std::string {
                if (s == MemeUI::PopupSide::TOP) return "top";
                if (s == MemeUI::PopupSide::LEFT) return "left";
                if (s == MemeUI::PopupSide::RIGHT) return "right";
                return "bottom";
            };
            auto alignStr = [](MemeUI::PopupAlign al) -> std::string {
                if (al == MemeUI::PopupAlign::CENTER) return "center";
                if (al == MemeUI::PopupAlign::END) return "end";
                return "start";
            };
            out->set("x", Value(static_cast<double>(res.x)));
            out->set("y", Value(static_cast<double>(res.y)));
            out->set("side", Value(sideStr(res.actualSide)));
            out->set("align", Value(alignStr(res.actualAlign)));
            out->set("flipped", Value(res.flipped));
            out->set("shifted", Value(res.shifted));
            return Value(out);
        })));
        obj->set("getTriggerProps", Value(std::make_shared<NativeMethodCallable>("getTriggerProps", 0, 1, [ctrl, syncOpen](Interpreter &interp, const std::vector<Value> &a) -> Value {
            auto p = std::make_shared<MemeMap>();
            p->set("aria-expanded", Value(interp.uiSession.popups.isOpen(ctrl->id)));
            p->set("aria-haspopup", Value(ctrl->modal ? std::string("dialog") : std::string("menu")));
            p->set("onClick", Value(std::make_shared<NativeMethodCallable>("onClick", 0, 1, [ctrl, syncOpen](Interpreter &in2, const std::vector<Value> &) -> Value {
                bool next = !in2.uiSession.popups.isOpen(ctrl->id);
                syncOpen(in2, next);
                return Value(next);
            })));
            Value u = (a.size() > 0) ? a[0] : Value(std::monostate{});
            return mergePropMaps({Value(p), u});
        })));
        obj->set("getContentProps", Value(std::make_shared<NativeMethodCallable>("getContentProps", 0, 1, [ctrl](Interpreter &interp, const std::vector<Value> &a) -> Value {
            auto p = std::make_shared<MemeMap>();
            p->set("role", Value(ctrl->modal ? std::string("dialog") : std::string("menu")));
            p->set("open", Value(interp.uiSession.popups.isOpen(ctrl->id)));
            p->set("portal", Value(true));
            Value u = (a.size() > 0) ? a[0] : Value(std::monostate{});
            return mergePropMaps({Value(p), u});
        })));
        return Value(obj);
    })));

    // 5. createRovingFocus(opts)
    m->set("createRovingFocus", Value(std::make_shared<NativeMethodCallable>("createRovingFocus", 0, 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto state = std::make_shared<MemeUI::RovingFocusState>();
        if (args.size() > 0 && args[0].deref().isMap()) {
            auto om = args[0].deref().getMap();
            if (om->has("items")) state->items = parseRovingItems(om->entries["items"]);
            if (om->has("activeId")) state->activeId = om->entries["activeId"].deref().toString();
            if (om->has("orientation")) state->orientation = om->entries["orientation"].deref().toString();
            if (om->has("columns")) state->columns = static_cast<int>(om->entries["columns"].deref().asDouble());
            if (om->has("loop")) state->loop = om->entries["loop"].deref().isTruthy();
            if (om->has("rtl")) state->rtl = om->entries["rtl"].deref().isTruthy();
        }
        state->ensureValidActive();

        auto obj = std::make_shared<MemeMap>();
        obj->set("setItems", Value(std::make_shared<NativeMethodCallable>("setItems", 1, 1, [state](Interpreter &, const std::vector<Value> &a) -> Value {
            state->items = parseRovingItems(a[0]);
            state->ensureValidActive();
            return Value(state->activeId);
        })));
        obj->set("handleKey", Value(std::make_shared<NativeMethodCallable>("handleKey", 1, 1, [state](Interpreter &, const std::vector<Value> &a) -> Value {
            std::string next;
            if (state->handleKey(a[0].deref().toString(), next)) return Value(next);
            return Value(std::monostate{});
        })));
        obj->set("getActiveId", Value(std::make_shared<NativeMethodCallable>("getActiveId", 0, 0, [state](Interpreter &, const std::vector<Value> &) -> Value {
            return Value(state->activeId);
        })));
        obj->set("setActiveId", Value(std::make_shared<NativeMethodCallable>("setActiveId", 1, 1, [state](Interpreter &, const std::vector<Value> &a) -> Value {
            state->activeId = a[0].deref().toString();
            state->ensureValidActive();
            return Value(state->activeId);
        })));
        obj->set("getTabIndex", Value(std::make_shared<NativeMethodCallable>("getTabIndex", 1, 1, [state](Interpreter &, const std::vector<Value> &a) -> Value {
            return Value(static_cast<int64_t>(state->getTabIndex(a[0].deref().toString())));
        })));
        obj->set("getItemProps", Value(std::make_shared<NativeMethodCallable>("getItemProps", 1, 2, [state](Interpreter &, const std::vector<Value> &a) -> Value {
            std::string id = a[0].deref().toString();
            auto p = std::make_shared<MemeMap>();
            p->set("tabIndex", Value(static_cast<int64_t>(state->getTabIndex(id))));
            p->set("onKeyDown", Value(std::make_shared<NativeMethodCallable>("onKeyDown", 0, 1, [state](Interpreter &, const std::vector<Value> &ka) -> Value {
                std::string key = (ka.size() > 0 && ka[0].deref().isMap() && ka[0].deref().getMap()->has("key"))
                    ? ka[0].deref().getMap()->entries["key"].deref().toString()
                    : (ka.size() > 0 ? ka[0].deref().toString() : "");
                std::string next;
                if (state->handleKey(key, next)) return Value(next);
                return Value(std::monostate{});
            })));
            Value u = (a.size() > 1) ? a[1] : Value(std::monostate{});
            return mergePropMaps({Value(p), u});
        })));
        return Value(obj);
    })));

    // 6. createFocusTrap(opts) & createFocusScope(opts)
    auto makeFocusScopeFn = [](bool defaultTrapped) {
        return [defaultTrapped](Interpreter &, const std::vector<Value> &args) -> Value {
            auto scope = std::make_shared<MemeUI::FocusScope>();
            scope->trapped = defaultTrapped;
            scope->id = defaultTrapped ? "focus-trap" : "focus-scope";
            if (args.size() > 0 && args[0].deref().isMap()) {
                auto om = args[0].deref().getMap();
                if (om->has("id")) scope->id = om->entries["id"].deref().toString();
                if (om->has("containerId")) scope->containerId = om->entries["containerId"].deref().toString();
                if (om->has("focusableIds")) scope->focusableIds = valueToStringList(om->entries["focusableIds"]);
                if (om->has("initialFocusId")) scope->initialFocusId = om->entries["initialFocusId"].deref().toString();
                if (om->has("restoreFocusToId")) scope->restoreFocusToId = om->entries["restoreFocusToId"].deref().toString();
                if (om->has("trapped")) scope->trapped = om->entries["trapped"].deref().isTruthy();
            }
            auto obj = std::make_shared<MemeMap>();
            obj->set("mount", Value(std::make_shared<NativeMethodCallable>("mount", 0, 1, [scope](Interpreter &interp, const std::vector<Value> &a) -> Value {
                std::string cur = (a.size() > 0) ? a[0].deref().toString() : interp.uiSession.interaction.focusedKey;
                std::string target = interp.uiSession.focusScopes.mountScope(*scope, cur);
                interp.uiSession.interaction.focusedKey = target;
                return Value(target);
            })));
            obj->set("unmount", Value(std::make_shared<NativeMethodCallable>("unmount", 0, 0, [scope](Interpreter &interp, const std::vector<Value> &) -> Value {
                std::string restored = interp.uiSession.focusScopes.unmountScope(scope->id);
                if (!restored.empty()) interp.uiSession.interaction.focusedKey = restored;
                return Value(restored);
            })));
            obj->set("handleTab", Value(std::make_shared<NativeMethodCallable>("handleTab", 0, 2, [](Interpreter &interp, const std::vector<Value> &a) -> Value {
                bool shift = (a.size() > 0) ? a[0].deref().isTruthy() : false;
                std::string cur = (a.size() > 1) ? a[1].deref().toString() : interp.uiSession.interaction.focusedKey;
                std::string next;
                if (interp.uiSession.focusScopes.handleTab(shift, cur, next)) {
                    interp.uiSession.interaction.focusedKey = next;
                    return Value(next);
                }
                return Value(std::monostate{});
            })));
            obj->set("setFocusables", Value(std::make_shared<NativeMethodCallable>("setFocusables", 1, 1, [scope](Interpreter &interp, const std::vector<Value> &a) -> Value {
                scope->focusableIds = valueToStringList(a[0]);
                for (auto &s : interp.uiSession.focusScopes.scopes) {
                    if (s.id == scope->id) s.focusableIds = scope->focusableIds;
                }
                return Value(true);
            })));
            obj->set("isActive", Value(std::make_shared<NativeMethodCallable>("isActive", 0, 0, [scope](Interpreter &interp, const std::vector<Value> &) -> Value {
                const auto *tr = interp.uiSession.focusScopes.activeTrap();
                return Value(tr != nullptr && tr->id == scope->id);
            })));
            return Value(obj);
        };
    };
    m->set("createFocusTrap", Value(std::make_shared<NativeMethodCallable>("createFocusTrap", 0, 1, makeFocusScopeFn(true))));
    m->set("createFocusScope", Value(std::make_shared<NativeMethodCallable>("createFocusScope", 0, 1, makeFocusScopeFn(false))));

    // 7. createSelection(opts)
    m->set("createSelection", Value(std::make_shared<NativeMethodCallable>("createSelection", 0, 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto st = std::make_shared<MemeUI::SelectionState>();
        if (args.size() > 0 && args[0].deref().isMap()) {
            auto om = args[0].deref().getMap();
            if (om->has("mode")) st->mode = om->entries["mode"].deref().toString();
            if (om->has("disallowEmpty")) st->disallowEmpty = om->entries["disallowEmpty"].deref().isTruthy();
            if (om->has("selectedIds")) st->selectedIds = valueToStringList(om->entries["selectedIds"]);
            if (om->has("disabledIds")) {
                for (const auto &d : valueToStringList(om->entries["disabledIds"])) st->disabledIds.insert(d);
            }
        }
        auto obj = std::make_shared<MemeMap>();
        obj->set("isSelected", Value(std::make_shared<NativeMethodCallable>("isSelected", 1, 1, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            return Value(st->isSelected(a[0].deref().toString()));
        })));
        obj->set("select", Value(std::make_shared<NativeMethodCallable>("select", 1, 1, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            return Value(st->select(a[0].deref().toString()));
        })));
        obj->set("deselect", Value(std::make_shared<NativeMethodCallable>("deselect", 1, 1, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            return Value(st->deselect(a[0].deref().toString()));
        })));
        obj->set("toggle", Value(std::make_shared<NativeMethodCallable>("toggle", 1, 1, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            return Value(st->toggle(a[0].deref().toString()));
        })));
        obj->set("selectRange", Value(std::make_shared<NativeMethodCallable>("selectRange", 2, 2, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            return Value(st->selectRange(a[0].deref().toString(), valueToStringList(a[1])));
        })));
        obj->set("selectAll", Value(std::make_shared<NativeMethodCallable>("selectAll", 1, 1, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            return Value(st->selectAll(valueToStringList(a[0])));
        })));
        obj->set("clear", Value(std::make_shared<NativeMethodCallable>("clear", 0, 0, [st](Interpreter &, const std::vector<Value> &) -> Value {
            return Value(st->clear());
        })));
        obj->set("getSelected", Value(std::make_shared<NativeMethodCallable>("getSelected", 0, 0, [st](Interpreter &, const std::vector<Value> &) -> Value {
            return stringListToValue(st->selectedIds);
        })));
        return Value(obj);
    })));

    // 8. createTypeahead(opts)
    m->set("createTypeahead", Value(std::make_shared<NativeMethodCallable>("createTypeahead", 0, 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto st = std::make_shared<MemeUI::TypeaheadState>();
        auto items = std::make_shared<std::vector<MemeUI::TypeaheadItem>>();
        if (args.size() > 0 && args[0].deref().isMap()) {
            auto om = args[0].deref().getMap();
            if (om->has("items")) *items = parseTypeaheadItems(om->entries["items"]);
            if (om->has("timeoutMs")) st->timeoutMs = om->entries["timeoutMs"].deref().asDouble();
        }
        auto obj = std::make_shared<MemeMap>();
        obj->set("setItems", Value(std::make_shared<NativeMethodCallable>("setItems", 1, 1, [items](Interpreter &, const std::vector<Value> &a) -> Value {
            *items = parseTypeaheadItems(a[0]);
            return Value(true);
        })));
        obj->set("handleChar", Value(std::make_shared<NativeMethodCallable>("handleChar", 1, 3, [st, items](Interpreter &, const std::vector<Value> &a) -> Value {
            std::string ch = a[0].deref().toString();
            double nowMs = (a.size() > 1) ? a[1].deref().asDouble() : 0.0;
            std::string cur = (a.size() > 2) ? a[2].deref().toString() : "";
            return Value(st->handleChar(ch, nowMs, *items, cur));
        })));
        obj->set("matchQuery", Value(std::make_shared<NativeMethodCallable>("matchQuery", 1, 2, [st, items](Interpreter &, const std::vector<Value> &a) -> Value {
            std::string q = a[0].deref().toString();
            std::string cur = (a.size() > 1) ? a[1].deref().toString() : "";
            return Value(st->matchQuery(q, *items, cur));
        })));
        obj->set("reset", Value(std::make_shared<NativeMethodCallable>("reset", 0, 0, [st](Interpreter &, const std::vector<Value> &) -> Value {
            st->reset();
            return Value(true);
        })));
        obj->set("getBuffer", Value(std::make_shared<NativeMethodCallable>("getBuffer", 0, 0, [st](Interpreter &, const std::vector<Value> &) -> Value {
            return Value(st->buffer);
        })));
        return Value(obj);
    })));

    // 9. createVirtualizer(opts)
    m->set("createVirtualizer", Value(std::make_shared<NativeMethodCallable>("createVirtualizer", 0, 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto st = std::make_shared<MemeUI::VirtualizerState>();
        int initCount = 0;
        float initEst = 44.0f;
        if (args.size() > 0 && args[0].deref().isMap()) {
            auto om = args[0].deref().getMap();
            if (om->has("count")) initCount = static_cast<int>(om->entries["count"].deref().asDouble());
            else if (om->has("itemCount")) initCount = static_cast<int>(om->entries["itemCount"].deref().asDouble());
            if (om->has("estimateSize")) initEst = static_cast<float>(om->entries["estimateSize"].deref().asDouble());
            if (om->has("overscan")) st->overscan = static_cast<int>(om->entries["overscan"].deref().asDouble());
            if (om->has("viewportSize")) st->viewportSize = static_cast<float>(om->entries["viewportSize"].deref().asDouble());
            if (om->has("stickyIndices") && om->entries["stickyIndices"].deref().isList()) {
                for (const auto &v : om->entries["stickyIndices"].deref().getList()->elements) {
                    st->stickyIndices.push_back(static_cast<int>(v.deref().asDouble()));
                }
            }
        }
        st->setCount(initCount, initEst);

        auto obj = std::make_shared<MemeMap>();
        obj->set("setCount", Value(std::make_shared<NativeMethodCallable>("setCount", 1, 2, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            float est = (a.size() > 1) ? static_cast<float>(a[1].deref().asDouble()) : st->estimateSize;
            st->setCount(static_cast<int>(a[0].deref().asDouble()), est);
            return Value(static_cast<double>(st->totalSize()));
        })));
        obj->set("measureItem", Value(std::make_shared<NativeMethodCallable>("measureItem", 2, 2, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            st->measureItem(static_cast<int>(a[0].deref().asDouble()), static_cast<float>(a[1].deref().asDouble()));
            return Value(static_cast<double>(st->totalSize()));
        })));
        obj->set("setScrollOffset", Value(std::make_shared<NativeMethodCallable>("setScrollOffset", 1, 1, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            st->scrollOffset = std::max(0.0f, static_cast<float>(a[0].deref().asDouble()));
            return Value(static_cast<double>(st->scrollOffset));
        })));
        obj->set("setViewportSize", Value(std::make_shared<NativeMethodCallable>("setViewportSize", 1, 1, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            st->viewportSize = std::max(1.0f, static_cast<float>(a[0].deref().asDouble()));
            return Value(static_cast<double>(st->viewportSize));
        })));
        obj->set("scrollToIndex", Value(std::make_shared<NativeMethodCallable>("scrollToIndex", 1, 2, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            std::string align = (a.size() > 1) ? a[1].deref().toString() : "auto";
            return Value(static_cast<double>(st->scrollToIndex(static_cast<int>(a[0].deref().asDouble()), align)));
        })));
        obj->set("getTotalSize", Value(std::make_shared<NativeMethodCallable>("getTotalSize", 0, 0, [st](Interpreter &, const std::vector<Value> &) -> Value {
            return Value(static_cast<double>(st->totalSize()));
        })));
        obj->set("offsetOf", Value(std::make_shared<NativeMethodCallable>("offsetOf", 1, 1, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            return Value(static_cast<double>(st->offsetOf(static_cast<int>(a[0].deref().asDouble()))));
        })));
        obj->set("getStickyIndex", Value(std::make_shared<NativeMethodCallable>("getStickyIndex", 0, 0, [st](Interpreter &, const std::vector<Value> &) -> Value {
            return Value(static_cast<int64_t>(st->computeSnapshot().stickyIndex));
        })));
        obj->set("getVirtualItems", Value(std::make_shared<NativeMethodCallable>("getVirtualItems", 0, 0, [st](Interpreter &, const std::vector<Value> &) -> Value {
            auto snap = st->computeSnapshot();
            auto list = std::make_shared<MemeList>();
            list->elements.reserve(snap.visibleItems.size());
            for (const auto &vi : snap.visibleItems) {
                auto rm = std::make_shared<MemeMap>();
                rm->set("index", Value(static_cast<int64_t>(vi.index)));
                rm->set("offset", Value(static_cast<double>(vi.offset)));
                rm->set("size", Value(static_cast<double>(vi.size)));
                list->elements.push_back(Value(rm));
            }
            return Value(list);
        })));
        return Value(obj);
    })));

    // 10. createReorder(opts)
    m->set("createReorder", Value(std::make_shared<NativeMethodCallable>("createReorder", 0, 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto st = std::make_shared<MemeUI::ReorderState>();
        if (args.size() > 0 && args[0].deref().isMap()) {
            auto om = args[0].deref().getMap();
            if (om->has("items")) st->items = valueToStringList(om->entries["items"]);
        }
        auto obj = std::make_shared<MemeMap>();
        obj->set("setItems", Value(std::make_shared<NativeMethodCallable>("setItems", 1, 1, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            st->items = valueToStringList(a[0]);
            return Value(true);
        })));
        obj->set("getItems", Value(std::make_shared<NativeMethodCallable>("getItems", 0, 0, [st](Interpreter &, const std::vector<Value> &) -> Value {
            return stringListToValue(st->items);
        })));
        obj->set("startDrag", Value(std::make_shared<NativeMethodCallable>("startDrag", 2, 2, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            return Value(st->startDrag(static_cast<int>(a[0].deref().asDouble()), static_cast<float>(a[1].deref().asDouble())));
        })));
        obj->set("updateDrag", Value(std::make_shared<NativeMethodCallable>("updateDrag", 2, 2, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            std::vector<float> mids;
            if (a[1].deref().isList()) {
                for (const auto &v : a[1].deref().getList()->elements) mids.push_back(static_cast<float>(v.deref().asDouble()));
            }
            return Value(st->updateDrag(static_cast<float>(a[0].deref().asDouble()), mids));
        })));
        obj->set("endDrag", Value(std::make_shared<NativeMethodCallable>("endDrag", 0, 0, [st](Interpreter &, const std::vector<Value> &) -> Value {
            st->endDrag();
            return stringListToValue(st->items);
        })));
        obj->set("handleKey", Value(std::make_shared<NativeMethodCallable>("handleKey", 2, 2, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            int nextIdx = 0;
            bool handled = st->handleKey(a[0].deref().toString(), static_cast<int>(a[1].deref().asDouble()), nextIdx);
            auto res = std::make_shared<MemeMap>();
            res->set("handled", Value(handled));
            res->set("focusedIndex", Value(static_cast<int64_t>(nextIdx)));
            res->set("grabbedIndex", Value(static_cast<int64_t>(st->grabbedIndex)));
            res->set("announcement", Value(st->announcement));
            res->set("items", stringListToValue(st->items));
            return Value(res);
        })));
        obj->set("cancel", Value(std::make_shared<NativeMethodCallable>("cancel", 0, 0, [st](Interpreter &, const std::vector<Value> &) -> Value {
            return Value(st->cancel());
        })));
        obj->set("getGrabbedIndex", Value(std::make_shared<NativeMethodCallable>("getGrabbedIndex", 0, 0, [st](Interpreter &, const std::vector<Value> &) -> Value {
            return Value(static_cast<int64_t>(st->grabbedIndex));
        })));
        obj->set("getAnnouncement", Value(std::make_shared<NativeMethodCallable>("getAnnouncement", 0, 0, [st](Interpreter &, const std::vector<Value> &) -> Value {
            return Value(st->announcement);
        })));
        return Value(obj);
    })));

    // 11. createResize(opts)
    m->set("createResize", Value(std::make_shared<NativeMethodCallable>("createResize", 0, 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto st = std::make_shared<MemeUI::ResizeState>();
        auto parseFloats = [](const Value &v) {
            std::vector<float> out;
            if (v.deref().isList()) {
                for (const auto &e : v.deref().getList()->elements) out.push_back(static_cast<float>(e.deref().asDouble()));
            }
            return out;
        };
        if (args.size() > 0 && args[0].deref().isMap()) {
            auto om = args[0].deref().getMap();
            if (om->has("sizes")) st->sizes = parseFloats(om->entries["sizes"]);
            if (om->has("minSizes")) st->minSizes = parseFloats(om->entries["minSizes"]);
            if (om->has("maxSizes")) st->maxSizes = parseFloats(om->entries["maxSizes"]);
            if (om->has("collapsible") && om->entries["collapsible"].deref().isList()) {
                st->collapsible.clear();
                for (const auto &e : om->entries["collapsible"].deref().getList()->elements) st->collapsible.push_back(e.deref().isTruthy());
            }
            if (om->has("step")) st->step = static_cast<float>(om->entries["step"].deref().asDouble());
            if (om->has("coarseStep")) st->coarseStep = static_cast<float>(om->entries["coarseStep"].deref().asDouble());
            if (om->has("vertical")) st->vertical = om->entries["vertical"].deref().isTruthy();
        }
        auto obj = std::make_shared<MemeMap>();
        obj->set("getSizes", Value(std::make_shared<NativeMethodCallable>("getSizes", 0, 0, [st](Interpreter &, const std::vector<Value> &) -> Value {
            auto l = std::make_shared<MemeList>();
            for (float s : st->sizes) l->elements.push_back(Value(static_cast<double>(s)));
            return Value(l);
        })));
        obj->set("resizeHandle", Value(std::make_shared<NativeMethodCallable>("resizeHandle", 2, 2, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            return Value(st->resizeHandle(static_cast<int>(a[0].deref().asDouble()), static_cast<float>(a[1].deref().asDouble())));
        })));
        obj->set("handleKey", Value(std::make_shared<NativeMethodCallable>("handleKey", 2, 3, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            bool shift = (a.size() > 2) ? a[2].deref().isTruthy() : false;
            return Value(st->handleKey(static_cast<int>(a[0].deref().asDouble()), a[1].deref().toString(), shift));
        })));
        return Value(obj);
    })));

    // 12. createDisclosure(opts)
    m->set("createDisclosure", Value(std::make_shared<NativeMethodCallable>("createDisclosure", 0, 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        struct DiscState { bool open = false; Value onOpenChange = Value(std::monostate{}); };
        auto st = std::make_shared<DiscState>();
        if (args.size() > 0 && args[0].deref().isMap()) {
            auto om = args[0].deref().getMap();
            if (om->has("defaultOpen")) st->open = om->entries["defaultOpen"].deref().isTruthy();
            if (om->has("open")) st->open = om->entries["open"].deref().isTruthy();
            if (om->has("onOpenChange")) st->onOpenChange = om->entries["onOpenChange"].deref();
        }
        auto setOpen = [st](Interpreter &interp, bool v) {
            if (st->open == v) return v;
            st->open = v;
            if (st->onOpenChange.isCallable()) invokeCallableSafe(interp, st->onOpenChange, {Value(v)});
            return v;
        };
        auto obj = std::make_shared<MemeMap>();
        obj->set("isOpen", Value(std::make_shared<NativeMethodCallable>("isOpen", 0, 0, [st](Interpreter &, const std::vector<Value> &) -> Value { return Value(st->open); })));
        obj->set("open", Value(std::make_shared<NativeMethodCallable>("open", 0, 0, [setOpen](Interpreter &i, const std::vector<Value> &) -> Value { return Value(setOpen(i, true)); })));
        obj->set("close", Value(std::make_shared<NativeMethodCallable>("close", 0, 0, [setOpen](Interpreter &i, const std::vector<Value> &) -> Value { return Value(setOpen(i, false)); })));
        obj->set("toggle", Value(std::make_shared<NativeMethodCallable>("toggle", 0, 0, [st, setOpen](Interpreter &i, const std::vector<Value> &) -> Value { return Value(setOpen(i, !st->open)); })));
        obj->set("getTriggerProps", Value(std::make_shared<NativeMethodCallable>("getTriggerProps", 0, 1, [st, setOpen](Interpreter &, const std::vector<Value> &a) -> Value {
            auto p = std::make_shared<MemeMap>();
            p->set("aria-expanded", Value(st->open));
            p->set("onClick", Value(std::make_shared<NativeMethodCallable>("onClick", 0, 1, [st, setOpen](Interpreter &in2, const std::vector<Value> &) -> Value {
                return Value(setOpen(in2, !st->open));
            })));
            Value u = (a.size() > 0) ? a[0] : Value(std::monostate{});
            return mergePropMaps({Value(p), u});
        })));
        obj->set("getContentProps", Value(std::make_shared<NativeMethodCallable>("getContentProps", 0, 1, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            auto p = std::make_shared<MemeMap>();
            p->set("hidden", Value(!st->open));
            p->set("open", Value(st->open));
            Value u = (a.size() > 0) ? a[0] : Value(std::monostate{});
            return mergePropMaps({Value(p), u});
        })));
        return Value(obj);
    })));

    // 13. createTabs(opts)
    m->set("createTabs", Value(std::make_shared<NativeMethodCallable>("createTabs", 0, 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto st = std::make_shared<MemeUI::TabsState>();
        if (args.size() > 0 && args[0].deref().isMap()) {
            auto om = args[0].deref().getMap();
            if (om->has("orientation")) st->roving.orientation = om->entries["orientation"].deref().toString();
            if (om->has("activationMode")) st->activationMode = om->entries["activationMode"].deref().toString();
            if (om->has("rtl")) st->roving.rtl = om->entries["rtl"].deref().isTruthy();
            if (om->has("loop")) st->roving.loop = om->entries["loop"].deref().isTruthy();
            if (om->has("items")) st->syncItems(parseRovingItems(om->entries["items"]));
            std::string defVal = om->has("value") ? om->entries["value"].deref().toString()
                               : (om->has("defaultValue") ? om->entries["defaultValue"].deref().toString() : "");
            if (!defVal.empty()) st->clickTab(defVal);
        }
        auto obj = std::make_shared<MemeMap>();
        obj->set("setItems", Value(std::make_shared<NativeMethodCallable>("setItems", 1, 1, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            st->syncItems(parseRovingItems(a[0]));
            return Value(st->activeTab());
        })));
        obj->set("activeTab", Value(std::make_shared<NativeMethodCallable>("activeTab", 0, 0, [st](Interpreter &, const std::vector<Value> &) -> Value {
            return Value(st->activeTab());
        })));
        obj->set("clickTab", Value(std::make_shared<NativeMethodCallable>("clickTab", 1, 1, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            return Value(st->clickTab(a[0].deref().toString()));
        })));
        obj->set("handleKey", Value(std::make_shared<NativeMethodCallable>("handleKey", 1, 1, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            std::string focused;
            st->handleKey(a[0].deref().toString(), focused);
            auto res = std::make_shared<MemeMap>();
            res->set("activeTab", Value(st->activeTab()));
            res->set("focusedTab", Value(focused));
            return Value(res);
        })));
        obj->set("getTabProps", Value(std::make_shared<NativeMethodCallable>("getTabProps", 1, 2, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            std::string id = a[0].deref().toString();
            auto p = std::make_shared<MemeMap>();
            p->set("role", Value(std::string("tab")));
            p->set("selected", Value(st->activeTab() == id));
            p->set("active", Value(st->activeTab() == id));
            p->set("tabIndex", Value(static_cast<int64_t>(st->roving.getTabIndex(id))));
            p->set("onClick", Value(std::make_shared<NativeMethodCallable>("onClick", 0, 1, [st, id](Interpreter &, const std::vector<Value> &) -> Value {
                return Value(st->clickTab(id));
            })));
            Value u = (a.size() > 1) ? a[1] : Value(std::monostate{});
            return mergePropMaps({Value(p), u});
        })));
        obj->set("getPanelProps", Value(std::make_shared<NativeMethodCallable>("getPanelProps", 1, 2, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            std::string id = a[0].deref().toString();
            bool vis = (st->activeTab() == id);
            auto p = std::make_shared<MemeMap>();
            p->set("role", Value(std::string("tabpanel")));
            p->set("hidden", Value(!vis));
            Value u = (a.size() > 1) ? a[1] : Value(std::monostate{});
            return mergePropMaps({Value(p), u});
        })));
        return Value(obj);
    })));

    // 14. createSlider(opts)
    m->set("createSlider", Value(std::make_shared<NativeMethodCallable>("createSlider", 0, 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto st = std::make_shared<MemeUI::SliderState>();
        if (args.size() > 0 && args[0].deref().isMap()) {
            auto om = args[0].deref().getMap();
            if (om->has("min")) st->min = om->entries["min"].deref().asDouble();
            if (om->has("max")) st->max = om->entries["max"].deref().asDouble();
            if (om->has("step")) st->step = om->entries["step"].deref().asDouble();
            if (om->has("minStepsBetweenThumbs")) st->minStepsBetweenThumbs = static_cast<int>(om->entries["minStepsBetweenThumbs"].deref().asDouble());
            if (om->has("vertical")) st->vertical = om->entries["vertical"].deref().isTruthy();
            if (om->has("rtl")) st->rtl = om->entries["rtl"].deref().isTruthy();
            if (om->has("disabled")) st->disabled = om->entries["disabled"].deref().isTruthy();
            if (om->has("values") && om->entries["values"].deref().isList()) {
                st->values.clear();
                for (const auto &v : om->entries["values"].deref().getList()->elements) st->values.push_back(v.deref().asDouble());
            } else if (om->has("value")) {
                st->values = {om->entries["value"].deref().asDouble()};
            } else if (om->has("defaultValue")) {
                st->values = {om->entries["defaultValue"].deref().asDouble()};
            }
        }
        auto obj = std::make_shared<MemeMap>();
        obj->set("getValues", Value(std::make_shared<NativeMethodCallable>("getValues", 0, 0, [st](Interpreter &, const std::vector<Value> &) -> Value {
            auto l = std::make_shared<MemeList>();
            for (double v : st->values) l->elements.push_back(Value(v));
            return Value(l);
        })));
        obj->set("setValue", Value(std::make_shared<NativeMethodCallable>("setValue", 1, 2, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            int idx = (a.size() == 2) ? static_cast<int>(a[0].deref().asDouble()) : 0;
            double val = (a.size() == 2) ? a[1].deref().asDouble() : a[0].deref().asDouble();
            return Value(st->setThumbValue(idx, val));
        })));
        obj->set("setFromRatio", Value(std::make_shared<NativeMethodCallable>("setFromRatio", 2, 2, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            return Value(st->setFromRatio(static_cast<int>(a[0].deref().asDouble()), a[1].deref().asDouble()));
        })));
        obj->set("handleKey", Value(std::make_shared<NativeMethodCallable>("handleKey", 2, 3, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            bool shift = (a.size() > 2) ? a[2].deref().isTruthy() : false;
            return Value(st->handleKey(static_cast<int>(a[0].deref().asDouble()), a[1].deref().toString(), shift));
        })));
        return Value(obj);
    })));

    // 15. createToast(opts)
    m->set("createToast", Value(std::make_shared<NativeMethodCallable>("createToast", 0, 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto localMgr = std::make_shared<MemeUI::ToastManager>();
        bool isolated = false;
        if (args.size() > 0 && args[0].deref().isMap()) {
            auto om = args[0].deref().getMap();
            if (om->has("maxVisible")) localMgr->maxVisible = static_cast<int>(om->entries["maxVisible"].deref().asDouble());
            if (om->has("isolated")) isolated = om->entries["isolated"].deref().isTruthy();
        }
        auto getMgr = [localMgr, isolated](Interpreter &interp) -> MemeUI::ToastManager & {
            return isolated ? *localMgr : interp.uiSession.toasts;
        };
        auto obj = std::make_shared<MemeMap>();
        obj->set("add", Value(std::make_shared<NativeMethodCallable>("add", 1, 4, [getMgr](Interpreter &interp, const std::vector<Value> &a) -> Value {
            std::string title = a[0].deref().toString();
            std::string desc = (a.size() > 1) ? a[1].deref().toString() : "";
            std::string var = (a.size() > 2) ? a[2].deref().toString() : "default";
            double dur = (a.size() > 3) ? a[3].deref().asDouble() : 4000.0;
            return Value(getMgr(interp).add(title, desc, var, dur));
        })));
        obj->set("dismiss", Value(std::make_shared<NativeMethodCallable>("dismiss", 1, 1, [getMgr](Interpreter &interp, const std::vector<Value> &a) -> Value {
            getMgr(interp).dismiss(a[0].deref().toString());
            return Value(true);
        })));
        obj->set("dismissAll", Value(std::make_shared<NativeMethodCallable>("dismissAll", 0, 0, [getMgr](Interpreter &interp, const std::vector<Value> &) -> Value {
            getMgr(interp).dismissAll();
            return Value(true);
        })));
        obj->set("setPaused", Value(std::make_shared<NativeMethodCallable>("setPaused", 2, 2, [getMgr](Interpreter &interp, const std::vector<Value> &a) -> Value {
            getMgr(interp).setPaused(a[0].deref().toString(), a[1].deref().isTruthy());
            return Value(true);
        })));
        obj->set("pauseAll", Value(std::make_shared<NativeMethodCallable>("pauseAll", 1, 1, [getMgr](Interpreter &interp, const std::vector<Value> &a) -> Value {
            getMgr(interp).pauseAll(a[0].deref().isTruthy());
            return Value(true);
        })));
        obj->set("tick", Value(std::make_shared<NativeMethodCallable>("tick", 1, 1, [getMgr](Interpreter &interp, const std::vector<Value> &a) -> Value {
            return Value(getMgr(interp).tick(a[0].deref().asDouble()));
        })));
        obj->set("list", Value(std::make_shared<NativeMethodCallable>("list", 0, 0, [getMgr](Interpreter &interp, const std::vector<Value> &) -> Value {
            auto l = std::make_shared<MemeList>();
            for (const auto &t : getMgr(interp).toasts) {
                auto tm = std::make_shared<MemeMap>();
                tm->set("id", Value(t.id));
                tm->set("title", Value(t.title));
                tm->set("description", Value(t.description));
                tm->set("variant", Value(t.variant));
                tm->set("remainingMs", Value(t.remainingMs));
                tm->set("paused", Value(t.paused));
                l->elements.push_back(Value(tm));
            }
            return Value(l);
        })));
        return Value(obj);
    })));

    // 16. createTooltip(opts)
    m->set("createTooltip", Value(std::make_shared<NativeMethodCallable>("createTooltip", 0, 1, [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto st = std::make_shared<MemeUI::TooltipDelayGroup>();
        if (args.size() > 0 && args[0].deref().isMap()) {
            auto om = args[0].deref().getMap();
            if (om->has("openDelayMs")) st->openDelayMs = om->entries["openDelayMs"].deref().asDouble();
            if (om->has("skipDelayMs")) st->skipDelayMs = om->entries["skipDelayMs"].deref().asDouble();
        }
        auto obj = std::make_shared<MemeMap>();
        obj->set("onTriggerHoverStart", Value(std::make_shared<NativeMethodCallable>("onTriggerHoverStart", 1, 1, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            st->onTriggerHoverStart(a[0].deref().toString());
            return Value(st->openTriggerId);
        })));
        obj->set("onTriggerHoverEnd", Value(std::make_shared<NativeMethodCallable>("onTriggerHoverEnd", 1, 1, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            st->onTriggerHoverEnd(a[0].deref().toString());
            return Value(st->openTriggerId);
        })));
        obj->set("onTriggerFocus", Value(std::make_shared<NativeMethodCallable>("onTriggerFocus", 1, 1, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            st->onTriggerFocus(a[0].deref().toString());
            return Value(st->openTriggerId);
        })));
        obj->set("onTooltipHoverChange", Value(std::make_shared<NativeMethodCallable>("onTooltipHoverChange", 1, 1, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            st->onTooltipHoverChange(a[0].deref().isTruthy());
            return Value(st->openTriggerId);
        })));
        obj->set("handleEscape", Value(std::make_shared<NativeMethodCallable>("handleEscape", 0, 0, [st](Interpreter &, const std::vector<Value> &) -> Value {
            return Value(st->handleEscape());
        })));
        obj->set("tick", Value(std::make_shared<NativeMethodCallable>("tick", 1, 1, [st](Interpreter &, const std::vector<Value> &a) -> Value {
            return Value(st->tick(a[0].deref().asDouble()));
        })));
        obj->set("getOpenTriggerId", Value(std::make_shared<NativeMethodCallable>("getOpenTriggerId", 0, 0, [st](Interpreter &, const std::vector<Value> &) -> Value {
            return Value(st->openTriggerId);
        })));
        return Value(obj);
    })));

    // 17. createAgent() — unified UI inspection, query, action & a11y audit API
    m->set("createAgent", Value(std::make_shared<NativeMethodCallable>("createAgent", 0, 0, [](Interpreter &interp, const std::vector<Value> &) -> Value {
        auto obj = std::make_shared<MemeMap>();
        obj->set("snapshotJson", Value(std::make_shared<NativeMethodCallable>("snapshotJson", 0, 0, [](Interpreter &i2, const std::vector<Value> &) -> Value {
            return Value(i2.uiSession.agentSnapshotJson());
        })));
        obj->set("snapshot", Value(std::make_shared<NativeMethodCallable>("snapshot", 0, 0, [](Interpreter &i2, const std::vector<Value> &) -> Value {
            auto snap = std::make_shared<MemeMap>();
            snap->set("version", Value(static_cast<int64_t>(i2.uiSession.version)));
            snap->set("focusedKey", Value(i2.uiSession.interaction.focusedKey));
            snap->set("openPopups", Value(static_cast<int64_t>(i2.uiSession.popups.stack.size())));
            auto nodes = std::make_shared<MemeList>();
            for (const auto &hr : i2.uiSession.lastHits) nodes->elements.push_back(hitRegionToValue(hr));
            snap->set("nodes", Value(nodes));
            return Value(snap);
        })));
        obj->set("query", Value(std::make_shared<NativeMethodCallable>("query", 0, 1, [](Interpreter &i2, const std::vector<Value> &a) -> Value {
            std::string role, name, testId;
            if (a.size() > 0 && a[0].deref().isMap()) {
                auto qm = a[0].deref().getMap();
                if (qm->has("role")) role = qm->entries["role"].deref().toString();
                if (qm->has("name")) name = qm->entries["name"].deref().toString();
                if (qm->has("testId")) testId = qm->entries["testId"].deref().toString();
            } else if (a.size() > 0) {
                name = a[0].deref().toString();
            }
            auto list = std::make_shared<MemeList>();
            for (const auto &hr : i2.uiSession.agentQuery(role, name, testId)) list->elements.push_back(hitRegionToValue(hr));
            return Value(list);
        })));
        obj->set("getByRole", Value(std::make_shared<NativeMethodCallable>("getByRole", 1, 2, [](Interpreter &i2, const std::vector<Value> &a) -> Value {
            std::string role = a[0].deref().toString();
            std::string name = (a.size() > 1) ? a[1].deref().toString() : "";
            auto matches = i2.uiSession.agentQuery(role, name, "");
            if (matches.empty()) return Value(std::monostate{});
            return hitRegionToValue(matches.front());
        })));
        obj->set("getByText", Value(std::make_shared<NativeMethodCallable>("getByText", 1, 1, [](Interpreter &i2, const std::vector<Value> &a) -> Value {
            auto matches = i2.uiSession.agentQuery("", a[0].deref().toString(), "");
            if (matches.empty()) return Value(std::monostate{});
            return hitRegionToValue(matches.front());
        })));
        obj->set("getByTestId", Value(std::make_shared<NativeMethodCallable>("getByTestId", 1, 1, [](Interpreter &i2, const std::vector<Value> &a) -> Value {
            auto matches = i2.uiSession.agentQuery("", "", a[0].deref().toString());
            if (matches.empty()) return Value(std::monostate{});
            return hitRegionToValue(matches.front());
        })));
        obj->set("act", Value(std::make_shared<NativeMethodCallable>("act", 2, 3, [](Interpreter &i2, const std::vector<Value> &a) -> Value {
            int targetId = 0;
            std::string targetKey;
            Value tgt = a[0].deref();
            if (tgt.isNumeric()) {
                targetId = static_cast<int>(tgt.asDouble());
            } else if (tgt.isMap() && tgt.getMap()->has("id")) {
                targetId = static_cast<int>(tgt.getMap()->entries["id"].deref().asDouble());
                if (tgt.getMap()->has("key")) targetKey = tgt.getMap()->entries["key"].deref().toString();
            } else {
                std::string s = tgt.toString();
                for (const auto &hr : i2.uiSession.lastHits) {
                    if (hr.nodeKey == s || hr.testId == s || hr.accessibleName == s) {
                        targetId = hr.id;
                        targetKey = hr.nodeKey;
                        break;
                    }
                }
            }
            if (targetKey.empty() && targetId > 0) {
                if (const auto *hr = i2.uiSession.findRegionById(targetId)) targetKey = hr->nodeKey;
            }
            std::string action = a[1].deref().toString();
            std::string val = (a.size() > 2) ? a[2].deref().toString() : "";
            double numVal = (a.size() > 2 && a[2].deref().isNumeric()) ? a[2].deref().asDouble() : 0.0;

            bool ok = false;
            if (action == "focus") {
                i2.uiSession.interaction.focusedId = targetId;
                i2.uiSession.interaction.focusedKey = targetKey;
                i2.uiSession.interaction.focusVisible = true;
                ok = !targetKey.empty();
            } else if (action == "keydown" || action == "key") {
                if (!targetKey.empty()) {
                    i2.uiSession.interaction.focusedId = targetId;
                    i2.uiSession.interaction.focusedKey = targetKey;
                }
                MemeUI::UiKeyEvent kev;
                kev.key = val;
                if (val.size() == 1 && !kev.ctrl && !kev.meta) kev.text = val;
                ok = i2.uiSession.handleKeyEvent(kev);
            } else if (action == "scroll") {
                if (const auto *hr = i2.uiSession.findRegionById(targetId)) {
                    ok = i2.uiSession.handleScroll(hr->x + 4.0f, hr->y + 4.0f, 0.0f, static_cast<float>(numVal != 0.0 ? numVal : -5.0));
                }
            } else if (action == "type") {
                if (!targetKey.empty()) {
                    i2.uiSession.interaction.focusedId = targetId;
                    i2.uiSession.interaction.focusedKey = targetKey;
                }
                for (char c : val) {
                    MemeUI::UiKeyEvent kev;
                    kev.text = std::string(1, c);
                    kev.key = kev.text;
                    ok = i2.uiSession.handleKeyEvent(kev) || ok;
                }
            } else if (targetId > 0) {
                ok = i2.uiSession.dispatchEventOnNode(targetId, action, val, numVal);
            }
            i2.triggerUiRerender();
            return Value(ok);
        })));
        obj->set("tick", Value(std::make_shared<NativeMethodCallable>("tick", 1, 1, [](Interpreter &i2, const std::vector<Value> &a) -> Value {
            bool ch = i2.uiSession.tick(a[0].deref().asDouble());
            if (ch || i2.uiSession.needsRender) i2.triggerUiRerender();
            return Value(ch);
        })));
        obj->set("audit", Value(std::make_shared<NativeMethodCallable>("audit", 0, 0, [](Interpreter &i2, const std::vector<Value> &) -> Value {
            auto res = i2.uiSession.agentAssertAccessible();
            auto out = std::make_shared<MemeMap>();
            out->set("valid", Value(res.ok));
            out->set("violations", stringListToValue(res.violations));
            return Value(out);
        })));
        obj->set("assertAccessible", Value(std::make_shared<NativeMethodCallable>("assertAccessible", 0, 0, [](Interpreter &i2, const std::vector<Value> &) -> Value {
            auto res = i2.uiSession.agentAssertAccessible();
            if (!res.ok) {
                std::string msg = "Accessibility audit failed:";
                for (const auto &v : res.violations) msg += " " + v + ";";
                throw std::runtime_error(msg);
            }
            return Value(true);
        })));
        (void)interp;
        return Value(obj);
    })));

    return m;
}

} // namespace MemePrimitivesModule
