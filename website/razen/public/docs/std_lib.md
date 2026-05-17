# Standard Library

The Razen standard library (`std`) is minimal. No heavy runtime. Only code you use reaches your binary.

## Module Listing

| Module | Description |
|--------|-------------|
| `std.core` | Always in scope. Compiler builtins, primitive types. Never imported. |
| `std.mem` | Allocators (page, arena, fixed, etc.), raw memory operations, `Layout` |
| `std.str` | `str` slice utilities (UTF-8, slicing, search) |
| `std.string` | Heap-allocated `string` type, `StringBuilder` |
| `std.fmt` | `print`, `println`, `eprint`, `format` |
| `std.io` | `Reader`/`Writer` behaviours, buffered I/O |
| `std.fs` | `File`, `Path`, `Dir` operations |
| `std.os` | `exit`, `clock`, `args`, `env`, process management |
| `std.vec` | `Vec(T)` growable array |
| `std.map` | `Map(K,V)` hash map |
| `std.set` | `Set(T)` hash set |
| `std.ring` | `Ring(T,N)` fixed-size ring buffer |
| `std.math` | Numeric operations, float functions, constants |
| `std.bits` | Bit manipulation utilities |
| `std.ascii` | ASCII character classification and conversion |
| `std.unicode` | UTF-8 encode/decode, `char` utilities |
| `std.parse` | String → number/bool parsing |
| `std.buf` | `ByteBuf`, little-endian/big-endian read/write |
| `std.hash` | FNV-1a, SipHash implementations |
| `std.sync` | Atomics, memory fence |
| `std.time` | `Duration`, `Instant`, monotonic clock |
| `std.testing` | Test assertions, test runner |
| `std.debug` | `assert`, `panic`, `unreachable`, stack trace |

## Status

All modules are designed in `design/std_new.md` but not yet implemented. Implementation is Phase 5.
