; razenc — MIXED_FEATURES — PASS
; ModuleID = 'MIXED_FEATURES'
source_filename = "MIXED_FEATURES"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [12 x i8] c"Hello\0AWorld\00", align 1

declare i32 @printf(ptr, ...)

declare i32 @puts(ptr)

declare void @exit(i32)

declare void @abort(...)

define void @greet() {
entry:
  %msg = alloca ptr, align 8
  store ptr @.str, ptr %msg, align 8
  ret void
}
