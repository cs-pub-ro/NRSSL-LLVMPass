; RUN: %opt_nrs -passes=ieee-to-posit --float_type=posit1 -S %s | FileCheck %s

; float arithmetic becomes the posit1 intrinsics, stored constants get posit bits

; CHECK-LABEL: define float @add(
; CHECK: call float @llvm.riscv.nrssl.f32.posit1.add(float %x, float %y)
define float @add(float %x, float %y) {
  %r = fadd float %x, %y
  ret float %r
}

; posit(32,2) 1.0 = 0x40000000, which prints as the IEEE value 2.0
; CHECK-LABEL: define void @store_one(
; CHECK: store float 2.000000e+00, ptr %p
define void @store_one(ptr %p) {
  store float 1.0, ptr %p
  ret void
}
