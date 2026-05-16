; razenc — CHAR_ESCAPES — PASS
; ModuleID = 'CHAR_ESCAPES'
source_filename = "CHAR_ESCAPES"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare i32 @printf(ptr, ...)

declare i32 @puts(ptr)

declare void @exit(i32)

declare void @abort(...)

define void @main() {
entry:
  %c4 = alloca i8, align 1
  %c3 = alloca i8, align 1
  %c2 = alloca i8, align 1
  %c1 = alloca i8, align 1
  store i8 10, ptr %c1, align 1
  store i8 9, ptr %c2, align 1
  store i8 92, ptr %c3, align 1
  store i8 39, ptr %c4, align 1
  ret void
}
