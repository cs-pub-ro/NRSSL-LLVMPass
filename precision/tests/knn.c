// k-nearest neighbours (k = 7) on the iris data, squared euclidean distance.
// Classifies a few query points and reports their predicted class and the
// distances to the neighbours of the first query.

#include "caial.h"
#include "iris.h"

#define K 7
#define NQUERIES 4

static const fp_t queries[NQUERIES][IRIS_FEATURES] = {
    {FP_C(5.4), FP_C(3.7), FP_C(1.5), FP_C(0.2)},
    {FP_C(6.1), FP_C(2.9), FP_C(4.5), FP_C(1.5)},
    {FP_C(6.3), FP_C(2.8), FP_C(5.0), FP_C(1.7)},
    {FP_C(7.0), FP_C(3.1), FP_C(6.0), FP_C(2.2)},
};

static fp_t distance2(const fp_t *a, const fp_t *b) {
  fp_t sum = FP_C(0.0);
  for (int i = 0; i < IRIS_FEATURES; i++) {
    fp_t d = a[i] - b[i];
    sum = sum + d * d;
  }
  return sum;
}

// Keeps the K nearest samples sorted by distance, returns the majority class.
static int classify(const fp_t *q, fp_t dist[K]) {
  int label[K];
  for (int i = 0; i < K; i++) {
    dist[i] = FP_C(1e30);
    label[i] = -1;
  }

  for (int s = 0; s < IRIS_N; s++) {
    fp_t d = distance2(iris_x[s], q);
    int pos = K;
    while (pos > 0 && d < dist[pos - 1])
      pos--;
    if (pos == K)
      continue;
    for (int i = K - 1; i > pos; i--) {
      dist[i] = dist[i - 1];
      label[i] = label[i - 1];
    }
    dist[pos] = d;
    label[pos] = iris_y[s];
  }

  int votes[3] = {0, 0, 0};
  for (int i = 0; i < K; i++)
    votes[label[i]]++;
  int best = 0;
  for (int c = 1; c < 3; c++)
    if (votes[c] > votes[best])
      best = c;
  return best;
}

int main(void) {
  long pred[NQUERIES];
  fp_t dist[K], first[K];
  for (int q = 0; q < NQUERIES; q++) {
    pred[q] = classify(queries[q], dist);
    if (q == 0)
      memcpy(first, dist, sizeof(first));
    for (int i = 0; i < K; i++)
      CAIAL_TRACE_VALUE("dist", q * K + i, dist[i]);
  }
  CAIAL_IRESULTS(class, pred, NQUERIES);
  CAIAL_RESULTS(dist, first, K);

  // exact distances on the decimal data
  static const char *exact[K] = {"0", "0.01", "0.08", "0.09", "0.11", "0.11", "0.12"};
  for (int i = 0; i < K; i++)
    CAIAL_EXACT(dist, i, exact[i]);
  return 0;
}
