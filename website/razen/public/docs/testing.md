# Testing

Status: **Not yet implemented.** Token exists in the lexer; parsing and codegen are planned.

## `test` Keyword

The `test` token is lexed (`TokenType::Test`) but not yet parsed. The planned syntax uses `test` as a declaration keyword:

```razen
test "description" {
    // assertions
}
```

## `#[test]` Attribute (Planned)

An alternative annotation-based form:

```razen
#[test] func test_addition() -> void {
    // assertions
}
```

This marks an ordinary function as a test that is compiled and run by the test runner.

## `std.testing` Module (Planned)

The standard library will provide a `std.testing` module with:

### Test Struct

```razen
struct Test {
    // test state
}
```

### Assertion Functions

| Function | Description |
|----------|-------------|
| `eq(expected, actual)` | Assert equality |
| `neq(a, b)` | Assert not-equal |
| `ok(val)` | Assert value is truthy/success |
| `err(val)` | Assert value is an error |
| `is_true(cond)` | Assert boolean true |
| `is_false(cond)` | Assert boolean false |
| `near(a, b, epsilon)` | Assert float within tolerance |
| `skip(msg)` | Skip this test |
| `fail(msg)` | Force test failure |

### Example (Planned Syntax)

```razen
use std.testing

#[test] func test_math() -> void {
    testing.assert(2 + 2 == 4)
    testing.eq(4, 2 + 2)
    testing.is_true(3 > 1)
}

#[test] func test_with_error() -> void {
    const result := try fallible_op()
    testing.eq(42, result)
}
```

## Test Runner (Planned)

```razen
std.testing.run(filter: ?str)     // run tests matching filter
std.testing.run_all()             // run all tests
```

## Current Status

| Feature | Status |
|---------|--------|
| `test` token | ✓ Lexed |
| `test` declaration parsing | ☐ Planned |
| `#[test]` attribute parsing | ☐ Planned |
| `std.testing` module | ☐ Not started |
| Test runner | ☐ Not started |
| Integration with build system | ☐ Not started |
