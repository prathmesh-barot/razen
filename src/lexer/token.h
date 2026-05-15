#pragma once
#include <string>
#include <string_view>
#include <array>

namespace razen {

enum class TokenType {
    Identifier,
    IntegerValue,
    DecimalValue,
    CharValue,
    StringValue,

    // numeric types
    I1, I2, I4, I8, I16, I32, I64, I128,
    Isize, Int,
    U1, U2, U4, U8, U16, U32, U64, U128,
    Usize, Uint,
    F16, F32, F64, F128,
    Float,

    // other built-in types
    Bool, Char, Void, Noret, Any, Str, String,

    // keywords
    Type, Enum, Union, Error, Struct, Behave, Ext,
    Func, Pub, Mod, Use, Const, Mut,
    If, Else, Match, Loop, Ret, Break, Skip,
    Try, Catch, Defer, Test, True, False, Async, Needs,

    // operators and separators
    Comment, EndComment,
    Equals, ColonEquals, PlusEquals, MinusEquals, StarEquals, SlashEquals, PercentEquals,
    Plus, Minus, Star, Slash, Percent,
    EqualsEquals, NotEquals,
    LessThan, LessThanEquals, GreaterThan, GreaterThanEquals,
    ExclamationMark, AndAnd, OrOr,
    And, Or, Caret, Tilde,
    ShiftLeft, ShiftRight,
    Dot, Comma, Semicolon, Colon, QuestionMark,
    LeftParen, RightParen, LeftBrace, RightBrace, LeftBracket, RightBracket,
    Arrow, BigArrow, TildeArrow,
    DotDotDot, DotDot, DotDotEquals,
    At,
    NA,
    EOF_
};

// operator characters
inline constexpr std::array<char, 15> OPERATORS = {
    '=', '+', '-', '*', '/', '%', '!', '&', '|', '^', '~', '<', '>', '?', '@'
};
// the colon ':' is also an operator character in some contexts
inline constexpr std::array<char, 16> OPERATORS_WITH_COLON = {
    '=', '+', '-', '*', '/', '%', '!', '&', '|', '^', '~', '<', '>', '?', '@', ':'
};

// separator characters
inline constexpr std::array<char, 13> SEPARATORS = {
    ';', '(', ')', '{', '}', '[', ']', ',', '.', '\n', '\r', '\t', '\\'
};

// type keyword strings
inline constexpr std::string_view I1_STR = "i1";
inline constexpr std::string_view I2_STR = "i2";
inline constexpr std::string_view I4_STR = "i4";
inline constexpr std::string_view I8_STR = "i8";
inline constexpr std::string_view I16_STR = "i16";
inline constexpr std::string_view I32_STR = "i32";
inline constexpr std::string_view I64_STR = "i64";
inline constexpr std::string_view I128_STR = "i128";
inline constexpr std::string_view ISIZE_STR = "isize";
inline constexpr std::string_view INT_STR = "int";
inline constexpr std::string_view U1_STR = "u1";
inline constexpr std::string_view U2_STR = "u2";
inline constexpr std::string_view U4_STR = "u4";
inline constexpr std::string_view U8_STR = "u8";
inline constexpr std::string_view U16_STR = "u16";
inline constexpr std::string_view U32_STR = "u32";
inline constexpr std::string_view U64_STR = "u64";
inline constexpr std::string_view U128_STR = "u128";
inline constexpr std::string_view USIZE_STR = "usize";
inline constexpr std::string_view UINT_STR = "uint";
inline constexpr std::string_view F16_STR = "f16";
inline constexpr std::string_view F32_STR = "f32";
inline constexpr std::string_view F64_STR = "f64";
inline constexpr std::string_view F128_STR = "f128";
inline constexpr std::string_view FLOAT_STR = "float";
inline constexpr std::string_view BOOL_STR = "bool";
inline constexpr std::string_view CHAR_STR = "char";
inline constexpr std::string_view VOID_STR = "void";
inline constexpr std::string_view NORET_STR = "noret";
inline constexpr std::string_view ANY_STR = "any";
inline constexpr std::string_view STR_STR = "str";
inline constexpr std::string_view STRING_STR = "string";

// keyword strings
inline constexpr std::string_view TYPE_STR = "type";
inline constexpr std::string_view ENUM_STR = "enum";
inline constexpr std::string_view UNION_STR = "union";
inline constexpr std::string_view ERROR_STR = "error";
inline constexpr std::string_view STRUCT_STR = "struct";
inline constexpr std::string_view BEHAVE_STR = "behave";
inline constexpr std::string_view EXT_STR = "ext";
inline constexpr std::string_view FUNC_STR = "func";
inline constexpr std::string_view PUB_STR = "pub";
inline constexpr std::string_view MOD_STR = "mod";
inline constexpr std::string_view USE_STR = "use";
inline constexpr std::string_view CONST_STR = "const";
inline constexpr std::string_view MUT_STR = "mut";
inline constexpr std::string_view IF_STR = "if";
inline constexpr std::string_view ELSE_STR = "else";
inline constexpr std::string_view MATCH_STR = "match";
inline constexpr std::string_view LOOP_STR = "loop";
inline constexpr std::string_view RET_STR = "ret";
inline constexpr std::string_view BREAK_STR = "break";
inline constexpr std::string_view SKIP_STR = "skip";
inline constexpr std::string_view TRY_STR = "try";
inline constexpr std::string_view CATCH_STR = "catch";
inline constexpr std::string_view DEFER_STR = "defer";
inline constexpr std::string_view TEST_STR = "test";
inline constexpr std::string_view TRUE_STR = "true";
inline constexpr std::string_view FALSE_STR = "false";
inline constexpr std::string_view ASYNC_STR = "async";
inline constexpr std::string_view NEEDS_STR = "needs";

// operator strings
inline constexpr std::string_view EQUALS_STR = "=";
inline constexpr std::string_view COLON_EQUALS_STR = ":=";
inline constexpr std::string_view PLUS_EQUALS_STR = "+=";
inline constexpr std::string_view MINUS_EQUALS_STR = "-=";
inline constexpr std::string_view STAR_EQUALS_STR = "*=";
inline constexpr std::string_view SLASH_EQUALS_STR = "/=";
inline constexpr std::string_view PERCENT_EQUALS_STR = "%=";
inline constexpr std::string_view PLUS_STR = "+";
inline constexpr std::string_view MINUS_STR = "-";
inline constexpr std::string_view STAR_STR = "*";
inline constexpr std::string_view SLASH_STR = "/";
inline constexpr std::string_view PERCENT_STR = "%";
inline constexpr std::string_view EQUALS_EQUALS_STR = "==";
inline constexpr std::string_view NOT_EQUALS_STR = "!=";
inline constexpr std::string_view LESS_THAN_STR = "<";
inline constexpr std::string_view LESS_THAN_EQUALS_STR = "<=";
inline constexpr std::string_view GREATER_THAN_STR = ">";
inline constexpr std::string_view GREATER_THAN_EQUALS_STR = ">=";
inline constexpr std::string_view EXPLANATION_MARK_STR = "!";
inline constexpr std::string_view AND_AND_STR = "&&";
inline constexpr std::string_view OR_OR_STR = "||";
inline constexpr std::string_view AND_STR = "&";
inline constexpr std::string_view OR_STR = "|";
inline constexpr std::string_view CARET_STR = "^";
inline constexpr std::string_view TILDE_STR = "~";
inline constexpr std::string_view SHIFT_LEFT_STR = "<<";
inline constexpr std::string_view SHIFT_RIGHT_STR = ">>";
inline constexpr std::string_view DOT_STR = ".";
inline constexpr std::string_view COMMA_STR = ",";
inline constexpr std::string_view SEMICOLON_STR = ";";
inline constexpr std::string_view COLON_STR = ":";
inline constexpr std::string_view QUESTION_MARK_STR = "?";
inline constexpr std::string_view LEFT_PAREN_STR = "(";
inline constexpr std::string_view RIGHT_PAREN_STR = ")";
inline constexpr std::string_view LEFT_BRACE_STR = "{";
inline constexpr std::string_view RIGHT_BRACE_STR = "}";
inline constexpr std::string_view LEFT_BRACKET_STR = "[";
inline constexpr std::string_view RIGHT_BRACKET_STR = "]";
inline constexpr std::string_view ARROW_STR = "->";
inline constexpr std::string_view BIG_ARROW_STR = "=>";
inline constexpr std::string_view TILDE_ARROW_STR = "~>";
inline constexpr std::string_view DOT_DOT_DOT_STR = "...";
inline constexpr std::string_view DOT_DOT_STR = "..";
inline constexpr std::string_view DOT_DOT_EQUALS_STR = "..=";
inline constexpr std::string_view AT_STR = "@";

} // namespace razen
