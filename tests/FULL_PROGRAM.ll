; razenc — FULL_PROGRAM — PASS
; ModuleID = 'FULL_PROGRAM'
source_filename = "FULL_PROGRAM"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@MAX = internal constant i32 100

declare i32 @printf(ptr, ...)

declare i32 @puts(ptr)

declare void @exit(i32)

declare void @abort(...)

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

define i1 @is_even(i32 %0) {
entry:
  %n = alloca i32, align 4
  store i32 %0, ptr %n, align 4
  %n1 = load i32, ptr %n, align 4
  %1 = srem i32 %n1, 2
  %2 = icmp eq i32 %1, 0
  ret i1 %2
}

define void @main() {
entry:
  %counter = alloca i32, align 4
  %result = alloca i32, align 4
  %y = alloca i32, align 4
  %x = alloca i32, align 4
  store i32 10, ptr %x, align 4
  store i32 20, ptr %y, align 4
  %x1 = load i32, ptr %x, align 4
  %y2 = load i32, ptr %y, align 4
  %call = call i32 @add(i32 %x1, i32 %y2)
  store i32 %call, ptr %result, align 4
  %result3 = load i32, ptr %result, align 4
  %0 = icmp eq i32 %result3, 30
  br i1 %0, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  br label %if.end

if.else:                                          ; preds = %entry
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  store i32 0, ptr %counter, align 4
  br label %loop.cond

loop.cond:                                        ; preds = %loop.body, %if.end
  br label %loop.body

loop.body:                                        ; preds = %loop.cond
  br label %loop.cond

loop.end:                                         ; No predecessors!
  ret void
}
