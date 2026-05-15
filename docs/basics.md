# Basics

## Comments

```razen
// single-line

/* multi-line
   block */
x := 10 /* inline */
```

## Variables

Immutable by default. The type can be inferred or explicit:

```razen
name := expr        // inferred
name : Type = expr  // explicit
```

Mutable:

```razen
mut name := expr
mut name : Type = expr
```

No uninitialized variables. Every binding must have a value.

## Constants

Compile-time evaluated. Must have explicit type.

```razen
const MAX_BUF : usize = 4096
const mut COUNTER : i32 = 0   // comptime mutable
```

## Primitive Types

| Category | Types |
|----------|-------|
| Signed ints | `i1`, `i2`, `i4`, `i8`, `i16`, `i32`, `i64`, `i128`, `isize`, `int` (= i32) |
| Unsigned ints | `u1`, `u2`, `u4`, `u8`, `u16`, `u32`, `u64`, `u128`, `usize`, `uint` (= u32) |
| Floats | `f16`, `f32`, `f64`, `f128`, `float` (= f32) |
| Other | `bool`, `char`, `void`, `noret`, `any` |
| Strings | `str` (immutable slice), `string` (heap-allocated) |

Arbitrary integer widths up to 32768 bits are supported (e.g. `i7`, `u257`).

## Operators

### Arithmetic

`+` `-` `*` `/` `%` with compound: `+=` `-=` `*=` `/=` `%=`

### Comparison

`==` `!=` `<` `>` `<=` `>=`

### Logical

`&&` `||` `!`

### Bitwise

`&` `|` `^` `~` `<<` `>>`

### Assignment

`=` (assign), `:=` (declare+infer)

### Access

`.` member access, `@` builtin/intrinsic prefix
