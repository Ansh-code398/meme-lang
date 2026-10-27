#ifndef SHADCN_COMPONENTS_H
#define SHADCN_COMPONENTS_H

#include "../interpreter/Value.h"
#include "../interpreter/Callable.h"
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

    return m;
}

} // namespace ShadcnComponents

#endif // SHADCN_COMPONENTS_H
