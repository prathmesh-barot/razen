# Quickstart

Razen is a systems language with Rust-like type safety, Zig-like explicitness, and C-like performance. No GC, no exceptions, no hidden control flow.

## Hello World

```razen
func main() -> void {
    fmt.println("Hello, Razen!")
}
```

## Compiling

Currently the compiler is built via Makefile:

```bash
make          # builds razenc in project root
./razenc      # compiles built-in sample programs (file input pending)
```

Planned CMake flow (once build system is migrated):

```bash
cmake -B build
cmake --build build
./build/razenc main.rz
```

## Basic Concepts

- **Immutability by default** — `x := 10` is immutable. Use `mut x := 10` to allow reassignment.
- **No hidden allocs** — `str` points to read-only data. `string` is a heap type. Allocation always explicit.
- **Explicit errors** — No try/catch exceptions. Functions return `!T` (failable) or `Error!T` (error union). `try` propagates, `catch` handles.
- **No implicit casts** — `@as(T, expr)` for type conversion. No silent truncation or sign extension.
- **Compile-time execution** — `const` and `const func` evaluate at comptime.

## Next Steps

- [Basics](basics.md) — variables, types, operators
- [Types](types.md) — primitives, pointers, optionals, collections
- [Functions](functions.md) — declarations, generics, FFI
- [Control Flow](control_flow.md) — if, loop, match, defer, try/catch
- [Behaviours](behaviours.md) — trait system
