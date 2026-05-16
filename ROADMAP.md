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
- ◐ Makefile build system (clang++-20, C++20, `make && ./razenc`) — uses Makefile, not CMake
- ✓ Dependency on C++20 or later (`-std=c++20`)
- ✓ `razenc` CLI binary (separate from host build)
- ✓ Source file input (positional `.rzn` args + `-f` flag)
- ◐ Output file flags (`--emit=obj`/`--emit=asm` via `emitObject`/`emitAssembly` — no `--emit=` CLI flag yet)
- ☐ Target triple specification for cross-compilation
- ☐ Optimization level flags (-O0 through -O3)
- ◐ DWARF debug info generation (`-g` flag present, no structured debug info pipeline)

### Documentation
- ✓ README.md with philosophy and quick start
- ✓ ROADMAP.md (this file)
- ✓ docs/ — introduction, basics, types, functions, control flow, behaviours, std_lib
- ✓ design/ — keywords, std_new (detailed std spec)
- ☐ Language specification (formal grammar)
- ☐ Compiler internals guide

### Testing
- ✓ Sample programs in src/samples/ (6 sample headers with 30+ test programs)
- ☐ Automated test runner (`ctest` via CMake)
- ☐ Unit tests for lexer, parser, semantic, codegen
- ☐ Integration tests (compile + verify LLVM IR output)
- ☐ Fuzz testing for parser and semantic analyzer
- ☐ Regression test suite for all open issues

---

## Stage 1: Lexer (Phase 1)

### Token Types — All Tokens Defined and Lexed
- ✓ Keywords: `func`, `ret`, `if`, `else`, `loop`, `break`, `skip`, `match`, `const`, `mut`, `pub`, `use`, `mod`, `struct`, `enum`, `union`, `error`, `behave`, `ext`, `async`, `defer`, `try`, `catch`, `type`, `true`, `false`, `void`, `noret`, `any`
- ✓ Primitive types: `i1`-`i128`, `u1`-`u128`, `isize`, `usize`, `int`, `uint`, `f16`-`f128`, `float`, `bool`, `char`, `str`, `string`
- ✓ Operators: `+`, `-`, `*`, `/`, `%`, `=`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `+=`, `-=`, `*=`, `/=`, `%=`, `!`, `&&`, `||`, `&`, `|`, `^`, `~`, `<<`, `>>`, `.`, `..`, `...`, `..=`, `->`, `=>`, `~>`, `:`, `:=`, `,`, `;`, `@`
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

## Stage 2: Parser & AST (Phase 2)

### AST Node Types
- ✓ Literal nodes: IntegerLiteral, FloatLiteral, StringLiteral, CharLiteral, BoolLiteral
- ✓ Identifier node
- ✓ Type nodes: VarType, PointerType (*T), ArrayType ([T], [T;N]), OptionalType (?T), FailableType (!T), ErrorUnionType (Error!T)
- ✓ Declaration nodes: FunctionDeclaration, VarDeclaration, ConstDeclaration, StructDeclaration, EnumDeclaration, UnionDeclaration, ErrorMapDeclaration, TypeAliasDeclaration, ModuleDeclaration, UseDeclaration, BehaviourDeclaration, ExtDeclaration
- ✓ Statement nodes: ReturnStatement, IfStatement, LoopStatement, MatchStatement, TryStatement (TryExpression), CatchBlock (CatchExpression), DeferStatement, Assignment, Block
- ✓ BreakStatement / SkipStatement — dedicated AST nodes with correct loop-scope validation
- ✓ Expression nodes: BinaryExpression, UnaryExpression, FunctionCall, MemberAccess, ArrayLiteral, Argument, Parameters, Parameter
- ✓ Structural nodes: ReturnType, IfBody, ElseBody, LoopBody, MatchBody
- ✓ Annotation node (for `@` attributes)
- ✓ Comment node (node type defined)

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
- ✓ `type Name = Type` — type aliases (full type parsing via parseTypeNodeWrapper)
- ✓ `mod Name;` — module declarations
- ✓ `use dotted.path;` — import statements
- ✓ `pub` visibility flag on all declarations (func, struct, enum, union, type, behave, const, mod, use)

### Statement Parsing
- ✓ Variable declarations: `name: type = expr`, `name := expr`, `mut` variant
- ✓ Assignment: `name = expr`, name `+=`/`-=`/`*=`/`/=`/`%=` expr
- ✓ `ret expr` / `ret` (void return)
- ✓ `if cond { ... } else { ... }` — including `else if` chaining (ElseIfStatement node)
- ✓ `loop { ... }` — infinite loop
- ✓ `loop cond { ... }` — conditional loop (parsed)
- ✓ `loop expr |item| { ... }` — iterator loop (parsed)
- ✓ `break`, `skip`
- ✓ `defer { ... }`, `defer stmt`
- ✓ `match expr { pat => body, ... }` with literal/enum/destructure/wildcard patterns
- ✓ `try expr`, `try expr catch |err| { ... }`, `try { ... } catch (err) { ... }` (block variant)
- ✓ `@as(Type, expr)` and other `@` builtins (parsed as annotation + function call)

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

### AST Builder Architecture
- ✓ ASTData cursor with token navigation (getToken, peekToken, advance)
- ✓ ASTNode allocation with left/middle/right/children tree structure
- ✓ Child list management
- ✓ Error reporting with context (error_detail, error_token, error_function)
- ✓ Recursive descent through all constructs

---

## Stage 3: Semantic Analysis (Phase 3)

### Symbol Table & Scope Management
- ✓ Scope class with parent chain for lexical scoping
- ✓ Symbol types: Variable, Function, Struct, Enum, Union, Trait, ErrorSet, Module, TypeAlias
- ✓ PushScope / PopScope for block boundaries
- ✓ Function, loop, if, else, match body scope management
- ✓ Symbol definition with duplicate detection
- ✓ Symbol resolution with parent scope walk
- ✓ Two-pass design: pass 1 (declare globals) + pass 2 (analyze bodies)

### Name Resolution
- ✓ Global declaration registration (functions, structs, enums, unions, traits, behaviours, aliases, modules)
- ✓ Variable name resolution in expressions
- ✓ Function name resolution for calls
- ◐ Module-scoped name resolution (mod / use paths — basic path tracking, no multi-file linking)
- ✓ `std` identifier whitelisted
- ✓ `self`/`true`/`false` whitelisted as built-in identifiers

### Declaration Validation
- ✓ Duplicate declaration detection in same scope
- ✓ Function parameter count validation on calls
- ✓ Function argument count validation
- ✓ Mutability checks (immutable assignment detection)
- ✓ Undeclared identifier detection
- ✓ Return type validation (checks if non-void function returns a value, shows expected vs actual types with `typeToString`)
- ✓ Function argument type matching (argument types analyzed, compared against parameter types with compatibility checks)
- ☐ Constant expression evaluation (comptime)
- ✓ Struct field declaration tracking
- ✓ Enum variant tracking
- ✓ Global duplicate detection

### Type Checking
- ✓ Expression type inference for `:=`
- ✓ Operator type compatibility (arithmetic, comparison, logical, bitwise) with rich error messages showing actual types
- ✓ Assignment type compatibility with `typeToString` diff messages
- ✓ Pointer/reference type validation (address-of returns pointer type, dereference requires pointer)
- ✓ If condition must be boolean — shows found type
- ✓ Loop condition must be boolean — shows found type
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
- ✓ Compat helpers explain WHY types don't match

---

## Stage 4: LLVM IR Code Generation (Phase 4)

### Phase 4 Architecture
- ✓ Module preamble: `source_filename`, target layout, target triple
- ✓ Libc function declarations (printf, puts, exit, abort)
- ☐ Std library IR injection (fmt, os, debug)
- ✓ Global node dispatch (genNode switch on all 30+ node types)
- ✓ ConvertData shared state (Codegen struct with locals, types, maps)
- ✓ StringBuilder for efficient IR assembly (IRBuilder with ostringstream)
- ✓ Comment nodes skipped in codegen
- ✓ Optimization pipeline (mem2reg + instcombine via new PM PassBuilder)
- ✓ Object/assembly emission (emitObject/emitAssembly via TargetMachine)
- ✓ CLI: --verbose/--debug, --help, --version, -f, positional file args

### Type Mapping to LLVM
- ✓ Primitive types: i1-u128, f16-f128, bool→i1, char→i8, void, str→i32, string→i32, any→i8*
- ✓ Pointer types: *T→ptr, *T (deref via ptr.*)
- ✓ Compound types: structs→%T, enums→iN, unions→{i32,[N x i8]}, error unions→{i1,T}, optionals→{i1,T}, failables→{i1,T}
- ✓ Array types: [N x T], slice→ptr

### Variable Declarations
- ✓ Local alloca with store initializer
- ✓ Global const declarations
- ✓ Type inference via `:=` with type mapping

### Function Code Generation
- ✓ define @name with parameter list
- ✓ Parameter alloca and store
- ✓ Entry block with proper terminator
- ✓ Return value handling with default zeroinit
- ✓ External function declarations (ext func)

### Expression Code Generation
- ✓ Literals: int (i32), float (double), bool (i1), char (i8), string (global @.str.N)
- ✓ Identifier: load from alloca with type tracking
- ✓ Binary operators: arithmetic, comparison, logical, bitwise (all with float/int dispatch)
- ✓ Unary operators: negate, not, bitnot, address-of (&), dereference (.*)
- ✓ Function calls with argument passing and known-function return types
- ✓ Member access: struct field GEP with field type lookup
- ✓ Enum variant access: Color.Red → i32 0 (with backing type)
- ✓ Union construction: Value.Int(42) → alloca, tag store, payload bitcast+store
- ◐ Method calls: c.increment() → mangled name + self arg (basic support)
- ✓ Array literal: temp alloca, per-element GEP+store, load whole array
- ✓ Tuple literal: anonymous struct, per-field GEP+store
- ✓ Range expression: {start, end} struct pair

### Statement Code Generation
- ✓ Variable declarations with initializer
- ✓ Assignment: =, +=, -=, *=, /=, %= (all with float/int dispatch)
- ✓ Assignment to member access: x.field = expr
- ✓ Return statement with value (including error union construction)
- ✓ If/else with else-if chaining (br based on i1 condition)
- ✓ Loops: infinite (loop {}), conditional (loop cond {}), basic blocks
- ✓ Break → br to loop.end, Skip → br to loop.continue
- ✓ Defer → LIFO flush before return
- ✓ Match → icmp eq chain for simple enums, tag dispatch for unions
- ✓ Try expression → flag check, branch to catch or propagate
- ✓ Try block → scope with catch target

### Struct Code Generation
- ✓ Type definition: %StructName = type { field_types }
- ✓ Field access via getelementptr
- ✓ Field type tracking for correct load/store
- ✓ Constructors with explicit field initialization
- ◐ Struct methods: collected and emitted with mangled names

### Enum Code Generation
- ✓ Type definition: %EnumName = type backing_integer (i32 default, custom backing)
- ✓ Variant values computed and tracked (explicit and implicit)
- ✓ Enum member access resolves to integer constant
- ✓ Match dispatch using icmp eq on backing integer type

### Union Code Generation
- ✓ Tagged union type: %UnionName = type { i32, [N x i8] }
- ✓ Max payload size computed from all variants
- ✓ Union construction: tag store + payload bitcast+store
- ✓ Match tag dispatch with payload extraction
- ✓ Payload variable binding in match arms

### Error Handling Code Generation
- ✓ Error set declaration with variant→code mapping
- ✓ Error variant reference in expressions: FileError.NotFound → i32 code
- ✓ Error union return construction: {i1 1, i32 code} or {i1 0, T value}
- ✓ Try expression: extractvalue flag check, branch to catch or propagate
- ✓ Try block: scope with catch handler target
- ☐ Behaviour / Trait Code Generation
- ☐ Generator / Async Code Generation

---

## Stage 5: Standard Library

### Current Std Architecture
- ☐ LLVM IR templates embedded in C++ string constants
- ☐ C++ wrapper files for std modules
- ☐ Std library source files
- ☐ Std function name mapping

### All Modules (std.core, std.mem, std.str, std.string, std.fmt, std.io, std.fs, std.os, std.vec, std.map, std.set, std.ring, std.math, std.bits, std.ascii, std.unicode, std.parse, std.buf, std.hash, std.sync, std.time, std.testing, std.debug)
- ☐ All items in all modules

---

## Stage 6: Critical Missing Codegen Features

### P0 — Must Fix (all ☐)
### P1 — High Priority (all ☐)
### P2 — Medium Priority (all ☐)
### P3 — Lower Priority (all ☐)

---

## Stage 7: Compiler Self-Hosting

### Bootstrap Path (all ☐)

---

## Priority Pipeline

### P0 — Blocking Everything Else (Codegen) — all ☐
### P1 — Core Usability — all ☐
### P2 — Std Library Enablement — all ☐
### P3 — Language Completeness — all ☐

---

## Milestone Summary

| Milestone | Description | Key Deliverables | Status |
|-----------|-------------|------------------|--------|
| M0 | C++ pipeline skeleton | Makefile, lexer, parser, semantic stubs | ✓ Complete |
| M1 | Working lexer | Full tokenization of all Razen constructs | ✓ Complete |
| M2 | Full parser + AST | All declarations, statements, expressions, generics, ranges, else-if, block try, ext struct | ✓ Complete |
| M3 | Semantic analysis | Type checking, scope, validation, categorized errors, typeToString, pointer/error union compatibility | ✓ Complete (improved) |
| M4 | Struct codegen | Struct types, field access, methods | ◐ Partial — types + fields done, methods basic |
| M5 | Enum + Match | Enumerations compile, match dispatches | ✓ Complete — backing types, variant values, icmp dispatch |
| M6 | Error handling | Error unions, try/catch | ◐ Partial — error sets, try expr branching done, union propagation basic |
| M7 | Codegen optimization | mem2reg, instcombine, object emission | ✓ Complete — new PM pipeline, TargetMachine |
| M8 | CLI & tooling | --help, --version, -v, file input, quiet mode | ✓ Complete |
| M9 | Collections | Vec, Map, Set with generics | ☐ Not started |
| M10 | Std complete | All 24 std modules implemented | ☐ Not started |
| M11 | Self-hosting | Razen compiler compiles itself | ☐ Not started |

---

## Design Constraints

- ☐ Zero hidden allocations — all allocation takes explicit Allocator param
- ☐ Predictable LLVM mapping — clear path from source to IR
- ☐ No implicit casts — type conversions must be explicit
- ☐ No hidden magic — no GC, no implicit allocs, no hidden control flow
- ☐ Zero-cost abstractions — behaviours dispatch without overhead

---

**Progress:** Phases 1-3 (Lexer, Parser, Semantic Analysis) complete. Phase 4 (Codegen) ~85% complete — core infrastructure (PLAN Phases 1-16) fully done, optimization pipeline (mem2reg + instcombine), object/assembly emission (emitObject/emitAssembly), and CLI (--help, --version, -v, file args) all done. Remaining codegen work: struct methods, string dedup, binary SExt→ZExt, global var non-constant init. Phases 5-7 (Std Lib, Self-hosting) not started.
**Std Library:** 0% complete.
**All 10 sample programs** produce valid `.o` and `.s` files; compiled `.o` files link and execute correctly.
**Next Target:** Remaining codegen bugs (#16 string dedup, #22 ZExt coercion, #23 global init) + struct method codegen.
