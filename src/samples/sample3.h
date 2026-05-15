#pragma once

namespace razen {
namespace sample3 {

inline const char* DEFER_ORDER = R"(use std.fmt
func setup() -> void {
    defer std.fmt.println("last")
    defer std.fmt.println("first")
    std.fmt.println("body")
})";

inline const char* DEFER_BEFORE_RETURN = R"(use std.fmt
func open_file(ok: bool) -> void {
    defer std.fmt.println("cleanup")
    if ok {
        ret
    }
    std.fmt.println("done")
})";

inline const char* TRY_CATCH_BASIC = R"(use std.fmt
ext func bind(port: int) -> int
func run() -> void {
    res := try bind(8080) catch |e| { ret }
    std.fmt.println(res)
})";

inline const char* TAGGED_UNION = R"(union Value {
    Int: i32,
    Float: f32,
    Text: str,
})";

inline const char* TAGGED_UNION_STRUCT_VARIANT = R"(union Expr {
    Num: i32,
    Binary { left: i32, right: i32, op: str },
})";

inline const char* SELF_IN_BEHAVE = R"(behave Printable {
    func print_self(self: @Self) -> void
})";

inline const char* USE_PATH = R"(use std.fmt
use std.os
use std.debug
func main() -> void {
    std.fmt.println("hello")
    std.os.exit(0)
})";

inline const char* DEBUG_ASSERT = R"(use std.debug
use std.fmt
func divide(a: i32, b: i32) -> i32 {
    std.debug.assert(b != 0)
    ret a / b
}
func main() -> void {
    x := divide(10, 2)
    std.fmt.println("done")
})";

inline const char* OS_SAMPLE = R"(use std.os
use std.fmt
func main() -> void {
    t := std.os.clock_ms()
    std.fmt.println("running")
    std.os.exit(0)
})";

inline const char* STRUCT_MATCH_DEFER = R"(use std.fmt

struct Counter {
    value: i32,
}

enum Dir { Up, Down }

func step(mut c: Counter, d: Dir) -> void {
    defer std.fmt.println("step done")
    match d {
        Dir.Up   => c.value += 1,
        Dir.Down => c.value -= 1
    }
})";

} // namespace sample3
} // namespace razen
