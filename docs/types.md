# Types

## Primitives

Signed: `i1` `i2` `i4` `i8` `i16` `i32` `i64` `i128` `isize` `int` (= i32)

Unsigned: `u1` `u2` `u4` `u8` `u16` `u32` `u64` `u128` `usize` `uint` (= u32)

Floats: `f16` `f32` `f64` `f128` `float` (= f32)

Other: `bool` `char` `void` `noret` `any`

Arbitrary-width integers up to 32768 bits (`i7`, `u513`, etc.).

## Pointers

```razen
ptr : *T          // pointer to T
addr := &x        // address-of, returns *T
val := ptr.*      // dereference
```

## Optionals

```razen
opt : ?T          // may hold T or null
```

Nullable pointers use `?*T`.

## Failables

```razen
result : !T       // returns T or an anonymous error
```

## Error Unions

```razen
result : IOError!T        // named error set
result : error!T          // anonymous error set
```

`try` propagates the error. `catch` handles it.

## Arrays

```razen
arr : [T]          // sized array, elements contiguous
arr : [T; N]       // array with explicit count
lit := [1, 2, 3]   // array literal
```

## Slices

Planned. Not yet implemented.

## Collections

```razen
v : vec[T]         // dynamic vector
m : map{K, V}      // hash map
s : set{T}          // hash set
```

## Tuples

```razen
t := .{1, "hello", true}
```

Unnamed, positional. Access via index.

## Strings

```razen
s1 : str = "literal"     // immutable, points to data section
mut s2 : string = "heap" // growable, heap-allocated
```

`str` is a `(ptr, len)` slice. `string` is an owning heap type.

## Special Types

- `void` — no value (function return)
- `noret` — diverging (never returns)
- `any` — opaque type, holds anything

## Type Modifiers

```razen
mut x : mut T  // mutable binding of a mutable type
```

## Built-in Type References

```razen
@Self         // the current type (inside struct/enum methods)
@Type         // the type of a value (comptime)
@Generic(T)   // generic parameter placeholder
```
