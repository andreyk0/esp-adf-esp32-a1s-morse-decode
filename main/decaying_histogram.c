#include "decaying_histogram.h"

#include "esp_err.h"
#include "esp_log.h"

#include <float.h> // For FLT_MAX
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "HIST";

esp_err_t decaying_histogram_init(decaying_histogram_t *hist, int32_t min_val, int32_t max_val, uint32_t num_bins,
                                  uint32_t num_bins_signal_spread, float decay_exponent) {
  if (hist == NULL || num_bins == 0 || decay_exponent <= 0.0f || decay_exponent >= 1.0f || min_val >= max_val) {
    return ESP_ERR_INVALID_ARG;
  }

  hist->bins = (float *)calloc(num_bins, sizeof(float));
  if (hist->bins == NULL) {
    return ESP_ERR_NO_MEM;
  }

  hist->min_val = min_val;
  hist->max_val = max_val;
  hist->num_bins = num_bins;
  hist->num_bins_signal_spread = num_bins_signal_spread;
  hist->decay_exponent = decay_exponent;
  hist->bin_width = (float)(max_val - min_val) / num_bins;

  return ESP_OK;
}

void decaying_histogram_dump(const decaying_histogram_t *hist) {
  for (uint32_t i = 0; i < hist->num_bins; i++) {
    float bval = hist->min_val + i * hist->bin_width;
    if (hist->bins[i] > 0.1) {
      ESP_LOGI(TAG, "v[%d]=%.1f\t%.1f", (int)i, hist->bins[i], bval);
    }
  }

  int32_t minidx;
  int32_t maxidx;

  ESP_ERROR_CHECK(decaying_histogram_get_min_max_bins(hist, &minidx, &maxidx));
  ESP_LOGI(TAG, "minidx=%d\tmaxidx=%d", (int)minidx, (int)maxidx);

  int32_t minv;
  int32_t maxv;

  decaying_histogram_get_min_max_values(hist, &minv, &maxv);
  ESP_LOGI(TAG, "min=%d\tmax=%d", (int)minv, (int)maxv);
}

inline void decaying_histogram_decay(decaying_histogram_t *hist) {
  if (hist == NULL || hist->bins == NULL) {
    return;
  }

  for (uint32_t i = 0; i < hist->num_bins; i++) {
    hist->bins[i] *= hist->decay_exponent;
  }
}

void decaying_histogram_add_sample(decaying_histogram_t *hist, int32_t sample) {
  if (hist == NULL || hist->bins == NULL) {
    return;
  }

  decaying_histogram_decay(hist);

  // Calculate the bin index for the new sample
  if (sample >= hist->min_val && sample <= hist->max_val) {
    int32_t bin_index = (int32_t)((float)(sample - hist->min_val) / hist->bin_width);
    if (bin_index >= 0 && bin_index < hist->num_bins) {
      hist->bins[bin_index]++;
    }
  }
}

int32_t decaying_histogram_get_threshold(const decaying_histogram_t *hist) {
  int32_t minv;
  int32_t maxv;

  decaying_histogram_get_min_max_values(hist, &minv, &maxv);

  return minv + (maxv - minv) / 2;
}

void decaying_histogram_get_min_max_values(const decaying_histogram_t *hist, int32_t *minv, int32_t *maxv) {
  int32_t min_bin_index;
  int32_t max_bin_index;

  ESP_ERROR_CHECK(decaying_histogram_get_min_max_bins(hist, &min_bin_index, &max_bin_index));

  *minv = hist->min_val + min_bin_index * hist->bin_width;
  *maxv = hist->min_val + max_bin_index * hist->bin_width;
}

esp_err_t decaying_histogram_get_min_max_bins(const decaying_histogram_t *hist, int32_t *min_bin_index,
                                              int32_t *max_bin_index) {
  if (hist == NULL || hist->bins == NULL || min_bin_index == NULL || max_bin_index == NULL || hist->num_bins == 0) {
    return ESP_ERR_INVALID_ARG;
  }

  // Find 2 maximums, assuming spread of values is constrained to a few bins around actual max.
  // The distribution should have 2 peaks but they can be of unequal value due
  // to the application of decay and fiffering sequences of repeated symbols.

  float count = -FLT_MAX;
  int32_t max1_idx = 0;

  for (int i = 0; i < hist->num_bins; i++) {
    if (hist->bins[i] > count) {
      count = hist->bins[i];
      max1_idx = i;
    }
  }

  int32_t max2_idx = 0;
  int32_t excluded_lower_idx = max1_idx - hist->num_bins_signal_spread;
  int32_t excluded_upper_idx = max1_idx + hist->num_bins_signal_spread;
  count = -FLT_MAX;

  for (int i = 0; i < hist->num_bins; i++) {
    if (hist->bins[i] > count && (i < excluded_lower_idx || i > excluded_upper_idx)) {
      count = hist->bins[i];
      max2_idx = i;
    }
  }

  if (max1_idx > max2_idx) {
    *max_bin_index = max1_idx;
    *min_bin_index = max2_idx;
  } else {
    *max_bin_index = max2_idx;
    *min_bin_index = max1_idx;
  }

  return ESP_OK;
}

void decaying_histogram_free(decaying_histogram_t *hist) {
  if (hist != NULL && hist->bins != NULL) {
    free(hist->bins);
    hist->bins = NULL;
  }
}
