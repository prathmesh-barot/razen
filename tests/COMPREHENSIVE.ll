; ModuleID = 'COMPREHENSIVE'
source_filename = "COMPREHENSIVE"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@MAX = internal constant i32 100

; Function Attrs: nounwind
declare i32 @printf(ptr readonly, ...) #0

; Function Attrs: nounwind
declare i32 @puts(ptr readonly) #0

; Function Attrs: nounwind
declare void @exit(i32) #0

; Function Attrs: nounwind
declare void @abort(...) #0

define i1 @is_positive(i32 %0) {
entry:
  %n = alloca i32, align 4
  store i32 %0, ptr %n, align 4
  %n1 = load i32, ptr %n, align 4
  %1 = icmp sgt i32 %n1, 0
  ret i1 %1
}

define i32 @abs(i32 %0) {
entry:
  %n = alloca i32, align 4
  store i32 %0, ptr %n, align 4
  %n1 = load i32, ptr %n, align 4
  %1 = icmp sge i32 %n1, 0
  br i1 %1, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  %n2 = load i32, ptr %n, align 4
  ret i32 %n2

if.else:                                          ; preds = %entry
  br label %if.end

if.end:                                           ; preds = %if.else
  %n3 = load i32, ptr %n, align 4
  %2 = sub i32 0, %n3
  ret i32 %2
}

define i32 @main() {
entry:
  %v = alloca i32, align 4
  store i32 -5, ptr %v, align 4
  %v1 = load i32, ptr %v, align 4
  %call = call i32 @abs(i32 %v1)
  store i32 %call, ptr %v, align 4
  %v2 = load i32, ptr %v, align 4
  %call3 = call i1 @is_positive(i32 %v2)
  br i1 %call3, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  %v4 = load i32, ptr %v, align 4
  %0 = add i32 %v4, 1
  store i32 %0, ptr %v, align 4
  br label %if.end

if.else:                                          ; preds = %entry
  store i32 0, ptr %v, align 4
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  %v5 = load i32, ptr %v, align 4
  ret i32 %v5
}

attributes #0 = { nounwind }
