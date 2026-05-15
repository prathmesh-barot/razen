# Modules

## Declaration

```razen
mod Name;
```

Declares a module named `Name`. The parser creates a `ModuleDeclaration` (`parser.cpp::parseModule`). No body — modules are file-scoped containers.

## Import

```razen
use path.to.module;
```

Imports all public symbols from `path.to.module` into the current scope. The parser collects the dotted path into a string and creates a `UseDeclaration` (`parser.cpp::parseUse`). Relative to project root; `std` is reserved for the standard library.

## Visibility

The `pub` keyword controls visibility on all top-level declarations:

```razen
pub func
pub struct
pub enum
pub union
pub type
pub const
pub behave
pub mod
pub use
```

Without `pub`, the declaration is module-private. `pub use` re-exports.

## Path Resolution

Dotted paths resolve from the current scope outward:

```razen
std.fmt.println(...)
```

The first component is looked up in scope (imported names, std lib injected automatically). Each subsequent `.` resolves to a member of the preceding module/namespace.

## Rules

- No circular dependencies between modules.
- File-as-module model is planned: each `.rzn` file implicitly declares a module matching its path.

## Example

```razen
mod network

use std.fmt

pub func connect() -> void {
    fmt.println("connected")
}
```
