/**
 * @file ook_adaptive_threshold.c
 * @brief Implementation of adaptive thresholding for OOK demodulation.
 */
#include "ook_adaptive_threshold.h"
#include <assert.h>    // For assert() to catch NULL pointers in update/get
#include <esp_check.h> // For ESP_RETURN_ON_FALSE checks
#include <esp_log.h>   // For ESP logging
#include <math.h>
#include <stdint.h>
#include <stdlib.h> // For abs, potentially needed if decay logic changes, though not strictly now

// Define a logging tag for this module
static const char *TAG = "OOKT";

// Helper macro for integer min/max (stdlib.h doesn't have standard int min/max)
#define INT32_MINIMUM(a, b) (((a) < (b)) ? (a) : (b))
#define INT32_MAXIMUM(a, b) (((a) > (b)) ? (a) : (b))

// --- Function Implementations ---

esp_err_t ook_adaptive_threshold_init(ook_adaptive_threshold_t *state, uint32_t decay_step, uint32_t min_range) {
  ESP_RETURN_ON_FALSE(state != NULL, ESP_ERR_INVALID_ARG, TAG, "State pointer cannot be NULL");
  ESP_RETURN_ON_FALSE(decay_step > 0, ESP_ERR_INVALID_ARG, TAG, "Decay step must be positive");
  ESP_RETURN_ON_FALSE(min_range > 2 * decay_step, ESP_ERR_INVALID_ARG, TAG, "Invalid min range");

  state->current_min = -1;
  state->current_max = 1;
  state->decay_step = decay_step;
  state->min_range = min_range;

  ESP_LOGI(TAG, "Initialized: min=%ld, max=%ld, step=%lu, min_range=%lu", (long int)state->current_min,
           (long int)state->current_max, (long unsigned int)state->decay_step, (long unsigned int)state->min_range);
  return ESP_OK;
}

void ook_adaptive_threshold_update(ook_adaptive_threshold_t *state, int32_t sample) {
  // 1. Update min/max bounds with the new sample
  state->current_min = INT32_MINIMUM(state->current_min, sample);
  state->current_max = INT32_MAXIMUM(state->current_max, sample);

  // 2. Apply additive decay - move min and max towards the midpoint
  // Check if max > min to avoid issues if they become equal/inverted
  // Only decay if the range is larger than twice the step size to prevent
  // crossing
  if (state->current_max > state->current_min &&
      ((uint32_t)state->current_max - state->current_min) >= state->min_range) {

    // Move max or min only, to avoid skewing midpoint too much when decoding a run of samples in a particular state
    if (sample > ook_adaptive_threshold_get(state)) {
      // Decrease max towards midpoint, but don't overshoot min
      state->current_max -= state->decay_step;
    } else {
      // Increase min towards midpoint, but don't overshoot max
      state->current_min += state->decay_step;
    }

    // shouldn't happen
    if (state->current_min > state->current_max) {
      state->current_max = state->current_min + state->min_range + state->decay_step;
      ESP_LOGV(TAG, "Min/Max crossed during decay, reset to %d", state->current_min);
    }
  }
}

inline int32_t ook_adaptive_threshold_get(const ook_adaptive_threshold_t *state) {
  // Threshold is the midpoint of the current estimated range
  // Use safer calculation to prevent potential overflow if min+max > INT32_MAX
  // midpoint = min + (max - min) / 2
  return state->current_min + (state->current_max - state->current_min) / 2;
}

// "edge" functions add a bit of a hysteresis to threshold transitions,
// adding them on the lower side because higher side has more variation due to signal fading in and out
inline int32_t ook_adaptive_threshold_get_positive_edge(const ook_adaptive_threshold_t *state) {
  return ook_adaptive_threshold_get(state);
}

// "edge" functions add a bit of a hysteresis to threshold transitions,
// adding them on the lower side because higher side has more variation due to signal fading in and out
inline int32_t ook_adaptive_threshold_get_negative_edge(const ook_adaptive_threshold_t *state) {
  int32_t range = state->current_max - state->current_min;
  return state->current_min + range / 4;
}
