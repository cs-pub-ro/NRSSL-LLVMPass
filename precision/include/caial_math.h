// Math functions on fp_t, written with plain arithmetic so that the NRS pass
// converts them too (libm is not converted).

#ifndef CAIAL_MATH_H
#define CAIAL_MATH_H

#include "caial.h"

// exp(x): halve x until |x| <= 1/2, Taylor series, then square back up.
// Negative x is handled directly instead of as 1/exp(-x): exp(-x) for large x
// can be out of range (Morris formats overflow to NaR), exp(x) just goes to 0.
// The halving is capped, an inf/NaN input must not hang the test.
__attribute__((unused)) static fp_t fp_exp(fp_t x) {
  fp_t a = x;
  int k = 0;
  while (k < 64 && FP_C(0.5) < fp_abs(a)) {
    a = a * FP_C(0.5);
    k++;
  }

  fp_t sum = FP_C(1.0);
  fp_t term = FP_C(1.0);
  for (int n = 1; n <= 10; n++) {
    term = term * a / (fp_t)n;
    sum = sum + term;
  }
  while (k-- > 0)
    sum = sum * sum;
  return sum;
}

__attribute__((unused)) static fp_t fp_sigmoid(fp_t x) { return FP_C(1.0) / (FP_C(1.0) + fp_exp(-x)); }

#endif // CAIAL_MATH_H
