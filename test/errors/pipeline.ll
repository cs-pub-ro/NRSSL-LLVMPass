; Converting twice would convert the constants twice, the second run refuses.
; A missing or unknown format is rejected when the pipeline is parsed.
; RUN: not %opt_nrs -passes='nrs-convert<posit1>,nrs-convert<posit1>' -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=TWICE
; RUN: not %opt_nrs -passes='nrs-convert<posit3>' -disable-output %s 2>&1 | FileCheck %s --check-prefix=UNKNOWN
; RUN: not %opt_nrs -passes='nrs-convert' -disable-output %s 2>&1 | FileCheck %s --check-prefix=MISSING

; TWICE: error: nrs-convert: the module was already converted to posit1
; UNKNOWN: unknown NRS format 'posit3', expected one of: posit1, posit2, morris, morrisHeb, morrisBiasHeb, morrisUnaryHeb
; MISSING: unknown NRS format '', expected one of:

define float @f(float %x) {
  %r = fadd float %x, 1.0
  ret float %r
}
