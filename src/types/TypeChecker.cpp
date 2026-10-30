#include "TypeChecker.h"
#include <stdexcept>

// ==========================================
// TypeEnvironment Implementation
// ==========================================

TypeEnvironment::TypeEnvironment(std::shared_ptr<TypeEnvironment> enclosing)
    : enclosing(enclosing) {}

void TypeEnvironment::define(const std::string &name, std::shared_ptr<Type> type, bool isExplicit)
{
    bindings[name] = TypeSymbol{type, isExplicit};
}

bool TypeEnvironment::assign(const std::string &name, std::shared_ptr<Type> type, int line)
{
    auto it = bindings.find(name);
    if (it != bindings.end())
    {
        if (it->second.isExplicit)
        {
            if (!type->isAssignableTo(*it->second.type))
            {
                throw std::runtime_error("Line " + std::to_string(line) + ": Aukaat ke bahar! Variable '" + name +
                                         "' ka type '" + it->second.type->toString() +
                                         "' hai, par tumne '" + type->toString() + "' pakda diya!");
            }
        }
        else
        {
            // Untyped variable reassignment: dynamically update inferred type
            it->second.type = type;
        }
        return true;
    }

    if (enclosing)
    {
        return enclosing->assign(name, type, line);
    }

    return false;
}

std::shared_ptr<TypeSymbol> TypeEnvironment::lookup(const std::string &name)
{
    auto it = bindings.find(name);
    if (it != bindings.end())
    {
        return std::make_shared<TypeSymbol>(it->second);
    }
    if (enclosing)
    {
        return enclosing->lookup(name);
    }
    return nullptr;
}

// ==========================================
// TypeChecker Implementation
// ==========================================

TypeChecker::TypeChecker()
{
    globals = std::make_shared<TypeEnvironment>();
    initGlobals();
}

void TypeChecker::initGlobals()
{
    // Conversion built-ins
    globals->define("ank", Type::Function({Type::Any()}, Type::Ank()), true);
    globals->define("sankhya", Type::Function({Type::Any()}, Type::Ank()), true);
    globals->define("int", Type::Function({Type::Any()}, Type::Ank()), true);
    globals->define("int32", Type::Function({Type::Any()}, Type::Int32()), true);
    globals->define("ank32", Type::Function({Type::Any()}, Type::Int32()), true);
    globals->define("byte", Type::Function({Type::Any()}, Type::Byte()), true);
    globals->define("chota_ank", Type::Function({Type::Any()}, Type::Byte()), true);
    globals->define("u8", Type::Function({Type::Any()}, Type::Byte()), true);
    globals->define("long", Type::Function({Type::Any()}, Type::Ank()), true);
    globals->define("bada_ank", Type::Function({Type::Any()}, Type::Ank()), true);
    globals->define("point", Type::Function({Type::Any()}, Type::Point()), true);
    globals->define("dashamlav", Type::Function({Type::Any()}, Type::Point()), true);
    globals->define("float", Type::Function({Type::Any()}, Type::Point()), true);
    globals->define("float32", Type::Function({Type::Any()}, Type::Float32()), true);
    globals->define("double", Type::Function({Type::Any()}, Type::Point()), true);
    globals->define("char", Type::Function({Type::Any()}, Type::Char()), true);
    globals->define("akshar", Type::Function({Type::Any()}, Type::Char()), true);
    globals->define("bytes", Type::Function({Type::Any()}, Type::Bytes()), true);
    globals->define("kaccha_data", Type::Function({Type::Any()}, Type::Bytes()), true);
    globals->define("set", Type::Function({Type::Any()}, Type::Set(Type::Any())), true);
    globals->define("jhund", Type::Function({Type::Any()}, Type::Set(Type::Any())), true);
    globals->define("tuple", Type::Function({Type::Any()}, Type::Tuple()), true);
    globals->define("jodi", Type::Function({Type::Any()}, Type::Tuple()), true);
    globals->define("bol", Type::Function({Type::Any()}, Type::Bol()), true);
    globals->define("shabd", Type::Function({Type::Any()}, Type::Bol()), true);
    globals->define("string", Type::Function({Type::Any()}, Type::Bol()), true);
    globals->define("str", Type::Function({Type::Any()}, Type::Bol()), true);
    globals->define("sach", Type::Function({Type::Any()}, Type::Sach()), true);
    globals->define("faisla", Type::Function({Type::Any()}, Type::Sach()), true);
    globals->define("type", Type::Function({Type::Any()}, Type::Bol()), true);
    globals->define("kism", Type::Function({Type::Any()}, Type::Bol()), true);

    // Collection built-ins
    globals->define("size", Type::Function({Type::Any()}, Type::Ank()), true);
    globals->define("lambai", Type::Function({Type::Any()}, Type::Ank()), true);
    globals->define("push", Type::Function({Type::Any(), Type::Any()}, Type::Any()), true);
    globals->define("daal", Type::Function({Type::Any(), Type::Any()}, Type::Any()), true);
    globals->define("pop", Type::Function({Type::Any()}, Type::Any()), true);
    globals->define("nikal", Type::Function({Type::Any()}, Type::Any()), true);
    globals->define("keys", Type::Function({Type::Any()}, Type::List(Type::Any())), true);
    globals->define("chabiya", Type::Function({Type::Any()}, Type::List(Type::Any())), true);
    globals->define("values", Type::Function({Type::Any()}, Type::List(Type::Any())), true);
    globals->define("kimatein", Type::Function({Type::Any()}, Type::List(Type::Any())), true);
    globals->define("clear", Type::Function({Type::Any()}, Type::Nalla()), true);
    globals->define("khali_kar", Type::Function({Type::Any()}, Type::Nalla()), true);
    globals->define("clone", Type::Function({Type::Any()}, Type::Any()), true);
    globals->define("nakal", Type::Function({Type::Any()}, Type::Any()), true);
    globals->define("has_key", Type::Function({Type::Any(), Type::Any()}, Type::Sach()), true);
    globals->define("hai_kya", Type::Function({Type::Any(), Type::Any()}, Type::Sach()), true);

    // I/O & Reflection
    globals->define("pooch", Type::Function({}, Type::Bol()), true);
    globals->define("kundli", Type::Function({Type::Any()}, Type::Map(Type::Bol(), Type::Any())), true);
    globals->define("chala_de", Type::Function({Type::Any(), Type::Bol(), Type::Any()}, Type::Any()), true);

    // Math built-ins
    globals->define("abs", Type::Function({Type::Any()}, Type::Point()), true);
    globals->define("floor", Type::Function({Type::Any()}, Type::Ank()), true);
    globals->define("ceil", Type::Function({Type::Any()}, Type::Ank()), true);
    globals->define("sqrt", Type::Function({Type::Any()}, Type::Point()), true);
    globals->define("round", Type::Function({Type::Any()}, Type::Ank()), true);
    globals->define("min", Type::Function({Type::Any(), Type::Any()}, Type::Point()), true);
    globals->define("max", Type::Function({Type::Any(), Type::Any()}, Type::Point()), true);
    globals->define("pow", Type::Function({Type::Any(), Type::Any()}, Type::Point()), true);
    globals->define("random", Type::Function({}, Type::Point()), true);
}

void TypeChecker::check(const std::vector<std::unique_ptr<Stmt>> &statements)
{
    for (const auto &stmt : statements)
    {
        checkStmt(stmt.get(), globals);
    }
}

void TypeChecker::checkStmt(const Stmt *stmt, std::shared_ptr<TypeEnvironment> env)
{
    if (!stmt) return;

    if (auto *v = dynamic_cast<const VarDeclStmt *>(stmt))
    {
        if (v->initializer)
        {
            auto initType = checkExpr(v->initializer.get(), env);
            if (v->typeAnnotation && !v->typeAnnotation->isAny())
            {
                if (!initType->isAssignableTo(*v->typeAnnotation))
                {
                    throw std::runtime_error("Line " + std::to_string(v->line) + ": Aukaat ke bahar! Variable '" +
                                             v->name + "' ka type '" + v->typeAnnotation->toString() +
                                             "' hai, par tumne '" + initType->toString() + "' pakda diya!");
                }
                env->define(v->name, v->typeAnnotation, true);
            }
            else
            {
                // Inferred type, but dynamically reassignable
                env->define(v->name, initType, false);
            }
        }
        else
        {
            if (v->typeAnnotation)
            {
                env->define(v->name, v->typeAnnotation, true);
            }
            else
            {
                env->define(v->name, Type::Any(), false);
            }
        }
        return;
    }

    if (auto *ads = dynamic_cast<const ArrayDestructureStmt *>(stmt))
    {
        if (ads->initializer)
        {
            checkExpr(ads->initializer.get(), env);
        }
        for (const auto &item : ads->items)
        {
            env->define(item.name, Type::Any(), false);
        }
        return;
    }

    if (auto *mds = dynamic_cast<const MapDestructureStmt *>(stmt))
    {
        if (mds->initializer)
        {
            checkExpr(mds->initializer.get(), env);
        }
        for (const auto &k : mds->keys)
        {
            env->define(k, Type::Any(), false);
        }
        return;
    }

    if (auto *e = dynamic_cast<const ExprStmt *>(stmt))
    {
        checkExpr(e->expression.get(), env);
        return;
    }

    if (auto *p = dynamic_cast<const PrintStmt *>(stmt))
    {
        for (const auto &arg : p->args)
        {
            checkExpr(arg.get(), env);
        }
        return;
    }

    if (auto *b = dynamic_cast<const BlockStmt *>(stmt))
    {
        auto blockEnv = std::make_shared<TypeEnvironment>(env);
        for (const auto &s : b->statements)
        {
            checkStmt(s.get(), blockEnv);
        }
        return;
    }

    if (auto *i = dynamic_cast<const IfStmt *>(stmt))
    {
        checkExpr(i->condition.get(), env);
        checkStmt(i->thenBranch.get(), env);
        if (i->elseBranch)
        {
            checkStmt(i->elseBranch.get(), env);
        }
        return;
    }

    if (auto *w = dynamic_cast<const WhileStmt *>(stmt))
    {
        checkExpr(w->condition.get(), env);
        checkStmt(w->body.get(), env);
        return;
    }

    if (auto *g = dynamic_cast<const GhoomStmt *>(stmt))
    {
        if (g->start) checkExpr(g->start.get(), env);
        if (g->end) checkExpr(g->end.get(), env);
        if (g->step) checkExpr(g->step.get(), env);
        auto loopEnv = std::make_shared<TypeEnvironment>(env);
        loopEnv->define(g->varName, Type::Ank(), true);
        checkStmt(g->body.get(), loopEnv);
        return;
    }

    if (auto *gm = dynamic_cast<const GhumaMeinStmt *>(stmt))
    {
        auto collType = checkExpr(gm->collection.get(), env);
        auto loopEnv = std::make_shared<TypeEnvironment>(env);
        std::shared_ptr<Type> elemType = (collType->kind == TypeKind::LIST && collType->elementType)
                                             ? collType->elementType
                                             : Type::Any();
        loopEnv->define(gm->varName, elemType, false);
        checkStmt(gm->body.get(), loopEnv);
        return;
    }

    if (auto *r = dynamic_cast<const ReturnStmt *>(stmt))
    {
        if (currentFunctionReturnType && !currentFunctionReturnType->isAny())
        {
            std::shared_ptr<Type> valType = r->value ? checkExpr(r->value.get(), env) : Type::Nalla();
            if (!valType->isAssignableTo(*currentFunctionReturnType))
            {
                throw std::runtime_error("Line " + std::to_string(r->line) + ": Dhokha! Jugaad ne '" +
                                         currentFunctionReturnType->toString() + "' wapas karne ka wada kiya tha, par '" +
                                         valType->toString() + "' fenk diya!");
            }
        }
        else if (r->value)
        {
            checkExpr(r->value.get(), env);
        }
        return;
    }

    if (dynamic_cast<const BreakStmt *>(stmt) || dynamic_cast<const ContinueStmt *>(stmt))
    {
        return;
    }

    if (auto *f = dynamic_cast<const FuncDeclStmt *>(stmt))
    {
        std::vector<std::shared_ptr<Type>> paramTypes;
        for (const auto &p : f->paramList)
        {
            paramTypes.push_back(p.typeAnnotation ? p.typeAnnotation : Type::Any());
        }
        auto fnType = Type::Function(paramTypes, f->returnTypeAnnotation ? f->returnTypeAnnotation : Type::Any());
        if (!f->name.empty())
        {
            env->define(f->name, fnType, true);
        }

        auto funcEnv = std::make_shared<TypeEnvironment>(env);
        for (const auto &p : f->paramList)
        {
            if (p.isMapDestructure)
            {
                for (const auto &k : p.destructureKeys)
                {
                    funcEnv->define(k.name, Type::Any(), false);
                }
            }
            else
            {
                funcEnv->define(p.name, p.typeAnnotation ? p.typeAnnotation : Type::Any(), p.typeAnnotation != nullptr);
            }
            if (p.defaultValue)
            {
                auto defType = checkExpr(p.defaultValue.get(), env);
                if (p.typeAnnotation && !p.typeAnnotation->isAny())
                {
                    if (!defType->isAssignableTo(*p.typeAnnotation))
                    {
                        throw std::runtime_error("Line " + std::to_string(f->line) + ": Doglapan! Parameter '" +
                                                 p.name + "' ka default value type '" + defType->toString() +
                                                 "' expected type '" + p.typeAnnotation->toString() + "' se match nahi karta!");
                    }
                }
            }
        }

        auto prevRet = currentFunctionReturnType;
        currentFunctionReturnType = f->returnTypeAnnotation;
        checkStmt(f->body.get(), funcEnv);
        currentFunctionReturnType = prevRet;
        return;
    }

    if (auto *c = dynamic_cast<const ClassDeclStmt *>(stmt))
    {
        env->define(c->name, Type::Class(c->name), true);
        auto classEnv = std::make_shared<TypeEnvironment>(env);
        for (const auto &m : c->methods)
        {
            checkStmt(m.get(), classEnv);
        }
        return;
    }

    if (auto *tc = dynamic_cast<const TryCatchStmt *>(stmt))
    {
        checkStmt(tc->tryBlock.get(), env);
        auto catchEnv = std::make_shared<TypeEnvironment>(env);
        catchEnv->define(tc->catchVar, Type::Any(), false);
        checkStmt(tc->catchBlock.get(), catchEnv);
        if (tc->finallyBlock)
        {
            checkStmt(tc->finallyBlock.get(), env);
        }
        return;
    }

    if (auto *th = dynamic_cast<const ThrowStmt *>(stmt))
    {
        checkExpr(th->expression.get(), env);
        return;
    }

    if (dynamic_cast<const ImportStmt *>(stmt))
    {
        return;
    }

    if (auto *ed = dynamic_cast<const EnumDeclStmt *>(stmt))
    {
        env->define(ed->name, Type::Enum(ed->name), true);
        return;
    }

    if (auto *js = dynamic_cast<const JaanchStmt *>(stmt))
    {
        checkExpr(js->target.get(), env);
        for (const auto &arm : js->arms)
        {
            if (arm.pattern) checkExpr(arm.pattern.get(), env);
            checkStmt(arm.body.get(), env);
        }
        if (js->defaultArm)
        {
            checkStmt(js->defaultArm.get(), env);
        }
        return;
    }
}

std::shared_ptr<Type> TypeChecker::checkExpr(const Expr *expr, std::shared_ptr<TypeEnvironment> env)
{
    if (!expr) return Type::Any();

    if (dynamic_cast<const IntExpr *>(expr))
    {
        return Type::Ank();
    }

    if (dynamic_cast<const ByteExpr *>(expr))
    {
        return Type::Byte();
    }

    if (dynamic_cast<const BytesExpr *>(expr))
    {
        return Type::Bytes();
    }

    if (dynamic_cast<const FloatExpr *>(expr))
    {
        return Type::Point();
    }

    if (dynamic_cast<const StringExpr *>(expr))
    {
        return Type::Bol();
    }

    if (dynamic_cast<const BoolExpr *>(expr))
    {
        return Type::Sach();
    }

    if (dynamic_cast<const NullExpr *>(expr))
    {
        return Type::Nalla();
    }

    if (auto *ident = dynamic_cast<const IdentifierExpr *>(expr))
    {
        auto sym = env->lookup(ident->name);
        if (sym && sym->type)
        {
            return sym->type;
        }
        return Type::Any();
    }

    if (auto *assign = dynamic_cast<const AssignExpr *>(expr))
    {
        auto valType = checkExpr(assign->value.get(), env);
        if (!env->assign(assign->name, valType, assign->line))
        {
            env->define(assign->name, valType, false);
        }
        return valType;
    }

    if (auto *da = dynamic_cast<const DestructureAssignExpr *>(expr))
    {
        auto valType = checkExpr(da->value.get(), env);
        std::shared_ptr<Type> elemType = Type::Any();
        if (valType->kind == TypeKind::LIST)
        {
            elemType = valType->elementType ? valType->elementType : Type::Any();
        }
        for (const auto &item : da->items)
        {
            if (!env->assign(item.name, elemType, da->line))
            {
                env->define(item.name, elemType, false);
            }
        }
        return valType;
    }

    if (auto *un = dynamic_cast<const UnaryExpr *>(expr))
    {
        auto operandType = checkExpr(un->operand.get(), env);
        if (un->op.type == TokenType::NOT)
        {
            return Type::Sach();
        }
        if (un->op.type == TokenType::MINUS || un->op.type == TokenType::PLUS)
        {
            if (operandType->isNumeric())
            {
                return operandType;
            }
            if (operandType->isAny())
            {
                return Type::Any();
            }
            throw std::runtime_error("Line " + std::to_string(un->line) + ": Aukaat ke bahar! Unary operator '" +
                                     un->op.value + "' type '" + operandType->toString() + "' par nahi chal sakta!");
        }
        return Type::Any();
    }

    if (auto *b = dynamic_cast<const BinaryExpr *>(expr))
    {
        auto left = checkExpr(b->left.get(), env);
        auto right = checkExpr(b->right.get(), env);

        // Escape hatch for dynamic types
        if (left->isAny() || right->isAny())
        {
            return Type::Any();
        }

        // Allow Tantrik methods on classes
        if (left->kind == TypeKind::CLASS)
        {
            return Type::Any();
        }

        TokenType op = b->op.type;

        // Equality operators
        if (op == TokenType::EQUAL || op == TokenType::NOT_EQUAL)
        {
            return Type::Sach();
        }

        // Logical operators
        if (op == TokenType::AUR || op == TokenType::YA)
        {
            return Type::Sach();
        }

        // Plus operator
        if (op == TokenType::PLUS)
        {
            if (left->kind == TypeKind::BOL || right->kind == TypeKind::BOL)
            {
                return Type::Bol();
            }
            if (left->kind == TypeKind::ANK && right->kind == TypeKind::ANK)
            {
                return Type::Ank();
            }
            if (left->isNumeric() && right->isNumeric())
            {
                return Type::Point();
            }
            if (left->kind == TypeKind::LIST && right->kind == TypeKind::LIST)
            {
                return left;
            }
            throw std::runtime_error("Line " + std::to_string(b->line) + ": Aukaat ke bahar! Operator '+' type '" +
                                     left->toString() + "' aur '" + right->toString() + "' ke beech nahi chal sakta!");
        }

        // Multiply operator
        if (op == TokenType::MULTIPLY)
        {
            if ((left->kind == TypeKind::BOL && right->kind == TypeKind::ANK) ||
                (left->kind == TypeKind::ANK && right->kind == TypeKind::BOL))
            {
                return Type::Bol();
            }
            if (left->kind == TypeKind::ANK && right->kind == TypeKind::ANK)
            {
                return Type::Ank();
            }
            if (left->isNumeric() && right->isNumeric())
            {
                return Type::Point();
            }
            throw std::runtime_error("Line " + std::to_string(b->line) + ": Aukaat ke bahar! Operator '*' type '" +
                                     left->toString() + "' aur '" + right->toString() + "' ke beech nahi chal sakta!");
        }

        // Minus, Divide, Modulo
        if (op == TokenType::MINUS || op == TokenType::DIVIDE || op == TokenType::MOD)
        {
            if (left->isNumeric() && right->isNumeric())
            {
                if (op == TokenType::MOD)
                {
                    return (left->kind == TypeKind::ANK && right->kind == TypeKind::ANK) ? Type::Ank() : Type::Point();
                }
                if (left->kind == TypeKind::ANK && right->kind == TypeKind::ANK)
                {
                    return Type::Ank();
                }
                return Type::Point();
            }
            throw std::runtime_error("Line " + std::to_string(b->line) + ": Aukaat ke bahar! Operator '" +
                                     b->op.value + "' type '" + left->toString() + "' aur '" +
                                     right->toString() + "' ke beech nahi chal sakta!");
        }

        // Comparison operators: <, <=, >, >=
        if (op == TokenType::LESS_THAN || op == TokenType::LESS_THAN_EQUAL ||
            op == TokenType::GREATER_THAN || op == TokenType::GREATER_THAN_EQUAL)
        {
            if ((left->isNumeric() && right->isNumeric()) ||
                (left->kind == TypeKind::BOL && right->kind == TypeKind::BOL))
            {
                return Type::Sach();
            }
            throw std::runtime_error("Line " + std::to_string(b->line) + ": Aukaat ke bahar! Operator '" +
                                     b->op.value + "' type '" + left->toString() + "' aur '" +
                                     right->toString() + "' ke beech nahi chal sakta!");
        }

        return Type::Any();
    }

    if (auto *call = dynamic_cast<const CallExpr *>(expr))
    {
        auto calleeType = checkExpr(call->callee.get(), env);
        std::vector<std::shared_ptr<Type>> argTypes;
        for (const auto &arg : call->args)
        {
            argTypes.push_back(checkExpr(arg.get(), env));
        }

        if (calleeType->kind == TypeKind::FUNCTION)
        {
            size_t paramCount = calleeType->paramTypes.size();
            for (size_t i = 0; i < paramCount && i < argTypes.size(); ++i)
            {
                auto expectedParamType = calleeType->paramTypes[i];
                auto actualArgType = argTypes[i];
                if (expectedParamType && !expectedParamType->isAny())
                {
                    if (!actualArgType->isAssignableTo(*expectedParamType))
                    {
                        throw std::runtime_error("Line " + std::to_string(call->line) +
                                                 ": Doglapan! Jugaad parameter ko '" +
                                                 expectedParamType->toString() +
                                                 "' chahiye tha, par tumne '" +
                                                 actualArgType->toString() + "' pass kiya!");
                    }
                }
            }
            return calleeType->returnType ? calleeType->returnType : Type::Any();
        }

        if (calleeType->kind == TypeKind::CLASS)
        {
            return calleeType;
        }

        return Type::Any();
    }

    if (dynamic_cast<const PoochExpr *>(expr))
    {
        return Type::Bol();
    }

    if (dynamic_cast<const GetExpr *>(expr))
    {
        return Type::Any();
    }

    if (auto *set = dynamic_cast<const SetExpr *>(expr))
    {
        return checkExpr(set->value.get(), env);
    }

    if (auto *arr = dynamic_cast<const ArrayLiteralExpr *>(expr))
    {
        if (arr->elements.empty())
        {
            return Type::List(Type::Any());
        }
        std::shared_ptr<Type> commonType = checkExpr(arr->elements[0].get(), env);
        for (size_t i = 1; i < arr->elements.size(); ++i)
        {
            auto t = checkExpr(arr->elements[i].get(), env);
            if (!t->isAssignableTo(*commonType))
            {
                if (commonType->isAssignableTo(*t))
                {
                    commonType = t;
                }
                else
                {
                    commonType = Type::Any();
                    break;
                }
            }
        }
        return Type::List(commonType);
    }

    if (auto *tup = dynamic_cast<const TupleExpr *>(expr))
    {
        std::vector<std::shared_ptr<Type>> elemTypes;
        for (const auto &el : tup->elements)
        {
            elemTypes.push_back(checkExpr(el.get(), env));
        }
        return Type::Tuple(std::move(elemTypes));
    }

    if (auto *st = dynamic_cast<const SetLiteralExpr *>(expr))
    {
        if (st->elements.empty())
        {
            return Type::Set(Type::Any());
        }
        std::shared_ptr<Type> commonType = checkExpr(st->elements[0].get(), env);
        for (size_t i = 1; i < st->elements.size(); ++i)
        {
            auto t = checkExpr(st->elements[i].get(), env);
            if (!t->isAssignableTo(*commonType))
            {
                if (commonType->isAssignableTo(*t))
                {
                    commonType = t;
                }
                else
                {
                    commonType = Type::Any();
                    break;
                }
            }
        }
        return Type::Set(commonType);
    }

    if (auto *t = dynamic_cast<const TernaryExpr *>(expr))
    {
        if (t->condition) checkExpr(t->condition.get(), env);
        auto thenType = t->thenBranch ? checkExpr(t->thenBranch.get(), env) : Type::Any();
        auto elseType = t->elseBranch ? checkExpr(t->elseBranch.get(), env) : Type::Any();
        if (thenType == elseType) return thenType;
        return Type::Any();
    }

    if (auto *sp = dynamic_cast<const SpreadExpr *>(expr))
    {
        return sp->expr ? checkExpr(sp->expr.get(), env) : Type::Any();
    }

    if (auto *map = dynamic_cast<const MapLiteralExpr *>(expr))
    {
        if (map->entries.empty())
        {
            return Type::Map(Type::Any(), Type::Any());
        }
        for (const auto &entry : map->entries)
        {
            if (entry.first) checkExpr(entry.first.get(), env);
            if (entry.second) checkExpr(entry.second.get(), env);
        }
        return Type::Map(Type::Any(), Type::Any());
    }

    if (auto *jsx = dynamic_cast<const JsxElementExpr *>(expr))
    {
        for (const auto &[key, val] : jsx->props)
        {
            if (val) checkExpr(val.get(), env);
        }
        for (const auto &child : jsx->children)
        {
            if (child) checkExpr(child.get(), env);
        }
        return Type::Any();
    }

    if (auto *idx = dynamic_cast<const IndexExpr *>(expr))
    {
        auto objType = checkExpr(idx->target.get(), env);
        checkExpr(idx->index.get(), env);
        if (objType->kind == TypeKind::LIST && objType->elementType)
        {
            return objType->elementType;
        }
        if (objType->kind == TypeKind::MAP && objType->valueType)
        {
            return objType->valueType;
        }
        if (objType->kind == TypeKind::BOL)
        {
            return Type::Bol();
        }
        return Type::Any();
    }

    if (auto *idxAssign = dynamic_cast<const IndexAssignExpr *>(expr))
    {
        checkExpr(idxAssign->target.get(), env);
        checkExpr(idxAssign->index.get(), env);
        return checkExpr(idxAssign->value.get(), env);
    }

    if (auto *lam = dynamic_cast<const LambdaExpr *>(expr))
    {
        if (lam->declaration)
        {
            std::vector<std::shared_ptr<Type>> pTypes;
            for (const auto &p : lam->declaration->paramList)
            {
                pTypes.push_back(p.typeAnnotation ? p.typeAnnotation : Type::Any());
            }
            auto fnType = Type::Function(pTypes, lam->declaration->returnTypeAnnotation ? lam->declaration->returnTypeAnnotation : Type::Any());

            auto lamEnv = std::make_shared<TypeEnvironment>(env);
            for (const auto &p : lam->declaration->paramList)
            {
                if (p.isMapDestructure)
                {
                    for (const auto &k : p.destructureKeys)
                    {
                        lamEnv->define(k.name, Type::Any(), false);
                    }
                }
                else
                {
                    lamEnv->define(p.name, p.typeAnnotation ? p.typeAnnotation : Type::Any(), p.typeAnnotation != nullptr);
                }
            }

            auto prevRet = currentFunctionReturnType;
            currentFunctionReturnType = lam->declaration->returnTypeAnnotation;
            checkStmt(lam->declaration->body.get(), lamEnv);
            currentFunctionReturnType = prevRet;

            return fnType;
        }
        return Type::Function({}, Type::Any());
    }

    if (auto *sp = dynamic_cast<const SpreadExpr *>(expr))
    {
        return checkExpr(sp->expr.get(), env);
    }

    if (auto *b = dynamic_cast<const BorrowExpr *>(expr))
    {
        return checkExpr(b->operand.get(), env);
    }

    if (auto *fs = dynamic_cast<const FStringExpr *>(expr))
    {
        for (const auto &p : fs->parts)
        {
            checkExpr(p.get(), env);
        }
        return Type::Bol();
    }

    if (auto *aw = dynamic_cast<const AwaitExpr *>(expr))
    {
        return checkExpr(aw->expression.get(), env);
    }

    return Type::Any();
}
