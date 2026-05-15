# Control Flow

## if / else

```razen
if cond { }
if cond { } else { }
if cond { } else if cond { } else { }
```

Condition is any boolean expression. No implicit truthiness.

## loop

Three forms:

```razen
// infinite
loop { break }

// conditional — loops while cond is true
loop cond { }

// iterator — binds item variable per iteration
loop iter |item| { fmt.println(item) }
```

- `break` exits the loop.
- `skip` jumps to next iteration.

## defer

Runs code at scope exit. LIFO order — last defer runs first.

```razen
{
    defer { cleanup() }
    defer file.close()
    // ... work ...
} // file.close() runs first, then cleanup()
```

Accepts a block `defer { }` or a single statement `defer stmt`.

## match

```razen
match expr {
    pat => body,
    ...
}
```

### Literal patterns

```razen
match code {
    200 => fmt.println("OK"),
    404 => fmt.println("Not Found"),
    else => fmt.println("Other"),
}
```

The `else` case serves as wildcard / default (`_`).

### Enum patterns

```razen
match color {
    Color.Red => fmt.println("red"),
    Color.Green => fmt.println("green"),
    Color.Blue => fmt.println("blue"),
}
```

### Union destructuring

```razen
match value {
    Value.Int(v) => fmt.println("int: {}", .{v}),
    Value.Float(f) => fmt.println("float: {}", .{f}),
    Value.Text(s) => fmt.println("str: {}", .{s}),
}
```

### Struct destructuring

```razen
match expr {
    Expr.Binary { left, right, op } => eval_binary(left, right, op),
    Expr.Number(n) => fmt.println(n),
}
```

Each arm is a `pattern => body` pair separated by commas. Body can be an expression or a block `{ }`.
