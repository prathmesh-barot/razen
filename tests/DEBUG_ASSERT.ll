; razenc — DEBUG_ASSERT — PASS
; ModuleID = 'DEBUG_ASSERT'
source_filename = "DEBUG_ASSERT"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare i32 @printf(ptr, ...)

declare i32 @puts(ptr)

declare void @exit(i32)

declare void @abort(...)

define i32 @divide(i32 %0, i32 %1) {
entry:
  %b = alloca i32, align 4
  %a = alloca i32, align 4
  store i32 %0, ptr %a, align 4
  store i32 %1, ptr %b, align 4
  %a1 = load i32, ptr %a, align 4
  %b2 = load i32, ptr %b, align 4
  %2 = sdiv i32 %a1, %b2
  ret i32 %2
}

define void @main() {
entry:
  %x = alloca i32, align 4
  %call = call i32 @divide(i32 10, i32 2)
  store i32 %call, ptr %x, align 4
  ret void
}
