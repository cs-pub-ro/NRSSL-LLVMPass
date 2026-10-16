// Gaussian naive Bayes on the iris data: per-class mean and variance of every
// feature, then classify a few query points by the product of the gaussian
// likelihoods (the 1/sqrt(2 pi) factor is the same for all classes and dropped).

#include "caial.h"
#include "caial_math.h"
#include "iris.h"

#define NCLASSES 3
#define NQUERIES 4

static const fp_t queries[NQUERIES][IRIS_FEATURES] = {
    {FP_C(5.4), FP_C(3.7), FP_C(1.5), FP_C(0.2)},
    {FP_C(6.1), FP_C(2.9), FP_C(4.5), FP_C(1.5)},
    {FP_C(6.3), FP_C(2.8), FP_C(5.0), FP_C(1.7)},
    {FP_C(7.0), FP_C(3.1), FP_C(6.0), FP_C(2.2)},
};

static fp_t mean[NCLASSES][IRIS_FEATURES], var[NCLASSES][IRIS_FEATURES];

int main(void) {
  for (int c = 0; c < NCLASSES; c++) {
    for (int f = 0; f < IRIS_FEATURES; f++) {
      fp_t sum = FP_C(0.0);
      for (int s = 0; s < IRIS_N; s++)
        if (iris_y[s] == c)
          sum = sum + iris_x[s][f];
      mean[c][f] = sum / FP_C(50.0);

      fp_t sq = FP_C(0.0);
      for (int s = 0; s < IRIS_N; s++) {
        if (iris_y[s] != c)
          continue;
        fp_t d = iris_x[s][f] - mean[c][f];
        sq = sq + d * d;
      }
      var[c][f] = sq / FP_C(50.0);
    }
  }

  long pred[NQUERIES];
  fp_t likelihood[NQUERIES];
  for (int q = 0; q < NQUERIES; q++) {
    int best = 0;
    fp_t best_p = FP_C(0.0);
    for (int c = 0; c < NCLASSES; c++) {
      fp_t p = FP_C(1.0);
      for (int f = 0; f < IRIS_FEATURES; f++) {
        fp_t d = queries[q][f] - mean[c][f];
        p = p * fp_exp(-(d * d) / (FP_C(2.0) * var[c][f])) / fp_sqrt(var[c][f]);
      }
      CAIAL_TRACE_VALUE("p", q * NCLASSES + c, p);
      if (c == 0 || best_p < p) {
        best = c;
        best_p = p;
      }
    }
    pred[q] = best;
    likelihood[q] = best_p;
  }

  CAIAL_RESULTS(mean, &mean[0][0], NCLASSES * IRIS_FEATURES);
  CAIAL_RESULTS(var, &var[0][0], NCLASSES * IRIS_FEATURES);
  CAIAL_RESULTS(likelihood, likelihood, NQUERIES);
  CAIAL_IRESULTS(class, pred, NQUERIES);

  // exact values on the decimal data
  static const char *mean_exact[] = {"5.006", "3.418", "1.464", "0.244", "5.936", "2.77",
                                     "4.26",  "1.326", "6.588", "2.974", "5.552", "2.026"};
  static const char *var_exact[] = {"0.121764", "0.142276", "0.029504", "0.011264", "0.261104", "0.0965",
                                    "0.2164",   "0.038324", "0.396256", "0.101924", "0.298496", "0.073924"};
  static const char *likelihood_exact[] = {"1.49559936696101516418e+02", "3.54995730788183649906e+01",
                                           "7.60700399003176652712e+00", "1.45622600255667791203e+01"};
  for (int i = 0; i < NCLASSES * IRIS_FEATURES; i++) {
    CAIAL_EXACT(mean, i, mean_exact[i]);
    CAIAL_EXACT(var, i, var_exact[i]);
  }
  for (int i = 0; i < NQUERIES; i++)
    CAIAL_EXACT(likelihood, i, likelihood_exact[i]);
  return 0;
}
