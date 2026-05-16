# LLVM IR Codegen — 100-Step Plan

## Architecture Overview

Razen uses the **LLVM 20 C++ API** directly — no string-based IR assembly. The codegen follows the same patterns as [llvm-tutor](../llvm-tutor/): `llvm::IRBuilder<>`, `llvm::Module`, `llvm::Function`, `llvm::BasicBlock`, `llvm::Value`, `llvm::Type`, and SSA form with mem2reg-friendly allocas.

### Core Files
| File | Purpose |
|------|---------|
| `codegen/ir.h` | `IRGen` struct — LLVM context, module, builder, type registry, scope maps |
| `codegen/ir.cpp` | Type mapping (`razenType`), primitive resolution, GEP helpers, module dump |
| `codegen/codegen.h` | `Codegen` driver — thin wrapper over `IRGen` |
| `codegen/codegen.cpp` | All codegen: functions, variables, expressions, control flow, types |

### Design Principles
- **mem2reg-friendly**: every local is an `alloca` in entry block, stores/loads for mutations
- **two-pass type collection**: structs/enums/unions/errors declared before function bodies
- **SSA via LLVM**: `IRBuilder` handles instruction creation, `opt -mem2reg` promotes to registers
- **zero string IR**: all types, values, and instructions are LLVM API objects

---

## Steps 1-10: Module Infrastructure

1. ✓ **LLVMContext + Module creation** — `IRGen` owns `LLVMContext`, `Module`, `IRBuilder<>`
2. ✓ **Target triple + datalayout** — `x86_64-pc-linux-gnu`, standard x86_64 layout
3. ✓ **Libc declarations** — `printf`, `puts`, `exit`, `abort` via `getOrInsertFunction`
4. ✓ **TypeRegistry** — maps struct/union names → `llvm::StructType*`, aliases → `llvm::Type*`
5. ✓ **Scope maps** — `named_values` (name → `AllocaInst*`), `named_types` (name → `Type*`)
6. ✓ **Control-flow state** — `loop_continue`, `loop_end`, `current_ret_type`, `has_return`
7. ✓ **Deferred collection** — `std::vector<ASTNode*>` for LIFO flush at scope exit
8. ✓ **Type metadata** — `EnumInfo`, `UnionInfo`, error sets collected during AST walk
9. ✓ **Two-pass codegen** — pass 1: type declarations, pass 2: function bodies + globals
10. ✓ **Module verification** — `verifyModule()` after codegen, `verifyFunction()` per-function

## Steps 11-20: Primitive Type Mapping

11. ✓ **void / noret** → `Type::getVoidTy(ctx)`
12. ✓ **bool** → `Type::getInt1Ty(ctx)`
13. ✓ **char** → `Type::getInt8Ty(ctx)`
14. ✓ **i8/u8** → `Type::getInt8Ty(ctx)`
15. ✓ **i16/u16** → `Type::getInt16Ty(ctx)`
16. ✓ **i32/u32/int/uint** → `Type::getInt32Ty(ctx)`
17. ✓ **i64/u64/isize/usize** → `Type::getInt64Ty(ctx)`
18. ✓ **i128/u128** → `Type::getInt128Ty(ctx)`
19. ✓ **f16** → `Type::getHalfTy(ctx)`, **f32/float** → `Type::getFloatTy(ctx)`, **f64** → `Type::getDoubleTy(ctx)`, **f128** → `Type::getFP128Ty(ctx)`
20. ✓ **any** → `PointerType::getUnqual(ctx)`, **str/string** → `PointerType::getUnqual(ctx)`

## Steps 21-30: Compound Type Mapping

21. ✓ **Pointer `*T`** → `PointerType::getUnqual(inner_type)`
22. ✓ **Optional `?T`** → `StructType::get(ctx, {i1, T})` — tagged with validity bit
23. ✓ **Failable `!T`** → `StructType::get(ctx, {i1, T})` — tagged with error bit
24. ✓ **Error union `Error!T`** → `StructType::get(ctx, {i1, T})` — same layout
25. ✓ **Array `[T; N]`** → `ArrayType::get(elem_type, N)`
26. ✓ **Dynamic array `[T]`** → `PointerType::getUnqual(elem_type)`
27. ✓ **Struct `%Name`** → `StructType::create(ctx, field_types, "Name")`, cached in `TypeRegistry`
28. ✓ **Enum** → backing type (`i32` default), values stored in `EnumInfo` map
29. ✓ **Union** → `StructType::create(ctx, {i32, [max_payload x i8]}, "Name")` — tag + byte array
30. ✓ **Self reference** → `PointerType::getUnqual(current_struct_type)` inside methods

## Steps 31-40: Global Variable & Constant Codegen

31. ✓ **Global const declaration** — `GlobalVariable` with `isConstant = true`, external linkage
32. ✓ **Global mutable variable** — `GlobalVariable` with initializer or zeroinitializer
33. ◐ **String literal globals** — `builder.CreateGlobalString(val, ".str")` — no name dedup
34. ✓ **Array constant** — `ConstantArray::get(ArrayType::get(elem, n), {elems...})`
35. ✓ **Struct constant** — `ConstantStruct::get(StructType::get(ctx, types), {fields...})`
36. ✓ **Null pointer** — `Constant::getNullValue(PointerType::getUnqual(ctx))`
37. ✓ **Zero initializer** — `Constant::getNullValue(ty)` for any type
38. ✓ **Integer constants** — `ConstantInt::get(type, value)` or `ConstantInt::get(type, string)`
39. ✓ **Float constants** — `ConstantFP::get(type, string_value)`
40. ✓ **Bool constants** — `ConstantInt::getTrue(ctx)` / `ConstantInt::getFalse(ctx)`

## Steps 41-50: Function Codegen

41. ✓ **Function signature** — `FunctionType::get(ret_type, param_types, isVarArg)`
42. ✓ **Function creation** — `Function::Create(ft, ExternalLinkage, name, module)`
43. ✓ **Entry block** — `BasicBlock::Create(ctx, "entry", fn)`, set builder insert point
44. ✓ **Parameter allocas** — `createEntryBlockAlloca(fn, name, type)` for each param
45. ✓ **Param store** — `builder.CreateStore(&arg, alloca)` to copy param into local
46. ✓ **Method self param** — first param in struct method gets `self` alloca with pointer type
47. ✓ **Body codegen** — `genBlock(node->right)` walks statement list
48. ✓ **Implicit return** — `builder.CreateRetVoid()` or `builder.CreateRet(Constant::getNullValue(ret))`
49. ✓ **External function** — `mod.getOrInsertFunction(name, ft)` for `ext func` declarations
50. ✓ **Function cleanup** — clear `named_values`, `named_types`, `deferred` after each function

## Steps 51-60: Variable Declaration & Assignment

51. ✓ **Typed declaration `x: T`** — `createEntryBlockAlloca(fn, name, type)`, store null
52. ✓ **Inferred declaration `x := expr`** — evaluate expr, get type from result, alloca + store
53. ✓ **Const declaration** — same as mutable but flag stored for potential optimization
54. ✓ **Simple assignment `x = expr`** — `builder.CreateStore(value, named_values[name])`
55. ✓ **Compound `+=`** — load, add, store: `CreateLoad` → `CreateAdd` → `CreateStore`
56. ✓ **Compound `-=`** — load, sub, store
57. ✓ **Compound `*=`** — load, mul/fmul, store
58. ✓ **Compound `/=`** — load, sdiv/fdiv, store
59. ✓ **Compound `%=`** — load, srem, store
60. ✓ **Member assignment `x.field = expr`** — `CreateStructGEP` to field pointer, then store

## Steps 61-70: Expression Codegen

61. ✓ **Integer literal** — `ConstantInt::get(Type::getInt32Ty(ctx), string_value)`
62. ✓ **Float literal** — `ConstantFP::get(Type::getDoubleTy(ctx), string_value)`
63. ✓ **Bool literal** — `ConstantInt::getTrue/False(ctx)`
64. ✓ **Char literal** — `ConstantInt::get(Type::getInt8Ty(ctx), ascii_value)`
65. ◐ **String literal** — `builder.CreateGlobalString(val)` + GEP to i8* (no dedup)
66. ✓ **Identifier load** — `builder.CreateLoad(type, named_values[name], name.c_str())`
67. ✓ **Binary arithmetic** — `CreateAdd`, `CreateSub`, `CreateMul`, `CreateSDiv`, `CreateSRem`
68. ✓ **Binary float arithmetic** — `CreateFAdd`, `CreateFSub`, `CreateFMul`, `CreateFDiv`
69. ✓ **Binary comparison** — `CreateICmpEQ/NE/SLT/SLE/SGT/SGE`, `CreateFCmpOEQ/UNE/OLT/OLE/OGT/OGE`
70. ✓ **Binary bitwise** — `CreateAnd`, `CreateOr`, `CreateXor`, `CreateShl`, `CreateAShr`

## Steps 71-80: Unary, Call, Member Access

71. ✓ **Negation `-x`** — `CreateNeg` (int) or `CreateFNeg` (float)
72. ✓ **Logical not `!x`** — `CreateXor(x, 1)` for i1, or `CreateICmpEQ(x, 0)` for wider
73. ✓ **Bitwise not `~x`** — `CreateXor(x, all_ones)`
74. ✓ **Address-of `&x`** — return the `AllocaInst*` directly (it's already a pointer)
75. ◐ **Dereference `ptr.*`** — `CreateLoad(pointee_type, ptr)` with AllocaInst/LoadInst/GEP type discovery; falls back to i32
76. ✓ **Function call** — `builder.CreateCall(fn, args, "call")` with type coercion
77. ✓ **Call arg coercion** — sign-extend/truncate integer args to match function signature
78. ✓ **Member access `x.field`** — `CreateStructGEP(struct_type, ptr, idx)` then `CreateLoad`
79. ✓ **Enum member `Color.Red`** — `ConstantInt::get(backing_type, enum_value)`
80. ✓ **Error member `FileError.NotFound`** — `ConstantInt::get(i32, error_code)`

## Steps 81-90: Control Flow

81. ✓ **If condition** — `CreateCondBr(cond, thenBB, elseBB)`, create 3 basic blocks
82. ✓ **If then-body** — set insert point to `thenBB`, gen body, `CreateBr(endBB)` if not returned
83. ✓ **If else-body** — set insert point to `elseBB`, gen body, `CreateBr(endBB)` if not returned
84. ✓ **Else-if chain** — recursive `genIf()` in else block, each creates its own 3 BBs
85. ✓ **Loop infinite** — `CreateBr(bodyBB)`, body ends with `CreateBr(condBB)`, cond is always true
86. ✓ **Loop conditional** — condBB checks condition, branches to bodyBB or endBB
87. ✓ **Break** — `builder.CreateBr(loop_end)` using saved loop-end block
88. ✓ **Skip (continue)** — `builder.CreateBr(loop_continue)` using saved loop-continue block
89. ✓ **Match (simple)** — chain of icmp + condbr, each case gets its own BB, fallthrough to end
90. ✓ **Match (union)** — extract tag field, chain of tag comparisons, extract payload via bitcast

## Steps 91-100: Advanced Features

91. ✓ **Defer statement** — push AST node to `deferred` vector, flush LIFO at function exit / return
92. ✓ **Try expression (propagate)** — extract error flag, condbr to propagate BB (returns error union) or success
93. ✓ **Try expression (catch)** — extract flag, condbr to catch BB (runs handler) or success BB, merge at end
94. ✓ **Union constructor `Value.Int(42)`** — alloca union, store tag, bitcast payload slot, store value, load
95. ✓ **Array literal** — `ConstantArray::get` with evaluated element constants + alloca fallback
96. ✓ **Tuple literal** — `ConstantStruct::get` with evaluated field constants + alloca fallback
97. ✓ **Range literal** — `ConstantStruct::get(StructType::get(ctx, {start_ty, end_ty}), {start, end})`
98. ◐ **Struct method call `c.increment()`** — mangle to `StructName.method`, call with self pointer as first arg
99. ✓ **Struct type forward reference** — `StructType::create(ctx)` (opaque) in pass 1, `setBody()` if needed
100. ✓ **Module verification + output** — `verifyModule(module, &errs())`, dump via `raw_string_ostream` to string

**Extra steps (beyond 100):**

101. ✓ **Optimization pipeline** — `PromotePass` + `InstCombinePass` via new PM `PassBuilder`
102. ✓ **Object emission** — `emitObject(path)` via `TargetMachine` + `legacy::PassManager` + `CodeGenFileType::ObjectFile`
103. ✓ **Assembly emission** — `emitAssembly(path)` via `CodeGenFileType::AssemblyFile`
104. ✓ **CLI: --verbose/--debug** — full phase-by-phase output control
105. ✓ **CLI: --help/-h** — usage information
106. ✓ **CLI: --version** — prints `CODEGEN_VERSION "0.0.3"`
107. ✓ **CLI: -f flag + positional args** — external `.rzn` file compilation
108. ✓ **Quiet mode** — completely silent on successful compilation
109. ✓ **SetLexerVerbose/SetParserVerbose** — flags for controlling debug output

## Summary

| Category | ✓ Done | ◐ Partial | ☐ Not started |
|----------|--------|-----------|---------------|
| Steps 1-10: Infrastructure | 10 | 0 | 0 |
| Steps 11-20: Primitives | 10 | 0 | 0 |
| Steps 21-30: Compound types | 10 | 0 | 0 |
| Steps 31-40: Globals/constants | 9 | 1 | 0 |
| Steps 41-50: Functions | 10 | 0 | 0 |
| Steps 51-60: Variables/assign | 10 | 0 | 0 |
| Steps 61-70: Expressions | 9 | 1 | 0 |
| Steps 71-80: Unary/call/member | 8 | 2 | 0 |
| Steps 81-90: Control flow | 10 | 0 | 0 |
| Steps 91-100: Advanced | 8 | 1 | 0 |
| Steps 101-109: Extra | 9 | 0 | 0 |
| **Total** | **103/109** | **6** | **0** |

## Build & Test

```bash
cd Razen-CPP
make clean && make
./razenc --help
./razenc -v samples/sample1.rzn
./razenc --debug samples/sample6.rzn
./razenc -o output/test.o samples/sample1.rzn
```

LLVM 20 must be installed (`llvm-config-20`, `clang++-20`). The Makefile links:
`-lLLVM-20` + system libs (shared build).

## Next Steps After Initial Codegen

- ✓ **mem2reg**: PromotePass via new PM (step 101)
- ✓ **instcombine**: InstCombinePass via new PM (step 101)
- ✓ **Object emission**: emitObject/emitAssembly via TargetMachine (steps 102-103)
- ☐ **String literal dedup**: merge identical string constants
- ☐ **ZExt for unsigned**: fix binary coercion to use CreateZExt for unsigned types
- ☐ **Global non-const init**: handle non-constant initializers for globals
- ☐ **Struct methods**: proper this pointer passing and method lookup
- ☐ **Pointer/reference types**: mutation and borrowing support
- ☐ **Defer + Try/catch full**: unwind/panic handling
