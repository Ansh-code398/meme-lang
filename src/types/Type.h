#ifndef TYPE_H
#define TYPE_H

#include <string>
#include <vector>
#include <memory>
#include <sstream>

enum class TypeKind
{
    ANY,
    ANK,        // long / int64
    POINT,      // double / float64
    BOL,
    SACH,
    NALLA,
    BYTE,       // byte / u8
    INT32,      // int / int32
    FLOAT32,    // float / f32
    CHAR,       // char / akshar
    BYTES,      // bytes / bytearray / kaccha_data
    SET,        // set / jhund
    TUPLE,      // tuple / jodi
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
    std::shared_ptr<Type> elementType;                 // For LIST or SET
    std::shared_ptr<Type> keyType;                     // For MAP
    std::shared_ptr<Type> valueType;                   // For MAP
    std::vector<std::shared_ptr<Type>> paramTypes;     // For FUNCTION or TUPLE
    std::shared_ptr<Type> returnType;                  // For FUNCTION

    Type(TypeKind kind, const std::string &name = "")
        : kind(kind), name(name) {}

    bool isAny() const { return kind == TypeKind::ANY; }

    bool isNumeric() const {
        return kind == TypeKind::ANK || kind == TypeKind::POINT ||
               kind == TypeKind::BYTE || kind == TypeKind::INT32 ||
               kind == TypeKind::FLOAT32;
    }

    bool isInteger() const {
        return kind == TypeKind::ANK || kind == TypeKind::BYTE ||
               kind == TypeKind::INT32;
    }

    bool isPrimitive() const {
        return isNumeric() || kind == TypeKind::BOL || kind == TypeKind::SACH ||
               kind == TypeKind::NALLA || kind == TypeKind::CHAR;
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
    static std::shared_ptr<Type> Byte() {
        return std::make_shared<Type>(TypeKind::BYTE, "byte");
    }
    static std::shared_ptr<Type> Int32() {
        return std::make_shared<Type>(TypeKind::INT32, "int32");
    }
    static std::shared_ptr<Type> Int64() {
        return std::make_shared<Type>(TypeKind::ANK, "ank");
    }
    static std::shared_ptr<Type> Float32() {
        return std::make_shared<Type>(TypeKind::FLOAT32, "float32");
    }
    static std::shared_ptr<Type> Double() {
        return std::make_shared<Type>(TypeKind::POINT, "point");
    }
    static std::shared_ptr<Type> Char() {
        return std::make_shared<Type>(TypeKind::CHAR, "char");
    }
    static std::shared_ptr<Type> Bytes() {
        return std::make_shared<Type>(TypeKind::BYTES, "bytes");
    }
    static std::shared_ptr<Type> Set(std::shared_ptr<Type> elem = nullptr) {
        auto t = std::make_shared<Type>(TypeKind::SET, "set");
        t->elementType = elem ? elem : Any();
        return t;
    }
    static std::shared_ptr<Type> Tuple(std::vector<std::shared_ptr<Type>> elems = {}) {
        auto t = std::make_shared<Type>(TypeKind::TUPLE, "tuple");
        t->paramTypes = std::move(elems);
        return t;
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

    // Numeric rank for automatic widening
    static int numericRank(TypeKind k)
    {
        switch (k)
        {
        case TypeKind::BYTE: return 1;
        case TypeKind::INT32: return 2;
        case TypeKind::ANK: return 3;
        case TypeKind::FLOAT32: return 4;
        case TypeKind::POINT: return 5;
        default: return 0;
        }
    }

    // Assignability check: can `this` (source) be assigned to `target`?
    bool isAssignableTo(const Type &target) const
    {
        // Dynamic escape hatch: `any` can receive or be assigned to anything
        if (this->isAny() || target.isAny())
        {
            return true;
        }

        // Nullable reference types: `nalla` can be assigned to object, list, map, set, tuple, bytes, or nalla
        if (this->kind == TypeKind::NALLA)
        {
            return target.kind == TypeKind::NALLA ||
                   target.kind == TypeKind::CLASS ||
                   target.kind == TypeKind::LIST ||
                   target.kind == TypeKind::MAP ||
                   target.kind == TypeKind::SET ||
                   target.kind == TypeKind::TUPLE ||
                   target.kind == TypeKind::BYTES;
        }

        // Numeric widening: BYTE -> INT32 -> ANK -> FLOAT32 -> POINT
        if (this->isNumeric() && target.isNumeric())
        {
            int rSrc = numericRank(this->kind);
            int rDst = numericRank(target.kind);
            if (rSrc > 0 && rDst > 0 && rSrc <= rDst)
            {
                return true;
            }
        }

        // Char assignable to Bol (string)
        if (this->kind == TypeKind::CHAR && target.kind == TypeKind::BOL)
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
            if (this->kind == TypeKind::LIST || this->kind == TypeKind::SET)
            {
                if (!this->elementType || !target.elementType) return true;
                return this->elementType->isAssignableTo(*target.elementType);
            }
            if (this->kind == TypeKind::TUPLE)
            {
                if (this->paramTypes.empty() || target.paramTypes.empty()) return true;
                if (this->paramTypes.size() != target.paramTypes.size()) return false;
                for (size_t i = 0; i < this->paramTypes.size(); ++i)
                {
                    if (!this->paramTypes[i]->isAssignableTo(*target.paramTypes[i])) return false;
                }
                return true;
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
        case TypeKind::BYTE:
            return "byte";
        case TypeKind::INT32:
            return "int32";
        case TypeKind::FLOAT32:
            return "float32";
        case TypeKind::CHAR:
            return "char";
        case TypeKind::BYTES:
            return "bytes";
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
        case TypeKind::SET:
            return "set[" + (elementType ? elementType->toString() : "any") + "]";
        case TypeKind::TUPLE:
        {
            if (paramTypes.empty()) return "tuple";
            std::string res = "tuple[";
            for (size_t i = 0; i < paramTypes.size(); ++i)
            {
                if (i > 0) res += ", ";
                res += paramTypes[i] ? paramTypes[i]->toString() : "any";
            }
            res += "]";
            return res;
        }
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
