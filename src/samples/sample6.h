#pragma once

namespace razen {
namespace sample6 {

// 1 — simplest possible: function call, return constant
inline const char* HELLO_RET = R"(pub func main() -> i32 {
    ret 42
})";

// 2 — helper function + function call, variable
inline const char* ADD_FUNC = R"(func add(a: i32, b: i32) -> i32 {
    ret a + b
}

pub func main() -> i32 {
    x := add(10, 20)
    ret x
})";

// 3 — recursive factorial (early-return style to avoid "may not return")
inline const char* FACTORIAL_REC = R"(func fact(n: i32) -> i32 {
    if n <= 1 {
        ret 1
    }
    ret n * fact(n - 1)
}

pub func main() -> i32 {
    ret fact(6)
})";

// 4 — recursive fibonacci (early-return style)
inline const char* FIBONACCI_REC = R"(func fib(n: i32) -> i32 {
    if n <= 1 {
        ret n
    }
    ret fib(n - 1) + fib(n - 2)
}

pub func main() -> i32 {
    ret fib(10)
})";

// 5 — if / else with boolean expressions (early-return style)
inline const char* IF_ELSE_NESTED = R"(func max(a: i32, b: i32) -> i32 {
    if a > b {
        ret a
    }
    ret b
}

pub func main() -> i32 {
    ret max(7, 3)
})";

// 6 — enum with pattern matching (with wildcard else)
inline const char* ENUM_MATCH = R"(enum Color {
    Red,
    Green,
    Blue,
}

func code(c: Color) -> i32 {
    mut r : i32 = 0
    match c {
        Color.Red   => r = 1,
        Color.Green => r = 2,
        Color.Blue  => r = 3,
        else => r = 0,
    }
    ret r
}

pub func main() -> i32 {
    ret code(Color.Blue)
})";

// 7 — infinite loop with break
inline const char* INFINITE_LOOP = R"(pub func main() -> i32 {
    mut sum : i32 = 0
    mut i : i32 = 0
    loop {
        if i > 5 {
            break
        }
        sum = sum + i
        i = i + 1
    }
    ret sum
})";

// 8 — tagged union with pattern matching (with wildcard)
inline const char* UNION_TAGGED = R"(union Value {
    Int(i32),
    Float(f32),
}

func load(v: Value) -> i32 {
    mut r : i32 = 0
    match v {
        Value.Int(x) => r = x,
        Value.Float(y) => r = 0,
        else => r = -1,
    }
    ret r
}

pub func main() -> i32 {
    v := Value.Int(42)
    ret load(v)
})";

// 9 — nested if/else with mutable var
inline const char* NESTED_IF = R"(pub func main() -> i32 {
    mut x : i32 = 10
    if x > 5 {
        x = x * 2
    } else {
        x = 0
    }
    if x == 20 {
        x = x + 1
    }
    ret x
})";

// 10 — comprehensive: const, struct, enum, if/else, operators
inline const char* COMPREHENSIVE = R"(const MAX : i32 = 100

struct Point {
    x: i32,
    y: i32,
}

enum Status {
    Active,
    Inactive,
}

func is_positive(n: i32) -> bool {
    ret n > 0
}

func abs(n: i32) -> i32 {
    if n >= 0 {
        ret n
    }
    ret -n
}

pub func main() -> i32 {
    mut v : i32 = -5
    v = abs(v)
    if is_positive(v) {
        v = v + 1
    } else {
        v = 0
    }
    ret v
})";

} // namespace sample6
} // namespace razen
