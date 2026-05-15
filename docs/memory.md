# Memory

## Pointers

```razen
x := 42
ptr := &x        // *int pointing to x
val := ptr.*     // dereference → 42
```

`*T` is a pointer type. `&expr` takes the address. `ptr.*` dereferences.

No borrow checker — you manage pointer lifetimes explicitly. No lifetime annotations, no aliasing rules enforced by the compiler.

## Allocation

No garbage collector. No hidden allocations. Every heap allocation requires an explicit allocator parameter.

### Allocator pattern

Allocation takes an explicit `*Allocator` parameter:

```razen
func build_string(alloc: *Allocator) -> !string {
    const s := try string.with_capacity(64, alloc)
    // ...
    ret s
}
```

### Allocator builtins

| Builtin | Description |
|---------|-------------|
| `@page` | OS page allocator (mmap/VirtualAlloc). Direct, no overhead, page-granular. |
| `@arena` | Bump allocator. Fastest. Reset or deinit all at once. No per-allocation free. |
| `@fixed(buf, len)` | Fixed-size buffer allocator. No heap, fails when full. Good for embedded / scratch. |
| `@stack(N)` | Stack-based arena with N bytes inline, spills to fallback allocator when exceeded. |
| `@pool(T, N)` | Typed slab allocator. Fixed-size slots for type `T`, up to `N` elements. |
| `@c` | Wraps libc `malloc`/`free`/`realloc`. For FFI interop. |
| `@debug(parent)` | Wraps any allocator, tracks allocations, detects leaks. |
| `@log(parent)` | Wraps any allocator, logs alloc/free/resize stats. |
| `@failing(rate, parent)` | Injects allocation failures at a given rate. For testing error paths. |
| `@thread_local` | Per-thread allocator backed by `@page`. No synchronization overhead. |

## Stack vs heap

```razen
static := "hello"    // str — compile-time, read-only, no allocation
dynamic := try string.from("hello", alloc)  // string — heap, needs allocator
```

- `str` is a pointer+length into static data or stack memory. Never allocated at runtime.
- `string` is a heap-allocated, growable buffer. Every operation that allocates takes an allocator.

## Principles

- `str` (static): zero allocation at runtime.
- `string` (heap): requires explicit allocator for every allocation.
- `vec[T]`, `map[K,V]`, `set[T]`: all take allocator in constructor.
- No global allocator. No implicit malloc. If it allocates, you pass the allocator.
