#pragma once

namespace razen {
namespace sample5 {

inline const char* OPTIONAL_TYPE = R"(struct Person { name: str, age: i32, }
func find_person(id: i32) -> ?Person {
    ret null
})";

inline const char* ERROR_UNION_RETURN = R"(error FileError { NotFound, PermissionDenied, }
func read_file(path: str) -> FileError!str {
    ret FileError.NotFound
})";

inline const char* ENUM_BACKING_TYPE = R"(enum HttpStatus: u16 {
    Ok = 200,
    NotFound = 404,
    ServerError = 500,
}
func check_status(code: HttpStatus) -> void {
    s := code
})";

inline const char* ENUM_BIT_FLAGS = R"(enum Permission: u8 {
    Read  = 1 << 0,
    Write = 1 << 1,
    Exec  = 1 << 2,
})";

inline const char* PUB_VISIBILITY = R"(use std.fmt
func private_helper(x: i32) -> i32 {
    ret x * 2
}
pub func public_api(x: i32) -> i32 {
    ret private_helper(x)
}
pub func main() -> void {
    x := public_api(5)
    std.fmt.println("done")
})";

inline const char* CONST_FUNC = R"(use std.fmt
const func double(x: i32) -> i32 {
    ret x * 2
}
pub func main() -> void {
    x := double(5)
    std.fmt.println("done")
})";

inline const char* FORMAT_STRING = R"(use std.fmt
pub func main() -> void {
    name : str = "Prathmesh"
    age : i32 = 22
    std.fmt.println("Hello {}", .{name})
    std.fmt.println("Age: {}", .{age})
    std.fmt.println("Name: {} Age: {}", .{name, age})
})";

inline const char* REFERENCES = R"(use std.fmt
func get_val(ptr: *i32) -> i32 {
    ret ptr.*
}
pub func main() -> void {
    mut x : i32 = 10
    y := get_val(&x)
    std.fmt.println("done")
})";

inline const char* BEHAVE_NEEDS = R"(use std.fmt
behave Animal {
    needs voice: str
    needs name: str
    func speak(a: @Self) -> void {
        std.fmt.println("speak")
    }
})";

inline const char* ALLOCATOR_BUILTINS = R"(use std.mem
use std.fmt
func main() -> void {
    alloc := @arena
    std.fmt.println("alloc ready")
})";

inline const char* COMBINED_FEATURES = R"(use std.fmt

enum Color: u8 { Red = 0, Green = 1, Blue = 2, }

const func double(x: i32) -> i32 {
    ret x * 2
}

func helper(x: i32) -> i32 {
    ret x + 1
}

pub func main() -> void {
    c := Color.Red
    x := helper(10)
    y := double(x)
    std.fmt.println("Result: {}", .{y})
})";

} // namespace sample5
} // namespace razen
