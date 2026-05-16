; razenc — DEFER_BEFORE_RETURN — PASS
; ModuleID = 'DEFER_BEFORE_RETURN'
source_filename = "DEFER_BEFORE_RETURN"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare i32 @printf(ptr, ...)

declare i32 @puts(ptr)

declare void @exit(i32)

declare void @abort(...)

define void @open_file(i1 %0) {
entry:
  %ok = alloca i1, align 1
  store i1 %0, ptr %ok, align 1
  %ok1 = load i1, ptr %ok, align 1
  br i1 %ok1, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  br label %if.end

if.else:                                          ; preds = %entry
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  ret void
}
