; ModuleID = 'NESTED_IF'
source_filename = "NESTED_IF"
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
  %x = alloca i32, align 4
  store i32 10, ptr %x, align 4
  %x1 = load i32, ptr %x, align 4
  %0 = icmp sgt i32 %x1, 5
  br i1 %0, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  %x2 = load i32, ptr %x, align 4
  %1 = mul i32 %x2, 2
  store i32 %1, ptr %x, align 4
  br label %if.end

if.else:                                          ; preds = %entry
  store i32 0, ptr %x, align 4
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  %x6 = load i32, ptr %x, align 4
  %2 = icmp eq i32 %x6, 20
  br i1 %2, label %if.then3, label %if.else4

if.then3:                                         ; preds = %if.end
  %x7 = load i32, ptr %x, align 4
  %3 = add i32 %x7, 1
  store i32 %3, ptr %x, align 4
  br label %if.end5

if.else4:                                         ; preds = %if.end
  br label %if.end5

if.end5:                                          ; preds = %if.else4, %if.then3
  %x8 = load i32, ptr %x, align 4
  ret i32 %x8
}

attributes #0 = { nounwind }
