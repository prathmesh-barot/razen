; ModuleID = 'INFINITE_LOOP'
source_filename = "INFINITE_LOOP"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nounwind
declare i32 @printf(ptr readonly, ...) #0

; Function Attrs: nounwind
declare i32 @puts(ptr readonly) #0

; Function Attrs: nounwind
declare void @exit(i32) #0

; Function Attrs: nounwind
declare void @abort(...) #0

define i32 @main() {
entry:
  %i = alloca i32, align 4
  %sum = alloca i32, align 4
  store i32 0, ptr %sum, align 4
  store i32 0, ptr %i, align 4
  br label %loop.cond

loop.cond:                                        ; preds = %if.end, %entry
  br label %loop.body

loop.body:                                        ; preds = %loop.cond
  %i1 = load i32, ptr %i, align 4
  %0 = icmp sgt i32 %i1, 5
  br i1 %0, label %if.then, label %if.else

loop.end:                                         ; preds = %if.then
  %sum5 = load i32, ptr %sum, align 4
  ret i32 %sum5

if.then:                                          ; preds = %loop.body
  br label %loop.end

if.else:                                          ; preds = %loop.body
  br label %if.end

if.end:                                           ; preds = %if.else
  %sum2 = load i32, ptr %sum, align 4
  %i3 = load i32, ptr %i, align 4
  %1 = add i32 %sum2, %i3
  store i32 %1, ptr %sum, align 4
  %i4 = load i32, ptr %i, align 4
  %2 = add i32 %i4, 1
  store i32 %2, ptr %i, align 4
  br label %loop.cond
}

attributes #0 = { nounwind }
