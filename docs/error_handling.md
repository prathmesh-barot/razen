# Error Handling

No exceptions. Errors are explicit values propagated with `try` or handled with `catch`.

## Error sets

```razen
error FileError {
    NotFound,
    PermissionDenied,
    InvalidPath,
}
```

Error sets are named collections of error variants. Variants are values, not types with payloads.

## Error union return

```razen
func read_file(path: str) -> FileError!str
```

`Error!T` means: returns `T` on success, or one of the error variants on failure.

## try — propagate

```razen
func read_config() -> FileError!str {
    const data := try read_file("config.txt")
    ret data
}
```

`try expr` unwraps the value on success, or returns the error from the enclosing function immediately.

## catch — handle

Two forms:

```razen
// expression try with catch
const data := try read_file("config.txt") catch |err| {
    fmt.println("error: {}", .{err})
    ret "default"
}

// block try with catch
try {
    const data := read_file("config.txt")
    fmt.println(data)
} catch (e) {
    fmt.println("failed: {}", .{e})
}
```

- `|err|` / `(e)` binds the error value in the catch handler.
- catch must return or diverge if the function returns a value.

## ret with error

```razen
func fail() -> FileError!str {
    ret FileError.NotFound
}
```

## noret

`noret` is the diverging type. Functions that never return (panic, exit, infinite loop) use it:

```razen
func panic(msg: str) -> noret
```

Catch handlers can call `noret` functions to satisfy type checking without producing a value.

## Summary

| Construct | Purpose |
|-----------|---------|
| `error Name { A, B }` | Declare error set |
| `-> Error!T` | Error union return type |
| `try expr` | Propagate error to caller |
| `try { } catch (e) { }` | Block try with handler |
| `try expr catch \|e\| { }` | Expression try with handler |
| `ret Error.Variant` | Return an error |
| `noret` | Never-returning type |
