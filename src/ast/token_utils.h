#pragma once
#include "../lexer/token.h"

namespace razen {

inline bool isVarType(TokenType tt) {
    switch (tt) {
        case TokenType::I1: case TokenType::I2: case TokenType::I4:
        case TokenType::I8: case TokenType::I16: case TokenType::I32:
        case TokenType::I64: case TokenType::I128: case TokenType::Isize:
        case TokenType::Int:
        case TokenType::U1: case TokenType::U2: case TokenType::U4:
        case TokenType::U8: case TokenType::U16: case TokenType::U32:
        case TokenType::U64: case TokenType::U128: case TokenType::Usize:
        case TokenType::Uint:
        case TokenType::F16: case TokenType::F32: case TokenType::F64:
        case TokenType::F128: case TokenType::Float:
        case TokenType::Bool: case TokenType::Char: case TokenType::Void:
        case TokenType::Noret: case TokenType::Any: case TokenType::Str:
        case TokenType::String:
            return true;
        default:
            return false;
    }
}

inline bool isIntegerType(TokenType tt) {
    switch (tt) {
        case TokenType::I1: case TokenType::I2: case TokenType::I4:
        case TokenType::I8: case TokenType::I16: case TokenType::I32:
        case TokenType::I64: case TokenType::I128: case TokenType::Isize:
        case TokenType::Int:
        case TokenType::U1: case TokenType::U2: case TokenType::U4:
        case TokenType::U8: case TokenType::U16: case TokenType::U32:
        case TokenType::U64: case TokenType::U128: case TokenType::Usize:
        case TokenType::Uint:
            return true;
        default:
            return false;
    }
}

inline bool isFloatType(TokenType tt) {
    switch (tt) {
        case TokenType::F16: case TokenType::F32:
        case TokenType::F64: case TokenType::F128:
        case TokenType::Float:
            return true;
        default:
            return false;
    }
}

inline bool isNumericType(TokenType tt) {
    return isIntegerType(tt) || isFloatType(tt);
}

inline bool isTypeToken(TokenType tt) {
    return isVarType(tt) || tt == TokenType::Identifier;
}

inline bool isLiteral(TokenType tt) {
    switch (tt) {
        case TokenType::IntegerValue: case TokenType::DecimalValue:
        case TokenType::StringValue: case TokenType::CharValue:
        case TokenType::True: case TokenType::False:
            return true;
        default:
            return false;
    }
}

inline bool isKeyword(TokenType tt) {
    switch (tt) {
        case TokenType::Type: case TokenType::Enum: case TokenType::Union:
        case TokenType::Error: case TokenType::Struct: case TokenType::Behave:
        case TokenType::Ext: case TokenType::Func: case TokenType::Pub:
        case TokenType::Mod: case TokenType::Use: case TokenType::Const:
        case TokenType::Mut: case TokenType::If: case TokenType::Else:
        case TokenType::Match: case TokenType::Loop: case TokenType::Ret:
        case TokenType::Break: case TokenType::Skip: case TokenType::Try:
        case TokenType::Catch: case TokenType::Defer: case TokenType::Test:
        case TokenType::True: case TokenType::False: case TokenType::Async:
            return true;
        default:
            return false;
    }
}

inline bool isBinaryOperator(TokenType tt) {
    switch (tt) {
        case TokenType::Plus: case TokenType::Minus: case TokenType::Star:
        case TokenType::Slash: case TokenType::Percent:
        case TokenType::EqualsEquals: case TokenType::NotEquals:
        case TokenType::LessThan: case TokenType::LessThanEquals:
        case TokenType::GreaterThan: case TokenType::GreaterThanEquals:
        case TokenType::AndAnd: case TokenType::OrOr:
        case TokenType::And: case TokenType::Or: case TokenType::Caret:
        case TokenType::ShiftLeft: case TokenType::ShiftRight:
        case TokenType::Catch:
        case TokenType::Dot:
        case TokenType::DotDot:
        case TokenType::DotDotEquals:
            return true;
        default:
            return false;
    }
}

inline bool isAssignmentOperator(TokenType tt) {
    switch (tt) {
        case TokenType::Equals: case TokenType::PlusEquals:
        case TokenType::MinusEquals: case TokenType::StarEquals:
        case TokenType::SlashEquals: case TokenType::PercentEquals:
            return true;
        default:
            return false;
    }
}

inline size_t getPrecedence(TokenType tt) {
    switch (tt) {
        case TokenType::Catch:      return 1;
        case TokenType::OrOr:       return 2;
        case TokenType::AndAnd:     return 3;
        case TokenType::EqualsEquals:
        case TokenType::NotEquals:  return 4;
        case TokenType::LessThan: case TokenType::LessThanEquals:
        case TokenType::GreaterThan: case TokenType::GreaterThanEquals: return 5;
        case TokenType::Or:         return 6;
        case TokenType::Caret:      return 6;
        case TokenType::And:        return 7;
        case TokenType::ShiftLeft:
        case TokenType::ShiftRight: return 8;
        case TokenType::Plus:
        case TokenType::Minus:      return 9;
        case TokenType::Star:
        case TokenType::Slash:
        case TokenType::Percent:    return 10;
        case TokenType::DotDot:
        case TokenType::DotDotEquals: return 11;
        case TokenType::Dot:        return 12;
        default:                    return 0;
    }
}

} // namespace razen
