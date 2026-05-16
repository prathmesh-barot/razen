; razenc — ASSIGNMENT_IN_MATCH — PASS
; ModuleID = 'ASSIGNMENT_IN_MATCH'
source_filename = "ASSIGNMENT_IN_MATCH"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%Counter = type { i32 }

declare i32 @printf(ptr, ...)

declare i32 @puts(ptr)

declare void @exit(i32)

declare void @abort(...)

define void @step(%Counter %0, i32 %1) {
entry:
  %d = alloca i32, align 4
  %c = alloca %Counter, align 8
  store %Counter %0, ptr %c, align 4
  store i32 %1, ptr %d, align 4
  %d1 = load i32, ptr %d, align 4
  br label %match.check

match.end:                                        ; preds = %match.case2, %match.next, %match.case
  ret void

match.case:                                       ; preds = %match.check
  br label %match.end

match.next:                                       ; preds = %match.check
  %2 = icmp eq i32 %d1, 1
  br i1 %2, label %match.case2, label %match.end

match.check:                                      ; preds = %entry
  %3 = icmp eq i32 %d1, 0
  br i1 %3, label %match.case, label %match.next

match.case2:                                      ; preds = %match.next
  br label %match.end
}
