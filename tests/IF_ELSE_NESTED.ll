; ModuleID = 'IF_ELSE_NESTED'
source_filename = "IF_ELSE_NESTED"
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

define i32 @max(i32 %0, i32 %1) {
entry:
  %b = alloca i32, align 4
  %a = alloca i32, align 4
  store i32 %0, ptr %a, align 4
  store i32 %1, ptr %b, align 4
  %a1 = load i32, ptr %a, align 4
  %b2 = load i32, ptr %b, align 4
  %2 = icmp sgt i32 %a1, %b2
  br i1 %2, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  %a3 = load i32, ptr %a, align 4
  ret i32 %a3

if.else:                                          ; preds = %entry
  br label %if.end

if.end:                                           ; preds = %if.else
  %b4 = load i32, ptr %b, align 4
  ret i32 %b4
}

define i32 @main() {
entry:
  %call = call i32 @max(i32 7, i32 3)
  ret i32 %call
}

attributes #0 = { nounwind }
