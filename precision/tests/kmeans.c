// k-means (k = 3) on the iris data, starting from samples 0, 50 and 100.
// Reports the final centroids and the cluster sizes.

#include "caial.h"
#include "iris.h"

#define K 3
#define ITERATIONS 10

static fp_t centroid[K][IRIS_FEATURES];
static int cluster[IRIS_N];

static fp_t distance2(const fp_t *a, const fp_t *b) {
  fp_t sum = FP_C(0.0);
  for (int i = 0; i < IRIS_FEATURES; i++) {
    fp_t d = a[i] - b[i];
    sum = sum + d * d;
  }
  return sum;
}

int main(void) {
  for (int c = 0; c < K; c++)
    memcpy(centroid[c], iris_x[c * 50], sizeof(centroid[c]));

  long size[K];
  for (int it = 0; it < ITERATIONS; it++) {
    for (int s = 0; s < IRIS_N; s++) {
      int best = 0;
      fp_t best_d = distance2(iris_x[s], centroid[0]);
      for (int c = 1; c < K; c++) {
        fp_t d = distance2(iris_x[s], centroid[c]);
        if (d < best_d) {
          best_d = d;
          best = c;
        }
      }
      cluster[s] = best;
    }

    for (int c = 0; c < K; c++) {
      fp_t sum[IRIS_FEATURES] = {FP_C(0.0)};
      size[c] = 0;
      for (int s = 0; s < IRIS_N; s++) {
        if (cluster[s] != c)
          continue;
        for (int f = 0; f < IRIS_FEATURES; f++)
          sum[f] = sum[f] + iris_x[s][f];
        size[c]++;
      }
      for (int f = 0; f < IRIS_FEATURES; f++) {
        if (size[c])
          centroid[c][f] = sum[f] / (fp_t)size[c];
        CAIAL_TRACE_VALUE("centroid", (it * K + c) * IRIS_FEATURES + f, centroid[c][f]);
      }
    }
  }

  CAIAL_RESULTS(centroid, &centroid[0][0], K * IRIS_FEATURES);
  CAIAL_IRESULTS(size, size, K);

  // same algorithm with exact rationals, converges to clusters of 50/62/38
  static const char *exact[K * IRIS_FEATURES] = {
      "5.006", "3.418", "1.464", "0.244",
      "5.901612903225806451612903", "2.748387096774193548387097", "4.393548387096774193548387", "1.433870967741935483870968",
      "6.85", "3.073684210526315789473684", "5.742105263157894736842105", "2.071052631578947368421053",
  };
  for (int i = 0; i < K * IRIS_FEATURES; i++)
    CAIAL_EXACT(centroid, i, exact[i]);
  return 0;
}
