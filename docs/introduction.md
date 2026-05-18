# Introduction

Razen is a systems programming language. No hidden magic, no implicit allocations, no runtime overhead you didn't ask for. The code you write is the code that executes.

## Philosophy

**Meaningful, Accurate, Simple, Maximum Performance.**

- **Meaningful** — Every construct has exactly one job. No ambiguous syntax, no context-dependent behavior.
- **Accurate** — Types, memory, and control flow are explicit. Nothing happens behind your back.
- **Simple** — Complexity is a liability. Features earn their place by improving performance or safety, not by adding sugar.
- **Maximum Performance** — Zero-cost abstractions. High-level code compiles to the machine instructions you'd write by hand.

## Four Pillars

| Pillar | Meaning |
|--------|---------|
| No hidden allocs | Every allocation takes an explicit allocator parameter. No GC, no heap-allocated closures. |
| No implicit casts | All type conversions are spelled out. `@as(T, expr)` or `T(expr)`. |
| Predictable control flow | No exceptions. Errors are values (`!T`, `Error!T`). `defer` runs at scope exit. |
| Compile-time evaluation | `const` and `const func` execute at comptime. No runtime metaprogramming. |

## Who This Is For

Engineers who work in C, Zig, Rust, or Go and want:
- Rust's type safety without the borrow checker complexity
- Zig's explicitness with more syntactic convenience
- C's performance without the footguns
- Go's simplicity with real control over memory

## Status

**Early development. Experimental. Not production-ready.**

The lexer, parser, and semantic analyzer are complete. LLVM codegen is ~85% complete with object/assembly emission working. The standard library has a minimal `fmt` module. See [ROADMAP.md](../ROADMAP.md) for current progress and milestones.
