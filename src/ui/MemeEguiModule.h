#pragma once

#include "../interpreter/Value.h"
#include "MemeUiTypes.h"
#include "MemeEgui.h"
#include "MemeUiRuntime.h"
#include "MemePrimitivesModule.h"
#include <memory>
#include <string>
#include <vector>
#include <algorithm>

namespace MemeEguiModule {

inline Value wrapUiPainter(MemeUI::UiPainter p, float ox = 0.0f, float oy = 0.0f, float w = 0.0f, float h = 0.0f) {
    auto pm = std::make_shared<MemeMap>();
    pm->set("width", Value(static_cast<double>(w)));
    pm->set("height", Value(static_cast<double>(h)));

    auto rectFilledFn = [p, ox, oy](Interpreter &, const std::vector<Value> &a) -> Value {
        auto pp = p;
        float x = ox + static_cast<float>(a[0].deref().asDouble());
        float y = oy + static_cast<float>(a[1].deref().asDouble());
        float rw = static_cast<float>(a[2].deref().asDouble());
        float rh = static_cast<float>(a[3].deref().asDouble());
        float rad = (a.size() > 4 && a[4].deref().isNumeric()) ? static_cast<float>(a[4].deref().asDouble()) : 0.0f;
        std::string col = (a.size() > 5) ? a[5].deref().toString() : ((a.size() > 4 && a[4].deref().isString()) ? a[4].deref().toString() : "#ffffff");
        float al = (a.size() > 6) ? static_cast<float>(a[6].deref().asDouble()) : 1.0f;
        pp.rectFilled(x, y, rw, rh, rad, col, al);
        return Value(std::monostate{});
    };
    pm->set("rect_filled", Value(std::make_shared<NativeMethodCallable>("rect_filled", 5, 7, rectFilledFn)));
    pm->set("rectFilled", Value(std::make_shared<NativeMethodCallable>("rectFilled", 5, 7, rectFilledFn)));

    auto rectStrokeFn = [p, ox, oy](Interpreter &, const std::vector<Value> &a) -> Value {
        auto pp = p;
        float x = ox + static_cast<float>(a[0].deref().asDouble());
        float y = oy + static_cast<float>(a[1].deref().asDouble());
        float rw = static_cast<float>(a[2].deref().asDouble());
        float rh = static_cast<float>(a[3].deref().asDouble());
        float rad = static_cast<float>(a[4].deref().asDouble());
        float lw = static_cast<float>(a[5].deref().asDouble());
        std::string col = a[6].deref().toString();
        float al = (a.size() > 7) ? static_cast<float>(a[7].deref().asDouble()) : 1.0f;
        pp.rectStroke(x, y, rw, rh, rad, lw, col, al);
        return Value(std::monostate{});
    };
    pm->set("rect_stroke", Value(std::make_shared<NativeMethodCallable>("rect_stroke", 7, 8, rectStrokeFn)));
    pm->set("rectStroke", Value(std::make_shared<NativeMethodCallable>("rectStroke", 7, 8, rectStrokeFn)));

    auto circleFilledFn = [p, ox, oy](Interpreter &, const std::vector<Value> &a) -> Value {
        auto pp = p;
        float cx = ox + static_cast<float>(a[0].deref().asDouble());
        float cy = oy + static_cast<float>(a[1].deref().asDouble());
        float r = static_cast<float>(a[2].deref().asDouble());
        std::string col = a[3].deref().toString();
        float al = (a.size() > 4) ? static_cast<float>(a[4].deref().asDouble()) : 1.0f;
        pp.circleFilled(cx, cy, r, col, al);
        return Value(std::monostate{});
    };
    pm->set("circle_filled", Value(std::make_shared<NativeMethodCallable>("circle_filled", 4, 5, circleFilledFn)));
    pm->set("circleFilled", Value(std::make_shared<NativeMethodCallable>("circleFilled", 4, 5, circleFilledFn)));

    auto circleStrokeFn = [p, ox, oy](Interpreter &, const std::vector<Value> &a) -> Value {
        auto pp = p;
        float cx = ox + static_cast<float>(a[0].deref().asDouble());
        float cy = oy + static_cast<float>(a[1].deref().asDouble());
        float r = static_cast<float>(a[2].deref().asDouble());
        float lw = static_cast<float>(a[3].deref().asDouble());
        std::string col = a[4].deref().toString();
        float al = (a.size() > 5) ? static_cast<float>(a[5].deref().asDouble()) : 1.0f;
        pp.circleStroke(cx, cy, r, lw, col, al);
        return Value(std::monostate{});
    };
    pm->set("circle_stroke", Value(std::make_shared<NativeMethodCallable>("circle_stroke", 5, 6, circleStrokeFn)));
    pm->set("circleStroke", Value(std::make_shared<NativeMethodCallable>("circleStroke", 5, 6, circleStrokeFn)));

    pm->set("line", Value(std::make_shared<NativeMethodCallable>("line", 5, 7, [p, ox, oy](Interpreter &, const std::vector<Value> &a) -> Value {
        auto pp = p;
        float x1 = ox + static_cast<float>(a[0].deref().asDouble());
        float y1 = oy + static_cast<float>(a[1].deref().asDouble());
        float x2 = ox + static_cast<float>(a[2].deref().asDouble());
        float y2 = oy + static_cast<float>(a[3].deref().asDouble());
        float lw = (a.size() >= 6 && a[4].deref().isNumeric()) ? static_cast<float>(a[4].deref().asDouble()) : 1.5f;
        std::string col = (a.size() >= 6 && a[5].deref().isString()) ? a[5].deref().toString() : a[4].deref().toString();
        float al = (a.size() > 6) ? static_cast<float>(a[6].deref().asDouble()) : 1.0f;
        pp.line(x1, y1, x2, y2, lw, col, al);
        return Value(std::monostate{});
    })));

    auto parsePoints = [ox, oy](const Value &v) {
        std::vector<MemeUI::UiPoint> pts;
        Value d = v.deref();
        if (!d.isList()) return pts;
        for (const auto &el : d.getList()->elements) {
            Value ed = el.deref();
            if (ed.isList() && ed.getList()->elements.size() >= 2) {
                pts.push_back({ox + static_cast<float>(ed.getList()->elements[0].deref().asDouble()),
                               oy + static_cast<float>(ed.getList()->elements[1].deref().asDouble())});
            } else if (ed.isMap()) {
                auto em = ed.getMap();
                float px = em->has("x") ? static_cast<float>(em->entries["x"].deref().asDouble()) : 0.0f;
                float py = em->has("y") ? static_cast<float>(em->entries["y"].deref().asDouble()) : 0.0f;
                pts.push_back({ox + px, oy + py});
            }
        }
        return pts;
    };

    pm->set("polyline", Value(std::make_shared<NativeMethodCallable>("polyline", 2, 4, [p, parsePoints](Interpreter &, const std::vector<Value> &a) -> Value {
        auto pp = p;
        auto pts = parsePoints(a[0]);
        float lw = a[1].deref().isNumeric() ? static_cast<float>(a[1].deref().asDouble()) : 2.0f;
        std::string col = (a.size() > 2 && a[2].deref().isString()) ? a[2].deref().toString() : (a[1].deref().isString() ? a[1].deref().toString() : "#ffffff");
        float al = (a.size() > 3) ? static_cast<float>(a[3].deref().asDouble()) : 1.0f;
        pp.polyline(pts, lw, col, al);
        return Value(std::monostate{});
    })));

    pm->set("polygon", Value(std::make_shared<NativeMethodCallable>("polygon", 2, 3, [p, parsePoints](Interpreter &, const std::vector<Value> &a) -> Value {
        auto pp = p;
        auto pts = parsePoints(a[0]);
        std::string col = a[1].deref().toString();
        float al = (a.size() > 2) ? static_cast<float>(a[2].deref().asDouble()) : 1.0f;
        pp.polygon(pts, col, al);
        return Value(std::monostate{});
    })));

    pm->set("text", Value(std::make_shared<NativeMethodCallable>("text", 4, 8, [p, ox, oy](Interpreter &, const std::vector<Value> &a) -> Value {
        auto pp = p;
        float x = ox + static_cast<float>(a[0].deref().asDouble());
        float y = oy + static_cast<float>(a[1].deref().asDouble());
        std::string txt = a[2].deref().toString();
        float fs = a[3].deref().isNumeric() ? static_cast<float>(a[3].deref().asDouble()) : 14.0f;
        std::string col = (a.size() > 4 && a[4].deref().isString()) ? a[4].deref().toString() : (a[3].deref().isString() ? a[3].deref().toString() : "#ffffff");
        int fw = (a.size() > 5) ? static_cast<int>(a[5].deref().asDouble()) : 400;
        bool mono = (a.size() > 6) ? a[6].deref().isTruthy() : false;
        float al = (a.size() > 7) ? static_cast<float>(a[7].deref().asDouble()) : 1.0f;
        pp.text(x, y, txt, fs, col, fw, mono, al);
        return Value(std::monostate{});
    })));

    auto svgPathFn = [p, ox, oy](Interpreter &, const std::vector<Value> &a) -> Value {
        auto pp = p;
        float x = ox + static_cast<float>(a[0].deref().asDouble());
        float y = oy + static_cast<float>(a[1].deref().asDouble());
        float sw = static_cast<float>(a[2].deref().asDouble());
        float sh = static_cast<float>(a[3].deref().asDouble());
        std::string d = a[4].deref().toString();
        std::string col = a[5].deref().toString();
        bool filled = (a.size() > 6) ? a[6].deref().isTruthy() : false;
        float lw = (a.size() > 7) ? static_cast<float>(a[7].deref().asDouble()) : 1.5f;
        float al = (a.size() > 8) ? static_cast<float>(a[8].deref().asDouble()) : 1.0f;
        pp.svgPath(x, y, sw, sh, d, col, filled, lw, al);
        return Value(std::monostate{});
    };
    pm->set("svg_path", Value(std::make_shared<NativeMethodCallable>("svg_path", 6, 9, svgPathFn)));
    pm->set("svgPath", Value(std::make_shared<NativeMethodCallable>("svgPath", 6, 9, svgPathFn)));

    auto pushClipFn = [p, ox, oy](Interpreter &, const std::vector<Value> &a) -> Value {
        auto pp = p;
        pp.pushClip(ox + static_cast<float>(a[0].deref().asDouble()), oy + static_cast<float>(a[1].deref().asDouble()),
                    static_cast<float>(a[2].deref().asDouble()), static_cast<float>(a[3].deref().asDouble()));
        return Value(std::monostate{});
    };
    pm->set("push_clip", Value(std::make_shared<NativeMethodCallable>("push_clip", 4, 4, pushClipFn)));
    pm->set("pushClip", Value(std::make_shared<NativeMethodCallable>("pushClip", 4, 4, pushClipFn)));

    auto popClipFn = [p](Interpreter &, const std::vector<Value> &) -> Value {
        auto pp = p;
        pp.popClip();
        return Value(std::monostate{});
    };
    pm->set("pop_clip", Value(std::make_shared<NativeMethodCallable>("pop_clip", 0, 0, popClipFn)));
    pm->set("popClip", Value(std::make_shared<NativeMethodCallable>("popClip", 0, 0, popClipFn)));

    return Value(pm);
}

inline Value wrapEguiResponse(const MemeUI::EguiResponse &r, const Value &val) {
    auto m = std::make_shared<MemeMap>();
    m->set("id", Value(r.id));
    m->set("clicked", Value(r.clicked));
    m->set("double_clicked", Value(r.doubleClicked));
    m->set("doubleClicked", Value(r.doubleClicked));
    m->set("hovered", Value(r.hovered));
    m->set("active", Value(r.active));
    m->set("has_focus", Value(r.hasFocus));
    m->set("hasFocus", Value(r.hasFocus));
    m->set("changed", Value(r.changed));
    m->set("dragged", Value(r.dragged));
    m->set("value", val);
    auto rm = std::make_shared<MemeMap>();
    rm->set("x", Value(static_cast<double>(r.rect.x)));
    rm->set("y", Value(static_cast<double>(r.rect.y)));
    rm->set("width", Value(static_cast<double>(r.rect.width)));
    rm->set("height", Value(static_cast<double>(r.rect.height)));
    m->set("rect", Value(rm));
    return Value(m);
}

inline Value wrapEguiUi(MemeUI::EguiUi *ui) {
    auto um = std::make_shared<MemeMap>();

    auto availWFn = [ui](Interpreter &, const std::vector<Value> &) -> Value { return Value(static_cast<double>(ui->availableWidth())); };
    auto availHFn = [ui](Interpreter &, const std::vector<Value> &) -> Value { return Value(static_cast<double>(ui->availableHeight())); };
    um->set("available_width", Value(std::make_shared<NativeMethodCallable>("available_width", 0, 0, availWFn)));
    um->set("availableWidth", Value(std::make_shared<NativeMethodCallable>("availableWidth", 0, 0, availWFn)));
    um->set("available_height", Value(std::make_shared<NativeMethodCallable>("available_height", 0, 0, availHFn)));
    um->set("availableHeight", Value(std::make_shared<NativeMethodCallable>("availableHeight", 0, 0, availHFn)));

    auto addSpaceFn = [ui](Interpreter &, const std::vector<Value> &a) -> Value {
        ui->addSpace(static_cast<float>(a[0].deref().asDouble()));
        return Value(std::monostate{});
    };
    um->set("add_space", Value(std::make_shared<NativeMethodCallable>("add_space", 1, 1, addSpaceFn)));
    um->set("addSpace", Value(std::make_shared<NativeMethodCallable>("addSpace", 1, 1, addSpaceFn)));
    um->set("separator", Value(std::make_shared<NativeMethodCallable>("separator", 0, 0, [ui](Interpreter &, const std::vector<Value> &) -> Value {
        ui->separator();
        return Value(std::monostate{});
    })));
    um->set("painter", Value(std::make_shared<NativeMethodCallable>("painter", 0, 0, [ui](Interpreter &, const std::vector<Value> &) -> Value {
        return wrapUiPainter(ui->painter(), 0.0f, 0.0f, ui->availableWidth(), ui->availableHeight());
    })));

    um->set("heading", Value(std::make_shared<NativeMethodCallable>("heading", 1, 1, [ui](Interpreter &, const std::vector<Value> &a) -> Value {
        auto r = ui->heading(a[0].deref().toString());
        return wrapEguiResponse(r, Value(r.clicked));
    })));
    um->set("label", Value(std::make_shared<NativeMethodCallable>("label", 1, 1, [ui](Interpreter &, const std::vector<Value> &a) -> Value {
        auto r = ui->label(a[0].deref().toString());
        return wrapEguiResponse(r, Value(r.clicked));
    })));
    auto coloredLabelFn = [ui](Interpreter &, const std::vector<Value> &a) -> Value {
        auto r = ui->coloredLabel(a[0].deref().toString(), a[1].deref().toString());
        return wrapEguiResponse(r, Value(r.clicked));
    };
    um->set("colored_label", Value(std::make_shared<NativeMethodCallable>("colored_label", 2, 2, coloredLabelFn)));
    um->set("coloredLabel", Value(std::make_shared<NativeMethodCallable>("coloredLabel", 2, 2, coloredLabelFn)));
    um->set("monospace", Value(std::make_shared<NativeMethodCallable>("monospace", 1, 1, [ui](Interpreter &, const std::vector<Value> &a) -> Value {
        return wrapEguiResponse(ui->monospace(a[0].deref().toString()), Value(false));
    })));
    um->set("small", Value(std::make_shared<NativeMethodCallable>("small", 1, 1, [ui](Interpreter &, const std::vector<Value> &a) -> Value {
        return wrapEguiResponse(ui->small(a[0].deref().toString()), Value(false));
    })));
    um->set("strong", Value(std::make_shared<NativeMethodCallable>("strong", 1, 1, [ui](Interpreter &, const std::vector<Value> &a) -> Value {
        return wrapEguiResponse(ui->strong(a[0].deref().toString()), Value(false));
    })));
    um->set("button", Value(std::make_shared<NativeMethodCallable>("button", 1, 3, [ui](Interpreter &, const std::vector<Value> &a) -> Value {
        float mw = (a.size() > 1) ? static_cast<float>(a[1].deref().asDouble()) : 0.0f;
        float mh = (a.size() > 2) ? static_cast<float>(a[2].deref().asDouble()) : 28.0f;
        auto r = ui->button(a[0].deref().toString(), mw, mh);
        return wrapEguiResponse(r, Value(r.clicked));
    })));
    auto smallBtnFn = [ui](Interpreter &, const std::vector<Value> &a) -> Value {
        auto r = ui->smallButton(a[0].deref().toString());
        return wrapEguiResponse(r, Value(r.clicked));
    };
    um->set("small_button", Value(std::make_shared<NativeMethodCallable>("small_button", 1, 1, smallBtnFn)));
    um->set("smallButton", Value(std::make_shared<NativeMethodCallable>("smallButton", 1, 1, smallBtnFn)));

    auto selLabelFn = [ui](Interpreter &, const std::vector<Value> &a) -> Value {
        auto r = ui->selectableLabel(a[0].deref().isTruthy(), a[1].deref().toString());
        return wrapEguiResponse(r, Value(r.clicked));
    };
    um->set("selectable_label", Value(std::make_shared<NativeMethodCallable>("selectable_label", 2, 2, selLabelFn)));
    um->set("selectableLabel", Value(std::make_shared<NativeMethodCallable>("selectableLabel", 2, 2, selLabelFn)));

    auto readBoolOrRef = [](const Value &v, bool &isRefMap, std::shared_ptr<MemeMap> &refMap, std::string &refKey) -> bool {
        Value d = v.deref();
        if (d.isMap()) {
            refMap = d.getMap();
            if (refMap->has("current")) { isRefMap = true; refKey = "current"; return refMap->entries["current"].deref().isTruthy(); }
            if (refMap->has("value")) { isRefMap = true; refKey = "value"; return refMap->entries["value"].deref().isTruthy(); }
        }
        return d.isTruthy();
    };
    auto readNumOrRef = [](const Value &v, bool &isRefMap, std::shared_ptr<MemeMap> &refMap, std::string &refKey) -> double {
        Value d = v.deref();
        if (d.isMap()) {
            refMap = d.getMap();
            if (refMap->has("current")) { isRefMap = true; refKey = "current"; return refMap->entries["current"].deref().asDouble(); }
            if (refMap->has("value")) { isRefMap = true; refKey = "value"; return refMap->entries["value"].deref().asDouble(); }
        }
        return d.asDouble();
    };
    auto readStrOrRef = [](const Value &v, bool &isRefMap, std::shared_ptr<MemeMap> &refMap, std::string &refKey) -> std::string {
        Value d = v.deref();
        if (d.isMap()) {
            refMap = d.getMap();
            if (refMap->has("current")) { isRefMap = true; refKey = "current"; return refMap->entries["current"].deref().toString(); }
            if (refMap->has("value")) { isRefMap = true; refKey = "value"; return refMap->entries["value"].deref().toString(); }
        }
        return d.toString();
    };

    um->set("checkbox", Value(std::make_shared<NativeMethodCallable>("checkbox", 2, 3, [ui, readBoolOrRef](Interpreter &, const std::vector<Value> &a) -> Value {
        std::string id = (a.size() >= 3) ? a[0].deref().toString() : a[1].deref().toString();
        bool isRef = false; std::shared_ptr<MemeMap> rm; std::string rk;
        bool val = readBoolOrRef((a.size() >= 3) ? a[1] : a[0], isRef, rm, rk);
        std::string lbl = (a.size() >= 3) ? a[2].deref().toString() : a[1].deref().toString();
        auto r = ui->checkbox(id, val, lbl);
        if (isRef && r.changed) rm->set(rk, Value(val));
        return wrapEguiResponse(r, Value(val));
    })));

    auto toggleFn = [ui, readBoolOrRef](Interpreter &, const std::vector<Value> &a) -> Value {
        std::string id = (a.size() >= 3) ? a[0].deref().toString() : a[1].deref().toString();
        bool isRef = false; std::shared_ptr<MemeMap> rm; std::string rk;
        bool val = readBoolOrRef((a.size() >= 3) ? a[1] : a[0], isRef, rm, rk);
        std::string lbl = (a.size() >= 3) ? a[2].deref().toString() : a[1].deref().toString();
        auto r = ui->toggleSwitch(id, val, lbl);
        if (isRef && r.changed) rm->set(rk, Value(val));
        return wrapEguiResponse(r, Value(val));
    };
    um->set("toggle", Value(std::make_shared<NativeMethodCallable>("toggle", 2, 3, toggleFn)));
    um->set("toggle_switch", Value(std::make_shared<NativeMethodCallable>("toggle_switch", 2, 3, toggleFn)));
    um->set("toggleSwitch", Value(std::make_shared<NativeMethodCallable>("toggleSwitch", 2, 3, toggleFn)));

    auto radioFn = [ui](Interpreter &, const std::vector<Value> &a) -> Value {
        std::string id = (a.size() >= 3) ? a[0].deref().toString() : a[1].deref().toString();
        bool sel = (a.size() >= 3) ? a[1].deref().isTruthy() : a[0].deref().isTruthy();
        std::string lbl = (a.size() >= 3) ? a[2].deref().toString() : a[1].deref().toString();
        auto r = ui->radioValue(id, sel, lbl);
        return wrapEguiResponse(r, Value(r.clicked));
    };
    um->set("radio", Value(std::make_shared<NativeMethodCallable>("radio", 2, 3, radioFn)));
    um->set("radio_value", Value(std::make_shared<NativeMethodCallable>("radio_value", 2, 3, radioFn)));
    um->set("radioValue", Value(std::make_shared<NativeMethodCallable>("radioValue", 2, 3, radioFn)));

    um->set("slider", Value(std::make_shared<NativeMethodCallable>("slider", 4, 6, [ui, readNumOrRef](Interpreter &, const std::vector<Value> &a) -> Value {
        std::string id = a[0].deref().toString();
        bool isRef = false; std::shared_ptr<MemeMap> rm; std::string rk;
        double val = readNumOrRef(a[1], isRef, rm, rk);
        double minV = a[2].deref().asDouble();
        double maxV = a[3].deref().asDouble();
        std::string lbl = (a.size() > 4) ? a[4].deref().toString() : "";
        double step = (a.size() > 5) ? a[5].deref().asDouble() : 0.0;
        auto r = ui->slider(id, val, minV, maxV, lbl, step);
        if (isRef && r.changed) rm->set(rk, Value(val));
        return wrapEguiResponse(r, Value(val));
    })));

    auto dragValFn = [ui, readNumOrRef](Interpreter &, const std::vector<Value> &a) -> Value {
        std::string id = a[0].deref().toString();
        bool isRef = false; std::shared_ptr<MemeMap> rm; std::string rk;
        double val = readNumOrRef(a[1], isRef, rm, rk);
        double minV = (a.size() > 2) ? a[2].deref().asDouble() : -1e9;
        double maxV = (a.size() > 3) ? a[3].deref().asDouble() : 1e9;
        double speed = (a.size() > 4) ? a[4].deref().asDouble() : 0.5;
        auto r = ui->dragValue(id, val, minV, maxV, speed);
        if (isRef && r.changed) rm->set(rk, Value(val));
        return wrapEguiResponse(r, Value(val));
    };
    um->set("drag_value", Value(std::make_shared<NativeMethodCallable>("drag_value", 2, 5, dragValFn)));
    um->set("dragValue", Value(std::make_shared<NativeMethodCallable>("dragValue", 2, 5, dragValFn)));

    auto progFn = [ui](Interpreter &, const std::vector<Value> &a) -> Value {
        double frac = a[0].deref().asDouble();
        std::string txt = (a.size() > 1) ? a[1].deref().toString() : "";
        auto r = ui->progressBar(frac, txt);
        return wrapEguiResponse(r, Value(frac));
    };
    um->set("progress_bar", Value(std::make_shared<NativeMethodCallable>("progress_bar", 1, 2, progFn)));
    um->set("progressBar", Value(std::make_shared<NativeMethodCallable>("progressBar", 1, 2, progFn)));

    auto textEditFn = [ui, readStrOrRef](Interpreter &, const std::vector<Value> &a) -> Value {
        std::string id = a[0].deref().toString();
        bool isRef = false; std::shared_ptr<MemeMap> rm; std::string rk;
        std::string txt = readStrOrRef(a[1], isRef, rm, rk);
        std::string ph = (a.size() > 2) ? a[2].deref().toString() : "";
        bool pw = (a.size() > 3) ? a[3].deref().isTruthy() : false;
        float w = (a.size() > 4) ? static_cast<float>(a[4].deref().asDouble()) : 0.0f;
        auto r = ui->textEditSingleline(id, txt, ph, pw, w);
        if (isRef && r.changed) rm->set(rk, Value(txt));
        return wrapEguiResponse(r, Value(txt));
    };
    um->set("text_edit", Value(std::make_shared<NativeMethodCallable>("text_edit", 2, 5, textEditFn)));
    um->set("text_edit_singleline", Value(std::make_shared<NativeMethodCallable>("text_edit_singleline", 2, 5, textEditFn)));
    um->set("textEditSingleline", Value(std::make_shared<NativeMethodCallable>("textEditSingleline", 2, 5, textEditFn)));

    auto textAreaFn = [ui, readStrOrRef](Interpreter &, const std::vector<Value> &a) -> Value {
        std::string id = a[0].deref().toString();
        bool isRef = false; std::shared_ptr<MemeMap> rm; std::string rk;
        std::string txt = readStrOrRef(a[1], isRef, rm, rk);
        std::string ph = (a.size() > 2) ? a[2].deref().toString() : "";
        float w = (a.size() > 3) ? static_cast<float>(a[3].deref().asDouble()) : 0.0f;
        float h = (a.size() > 4) ? static_cast<float>(a[4].deref().asDouble()) : 96.0f;
        auto r = ui->textEditMultiline(id, txt, ph, w, h);
        if (isRef && r.changed) rm->set(rk, Value(txt));
        return wrapEguiResponse(r, Value(txt));
    };
    um->set("text_edit_multiline", Value(std::make_shared<NativeMethodCallable>("text_edit_multiline", 2, 5, textAreaFn)));
    um->set("textEditMultiline", Value(std::make_shared<NativeMethodCallable>("textEditMultiline", 2, 5, textAreaFn)));

    auto comboFn = [ui, readStrOrRef](Interpreter &, const std::vector<Value> &a) -> Value {
        std::string id = a[0].deref().toString();
        bool isRef = false; std::shared_ptr<MemeMap> rm; std::string rk;
        std::string sel = readStrOrRef(a[1], isRef, rm, rk);
        std::vector<std::string> opts = MemePrimitivesModule::valueToStringList(a[2]);
        auto r = ui->comboBox(id, sel, opts);
        if (isRef && r.changed) rm->set(rk, Value(sel));
        return wrapEguiResponse(r, Value(sel));
    };
    um->set("combo_box", Value(std::make_shared<NativeMethodCallable>("combo_box", 3, 3, comboFn)));
    um->set("comboBox", Value(std::make_shared<NativeMethodCallable>("comboBox", 3, 3, comboFn)));

    um->set("spinner", Value(std::make_shared<NativeMethodCallable>("spinner", 0, 1, [ui](Interpreter &, const std::vector<Value> &a) -> Value {
        ui->spinner((a.size() > 0) ? static_cast<float>(a[0].deref().asDouble()) : 18.0f);
        return Value(std::monostate{});
    })));

    um->set("horizontal", Value(std::make_shared<NativeMethodCallable>("horizontal", 1, 1, [ui](Interpreter &interp, const std::vector<Value> &a) -> Value {
        ui->horizontal([&](MemeUI::EguiUi &sub) {
            MemePrimitivesModule::invokeCallableSafe(interp, a[0], {wrapEguiUi(&sub)});
        });
        return Value(std::monostate{});
    })));
    um->set("vertical", Value(std::make_shared<NativeMethodCallable>("vertical", 1, 1, [ui](Interpreter &interp, const std::vector<Value> &a) -> Value {
        ui->vertical([&](MemeUI::EguiUi &sub) {
            MemePrimitivesModule::invokeCallableSafe(interp, a[0], {wrapEguiUi(&sub)});
        });
        return Value(std::monostate{});
    })));
    um->set("group", Value(std::make_shared<NativeMethodCallable>("group", 1, 1, [ui](Interpreter &interp, const std::vector<Value> &a) -> Value {
        ui->group([&](MemeUI::EguiUi &sub) {
            MemePrimitivesModule::invokeCallableSafe(interp, a[0], {wrapEguiUi(&sub)});
        });
        return Value(std::monostate{});
    })));
    um->set("collapsing", Value(std::make_shared<NativeMethodCallable>("collapsing", 2, 4, [ui](Interpreter &interp, const std::vector<Value> &a) -> Value {
        std::string id = a[0].deref().toString();
        std::string hdr = id;
        bool defOpen = false;
        if (a.size() == 3) {
            if (std::holds_alternative<bool>(a[1].deref().data)) defOpen = a[1].deref().isTruthy();
            else hdr = a[1].deref().toString();
        } else if (a.size() >= 4) {
            hdr = a[1].deref().toString();
            defOpen = a[2].deref().isTruthy();
        }
        Value cb = a.back();
        ui->collapsing(id, hdr, defOpen, [&](MemeUI::EguiUi &sub) {
            MemePrimitivesModule::invokeCallableSafe(interp, cb, {wrapEguiUi(&sub)});
        });
        return Value(std::monostate{});
    })));
    auto scrollAreaFn = [ui](Interpreter &interp, const std::vector<Value> &a) -> Value {
        std::string id = (a.size() == 3) ? a[0].deref().toString() : "scroll";
        float maxH = (a.size() == 3) ? static_cast<float>(a[1].deref().asDouble()) : ((a.size() == 2 && a[0].deref().isNumeric()) ? static_cast<float>(a[0].deref().asDouble()) : 240.0f);
        Value cb = a.back();
        ui->scrollArea(id, maxH, [&](MemeUI::EguiUi &sub) {
            MemePrimitivesModule::invokeCallableSafe(interp, cb, {wrapEguiUi(&sub)});
        });
        return Value(std::monostate{});
    };
    um->set("scroll_area", Value(std::make_shared<NativeMethodCallable>("scroll_area", 1, 3, scrollAreaFn)));
    um->set("scrollArea", Value(std::make_shared<NativeMethodCallable>("scrollArea", 1, 3, scrollAreaFn)));

    um->set("columns", Value(std::make_shared<NativeMethodCallable>("columns", 2, 2, [ui](Interpreter &interp, const std::vector<Value> &a) -> Value {
        int n = static_cast<int>(a[0].deref().asDouble());
        ui->columns(n, [&](std::vector<MemeUI::EguiUi> &cols) {
            auto cl = std::make_shared<MemeList>();
            for (auto &c : cols) cl->elements.push_back(wrapEguiUi(&c));
            MemePrimitivesModule::invokeCallableSafe(interp, a[1], {Value(cl)});
        });
        return Value(std::monostate{});
    })));

    return Value(um);
}

inline Value wrapEguiContext(MemeUI::EguiContext *ctx) {
    auto cm = std::make_shared<MemeMap>();

    auto topFn = [ctx](Interpreter &interp, const std::vector<Value> &a) -> Value {
        std::string id = (a.size() >= 2 && a[0].deref().isString()) ? a[0].deref().toString() : "top_panel";
        float h = (a.size() >= 3) ? static_cast<float>(a[1].deref().asDouble()) : ((a.size() == 2 && a[0].deref().isNumeric()) ? static_cast<float>(a[0].deref().asDouble()) : 44.0f);
        Value cb = a.back();
        ctx->topPanel(id, h, [&](MemeUI::EguiUi &ui) { MemePrimitivesModule::invokeCallableSafe(interp, cb, {wrapEguiUi(&ui)}); });
        return Value(std::monostate{});
    };
    cm->set("top_panel", Value(std::make_shared<NativeMethodCallable>("top_panel", 1, 3, topFn)));
    cm->set("topPanel", Value(std::make_shared<NativeMethodCallable>("topPanel", 1, 3, topFn)));

    auto bottomFn = [ctx](Interpreter &interp, const std::vector<Value> &a) -> Value {
        std::string id = (a.size() >= 2 && a[0].deref().isString()) ? a[0].deref().toString() : "bottom_panel";
        float h = (a.size() >= 3) ? static_cast<float>(a[1].deref().asDouble()) : 48.0f;
        Value cb = a.back();
        ctx->bottomPanel(id, h, [&](MemeUI::EguiUi &ui) { MemePrimitivesModule::invokeCallableSafe(interp, cb, {wrapEguiUi(&ui)}); });
        return Value(std::monostate{});
    };
    cm->set("bottom_panel", Value(std::make_shared<NativeMethodCallable>("bottom_panel", 1, 3, bottomFn)));
    cm->set("bottomPanel", Value(std::make_shared<NativeMethodCallable>("bottomPanel", 1, 3, bottomFn)));

    auto leftFn = [ctx](Interpreter &interp, const std::vector<Value> &a) -> Value {
        std::string id = (a.size() >= 2 && a[0].deref().isString()) ? a[0].deref().toString() : "left_panel";
        float w = (a.size() >= 3) ? static_cast<float>(a[1].deref().asDouble()) : 200.0f;
        Value cb = a.back();
        ctx->leftPanel(id, w, [&](MemeUI::EguiUi &ui) { MemePrimitivesModule::invokeCallableSafe(interp, cb, {wrapEguiUi(&ui)}); });
        return Value(std::monostate{});
    };
    cm->set("left_panel", Value(std::make_shared<NativeMethodCallable>("left_panel", 1, 3, leftFn)));
    cm->set("leftPanel", Value(std::make_shared<NativeMethodCallable>("leftPanel", 1, 3, leftFn)));

    auto rightFn = [ctx](Interpreter &interp, const std::vector<Value> &a) -> Value {
        std::string id = (a.size() >= 2 && a[0].deref().isString()) ? a[0].deref().toString() : "right_panel";
        float w = (a.size() >= 3) ? static_cast<float>(a[1].deref().asDouble()) : 200.0f;
        Value cb = a.back();
        ctx->rightPanel(id, w, [&](MemeUI::EguiUi &ui) { MemePrimitivesModule::invokeCallableSafe(interp, cb, {wrapEguiUi(&ui)}); });
        return Value(std::monostate{});
    };
    cm->set("right_panel", Value(std::make_shared<NativeMethodCallable>("right_panel", 1, 3, rightFn)));
    cm->set("rightPanel", Value(std::make_shared<NativeMethodCallable>("rightPanel", 1, 3, rightFn)));

    auto centralFn = [ctx](Interpreter &interp, const std::vector<Value> &a) -> Value {
        ctx->centralPanel([&](MemeUI::EguiUi &ui) { MemePrimitivesModule::invokeCallableSafe(interp, a[0], {wrapEguiUi(&ui)}); });
        return Value(std::monostate{});
    };
    cm->set("central_panel", Value(std::make_shared<NativeMethodCallable>("central_panel", 1, 1, centralFn)));
    cm->set("centralPanel", Value(std::make_shared<NativeMethodCallable>("centralPanel", 1, 1, centralFn)));

    cm->set("window", Value(std::make_shared<NativeMethodCallable>("window", 2, 6, [ctx](Interpreter &interp, const std::vector<Value> &a) -> Value {
        std::string id = "win_default", title = a[0].deref().toString();
        float dx = 60.0f, dy = 60.0f, w = 280.0f;
        if (a.size() == 2) {
            id = "win_" + title;
        } else if (a.size() == 3) {
            id = "win_" + title;
            w = static_cast<float>(a[1].deref().asDouble());
        } else if (a.size() == 4) {
            id = "win_" + title;
            dx = static_cast<float>(a[1].deref().asDouble());
            dy = static_cast<float>(a[2].deref().asDouble());
        } else if (a.size() == 5) {
            id = "win_" + title;
            dx = static_cast<float>(a[1].deref().asDouble());
            dy = static_cast<float>(a[2].deref().asDouble());
            w = static_cast<float>(a[3].deref().asDouble());
        } else if (a.size() >= 6) {
            id = a[0].deref().toString();
            title = a[1].deref().toString();
            dx = static_cast<float>(a[2].deref().asDouble());
            dy = static_cast<float>(a[3].deref().asDouble());
            w = static_cast<float>(a[4].deref().asDouble());
        }
        Value cb = a.back();
        ctx->window(id, title, dx, dy, w, [&](MemeUI::EguiUi &ui) { MemePrimitivesModule::invokeCallableSafe(interp, cb, {wrapEguiUi(&ui)}); });
        return Value(std::monostate{});
    })));

    cm->set("elapsed_ms", Value(std::make_shared<NativeMethodCallable>("elapsed_ms", 0, 0, [ctx](Interpreter &, const std::vector<Value> &) -> Value {
        return Value(ctx->input.elapsedMs);
    })));
    cm->set("delta_ms", Value(std::make_shared<NativeMethodCallable>("delta_ms", 0, 0, [ctx](Interpreter &, const std::vector<Value> &) -> Value {
        return Value(ctx->input.deltaMs);
    })));

    return Value(cm);
}

} // namespace MemeEguiModule
