; Compares: eq/lt/le map directly, the others are built from them.
; The intrinsics return i64, the pass truncates to i1.
; RUN: %opt_nrs -passes='nrs-convert<posit1>' -S %s | FileCheck %s

; CHECK-LABEL: define i1 @oeq(
; CHECK: [[R:%.*]] = call i64 @llvm.riscv.nrssl.f32.posit1.eq(float %x, float %y)
; CHECK-NEXT: trunc i64 [[R]] to i1
define i1 @oeq(float %x, float %y) {
  %c = fcmp oeq float %x, %y
  ret i1 %c
}

; CHECK-LABEL: define i1 @ueq(
; CHECK: call i64 @llvm.riscv.nrssl.f32.posit1.eq(float %x, float %y)
define i1 @ueq(float %x, float %y) {
  %c = fcmp ueq float %x, %y
  ret i1 %c
}

; CHECK-LABEL: define i1 @olt(
; CHECK: call i64 @llvm.riscv.nrssl.f32.posit1.lt(float %x, float %y)
define i1 @olt(float %x, float %y) {
  %c = fcmp olt float %x, %y
  ret i1 %c
}

; CHECK-LABEL: define i1 @ult(
; CHECK: call i64 @llvm.riscv.nrssl.f32.posit1.lt(float %x, float %y)
define i1 @ult(float %x, float %y) {
  %c = fcmp ult float %x, %y
  ret i1 %c
}

; CHECK-LABEL: define i1 @ole(
; CHECK: call i64 @llvm.riscv.nrssl.f32.posit1.le(float %x, float %y)
define i1 @ole(float %x, float %y) {
  %c = fcmp ole float %x, %y
  ret i1 %c
}

; CHECK-LABEL: define i1 @ule(
; CHECK: call i64 @llvm.riscv.nrssl.f32.posit1.le(float %x, float %y)
define i1 @ule(float %x, float %y) {
  %c = fcmp ule float %x, %y
  ret i1 %c
}

; x > y is y < x
; CHECK-LABEL: define i1 @ogt(
; CHECK: call i64 @llvm.riscv.nrssl.f32.posit1.lt(float %y, float %x)
define i1 @ogt(float %x, float %y) {
  %c = fcmp ogt float %x, %y
  ret i1 %c
}

; CHECK-LABEL: define i1 @ugt(
; CHECK: call i64 @llvm.riscv.nrssl.f32.posit1.lt(float %y, float %x)
define i1 @ugt(float %x, float %y) {
  %c = fcmp ugt float %x, %y
  ret i1 %c
}

; x >= y is y <= x
; CHECK-LABEL: define i1 @oge(
; CHECK: call i64 @llvm.riscv.nrssl.f32.posit1.le(float %y, float %x)
define i1 @oge(float %x, float %y) {
  %c = fcmp oge float %x, %y
  ret i1 %c
}

; CHECK-LABEL: define i1 @uge(
; CHECK: call i64 @llvm.riscv.nrssl.f32.posit1.le(float %y, float %x)
define i1 @uge(float %x, float %y) {
  %c = fcmp uge float %x, %y
  ret i1 %c
}

; x != y is !(x == y)
; CHECK-LABEL: define i1 @one(
; CHECK: [[E:%.*]] = call i64 @llvm.riscv.nrssl.f32.posit1.eq(float %x, float %y)
; CHECK-NEXT: [[T:%.*]] = trunc i64 [[E]] to i1
; CHECK-NEXT: xor i1 [[T]], true
define i1 @one(float %x, float %y) {
  %c = fcmp one float %x, %y
  ret i1 %c
}

; CHECK-LABEL: define i1 @une(
; CHECK: call i64 @llvm.riscv.nrssl.f32.posit1.eq(float %x, float %y)
; CHECK: xor i1
define i1 @une(float %x, float %y) {
  %c = fcmp une float %x, %y
  ret i1 %c
}
