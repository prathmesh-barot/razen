# FAQ

## Why another language?

Existing systems languages force trade-offs between control and convenience. Razen targets the space where you need Rust-level type safety without the borrow checker, Zig-level explicitness with syntactic regularity, and C-level performance without undefined behavior footguns.

## How is this different from Zig?

- Razen has `match` expressions with pattern matching and destructuring.
- Functions are `func`, not `fn`. Struct literals use commas, not semicolons.
- Error handling uses `try`/`catch` with error unions (`Error!T`), similar conceptually but syntactic differences.
- Razen has behaviours (traits) with `~>` impl syntax and `needs` field requirements.
- Razen uses `loop` for all iteration (infinite, conditional, iterator) instead of Zig's `while`/`for`.

## How is this different from Rust?

- **No borrow checker.** Razen uses explicit `*T` pointers. The programmer manages lifetimes.
- No `enum` with payloads — payload variants are in `union`. `enum` is for simple tagged values (with integer backing optional).
- No `impl` blocks — methods are written directly inside `struct`/`enum`/`union` bodies.
- No `trait` — Razen has `behave` with `~>` implementation syntax and optional `@Dyn` for dynamic dispatch.
- No macros — Razen uses `@` annotations and comptime `const func` evaluation instead.
- No `;` required after statements (Rust's expression-ending semicolons are optional in Razen).

## Does Razen have a borrow checker?

No. Razen uses explicit pointer types (`*T`), explicit address-of (`&x`), and explicit dereference (`ptr.*`). The programmer is responsible for memory safety. The compiler checks type compatibility and mutability (you cannot write to an immutable binding), but there is no lifetime analysis or borrow checking.

## Does Razen have exceptions?

No. Errors use explicit error unions (`Error!T` / `!T`), propagated with `try` and handled with `catch`. The error path is visible in the return type and the syntax. No hidden control flow, no unwinding.

## Is Razen garbage collected?

No. Razen uses explicit allocators passed as parameters. Every allocation is visible in the source code. The language defines allocator patterns (`@page`, `@arena`, `@fixed`, `@stack`, `@pool`, `@c`, `@debug`, etc.) that the programmer chooses explicitly.

## Can I call C code?

Yes. `ext func` declares an external function with C ABI:

```razen
ext func puts(s: *u8) -> i32
```

`ext struct`, `ext enum`, and `ext union` can also be used for FFI type declarations. `ext` can also implement behaviours on external types.

## Does Razen have async/await?

Planned. The `async func` keyword flag exists in the lexer and parser but codegen is not implemented. The async model is not yet designed.

## Can I write Razen in Razen?

Planned. Self-hosting is Phase 7 of the roadmap. The compiler is currently written in C++20. The goal is to bootstrap the compiler so Razen compiles itself.

## Is there a package manager?

Not yet. There is no dependency management or package registry. Modules are currently file-system-based within a single project.

## What architectures are supported?

Currently targeting x86-64 (via LLVM). ARM64 is planned. The LLVM backend handles architecture-specific codegen; once the IR pipeline is complete, adding targets depends on LLVM support.

## What is the license?

Apache 2.0.
