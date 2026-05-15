#pragma once

namespace razen {
namespace sample_comments {

// Single-line comment at start of line
inline const char* SINGLE_LINE_BASIC = R"(func main() -> void {
    // this is a comment
    ret
})";

// Single-line comment after code
inline const char* SINGLE_LINE_AFTER_CODE = R"(func main() -> void {
    x : i32 = 10 // inline comment
    ret x
})";

// Multiple single-line comments
inline const char* SINGLE_LINE_MULTIPLE = R"(func main() -> void {
    // first comment
    // second comment
    // third comment
    ret
})";

// Block comment basic
inline const char* BLOCK_BASIC = R"(func main() -> void {
    /* block comment */
    ret
})";

// Block comment multi-line
inline const char* BLOCK_MULTI_LINE = R"(func main() -> void {
    /*
     * multi-line
     * block comment
     */
    ret
})";

// Block comment before code
inline const char* BLOCK_BEFORE_CODE = R"(func main() -> void {
    /* comment */ x : i32 = 42
    ret x
})";

// String with escape sequences
inline const char* STRING_ESCAPES = R"(func main() -> void {
    s1 : str = "hello\nworld"
    s2 : str = "tab\there"
    s3 : str = "quote\"inside"
    s4 : str = "back\\slash"
    ret
})";

// Char with escape sequences
inline const char* CHAR_ESCAPES = R"(func main() -> void {
    c1 : char = '\n'
    c2 : char = '\t'
    c3 : char = '\\'
    c4 : char = '\''
    ret
})";

// Mixed comments and escape sequences
inline const char* MIXED_FEATURES = R"(// top-level comment
func greet() -> void {
    /* block
       comment */
    msg : str = "Hello\nWorld"
    ret
})";

// Empty block comment
inline const char* EMPTY_BLOCK = R"(func main() -> void {
    /**/
    ret
})";

} // namespace sample_comments
} // namespace razen
