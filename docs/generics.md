# Generics

Razen uses compile-time monomorphization. A `@Generic(T)` annotation on a function or struct makes it generic over type `T`. The compiler generates a specialized copy for each type combination used.

## Generic functions

```razen
@Generic(T) func identity(x: T) -> T {
    ret x
}

@Generic(T, E) func pair(first: T, second: E) -> .{T, E} {
    ret .{first, second}
}
```

Multiple type parameters are listed: `@Generic(T, E)`.

## Generic structs

```razen
@Generic(T) struct Container {
    value: T,
}
```

Generic structs are monomorphized at each use site:

```razen
c := Container(int){ .value = 42 }
```

## Type parameters via `@Type`

Pass types as runtime-visible values:

```razen
func wrap(const T: @Type, val: T) -> T {
    ret val
}
```

`@Type` is the compile-time type-of-types. Parameters declared `const` are evaluated at compile time.

## Monomorphization

- Each unique set of type arguments produces a separate function/struct instance.
- No vtable overhead, no dynamic dispatch.
- Generic expansions happen during compilation — no runtime type erasure.

## Dynamic dispatch vs generics

```razen
// generics — monomorphized, static dispatch
@Generic(T) func process(items: []T) { ... }

// @Dyn behaviour — runtime dispatch
@Dyn behave Processor {
    func process(p: @Self) -> void
}

func run_all(items: []@Dyn Processor) {
    for item in items { item.process() }
}
```

Use `@Dyn` behaviours when you need heterogeneous collections or runtime polymorphism. Use generics when types are known at compile time.

## Constraints

Generic constraints (e.g. `T ~> Eq`) are not yet implemented. Any type parameter currently accepts any type. This will be added in a future version.
