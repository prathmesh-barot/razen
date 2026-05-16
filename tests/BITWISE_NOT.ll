; razenc — BITWISE_NOT — PASS
; ModuleID = 'BITWISE_NOT'
source_filename = "BITWISE_NOT"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare i32 @printf(ptr, ...)

declare i32 @puts(ptr)

declare void @exit(i32)

declare void @abort(...)

define void @main() {
entry:
  %y = alloca i32, align 4
  %x = alloca i32, align 4
  store i32 42, ptr %x, align 4
  %x1 = load i32, ptr %x, align 4
  %bitnot = xor i32 %x1, -1
  store i32 %bitnot, ptr %y, align 4
  ret void
}
