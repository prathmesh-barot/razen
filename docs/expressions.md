# Expressions

## Primary Expressions

Literals — `42`, `3.14`, `true`, `'x'`, `"hello"`. Identifiers resolve to local/global bindings. Grouped: `(expr)`.

## Unary Operators

| Op | Meaning |
|----|---------|
| `-expr` | Numeric negation |
| `!expr` | Logical not |
| `~expr` | Bitwise not |
| `&expr` | Address-of, yields `*T` |

## Postfix Operators

| Op | Meaning |
|----|---------|
| `ptr.*` | Dereference `*T` → `T` |
| `a.b` | Member access (struct field, enum variant) |
| `f(args)` | Function call |

## Binary Operator Precedence (highest to lowest)

| Level | Operators | Assoc | Description |
|-------|-----------|-------|-------------|
| 12 | `.` | left | Member access |
| 11 | `..` `..=` | left | Range (exclusive, inclusive) |
| 10 | `*` `/` `%` | left | Multiplicative |
| 9 | `+` `-` | left | Additive |
| 8 | `<<` `>>` | left | Shift |
| 7 | `&` | left | Bitwise AND |
| 6 | `|` `^` | left | Bitwise OR / XOR |
| 5 | `<` `>` `<=` `>=` | left | Relational |
| 4 | `==` `!=` | left | Equality |
| 3 | `&&` | left | Logical AND |
| 2 | `||` | left | Logical OR |
| 1 | `catch` | left | Error recovery |

Precedence is encoded in `token_utils.h::getPrecedence()`. The parser uses a Pratt-style top-down operator precedence loop in `parseBinaryExpr`.

## Range

```razen
a..b    // exclusive: [a, b)
a..=b   // inclusive: [a, b]
```

Parsed as `RangeExpression` AST node when `..` or `..=` is encountered.

## Array Literal

```razen
[1, 2, 3]
```

Parsed as `ArrayLiteral`. Elements are comma-separated binary expressions.

## Tuple Literal

```razen
.{42, "hello", true}
```

Parsed as `TupleLiteral`. The `.{` sequence enters tuple parsing.

## Capture / Closure

```razen
|x| expr
|x| { body }
```

Parsed as `CaptureBlock`. Pipe-delimited parameter followed by an expression or block body.

## Builtins / Annotations

```razen
@as(Type, expr)     // type cast
@Name(args)         // annotation or intrinsic call
```

The `@` token in expression position enters builtin parsing (`expression.cpp` line 106). If the name is `as`, the next tokens are parsed as `@as(Type, value)`. Otherwise the result is an `Annotation` node. If the annotation is followed by `(...)`, it becomes a function call on the annotation.
