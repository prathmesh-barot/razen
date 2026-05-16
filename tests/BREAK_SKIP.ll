; razenc — BREAK_SKIP — PASS
; ModuleID = 'BREAK_SKIP'
source_filename = "BREAK_SKIP"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare i32 @printf(ptr, ...)

declare i32 @puts(ptr)

declare void @exit(i32)

declare void @abort(...)

define void @main() {
entry:
  br label %loop.cond

loop.cond:                                        ; preds = %loop.body, %entry
  br label %loop.body

loop.body:                                        ; preds = %loop.cond
  br label %loop.cond

loop.end:                                         ; No predecessors!
  ret void
}
