; razenc — COMBINED_FEATURES — PASS
; ModuleID = 'COMBINED_FEATURES'
source_filename = "COMBINED_FEATURES"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare i32 @printf(ptr, ...)

declare i32 @puts(ptr)

declare void @exit(i32)

declare void @abort(...)

define i32 @double(i32 %0) {
entry:
  %x = alloca i32, align 4
  store i32 %0, ptr %x, align 4
  %x1 = load i32, ptr %x, align 4
  %1 = mul i32 %x1, 2
  ret i32 %1
}

define i32 @helper(i32 %0) {
entry:
  %x = alloca i32, align 4
  store i32 %0, ptr %x, align 4
  %x1 = load i32, ptr %x, align 4
  %1 = add i32 %x1, 1
  ret i32 %1
}

define void @main() {
entry:
  %y = alloca i32, align 4
  %x = alloca i32, align 4
  %c = alloca i8, align 1
  store i8 0, ptr %c, align 1
  %call = call i32 @helper(i32 10)
  store i32 %call, ptr %x, align 4
  %x1 = load i32, ptr %x, align 4
  %call2 = call i32 @double(i32 %x1)
  store i32 %call2, ptr %y, align 4
  ret void
}
