# Control Flow

Razen provides a set of lean, explicit control flow primitives designed for predictability and performance.

## Conditionals
The `if` statement handles branching. You can chain multiple conditions using `else if`:

```razen
if score > 90 {
    fmt.println("Grade A")
} else if score > 80 {
    fmt.println("Grade B")
} else {
    fmt.println("Grade C")
}
```

## Loops
Razen uses a universal `loop` construct. It is an infinite loop by default, and the developer explicitly defines the exit condition.

```razen
mut i := 0
loop {
    if i >= 10 {
        break // Exit the loop
    }
    fmt.println(i)
    i += 1
}
```

- `break`: Terminates the loop.
- `skip`: Skips the current iteration and proceeds to the next.

## Pattern Matching
The `match` statement is a powerful tool for exhaustive branching based on values, enums, and unions.

### Basic Matching
```razen
match state {
    State.Idle => fmt.println("Waiting..."),
    State.Running => fmt.println("Working..."),
    State.Stopped => fmt.println("Done."),
}
```

### Union Destructuring
Payload unions can be destructured directly in the match arm to access their inner data.

```razen
match value {
    Value.Int(n) => fmt.println("Integer: {}", .{n}),
    Value.Text(s) => fmt.println("String: {}", .{s}),
}
```

### Struct Destructuring
Matching can also be used to extract fields from structs.
```razen
match user {
    User { id, active } => fmt.println("User {} is active: {}", .{id, active}),
}
```

## Error Handling (`try` / `catch`)
Razen handles errors explicitly. The `try` keyword is used to attempt an operation that might return an error.

```razen
try {
    const data = read_file("config.txt")
    fmt.println(data)
} catch (err) {
    fmt.println("Error occurred: {}", .{err})
}
```

## Defer
The `defer` keyword schedules a block of code to run exactly when the current scope exits. This is essential for resource cleanup (e.g., closing files or releasing locks).

`defer` accepts a single statement without braces (`defer stmt`), or a block `{ }` for multiple statements.

```razen
func process_file() -> void {
    const file = open_file("test.txt")
    defer file.close() // Executes when process_file returns

    // ... process file ...
}
```

Defers are executed in **Last-In, First-Out (LIFO)** order.

## Range Expressions
Range expressions create a range of values using `..` (inclusive-exclusive) and `..=` (inclusive-inclusive).

```razen
// Exclusive range: 0, 1, 2, 3, 4
range1 := 0..5

// Inclusive range: 0, 1, 2, 3, 4, 5
range2 := 0..=5
```

Ranges are commonly used with loops and slices.

## Capture Blocks
Capture blocks use the `|e|` syntax to create a closure or inline expression context.

```razen
// Single expression capture
filtered := items.filter(|e| e > 0)

// Block body capture
sorted := items.sort(|a, b| {
    if a < b { ret Ordering.Less }
    else if a > b { ret Ordering.Greater }
    else { ret Ordering.Equal }
})
```
