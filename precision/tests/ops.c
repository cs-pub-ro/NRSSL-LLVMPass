// One test per NRS instruction on a small table of inputs, replaces the old
// SimpleFPUTest. Float results are compared to IEEE and the exact value,
// integer results (compares, float->int) should match IEEE exactly.

#include "caial.h"

#define NPAIRS 6

static const fp_t in_a[NPAIRS] = {FP_C(1.5), FP_C(-4.0), FP_C(0.001), FP_C(7.0), FP_C(0.1), FP_C(123.456)};
static const fp_t in_b[NPAIRS] = {FP_C(2.75), FP_C(0.5), FP_C(3000.0), FP_C(-7.0), FP_C(0.2), FP_C(-0.0078125)};
static const int in_int[] = {0, 1, -1, 1000, -1000, 1234567, -16777217};

// volatile so that nothing gets folded or hoisted out of the loops
static volatile fp_t va, vb;
static volatile int vi;

static void exact(const char *name, const char *const *v, int n) {
  for (int i = 0; i < n; i++)
    printf("@exact %s %d %s\n", name, i, v[i]);
}

int main(void) {
  fp_t add[NPAIRS], sub[NPAIRS], mul[NPAIRS], div[NPAIRS], min[NPAIRS], max[NPAIRS];
  fp_t sqrt[NPAIRS], neg[NPAIRS], abs[NPAIRS];
  long lt[NPAIRS], le[NPAIRS], eq[NPAIRS], gt[NPAIRS], ge[NPAIRS], ne[NPAIRS], to_int[NPAIRS];

  for (int i = 0; i < NPAIRS; i++) {
    va = in_a[i];
    vb = in_b[i];
    fp_t a = va, b = vb;
    add[i] = a + b;
    sub[i] = a - b;
    mul[i] = a * b;
    div[i] = a / b;
    min[i] = a < b ? a : b;
    max[i] = a < b ? b : a;
    sqrt[i] = fp_sqrt(fp_abs(a));
    neg[i] = -a;
    abs[i] = fp_abs(a);
    lt[i] = a < b;
    le[i] = a <= b;
    eq[i] = a == b;
    gt[i] = a > b;
    ge[i] = a >= b;
    ne[i] = a != b;
    to_int[i] = (int)(a * FP_C(10.0));
  }

  fp_t from_int[CAIAL_ARRAY_SIZE(in_int)];
  for (int i = 0; i < CAIAL_ARRAY_SIZE(in_int); i++) {
    vi = in_int[i];
    from_int[i] = (fp_t)vi;
  }

  CAIAL_RESULTS(add, add, NPAIRS);
  CAIAL_RESULTS(sub, sub, NPAIRS);
  CAIAL_RESULTS(mul, mul, NPAIRS);
  CAIAL_RESULTS(div, div, NPAIRS);
  CAIAL_RESULTS(min, min, NPAIRS);
  CAIAL_RESULTS(max, max, NPAIRS);
  CAIAL_RESULTS(sqrt, sqrt, NPAIRS);
  CAIAL_RESULTS(neg, neg, NPAIRS);
  CAIAL_RESULTS(abs, abs, NPAIRS);
  CAIAL_RESULTS(from_int, from_int, CAIAL_ARRAY_SIZE(in_int));
  CAIAL_IRESULTS(lt, lt, NPAIRS);
  CAIAL_IRESULTS(le, le, NPAIRS);
  CAIAL_IRESULTS(eq, eq, NPAIRS);
  CAIAL_IRESULTS(gt, gt, NPAIRS);
  CAIAL_IRESULTS(ge, ge, NPAIRS);
  CAIAL_IRESULTS(ne, ne, NPAIRS);
  CAIAL_IRESULTS(to_int, to_int, NPAIRS);

  // exact values of the operations on the decimal inputs
  static const char *const add_exact[] = {"4.25", "-3.5", "3000.001", "0", "0.3", "123.4481875"};
  static const char *const sub_exact[] = {"-1.25", "-4.5", "-2999.999", "14", "-0.1", "123.4638125"};
  static const char *const mul_exact[] = {"4.125", "-2", "3", "-49", "0.02", "-0.9645"};
  static const char *const div_exact[] = {"0.545454545454545454545454545455", "-8", "0.000000333333333333333333333333333333", "-1", "0.5", "-15802.368"};
  static const char *const min_exact[] = {"1.5", "-4", "0.001", "-7", "0.1", "-0.0078125"};
  static const char *const max_exact[] = {"2.75", "0.5", "3000", "7", "0.2", "123.456"};
  static const char *const sqrt_exact[] = {"1.22474487139158904909864203735", "2", "0.0316227766016837933199889354443", "2.64575131106459059050161575364", "0.316227766016837933199889354443", "11.1110755554986664846214940412"};
  static const char *const neg_exact[] = {"-1.5", "4", "-0.001", "-7", "-0.1", "-123.456"};
  static const char *const abs_exact[] = {"1.5", "4", "0.001", "7", "0.1", "123.456"};
  static const char *const from_int_exact[] = {"0", "1", "-1", "1000", "-1000", "1234567", "-16777217"};
  exact("add", add_exact, CAIAL_ARRAY_SIZE(add_exact));
  exact("sub", sub_exact, CAIAL_ARRAY_SIZE(sub_exact));
  exact("mul", mul_exact, CAIAL_ARRAY_SIZE(mul_exact));
  exact("div", div_exact, CAIAL_ARRAY_SIZE(div_exact));
  exact("min", min_exact, CAIAL_ARRAY_SIZE(min_exact));
  exact("max", max_exact, CAIAL_ARRAY_SIZE(max_exact));
  exact("sqrt", sqrt_exact, CAIAL_ARRAY_SIZE(sqrt_exact));
  exact("neg", neg_exact, CAIAL_ARRAY_SIZE(neg_exact));
  exact("abs", abs_exact, CAIAL_ARRAY_SIZE(abs_exact));
  exact("from_int", from_int_exact, CAIAL_ARRAY_SIZE(from_int_exact));
  return 0;
}
