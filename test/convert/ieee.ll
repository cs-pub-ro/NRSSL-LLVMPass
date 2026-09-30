; Floating point code that is not f32 stays IEEE, with one warning per
; function. Calls to f32 functions outside the module get a warning each.
; RUN: %opt_nrs -passes='nrs-convert<posit1>' -S %s 2>%t.err | FileCheck %s
; RUN: FileCheck %s --check-prefix=WARN < %t.err

; WARN: warning: {{.*}}in function doubles{{.*}}floating point code that is not f32 stays IEEE
; WARN-NOT: in function doubles
; WARN: warning: {{.*}}in function libm{{.*}}the called function takes or returns f32 but is not in this module
; WARN-NOT: warning

; double constants stay IEEE
; CHECK: @d = global double 1.000000e+00
@d = global double 1.0

; CHECK-LABEL: define double @doubles(
; CHECK-NEXT: %a = fadd double %x, 1.000000e+00
; CHECK-NEXT: %m = fmul double %a, %x
; CHECK-NEXT: %c = fcmp olt double %m, %x
define double @doubles(double %x) {
  %a = fadd double %x, 1.0
  %m = fmul double %a, %x
  %c = fcmp olt double %m, %x
  %r = select i1 %c, double %m, double %x
  ret double %r
}

declare float @sinf(float)

; CHECK-LABEL: define float @libm(
; CHECK: call float @sinf(float %x)
define float @libm(float %x) {
  %r = call float @sinf(float %x)
  ret float %r
}

; calls within the module are fine
define float @local(float %x) {
  ret float %x
}

; CHECK-LABEL: define float @calls_local(
; CHECK: call float @local(float %x)
define float @calls_local(float %x) {
  %r = call float @local(float %x)
  ret float %r
}
