; razenc — FORMAT_STRING — PASS
; ModuleID = 'FORMAT_STRING'
source_filename = "FORMAT_STRING"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [10 x i8] c"Prathmesh\00", align 1

declare i32 @printf(ptr, ...)

declare i32 @puts(ptr)

declare void @exit(i32)

declare void @abort(...)

define void @main() {
entry:
  %age = alloca i32, align 4
  %name = alloca ptr, align 8
  store ptr @.str, ptr %name, align 8
  store i32 22, ptr %age, align 4
  ret void
}
