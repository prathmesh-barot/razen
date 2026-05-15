#pragma once

namespace razen {
namespace semantic_errors {

inline const char* S01_IMMUTABLE_ASSIGN = R"(func main() -> void {
    x : i32 = 10
    x = 20
    y : bool = true
    y = false
    z := 1
    z = 2
    ret
})";

inline const char* S02_REDECLARED_VAR = R"(func main() -> void {
    a : i32 = 1
    a : i32 = 2
    b : bool = true
    b : bool = false
    c := 0
    c := 1
    ret
})";

inline const char* S03_UNDECLARED_IDENT = R"(func main() -> void {
    result := x + 1
    y := z + 2
    ret
})";

inline const char* S04_ARG_COUNT_MISMATCH = R"(func add(a: i32, b: i32) -> i32 { ret a + b }
func triple(a: i32, b: i32, c: i32) -> i32 { ret a + b + c }
func main() -> void {
    x := add(5)
    y := triple(1, 2)
    ret
})";

inline const char* S05_RETURN_TYPE_MISMATCH = R"(func get_flag() -> bool { ret 0 }
func main() -> i32 {
    ret true
})";

inline const char* S06_BREAK_OUTSIDE_LOOP = R"(func test1() -> void {
    if true { break }
    if false { skip }
    ret
}
func test2() -> void {
    break
    skip
    ret
}
func main() -> void { ret }
)";

inline const char* S07_IF_COND_NOT_BOOL = R"(func main() -> void {
    if 42 { ret }
    if 3 + 4 { ret }
    ret
})";

inline const char* S08_STRUCT_FIELD_NOT_FOUND = R"(struct Point { x: i32, y: i32 }
struct Line { start: Point, end: Point }
func main() -> void {
    p : Point
    z := p.z
    l : Line
    m := l.mid
    ret
})";

inline const char* S09_UNDECLARED_FUNC = R"(func main() -> void {
    unknown_func()
    missing_func(42)
    ret
})";

inline const char* S10_GLOBAL_DUPLICATE = R"(func helper() -> void { ret }
func helper() -> void { ret }
const X : i32 = 1
const X : i32 = 2
func main() -> void { ret }
)";

} // namespace semantic_errors
} // namespace razen
