# Razen

Razen is a systems language. No magic, no hidden costs, no implicit behavior. Built for people who want total control over the hardware.

## What is Razen for?

Razen is designed for modern, performance-critical software:
- **AI/ML**: High-speed tensor ops and model infra.
- **Servers**: Low-latency, high-concurrency backends.
- **Apps**: Core logic that needs to be lean and fast.

**The goal: Meaningful, Accurate, Simple, Maximum Performance.**

## Quick Start

```razen
func main() -> void {
    fmt.println("Hello, Razen!")
}
```

## Build and Run

### Requirements
- **clang++-20**: Required to build the compiler (`-std=c++20`).
- **LLVM 20**: Development libraries for code generation.
- **Make**: Standard GNU Make.

### Build
```bash
git clone https://github.com/razen-lang/razen.git
cd razen
make
./razenc main.rzn
```

### Compiler Pipeline (6 Phases)
```
Source → Lexer → AST → Semantic Analysis → LLVM IR Codegen → Optimize → Emit (.o/.s)
```

### CLI
```bash
./razenc                    # compiles 10 built-in sample programs
./razenc -v                 # verbose debug output (all phases)
./razenc --version          # razenc 0.0.3 (LLVM 20, x86-64)
./razenc --help             # print usage
./razenc main.rzn           # compile a .rzn file
./razenc -f file1.rzn file2.rzn  # compile multiple files
```

Each `.rzn` file produces `output/<name>.o` and `output/<name>.s`.

### Update
```bash
git pull
make clean && make
```

## Current Status

| Phase | Component | Status |
|-------|-----------|--------|
| 1 | Lexer | ✓ Complete — full tokenization with line/col tracking |
| 2 | Parser & AST | ✓ Complete — all declarations, expressions, types |
| 3 | Semantic Analysis | ✓ Complete — type checking, scope, validation, categorized errors |
| 4 | LLVM IR Codegen | ≈85% Complete — types, variables, functions, if/loop/match/defer/try, structs, enums, unions, error handling, optimization (mem2reg + instcombine), object/assembly emission |
| 5 | Standard Library | ≈5% — `fmt.rzn` provides `print`/`println`/`printf`/`puts` |
| 6 | Self-hosting | ☐ Not started |

## Docs & Progress

- [Introduction](./docs/introduction.md)
- [Quickstart](./docs/quickstart.md)
- [Basics](./docs/basics.md)
- [Types](./docs/types.md)
- [Functions](./docs/functions.md)
- [Control Flow](./docs/control_flow.md)
- [Roadmap](./ROADMAP.md)

## License
Apache 2.0

**Creator**: [Prathmesh Barot](https://github.com/prathmesh-barot)
