#ifndef TYPE_H
#define TYPE_H

#include <string>
#include <vector>
#include <memory>
#include <sstream>

enum class TypeKind
{
    ANY,
    ANK,
    POINT,
    BOL,
    SACH,
    NALLA,
    LIST,
    MAP,
    FUNCTION,
    CLASS,
    ENUM
};

class Type : public std::enable_shared_from_this<Type>
{
public:
    TypeKind kind;
    std::string name;                                  // For CLASS, ENUM, or custom alias
    std::shared_ptr<Type> elementType;                 // For LIST
    std::shared_ptr<Type> keyType;                     // For MAP
    std::shared_ptr<Type> valueType;                   // For MAP
    std::vector<std::shared_ptr<Type>> paramTypes;     // For FUNCTION
    std::shared_ptr<Type> returnType;                  // For FUNCTION

    Type(TypeKind kind, const std::string &name = "")
        : kind(kind), name(name) {}

    bool isAny() const { return kind == TypeKind::ANY; }
    bool isNumeric() const { return kind == TypeKind::ANK || kind == TypeKind::POINT; }
    bool isPrimitive() const {
        return kind == TypeKind::ANK || kind == TypeKind::POINT ||
               kind == TypeKind::BOL || kind == TypeKind::SACH ||
               kind == TypeKind::NALLA;
    }

    // Factory methods
    static std::shared_ptr<Type> Any() {
        return std::make_shared<Type>(TypeKind::ANY, "any");
    }
    static std::shared_ptr<Type> Ank() {
        return std::make_shared<Type>(TypeKind::ANK, "ank");
    }
    static std::shared_ptr<Type> Point() {
        return std::make_shared<Type>(TypeKind::POINT, "point");
    }
    static std::shared_ptr<Type> Bol() {
        return std::make_shared<Type>(TypeKind::BOL, "bol");
    }
    static std::shared_ptr<Type> Sach() {
        return std::make_shared<Type>(TypeKind::SACH, "sach");
    }
    static std::shared_ptr<Type> Nalla() {
        return std::make_shared<Type>(TypeKind::NALLA, "nalla");
    }
    static std::shared_ptr<Type> List(std::shared_ptr<Type> elem = nullptr) {
        auto t = std::make_shared<Type>(TypeKind::LIST, "list");
        t->elementType = elem ? elem : Any();
        return t;
    }
    static std::shared_ptr<Type> Map(std::shared_ptr<Type> k = nullptr, std::shared_ptr<Type> v = nullptr) {
        auto t = std::make_shared<Type>(TypeKind::MAP, "map");
        t->keyType = k ? k : Any();
        t->valueType = v ? v : Any();
        return t;
    }
    static std::shared_ptr<Type> Function(std::vector<std::shared_ptr<Type>> params, std::shared_ptr<Type> ret = nullptr) {
        auto t = std::make_shared<Type>(TypeKind::FUNCTION, "jugaad");
        t->paramTypes = std::move(params);
        t->returnType = ret ? ret : Any();
        return t;
    }
    static std::shared_ptr<Type> Class(const std::string &className) {
        return std::make_shared<Type>(TypeKind::CLASS, className);
    }
    static std::shared_ptr<Type> Enum(const std::string &enumName) {
        return std::make_shared<Type>(TypeKind::ENUM, enumName);
    }

    // Assignability check: can `this` (source) be assigned to `target`?
    bool isAssignableTo(const Type &target) const
    {
        // Dynamic escape hatch: `any` can receive or be assigned to anything
        if (this->isAny() || target.isAny())
        {
            return true;
        }

        // Nullable reference types: `nalla` can be assigned to object, list, map, or nalla
        if (this->kind == TypeKind::NALLA)
        {
            return target.kind == TypeKind::NALLA ||
                   target.kind == TypeKind::CLASS ||
                   target.kind == TypeKind::LIST ||
                   target.kind == TypeKind::MAP;
        }

        // Numeric widening: integer (ank) can be promoted to float (point)
        if (this->kind == TypeKind::ANK && target.kind == TypeKind::POINT)
        {
            return true;
        }

        // If kinds match:
        if (this->kind == target.kind)
        {
            if (this->kind == TypeKind::CLASS || this->kind == TypeKind::ENUM)
            {
                return this->name.empty() || target.name.empty() || this->name == target.name;
            }
            if (this->kind == TypeKind::LIST)
            {
                if (!this->elementType || !target.elementType) return true;
                return this->elementType->isAssignableTo(*target.elementType);
            }
            if (this->kind == TypeKind::MAP)
            {
                bool kOk = (!this->keyType || !target.keyType) ? true : this->keyType->isAssignableTo(*target.keyType);
                bool vOk = (!this->valueType || !target.valueType) ? true : this->valueType->isAssignableTo(*target.valueType);
                return kOk && vOk;
            }
            if (this->kind == TypeKind::FUNCTION)
            {
                if (this->paramTypes.size() != target.paramTypes.size())
                {
                    return false;
                }
                for (size_t i = 0; i < this->paramTypes.size(); ++i)
                {
                    // Parameters are checked for compatibility
                    if (!target.paramTypes[i]->isAssignableTo(*this->paramTypes[i]))
                    {
                        return false;
                    }
                }
                if (this->returnType && target.returnType)
                {
                    return this->returnType->isAssignableTo(*target.returnType);
                }
                return true;
            }
            return true;
        }

        return false;
    }

    std::string toString() const
    {
        switch (kind)
        {
        case TypeKind::ANY:
            return "any";
        case TypeKind::ANK:
            return "ank";
        case TypeKind::POINT:
            return "point";
        case TypeKind::BOL:
            return "bol";
        case TypeKind::SACH:
            return "sach";
        case TypeKind::NALLA:
            return "nalla";
        case TypeKind::CLASS:
            return name.empty() ? "object" : name;
        case TypeKind::ENUM:
            return name.empty() ? "avtar" : name;
        case TypeKind::LIST:
            return "list[" + (elementType ? elementType->toString() : "any") + "]";
        case TypeKind::MAP:
            return "map[" + (keyType ? keyType->toString() : "any") + ", " +
                   (valueType ? valueType->toString() : "any") + "]";
        case TypeKind::FUNCTION:
        {
            std::string res = "jugaad(";
            for (size_t i = 0; i < paramTypes.size(); ++i)
            {
                if (i > 0) res += ", ";
                res += paramTypes[i] ? paramTypes[i]->toString() : "any";
            }
            res += ") -> " + (returnType ? returnType->toString() : "any");
            return res;
        }
        }
        return "any";
    }
};

#endif
