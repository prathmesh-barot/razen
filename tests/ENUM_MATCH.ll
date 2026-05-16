; ModuleID = 'ENUM_MATCH'
source_filename = "ENUM_MATCH"
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

define i32 @code(i32 %0) {
entry:
  %r = alloca i32, align 4
  %c = alloca i32, align 4
  store i32 %0, ptr %c, align 4
  store i32 0, ptr %r, align 4
  %c1 = load i32, ptr %c, align 4

match.end:                                        ; No predecessors!
  %r2 = load i32, ptr %r, align 4
  ret i32 %r2
}

define i32 @main() {
entry:
  %call = call i32 @code(i32 2)
  ret i32 %call
}

attributes #0 = { nounwind }
