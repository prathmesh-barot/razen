; razenc — BLOCK_MULTI_LINE — PASS
; ModuleID = 'BLOCK_MULTI_LINE'
source_filename = "BLOCK_MULTI_LINE"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare i32 @printf(ptr, ...)

declare i32 @puts(ptr)

declare void @exit(i32)

declare void @abort(...)

define void @main() {
entry:
  ret void
}
