// A single sigmoid neuron (logistic regression) trained with full-batch
// gradient descent on the iris data to tell versicolor (1) from virginica (0),
// the two classes that overlap. Reports the weights (bias last), the mean
// squared error and the number of correctly classified training samples.

#include "caial.h"
#include "caial_math.h"
#include "iris.h"

#define EPOCHS 30
#define FIRST 50 // samples 50..149
#define NS 100
#define NW (IRIS_FEATURES + 1)

int main(void) {
  fp_t w[NW] = {FP_C(0.0)};
  fp_t mse = FP_C(0.0);

  for (int ep = 0; ep < EPOCHS; ep++) {
    fp_t grad[NW] = {FP_C(0.0)};
    mse = FP_C(0.0);
    for (int s = FIRST; s < FIRST + NS; s++) {
      fp_t z = w[IRIS_FEATURES];
      for (int f = 0; f < IRIS_FEATURES; f++)
        z = z + w[f] * iris_x[s][f];
      fp_t y = fp_sigmoid(z);
      fp_t target = iris_y[s] == 1 ? FP_C(1.0) : FP_C(0.0);
      fp_t err = y - target;
      mse = mse + err * err;
      // d/dz of the squared error through the sigmoid
      fp_t delta = err * y * (FP_C(1.0) - y);
      for (int f = 0; f < IRIS_FEATURES; f++)
        grad[f] = grad[f] + delta * iris_x[s][f];
      grad[IRIS_FEATURES] = grad[IRIS_FEATURES] + delta;
    }
    mse = mse / (fp_t)NS;
    for (int f = 0; f < NW; f++)
      w[f] = w[f] - FP_C(0.5) * grad[f] / (fp_t)NS;
    CAIAL_TRACE_VALUE("mse", ep, mse);
  }

  long correct = 0;
  for (int s = FIRST; s < FIRST + NS; s++) {
    fp_t z = w[IRIS_FEATURES];
    for (int f = 0; f < IRIS_FEATURES; f++)
      z = z + w[f] * iris_x[s][f];
    int pred = FP_C(0.5) < fp_sigmoid(z);
    correct += pred == (iris_y[s] == 1);
  }

  CAIAL_RESULTS(w, w, NW);
  CAIAL_RESULT(mse, mse);
  CAIAL_IRESULT(correct, correct);
  return 0;
}
