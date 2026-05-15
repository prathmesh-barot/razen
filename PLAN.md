# LLVM IR Codegen — 20-Phase Plan

## Phase 1-5: Infrastructure & Type Mapping
1. **Project setup** — codegen/ directory, Makefile update, IR builder class skeleton
2. **IR Builder** — StringBuilder-based IR assembly, label/tmp counters, indentation, output
3. **Module preamble** — target triple, datalayout, source_filename, libc declarations
4. **Primitive type mapping** — i1-u128, f16-f128, bool→i1, void, str→i32 stub
5. **Compound type mapping** — *T→ptr, structs→%T, enums→iN, unions→tagged, error unions, arrays

## Phase 6-10: Core Codegen
6. **Global dispatch** — ConvertData, walk AST, dispatch per node type
7. **Variable declarations** — alloca, store initializer, global const
8. **Function codegen** — define @name, params, entry block, ret
9. **Literal codegen** — int, float, bool, char, string (global @.str.N)
10. **Identifier + binary ops** — load from alloca, arithmetic, comparison

## Phase 11-15: Expressions & Control Flow
11. **Logical + bitwise + unary ops** — && || ! ~ & ptr.*
12. **Member access + function calls** — GEP, call, arg passing
13. **Assignment + compound** — store, load+op+store
14. **If/else + loop** — br, basic blocks, back-edge
15. **break/skip/defer** — exit/continue labels, LIFO flush

## Phase 16-20: Advanced Features
16. **Match statement** — switch/icmp chain
17. **Struct codegen** — type, GEP, construction, methods
18. **Enum codegen** — discriminant, switch dispatch
19. **Union codegen** — tag+payload, tag read, construction
20. **Error handling** — try/catch, error union, propagation
