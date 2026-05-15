#pragma once

namespace razen {
namespace sample_stage2 {

// Break and skip as proper statements (void, no trailing ret needed)
inline const char* BREAK_SKIP = R"(func main() -> void {
    loop {
        if true { break }
        if false { skip }
    }
})";

// else if chaining (void, ret only in early-exit branches)
inline const char* ELSE_IF = R"(func check(x: i32) -> void {
    if x > 10 {
    } else if x > 5 {
    } else if x > 0 {
    } else {
    }
})";

// Range expressions
inline const char* RANGE_EXPR = R"(func main() -> void {
    r1 := 0..10
    r2 := 0..=5
    ret
})";

// Block try/catch
inline const char* TRY_BLOCK = R"(func main() -> void {
    try {
        ret
    } catch (e) {
        ret
    }
})";

// Generic annotation on function (parsed but no codegen)
inline const char* GENERIC_FUNC = R"(@Generic(T) func identity(x: T) -> T {
    ret x
})";

// Generic annotation on struct
inline const char* GENERIC_STRUCT = R"(@Generic(T) struct Container {
    value: T,
})";

// pub on behave, const, mod, use
inline const char* PUB_DECLARATIONS = R"(pub const MAX : i32 = 100
pub mod network
pub use std.fmt
pub behave Printable {
    func print_it(p: @Self) -> void
})";

// ~> behaviour rename in struct
inline const char* BEHAVE_RENAME = R"(behave Speaker {
    func speak(p: @Self) -> void
}
struct Animal ~> Speaker {
    name: str,
    func dog_speak ~> Speaker.speak(a: *Animal) -> void {
        ret
    }
})";

// Bitwise NOT unary prefix
inline const char* BITWISE_NOT = R"(func main() -> void {
    x : i32 = 42
    y := ~x
    ret
})";

// Complex type alias with error union
inline const char* TYPE_ALIAS_COMPLEX = R"(error MyError { NotFound, }
type Result = MyError!str
func process() -> Result {
    ret "ok"
})";

// ext struct for external behaviour (void, no ret needed)
inline const char* EXT_STRUCT = R"(behave Printable {
    func print_it(p: @Self) -> void
}
ext struct int ~> Printable {
    func print_it(p: *int) -> void {
    }
})";

// Void function with no ret at all
inline const char* VOID_NO_RET = R"(func noop() -> void {
}
func main() -> void {
    noop()
})";

} // namespace sample_stage2
} // namespace razen
