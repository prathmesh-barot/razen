; ModuleID = 'FIBONACCI_REC'
source_filename = "FIBONACCI_REC"
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

define i32 @fib(i32 %0) {
entry:
  %n = alloca i32, align 4
  store i32 %0, ptr %n, align 4
  %n1 = load i32, ptr %n, align 4
  %1 = icmp sle i32 %n1, 1
  br i1 %1, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  %n2 = load i32, ptr %n, align 4
  ret i32 %n2

if.else:                                          ; preds = %entry
  br label %if.end

if.end:                                           ; preds = %if.else
  %n3 = load i32, ptr %n, align 4
  %2 = sub i32 %n3, 1
  %call = call i32 @fib(i32 %2)
  %n4 = load i32, ptr %n, align 4
  %3 = sub i32 %n4, 2
  %call5 = call i32 @fib(i32 %3)
  %4 = add i32 %call, %call5
  ret i32 %4
}

define i32 @main() {
entry:
  %call = call i32 @fib(i32 10)
  ret i32 %call
}

attributes #0 = { nounwind }
