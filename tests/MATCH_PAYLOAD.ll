; razenc — MATCH_PAYLOAD — PASS
; ModuleID = 'MATCH_PAYLOAD'
source_filename = "MATCH_PAYLOAD"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%Value = type { i32, [8 x i8] }

declare i32 @printf(ptr, ...)

declare i32 @puts(ptr)

declare void @exit(i32)

declare void @abort(...)

define void @describe(%Value %0) {
entry:
  %value = alloca %Value, align 8
  store %Value %0, ptr %value, align 4
  %value1 = load %Value, ptr %value, align 4
  %1 = alloca %Value, align 8
  store %Value %value1, ptr %1, align 4
  %tag.ptr = getelementptr inbounds nuw %Value, ptr %1, i32 0, i32 0
  %tag = load i32, ptr %tag.ptr, align 4
  br label %match.check

match.end:                                        ; preds = %match.case6, %match.next3, %match.case2, %match.case
  ret void

match.case:                                       ; preds = %match.check
  %payload.ptr = getelementptr inbounds nuw %Value, ptr %1, i32 0, i32 1
  %payload = load i32, ptr %payload.ptr, align 4
  %2 = alloca i32, align 4
  store i32 %payload, ptr %2, align 4
  br label %match.end

match.next:                                       ; preds = %match.check
  %3 = icmp eq i32 %tag, 1
  br i1 %3, label %match.case2, label %match.next3

match.check:                                      ; preds = %entry
  %4 = icmp eq i32 %tag, 0
  br i1 %4, label %match.case, label %match.next

match.case2:                                      ; preds = %match.next
  %payload.ptr4 = getelementptr inbounds nuw %Value, ptr %1, i32 0, i32 1
  %payload5 = load float, ptr %payload.ptr4, align 4
  %5 = alloca float, align 4
  store float %payload5, ptr %5, align 4
  br label %match.end

match.next3:                                      ; preds = %match.next
  %6 = icmp eq i32 %tag, 2
  br i1 %6, label %match.case6, label %match.end

match.case6:                                      ; preds = %match.next3
  %payload.ptr7 = getelementptr inbounds nuw %Value, ptr %1, i32 0, i32 1
  %payload8 = load ptr, ptr %payload.ptr7, align 8
  %7 = alloca ptr, align 8
  store ptr %payload8, ptr %7, align 8
  br label %match.end
}
