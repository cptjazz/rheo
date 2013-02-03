; ModuleID = '1.c'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

define i32 @main() nounwind uwtable {
  %1 = alloca i32, align 4
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  store i32 0, i32* %1
  store i32 0, i32* %a, align 4
  store i32 1, i32* %b, align 4
  %2 = load i32* %b, align 4
  %3 = add nsw i32 %2, 5
  store i32 %3, i32* %a, align 4
  br label %4

; <label>:4                                       ; preds = %7, %0
  %5 = load i32* %a, align 4
  %6 = icmp slt i32 %5, 10
  br i1 %6, label %7, label %10

; <label>:7                                       ; preds = %4
  %8 = load i32* %a, align 4
  %9 = sub nsw i32 0, %8
  store i32 %9, i32* %b, align 4
  br label %4

; <label>:10                                      ; preds = %4
  %11 = load i32* %a, align 4
  ret i32 %11
}

define i32 @add(i32 %x, i32 %y) nounwind uwtable {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  store i32 %x, i32* %1, align 4
  store i32 %y, i32* %2, align 4
  %3 = load i32* %1, align 4
  %4 = load i32* %2, align 4
  %5 = add nsw i32 %3, %4
  ret i32 %5
}
