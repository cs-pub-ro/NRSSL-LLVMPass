// Polynomial evaluation with Horner's scheme:
//   poly  (x-1)^7 expanded, near x = 1 the terms cancel almost completely
//   sin   Taylor series of sin(x) up to x^17, nested form

#include "caial.h"

static const fp_t poly_x[] = {FP_C(0.9), FP_C(0.95), FP_C(0.99), FP_C(0.999),
                              FP_C(1.001), FP_C(1.01), FP_C(1.05), FP_C(1.1)};

// x^7 - 7x^6 + 21x^5 - 35x^4 + 35x^3 - 21x^2 + 7x - 1
static const fp_t poly_c[] = {FP_C(1.0),  FP_C(-7.0), FP_C(21.0), FP_C(-35.0),
                              FP_C(35.0), FP_C(-21.0), FP_C(7.0), FP_C(-1.0)};

static const fp_t sin_x[] = {FP_C(0.5), FP_C(1.0), FP_C(2.0), FP_C(3.0)};

// sin x = x (1 - x^2/(2*3) (1 - x^2/(4*5) (1 - ...)))
static fp_t taylor_sin(fp_t x) {
  fp_t x2 = x * x;
  fp_t s = FP_C(1.0);
  for (int k = 8; k >= 1; k--)
    s = FP_C(1.0) - x2 / (fp_t)(2 * k * (2 * k + 1)) * s;
  return x * s;
}

int main(void) {
  fp_t poly[CAIAL_ARRAY_SIZE(poly_x)];
  for (int i = 0; i < CAIAL_ARRAY_SIZE(poly_x); i++) {
    fp_t p = poly_c[0];
    for (int j = 1; j < CAIAL_ARRAY_SIZE(poly_c); j++) {
      p = p * poly_x[i] + poly_c[j];
      CAIAL_TRACE_VALUE("poly", i * 8 + j, p);
    }
    poly[i] = p;
  }
  CAIAL_RESULTS(poly, poly, CAIAL_ARRAY_SIZE(poly_x));

  static const char *poly_exact[] = {"-1e-7", "-7.8125e-10", "-1e-14", "-1e-21",
                                     "1e-21", "1e-14", "7.8125e-10", "1e-7"};
  for (int i = 0; i < CAIAL_ARRAY_SIZE(poly_exact); i++)
    CAIAL_EXACT(poly, i, poly_exact[i]);

  fp_t sin[CAIAL_ARRAY_SIZE(sin_x)];
  for (int i = 0; i < CAIAL_ARRAY_SIZE(sin_x); i++)
    sin[i] = taylor_sin(sin_x[i]);
  CAIAL_RESULTS(sin, sin, CAIAL_ARRAY_SIZE(sin_x));

  static const char *sin_exact[] = {"0.47942553860420300027", "0.84147098480789650665",
                                    "0.90929742682568169540", "0.14112000805986722210"};
  for (int i = 0; i < CAIAL_ARRAY_SIZE(sin_exact); i++)
    CAIAL_EXACT(sin, i, sin_exact[i]);
  return 0;
}
