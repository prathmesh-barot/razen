#pragma once
#include <string>
#include "../lexer/token.h"
#include "../ast/token_utils.h"

namespace razen {

enum class TypeCategory {
    Integer, Float, Bool, Char, Void, Noret, Str, String, Any,
    Pointer, Optional, Failable, ErrorUnion, Array,
    Struct, Enum, Union, ErrorSet, Function, Named, Unknown
};

struct TypeInfo {
    TypeCategory category;
    std::string name;
    TypeInfo* pointee_type = nullptr;
    TypeInfo* elem_type = nullptr;
    TypeInfo* error_type = nullptr;
    TypeInfo* ok_type = nullptr;
    uint64_t array_size = 0;
    bool is_mut = false;

    bool isNumeric() const { return category == TypeCategory::Integer || category == TypeCategory::Float; }
    bool isInteger() const { return category == TypeCategory::Integer; }
    bool isFloat() const { return category == TypeCategory::Float; }
    bool isBool() const { return category == TypeCategory::Bool; }
    bool isVoid() const { return category == TypeCategory::Void; }
    bool canBeBool() const { return category == TypeCategory::Bool || category == TypeCategory::Integer; }
};

// Convert a TypeInfo to a human-readable string for error messages
inline std::string typeToString(const TypeInfo* t) {
    if (!t) return "unknown";
    switch (t->category) {
        case TypeCategory::Integer:  return t->name.empty() ? "integer" : t->name;
        case TypeCategory::Float:    return t->name.empty() ? "float" : t->name;
        case TypeCategory::Bool:     return "bool";
        case TypeCategory::Char:     return "char";
        case TypeCategory::Void:     return "void";
        case TypeCategory::Noret:    return "noret";
        case TypeCategory::Str:      return "str";
        case TypeCategory::String:   return "string";
        case TypeCategory::Any:      return "any";
        case TypeCategory::Pointer:  return "*" + (t->pointee_type ? typeToString(t->pointee_type) : "void");
        case TypeCategory::Optional: return "?" + (t->elem_type ? typeToString(t->elem_type) : "void");
        case TypeCategory::Failable: return "!" + (t->elem_type ? typeToString(t->elem_type) : "void");
        case TypeCategory::ErrorUnion: {
            std::string err = t->error_type ? typeToString(t->error_type) : "error";
            return err + "!" + (t->ok_type ? typeToString(t->ok_type) : "void");
        }
        case TypeCategory::Array:    return "[" + typeToString(t->elem_type) + "]";
        case TypeCategory::Struct:   return t->name.empty() ? "struct" : t->name;
        case TypeCategory::Enum:     return t->name.empty() ? "enum" : t->name;
        case TypeCategory::Union:    return t->name.empty() ? "union" : t->name;
        case TypeCategory::ErrorSet: return t->name.empty() ? "error" : t->name;
        case TypeCategory::Function: return "function";
        case TypeCategory::Named:    return t->name.empty() ? "type" : t->name;
        case TypeCategory::Unknown:  return "unknown";
    }
    return "unknown";
}

inline bool isNumericToken(TokenType tt) {
    return isIntegerType(tt) || isFloatType(tt);
}

inline TypeInfo primitiveFromToken(TokenType tt, const std::string& name) {
    switch (tt) {
        case TokenType::Bool:   return TypeInfo{TypeCategory::Bool, "bool"};
        case TokenType::Char:   return TypeInfo{TypeCategory::Char, "char"};
        case TokenType::Void:   return TypeInfo{TypeCategory::Void, "void"};
        case TokenType::Noret:  return TypeInfo{TypeCategory::Noret, "noret"};
        case TokenType::Str:    return TypeInfo{TypeCategory::Str, "str"};
        case TokenType::String: return TypeInfo{TypeCategory::String, "string"};
        case TokenType::Any:    return TypeInfo{TypeCategory::Any, "any"};
        default:
            if (isIntegerType(tt)) return TypeInfo{TypeCategory::Integer, name};
            if (isFloatType(tt))   return TypeInfo{TypeCategory::Float, name};
            return TypeInfo{TypeCategory::Unknown, "unknown"};
    }
}

} // namespace razen
