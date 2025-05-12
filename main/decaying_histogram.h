#ifndef DECAYING_HISTOGRAM_H_
#define DECAYING_HISTOGRAM_H_

#include "esp_err.h"

#include <float.h> // For FLT_MAX
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  float *bins;
  int32_t min_val;
  int32_t max_val;

  // total number of bins
  int num_bins;

  // Number of bins around max that a signal is assumed to spread into due to jitter,
  // these will be excluded when looking for a 2nd max.
  int num_bins_signal_spread;

  float decay_exponent;
  float bin_width;
} decaying_histogram_t;

esp_err_t decaying_histogram_init(decaying_histogram_t *hist, int32_t min_val, int32_t max_val, uint32_t num_bins,
                                  uint32_t num_bins_signal_spread, float decay_exponent);

void decaying_histogram_dump(const decaying_histogram_t *hist);

void decaying_histogram_decay(decaying_histogram_t *hist);

void decaying_histogram_add_sample(decaying_histogram_t *hist, int32_t sample);

int32_t decaying_histogram_get_threshold(const decaying_histogram_t *hist);

void decaying_histogram_get_min_max_values(const decaying_histogram_t *hist, int32_t *minv, int32_t *maxv);

esp_err_t decaying_histogram_get_min_max_bins(const decaying_histogram_t *hist, int32_t *min_bin_index,
                                              int32_t *max_bin_index);

void decaying_histogram_free(decaying_histogram_t *hist);

#endif // DECAYING_HISTOGRAM_H_
