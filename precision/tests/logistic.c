// Logistic map x_{n+1} = r x_n (1 - x_n), r = 3.9 (chaotic). Rounding errors
// grow exponentially, so this shows how long each representation tracks the
// exact orbit. Reports x_10, x_20, ..., x_60.

#include "caial.h"

#define STEPS 60

int main(void) {
  fp_t x = FP_C(0.5);
  fp_t out[STEPS / 10];
  for (int n = 1; n <= STEPS; n++) {
    x = FP_C(3.9) * x * (FP_C(1.0) - x);
    CAIAL_TRACE_VALUE("x", n, x);
    if (n % 10 == 0)
      out[n / 10 - 1] = x;
  }
  CAIAL_RESULTS(x, out, STEPS / 10);

  // exact orbit for r = 3.9, x_0 = 0.5 (computed with 200 digits)
  static const char *exact[] = {
      "0.10400971326747192817", "0.32783351154932330846", "0.97284343956312305757",
      "0.56615771711443435441", "0.24134901657625967852", "0.41073520021934514186",
  };
  for (int i = 0; i < STEPS / 10; i++)
    CAIAL_EXACT(x, i, exact[i]);
  return 0;
}
