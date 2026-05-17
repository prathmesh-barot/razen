# Style Guide

## Naming

| Construct | Convention | Example |
|-----------|-----------|---------|
| Variables | `snake_case` | `user_name`, `buffer_size` |
| Functions | `snake_case` | `read_file`, `compute_hash` |
| Types (struct, enum, union, error) | `PascalCase` | `HttpResponse`, `Color`, `FileError` |
| Constants | `SCREAMING_SNAKE` | `MAX_BUFFER_SIZE`, `PI` |
| Behaviours | `PascalCase` | `Printable`, `Cloneable` |
| Modules | `snake_case` | `std.fmt`, `os.file` |

## Immutability

Immutable by default. Only add `mut` when mutation is required:

```razen
// preferred
name := expr
count : usize = 0

// only when mutation is needed
mut buffer := allocator.alloc(256)
mut index : usize = 0
```

## Functions

### Explicit `self` Parameter

Methods take an explicit self parameter rather than a language-level `self`/`this` keyword:

```razen
func get_name(self: *User) -> str {
    ret self.name
}
```

Convention: name it `self` of type `*TypeName`.

### Void Returns

Functions returning `void` do not need a trailing `ret`:

```razen
// good
func log(msg: str) {
    debug.write(msg)
}

// unnecessary trailing ret
func log(msg: str) {
    debug.write(msg)
    ret
}
```

Omit `-> void` from the signature — it is the default when no return type is specified.

## Error Handling

Always use `try`/`catch` for fallible operations. Do not discard errors with `_`:

```razen
// correct
const data := try read_file(path) catch |err| {
    debug.panic("failed: {}")
}

// wrong — ignores error
const data := try read_file(path) // expects catch or propagation
```

Match the error handling style to context:
- Use `try expr catch |e| { recovery }` for expression-level handling
- Use `try { block } catch (e) { handlers }` for multi-statement fallible blocks
- Propagate with `try` when the caller should handle the error

## Pointers

Dereference explicitly with `.*`:

```razen
val := ptr.*
```

Never hide pointer operations behind syntax sugar. Address-of uses `&`:

```razen
ptr := &val
```

## Allocators

Pass allocators explicitly as function parameters. Never use a global allocator:

```razen
func build_user(alloc: *@page, name: str) -> !User {
    const u := alloc.create(User)
    // ...
    ret u
}
```

## Behaviour Implementations

Within a `~>` implementation block, use the method name directly without `func`:

```razen
struct User ~> Printable {
    name: str,

    // direct implementation — no func keyword
    print_info(self: *User) -> void {
        fmt.println(self.name)
    }
}
```

Only use `func` for renaming (`func alias ~> Behaviour.method`).

## Layout

- One field per line in struct/enum/union bodies.
- Trailing comma on the last field.
- Opening brace on same line as declaration keyword.
- Indent body by 4 spaces.

```razen
struct User {
    name: str,
    age: u8,
    email: str,
}
```

## Match

Prefer exhaustive matching. Use `else` for wildcard/default:

```razen
match code {
    200 => fmt.println("ok"),
    404 => fmt.println("not found"),
    else => fmt.println("other"),
}
```

For union destructuring, name the bound variables clearly:

```razen
match value {
    Value.Int(v) => handle_int(v),
    Value.Float(f) => handle_float(f),
    Value.Text(s) => handle_str(s),
}
```

## Constants

Global constants use `SCREAMING_SNAKE`. Module-level only — do not define `const` inside functions (use `:=` instead).

```razen
const MAX_CONNECTIONS: usize = 1024
```

## Comments

Use `//` for line comments. Use `/* */` for block comments spanning multiple lines. Prefer line comments for most documentation.

```razen
// This is a single-line comment.

/* Block comments
   for longer explanations
   spanning multiple lines */
```
