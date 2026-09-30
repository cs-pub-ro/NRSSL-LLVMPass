; min/max/sqrt map to intrinsics, fneg and fabs are built from sub and lt.
; RUN: %opt_nrs -passes=ieee-to-posit --float_type=posit1 -S %s | FileCheck %s

; CHECK-LABEL: define float @min(
; CHECK: call float @llvm.riscv.nrssl.f32.posit1.min(float %x, float %y)
define float @min(float %x, float %y) {
  %r = call float @llvm.minnum.f32(float %x, float %y)
  ret float %r
}

; CHECK-LABEL: define float @max(
; CHECK: call float @llvm.riscv.nrssl.f32.posit1.max(float %x, float %y)
define float @max(float %x, float %y) {
  %r = call float @llvm.maxnum.f32(float %x, float %y)
  ret float %r
}

; CHECK-LABEL: define float @sqrt(
; CHECK: call float @llvm.riscv.nrssl.f32.posit1.sqrt(float %x)
define float @sqrt(float %x) {
  %r = call float @llvm.sqrt.f32(float %x)
  ret float %r
}

; -x is 0 - x (posit zero is all zero bits)
; CHECK-LABEL: define float @neg(
; CHECK: call float @llvm.riscv.nrssl.f32.posit1.sub(float 0.000000e+00, float %x)
define float @neg(float %x) {
  %r = fneg float %x
  ret float %r
}

; |x| is x < 0 ? 0 - x : x
; CHECK-LABEL: define float @abs(
; CHECK: [[N:%.*]] = call float @llvm.riscv.nrssl.f32.posit1.sub(float 0.000000e+00, float %x)
; CHECK-NEXT: [[C:%.*]] = call i64 @llvm.riscv.nrssl.f32.posit1.lt(float %x, float 0.000000e+00)
; CHECK-NEXT: [[T:%.*]] = trunc i64 [[C]] to i1
; CHECK-NEXT: select i1 [[T]], float [[N]], float %x
define float @abs(float %x) {
  %r = call float @llvm.fabs.f32(float %x)
  ret float %r
}

declare float @llvm.minnum.f32(float, float)
declare float @llvm.maxnum.f32(float, float)
declare float @llvm.sqrt.f32(float)
declare float @llvm.fabs.f32(float)
