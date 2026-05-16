; ModuleID = 'UNION_TAGGED'
source_filename = "UNION_TAGGED"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%Value = type { i32, [4 x i8] }

; Function Attrs: nounwind
declare i32 @printf(ptr readonly, ...) #0

; Function Attrs: nounwind
declare i32 @puts(ptr readonly) #0

; Function Attrs: nounwind
declare void @exit(i32) #0

; Function Attrs: nounwind
declare void @abort(...) #0

define i32 @load(%Value %0) {
entry:
  %r = alloca i32, align 4
  %v = alloca %Value, align 8
  store %Value %0, ptr %v, align 4
  store i32 0, ptr %r, align 4
  %v1 = load %Value, ptr %v, align 4
  %1 = alloca %Value, align 8
  store %Value %v1, ptr %1, align 4
  %tag.ptr = getelementptr inbounds nuw %Value, ptr %1, i32 0, i32 0
  %tag = load i32, ptr %tag.ptr, align 4
  br label %match.check

match.end:                                        ; preds = %match.else, %match.case2, %match.case
  %r5 = load i32, ptr %r, align 4
  ret i32 %r5

match.case:                                       ; preds = %match.check
  %payload.ptr = getelementptr inbounds nuw %Value, ptr %1, i32 0, i32 1
  %payload = load i32, ptr %payload.ptr, align 4
  %2 = alloca i32, align 4
  store i32 %payload, ptr %2, align 4
  %x = load i32, ptr %2, align 4
  store i32 %x, ptr %r, align 4
  br label %match.end

match.next:                                       ; preds = %match.check
  %3 = icmp eq i32 %tag, 1
  br i1 %3, label %match.case2, label %match.else

match.check:                                      ; preds = %entry
  %4 = icmp eq i32 %tag, 0
  br i1 %4, label %match.case, label %match.next

match.case2:                                      ; preds = %match.next
  %payload.ptr3 = getelementptr inbounds nuw %Value, ptr %1, i32 0, i32 1
  %payload4 = load float, ptr %payload.ptr3, align 4
  %5 = alloca float, align 4
  store float %payload4, ptr %5, align 4
  store i32 0, ptr %r, align 4
  br label %match.end

match.else:                                       ; preds = %match.next
  store i32 -1, ptr %r, align 4
  br label %match.end
}

define i32 @main() {
entry:
  %v = alloca %Value, align 8
  %0 = alloca %Value, align 8
  %tag.ptr = getelementptr inbounds nuw %Value, ptr %0, i32 0, i32 0
  store i32 0, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %Value, ptr %0, i32 0, i32 1
  store i32 42, ptr %payload.ptr, align 4
  %union.val = load %Value, ptr %0, align 4
  store %Value %union.val, ptr %v, align 4
  %v1 = load %Value, ptr %v, align 4
  %call = call i32 @load(%Value %v1)
  ret i32 %call
}

attributes #0 = { nounwind }
