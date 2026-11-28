#pragma once

#include "JsEngineCore.h"
#include "../interpreter/Value.h"
#include "../interpreter/Callable.h"
#include "../ui/ShadcnComponents.h"

namespace MemeJsBridge {

inline Value jsNodeToValue(const MemeJs::JsNode &n, const std::shared_ptr<MemeJs::JsEngineSession> &sess, int depth = 0);

inline MemeJs::JsNode valueToJsNode(const Value &v, const std::shared_ptr<MemeJs::JsEngineSession> &sess = nullptr, Interpreter *interp = nullptr, int depth = 0) {
    if (depth >= 12) return MemeJs::JsNode::makeNull();
    Value d = v.deref();
    if (d.isNil()) return MemeJs::JsNode::makeNull();
    if (std::holds_alternative<bool>(d.data)) return MemeJs::JsNode::makeBool(std::get<bool>(d.data));
    if (std::holds_alternative<long long>(d.data)) return MemeJs::JsNode::makeInt(std::get<long long>(d.data));
    if (std::holds_alternative<double>(d.data)) return MemeJs::JsNode::makeDouble(std::get<double>(d.data));
    if (d.isNumeric()) return MemeJs::JsNode::makeDouble(d.asDouble());
    if (d.isString()) return MemeJs::JsNode::makeString(d.toString());
    if (d.isCallable() && interp && sess) {
        auto fn = d.getCallable();
        return MemeJs::JsNode::makeHostFn([fn, interp, sess](const std::vector<MemeJs::JsNode> &args) -> MemeJs::JsNode {
            std::vector<Value> margs;
            margs.reserve(args.size());
            for (const auto &a : args) margs.push_back(jsNodeToValue(a, sess));
            Value r = fn->call(*interp, margs);
            return valueToJsNode(r, sess, interp);
        });
    }
    if (d.isList()) {
        std::vector<MemeJs::JsNode> arr;
        auto l = d.getList();
        arr.reserve(l->elements.size());
        for (const auto &el : l->elements) arr.push_back(valueToJsNode(el, sess, interp, depth + 1));
        return MemeJs::JsNode::makeArray(std::move(arr));
    }
    if (d.isMap()) {
        std::vector<std::pair<std::string, MemeJs::JsNode>> obj;
        auto m = d.getMap();
        obj.reserve(m->entries.size());
        for (const auto &[k, val] : m->entries) {
            obj.emplace_back(k, valueToJsNode(val, sess, interp, depth + 1));
        }
        return MemeJs::JsNode::makeObject(std::move(obj));
    }
    return MemeJs::JsNode::makeString(d.toString());
}

inline Value jsNodeToValue(const MemeJs::JsNode &n, const std::shared_ptr<MemeJs::JsEngineSession> &sess, int depth) {
    if (depth >= 12) return Value(std::monostate{});
    switch (n.kind) {
        case MemeJs::JsNode::K_NULL: return Value(std::monostate{});
        case MemeJs::JsNode::K_BOOL: return Value(n.bVal);
        case MemeJs::JsNode::K_INT: return Value(static_cast<long long>(n.iVal));
        case MemeJs::JsNode::K_DOUBLE: return Value(n.dVal);
        case MemeJs::JsNode::K_STRING: return Value(n.sVal);
        case MemeJs::JsNode::K_ARRAY: {
            auto l = std::make_shared<MemeList>();
            l->elements.reserve(n.arrVal.size());
            for (const auto &el : n.arrVal) l->elements.push_back(jsNodeToValue(el, sess, depth + 1));
            return Value(l);
        }
        case MemeJs::JsNode::K_OBJECT: {
            auto m = std::make_shared<MemeMap>();
            for (const auto &kv : n.objVal) {
                m->set(kv.first, jsNodeToValue(kv.second, sess, depth + 1));
            }
            return Value(m);
        }
        case MemeJs::JsNode::K_FUNCTION: {
            uint64_t fnId = n.fnId;
            auto callable = Value(std::make_shared<NativeMethodCallable>(
                "js_fn_" + std::to_string(fnId), 0, 16,
                [sess, fnId](Interpreter &it, const std::vector<Value> &args) -> Value {
                    std::vector<MemeJs::JsNode> jsArgs;
                    jsArgs.reserve(args.size());
                    for (const auto &a : args) jsArgs.push_back(valueToJsNode(a, sess, &it));
                    return jsNodeToValue(sess->callFunction(fnId, jsArgs), sess);
                }));
            if (!n.objVal.empty()) {
                auto m = std::make_shared<MemeMap>();
                m->set("__call__", callable);
                m->set("default", callable);
                for (const auto &kv : n.objVal) {
                    m->set(kv.first, jsNodeToValue(kv.second, sess, depth + 1));
                }
                return Value(m);
            }
            return callable;
        }
        case MemeJs::JsNode::K_HOST_FUNCTION:
            return Value(std::monostate{});
    }
    return Value(std::monostate{});
}

inline Value loadNpmPackageValue(const std::string &pkg, const std::string &callerDir = "") {
    // 1. Compile-time & native Zero-WebView UI ecosystem adapters
    if (pkg == "lucide-react") {
        return Value(ShadcnComponents::createIconsModule());
    }
    if (pkg == "framer-motion") {
        return Value(ShadcnComponents::createMotionModule());
    }
    if (pkg == "@radix-ui/react-popover" || pkg == "@radix-ui/react-dialog" || pkg == "@radix-ui/react-tabs") {
        return Value(ShadcnComponents::createShadcnModule());
    }
    if (pkg == "js:eval" || pkg == "eval") {
        auto sess = MemeJs::sharedSession();
        auto m = std::make_shared<MemeMap>();
        m->isModule = true;
        m->set("eval", Value(std::make_shared<NativeMethodCallable>("eval", 1, [sess](Interpreter &, const std::vector<Value> &args) {
            return jsNodeToValue(sess->evalScript(args[0].deref().toString(), "<js:eval>"), sess);
        })));
        m->set("__jsEngine", Value(std::string(MemeJs::JsEngineSession::engineName())));
        return Value(m);
    }

    auto sess = MemeJs::sharedSession();
    MemeJs::JsNode res = sess->loadPackage(pkg, callerDir);
    Value converted = jsNodeToValue(res, sess);

    auto modMap = std::make_shared<MemeMap>();
    modMap->isModule = true;
    std::string aliasSym = MemeJs::deriveModuleSymbolAlias(pkg);
    if (converted.isMap()) {
        *modMap = *converted.getMap();
        modMap->isModule = true;
        if (!modMap->has("default") && modMap->has(aliasSym)) modMap->set("default", modMap->entries[aliasSym]);
    } else if (std::holds_alternative<std::shared_ptr<Callable>>(converted.data)) {
        modMap->set("__call__", converted);
        modMap->set("default", converted);
        if (!aliasSym.empty()) modMap->set(aliasSym, converted);
    } else {
        modMap->set("default", converted);
    }
    modMap->set("__jsEngine", Value(std::string(MemeJs::JsEngineSession::engineName())));
    modMap->set("eval", Value(std::make_shared<NativeMethodCallable>("eval", 1, [sess](Interpreter &, const std::vector<Value> &args) {
        return jsNodeToValue(sess->evalScript(args[0].deref().toString(), "<js:eval>"), sess);
    })));
    return Value(modMap);
}

} // namespace MemeJsBridge
