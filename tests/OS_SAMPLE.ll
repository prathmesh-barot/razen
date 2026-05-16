; razenc — OS_SAMPLE — PASS
; ModuleID = 'OS_SAMPLE'
source_filename = "OS_SAMPLE"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare i32 @printf(ptr, ...)

declare i32 @puts(ptr)

declare void @exit(i32)

declare void @abort(...)

define void @main() {
entry:
  %t = alloca i32, align 4
  store i32 0, ptr %t, align 4
  ret void
}
