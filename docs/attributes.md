# Attributes & Intrinsics

The `@` prefix introduces annotations and compiler intrinsics. In expression position, `@` is parsed as a builtin/annotation (`expression.cpp` line 106). In declaration position, `@` attaches metadata to the next declaration (`parser.cpp` line 108).

## Compiler Builtins

```razen
@as(Type, val)       // reinterpret cast between types
@SizeOf(T)           // comptime size of T in bytes
@AlignOf(T)          // comptime alignment of T
@TypeOf(expr)        // returns the type of expr as a comptime value
```

## Type References

```razen
@Self               // the implementing type in behave/struct methods
@Type               // the type as a comptime value
@Generic(T), @Generic(T, E)         // generic parameter binding
@Dyn                // dynamic dispatch marker
```

Used in type-position (e.g. `const T: @Type` for generic functions, `@Generic(T)` for explicit generics).

## Allocator Intrinsics

Each returns an allocator value (`std.mem.Allocator`). They are keywords in call position, not library functions.

| Intrinsic | Description |
|-----------|-------------|
| `@page` | OS page allocator backed by `mmap` |
| `@arena` | Bump allocator; supports `reset`, `restore`, `promote` |
| `@fixed` | Fixed-size buffer allocator, no heap |
| `@stack(N)` | Stack buffer of N bytes, falls back to heap on overflow |
| `@pool(T, N)` | Typed slab allocator for `T`, preallocates N slots |
| `@c` | Wraps libc `malloc`/`free`/`realloc` |
| `@debug(parent)` | Wraps parent allocator with leak detection |
| `@log(parent)` | Wraps parent allocator; logs allocation stats |
| `@failing` | Test helper that injects allocation failures at configurable rate |
| `@thread_local` | Thread-local storage allocator |

Example:

```razen
alloc := @arena
ptr := alloc.alloc(Layout.of::i32)
```
