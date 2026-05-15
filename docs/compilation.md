# Compilation

## Compiler

The Razen compiler (`razenc`) is written in C++20 and built with `clang++`.

## Current Pipeline

```
Source → Lexer → Parser/AST → Semantic Analysis
```

- **Phase 1 — Lexer**: Tokenizes source into a flat token stream (`lexer/lexer.cpp`).
- **Phase 2 — Parser**: Builds AST via recursive descent + Pratt-style expression parser (`parser/parser.cpp`, `ast/expression.cpp`).
- **Phase 3 — Semantic**: Currently minimal partial type checking. Full scope resolution and type inference are planned as Phase 3 proper.

## Planned

- **Phase 4 — Codegen**: LLVM IR generation, then machine code through LLVM backend.

## Build & Run

```bash
make                # compiles razenc
./razenc            # currently runs hardcoded sample programs
```

The Makefile lives in the project root. No configure step is needed.

## Future CLI Flags

```
--emit=ir      # dump LLVM IR
--emit=obj     # produce object file
--emit=bin     # produce executable
-O0 .. -O3     # optimization level
--target=      # cross-compilation target triple
```

## Error Format

```
[Category] at line N:M message
```

Example:

```
[Syntax] at line 14:5 expected ';' after expression
```
