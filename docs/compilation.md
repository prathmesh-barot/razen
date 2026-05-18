# Compilation

## Compiler

The Razen compiler (`razenc`) is written in C++20 and built with `clang++-20` and LLVM 20.

## Current Pipeline

```
Source → Lexer → AST → Semantic Analysis → LLVM IR Codegen → Optimize → Emit (.o/.s)
```

Six phases, all implemented:

- **Phase 1 — Lexer**: Tokenizes source into a flat token stream (`lexer/lexer.cpp`). Full line/column tracking, escape sequences, all operators, keywords, and literals.
- **Phase 2 — Parser**: Builds AST via recursive descent + Pratt-style expression parser (`parser/parser.cpp`, `ast/expression.cpp`). All declarations, statements, expressions, and types.
- **Phase 3 — Semantic**: Full type checking, scope resolution, validation with categorized errors. Two-pass design (global declaration pass + body analysis pass).
- **Phase 4 — Codegen**: LLVM IR generation for all constructs: expressions, statements, control flow, compound types, error handling.
- **Phase 5 — Optimization**: LLVM `mem2reg` (SSA construction) + `InstCombine` via new PM PassBuilder pipeline.
- **Phase 6 — Emit**: Object file (`.o`) and assembly (`.s`) emission via LLVM `TargetMachine`.

## Build & Run

```bash
make                # compiles razenc
./razenc            # compiles 10 built-in sample programs
./razenc main.rzn   # compiles a .rzn file → output/main.o and output/main.s
./razenc -f a.rzn b.rzn  # compile multiple files
```

The Makefile lives in the project root. No configure step is needed.

## CLI Flags

```
Usage: ./razenc [flags] [files...]

Flags:
  -h, --help       Print help
  --version        Print version
  -v, --verbose    Full phase-by-phase debug output
  --debug          Alias for --verbose
  -f, --files      Treat remaining args as files

With no files, compiles 10 built-in samples.
Otherwise compiles each .rzn file, emits output/<name>.o/.s
```

## Error Format

```
[Category] at line N:M message
```

Example:

```
[Syntax] at line 14:5 expected ';' after expression
```

Error categories: `[TypeError]`, `[NameError]`, `[MutError]`, `[ReturnError]`, `[DeclError]`, `[FlowError]`, `[ArgError]`, `[SyntaxError]`, `[FieldError]`. Color-coded output in verbose mode.

## Planned CLI Additions

```
--emit=ir      # dump LLVM IR to stdout
-O0 .. -O3     # optimization level flag
--target=      # cross-compilation target triple
```
