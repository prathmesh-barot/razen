; razenc — STRING_ESCAPES — PASS
; ModuleID = 'STRING_ESCAPES'
source_filename = "STRING_ESCAPES"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [12 x i8] c"hello\0Aworld\00", align 1
@.str.1 = private unnamed_addr constant [9 x i8] c"tab\09here\00", align 1
@.str.2 = private unnamed_addr constant [13 x i8] c"quote\22inside\00", align 1
@.str.3 = private unnamed_addr constant [11 x i8] c"back\\slash\00", align 1

declare i32 @printf(ptr, ...)

declare i32 @puts(ptr)

declare void @exit(i32)

declare void @abort(...)

define void @main() {
entry:
  %s4 = alloca ptr, align 8
  %s3 = alloca ptr, align 8
  %s2 = alloca ptr, align 8
  %s1 = alloca ptr, align 8
  store ptr @.str, ptr %s1, align 8
  store ptr @.str.1, ptr %s2, align 8
  store ptr @.str.2, ptr %s3, align 8
  store ptr @.str.3, ptr %s4, align 8
  ret void
}
