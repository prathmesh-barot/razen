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

inline bool isNumericToken(TokenType tt) {
    return isIntegerType(tt) || isFloatType(tt);
}

inline TypeInfo primitiveFromToken(TokenType tt, const std::string& name) {
    switch (tt) {
        case TokenType::Bool:   return TypeInfo{TypeCategory::Bool};
        case TokenType::Char:   return TypeInfo{TypeCategory::Char};
        case TokenType::Void:   return TypeInfo{TypeCategory::Void};
        case TokenType::Noret:  return TypeInfo{TypeCategory::Noret};
        case TokenType::Str:    return TypeInfo{TypeCategory::Str};
        case TokenType::String: return TypeInfo{TypeCategory::String};
        case TokenType::Any:    return TypeInfo{TypeCategory::Any};
        default:
            if (isIntegerType(tt)) return TypeInfo{TypeCategory::Integer, name};
            if (isFloatType(tt))   return TypeInfo{TypeCategory::Float, name};
            return TypeInfo{TypeCategory::Unknown};
    }
}

} // namespace razen
