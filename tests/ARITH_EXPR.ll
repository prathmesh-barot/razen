; razenc — ARITH_EXPR — PASS
; ModuleID = 'ARITH_EXPR'
source_filename = "ARITH_EXPR"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare i32 @printf(ptr, ...)

declare i32 @puts(ptr)

declare void @exit(i32)

declare void @abort(...)

define i32 @compute() {
entry:
  %b = alloca i32, align 4
  %a = alloca i32, align 4
  store i32 11, ptr %a, align 4
  %a1 = load i32, ptr %a, align 4
  %0 = sub i32 %a1, 1
  store i32 %0, ptr %b, align 4
  %a2 = load i32, ptr %a, align 4
  %b3 = load i32, ptr %b, align 4
  %1 = add i32 %a2, %b3
  ret i32 %1
}
