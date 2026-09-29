// Three series for pi:
//   Leibniz     pi = 4 * sum (-1)^k / (2k+1)
//   Nilakantha  pi = 3 + 4/(2*3*4) - 4/(4*5*6) + 4/(6*7*8) - ...
//   Viete       2/pi = sqrt(1/2) * sqrt(1/2 + 1/2 sqrt(1/2)) * ...

#include "caial.h"

#define LEIBNIZ_N 1000
#define NILAKANTHA_N 200
#define VIETE_N 20

static fp_t leibniz(void) {
  fp_t sum = FP_C(0.0);
  fp_t sign = FP_C(1.0);
  for (int k = 0; k < LEIBNIZ_N; k++) {
    sum = sum + sign / (fp_t)(2 * k + 1);
    sign = -sign;
    CAIAL_TRACE_VALUE("leibniz", k, sum);
  }
  return FP_C(4.0) * sum;
}

static fp_t nilakantha(void) {
  fp_t pi = FP_C(3.0);
  fp_t sign = FP_C(1.0);
  fp_t i = FP_C(2.0);
  for (int k = 0; k < NILAKANTHA_N; k++) {
    pi = pi + sign * FP_C(4.0) / (i * (i + FP_C(1.0)) * (i + FP_C(2.0)));
    sign = -sign;
    i = i + FP_C(2.0);
    CAIAL_TRACE_VALUE("nilakantha", k, pi);
  }
  return pi;
}

static fp_t viete(void) {
  fp_t a = FP_C(0.0);
  fp_t prod = FP_C(1.0);
  for (int k = 0; k < VIETE_N; k++) {
    a = fp_sqrt(FP_C(2.0) + a);
    prod = prod * a / FP_C(2.0);
    CAIAL_TRACE_VALUE("viete", k, prod);
  }
  return FP_C(2.0) / prod;
}

int main(void) {
  CAIAL_RESULT(leibniz, leibniz());
  CAIAL_RESULT(nilakantha, nilakantha());
  CAIAL_RESULT(viete, viete());

  CAIAL_EXACT(leibniz, 0, "3.14159265358979323846");
  CAIAL_EXACT(nilakantha, 0, "3.14159265358979323846");
  CAIAL_EXACT(viete, 0, "3.14159265358979323846");
  return 0;
}
