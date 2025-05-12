/**
 * @file ook_adaptive_threshold.h
 * @brief Adaptive thresholding for OOK demodulation using int32_t and additive decay.
 */
#ifndef OOK_ADAPTIVE_THRESHOLD_H
#define OOK_ADAPTIVE_THRESHOLD_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Structure to hold the state of the adaptive threshold algorithm
typedef struct {
  int32_t current_min; // Current estimated minimum level ('off' level)
  int32_t current_max; // Current estimated maximum level ('on' level)
  uint32_t decay_step; // Additive amount to decay min/max towards midpoint per update
  uint32_t min_range;  // min distance between max and min
} ook_adaptive_threshold_t;

/**
 * @brief Initializes the adaptive threshold state.
 *
 * @param[out] state Pointer to the ook_adaptive_threshold_t struct to initialize. Must not be NULL.
 * @param[in] decay_step Positive additive step for decaying the range each update (e.g., 1 or 2).
 *
 * @return
 * - ESP_OK on success
 * - ESP_ERR_INVALID_ARG if state is NULL, decay_step is 0, or initial_min >= initial_max.
 */
esp_err_t ook_adaptive_threshold_init(ook_adaptive_threshold_t *state, uint32_t decay_step, uint32_t min_range);

/**
 * @brief Updates the adaptive threshold state with a new sample.
 */
void ook_adaptive_threshold_update(ook_adaptive_threshold_t *state, int32_t sample);

/**
 * @brief Returns current adaptive threshold value.
 */
int32_t ook_adaptive_threshold_get(const ook_adaptive_threshold_t *state);

int32_t ook_adaptive_threshold_get_positive_edge(const ook_adaptive_threshold_t *state);

int32_t ook_adaptive_threshold_get_negative_edge(const ook_adaptive_threshold_t *state);

#ifdef __cplusplus
}
#endif

#endif // OOK_ADAPTIVE_THRESHOLD_H
