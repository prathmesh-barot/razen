# Codegen Bug & Issue Audit Report (Updated 2026-05-16)

**Target:** LLVM 20 / clang++-20
**CODEGEN_VERSION:** 0.0.3

---

## ✅ FIXED (was CRITICAL)

### #1 — Struct Field Index Always 0
- **Status:** FIXED — both `genAssign` and `genMemberAccess` now match field names correctly.
- `genAssign:585-588` iterates `fnames[i] == fname`.
- `genMemberAccess:1118-1120` uses same pattern.

### #3 — Match `checkBB` Double Terminator
- **Status:** FIXED — `genMatch:653` checks for terminator before branching; no premature `CreateBr`.

### #4 — `genIf` `has_return` Gating Broken
- **Status:** FIXED — per-branch return tracking, terminator check before `CreateBr(endBB)`, `has_return = has_return || (then_ret && else_ret)`.

### #5 — `genReturn` in If-Bodies Produces Branch Instead of Ret
- **Status:** FIXED — verified: `abs(-5)` returns 5 correctly; `genReturn:416` emits `CreateRet(val)` in correct BB.

### #7 — `genCall` Silently Invents Declarations
- **Status:** FIXED — `genCall:978-981` now prints "Error: call to undeclared function 'name'" and returns null. Semantic analyzer also catches undeclared functions with `[NameError]`.

### #14 — Tuple Literal Forces Constant Cast
- **Status:** FIXED — `genTupleLiteral:1208` uses `dyn_cast<Constant>` with alloca+store fallback (same pattern as `genArrayLiteral`).

## ✅ FIXED (was HIGH)

### #8 — Missing `#include <llvm/IR/Constants.h>`
- **Status:** FIXED — included at `codegen.cpp:6`.

### #9 — Missing `#include <llvm/IR/GlobalValue.h>`
- **Status:** FIXED — included at `codegen.cpp:7`.

### #11 — No Optimization Pipeline
- **Status:** FIXED — `Codegen::optimize()` runs `PromotePass` + `InstCombinePass` via new PM `PassBuilder`.

### #12 — No Object Code Emission or JIT
- **Status:** FIXED — `emitObject()` and `emitAssembly()` via `TargetMachine` + `legacy::PassManager` + `addPassesToEmitFile`.

## ✅ FIXED (was MEDIUM)

### #17 — `has_return` Leaks Across Top-Level Declarations
- **Status:** FIXED — `has_return` always reset to `false` at `genFunc:296` before each function body.

### #18 — Duplicate Type Check Functions
- **Status:** FIXED — `isIntTy`/`isFloatTy` in `ir.h:23-26` match lambdas in `codegen.cpp`. No duplicates.

### #20 — Dead Code: `genTop()` in `codegen.h`
- **Status:** FIXED — `genTop()` no longer exists; `Codegen` uses `generate()`.

---

## 🔴 OPEN — MEDIUM/HIGH Priority

### #2 — Union Payload Size Hardcoded
- **Status:** OPEN — `genUnion:217` uses `module.getDataLayout().getTypeAllocSize(vi.payload_type)` which is correct LLVM API. Actual union size may be wrong for complex nested types or alignment mismatches.

### #13 — `verifyModule` Called Once at End
- **Status:** PARTIAL — per-function `verifyFunction` in `genFunc:331`. Module-level `verifyModule:104` remains as final check. Consider `verifyFunction` after ALL functions, not just the current one.

### #15 — `genUnary` Dereference Hardcodes i32 Fallback
- **Status:** PARTIAL — now handles `AllocaInst`, `LoadInst`, `GetElementPtrInst` with type discovery (`genUnary:949-955`). Falls back to i32 for unknown cases.

### #16 — String Literal Globals Have No Name Dedup
- **Status:** OPEN — `genLiteral:817`: `builder.CreateGlobalString(val, ".str")` — same base name for every literal, no dedup.

### #22 — `genBinary` Coercion Always Uses SExt
- **Status:** OPEN — `genBinary:868` uses `CreateSExt` unconditionally. Should use `CreateZExt` for unsigned types.

### #23 — `genVar` Top-Level Global Uses NullValue on Non-Constant Init
- **Status:** OPEN — `genVar:357`: non-constant initializers are silently replaced with `Constant::getNullValue(ty)`.

### #6 — `IRGen` Non-Movable (Dangling Builder Pointer)
- **Status:** MITIGATED — `IRGen` copy/move deleted (`ir.h:82-85`), `Codegen` copy/move deleted (`codegen.h:14-17`). Safe as long as no std::move is attempted at runtime (will fail at compile time).

---

## 🟡 OPEN — LOW Priority

### #19 — Unused `#include <memory>` in `ir.h:5`
- **Status:** LOW — `std::unique_ptr` not used (move ops deleted instead). Harmless but unnecessary.

### #21 — `genMemberAccess` Calls `genExpr` for Type Discovery
- **Status:** LOW — `genMemberAccess:1069`: calls `genExpr(a->left)->getType()` for method call type discovery, generating dead code. Side-effect-free in practice.

---

## Summary

| Priority | OPEN | FIXED | PARTIAL |
|----------|------|-------|---------|
| CRITICAL | 0 | 5 | 0 |
| HIGH | 1 | 4 | 1 |
| MEDIUM | 2 | 3 | 1 |
| LOW | 2 | 0 | 0 |
| **Total** | **5** | **12** | **2** |
