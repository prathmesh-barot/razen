# Syntax Reference

This is a complete grammar reference. Not tutorial material.

## Declarations

### Functions

```
func name(params) -> Type { body }
pub func name(params) -> Type { body }
const func name(params) -> Type { body }
async func name(params) -> Type { body }
ext func name(params) -> Type
```

- `pub` makes it visible outside the module.
- `const func` is evaluated at compile time.
- `async func` is a flag-only declaration; codegen pending.
- `ext func` declares an external (C ABI) function with no body.
- Return type is required after `->`. Omit `-> void` for void functions — the `-> void` is optional.

### Generic Functions

```
@Generic(T) func name(params) -> Type { body }
@Generic(T, E) func name(params) -> Type { body }
```

Generic parameters are parsed and stored on the AST. Codegen pending.

### Parameters

```
func f(x: Type, mut y: Type, const z: Type, args: ...) -> void
```

- `mut` — parameter is mutable in the body
- `const` — compile-time parameter
- `...` — trailing variadic argument

### Structs

```
struct Name { field: Type, ... }
struct Name ~> BehaveName { field: Type, ..., method_declarations }
```

- Fields separated by commas.
- Trailing comma allowed.
- `~>` attaches a behaviour implementation.
- Methods inside a struct use `func method_name(self: *Name) -> Type { body }`.
- Behaviour implementations omit the `func` keyword: `method_name(self: *Name) { body }`.
- Renaming uses `func alias ~> Behave.method(self: *Name) { body }`.
- Structs can have field default values with `field: Type = expr`.

### Enums

```
enum Name { Variant, ... }
enum Name: BackingType { Variant = expr, ... }
```

- Backing type is an integer type (u8, i16, etc.).
- Variants can have explicit discriminant values (must be comptime-evaluable).
- Enums support methods and `~>` behaviour impls, same syntax as structs.

### Unions

```
union Name { Variant, ... }
union Name { Variant(Type), ... }
union Name { Variant { f: Type, ... }, ... }
```

- Unit variants: just a name.
- Tuple variants: `Variant(Type, Type)`.
- Struct variants: `Variant { f: Type, ... }`.
- Mixed styles allowed in one union.

### Error Sets

```
error Name { Variant, ... }
```

Error sets are named collections of error values. Variants have no payload.
Used with error union types: `Name!T`.

### Behaviours (Traits)

```
behave Name { 
    func method(s :@Self) -> Type,
    needs field: Type
}
```

- `@Self` is the implementing type.
- `needs` declares a field requirement — implementing types must have this field.
- Methods can have default bodies.
- Behaviour inheritance: `behave Child ~> Parent { ... }`

### External Implementations

```
ext struct Type ~> Behave { func method(@Self) -> Type { body } }
ext enum Type ~> Behave { ... }
ext union Type ~> Behave { ... }
ext func name(params) -> Type
```

### Type Aliases

```
type Alias = Type
```

### Constants

```
const NAME: Type = expr
const mut NAME: Type = expr
```

Constants are compile-time evaluated. Must have explicit type.

### Modules

```
mod Name
```

Declares a submodule. No block — just a name and optional body.

### Imports

```
use path.to.module
use path.to.module as Alias
```

Brings a module or symbol into scope.

## Statements

### Variable Declarations

```
name := expr                 // type inferred
name : Type = expr            // type explicit
mut name := expr              // mutable, inferred
mut name : Type = expr        // mutable, explicit
```

No uninitialized variables. Every declaration must have a value.

### Assignment

```
name = expr
name += expr
name -= expr
name *= expr
name /= expr
name %= expr
```

Compound assignments are parsed as Assignment nodes with operator metadata.

### Return

```
ret expr
ret
```

`ret` with no value is for void functions. `ret` with value for non-void.

### If / Else

```
if cond { }
if cond { } else { }
if cond { } else if cond { } else { }
```

Condition must be boolean. No implicit truthiness.

### Loop

```
loop { }                    // infinite
loop cond { }               // conditional (while-like)
loop expr |item| { }        // iterator (for-like)
```

- `break` exits the loop.
- `skip` jumps to next iteration.
- Break/skip outside a loop is a compile error.

### Defer

```
defer { }
defer stmt
```

Runs at scope exit. LIFO order. Accepts block or single statement.

### Match

```
match expr {
    pattern => body,
    pattern => { body },
    else => body,
}
```

Patterns:
- Literal: `42`, `"hello"`, `true`
- Enum: `Color.Red`
- Union destructure: `Variant(vars...)` or `Variant { field, ... }`
- Wildcard: `else`

Arms are comma-separated. Body is an expression or block.

### Try / Catch

```
try expr
try expr catch |err| { handler }
try { block } catch (err) { handler }
```

- `try expr` unwraps an error union, propagating errors to the caller.
- Expression try with catch: unwrap or handle.
- Block try: wrap fallible operations, catch any error from the block.

## Expressions (Precedence-Climbing, 12 Levels)

### Primary

```
literal              // int, float, string, char, bool
identifier           // name, path.to.symbol
(expr)               // grouping
```

### Unary (precedence 12, right-to-left)

```
-expr
!expr
~expr                // bitwise not
&expr                // address-of
```

### Postfix Dereference (precedence 11, left-to-right)

```
ptr.*                // dereference pointer
```

### Member Access (precedence 10, left-to-right)

```
a.b
```

### Function Calls (precedence 9, left-to-right)

```
f(args)
name(arg, ...)
```

### Binary Operators

| Precedence | Operators | Assoc |
|-----------|-----------|-------|
| 12 | `*` `/` `%` | left |
| 11 | `+` `-` | left |
| 10 | `<<` `>>` | left |
| 9 | `&` | left |
| 8 | `^` | left |
| 7 | `\|` | left |
| 6 | `==` `!=` `<` `>` `<=` `>=` | left |
| 5 | `&&` | left |
| 4 | `\|\|` | left |
| 3 | `..` `..=` | left |
| 2 | `=` `+=` `-=` `*=` `/=` `%=` | right |

### Range

```
expr..expr
expr..=expr
expr...expr          // exclusive end (parsed as RangeExpression node, precedence 11)
```

### Array Literals

```
[expr, expr, ...]
```

### Tuple Literals

```
.{expr, expr, ...}
```

### Capture Blocks

```
|param| expr
```

### Annotations / Builtins

```
@name
@name(args)
@Generic(T)
@Self                 // current type (in method bodies)
@Type                 // type of value (comptime)
@as(Type, expr)       // explicit type conversion
@TypeOf(expr)
@SizeOf(Type)
@AlignOf(Type)
```

## Types

### Primitives

| Category | Types |
|----------|-------|
| Signed integer | `i1` `i2` `i4` `i8` `i16` `i32` `i64` `i128` `isize` `int` |
| Unsigned integer | `u1` `u2` `u4` `u8` `u16` `u32` `u64` `u128` `usize` `uint` |
| Float | `f16` `f32` `f64` `f128` `float` |
| Other | `bool` `char` `void` `noret` `any` |
| Strings | `str` `string` |

Arbitrary-width integers up to 32768 bits: `i7`, `u93`, etc.

### Compound Types

```
*T                  // pointer to T
?T                  // optional (nullable)
!T                  // failable (may produce anonymous error)
Error!T             // error union (named error set)
error!T             // error union (anonymous error set)
[T]                 // array (sized)
[T; N]              // array with explicit element count
vec[T]              // dynamic vector (collection)
map{K, V}           // hash map (collection)
set{T}              // hash set (collection)
```

### Type Modifiers

```
mut T               // mutable type modifer
```

### Built-in Type References

```
@Self               // current type (method bodies)
@Type               // type of a value (comptime)
@Generic(T)         // generic parameter
```

## Annotations (Attributes)

```
@name
@name(arg)
@Generic(T)
@Dyn
```

Annotations are parsed as Annotation AST nodes. Stored on the declaration they precede.
`@Generic` and `@Dyn` have specific semantic meaning; others are passthrough.

## Comments

```
// line comment
/* block comment */
```

Block comments can span multiple lines. Both are preserved as Comment AST nodes.

## Literals

```
42                  // integer (decimal)
3.14                // float
"hello\nworld"      // string (escape sequences: \n \t \" \\ \' \0 \r \xNN)
'A'                 // char
true                // bool
false               // bool
```
