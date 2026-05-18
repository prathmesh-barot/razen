# Roadmap

For the full detailed roadmap, see [ROADMAP.md](../ROADMAP.md) in the project root.

## Implementation Status

| Phase | Component | Status |
|-------|-----------|--------|
| 1 | Lexer — all tokens, comments, escape sequences, line/col tracking | ✓ Complete |
| 2 | Parser & AST — all declarations, expressions, types, control flow | ✓ Complete |
| 3 | Semantic Analysis — type checking, scope, validation, categorized errors | ✓ Complete |
| 4 | LLVM IR Codegen — all major constructs, optimization, object/asm emission | ◐ ≈85% |
| 5 | Standard Library — 24 modules designed, `fmt.rzn` functional | ◐ ≈5% |
| 6 | Self-hosting | ☐ Not started |

## Milestones

| M# | Description | Status |
|----|-------------|--------|
| M0 | Project infrastructure (Makefile, CLI, docs) | ✓ Complete |
| M1 | Working lexer | ✓ Complete |
| M2 | Full parser + AST | ✓ Complete |
| M3 | Semantic analysis | ✓ Complete |
| M4 | LLVM codegen (types, expressions, control flow, optimization, emit) | ◐ ≈85% |
| M5 | Struct codegen (types, fields, methods) | ✓ Fields ✓, Methods ◐ |
| M6 | Enum + Match + Union | ✓ Complete |
| M7 | Error handling (sets, unions, try/catch) | ◐ Partial |
| M8 | Codegen optimization + emission | ✓ Complete |
| M9 | CLI & tooling | ✓ Complete |
| M10 | Std library implementation | ☐ Not started |
| M11 | Self-hosting | ☐ Not started |

See the full [ROADMAP.md](../ROADMAP.md) for the complete per-item status with 200+ tracked deliverables.
