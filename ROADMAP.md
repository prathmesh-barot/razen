# Razen Compiler Roadmap (C++ Implementation)

**Philosophy:** Meaningful. Accurate. Simple. Maximum Performance. No Hidden Magic.
**Style:** Direct. No fluff. Every checkbox is a concrete deliverable.

---

## Legend

| Mark | Meaning |
|------|---------|
| ✓ | Done — tested and working in pipeline |
| ◐ | Partial — parsed/validated but codegen missing or broken |
| ☐ | Not started |

---

## Stage 0: Project Infrastructure

### Build System
- ✓ Makefile build system (clang++-20, C++20, `make && ./razenc`)
- ✓ Dependency on C++20 or later (`-std=c++20`)
- ✓ `razenc` CLI binary (separate from host build)
- ✓ Source file input (positional `.rzn` args + `-f` flag)
- ◐ Output file flags (`emitObject`/`emitAssembly` via IRGen — no `--emit=` CLI flag yet)
- ☐ Target triple specification for cross-compilation
- ☐ Optimization level flags (`-O0` through `-O3`)
- ☐ DWARF debug info generation (no `-g` CLI flag yet)

### Documentation
- ✓ README.md with philosophy, build, and quick start
- ✓ ROADMAP.md (this file)
- ✓ docs/ — introduction, basics, types, functions, control flow, behaviours, std_lib, compilation, syntax, style, faq, memory, modules, expressions, generics, attributes, error handling, testing
- ✓ design/ — keywords, std_new (detailed std spec), examples
- ☐ Language specification (formal grammar)
- ☐ Compiler internals guide

### Testing
- ✓ Sample programs in src/samples/ (6 sample headers with 30+ test programs)
- ☐ Automated test runner
- ☐ Unit tests for lexer, parser, semantic, codegen
- ☐ Integration tests (compile + verify LLVM IR output)
- ☐ Fuzz testing for parser and semantic analyzer
- ☐ Regression test suite for all open issues

---

## Stage 1: Lexer (Phase 1) ✓ Complete

### Token Types — All Tokens Defined and Lexed
- ✓ Keywords: `func`, `ret`, `if`, `else`, `loop`, `break`, `skip`, `match`, `const`, `mut`, `pub`, `use`, `mod`, `struct`, `enum`, `union`, `error`, `behave`, `ext`, `async`, `defer`, `try`, `catch`, `type`, `true`, `false`, `void`, `noret`, `any`, `test`, `needs`
- ✓ Primitive types: `i1`-`i128`, `u1`-`u128`, `isize`, `usize`, `int`, `uint`, `f16`-`f128`, `float`, `bool`, `char`, `str`, `string`
- ✓ Operators: `+`, `-`, `*`, `/`, `%`, `=`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `+=`, `-=`, `*=`, `/=`, `%=`, `!`, `&&`, `||`, `&`, `|`, `^`, `~`, `<<`, `>>`, `.`, `..`, `...`, `..=`, `->`, `=>`, `~>`, `:`, `:=`, `,`, `;`, `@`, `?`
- ✓ Delimiters: `(`, `)`, `{`, `}`, `[`, `]`
- ✓ Integer literals (decimal)
- ✓ Float literals
- ✓ String literals with escape sequences (`\n`, `\t`, `\"`, `\\`, `\'`, `\0`, `\r`, `\xNN`)
- ✓ Char literals with escape sequences
- ✓ Bool literals (`true`, `false`)
- ✓ Single-line comments (`//`) with correct line tracking
- ✓ Block comments (`/* */`) spanning multiple lines with correct line tracking
- ✓ Line/column tracking on every token
- ✓ EOF token

### Lexer Architecture
- ✓ Stateful Lexer class with position, line, char tracking
- ✓ Character-by-character processing loop
- ✓ Operator multi-character peek-ahead
- ✓ Dot operator differentiation (`.`, `..`, `...`, `..=`)
- ✓ Word/keyword/number disambiguation
- ✓ Identifier tokenization
- ✓ Unrecognized character handling (throws `LexerError` with context)

---

## Stage 2: Parser & AST (Phase 2) ✓ Complete

### AST Node Types (48 node types)
- ✓ Literal nodes: IntegerLiteral, FloatLiteral, StringLiteral, CharLiteral, BoolLiteral, ArrayLiteral, TupleLiteral, ArrayType
- ✓ Identifier node
- ✓ Type nodes: VarType, PointerType (*T), ArrayType ([T], [T;N]), OptionalType (?T), FailableType (!T), ErrorUnionType (Error!T)
- ✓ Declaration nodes: FunctionDeclaration, VarDeclaration, ConstDeclaration, StructDeclaration, EnumDeclaration, UnionDeclaration, ErrorMapDeclaration, TypeAliasDeclaration, ModuleDeclaration, UseDeclaration, BehaviourDeclaration, ExtDeclaration, Annotation, GenericParams
- ✓ Statement nodes: ReturnStatement, IfStatement, ElseIfStatement, LoopStatement, MatchStatement, TryStatement (TryExpression), CatchBlock (CatchExpression), DeferStatement, Assignment, Block, TryBlock
- ✓ BreakStatement / SkipStatement — dedicated AST nodes with correct loop-scope validation
- ✓ Expression nodes: BinaryExpression, UnaryExpression, FunctionCall, MemberAccess, BuiltinExpression, RangeExpression, CaptureBlock
- ✓ Structural nodes: ReturnType, IfBody, ElseBody, LoopBody, MatchBody, MatchCase, Parameters, Parameter, Argument
- ✓ Comment node

### Declaration Parsing
- ✓ `func name(params) -> ret_type { body }` — full function parsing
- ✓ `pub func` / `async func` / `const func` / `ext func` / `ext struct` / `ext enum` / `ext union` variants
- ✓ Generic parameters: `@Generic(T)`, `@Generic(T, E)` — parsed with GenericParams node, stored on declaration
- ✓ Parameter parsing with `mut`/`const` prefix, variadic `...`
- ✓ `struct Name { fields... }` with methods, `~>` trait impls, `~>` rename syntax, field defaults
- ✓ `enum Name: backing_type { variants... }` with explicit values, methods, `~>` (traits in children, consistent with struct/union)
- ✓ `union Name { variants... }` — tuple-style, struct-variant
- ✓ `error Name { variants... }` — error set declaration
- ✓ `behave Name { needs..., func... }` — behaviour/trait declaration, with `~>` inheritance
- ✓ `const Name: type = expr` — compile-time constants
- ✓ `type Name = Type` — type aliases
- ✓ `mod Name;` — module declarations
- ✓ `use dotted.path;` — import statements
- ✓ `pub` visibility flag on all declarations

### Statement Parsing
- ✓ Variable declarations: `name: type = expr`, `name := expr`, `mut` variant
- ✓ Assignment: `name = expr`, name `+=`/`-=`/`*=`/`/=`/`%=` expr
- ✓ `ret expr` / `ret` (void return)
- ✓ `if cond { ... } else { ... }` — including `else if` chaining
- ✓ `loop { ... }` — infinite loop
- ✓ `loop cond { ... }` — conditional loop
- ✓ `loop expr |item| { ... }` — iterator loop (parsed)
- ✓ `break`, `skip`
- ✓ `defer { ... }`, `defer stmt`
- ✓ `match expr { pat => body, ... }` with literal/enum/destructure/wildcard patterns
- ✓ `try expr`, `try expr catch |err| { ... }`, `try { ... } catch (err) { ... }` (block variant)
- ✓ `@as(Type, expr)` and other `@` builtins (parsed as BuiltinExpression)

### Expression Parsing
- ✓ Full precedence-climbing expression parser (12 precedence levels)
- ✓ All binary operators with correct associativity
- ✓ Unary: `-`, `!`, `~` (bitwise not), `&` (address-of), `*` (dereference via `ptr.*`)
- ✓ Pointer dereference: `ptr.*` (postfix)
- ✓ Member access: `a.b.c`
- ✓ Function calls: `f(args)` with argument lists
- ✓ Array literals: `[1, 2, 3]`
- ✓ Tuple literals: `.{a, b, c}`
- ✓ Range expressions: `..`, `..=`, `...` parsed as dedicated RangeExpression nodes with precedence 11
- ✓ Capture blocks: `|e| expr`
- ✓ Parenthesized grouping
- ✓ Type annotations in expression context

### Type Parsing
- ✓ All primitive types (i1-u128, f16-f128, bool, char, void, noret, any)
- ✓ Pointer types: `*T`
- ✓ Optional types: `?T`
- ✓ Failable types: `!T`
- ✓ Error union types: `Error!T` (named) and `error!T` (anonymous)
- ✓ Array types: `[T]`, `[T; N]`
- ✓ Collection types: `vec[T]`, `map{K,V}`, `set{T}`
- ◐ Builtin types: `@Self`, `@Type`, `@Generic(T)` — parsed as identifiers, no special validation
- ✓ `mut` type modifier

---

## Stage 3: Semantic Analysis (Phase 3) ✓ Complete

### Symbol Table & Scope Management
- ✓ Scope class with parent chain for lexical scoping
- ✓ Symbol types: Variable, Function, Struct, Enum, Union, Trait, ErrorSet, Module, TypeAlias
- ✓ PushScope / PopScope for block boundaries
- ✓ Two-pass design: pass 1 (declare globals) + pass 2 (analyze bodies)

### Name Resolution
- ✓ Global declaration registration (functions, structs, enums, unions, traits, behaviours, aliases, modules)
- ✓ Variable name resolution in expressions
- ✓ Function name resolution for calls
- ◐ Module-scoped name resolution (`mod` / `use` paths — basic path tracking, no multi-file linking)
- ✓ `std` identifier whitelisted
- ✓ Built-in identifier whitelist: `self`, `true`, `false`, `null`, `print`, `println`, `printf`, `puts`, `eprint`, `eprintln`, `exit`, `assert`, `panic`, `clock_ms`, `clock_ns`

### Declaration Validation
- ✓ Duplicate declaration detection in same scope
- ✓ Function parameter count validation on calls
- ✓ Function argument count validation
- ✓ Mutability checks (immutable assignment detection)
- ✓ Undeclared identifier detection
- ✓ Return type validation (shows expected vs actual types with `typeToString`)
- ✓ Function argument type matching
- ☐ Constant expression evaluation (comptime)
- ✓ Struct field declaration tracking
- ✓ Enum variant tracking

### Type Checking
- ✓ Expression type inference for `:=`
- ✓ Operator type compatibility (arithmetic, comparison, logical, bitwise) with rich error messages
- ✓ Assignment type compatibility with `typeToString` diff messages
- ✓ Pointer/reference type validation (address-of returns pointer type, dereference requires pointer)
- ✓ If condition must be boolean
- ✓ Loop condition must be boolean
- ✓ Break/skip outside loop detection
- ✓ Struct field access validation (field existence, returns field type)
- ✓ Error union handling (error_type extracted from named error sets, try/catch recognized)
- ☐ Array/slice index validation
- ☐ Behaviour implementation signature checking
- ☐ Comptime const expression validation

### Error Reporting
- ✓ Categorized errors: `[TypeError]`, `[NameError]`, `[MutError]`, `[ReturnError]`, `[DeclError]`, `[FlowError]`, `[ArgError]`, `[SyntaxError]`, `[FieldError]`
- ✓ Color-coded output with RED category tags and CYAN position info
- ✓ Line:column position on every error (`line N:M`)
- ✓ Expected vs found type display via `typeToString()` for pointer, optional, error union, struct, enum types

---

## Stage 4: LLVM IR Code Generation (Phase 4) ≈85%

### Phase 4 Architecture
- ✓ Module preamble: `source_filename`, target layout (`e-m:e-p270:...`), target triple (`x86_64-pc-linux-gnu`)
- ✓ Libc function declarations (`printf`, `puts`, `exit`, `abort`) with LLVM attributes
- ◐ Std library IR injection (`std.fmt` module injection from `src/std/fmt.rzn`)
- ✓ Global node dispatch (genNode switch on all node types)
- ✓ IRGen shared state (Codegen struct with locals, types, enums, unions, errors maps)
- ✓ StringBuilder for efficient IR assembly (IRBuilder with LLVM API)
- ✓ Comment/Annotation/GenericParams/Module/Use/Behave/TypeAlias nodes skipped in codegen
- ✓ Optimization pipeline (mem2reg + instcombine via new PM PassBuilder)
- ✓ Object/assembly emission (emitObject/emitAssembly via TargetMachine)
- ✓ CLI: `--verbose`/`--debug`, `--help`, `--version`, `-f`, positional file args

### Type Mapping to LLVM
- ✓ Primitive types: i1-u128, f16-f128, bool→i1, char→i8, void, str→ptr, string→ptr, any→ptr
- ✓ Pointer types: `*T` → `ptr` (opaque)
- ✓ Compound types: structs→`%T`, enums→`iN`, unions→`{i32,[N x i8]}`, error unions→`{i1,T}`, optionals→`{i1,T}`, failables→`{i1,T}`
- ✓ Array types: `[N x T]`, slice→`ptr`

### Variable Declarations
- ✓ Local alloca with store initializer
- ✓ Global const declarations with `InternalLinkage` and constant initializers
- ✓ Global non-const declarations with deferred init via `__raz_global_init()` constructor
- ✓ Type inference via `:=` with type mapping

### Function Code Generation
- ✓ `define @name` with parameter list
- ✓ Parameter alloca and store at entry block
- ✓ Return value handling with default zeroinit for void
- ✓ External function declarations (`ext func`) with variadic support
- ✓ Self parameter handling for method calls

### Expression Code Generation
- ✓ Literals: int (i32), float (double), bool (i1), char (i8), string (global `@.str.N` with dedup and PrivateLinkage)
- ✓ Identifier: load from alloca with type tracking, enum/error lookup, `null` → null constant
- ✓ Binary operators: arithmetic (Add/FAdd), comparison (ICmp/FCmp), logical (And/Or i1), bitwise (And/Or/Xor/Shl/LShr) — all with float/int dispatch and unsigned-aware SExt/ZExt/Trunc widening
- ✓ Unary operators: negate (Neg/FNeg), not (Xor 1), bitnot (Xor all-ones), address-of (`&`), dereference (`.*` via LoadInst)
- ✓ Function calls with argument type widening (SExt/Trunc)
- ✓ Member access: struct field GEP, enum variant constant, error set constant, method call with mangled name
- ◐ Method calls: `c.method()` → mangled name `struct.method` + self arg (basic support, no vtable dispatch)
- ✓ Array literal: `ConstantArray` or alloca + per-element GEP+store
- ✓ Tuple literal: `ConstantStruct` or alloca + stores
- ✓ Range expression: `{start, end}` struct pair
- ✓ Union construction: alloca, tag store, payload bitcast+store

### Statement Code Generation
- ✓ Variable declarations with initializer
- ✓ Assignment: `=`, `+=`, `-=`, `*=`, `/=`, `%=` (all with float/int dispatch)
- ✓ Assignment to struct field: `x.field = expr` via GEP chain
- ✓ Return statement with value (including unsigned-aware SExt/ZExt/Trunc for integer width mismatch)
- ✓ If/else with else-if chaining (CondBr based on i1 condition, end block joining)
- ✓ Loops: infinite (`loop {}`), conditional (`loop cond {}`), with cond/body/end basic blocks
- ✓ Break → `br` to `loop.end`, Skip → `br` to `loop.continue`
- ✓ Defer → LIFO flush before return (reverse iteration of deferred vector)
- ✓ Match → `icmp eq` chain for simple enums, tag dispatch for tagged unions with payload extraction and variable binding
- ✓ Try expression → flag check, branch to catch or propagate (return)
- ✓ Try block → scope with catch target

### Struct Code Generation
- ✓ Type definition: `%StructName = type { field_types }`
- ✓ Field access via `getelementptr` with field type tracking
- ✓ Struct field assignment via GEP chain
- ✓ Constructors with explicit field initialization
- ◐ Struct methods: collected and emitted with mangled names (`struct.method`)

### Enum Code Generation
- ✓ Type definition: `%EnumName = type backing_integer` (i32 default, custom backing)
- ✓ Variant values computed and tracked (explicit and implicit, auto-increment)
- ✓ Enum member access resolves to integer constant
- ✓ Match dispatch using `icmp eq` on backing integer type

### Union Code Generation
- ✓ Tagged union type: `%UnionName = type { i32, [N x i8] }`
- ✓ Max payload size computed from all variants
- ✓ Union construction: tag store + payload bitcast+store
- ✓ Match tag dispatch with payload extraction
- ✓ Payload variable binding in match arms

### Error Handling Code Generation
- ✓ Error set declaration with variant→code mapping (incrementing from 0)
- ✓ Error variant reference in expressions: `FileError.NotFound` → i32 code
- ✓ Error union return construction: `{i1 1, i32 code}` or `{i1 0, T value}`
- ✓ Try expression: `extractvalue` flag check, branch to catch or propagate
- ✓ Try block: scope with catch handler target
- ◐ Behaviour / Trait code generation (vtable type creation + instance generation + dispatch implemented, end-to-end untested)
- ☐ Generator / Async code generation

---

## Stage 5: Standard Library

### Current Std Architecture
- ◐ `std.fmt` module — `src/std/fmt.rzn` provides `print`, `println`, `printf`, `puts` wrappers (injected at compile time when `use std.fmt` is found)
- ◐ C++ source-level module injection via file read during AST building
- ☐ LLVM IR templates embedded in C++ string constants
- ☐ C++ wrapper files for std modules
- ☐ Std function name mapping

### All Modules (std.core, std.mem, std.str, std.string, std.fmt, std.io, std.fs, std.os, std.vec, std.map, std.set, std.ring, std.math, std.bits, std.ascii, std.unicode, std.parse, std.buf, std.hash, std.sync, std.time, std.testing, std.debug)
- ☐ All items in all modules (design complete in `design/std_new.md`, implementation not started)

---

## Stage 6: Critical Missing Features

### Codegen Gaps
- ✓ Short-circuit evaluation for `&&` / `||` (PHI-node based with correct basic block control flow)
- ☐ Block-level break/continue labels (break/skip only target innermost loop)
- ☐ Behaviour/trait vtable dispatch
- ☐ Comptime / metaprogramming
- ☐ String literal interpolation (`{}` in format strings)
- ☐ Enum match IR — basic block ordering issues in some cases
- ☐ Generic monomorphization (`@Generic(T)` annotations parsed but not specialized)

### Usability
- ☐ Automated test runner
- ☐ Package/module resolution across files
- ☐ Error recovery in parser (currently panics on first error)
- ☐ Language server protocol (LSP) support
- ☐ WASM target

---

## Stage 7: Compiler Self-Hosting

### Bootstrap Path (all ☐)
- ☐ Razen type system self-hosting
- ☐ Razen lexer written in Razen
- ☐ Razen parser written in Razen
- ☐ Razen → C/C++ transpilation bootstrapping
- ☐ Full self-hosting

---

## Milestone Summary

| Milestone | Description | Key Deliverables | Status |
|-----------|-------------|------------------|--------|
| M0 | C++ pipeline skeleton | Makefile, lexer, parser, semantic stubs | ✓ Complete |
| M1 | Working lexer | Full tokenization of all Razen constructs | ✓ Complete |
| M2 | Full parser + AST | All declarations, statements, expressions, generics, ranges, else-if, block try, ext struct | ✓ Complete |
| M3 | Semantic analysis | Type checking, scope, validation, categorized errors, typeToString, pointer/error union compatibility | ✓ Complete |
| M4 | LLVM codegen | All types, expressions, statements, control flow, optimization, emission | ◐ ≈90% — short-circuit ✓, traits ◐, comptime ☐ |
| M5 | Struct codegen | Struct types, field access, methods | ✓ Types+fields ✓, methods ◐ |
| M6 | Enum + Match + Union | Enumerations, tagged unions, match dispatch | ✓ Enum✓ Union✓ Match✓ |
| M7 | Error handling | Error sets, error unions, try/catch | ◐ Error sets + try expr done, union propagation basic |
| M8 | Codegen optimization | mem2reg, instcombine, object/assembly emission | ✓ Complete — new PM PassBuilder, TargetMachine |
| M9 | CLI & tooling | --help, --version, -v, -f, file input | ✓ Complete |
| M10 | Collections | Vec, Map, Set with generics | ☐ Not started |
| M11 | Std library | All 24 std modules implemented | ☐ Not started (fmt.rzn functional) |
| M12 | Self-hosting | Razen compiler compiles itself | ☐ Not started |

---

## Design Constraints

- ☐ Zero hidden allocations — all allocation takes explicit Allocator param
- ☐ Predictable LLVM mapping — clear path from source to IR
- ☐ No implicit casts — type conversions must be explicit
- ☐ No hidden magic — no GC, no implicit allocs, no hidden control flow
- ☐ Zero-cost abstractions — behaviours dispatch without overhead

---

**Progress:** Phases 1-3 (Lexer, Parser, Semantic Analysis) complete. Phase 4 (Codegen) ~90% complete — core infrastructure fully done, optimization pipeline (mem2reg + instcombine), object/assembly emission (`emitObject`/`emitAssembly`), CLI (`--help`, `--version`, `-v`, file args), all major control flow (if/loop/match/defer/try), compound types (struct/enum/union/error), all expression types. Short-circuit `&&`/`||` implemented with PHI-node based control flow. Unsigned-aware integer widening and comparisons. Trait vtable codegen implemented (type creation, instance generation, dispatch). Remaining codegen work: comptime evaluation, match IR block ordering edge cases, end-to-end trait dispatch verification.

**Std Library:** ~5% — `fmt.rzn` (print/println/printf/puts) is functional and injected on `use std.fmt`.

**All 6 test programs in `tests/`** produce valid `.o` and `.s` files in `output/`; compiled object files link and execute correctly.

**Next Target:** Fix factorial recursion optimization bug (returns 208 for fact(6)), complete comptime evaluation, enum match CFG fix, end-to-end trait dispatch verification, std library modules (std.os, std.mem).
