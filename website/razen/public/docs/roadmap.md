# Roadmap

**Current progress:** Phases 1-3 (Lexer, Parser, Semantic Analysis) complete. Phase 4 (LLVM Codegen) not started. Phases 5-7 not started.

For the full detailed checklist with per-item status, see [ROADMAP.md](../ROADMAP.md).

---

## Phase 1: Lexer — Complete

All token types are defined and lexed: keywords, primitive types, operators, delimiters, literals (int, float, string, char, bool), comments (line and block). Line/column tracking on every token. Error handling with context.

Full list of tokens in `src/lexer/token.h`.

## Phase 2: Parser & AST — Complete

Recursive descent parser covering all declarations, statements, and expressions.

### Declarations
- Functions (with pub/async/const/ext variants)
- Generic parameters (`@Generic(T)`)
- Struct, enum (with backing types), union (unit/tuple/struct variants)
- Error sets, behaviours (traits) with `needs` and `~>` inheritance
- Type aliases, constants, modules, imports

### Statements
- Variable declarations (`:=` / `: Type =`), mutability
- Assignment and compound assignment
- `ret`, `if`/`else` with else-if chaining
- `loop` (infinite, conditional, iterator), `break`, `skip`
- `defer` (block and statement forms)
- `match` with literal/enum/destructure/wildcard patterns
- `try`/`catch` (expression and block forms)
- `@` annotations and builtins (`@as`, `@Generic`, etc.)

### Expressions
- Precedence-climbing expression parser (12 levels)
- All binary/unary operators, ranges (`..` `..=`), array/tuple literals, capture blocks

### AST Nodes
Full AST node hierarchy in `src/ast/node.h` with typed nodes for every construct, including annotations and comments.

## Phase 3: Semantic Analysis — Complete

### Scope & Symbols
Lexical scoping with parent chains, two-pass design (declare globals, analyze bodies), duplicate detection, name resolution.

### Type Checking
Expression type inference, operator compatibility, assignment validation, pointer/reference validation, if/loop condition must be boolean, struct field access validation, error union handling.

### Error Reporting
Categorized errors (`[TypeError]`, `[NameError]`, `[MutError]`, `[ReturnError]`, `[DeclError]`, `[FlowError]`, `[ArgError]`, `[SyntaxError]`, `[FieldError]`) with color-coded output, line:column positions, and expected vs found type display via `typeToString()`.

## Phase 4: LLVM Codegen — Not Started

All items in this phase are pending:

- Module preamble (source_filename, target layout)
- Libc function declarations
- Std library IR injection
- Global node dispatch
- Type mapping to LLVM (all types)
- Variable/function/expression/statement codegen
- Struct/enum/union/error handling/behaviour codegen
- Generator/async codegen

## Phase 5: Standard Library — Not Started

All 24 std modules pending: `std.core`, `std.mem`, `std.str`, `std.string`, `std.fmt`, `std.io`, `std.fs`, `std.os`, `std.vec`, `std.map`, `std.set`, `std.ring`, `std.math`, `std.bits`, `std.ascii`, `std.unicode`, `std.parse`, `std.buf`, `std.hash`, `std.sync`, `std.time`, `std.testing`, `std.debug`.

## Phase 6: Critical Codegen Features — Not Started

P0-P3 items including zero hidden allocations, predictable LLVM mapping, no implicit casts, no hidden magic, zero-cost abstractions.

## Phase 7: Self-Hosting — Not Started

Bootstrap path: make the Razen compiler compile itself.

---

## Milestones

| Milestone | Description | Key Deliverables | Status |
|-----------|-------------|------------------|--------|
| M0 | C++ pipeline skeleton | Makefile, lexer, parser, semantic stubs | ✓ Complete |
| M1 | Working lexer | Full tokenization of all Razen constructs | ✓ Complete |
| M2 | Full parser + AST | All declarations, statements, expressions, generics, ranges, else-if, block try, ext struct | ✓ Complete |
| M3 | Semantic analysis | Type checking, scope, validation, categorized errors, typeToString, pointer/error union compatibility | ✓ Complete |
| M4 | Struct codegen | Struct types, field access, methods | ☐ Not started |
| M5 | Enum + Match | Enumerations compile, match dispatches | ☐ Not started |
| M6 | Error handling | Error unions, try/catch | ☐ Not started |
| M7 | Collections | Vec, Map, Set with generics | ☐ Not started |
| M8 | Std complete | All 24 std modules implemented | ☐ Not started |
| M9 | Self-hosting | Razen compiler compiles itself | ☐ Not started |

---

### Std Library: 0% complete
### Next Target: Phase 4 codegen completion and Phase 5 (Std Library)
