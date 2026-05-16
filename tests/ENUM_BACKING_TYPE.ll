; razenc — ENUM_BACKING_TYPE — PASS
; ModuleID = 'ENUM_BACKING_TYPE'
source_filename = "ENUM_BACKING_TYPE"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare i32 @printf(ptr, ...)

declare i32 @puts(ptr)

declare void @exit(i32)

declare void @abort(...)

define void @check_status(i32 %0) {
entry:
  %s = alloca i32, align 4
  %code = alloca i32, align 4
  store i32 %0, ptr %code, align 4
  %code1 = load i32, ptr %code, align 4
  store i32 %code1, ptr %s, align 4
  ret void
}
