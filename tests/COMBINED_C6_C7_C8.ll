; razenc — COMBINED_C6_C7_C8 — PASS
; ModuleID = 'COMBINED_C6_C7_C8'
source_filename = "COMBINED_C6_C7_C8"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%Calc = type { i32 }
%Expr = type { i32, [4 x i8] }

declare i32 @printf(ptr, ...)

declare i32 @puts(ptr)

declare void @exit(i32)

declare void @abort(...)

define void @eval(%Calc %0, %Expr %1) {
entry:
  %e = alloca %Expr, align 8
  %c = alloca %Calc, align 8
  store %Calc %0, ptr %c, align 4
  store %Expr %1, ptr %e, align 4
  %e1 = load %Expr, ptr %e, align 4
  %2 = alloca %Expr, align 8
  store %Expr %e1, ptr %2, align 4
  %tag.ptr = getelementptr inbounds nuw %Expr, ptr %2, i32 0, i32 0
  %tag = load i32, ptr %tag.ptr, align 4
  br label %match.check

match.end:                                        ; preds = %match.case2, %match.next, %match.case
  ret void

match.case:                                       ; preds = %match.check
  %payload.ptr = getelementptr inbounds nuw %Expr, ptr %2, i32 0, i32 1
  %payload = load i32, ptr %payload.ptr, align 4
  %3 = alloca i32, align 4
  store i32 %payload, ptr %3, align 4
  br label %match.end

match.next:                                       ; preds = %match.check
  %4 = icmp eq i32 %tag, 1
  br i1 %4, label %match.case2, label %match.end

match.check:                                      ; preds = %entry
  %5 = icmp eq i32 %tag, 0
  br i1 %5, label %match.case, label %match.next

match.case2:                                      ; preds = %match.next
  %payload.ptr3 = getelementptr inbounds nuw %Expr, ptr %2, i32 0, i32 1
  %payload4 = load i32, ptr %payload.ptr3, align 4
  %6 = alloca i32, align 4
  store i32 %payload4, ptr %6, align 4
  br label %match.end
}

define void @main() {
entry:
  %eval_result = alloca %Expr, align 8
  %n = alloca %Expr, align 8
  %0 = alloca %Expr, align 8
  %tag.ptr = getelementptr inbounds nuw %Expr, ptr %0, i32 0, i32 0
  store i32 0, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %Expr, ptr %0, i32 0, i32 1
  store i32 10, ptr %payload.ptr, align 4
  %union.val = load %Expr, ptr %0, align 4
  store %Expr %union.val, ptr %n, align 4
  %n1 = load %Expr, ptr %n, align 4
  store %Expr %n1, ptr %eval_result, align 4
  ret void
}
