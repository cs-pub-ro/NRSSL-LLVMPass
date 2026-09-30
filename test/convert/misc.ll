; RUN: %opt_nrs -passes='nrs-convert<posit1>' -S %s | FileCheck %s

; struct and vector constants, posit1 1.0 prints as 2.0
; CHECK: @s = global { i32, float, [2 x float] } { i32 7, float 2.000000e+00, [2 x float] [float 2.000000e+00, float 0.000000e+00] }
@s = global { i32, float, [2 x float] } { i32 7, float 1.0, [2 x float] [float 1.0, float 0.0] }
; CHECK: @v = global <2 x float> <float 2.000000e+00, float 2.000000e+00>
@v = global <2 x float> <float 1.0, float 1.0>

; CHECK-LABEL: define i1 @always(
; CHECK-NEXT: ret i1 true
define i1 @always(float %x, float %y) {
  %c = fcmp true float %x, %y
  ret i1 %c
}

; CHECK-LABEL: define i1 @never(
; CHECK-NEXT: ret i1 false
define i1 @never(float %x, float %y) {
  %c = fcmp false float %x, %y
  ret i1 %c
}

; CHECK-LABEL: define i64 @fptoui_i64(
; CHECK: [[R:%.*]] = call i64 @llvm.riscv.nrssl.f32.posit1.cvtToUInt(float %x)
; CHECK-NEXT: ret i64 [[R]]
define i64 @fptoui_i64(float %x) {
  %r = fptoui float %x to i64
  ret i64 %r
}

; the format is recorded in the module
; CHECK: !{i32 1, !"nrs.format", !"posit1"}
