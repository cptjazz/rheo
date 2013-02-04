; ModuleID = '1.c'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

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

define i32 @foo(i32 %a, i32 %b, i32* %c) nounwind uwtable {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i32*, align 8
  %d = alloca i32, align 4
  %x = alloca i32, align 4
  %y = alloca i32, align 4
  store i32 %a, i32* %1, align 4
  store i32 %b, i32* %2, align 4
  store i32* %c, i32** %3, align 8
  %4 = load i32* %1, align 4
  store i32 %4, i32* %d, align 4
  %5 = load i32* %1, align 4
  %6 = load i32* %2, align 4
  %7 = add nsw i32 %5, %6
  store i32 %7, i32* %x, align 4
  %8 = load i32** %3, align 8
  store i32 5, i32* %8, align 4
  store i32 9, i32* %y, align 4
  %9 = load i32* %y, align 4
  %10 = icmp ne i32 %9, 0
  br i1 %10, label %11, label %14

; <label>:11                                      ; preds = %0
  %12 = load i32* %1, align 4
  %13 = add nsw i32 %12, 1
  store i32 %13, i32* %1, align 4
  br label %17

; <label>:14                                      ; preds = %0
  %15 = load i32* %2, align 4
  %16 = load i32** %3, align 8
  store i32 %15, i32* %16, align 4
  br label %17

; <label>:17                                      ; preds = %14, %11
  br label %18

; <label>:18                                      ; preds = %21, %17
  %19 = load i32* %x, align 4
  %20 = icmp ne i32 %19, 0
  br i1 %20, label %21, label %27

; <label>:21                                      ; preds = %18
  %22 = load i32* %1, align 4
  %23 = load i32** %3, align 8
  %24 = sext i32 %22 to i64
  %25 = getelementptr inbounds i32* %23, i64 %24
  %26 = ptrtoint i32* %25 to i32
  store i32 %26, i32* %2, align 4
  br label %18

; <label>:27                                      ; preds = %18
  %28 = load i32* %y, align 4
  %29 = call i32 @add(i32 %28, i32 3)
  %30 = load i32** %3, align 8
  store i32 %29, i32* %30, align 4
  %31 = load i32* %d, align 4
  %32 = load i32* %2, align 4
  %33 = sub nsw i32 %31, %32
  ret i32 %33
}
