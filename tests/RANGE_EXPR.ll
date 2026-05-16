; razenc — RANGE_EXPR — PASS
; ModuleID = 'RANGE_EXPR'
source_filename = "RANGE_EXPR"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare i32 @printf(ptr, ...)

declare i32 @puts(ptr)

declare void @exit(i32)

declare void @abort(...)

define void @main() {
entry:
  %r2 = alloca { i32, i32 }, align 8
  %r1 = alloca { i32, i32 }, align 8
  store { i32, i32 } { i32 0, i32 10 }, ptr %r1, align 4
  store { i32, i32 } { i32 0, i32 5 }, ptr %r2, align 4
  ret void
}
