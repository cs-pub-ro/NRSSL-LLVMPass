// Converging sequences:
//   newton     a_{n+1} = (a_n + 2/a_n) / 2          -> sqrt(2)
//   fibonacci  F_{n+1}/F_n                          -> golden ratio
//   rational   a_n = (3n^2 - 1) / (n + 2n^2), n=100 -> 29999/20100

#include "caial.h"

int main(void) {
  fp_t a = FP_C(1.0);
  for (int n = 0; n < 10; n++) {
    a = (a + FP_C(2.0) / a) / FP_C(2.0);
    CAIAL_TRACE_VALUE("newton", n, a);
  }
  CAIAL_RESULT(newton, a);
  CAIAL_EXACT(newton, 0, "1.41421356237309504880168872421");

  // F_36 and up need more than 24 bits, so the last terms get rounded in float.
  // F_41/F_40 is within 1e-16 of the golden ratio.
  fp_t prev = FP_C(1.0), cur = FP_C(1.0), ratio = FP_C(1.0);
  for (int n = 0; n < 40; n++) {
    fp_t next = prev + cur;
    prev = cur;
    cur = next;
    ratio = cur / prev;
    CAIAL_TRACE_VALUE("fibonacci", n, ratio);
  }
  CAIAL_RESULT(fibonacci, ratio);
  CAIAL_EXACT(fibonacci, 0, "1.61803398874989484820458683437");

  fp_t i = FP_C(1.0), r = FP_C(0.0);
  for (int n = 1; n <= 100; n++) {
    r = (FP_C(3.0) * i * i - FP_C(1.0)) / (i + FP_C(2.0) * i * i);
    i = i + FP_C(1.0);
    CAIAL_TRACE_VALUE("rational", n, r);
  }
  CAIAL_RESULT(rational, r);
  CAIAL_EXACT(rational, 0, "1.49248756218905472636815920398");
  return 0;
}
