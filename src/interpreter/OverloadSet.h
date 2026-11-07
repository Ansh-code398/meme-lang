#ifndef OVERLOADSET_H
#define OVERLOADSET_H

#include <vector>
#include <string>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include "Callable.h"
#include "Function.h"
#include "Value.h"
#include "Environment.h"
#include "../types/Type.h"

class OverloadSet : public Callable
{
private:
    std::string name;
    std::vector<std::shared_ptr<UserFunction>> candidates;

public:
    OverloadSet(std::string name = "") : name(std::move(name)) {}

    void addCandidate(std::shared_ptr<UserFunction> fn)
    {
        candidates.push_back(std::move(fn));
    }

    const std::vector<std::shared_ptr<UserFunction>> &getCandidates() const
    {
        return candidates;
    }

    const std::string &getName() const
    {
        return name;
    }

    size_t arity() const override { return minArity(); }

    size_t minArity() const override
    {
        if (candidates.empty()) return 0;
        size_t m = candidates[0]->minArity();
        for (const auto &c : candidates) {
            if (c->minArity() < m) m = c->minArity();
        }
        return m;
    }

    size_t maxArity() const override
    {
        if (candidates.empty()) return 0;
        size_t m = candidates[0]->maxArity();
        for (const auto &c : candidates) {
            if (c->maxArity() > m) m = c->maxArity();
        }
        return m;
    }

    bool isVariadic() const override
    {
        for (const auto &c : candidates) {
            if (c->isVariadic()) return true;
        }
        return false;
    }

    bool checkArity(size_t count) const override
    {
        for (const auto &c : candidates) {
            if (c->checkArity(count)) return true;
        }
        return false;
    }

    std::shared_ptr<UserFunction> resolve(const std::vector<Value> &args) const
    {
        std::vector<std::shared_ptr<UserFunction>> arityMatches;
        for (const auto &cand : candidates)
        {
            if (cand && cand->checkArity(args.size()))
            {
                arityMatches.push_back(cand);
            }
        }

        std::string displayName = !name.empty() ? name : (!candidates.empty() ? candidates[0]->getName() : "anonymous");

        if (arityMatches.empty())
        {
            throw std::runtime_error(
                "Aukaat ke bahar! Jugaad '" + displayName +
                "' ke liye koi matching overload nahi mila (argument count mismatch)!");
        }

        struct ScoredCandidate {
            std::shared_ptr<UserFunction> cand;
            int score = 0;
        };
        std::vector<ScoredCandidate> compatible;

        for (const auto &cand : arityMatches)
        {
            const auto *decl = cand->getDeclaration();
            const auto &paramList = decl->paramList;
            bool isCompat = true;
            int totalScore = 0;

            for (size_t i = 0; i < args.size(); ++i)
            {
                std::shared_ptr<Type> paramType = Type::Any();
                if (i < paramList.size())
                {
                    if (paramList[i].typeAnnotation)
                    {
                        paramType = paramList[i].typeAnnotation;
                    }
                }
                else if (decl->isVariadic() && !paramList.empty())
                {
                    if (paramList.back().typeAnnotation)
                    {
                        paramType = paramList.back().typeAnnotation;
                    }
                }

                auto argType = Type::fromRuntimeValue(args[i]);
                if (!argType) argType = Type::Any();
                if (!paramType) paramType = Type::Any();

                if (!argType->isAssignableTo(*paramType))
                {
                    isCompat = false;
                    break;
                }

                int dist = Type::computeTypeDistance(argType, paramType);
                totalScore += dist;
            }

            if (isCompat)
            {
                compatible.push_back({cand, totalScore});
            }
        }

        if (compatible.empty())
        {
            throw std::runtime_error(
                "Aukaat ke bahar! Jugaad '" + displayName +
                "' ke liye koi matching overload nahi mila!");
        }

        int minScore = compatible[0].score;
        for (const auto &sc : compatible)
        {
            if (sc.score < minScore) minScore = sc.score;
        }

        std::vector<std::shared_ptr<UserFunction>> best;
        for (const auto &sc : compatible)
        {
            if (sc.score == minScore) best.push_back(sc.cand);
        }

        if (best.size() > 1)
        {
            throw std::runtime_error(
                "Confuse kar diya! Jugaad '" + displayName +
                "' ke overloads mein tie ho gaya!");
        }

        return best[0];
    }

    Value call(Interpreter &interpreter, const std::vector<Value> &args) override
    {
        auto best = resolve(args);
        return best->call(interpreter, args);
    }

    std::string toString() const override
    {
        return "<overload " + name + " (" + std::to_string(candidates.size()) + " variants)>";
    }
};

inline void Environment::defineOverload(const std::string &name, std::shared_ptr<UserFunction> func)
{
    std::lock_guard<std::recursive_mutex> lock(envMutex);
    auto it = values.find(name);
    if (it != values.end())
    {
        if (it->second.value.isCallable())
        {
            auto c = it->second.value.getCallable();
            if (auto os = std::dynamic_pointer_cast<OverloadSet>(c))
            {
                os->addCandidate(func);
                return;
            }
            if (auto uf = std::dynamic_pointer_cast<UserFunction>(c))
            {
                auto os = std::make_shared<OverloadSet>(name);
                os->addCandidate(uf);
                os->addCandidate(func);
                it->second.value = Value(std::static_pointer_cast<Callable>(os));
                return;
            }
        }
    }
    auto os = std::make_shared<OverloadSet>(name);
    os->addCandidate(func);
    define(name, Value(std::static_pointer_cast<Callable>(os)));
}

#endif
