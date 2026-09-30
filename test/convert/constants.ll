; Float constants get the bits of the selected format, wherever they are.
; ONE and MTWOHALF are 1.0 and -2.5 in that format, printed as IEEE floats.
; (checked with NRSSL's decoder: they decode back to exactly 1.0 and -2.5)
;
; RUN: %opt_nrs -passes=ieee-to-posit --float_type=posit1 -S %s | FileCheck %s -DFMT=posit1 \
; RUN:   -DONE=2.000000e+00 -DMTWOHALF=0xBEC0000000000000
; RUN: %opt_nrs -passes=ieee-to-posit --float_type=posit2 -S %s | FileCheck %s -DFMT=posit2 \
; RUN:   -DONE=2.000000e+00 -DMTWOHALF=0xBF60000000000000
; RUN: %opt_nrs -passes=ieee-to-posit --float_type=morris -S %s | FileCheck %s -DFMT=morris \
; RUN:   -DONE=0x3880000000000000 -DMTWOHALF=0xB850000000000000
; RUN: %opt_nrs -passes=ieee-to-posit --float_type=morrisHeb -S %s | FileCheck %s -DFMT=morrisHeb \
; RUN:   -DONE=0x3880000000000000 -DMTWOHALF=0xB920000000000000
; RUN: %opt_nrs -passes=ieee-to-posit --float_type=morrisBiasHeb -S %s | FileCheck %s -DFMT=morrisBiasHeb \
; RUN:   -DONE=0x3F00000000000000 -DMTWOHALF=-3.200000e+01
; RUN: %opt_nrs -passes=ieee-to-posit --float_type=morrisUnaryHeb -S %s | FileCheck %s -DFMT=morrisUnaryHeb \
; RUN:   -DONE=2.000000e+00 -DMTWOHALF=0xC480000000000000

; CHECK: @scalar = global float [[ONE]]
@scalar = global float 1.0
; CHECK: @array = global [2 x float] [float [[ONE]], float [[MTWOHALF]]]
@array = global [2 x float] [float 1.0, float -2.5]
; CHECK: @nested = global [2 x [2 x float]] {{\[}}[2 x float] [float [[ONE]], float [[MTWOHALF]]], [2 x float] [float [[MTWOHALF]], float [[ONE]]]]
@nested = global [2 x [2 x float]] [[2 x float] [float 1.0, float -2.5], [2 x float] [float -2.5, float 1.0]]
; zero is zero in every format
; CHECK: @zeros = global [2 x float] zeroinitializer
@zeros = global [2 x float] zeroinitializer

; CHECK-LABEL: define void @store(
; CHECK: store float [[ONE]], ptr %p
define void @store(ptr %p) {
  store float 1.0, ptr %p
  ret void
}

; CHECK-LABEL: define float @operand(
; CHECK: call float @llvm.riscv.nrssl.f32.[[FMT]].add(float %x, float [[ONE]])
define float @operand(float %x) {
  %r = fadd float %x, 1.0
  ret float %r
}

declare float @callee(float)

; CHECK-LABEL: define float @call_arg(
; CHECK: call float @callee(float [[MTWOHALF]])
define float @call_arg() {
  %r = call float @callee(float -2.5)
  ret float %r
}

; CHECK-LABEL: define float @ret(
; CHECK: ret float [[ONE]]
define float @ret() {
  ret float 1.0
}

; CHECK-LABEL: define float @select(
; CHECK: select i1 %c, float [[ONE]], float [[MTWOHALF]]
define float @select(i1 %c) {
  %r = select i1 %c, float 1.0, float -2.5
  ret float %r
}

; CHECK-LABEL: define float @phi(
; CHECK: phi float [ [[ONE]], %entry ], [ [[MTWOHALF]], %other ]
define float @phi(i1 %c) {
entry:
  br i1 %c, label %done, label %other
other:
  br label %done
done:
  %r = phi float [ 1.0, %entry ], [ -2.5, %other ]
  ret float %r
}
