; ModuleID = 'ADD_FUNC'
source_filename = "ADD_FUNC"
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

define i32 @add(i32 %0, i32 %1) {
entry:
  %b = alloca i32, align 4
  %a = alloca i32, align 4
  store i32 %0, ptr %a, align 4
  store i32 %1, ptr %b, align 4
  %a1 = load i32, ptr %a, align 4
  %b2 = load i32, ptr %b, align 4
  %2 = add i32 %a1, %b2
  ret i32 %2
}

define i32 @main() {
entry:
  %x = alloca i32, align 4
  %call = call i32 @add(i32 10, i32 20)
  store i32 %call, ptr %x, align 4
  %x1 = load i32, ptr %x, align 4
  ret i32 %x1
}

attributes #0 = { nounwind }
