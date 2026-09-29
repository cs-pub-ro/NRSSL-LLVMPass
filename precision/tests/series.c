// Slowly converging sums, where the terms get small next to the partial sum:
//   harmonic      H_n = sum 1/k
//   basel         sum 1/k^2 -> pi^2/6
//   alt_harmonic  sum (-1)^(k+1)/k -> ln 2

#include "caial.h"

#define N 1000

int main(void) {
  fp_t harmonic = FP_C(0.0);
  fp_t basel = FP_C(0.0);
  fp_t alt = FP_C(0.0);
  fp_t sign = FP_C(1.0);

  for (int k = 1; k <= N; k++) {
    fp_t x = (fp_t)k;
    harmonic = harmonic + FP_C(1.0) / x;
    basel = basel + FP_C(1.0) / (x * x);
    alt = alt + sign / x;
    sign = -sign;
    CAIAL_TRACE_VALUE("harmonic", k, harmonic);
    CAIAL_TRACE_VALUE("basel", k, basel);
    CAIAL_TRACE_VALUE("alt_harmonic", k, alt);
  }

  CAIAL_RESULT(harmonic, harmonic);
  CAIAL_RESULT(basel, basel);
  CAIAL_RESULT(alt_harmonic, alt);

  // H_1000 has no closed form, this is the exact partial sum
  CAIAL_EXACT(harmonic, 0, "7.48547086055034491265651820433");
  CAIAL_EXACT(basel, 0, "1.64493406684822643647241516665");
  CAIAL_EXACT(alt_harmonic, 0, "0.693147180559945309417232121458");
  return 0;
}
