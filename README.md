# Razen

Razen is a systems language. No magic, no hidden costs, and no implicit behavior. It's built for people who want total control over the hardware.

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
- **C++20**: Required to build the compiler. A modern C++ compiler (GCC 12+, Clang 16+, MSVC 2022+).
- **CMake**: Version 3.20 or later.
- **LLVM**: LLVM 18+ development libraries (for code generation).

### Build
```bash
git clone https://github.com/razen-lang/razen.git
cd razen
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/razenc main.rz
```

### Update
```bash
git pull
cmake --build build
./build/razenc main.rz
```

## Docs & Progress

Everything is documented in the `/docs` folder. If you want to see what's left to build, check `ROADMAP.md`.

- [Introduction](./docs/introduction.md)
- [Basics](./docs/basics.md)
- [Types](./docs/types.md)
- [Roadmap](./ROADMAP.md)

## License
Apache 2.0

**Creator**: [Prathmesh Barot](https://github.com/prathmesh-barot)
