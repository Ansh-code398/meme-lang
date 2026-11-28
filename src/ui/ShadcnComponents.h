#ifndef SHADCN_COMPONENTS_H
#define SHADCN_COMPONENTS_H

#include "../interpreter/Value.h"
#include "../interpreter/Callable.h"
#include "../platform/MemePlatform.h"
#include "MemeIconsRegistry.h"
#include <string>
#include <memory>
#include <vector>

namespace ShadcnComponents
{

inline Value createJsxElement(
    const std::string &tag,
    const std::string &classes,
    const std::shared_ptr<MemeMap> &extraProps,
    const Value &children)
{
    auto node = std::make_shared<MemeMap>();
    node->set("type", Value("jsx_element"));
    node->set("tag", Value(tag));

    auto props = std::make_shared<MemeMap>();
    if (extraProps) {
        for (const auto &[k, v] : extraProps->entries) {
            props->set(k, v);
        }
    }
    std::string existingClass = props->has("class") ? props->entries["class"].deref().toString() : "";
    if (existingClass.empty() && props->has("className")) {
        existingClass = props->entries["className"].deref().toString();
    }

    std::string merged = classes;
    if (!existingClass.empty()) {
        merged += " " + existingClass;
    }
    props->set("class", Value(merged));
    props->set("className", Value(merged));

    node->set("props", Value(props));
    node->set("class", Value(merged));
    node->set("children", children);
    return Value(node);
}

inline Value createPlatformComponent()
{
    auto platformFn = [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        auto matchesVal = [](const Value &v) -> bool {
            Value d = v.deref();
            if (d.isList()) {
                for (const auto &it : d.getList()->elements) {
                    if (MemePlatform::matchPlatformSpec(it.deref().toString())) return true;
                }
                return false;
            }
            return MemePlatform::matchPlatformSpec(d.toString());
        };
        bool matched = true;
        if (props->has("only")) matched = matched && matchesVal(props->entries["only"]);
        if (props->has("os")) matched = matched && matchesVal(props->entries["os"]);
        if (props->has("target")) matched = matched && matchesVal(props->entries["target"]);
        if (props->has("is")) matched = matched && matchesVal(props->entries["is"]);

        static const char *kFlags[] = {
            "ios", "android", "mac", "macos", "windows", "win",
            "linux", "web", "wasm", "mobile", "desktop", "apple", "native"
        };
        bool hasPosFlag = false;
        bool posFlagHit = false;
        for (const char *fk : kFlags) {
            if (props->has(fk)) {
                if (props->entries[fk].deref().isTruthy()) {
                    hasPosFlag = true;
                    if (MemePlatform::matchPlatformSpec(fk)) posFlagHit = true;
                } else {
                    if (MemePlatform::matchPlatformSpec(fk)) matched = false;
                }
            }
        }
        if (hasPosFlag) matched = matched && posFlagHit;

        if (props->has("exclude") && matchesVal(props->entries["exclude"])) matched = false;
        if (props->has("not") && matchesVal(props->entries["not"])) matched = false;

        if (matched) {
            Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
            return createJsxElement("fragment", "", std::make_shared<MemeMap>(), children);
        }
        if (props->has("fallback")) {
            Value fb = props->entries["fallback"].deref();
            if (std::holds_alternative<std::shared_ptr<Callable>>(fb.data)) {
                return std::get<std::shared_ptr<Callable>>(fb.data)->call(interp, {});
            }
            return fb;
        }
        return Value(std::monostate{});
    };
    return Value(std::make_shared<NativeMethodCallable>("Platform", 0, 1, platformFn));
}

inline std::shared_ptr<MemeMap> createShadcnModule()
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    // Card: rounded-2xl border border-slate-800 bg-slate-900/90 text-slate-100 shadow-sm
    auto cardFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
        return createJsxElement("div", "rounded-2xl border border-slate-800 bg-slate-900/90 text-slate-100", props, children);
    };
    m->set("Card", Value(std::make_shared<NativeMethodCallable>("Card", 0, 1, cardFn)));

    // CardHeader: flex flex-col space-y-1.5 p-6 pb-2
    auto cardHeaderFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
        return createJsxElement("div", "flex flex-col gap-1.5 p-6 pb-2", props, children);
    };
    m->set("CardHeader", Value(std::make_shared<NativeMethodCallable>("CardHeader", 0, 1, cardHeaderFn)));

    // CardTitle: text-2xl font-semibold leading-none tracking-tight
    auto cardTitleFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
        return createJsxElement("h3", "text-2xl font-semibold tracking-tight text-white", props, children);
    };
    m->set("CardTitle", Value(std::make_shared<NativeMethodCallable>("CardTitle", 0, 1, cardTitleFn)));

    // CardContent: p-6 pt-0
    auto cardContentFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
        return createJsxElement("div", "p-6 pt-0", props, children);
    };
    m->set("CardContent", Value(std::make_shared<NativeMethodCallable>("CardContent", 0, 1, cardContentFn)));

    // Button: inline-flex items-center justify-center rounded-xl text-sm font-medium
    auto buttonFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
        std::string variant = props->has("variant") ? props->entries["variant"].deref().toString() : "default";

        std::string vClass = "bg-indigo-600 text-white hover:bg-indigo-500";
        if (variant == "destructive") vClass = "bg-rose-600 text-white hover:bg-rose-500";
        else if (variant == "outline") vClass = "border border-slate-700 bg-transparent hover:bg-slate-800 text-slate-100";
        else if (variant == "secondary") vClass = "bg-slate-800 text-slate-100 hover:bg-slate-700";
        else if (variant == "ghost") vClass = "hover:bg-slate-800 text-slate-100";

        std::string baseClass = "inline-flex items-center justify-center rounded-xl font-medium px-4 py-2 text-sm " + vClass;
        return createJsxElement("button", baseClass, props, children);
    };
    m->set("Button", Value(std::make_shared<NativeMethodCallable>("Button", 0, 1, buttonFn)));

    // Badge: inline-flex items-center rounded-full border px-2.5 py-0.5 text-xs font-semibold
    auto badgeFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
        std::string baseClass = "inline-flex items-center rounded-full px-2.5 py-0.5 text-xs font-semibold";
        return createJsxElement("span", baseClass, props, children);
    };
    m->set("Badge", Value(std::make_shared<NativeMethodCallable>("Badge", 0, 1, badgeFn)));

    // Input: flex h-10 w-full rounded-xl border border-slate-800 bg-slate-950 px-3 py-2 text-sm
    auto inputFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        std::string baseClass = "flex h-10 w-full rounded-xl border border-slate-800 bg-slate-950 px-3 py-2 text-sm text-white";
        return createJsxElement("input", baseClass, props, Value(std::make_shared<MemeList>()));
    };
    m->set("Input", Value(std::make_shared<NativeMethodCallable>("Input", 0, 1, inputFn)));

    // Separator: shrink-0 bg-slate-800 h-[1px] w-full
    auto separatorFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        return createJsxElement("div", "h-[1px] w-full bg-slate-800 my-2", props, Value(std::make_shared<MemeList>()));
    };
    m->set("Separator", Value(std::make_shared<NativeMethodCallable>("Separator", 0, 1, separatorFn)));

    // Alert: relative w-full rounded-xl border p-4
    auto alertFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
        return createJsxElement("div", "relative w-full rounded-xl border border-slate-800 p-4 bg-slate-950", props, children);
    };
    m->set("Alert", Value(std::make_shared<NativeMethodCallable>("Alert", 0, 1, alertFn)));

    // CardDescription & CardFooter
    auto cardDescFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
        return createJsxElement("p", "text-sm text-slate-400", props, children);
    };
    m->set("CardDescription", Value(std::make_shared<NativeMethodCallable>("CardDescription", 0, 1, cardDescFn)));

    auto cardFooterFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
        return createJsxElement("div", "flex items-center p-6 pt-0 gap-2", props, children);
    };
    m->set("CardFooter", Value(std::make_shared<NativeMethodCallable>("CardFooter", 0, 1, cardFooterFn)));

    // Textarea
    auto textareaFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
        return createJsxElement("textarea", "flex min-h-[80px] w-full rounded-xl border border-slate-800 bg-slate-950 px-3 py-2 text-sm text-slate-100", props, children);
    };
    m->set("Textarea", Value(std::make_shared<NativeMethodCallable>("Textarea", 0, 1, textareaFn)));

    auto addLabelClickToggle = [](const std::shared_ptr<MemeMap> &props, const std::shared_ptr<MemeMap> &spanProps) {
        spanProps->set("class", Value(std::string("text-sm text-slate-200 cursor-pointer")));
        bool isDis = props->has("disabled") && props->entries["disabled"].deref().isTruthy();
        if (isDis) return;
        Value cbVal;
        if (props->has("onChange") && props->entries["onChange"].deref().isCallable()) cbVal = props->entries["onChange"].deref();
        else if (props->has("onchange") && props->entries["onchange"].deref().isCallable()) cbVal = props->entries["onchange"].deref();
        else if (props->has("onClick") && props->entries["onClick"].deref().isCallable()) cbVal = props->entries["onClick"].deref();
        else if (props->has("onclick") && props->entries["onclick"].deref().isCallable()) cbVal = props->entries["onclick"].deref();
        if (cbVal.isCallable()) {
            bool curChecked = props->has("checked") && props->entries["checked"].deref().isTruthy();
            auto fn = [cbVal, curChecked](Interpreter &interp, const std::vector<Value> &) -> Value {
                auto ev = std::make_shared<MemeMap>();
                ev->set("type", Value(std::string("click")));
                ev->set("checked", Value(!curChecked));
                auto tgt = std::make_shared<MemeMap>();
                tgt->set("checked", Value(!curChecked));
                ev->set("target", Value(tgt));
                cbVal.getCallable()->call(interp, {Value(ev)});
                return Value();
            };
            spanProps->set("onClick", Value(std::make_shared<NativeMethodCallable>("labelToggle", 0, 1, fn)));
        }
    };

    // Checkbox
    auto checkboxFn = [addLabelClickToggle](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        Value emptyChildren(std::make_shared<MemeList>());
        if (props->has("label") || (props->has("children") && props->entries["children"].deref().isList() && !props->entries["children"].deref().getList()->elements.empty())) {
            auto rowChildren = std::make_shared<MemeList>();
            auto boxProps = std::make_shared<MemeMap>();
            boxProps->entries = props->entries;
            boxProps->entries.erase("class");
            boxProps->entries.erase("className");
            if (!boxProps->has("name") && props->has("label")) boxProps->set("name", props->entries["label"]);
            rowChildren->elements.push_back(createJsxElement("checkbox", "shrink-0", boxProps, emptyChildren));
            auto spanProps = std::make_shared<MemeMap>();
            addLabelClickToggle(props, spanProps);
            if (props->has("label")) {
                auto spanList = std::make_shared<MemeList>();
                spanList->elements.push_back(props->entries["label"]);
                rowChildren->elements.push_back(createJsxElement("span", "text-sm text-slate-200", spanProps, Value(spanList)));
            } else {
                auto spanList = std::make_shared<MemeList>();
                spanList->elements = props->entries["children"].deref().getList()->elements;
                rowChildren->elements.push_back(createJsxElement("span", "text-sm text-slate-200", spanProps, Value(spanList)));
            }
            auto wrapProps = std::make_shared<MemeMap>();
            if (props->has("class")) wrapProps->set("class", props->entries["class"]);
            if (props->has("className")) wrapProps->set("className", props->entries["className"]);
            return createJsxElement("div", "inline-flex items-center gap-2", wrapProps, Value(rowChildren));
        }
        return createJsxElement("checkbox", "", props, emptyChildren);
    };
    m->set("Checkbox", Value(std::make_shared<NativeMethodCallable>("Checkbox", 0, 1, checkboxFn)));

    // Switch
    auto switchFn = [addLabelClickToggle](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        Value emptyChildren(std::make_shared<MemeList>());
        if (props->has("label")) {
            auto rowChildren = std::make_shared<MemeList>();
            auto swProps = std::make_shared<MemeMap>();
            swProps->entries = props->entries;
            swProps->entries.erase("class");
            swProps->entries.erase("className");
            if (!swProps->has("name")) swProps->set("name", props->entries["label"]);
            rowChildren->elements.push_back(createJsxElement("switch", "shrink-0", swProps, emptyChildren));
            auto spanProps = std::make_shared<MemeMap>();
            addLabelClickToggle(props, spanProps);
            auto spanList = std::make_shared<MemeList>();
            spanList->elements.push_back(props->entries["label"]);
            rowChildren->elements.push_back(createJsxElement("span", "text-sm text-slate-200", spanProps, Value(spanList)));
            return createJsxElement("div", "inline-flex items-center gap-2", std::make_shared<MemeMap>(), Value(rowChildren));
        }
        return createJsxElement("switch", "", props, emptyChildren);
    };
    m->set("Switch", Value(std::make_shared<NativeMethodCallable>("Switch", 0, 1, switchFn)));

    // RadioGroup & RadioItem
    auto radioGroupFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        if (!props->has("role")) props->set("role", Value(std::string("radiogroup")));
        Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
        return createJsxElement("div", "flex flex-col gap-2", props, children);
    };
    m->set("RadioGroup", Value(std::make_shared<NativeMethodCallable>("RadioGroup", 0, 1, radioGroupFn)));

    auto radioItemFn = [addLabelClickToggle](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        Value emptyChildren(std::make_shared<MemeList>());
        if (props->has("label")) {
            auto rowChildren = std::make_shared<MemeList>();
            auto rProps = std::make_shared<MemeMap>();
            rProps->entries = props->entries;
            if (!rProps->has("name")) rProps->set("name", props->entries["label"]);
            rowChildren->elements.push_back(createJsxElement("radio", "shrink-0", rProps, emptyChildren));
            auto spanProps = std::make_shared<MemeMap>();
            addLabelClickToggle(props, spanProps);
            auto spanList = std::make_shared<MemeList>();
            spanList->elements.push_back(props->entries["label"]);
            rowChildren->elements.push_back(createJsxElement("span", "text-sm text-slate-200", spanProps, Value(spanList)));
            return createJsxElement("div", "inline-flex items-center gap-2", std::make_shared<MemeMap>(), Value(rowChildren));
        }
        return createJsxElement("radio", "", props, emptyChildren);
    };
    m->set("RadioItem", Value(std::make_shared<NativeMethodCallable>("RadioItem", 0, 1, radioItemFn)));

    // Slider & Progress
    auto sliderFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        return createJsxElement("slider", "w-full h-6", props, Value(std::make_shared<MemeList>()));
    };
    m->set("Slider", Value(std::make_shared<NativeMethodCallable>("Slider", 0, 1, sliderFn)));

    auto progressFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        return createJsxElement("progress", "w-full h-2.5 rounded-full bg-slate-800", props, Value(std::make_shared<MemeList>()));
    };
    m->set("Progress", Value(std::make_shared<NativeMethodCallable>("Progress", 0, 1, progressFn)));

    // Select & Combobox
    auto selectFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
        return createJsxElement("select", "inline-flex items-center justify-between rounded-xl border border-slate-700 bg-slate-900 px-3 py-2 text-sm text-slate-100", props, children);
    };
    m->set("Select", Value(std::make_shared<NativeMethodCallable>("Select", 0, 1, selectFn)));

    auto comboboxFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
        return createJsxElement("combobox", "inline-flex items-center justify-between rounded-xl border border-slate-700 bg-slate-900 px-3 py-2 text-sm text-slate-100", props, children);
    };
    m->set("Combobox", Value(std::make_shared<NativeMethodCallable>("Combobox", 0, 1, comboboxFn)));

    // Accordion, Tabs, TabsList, TabsTrigger, TabsContent
    auto accordionFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
        return createJsxElement("div", "flex flex-col divide-y divide-slate-800 rounded-xl border border-slate-800 bg-slate-900/60", props, children);
    };
    m->set("Accordion", Value(std::make_shared<NativeMethodCallable>("Accordion", 0, 1, accordionFn)));

    auto tabsFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
        return createJsxElement("div", "flex flex-col gap-2", props, children);
    };
    m->set("Tabs", Value(std::make_shared<NativeMethodCallable>("Tabs", 0, 1, tabsFn)));

    auto tabsListFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        if (!props->has("role")) props->set("role", Value(std::string("tablist")));
        Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
        return createJsxElement("div", "inline-flex items-center gap-1 rounded-xl bg-slate-900 border border-slate-800 p-1", props, children);
    };
    m->set("TabsList", Value(std::make_shared<NativeMethodCallable>("TabsList", 0, 1, tabsListFn)));

    auto tabsTriggerFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        if (!props->has("role")) props->set("role", Value(std::string("tab")));
        bool active = props->has("active") ? props->entries["active"].deref().isTruthy()
                    : (props->has("selected") ? props->entries["selected"].deref().isTruthy() : false);
        std::string cls = active
            ? "inline-flex items-center justify-center rounded-lg px-3 py-1.5 text-sm font-medium bg-indigo-600 text-white"
            : "inline-flex items-center justify-center rounded-lg px-3 py-1.5 text-sm font-medium text-slate-400 hover:bg-slate-800 hover:text-slate-100";
        Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
        return createJsxElement("tab", cls, props, children);
    };
    m->set("TabsTrigger", Value(std::make_shared<NativeMethodCallable>("TabsTrigger", 0, 1, tabsTriggerFn)));

    auto tabsContentFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        if (!props->has("role")) props->set("role", Value(std::string("tabpanel")));
        Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
        return createJsxElement("div", "mt-2 rounded-xl border border-slate-800 bg-slate-950/60 p-4", props, children);
    };
    m->set("TabsContent", Value(std::make_shared<NativeMethodCallable>("TabsContent", 0, 1, tabsContentFn)));

    // Dialog, Popover, Tooltip, ContextMenu, Toast
    auto dialogFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        if (!props->has("role")) props->set("role", Value(std::string("dialog")));
        Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
        return createJsxElement("dialog", "rounded-2xl border border-slate-700 bg-slate-900 p-6 shadow-2xl text-slate-100", props, children);
    };
    m->set("Dialog", Value(std::make_shared<NativeMethodCallable>("Dialog", 0, 1, dialogFn)));

    auto popoverFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
        return createJsxElement("popover", "rounded-xl border border-slate-700 bg-slate-900 p-3 shadow-xl text-slate-100", props, children);
    };
    m->set("Popover", Value(std::make_shared<NativeMethodCallable>("Popover", 0, 1, popoverFn)));

    auto tooltipFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
        return createJsxElement("tooltip", "rounded-lg border border-slate-700 bg-slate-950 px-2.5 py-1.5 text-xs text-slate-100 shadow-lg", props, children);
    };
    m->set("Tooltip", Value(std::make_shared<NativeMethodCallable>("Tooltip", 0, 1, tooltipFn)));

    auto contextMenuFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
        return createJsxElement("contextmenu", "min-w-[160px] rounded-xl border border-slate-700 bg-slate-900 p-1 shadow-xl text-slate-100", props, children);
    };
    m->set("ContextMenu", Value(std::make_shared<NativeMethodCallable>("ContextMenu", 0, 1, contextMenuFn)));

    auto toastFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
        return createJsxElement("toast", "rounded-xl border border-slate-700 bg-slate-900 px-4 py-3 text-sm text-slate-100 shadow-xl", props, children);
    };
    m->set("Toast", Value(std::make_shared<NativeMethodCallable>("Toast", 0, 1, toastFn)));

    // Table, ResizablePanelGroup/Panel/Handle, ScrollArea, Avatar, Skeleton, Canvas, Icon
    auto tableFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
        return createJsxElement("div", "w-full rounded-xl border border-slate-800 bg-slate-950/60 overflow-hidden", props, children);
    };
    m->set("Table", Value(std::make_shared<NativeMethodCallable>("Table", 0, 1, tableFn)));

    auto resizeGroupFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        std::string dir = props->has("direction") ? props->entries["direction"].deref().toString() : "horizontal";
        std::string cls = (dir == "vertical") ? "flex flex-col w-full h-full" : "flex flex-row w-full h-full";
        Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
        return createJsxElement("div", cls, props, children);
    };
    m->set("ResizablePanelGroup", Value(std::make_shared<NativeMethodCallable>("ResizablePanelGroup", 0, 1, resizeGroupFn)));

    auto resizePanelFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
        return createJsxElement("div", "flex-1 min-w-0 min-h-0 overflow-hidden", props, children);
    };
    m->set("ResizablePanel", Value(std::make_shared<NativeMethodCallable>("ResizablePanel", 0, 1, resizePanelFn)));

    auto resizeHandleFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        if (!props->has("role")) props->set("role", Value(std::string("separator")));
        if (!props->has("aria-label") && !props->has("accessibleName")) props->set("aria-label", Value(std::string("Resize panel handle")));
        return createJsxElement("div", "w-1.5 min-h-[24px] bg-slate-800 hover:bg-indigo-500 cursor-col-resize shrink-0", props, Value(std::make_shared<MemeList>()));
    };
    m->set("ResizableHandle", Value(std::make_shared<NativeMethodCallable>("ResizableHandle", 0, 1, resizeHandleFn)));

    auto scrollAreaFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
        return createJsxElement("scrollarea", "overflow-auto flex flex-col", props, children);
    };
    m->set("ScrollArea", Value(std::make_shared<NativeMethodCallable>("ScrollArea", 0, 1, scrollAreaFn)));

    auto avatarFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        Value children = props->has("children") ? props->entries["children"] : Value(std::make_shared<MemeList>());
        return createJsxElement("div", "inline-flex items-center justify-center w-10 h-10 rounded-full bg-indigo-600 text-white text-sm font-semibold", props, children);
    };
    m->set("Avatar", Value(std::make_shared<NativeMethodCallable>("Avatar", 0, 1, avatarFn)));

    auto skeletonFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        return createJsxElement("div", "rounded-xl bg-slate-800/70 h-4 w-full", props, Value(std::make_shared<MemeList>()));
    };
    m->set("Skeleton", Value(std::make_shared<NativeMethodCallable>("Skeleton", 0, 1, skeletonFn)));

    auto canvasFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        return createJsxElement("canvas", "w-full h-40 rounded-xl bg-slate-950 border border-slate-800", props, Value(std::make_shared<MemeList>()));
    };
    m->set("Canvas", Value(std::make_shared<NativeMethodCallable>("Canvas", 0, 1, canvasFn)));

    auto iconFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        return createJsxElement("icon", "w-4 h-4 text-slate-200", props, Value(std::make_shared<MemeList>()));
    };
    m->set("Icon", Value(std::make_shared<NativeMethodCallable>("Icon", 0, 1, iconFn)));
    m->set("Platform", createPlatformComponent());

    return m;
}

inline std::shared_ptr<MemeMap> createIconsModule()
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    auto iconFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        auto props = (args.size() > 0 && args[0].deref().isMap()) ? args[0].deref().getMap() : std::make_shared<MemeMap>();
        return createJsxElement("icon", "w-4 h-4 text-slate-200", props, Value(std::make_shared<MemeList>()));
    };
    m->set("Icon", Value(std::make_shared<NativeMethodCallable>("Icon", 0, 1, iconFn)));

    for (const std::string &kebab : MemeUI::Icons::allIconNames()) {
        auto namedFn = [kebab](Interpreter &, const std::vector<Value> &args) -> Value {
            auto props = std::make_shared<MemeMap>();
            if (!args.empty() && args[0].deref().isMap()) {
                for (const auto &[k, v] : args[0].deref().getMap()->entries) {
                    props->set(k, v);
                }
            }
            props->set("name", Value(kebab));
            return createJsxElement("icon", "w-4 h-4 text-slate-200", props, Value(std::make_shared<MemeList>()));
        };
        std::string pascalName = MemeUI::Icons::toPascalIconName(kebab);
        m->set(pascalName, Value(std::make_shared<NativeMethodCallable>(pascalName, 0, 1, namedFn)));
        m->set(kebab, Value(std::make_shared<NativeMethodCallable>(kebab, 0, 1, namedFn)));
    }
    return m;
}

inline std::shared_ptr<MemeMap> createMotionModule()
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    auto makeMotionTag = [](const std::string &htmlTag) {
        return Value(std::make_shared<NativeMethodCallable>(
            "motion_" + htmlTag, 0, 1,
            [htmlTag](Interpreter &, const std::vector<Value> &args) -> Value {
                auto props = std::make_shared<MemeMap>();
                Value childrenVal = Value(std::make_shared<MemeList>());
                if (!args.empty() && args[0].deref().isMap()) {
                    auto inP = args[0].deref().getMap();
                    for (const auto &[k, v] : inP->entries) {
                        if (k == "children") {
                            childrenVal = v;
                            continue;
                        }
                        if (k == "initial" && v.deref().isMap()) {
                            auto im = v.deref().getMap();
                            if (im->has("opacity")) props->set("initialOpacity", im->entries["opacity"]);
                            if (im->has("y")) props->set("initialY", im->entries["y"]);
                            continue;
                        }
                        if (k == "animate" && v.deref().isMap()) {
                            auto am = v.deref().getMap();
                            if (am->has("opacity")) props->set("animateOpacity", am->entries["opacity"]);
                            if (am->has("y")) props->set("animateY", am->entries["y"]);
                            continue;
                        }
                        props->set(k, v);
                    }
                }
                return createJsxElement(htmlTag, "transition-all duration-200", props, childrenVal);
            }));
    };

    auto innerMotion = std::make_shared<MemeMap>();
    innerMotion->isModule = true;
    for (const char *tag : {"div", "span", "button", "section", "article", "aside", "header", "footer", "p", "h1", "h2", "h3", "ul", "li"}) {
        Value tv = makeMotionTag(tag);
        m->set(tag, tv);
        innerMotion->set(tag, tv);
    }
    Value viewV = makeMotionTag("div");
    m->set("View", viewV);
    innerMotion->set("View", viewV);
    m->set("motion", Value(innerMotion));

    auto springFallback = [](Interpreter &, const std::vector<Value> &args) -> Value {
        return Value(args.empty() ? 0.0 : args[0].deref().asDouble());
    };
    m->set("useSpring", Value(std::make_shared<NativeMethodCallable>("useSpring", 1, 2, springFallback)));
    m->set("spring", Value(std::make_shared<NativeMethodCallable>("spring", 1, 2, springFallback)));
    return m;
}

} // namespace ShadcnComponents

#endif // SHADCN_COMPONENTS_H
