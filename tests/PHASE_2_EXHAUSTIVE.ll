; razenc — PHASE_2_EXHAUSTIVE — PASS
; ModuleID = 'PHASE_2_EXHAUSTIVE'
source_filename = "PHASE_2_EXHAUSTIVE"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare i32 @printf(ptr, ...)

declare i32 @puts(ptr)

declare void @exit(i32)

declare void @abort(...)

declare i32 @bind(i32)

define void @handle_conn() {
entry:
  %res = alloca i32, align 4
  %items = alloca [0 x i32], align 4
  %s = alloca i32, align 4
  store i32 0, ptr %s, align 4
  %s1 = load i32, ptr %s, align 4
  br label %match.check

match.end:                                        ; preds = %match.case2, %match.next, %match.case
  store [0 x i32] zeroinitializer, ptr %items, align 4
  br label %loop.cond

match.case:                                       ; preds = %match.check
  br label %match.end

match.next:                                       ; preds = %match.check
  %0 = icmp eq i32 %s1, 1
  br i1 %0, label %match.case2, label %match.end

match.check:                                      ; preds = %entry
  %1 = icmp eq i32 %s1, 0
  br i1 %1, label %match.case, label %match.next

match.case2:                                      ; preds = %match.next
  br label %match.end

loop.cond:                                        ; preds = %loop.body, %match.end
  %items3 = load [0 x i32], ptr %items, align 4
  br [0 x i32] %items3, label %loop.body, label %loop.end

loop.body:                                        ; preds = %loop.cond
  br label %loop.cond

loop.end:                                         ; preds = %loop.cond
  store i32 0, ptr %res, align 4
  ret void
}

define void @main() {
entry:
  %call = call void @handle_conn()
  ret void
}
