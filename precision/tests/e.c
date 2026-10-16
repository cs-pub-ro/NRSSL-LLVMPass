// e = sum 1/k!, k = 0..N-1

#include "caial.h"

#define N 20

int main(void) {
  fp_t e = FP_C(1.0);
  fp_t fact = FP_C(1.0);
  for (int k = 1; k < N; k++) {
    fact = fact * (fp_t)k;
    e = e + FP_C(1.0) / fact;
    CAIAL_TRACE_VALUE("e", k, e);
  }

  CAIAL_RESULT(e, e);
  CAIAL_EXACT(e, 0, "2.71828182845904523536");
  return 0;
}
