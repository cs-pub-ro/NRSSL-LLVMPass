; Int <-> float conversions. The intrinsics work on i64, the pass extends or
; truncates to the actual integer type.
; RUN: %opt_nrs -passes=ieee-to-posit --float_type=posit1 -S %s | FileCheck %s

; CHECK-LABEL: define i32 @fptosi_i32(
; CHECK: [[R:%.*]] = call i64 @llvm.riscv.nrssl.f32.posit1.cvtToInt(float %x)
; CHECK-NEXT: trunc i64 [[R]] to i32
define i32 @fptosi_i32(float %x) {
  %r = fptosi float %x to i32
  ret i32 %r
}

; CHECK-LABEL: define i64 @fptosi_i64(
; CHECK: [[R:%.*]] = call i64 @llvm.riscv.nrssl.f32.posit1.cvtToInt(float %x)
; CHECK-NEXT: ret i64 [[R]]
define i64 @fptosi_i64(float %x) {
  %r = fptosi float %x to i64
  ret i64 %r
}

; CHECK-LABEL: define i32 @fptoui_i32(
; CHECK: [[R:%.*]] = call i64 @llvm.riscv.nrssl.f32.posit1.cvtToUInt(float %x)
; CHECK-NEXT: trunc i64 [[R]] to i32
define i32 @fptoui_i32(float %x) {
  %r = fptoui float %x to i32
  ret i32 %r
}

; CHECK-LABEL: define float @sitofp_i32(
; CHECK: [[E:%.*]] = sext i32 %i to i64
; CHECK-NEXT: call float @llvm.riscv.nrssl.f32.posit1.cvtFromInt(i64 [[E]])
define float @sitofp_i32(i32 %i) {
  %r = sitofp i32 %i to float
  ret float %r
}

; CHECK-LABEL: define float @sitofp_i64(
; CHECK: call float @llvm.riscv.nrssl.f32.posit1.cvtFromInt(i64 %i)
define float @sitofp_i64(i64 %i) {
  %r = sitofp i64 %i to float
  ret float %r
}

; CHECK-LABEL: define float @uitofp_i32(
; CHECK: [[E:%.*]] = zext i32 %i to i64
; CHECK-NEXT: call float @llvm.riscv.nrssl.f32.posit1.cvtFromUInt(i64 [[E]])
define float @uitofp_i32(i32 %i) {
  %r = uitofp i32 %i to float
  ret float %r
}
