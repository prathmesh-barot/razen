#include "lexer.h"
#include "errors.h"
#include <iostream>
#include <cstring>

namespace razen {

// ── helper: check if index is in range ──────────────────────────────────────
static bool isIndexInRange(size_t max, size_t index) {
    return index < max;
}

// ── compare two string_views for byte-for-byte equality ─────────────────────
static bool twoSlicesAreTheSame(std::string_view a, std::string_view b) {
    return a == b;
}

// ── check if a slice contains a character ───────────────────────────────────
static bool contains(std::string_view slice, char c) {
    return slice.find(c) != std::string_view::npos;
}

// ── print helpers ───────────────────────────────────────────────────────────

void printNumberSlice(const std::vector<int>& source) {
    std::cout << "[";
    for (size_t i = 0; i < source.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << source[i];
    }
    std::cout << "]";
}

void printlnNumberSlice(const std::vector<int>& source) {
    printNumberSlice(source);
    std::cout << "\n";
}

void printlnQuotes(std::string_view source) {
    std::cout << "'" << source << "'\n";
}

void printMessage(std::string_view message, std::string_view source) {
    std::cout << message << ": '" << source << "'";
}

void printlnMessage(std::string_view message, std::string_view source) {
    std::cout << message << ": '" << source << "'\n";
}

void printStr(std::string_view source) {
    std::cout << source;
}

void printlnStr(std::string_view source) {
    std::cout << source << "\n";
}

void newLine() {
    std::cout << "\n";
}

void Token::printValues() const {
    std::cout << "Token Type: " << static_cast<int>(type)
              << ", Value: " << value
              << ", Line: " << line
              << ", Character: " << character << "\n";
}

// ── helpers: operator/separator checks ─────────────────────────────────────

bool isOperator(char c) {
    for (char op : OPERATORS_WITH_COLON) {
        if (c == op) return true;
    }
    return false;
}

bool isSeparator(char c) {
    for (char sep : SEPARATORS) {
        if (c == sep) return true;
    }
    return false;
}

bool isLetterOrDigit(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9');
}

bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

bool isInteger(std::string_view s) {
    if (s.empty()) return false;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c == '-') {
            if (i != 0) return false;
            continue;
        }
        if (!isDigit(c)) return false;
    }
    return true;
}

bool isDecimal(std::string_view s) {
    if (s.empty()) return false;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c == '-') {
            if (i != 0) return false;
            continue;
        }
        if (c == '.') continue;
        if (!isDigit(c)) return false;
    }
    return true;
}

// ── getTokenType: map string to TokenType ──────────────────────────────────

TokenType getTokenType(std::string_view s) {
    // built-in types
    if (twoSlicesAreTheSame(s, I1_STR)) return TokenType::I1;
    if (twoSlicesAreTheSame(s, I2_STR)) return TokenType::I2;
    if (twoSlicesAreTheSame(s, I4_STR)) return TokenType::I4;
    if (twoSlicesAreTheSame(s, I8_STR)) return TokenType::I8;
    if (twoSlicesAreTheSame(s, I16_STR)) return TokenType::I16;
    if (twoSlicesAreTheSame(s, I32_STR)) return TokenType::I32;
    if (twoSlicesAreTheSame(s, I64_STR)) return TokenType::I64;
    if (twoSlicesAreTheSame(s, I128_STR)) return TokenType::I128;
    if (twoSlicesAreTheSame(s, ISIZE_STR)) return TokenType::Isize;
    if (twoSlicesAreTheSame(s, INT_STR)) return TokenType::Int;
    if (twoSlicesAreTheSame(s, U1_STR)) return TokenType::U1;
    if (twoSlicesAreTheSame(s, U2_STR)) return TokenType::U2;
    if (twoSlicesAreTheSame(s, U4_STR)) return TokenType::U4;
    if (twoSlicesAreTheSame(s, U8_STR)) return TokenType::U8;
    if (twoSlicesAreTheSame(s, U16_STR)) return TokenType::U16;
    if (twoSlicesAreTheSame(s, U32_STR)) return TokenType::U32;
    if (twoSlicesAreTheSame(s, U64_STR)) return TokenType::U64;
    if (twoSlicesAreTheSame(s, U128_STR)) return TokenType::U128;
    if (twoSlicesAreTheSame(s, USIZE_STR)) return TokenType::Usize;
    if (twoSlicesAreTheSame(s, UINT_STR)) return TokenType::Uint;
    if (twoSlicesAreTheSame(s, F16_STR)) return TokenType::F16;
    if (twoSlicesAreTheSame(s, F32_STR)) return TokenType::F32;
    if (twoSlicesAreTheSame(s, F64_STR)) return TokenType::F64;
    if (twoSlicesAreTheSame(s, F128_STR)) return TokenType::F128;
    if (twoSlicesAreTheSame(s, FLOAT_STR)) return TokenType::Float;
    if (twoSlicesAreTheSame(s, BOOL_STR)) return TokenType::Bool;
    if (twoSlicesAreTheSame(s, CHAR_STR)) return TokenType::Char;
    if (twoSlicesAreTheSame(s, VOID_STR)) return TokenType::Void;
    if (twoSlicesAreTheSame(s, NORET_STR)) return TokenType::Noret;
    if (twoSlicesAreTheSame(s, ANY_STR)) return TokenType::Any;
    if (twoSlicesAreTheSame(s, STR_STR)) return TokenType::Str;
    if (twoSlicesAreTheSame(s, STRING_STR)) return TokenType::String;

    // keywords
    if (twoSlicesAreTheSame(s, TYPE_STR)) return TokenType::Type;
    if (twoSlicesAreTheSame(s, ENUM_STR)) return TokenType::Enum;
    if (twoSlicesAreTheSame(s, UNION_STR)) return TokenType::Union;
    if (twoSlicesAreTheSame(s, ERROR_STR)) return TokenType::Error;
    if (twoSlicesAreTheSame(s, STRUCT_STR)) return TokenType::Struct;
    if (twoSlicesAreTheSame(s, BEHAVE_STR)) return TokenType::Behave;
    if (twoSlicesAreTheSame(s, EXT_STR)) return TokenType::Ext;
    if (twoSlicesAreTheSame(s, FUNC_STR)) return TokenType::Func;
    if (twoSlicesAreTheSame(s, PUB_STR)) return TokenType::Pub;
    if (twoSlicesAreTheSame(s, MOD_STR)) return TokenType::Mod;
    if (twoSlicesAreTheSame(s, USE_STR)) return TokenType::Use;
    if (twoSlicesAreTheSame(s, CONST_STR)) return TokenType::Const;
    if (twoSlicesAreTheSame(s, MUT_STR)) return TokenType::Mut;
    if (twoSlicesAreTheSame(s, IF_STR)) return TokenType::If;
    if (twoSlicesAreTheSame(s, ELSE_STR)) return TokenType::Else;
    if (twoSlicesAreTheSame(s, MATCH_STR)) return TokenType::Match;
    if (twoSlicesAreTheSame(s, LOOP_STR)) return TokenType::Loop;
    if (twoSlicesAreTheSame(s, RET_STR)) return TokenType::Ret;
    if (twoSlicesAreTheSame(s, BREAK_STR)) return TokenType::Break;
    if (twoSlicesAreTheSame(s, SKIP_STR)) return TokenType::Skip;
    if (twoSlicesAreTheSame(s, TRY_STR)) return TokenType::Try;
    if (twoSlicesAreTheSame(s, CATCH_STR)) return TokenType::Catch;
    if (twoSlicesAreTheSame(s, DEFER_STR)) return TokenType::Defer;
    if (twoSlicesAreTheSame(s, TEST_STR)) return TokenType::Test;
    if (twoSlicesAreTheSame(s, TRUE_STR)) return TokenType::True;
    if (twoSlicesAreTheSame(s, FALSE_STR)) return TokenType::False;
    if (twoSlicesAreTheSame(s, ASYNC_STR)) return TokenType::Async;
    if (twoSlicesAreTheSame(s, NEEDS_STR)) return TokenType::Needs;

    // operators
    if (twoSlicesAreTheSame(s, EQUALS_STR)) return TokenType::Equals;
    if (twoSlicesAreTheSame(s, COLON_EQUALS_STR)) return TokenType::ColonEquals;
    if (twoSlicesAreTheSame(s, PLUS_EQUALS_STR)) return TokenType::PlusEquals;
    if (twoSlicesAreTheSame(s, MINUS_EQUALS_STR)) return TokenType::MinusEquals;
    if (twoSlicesAreTheSame(s, STAR_EQUALS_STR)) return TokenType::StarEquals;
    if (twoSlicesAreTheSame(s, SLASH_EQUALS_STR)) return TokenType::SlashEquals;
    if (twoSlicesAreTheSame(s, PERCENT_EQUALS_STR)) return TokenType::PercentEquals;
    if (twoSlicesAreTheSame(s, PLUS_STR)) return TokenType::Plus;
    if (twoSlicesAreTheSame(s, MINUS_STR)) return TokenType::Minus;
    if (twoSlicesAreTheSame(s, STAR_STR)) return TokenType::Star;
    if (twoSlicesAreTheSame(s, SLASH_STR)) return TokenType::Slash;
    if (twoSlicesAreTheSame(s, PERCENT_STR)) return TokenType::Percent;
    if (twoSlicesAreTheSame(s, EQUALS_EQUALS_STR)) return TokenType::EqualsEquals;
    if (twoSlicesAreTheSame(s, NOT_EQUALS_STR)) return TokenType::NotEquals;
    if (twoSlicesAreTheSame(s, LESS_THAN_STR)) return TokenType::LessThan;
    if (twoSlicesAreTheSame(s, LESS_THAN_EQUALS_STR)) return TokenType::LessThanEquals;
    if (twoSlicesAreTheSame(s, GREATER_THAN_STR)) return TokenType::GreaterThan;
    if (twoSlicesAreTheSame(s, GREATER_THAN_EQUALS_STR)) return TokenType::GreaterThanEquals;
    if (twoSlicesAreTheSame(s, EXPLANATION_MARK_STR)) return TokenType::ExclamationMark;
    if (twoSlicesAreTheSame(s, AND_AND_STR)) return TokenType::AndAnd;
    if (twoSlicesAreTheSame(s, OR_OR_STR)) return TokenType::OrOr;
    if (twoSlicesAreTheSame(s, AND_STR)) return TokenType::And;
    if (twoSlicesAreTheSame(s, OR_STR)) return TokenType::Or;
    if (twoSlicesAreTheSame(s, CARET_STR)) return TokenType::Caret;
    if (twoSlicesAreTheSame(s, TILDE_STR)) return TokenType::Tilde;
    if (twoSlicesAreTheSame(s, SHIFT_LEFT_STR)) return TokenType::ShiftLeft;
    if (twoSlicesAreTheSame(s, SHIFT_RIGHT_STR)) return TokenType::ShiftRight;
    if (twoSlicesAreTheSame(s, DOT_STR)) return TokenType::Dot;
    if (twoSlicesAreTheSame(s, COMMA_STR)) return TokenType::Comma;
    if (twoSlicesAreTheSame(s, SEMICOLON_STR)) return TokenType::Semicolon;
    if (twoSlicesAreTheSame(s, COLON_STR)) return TokenType::Colon;
    if (twoSlicesAreTheSame(s, QUESTION_MARK_STR)) return TokenType::QuestionMark;
    if (twoSlicesAreTheSame(s, LEFT_PAREN_STR)) return TokenType::LeftParen;
    if (twoSlicesAreTheSame(s, RIGHT_PAREN_STR)) return TokenType::RightParen;
    if (twoSlicesAreTheSame(s, LEFT_BRACE_STR)) return TokenType::LeftBrace;
    if (twoSlicesAreTheSame(s, RIGHT_BRACE_STR)) return TokenType::RightBrace;
    if (twoSlicesAreTheSame(s, LEFT_BRACKET_STR)) return TokenType::LeftBracket;
    if (twoSlicesAreTheSame(s, RIGHT_BRACKET_STR)) return TokenType::RightBracket;
    if (twoSlicesAreTheSame(s, ARROW_STR)) return TokenType::Arrow;
    if (twoSlicesAreTheSame(s, BIG_ARROW_STR)) return TokenType::BigArrow;
    if (twoSlicesAreTheSame(s, TILDE_ARROW_STR)) return TokenType::TildeArrow;
    if (twoSlicesAreTheSame(s, DOT_DOT_DOT_STR)) return TokenType::DotDotDot;
    if (twoSlicesAreTheSame(s, DOT_DOT_STR)) return TokenType::DotDot;
    if (twoSlicesAreTheSame(s, DOT_DOT_EQUALS_STR)) return TokenType::DotDotEquals;
    if (twoSlicesAreTheSame(s, AT_STR)) return TokenType::At;

    // number literals
    if (isInteger(s)) return TokenType::IntegerValue;
    if (isDecimal(s)) return TokenType::DecimalValue;

    // string / char (already surrounded by quotes)
    if (contains(s, '"')) return TokenType::StringValue;
    if (contains(s, '\'')) return TokenType::CharValue;

    return TokenType::Identifier;
}

// ── internal lexer functions ───────────────────────────────────────────────

// skip whitespace
static bool shouldSkip(Lexer& lex_data) {
    lex_data.character_count += 1;

    if (lex_data.character_index >= lex_data.source.size()) return false;
    char currentChar = lex_data.source[lex_data.character_index];

    if (currentChar == '\n') {
        lex_data.line_count += 1;
        lex_data.character_count = 0;
        lex_data.character_index += 1;
        return true;
    }

    bool is_special = currentChar == '\r' ||
                      currentChar == '\t' ||
                      currentChar == ' ' ||
                      currentChar == '\\';

    if (is_special) {
        lex_data.character_index += 1;
        return true;
    }
    return false;
}

// read a single escape sequence (character_index points to char after \)
static char readEscapeChar(Lexer& lex_data) {
    if (lex_data.character_index >= lex_data.source.size())
        throw LexerError("Unterminated escape sequence");

    char c = lex_data.source[lex_data.character_index];
    lex_data.character_index += 1;

    switch (c) {
        case 'n':  return '\n';
        case 't':  return '\t';
        case 'r':  return '\r';
        case '0':  return '\0';
        case '\\': return '\\';
        case '"':  return '"';
        case '\'': return '\'';
        case 'x': {
            if (lex_data.character_index + 1 >= lex_data.source.size())
                throw LexerError("Incomplete hex escape sequence");
            char hex[3] = {lex_data.source[lex_data.character_index],
                           lex_data.source[lex_data.character_index + 1], 0};
            char* end = nullptr;
            long val = strtol(hex, &end, 16);
            if (end != hex + 2)
                throw LexerError("Invalid hex escape sequence");
            lex_data.character_index += 2;
            return static_cast<char>(val);
        }
        default:
        {
            std::string msg = "Unknown escape sequence: \\";
            msg += c;
            throw LexerError(msg);
        }
    }
}

static Token readString(Lexer& lex_data) {
    std::string text;
    lex_data.character_index += 1; // skip past opening "

    while (lex_data.character_index < lex_data.source.size()) {
        char c = lex_data.source[lex_data.character_index];
        if (c == '"') {
            lex_data.character_index += 1;
            return Token{TokenType::StringValue, text, lex_data.line_count, lex_data.character_count};
        }
        if (c == '\\') {
            lex_data.character_index += 1; // skip backslash
            text += readEscapeChar(lex_data);
        } else {
            text.push_back(c);
            lex_data.character_index += 1;
        }
    }

    throw LexerError("Unterminated string literal");
}

static Token readSeparator(Lexer& lex_data) {
    std::string text(1, lex_data.source[lex_data.character_index]);
    lex_data.character_index += 1;
    return Token{getTokenType(text), text, lex_data.line_count, lex_data.character_count};
}

static Token readDotOperator(Lexer& lex_data) {
    std::string text;
    text.push_back('.');
    lex_data.character_index += 1;

    if (lex_data.character_index < lex_data.source.size() &&
        lex_data.source[lex_data.character_index] == '.') {
        text.push_back('.');
        lex_data.character_index += 1;

        if (lex_data.character_index < lex_data.source.size()) {
            char third = lex_data.source[lex_data.character_index];
            if (third == '.' || third == '=') {
                text.push_back(third);
                lex_data.character_index += 1;
            }
        }
    }

    return Token{getTokenType(text), text, lex_data.line_count, lex_data.character_count};
}

static Token readChar(Lexer& lex_data) {
    lex_data.character_index += 1; // skip opening '
    if (lex_data.character_index >= lex_data.source.size())
        throw LexerError("Unexpected end of input in char literal");

    std::string char_value;
    if (lex_data.source[lex_data.character_index] == '\\') {
        lex_data.character_index += 1; // skip backslash
        char_value += readEscapeChar(lex_data);
    } else {
        char_value = lex_data.source[lex_data.character_index];
        lex_data.character_index += 1;
    }

    if (lex_data.character_index >= lex_data.source.size())
        throw LexerError("Unexpected end of input in char literal");

    if (lex_data.source[lex_data.character_index] != '\'')
        throw LexerError("Unterminated char literal");
    lex_data.character_index += 1;

    return Token{TokenType::CharValue, char_value, lex_data.line_count, lex_data.character_count};
}

static Token readOperator(Lexer& lex_data) {
    std::string text;
    char first = lex_data.source[lex_data.character_index];
    text.push_back(first);
    lex_data.character_index += 1;

    if (lex_data.character_index < lex_data.source.size()) {
        char next = lex_data.source[lex_data.character_index];

        bool is_compound = (next == '=') ||
            (first == '-' && next == '>') ||
            (first == '~' && next == '>') ||
            (first == '<' && next == '<') ||
            (first == '>' && next == '>');

        // also handle :: but that's not a Razen operator; handle := (colon already consumed)
        // actually colon is an operator character, so : and = need special handling
        // ColonEquals is already handled: if first == ':' and next == '='

        if (is_compound) {
            text.push_back(next);
            lex_data.character_index += 1;
        }
    }

    return Token{getTokenType(text), text, lex_data.line_count, lex_data.character_count};
}

static Token readWord(Lexer& lex_data) {
    std::string text;

    while (lex_data.character_index < lex_data.source.size()) {
        char c = lex_data.source[lex_data.character_index];
        if (isLetterOrDigit(c) || c == '_') {
            text.push_back(c);
            lex_data.character_index += 1;
        } else {
            break;
        }
    }

    return Token{getTokenType(text), text, lex_data.line_count, lex_data.character_count};
}

static Token readSingleLineComment(Lexer& lex_data) {
    size_t start_line = lex_data.line_count;
    size_t start_char = lex_data.character_count;
    lex_data.character_index += 2; // skip //

    std::string text;
    while (lex_data.character_index < lex_data.source.size()) {
        char c = lex_data.source[lex_data.character_index];
        if (c == '\n') break; // comment ends at newline, don't consume it
        text.push_back(c);
        lex_data.character_index += 1;
    }

    return Token{TokenType::Comment, text, start_line, start_char};
}

static Token readBlockComment(Lexer& lex_data) {
    size_t start_line = lex_data.line_count;
    size_t start_char = lex_data.character_count;
    lex_data.character_index += 2; // skip /*

    std::string text;
    while (lex_data.character_index < lex_data.source.size()) {
        char c = lex_data.source[lex_data.character_index];
        if (c == '*' && lex_data.character_index + 1 < lex_data.source.size() &&
            lex_data.source[lex_data.character_index + 1] == '/') {
            lex_data.character_index += 2; // skip */
            return Token{TokenType::Comment, text, start_line, start_char};
        }
        if (c == '\n') {
            lex_data.line_count += 1;
            lex_data.character_count = 0;
        }
        text.push_back(c);
        lex_data.character_index += 1;
    }

    throw LexerError("Unterminated block comment");
}

static Token getToken(Lexer& lex_data) {
    if (lex_data.character_index >= lex_data.source.size()) {
        return Token{TokenType::EOF_, "", lex_data.line_count, lex_data.character_count};
    }

    char current_char = lex_data.source[lex_data.character_index];

    if (current_char == '"') return readString(lex_data);
    if (current_char == '\'') return readChar(lex_data);
    if (current_char == '.') return readDotOperator(lex_data);

    // check for comments before operators (since / is an operator character)
    if (current_char == '/') {
        if (lex_data.character_index + 1 < lex_data.source.size()) {
            char next = lex_data.source[lex_data.character_index + 1];
            if (next == '/') return readSingleLineComment(lex_data);
            if (next == '*') return readBlockComment(lex_data);
        }
    }

    if (isOperator(current_char)) return readOperator(lex_data);
    if (isSeparator(current_char)) return readSeparator(lex_data);

    // word / keyword / identifier
    if (isLetterOrDigit(current_char) || current_char == '_') {
        return readWord(lex_data);
    }

    // unrecognized character
    std::string msg = "Unexpected character '";
    msg += current_char;
    msg += "'";
    throw LexerError(msg);
}

static void processCharacter(Lexer& lex_data) {
    if (shouldSkip(lex_data)) return;

    size_t previous_index = lex_data.character_index;
    Token token = getToken(lex_data);

    if (previous_index == lex_data.character_index) {
        lex_data.character_index += 1;
    }

    lex_data.token_list.push_back(token);
    lex_data.last_token = token;
}

// ── main entry point ────────────────────────────────────────────────────────

std::vector<Token> parseToTokens(std::string_view source) {
    if (source.empty()) {
        throw LexerError("Source code length is zero");
    }

    std::cout << "\t" << GREY << "Parsing" << RESET << "\t\t\t\t";

    Lexer lex_data;
    lex_data.source = source;

    while (lex_data.character_index < source.size()) {
        processCharacter(lex_data);
    }

    std::cout << CYAN << "Done" << RESET << "\n";
    return std::move(lex_data.token_list);
}

} // namespace razen
