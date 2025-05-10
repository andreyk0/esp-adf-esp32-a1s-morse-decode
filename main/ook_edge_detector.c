/**
 * @file ook_edge_detector.c
 * @brief Implementation of the OOK edge detector.
 */
#include "ook_edge_detector.h"
#include "ook_adaptive_threshold.h"
#include <assert.h>    // For assert() to catch NULL pointers in update
#include <esp_check.h> // For ESP_RETURN_ON_FALSE checks
#include <esp_log.h>   // For ESP logging
#include <limits.h>    // For INT32_MAX, INT32_MIN, UINT32_MAX

// Define a logging tag for this module
static const char *TAG = "OOKE";

// --- Function Implementations ---

esp_err_t ook_edge_detector_init(ook_edge_detector_t *edge_state, ook_adaptive_threshold_t *threshold_state,
                                 int16_t initial_sample) {

  edge_state->threshold_state = threshold_state;
  edge_state->samples_in_state = 0; // Start counter at 0, update will increment to 1

  // Determine initial state based on the first sample
  // Note: Threshold state might not be fully adapted yet, but this sets a starting point.
  // Update threshold state *before* getting the initial threshold
  ook_adaptive_threshold_update(edge_state->threshold_state, initial_sample);
  int16_t initial_threshold = ook_adaptive_threshold_get(edge_state->threshold_state);
  edge_state->below_threshold = (initial_sample <= initial_threshold); // Use <= for below to match '>' check later

  ESP_LOGD(TAG, "Initialized: Initial sample=%d, Initial threshold=%d, Starting state=%s", initial_sample,
           initial_threshold, edge_state->below_threshold ? "Below" : "Above");

  // Initialize counter after determining state
  edge_state->samples_in_state = 1; // First sample counts as 1 sample in the initial state

  return ESP_OK;
}

int32_t ook_edge_detector_update(ook_edge_detector_t *edge_state, int16_t sample) {
  // 1. Update the adaptive threshold first
  ook_adaptive_threshold_update(edge_state->threshold_state, sample);

  // 2. Get the current threshold
  int16_t current_threshold_positive_edge = ook_adaptive_threshold_get_positive_edge(edge_state->threshold_state);
  int16_t current_threshold_negative_edge = ook_adaptive_threshold_get_negative_edge(edge_state->threshold_state);

  // 3. Determine the new state based on the current sample
  bool new_state_is_positive = (sample >= current_threshold_positive_edge);
  bool new_state_is_negative = (sample <= current_threshold_negative_edge);

  bool is_rising_edge = new_state_is_positive && edge_state->below_threshold;
  bool is_falling_edge = new_state_is_negative && (!edge_state->below_threshold);

  // 4. Check if the state has changed (edge detected)
  if (is_rising_edge || is_falling_edge) {
    // --- Edge Detected ---
    int32_t return_value = 0; // Changed to int32_t
    uint32_t samples_in_previous_state = edge_state->samples_in_state;

    // Clamp the count to fit within int32_t range before assigning polarity
    int32_t clamped_count;                       // Changed to int32_t
    if (samples_in_previous_state > INT32_MAX) { // Check against INT32_MAX
      clamped_count = INT32_MAX;                 // Clamp to INT32_MAX
      ESP_LOGW(TAG, "Sample count (%lu) exceeded INT32_MAX, clamping.", (unsigned long)samples_in_previous_state);
    } else {
      clamped_count = (int32_t)samples_in_previous_state; // Cast to int32_t
    }

    if (is_falling_edge) {
      // Falling edge (was above, now below)
      // Check if clamped_count is INT32_MAX to avoid overflow when negating
      if (clamped_count == INT32_MAX) {
        return_value = INT32_MIN; // Assign INT32_MIN directly if count was maxed out
        ESP_LOGW(TAG, "Falling edge duration clamped to INT32_MIN");
      } else {
        return_value = -clamped_count; // Negative value indicates falling edge
      }
      ESP_LOGV(TAG, "Falling edge detected after %ld samples above.", (long)clamped_count); // Use %ld for int32_t

      edge_state->below_threshold = true;
    } else if (is_rising_edge) {
      // Rising edge (was below, now above)
      return_value = clamped_count; // Positive value indicates rising edge
      ESP_LOGV(TAG, "Rising edge detected after %ld samples below.", (long)clamped_count); // Use %ld for int32_t

      edge_state->below_threshold = false;
    }

    edge_state->samples_in_state = 1; // Start count for the new state
    return return_value;
  } else {
    // --- No Edge Detected ---
    // Increment sample counter for the current state
    // Check for potential overflow of the uint32_t counter
    if (edge_state->samples_in_state < UINT32_MAX) {
      edge_state->samples_in_state++;
    } else {
      // Handle extremely rare overflow case (e.g., reset or log)
      ESP_LOGW(TAG, "Sample counter overflowed (UINT32_MAX reached)!");
      // Optionally reset or saturate: edge_state->samples_in_state = 1;
    }
    return 0; // No edge
  }
}
