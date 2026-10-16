// Accumulating many small terms:
//   naive      sum of 0.1, 10000 times
//   kahan      the same with compensated (Kahan) summation
//   telescope  sum 1/k - 1/(k+1), k=1..1000 = 1 - 1/1001, with cancellation in every term

#include "caial.h"

#define N 10000

int main(void) {
  fp_t naive = FP_C(0.0);
  fp_t sum = FP_C(0.0), c = FP_C(0.0);
  for (int k = 0; k < N; k++) {
    naive = naive + FP_C(0.1);

    fp_t y = FP_C(0.1) - c;
    fp_t t = sum + y;
    c = (t - sum) - y;
    sum = t;

    if (k % 100 == 99) {
      CAIAL_TRACE_VALUE("naive", k, naive);
      CAIAL_TRACE_VALUE("kahan", k, sum);
    }
  }
  CAIAL_RESULT(naive, naive);
  CAIAL_RESULT(kahan, sum);
  CAIAL_EXACT(naive, 0, "1000");
  CAIAL_EXACT(kahan, 0, "1000");

  fp_t tel = FP_C(0.0);
  for (int k = 1; k <= 1000; k++) {
    fp_t x = (fp_t)k;
    tel = tel + (FP_C(1.0) / x - FP_C(1.0) / (x + FP_C(1.0)));
    CAIAL_TRACE_VALUE("telescope", k, tel);
  }
  CAIAL_RESULT(telescope, tel);
  CAIAL_EXACT(telescope, 0, "0.999000999000999000999000999001");
  return 0;
}
