# Codegen Real Status — Razen-CPP

**Last updated:** 2026-05-18  
**Codegen version:** 0.0.3  
**Based on:** Direct code audit of `src/codegen/`, `src/semantic/`, and test results

---

## Overall: ~70-75% functional

The pipeline (lexer → parser → semantic → LLVM IR → optimize → `.o`/`.s`) works end-to-end for a **C-like subset**. The documentation and roadmap overstate completeness where traits, generic handling, and several expression types are concerned.

---

## Legend

| Mark | Meaning |
|------|---------|
| ✓ | Working — tested, output correct |
| ◐ | Partial — code exists but has known bugs or is untested |
| ⚠️ | Buggy — has known incorrect behavior |
| ☐ | Not implemented — no code or only dead scaffolding |

---

## Phase 4: LLVM IR Codegen

### Pipeline Architecture

| Component | Status | Details |
|-----------|--------|---------|
| Module preamble (triple, datalayout) | ✓ | `x86_64-pc-linux-gnu` hardcoded |
| Libc declarations (printf, puts, exit, abort) | ✓ | With LLVM attributes (readonly, nothrow) |
| Multi-pass dispatch (types → ext → func+globals) | ✓ | 3 passes over AST nodes |
| Comment/Annotation/GenericParams skipped | ✓ | Intentional no-op |
| Optimization (mem2reg + instcombine) | ✓ | New PM PassBuilder |
| Object file emission | ✓ | TargetMachine + legacy PM |
| Assembly file emission | ✓ | |
| `--verbose` debug output | ✓ | Per-phase + IR dump |

### Type Mapping → LLVM

| Razen Type | LLVM Repr | Status | Notes |
|------------|-----------|--------|-------|
| `i1`–`i128` | `iN` | ✓ | |
| `u1`–`u128` | `iN` | ✓ | |
| `isize`/`usize` | `i64` | ✓ | |
| `int`/`uint` | `i32` | ✓ | |
| `f16`–`f128` | `half`–`fp128` | ✓ | |
| `float` | `float` | ✓ | |
| `bool` | `i1` | ✓ | |
| `char` | `i8` | ✓ | |
| `void` | `void` | ✓ | |
| `noret` | `void` | ☐ | Not distinguished from void |
| `any` | `ptr` | ✓ | |
| `str`/`string` | `ptr` | ✓ | |
| `*T` | `ptr` (opaque) | ✓ | |
| `?T` optional | `{i1, T}` | ✓ | |
| `!T` failable | `{i1, T}` | ✓ | |
| `Error!T` error union | `{i1, T}` | ✓ | |
| `[T]` array | `ArrayType` or `ptr` | ✓ | Sized → array, unsized → ptr |
| `vec[T]`/`map{}`/`set{}` | `ptr` | ◐ | Parsed as type, no special handling |
| `Self` in methods | `*StructType` | ◐ | `current_struct` never assigned — works by accident |

### Literal Codegen

| Literal | Status | Notes |
|---------|--------|-------|
| Integer `42` → `i32` | ✓ | |
| Float `3.14` → `f64` | ✓ | Always double |
| Bool `true`/`false` → `i1` | ✓ | |
| Char `'x'` → `i8` | ✓ | |
| String `"hello"` → global `@.str.N` | ✓ | Dedup, PrivateLinkage, GEP for ptr |
| Array `[1,2,3]` | ⚠️ | Non-constant elements silently discarded |
| Tuple `.{42, "h"}` | ⚠️ | Non-constant elements silently discarded |
| Range `a..b` | ⚠️ | Non-constant values silently discarded |

### Expression Codegen

| Expression | Status | Notes |
|------------|--------|-------|
| Identifier (var load) | ✓ | |
| Identifier (enum value) | ✓ | |
| Identifier (`null`) | ✓ | Null constant |
| Identifier (function ref) | ✓ | Function pointer |
| Binary `+` `-` `*` `/` | ✓ | Float/int dispatch |
| Binary `%` | ⚠️ | Always SRem — wrong for unsigned |
| Binary `==` `!=` `<` `>` `<=` `>=` | ✓ | Signed/unsigned ICmp aware |
| Binary `&` `\|` `^` | ✓ | |
| Binary `<<` | ✓ | Shl |
| Binary `>>` | ⚠️ | Always LShr — wrong for signed |
| Binary `&&` `\|\|` (short-circuit) | ✓ | PHI-node based |
| Unary `-` (negate) | ✓ | FNeg or Neg |
| Unary `!` (not) | ⚠️ | XOR with 1 — wrong for non-i1 types |
| Unary `~` (bitwise not) | ✓ | XOR all-ones |
| Unary `&` (address-of) | ✓ | Returns alloca ptr |
| Unary `.*` (deref) | ✓ | LoadInst |
| Function call `f(args)` | ✓ | With integer widening |
| Struct field `s.field` | ✓ | GEP + load |
| Struct method `s.method(args)` | ◐ | Mangling mismatch works by accident for basic cases |
| Enum access `Color.Red` | ✓ | Returns i32 constant |
| Error access `Err.NotFound` | ✓ | Returns i32 constant |
| Union constructor `Value.Int(42)` | ✓ | Tag + payload |
| `@as(Type, expr)` | ☐ | Silently returns `i32 0` |
| `@SizeOf`/`@AlignOf`/`@TypeOf` | ☐ | Not implemented |
| Capture block `\|e\| expr` | ☐ | Silently returns `i32 0` |

### Statement Codegen

| Statement | Status | Notes |
|-----------|--------|-------|
| `ret expr` | ✓ | With unsigned-aware widening |
| `ret` (void) | ✓ | |
| `x = expr` | ✓ | |
| `x += expr` etc. | ✓ | All 5 compound forms |
| `s.field = expr` | ◐ | Fragile struct type lookup |
| `if cond { } else { }` | ✓ | CondBr with else-if recursion |
| `loop {}` | ✓ | |
| `loop cond {}` | ✓ | |
| `break` | ✓ | Br to loop_end |
| `skip` | ✓ | Br to loop_continue |
| `defer { }` | ✓ | LIFO flush before return |
| `match expr { pat => body }` | ◐ | Works, but CFG edge cases buggy |
| `match union { V(v) => ... }` | ✓ | Tag dispatch + payload extraction |
| `try expr` | ✓ | Extractvalue + cond branch |
| `try expr catch` | ✓ | Handler block |
| `try { } catch (e) { }` | ◐ | Untested |

### Compound Type Codegen

| Type | Status | Details |
|------|--------|---------|
| Struct `%T = type { ... }` | ✓ | Field names tracked |
| Struct methods collected | ◐ | Collected but mangling broken |
| Enum backing integer (default i32) | ✓ | |
| Enum auto-increment values | ✓ | |
| Enum explicit values | ✓ | |
| Tagged union `{i32, [N x i8]}` | ✓ | Max payload computed |
| Union payload extraction in match | ✓ | |
| Error set → integer codes | ✓ | |

### Trait / Behaviour Codegen

| Feature | Status | Details |
|---------|--------|---------|
| Behaviour declaration → VTableInfo | ✓ | Method signatures + fn types |
| VTable struct type creation | ✓ | Struct of function ptrs |
| VTable instance for struct impl | ❌ | **Null function pointers** — `genFunc` uses bare names, genTraitVTable looks for mangled names |
| Trait method call dispatch | ❌ | Loads null function ptr → segfault |
| `@Dyn` dynamic dispatch | ☐ | Not implemented |

### Dead Code in Codegen

| Function | File:Line | Never called |
|----------|-----------|--------------|
| `razenType(const TypeInfo*)` | ir.cpp:102 | Codegen ignores semantic analysis types |
| `loadVariable()` | ir.cpp:182 | |
| `storeVariable()` | ir.cpp:188 | |
| `createGEP()` | ir.cpp:196 | |
| `createStructGEP()` | ir.cpp:200 | |
| `setPointeeType()` | ir.cpp:207 | |
| `current_struct` field | ir.h:87 | Declared, never assigned (always `""`) |

---

## Phase 3: Semantic Analysis

| Feature | Status | Details |
|---------|--------|---------|
| Two-pass (declare + analyze) | ◐ | No topological ordering; forward refs may fail |
| Function declaration registration | ✓ | |
| Struct field registration | ✓ | |
| Enum variant registration | ✓ | |
| Variable type checking | ✓ | |
| Immutability check | ✓ | |
| Redeclaration detection | ✓ | |
| Return type compatibility | ✓ | |
| If condition must be boolean | ✓ | |
| Loop condition must be boolean | ✓ | |
| Break/skip outside loop | ✓ | |
| Struct field existence | ✓ | |
| Function argument COUNT checking | ✓ | |
| Function argument TYPE checking | ❌ | Commented out (line 953-954) |
| Global variable registration | ❌ | declareGlobal() skips VarDeclaration |
| Match exhaustiveness | ☐ | Not checked |
| Behaviour impl signature check | ☐ | Not checked |
| `SemanticError` exception thrown | ☐ | Class exists, never thrown |
| `SymbolTable` passed by value to Analyzer | ❌ | Changes lost to caller |

---

## Phase 2: Parser + AST ✓ (95% complete)

All 48 `ASTNodeType` values are parsed. See `src/ast/node.h:10-90` for full list.

Constructs parsed but silently ignored by codegen:
- `@generic(Type)` — `Annotation` node skipped
- `@as(Type, expr)` — `BuiltinExpression` → returns `i32 0`
- `CaptureBlock` (`|e| expr`) → returns `i32 0`
- `TypeAliasDeclaration` → skipped in genTopLevel

---

## Test Coverage Reality

| Category | Coverage |
|----------|----------|
| Tests that compile | 10 samples (sample6.h) + 6 tests/.rzn files |
| Features actually tested | arithmetic, comparisons, short-circuit `&&`/`||`, strings, basic struct, basic enum, basic union, if/else, loop/break, recursion, compound assignment, `match` |
| Features **not** tested | `defer`, `try`/`catch`, pointers (`*`/`&`/`.*`), block try, `skip`, range, range loops, `else if`, tuple literal, array literal, `@` annotations, generics, behaviours/traits, `error` sets, `ext struct`, `const func`, `async func`, `mod`/`use`, capture blocks, `pub` visibility, enum backing types, char/string escape sequences, multi-file compilation |
| Negative testing | 10 semantic error cases defined in `semantic_errors.h` but **never compiled** to verify rejection |
| Test infrastructure | `tests/run_tests.sh` — shell script, 6 tests, pass/fail only on crash |

---

## Bug Inventory

### Critical Bugs (incorrect output or crash)

| # | Bug | File:Line | Impact |
|---|-----|-----------|--------|
| 1 | `!x` for wide ints: XOR with 1 flips only LSB | codegen.cpp:1258 | `!2` → `3` (truthy) instead of `0` |
| 2 | `>>` always uses LShr (logical) | codegen.cpp:1238 | Signed right shift is wrong |
| 3 | `%` always SRem | codegen.cpp:1210 | Unsigned remainder is wrong |
| 4 | Trait vtables have null function pointers | codegen.cpp:342-389 | `genTraitMethodCall` segfaults |
| 5 | Array literal runtime elements discarded | codegen.cpp:1496-1502 | `genExpr` values thrown away |
| 6 | `current_struct` never assigned | codegen.cpp:169, ir.h:87 | Method dispatch works by accident |
| 7 | `@as(Type, val)` → `i32 0` | codegen.cpp:1049 | Type casts silently broken |
| 8 | Semantic arg TYPE checking commented out | analyzer.cpp:953-954 | Type mismatches in calls not caught |
| 9 | `SymbolTable` passed by value to Analyzer | analyzer.h:17-18 | All symbol table changes lost |
| 10 | Global variables invisible to declare pass | analyzer.cpp:167-330 | Globals unregistered in global scope |

### Moderate Issues

| # | Issue | Details |
|---|-------|---------|
| 11 | 6 dead functions in codegen | loadVariable, storeVariable, createGEP, createStructGEP, setPointeeType, razenType(TypeInfo*) — all never called |
| 12 | `genUnionConstruct` only processes 1st child | Multi-arg union constructors truncated |
| 13 | genBlock may double-process nodes | Non-Block nodes with children AND left/right get processed twice |
| 14 | `verifyFunction` failure not propagated | Broken functions remain in module silently |
| 15 | SemanticError exception never thrown | Dead exception class |
| 16 | No enum bit shift in codegen | `1 << 0` parsed but codegen may fail for non-constant |
| 17 | No topological ordering in semantic pass | Function A calls B declared after A → B unknown |
| 18 | `unsigned_vars` only tracks direct identifiers | `x + y` works but `x + 1` where `x` is u32 doesn't propagate unsignedness to the literal |
| 19 | Match CFG edge cases produce incorrect block ordering | Noted in ROADMAP.md |
| 20 | Struct field assignment creates wrong GEP for pointer-to-struct | Extra indirection via temporary alloca |

### Missing Features (no code)

| # | Feature | Status |
|---|---------|--------|
| 21 | Generic monomorphization (`@Generic(T)`) | ☐ |
| 22 | Comptime evaluation (`const func`, `@SizeOf`, `@AlignOf`, `@TypeOf`) | ☐ |
| 23 | Async codegen | ☐ |
| 24 | Iterator loops (`loop expr \|item\| { }`) | ☐ |
| 25 | `@Dyn` dynamic dispatch | ☐ |
| 26 | DWARF debug info | ☐ |
| 27 | Cross-compilation target flag | ☐ |
| 28 | Optimization level flags (`-O0`..`-O3`) | ☐ |
| 29 | Error recovery in parser | ☐ |
| 30 | Standard library (23 of 24 modules) | ☐ |

---

## Feature-by-Feature: Expected vs Actual

| Language Feature | Should Produce | What Codegen Actually Does |
|-----------------|----------------|---------------------------|
| `@as(i32, x)` | LLVM bitcast | Returns `i32 0`, no bitcast |
| `@Generic(T) func` | Monomorphized copy | Annotation skipped, function emitted once |
| `~Trait.method` impl | VTable entry | VTable entry is null |
| `behave X { func... }` | VTable type | VTable type created but unused |
| `defer { cleanup() }` | LIFO flush | ✓ Working |
| `try expr` | Propagate on error | ✓ Working for `{i1,T}` |
| `loop items \|i\| { }` | Iterator body | Not implemented |
| `const func` | Comptime eval | `is_const` flag ignored |
| `async func` | Async dispatch | `is_async` flag ignored |

---

## What Would It Take to Reach 90%?

1. **Fix `current_struct`** — one line assignment in genStruct (5 min)
2. **Fix trait vtables** — make genFunc produce mangled names, or make genTraitVTable look up bare names (2-3 hours)
3. **Fix `operator!`** — use `ICmpEQ` against zero instead of XOR 1 (15 min)
4. **Fix `>>` and `%`** — use AShr for signed, select SRem/URem by type (30 min)
5. **Fix array/tuple/range non-constant elements** — store actual genExpr results (30 min)
6. **Fix `@as(Type, val)`** — emit proper bitcast (1 hour)
7. **Restore argument type checking in semantic** — uncomment + fix the code (2 hours)
8. **Fix SymbolTable pass-by-value** — pass by reference (15 min)
9. **Register global variables in declareGlobal** — add VarDeclaration case (15 min)
10. **Add a simple test for each basic language feature** — fill current gap of untested features

Estimated total: ~2 days of focused work to reach genuine 90% functionality.
