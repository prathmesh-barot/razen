; razenc — ELSE_IF — PASS
; ModuleID = 'ELSE_IF'
source_filename = "ELSE_IF"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare i32 @printf(ptr, ...)

declare i32 @puts(ptr)

declare void @exit(i32)

declare void @abort(...)

define void @check(i32 %0) {
entry:
  %x = alloca i32, align 4
  store i32 %0, ptr %x, align 4
  %x1 = load i32, ptr %x, align 4
  %1 = icmp sgt i32 %x1, 10
  br i1 %1, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  br label %if.end

if.else:                                          ; preds = %entry
  %x5 = load i32, ptr %x, align 4
  %2 = icmp sgt i32 %x5, 5
  br i1 %2, label %if.then2, label %if.else3

if.end:                                           ; preds = %if.end4, %if.then
  ret void

if.then2:                                         ; preds = %if.else
  br label %if.end4

if.else3:                                         ; preds = %if.else
  %x9 = load i32, ptr %x, align 4
  %3 = icmp sgt i32 %x9, 0
  br i1 %3, label %if.then6, label %if.else7

if.end4:                                          ; preds = %if.end8, %if.then2
  br label %if.end

if.then6:                                         ; preds = %if.else3
  br label %if.end8

if.else7:                                         ; preds = %if.else3
  br label %if.end8

if.end8:                                          ; preds = %if.else7, %if.then6
  br label %if.end4
}
