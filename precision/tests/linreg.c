// Linear regression on the iris data: predict petal width from the other three
// features, full-batch gradient descent on the mean squared error.
// Reports the weights (bias last) and the final loss.

#include "caial.h"
#include "iris.h"

#define EPOCHS 200
#define NW IRIS_FEATURES // 3 features + bias

int main(void) {
  fp_t w[NW] = {FP_C(0.0)};
  fp_t loss = FP_C(0.0);

  for (int ep = 0; ep < EPOCHS; ep++) {
    fp_t grad[NW] = {FP_C(0.0)};
    loss = FP_C(0.0);
    for (int s = 0; s < IRIS_N; s++) {
      const fp_t *x = iris_x[s];
      fp_t pred = w[3];
      for (int f = 0; f < 3; f++)
        pred = pred + w[f] * x[f];
      fp_t err = pred - x[3];
      loss = loss + err * err;
      for (int f = 0; f < 3; f++)
        grad[f] = grad[f] + err * x[f];
      grad[3] = grad[3] + err;
    }
    loss = loss / (fp_t)IRIS_N;
    for (int f = 0; f < NW; f++)
      w[f] = w[f] - FP_C(0.01) * grad[f] / (fp_t)IRIS_N;
    CAIAL_TRACE_VALUE("loss", ep, loss);
  }

  CAIAL_RESULTS(w, w, NW);
  CAIAL_RESULT(loss, loss);
  return 0;
}
