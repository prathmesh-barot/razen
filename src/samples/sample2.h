#pragma once

namespace razen {
namespace sample2 {

inline const char* RETURN_ZERO = R"(func main() -> i32 {
    ret 0
})";

inline const char* ARITH_EXPR = R"(func compute() -> i32 {
    a : i32 = 3 + 4 * 2
    b := a - 1
    ret a + b
})";

inline const char* IF_ELSE = R"(func check(n: i32) -> bool {
    if n > 0 {
        ret true
    } else {
        ret false
    }
})";

inline const char* FULL_PROGRAM = R"(use std.fmt

const MAX : i32 = 100

func add(a: i32, b: i32) -> i32 {
    ret a + b
}

func is_even(n: i32) -> bool {
    ret n % 2 == 0
}

pub func main() -> void {
    x : i32 = 10
    y := 20
    mut result : i32 = add(x, y)
    if result == 30 {
        result += 1
    } else {
        result = 0
    }
    mut counter : i32 = 0
    loop {
        if counter == MAX {
            break
        }
        counter += 1
    }
    std.fmt.println("done")
})";

inline const char* PHASE_2_EXHAUSTIVE = R"(mod Network
use std.fmt
use std.os

type Flags = u32

behave SerDe {
    needs tag: u8
    func serialize(x: @Self) -> [u8]
}

struct Packet ~> SerDe {
    tag: u8,
    data: [u8],
}

enum State {
    Open,
    Closed,
}

union NetErr {
    Code: i32,
    Msg: str,
}

error SystemError { ConnReset, Timeout }

ext func bind(port: int) -> int

pub func handle_conn() -> void {
    defer std.fmt.println("Closed!")

    s := State.Open
    match s {
        State.Open   => std.fmt.println("open"),
        State.Closed => std.fmt.println("closed")
    }

    items := [1, 2, 3]
    loop items |i| {
        std.fmt.print(i)
    }

    res := try bind(8080) catch |e| { ret }
}

pub func main() -> void {
    handle_conn()
    std.os.exit(0)
})";

} // namespace sample2
} // namespace razen
