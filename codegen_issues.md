# Codegen Bug & Issue Audit Report

**Date:** 2026-05-16  
**Target:** LLVM 20 / clang++-20  
**Files audited:** `codegen.cpp`, `codegen.h`, `ir.h`, `ir.cpp`

---

## 🚨 CRITICAL (7 issues)

### #1 — Struct Field Index Always 0
- **Files:** `codegen.cpp:572-576` (genAssign), `codegen.cpp:1092-1096` (genMemberAccess)
- **Code:**
  ```cpp
  unsigned fidx = 0;
  for (unsigned i = 0; i < st->getNumElements(); i++) {
      fidx = i;
      break;  // ← always breaks at i=0
  }
  ```
- **Impact:** Every struct field read/write accesses element 0. `point.y` reads `point.x`.
- **Fix:** Remove `break;` and match `fname` against stored field names.

### #2 — Union Payload Size Hardcoded
- **File:** `codegen.cpp:205-215`
- **Code:** `st->getNumElements() * 8` — assumes every struct field is 8 bytes.
- **Impact:** Wrong union memory layout on any platform where fields aren't 8 bytes.
- **Fix:** Use `module.getDataLayout().getTypeAllocSize(ty)`.

### #3 — Match `checkBB` Double Terminator
- **File:** `codegen.cpp:637-647`
- **Code:** `builder.CreateBr(checkBB)` + `builder.CreateCondBr(...)` in same block.
- **Impact:** Unreachable code after first branch; `verifyModule` may reject IR.
- **Fix:** Remove the premature `CreateBr`; use a separate pre-check block.

### #4 — `genIf` `has_return` Gating Broken
- **File:** `codegen.cpp:431-448`
- **Code:** `has_return` saved/restored around each branch, then function adds default ret.
- **Impact:** Functions ending with `if (cond) { ret val }` get a dead-code `ret` appended.
- **Fix:** Track whether at least one branch definitely returns; skip default ret only if all paths return.

### #5 — `genReturn` in If-Bodies Produces Branch Instead of Ret
- **File:** `codegen.cpp:388-411` (genReturn), `415-453` (genIf)
- **Evidence:** Generated IR shows `if.then: br label %if.end` instead of `ret i32 %val`.
- **Impact:** Conditional returns become no-ops; always falls through to code after the if.
- **Fix:** Ensure `genReturn`'s insertion point is in the correct basic block; verify `builder.CreateRet(val)` isn't being overridden.

### #6 — `IRGen` Non-Movable (Dangling Builder Pointer)
- **Files:** `ir.h:32-34`, `codegen.h:12`
- **Code:** `IRBuilder<>` stores raw `LLVMContext*`; `Codegen` holds `IRGen` by value.
- **Impact:** On move, builder points to freed context → silent use-after-free.
- **Fix:** Use `std::unique_ptr<LLVMContext>` or delete copy/move operations.

### #7 — `genCall` Silently Invents Declarations
- **File:** `codegen.cpp:951-957`
- **Code:** If `module.getFunction(name)` returns null, creates new external function from call args.
- **Impact:** Typos like `prnitf()` compile to a new external symbol; linker errors at runtime instead of compile-time errors.
- **Fix:** Report an error for undeclared functions instead of inventing declarations.

---

## 🔴 HIGH (6 issues)

### #8 — Missing `#include <llvm/IR/Constants.h>`
- **Files:** `codegen.cpp`, `ir.cpp`
- **Used:** `ConstantInt::get()`, `ConstantFP::get()`, `ConstantArray::get()`, `ConstantStruct::get()`, `Constant::getNullValue()`, `ConstantPointerNull::get()`
- **Impact:** Fragile compilation — depends on transitive includes. May break with LLVM version changes.

### #9 — Missing `#include <llvm/IR/GlobalValue.h>`
- **File:** `codegen.cpp:353`
- **Used:** `GlobalValue::InternalLinkage`
- **Impact:** May fail to compile on some LLVM 20 configurations.

### #10 — `Module(ctx, name)` Constructor Ordering
- **File:** `ir.h:71`
- **Code:** `module(source, ctx)` — verify this matches LLVM 20's `Module(StringRef, LLVMContext&)` signature.
- **Note:** The LLVM 17+ API is `Module(name, ctx)`. Verify this compiles — the code uses `module(source, ctx)` which passes source first.

### #11 — No Optimization Pipeline
- **File:** Entire codebase
- **Impact:** All variables remain as alloca+load+store (no `mem2reg`). No constant folding, no inlining. IR is unoptimized; programs run 10-100x slower.
- **Note:** Design decision to fix later, but critical for performance.

### #12 — No Object Code Emission or JIT
- **File:** Entire codebase
- **Impact:** Only text IR output; no `--emit=obj` or `--emit=bin`. Cannot produce executables.
- **Note:** Design decision; `llvm-tutor` has a `static` tool and `lli` execution tests.

### #13 — `verifyModule` Called Once at End
- **File:** `codegen.cpp:93`
- **Impact:** If any function has invalid IR, it's only caught after all codegen. No per-function rollback.
- **Fix:** Call `verifyFunction()` after each `genFunc`.

### #14 — Tuple Literal Forces Constant Cast
- **File:** `codegen.cpp:1158-1161`
- **Code:** `cast<Constant>(v)` on all tuple elements. If any element is a runtime expression (function call, variable), cast returns null → LLVM assertion crash.
- **Fix:** Use `dyn_cast<Constant>` and handle non-constant values by storing to an alloca.

---

## 🟡 MEDIUM (6 issues)

### #15 — `genUnary` Dereference Hardcodes i32
- **File:** `codegen.cpp:930`
- **Code:** `builder.CreateLoad(Type::getInt32Ty(ctx), inner)` always loads 32 bits.

### #16 — String Literal Globals Have No Name Dedup
- **File:** `codegen.cpp:798`
- **Code:** `builder.CreateGlobalString(val, ".str")` — same base name for every literal.

### #17 — `has_return` Leaks Across Top-Level Declarations
- **File:** `codegen.cpp:98-119`
- **Code:** `has_return` not reset between top-level items in `genTopLevel`.

### #18 — Duplicate Type Check Functions
- **Files:** `ir.h:21-24`, `ir.cpp:34-41`
- **Code:** Both define `isFloatTy`/`isIntTy` with different implementations.

### #19 — Unused `#include <memory>` in `ir.h:5`

### #20 — Dead Code: `genTop()` in `codegen.h:20`

---

## 🟢 LOW (3 issues)

### #21 — `genMemberAccess` Calls `genExpr` for Type Discovery
- **File:** `codegen.cpp:1045`
- **Impact:** Side-effecting type discovery generates dead code.

### #22 — `genBinary` Coercion Always Uses SExt
- **File:** `codegen.cpp:849-850`
- **Impact:** Unsigned type promotion is wrong.

### #23 — `genVar` Top-Level Global Uses NullValue on Non-Constant Init
- **File:** `codegen.cpp:352`
- **Impact:** Complex initializers silently replaced with zero.
