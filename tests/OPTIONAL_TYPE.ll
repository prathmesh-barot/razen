; razenc — OPTIONAL_TYPE — PASS
; ModuleID = 'OPTIONAL_TYPE'
source_filename = "OPTIONAL_TYPE"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%Person = type { ptr, i32 }

declare i32 @printf(ptr, ...)

declare i32 @puts(ptr)

declare void @exit(i32)

declare void @abort(...)

define { i1, %Person } @find_person(i32 %0) {
entry:
  %id = alloca i32, align 4
  store i32 %0, ptr %id, align 4
  ret i32 0
}
