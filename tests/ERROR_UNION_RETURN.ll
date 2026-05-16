; razenc — ERROR_UNION_RETURN — PASS
; ModuleID = 'ERROR_UNION_RETURN'
source_filename = "ERROR_UNION_RETURN"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare i32 @printf(ptr, ...)

declare i32 @puts(ptr)

declare void @exit(i32)

declare void @abort(...)

define { i1, ptr } @read_file(ptr %0) {
entry:
  %path = alloca ptr, align 8
  store ptr %0, ptr %path, align 8
  ret i32 0
}
