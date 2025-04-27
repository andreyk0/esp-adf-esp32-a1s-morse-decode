/**
 * @file ook_adaptive_threshold.c
 * @brief Implementation of adaptive thresholding for OOK demodulation.
 */
#include "ook_adaptive_threshold.h"
#include <assert.h>    // For assert() to catch NULL pointers in update/get
#include <esp_check.h> // For ESP_RETURN_ON_FALSE checks
#include <esp_log.h>   // For ESP logging
#include <math.h>
#include <stdlib.h> // For abs, potentially needed if decay logic changes, though not strictly now

// Define a logging tag for this module
static const char *TAG = "ook_thresh";

// Helper macro for integer min/max (stdlib.h doesn't have standard int min/max)
#define INT16_MINIMUM(a, b) (((a) < (b)) ? (a) : (b))
#define INT16_MAXIMUM(a, b) (((a) > (b)) ? (a) : (b))

// --- Function Implementations ---

esp_err_t ook_adaptive_threshold_init(ook_adaptive_threshold_t *state, uint16_t decay_step) {
  // Check for NULL pointer
  ESP_RETURN_ON_FALSE(state != NULL, ESP_ERR_INVALID_ARG, TAG, "State pointer cannot be NULL");

  // Check for invalid decay step
  ESP_RETURN_ON_FALSE(decay_step > 0, ESP_ERR_INVALID_ARG, TAG, "Decay step must be positive");

  state->current_min = -1;
  state->current_max = 1;
  state->decay_step = decay_step;

  ESP_LOGI(TAG, "Initialized: min=%d, max=%d, step=%u", state->current_min, state->current_max, state->decay_step);
  return ESP_OK;
}

void ook_adaptive_threshold_update(ook_adaptive_threshold_t *state, int16_t sample) {
  // 1. Update min/max bounds with the new sample
  state->current_min = INT16_MINIMUM(state->current_min, sample);
  state->current_max = INT16_MAXIMUM(state->current_max, sample);

  // 2. Apply additive decay - move min and max towards the midpoint
  // Check if max > min to avoid issues if they become equal/inverted
  // Only decay if the range is larger than twice the step size to prevent
  // crossing
  if (state->current_max > state->current_min && (state->current_max - state->current_min) >= state->decay_step) {
    // Move max or min only, to avoid skewing midpoint too much when decoding a run of samples in a particular state
    if (sample > ook_adaptive_threshold_get(state)) {
      // Decrease max towards midpoint, but don't overshoot min
      state->current_max -= state->decay_step;
    } else {
      // Increase min towards midpoint, but don't overshoot max
      state->current_min += state->decay_step;
    }

    // Sanity check to prevent crossing due to large step or precision issues
    // (though less likely with int) This check might be redundant given the
    // condition above, but adds safety.
    if (state->current_min > state->current_max) {
      // If they crossed, set them to the midpoint (average)
      // Use the safer midpoint calculation to avoid overflow: min + (max - min)
      // / 2 Note: We need the original values before decay for this average.
      // Simpler recovery: just set them equal if they crossed.
      state->current_min = state->current_max = state->current_min - state->decay_step; // Revert min step
      ESP_LOGV(TAG, "Min/Max crossed during decay, reset to %d", state->current_min);
    }
  } else if (state->current_max > state->current_min &&
             (state->current_max - state->current_min) < (2 * state->decay_step)) {
    // If the range is too small to apply the full decay step without crossing,
    // just move them to the midpoint directly.
    int16_t midpoint = state->current_min + (state->current_max - state->current_min) / 2;
    state->current_min = midpoint;
    state->current_max = midpoint;
    ESP_LOGV(TAG, "Range too small for decay step, set min/max to midpoint %d", midpoint);
  }

  // Optional: Add verbose log here to see values evolve
  // ESP_LOGV(TAG, "Update: s=%d -> min=%d max=%d", sample, state->current_min,
  // state->current_max);
}

inline int16_t ook_adaptive_threshold_get(const ook_adaptive_threshold_t *state) {
  // Threshold is the midpoint of the current estimated range
  // Use safer calculation to prevent potential overflow if min+max > INT16_MAX
  // midpoint = min + (max - min) / 2
  return state->current_min + (state->current_max - state->current_min) / 2;
}
