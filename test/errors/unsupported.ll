; f32 operations without an NRS equivalent are errors, with the function they are in.
; RUN: split-file %s %t
; RUN: not %opt_nrs -passes='nrs-convert<posit1>' -disable-output %t/frem.ll 2>&1 | FileCheck %s --check-prefix=FREM
; RUN: not %opt_nrs -passes='nrs-convert<posit1>' -disable-output %t/ord.ll 2>&1 | FileCheck %s --check-prefix=ORD
; RUN: not %opt_nrs -passes='nrs-convert<posit1>' -disable-output %t/fma.ll 2>&1 | FileCheck %s --check-prefix=FMA
; RUN: not %opt_nrs -passes='nrs-convert<posit1>' -disable-output %t/vector.ll 2>&1 | FileCheck %s --check-prefix=VECTOR
; RUN: not %opt_nrs -passes='nrs-convert<posit1>' -disable-output %t/fpext.ll 2>&1 | FileCheck %s --check-prefix=FPEXT
; RUN: not %opt_nrs -passes='nrs-convert<posit1>' -disable-output %t/fptrunc.ll 2>&1 | FileCheck %s --check-prefix=FPTRUNC

; FREM: error: {{.*}}in function frem{{.*}}frem has no NRS equivalent
; ORD: error: {{.*}}in function ord{{.*}}fcmp ord (NaN check) has no NRS equivalent
; FMA: error: {{.*}}in function fma{{.*}}llvm.fma.f32 has no NRS equivalent
; VECTOR: error: {{.*}}in function vector{{.*}}vector f32 operations are not supported
; FPEXT: error: {{.*}}in function fpext{{.*}}fpext of an NRS f32 value to an IEEE type
; FPTRUNC: error: {{.*}}in function fptrunc{{.*}}fptrunc of an IEEE value to an NRS f32 value

;--- frem.ll
define float @frem(float %x, float %y) {
  %r = frem float %x, %y
  ret float %r
}

;--- ord.ll
define i1 @ord(float %x, float %y) {
  %r = fcmp ord float %x, %y
  ret i1 %r
}

;--- fma.ll
define float @fma(float %x, float %y, float %z) {
  %r = call float @llvm.fma.f32(float %x, float %y, float %z)
  ret float %r
}
declare float @llvm.fma.f32(float, float, float)

;--- vector.ll
define <2 x float> @vector(<2 x float> %x, <2 x float> %y) {
  %r = fadd <2 x float> %x, %y
  ret <2 x float> %r
}

;--- fpext.ll
define double @fpext(float %x) {
  %r = fpext float %x to double
  ret double %r
}

;--- fptrunc.ll
define float @fptrunc(double %x) {
  %r = fptrunc double %x to float
  ret float %r
}
