; Float arithmetic becomes the intrinsics of the selected format.
; RUN: %opt_nrs -passes='nrs-convert<posit1>' -S %s | FileCheck %s -DFMT=posit1
; RUN: %opt_nrs -passes='nrs-convert<posit2>' -S %s | FileCheck %s -DFMT=posit2
; RUN: %opt_nrs -passes='nrs-convert<morris>' -S %s | FileCheck %s -DFMT=morris
; RUN: %opt_nrs -passes='nrs-convert<morrisHeb>' -S %s | FileCheck %s -DFMT=morrisHeb
; RUN: %opt_nrs -passes='nrs-convert<morrisBiasHeb>' -S %s | FileCheck %s -DFMT=morrisBiasHeb
; RUN: %opt_nrs -passes='nrs-convert<morrisUnaryHeb>' -S %s | FileCheck %s -DFMT=morrisUnaryHeb

; CHECK-LABEL: define float @arith(
; CHECK-NEXT: [[A:%.*]] = call float @llvm.riscv.nrssl.f32.[[FMT]].add(float %x, float %y)
; CHECK-NEXT: [[S:%.*]] = call float @llvm.riscv.nrssl.f32.[[FMT]].sub(float [[A]], float %y)
; CHECK-NEXT: [[M:%.*]] = call float @llvm.riscv.nrssl.f32.[[FMT]].mul(float [[S]], float %x)
; CHECK-NEXT: [[D:%.*]] = call float @llvm.riscv.nrssl.f32.[[FMT]].div(float [[M]], float %y)
; CHECK-NEXT: ret float [[D]]
define float @arith(float %x, float %y) {
  %a = fadd float %x, %y
  %s = fsub float %a, %y
  %m = fmul float %s, %x
  %d = fdiv float %m, %y
  ret float %d
}
