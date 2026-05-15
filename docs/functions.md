# Functions

## Declaration

```razen
func add(a: i32, b: i32) -> i32 {
    ret a + b
}
```

Parameters are `name: Type`. Return type via `-> Type`. Omit `-> void` for void functions:

```razen
func greet() {
    fmt.println("hi")
}
```

## Parameters

```razen
func f(x: i32, mut y: T, const z: T, args: ...) -> void
```

- `mut` prefix — parameter is mutable inside the function body
- `const` prefix — parameter is a compile-time constant
- `...` variadic — zero or more trailing arguments

No default parameters. No function overloading.

## Variants

```razen
pub func exported() -> void         // public visibility
async func concurrent() -> void     // async (flag only, codegen pending)
const func comptime_fn() -> i32     // comptime evaluation
ext func ffi_func(x: i32) -> i32    // external/FFI
```

## Generic Functions

```razen
@Generic(T)
func identity(x: T) -> T {
    ret x
}
```

Multiple parameters: `@Generic(T, E)`. Generic params are parsed and stored but codegen is pending.

## External Functions (FFI)

```razen
ext func puts(s: *u8) -> i32
```

`ext func` generates a declaration with C ABI. Body is not provided by Razen code.

## Examples

```razen
// simple
func square(x: f32) -> f32 {
    ret x * x
}

// mutable parameter
func increment(mut x: i32) {
    x += 1
}

// void return (implicit)
func log(msg: str) {
    debug.write(msg)
}

// generic
@Generic(T)
func max(a: T, b: T) -> T {
    if a > b { ret a }
    ret b
}

// variadic
func sum(args: ...i32) -> i32 {
    mut total := 0
    // ...
    ret total
}
```
