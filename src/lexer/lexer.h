#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <optional>

#include "token.h"

namespace razen {

// ANSI color constants
inline constexpr auto RED = "\x1b[31m";
inline constexpr auto RESET = "\x1b[0m";
inline constexpr auto GREEN = "\x1b[32m";
inline constexpr auto YELLOW = "\x1b[33m";
inline constexpr auto MAGENTA = "\x1b[35m";
inline constexpr auto CYAN = "\x1b[36m";
inline constexpr auto BLUE = "\x1b[34m";
inline constexpr auto ORANGE = "\x1b[38;2;206;145;120m";
inline constexpr auto GREY = "\x1b[38;2;156;156;156m";
inline constexpr auto CREAM = "\x1b[38;2;220;220;145m";
inline constexpr auto LIGHT_GREEN = "\x1b[38;2;181;206;143m";
inline constexpr auto LIGHT_BLUE = "\x1b[38;2;5;169;173m";
inline constexpr auto PEACH = "\x1b[38;2;255;231;190m";

struct Token {
    TokenType type;
    std::string value;
    size_t line;
    size_t character;

    void printValues() const;
    bool isType(TokenType t) const { return type == t; }
};

struct Lexer {
    std::vector<Token> token_list;
    std::optional<Token> last_token;
    size_t character_index = 0;
    std::string_view source;
    size_t line_count = 0;
    size_t character_count = 0;
};

// lexer entry point
std::vector<Token> parseToTokens(std::string_view source);

// control debug output
void setLexerVerbose(bool v);

// helpers
bool isOperator(char c);
bool isSeparator(char c);
bool isLetterOrDigit(char c);
bool isDigit(char c);
bool isInteger(std::string_view s);
bool isDecimal(std::string_view s);
TokenType getTokenType(std::string_view s);

// print helpers
void printNumberSlice(const std::vector<int>& source);
void printlnNumberSlice(const std::vector<int>& source);
void printlnQuotes(std::string_view source);
void printMessage(std::string_view message, std::string_view source);
void printlnMessage(std::string_view message, std::string_view source);
void printStr(std::string_view source);
void printlnStr(std::string_view source);
void newLine();

} // namespace razen
