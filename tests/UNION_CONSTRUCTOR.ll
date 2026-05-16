; razenc — UNION_CONSTRUCTOR — PASS
; ModuleID = 'UNION_CONSTRUCTOR'
source_filename = "UNION_CONSTRUCTOR"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%Value = type { i32, [4 x i8] }

declare i32 @printf(ptr, ...)

declare i32 @puts(ptr)

declare void @exit(i32)

declare void @abort(...)

define %Value @make_int() {
entry:
  %x = alloca %Value, align 8
  %0 = alloca %Value, align 8
  %tag.ptr = getelementptr inbounds nuw %Value, ptr %0, i32 0, i32 0
  store i32 0, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %Value, ptr %0, i32 0, i32 1
  store i32 42, ptr %payload.ptr, align 4
  %union.val = load %Value, ptr %0, align 4
  store %Value %union.val, ptr %x, align 4
  %x1 = load %Value, ptr %x, align 4
  ret %Value %x1
}
