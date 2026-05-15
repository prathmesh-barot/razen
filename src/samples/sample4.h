#pragma once

namespace razen {
namespace sample4 {

inline const char* MATCH_PAYLOAD = R"(use std.fmt
union Value {
    Int(i32),
    Float(f32),
    Text(str),
}
func describe(value: Value) -> void {
    match value {
        Value.Int(v)   => std.fmt.println("int"),
        Value.Float(v) => std.fmt.println("float"),
        Value.Text(v)  => std.fmt.println("text")
    }
})";

inline const char* UNION_CONSTRUCTOR = R"(use std.fmt
union Value {
    Int(i32),
    Float(f32),
}
func make_int() -> Value {
    x := Value.Int(42)
    ret x
})";

inline const char* ASSIGNMENT_IN_MATCH = R"(use std.fmt
struct Counter { value: i32, }
enum Dir { Up, Down }
func step(mut c: Counter, d: Dir) -> void {
    match d {
        Dir.Up   => c.value += 1,
        Dir.Down => c.value -= 1
    }
    std.fmt.println("done")
})";

inline const char* COMBINED_C6_C7_C8 = R"(use std.fmt

union Expr {
    Num(i32),
    Neg(i32),
}

struct Calc { result: i32, }

func eval(mut c: Calc, e: Expr) -> void {
    match e {
        Expr.Num(v) => c.result = v,
        Expr.Neg(v) => c.result -= v
    }
    std.fmt.println("done")
}

func main() -> void {
    n := Expr.Num(10)
    eval_result := n
    std.fmt.println("done")
})";

} // namespace sample4
} // namespace razen
