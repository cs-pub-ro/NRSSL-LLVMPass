// Linear SVM (hinge loss, L2 regularization) trained with the Pegasos SGD
// schedule, step 1/(lambda t), on the iris data: setosa (+1) against
// versicolor (-1), samples 0..99 in order. Reports w, b and the number of
// correctly classified training samples.

#include "caial.h"
#include "iris.h"

#define EPOCHS 10
#define NS 100

int main(void) {
  fp_t w[IRIS_FEATURES] = {FP_C(0.0)};
  fp_t b = FP_C(0.0);
  const fp_t lambda = FP_C(0.01);

  int t = 0;
  for (int ep = 0; ep < EPOCHS; ep++) {
    for (int s = 0; s < NS; s++) {
      t++;
      fp_t eta = FP_C(1.0) / (lambda * (fp_t)t);
      fp_t y = iris_y[s] == 0 ? FP_C(1.0) : FP_C(-1.0);
      fp_t margin = b;
      for (int f = 0; f < IRIS_FEATURES; f++)
        margin = margin + w[f] * iris_x[s][f];
      margin = y * margin;

      fp_t shrink = FP_C(1.0) - eta * lambda;
      for (int f = 0; f < IRIS_FEATURES; f++) {
        w[f] = shrink * w[f];
        if (margin < FP_C(1.0))
          w[f] = w[f] + eta * y * iris_x[s][f];
      }
      if (margin < FP_C(1.0))
        b = b + eta * y;
    }
    for (int f = 0; f < IRIS_FEATURES; f++)
      CAIAL_TRACE_VALUE("w", ep * IRIS_FEATURES + f, w[f]);
    CAIAL_TRACE_VALUE("b", ep, b);
  }

  long correct = 0;
  for (int s = 0; s < NS; s++) {
    fp_t m = b;
    for (int f = 0; f < IRIS_FEATURES; f++)
      m = m + w[f] * iris_x[s][f];
    correct += (FP_C(0.0) < m) == (iris_y[s] == 0);
  }

  CAIAL_RESULTS(w, w, IRIS_FEATURES);
  CAIAL_RESULT(b, b);
  CAIAL_IRESULT(correct, correct);
  return 0;
}
