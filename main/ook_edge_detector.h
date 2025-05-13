/**
 * @file ook_edge_detector.h
 * @brief Detects edges (state changes) based on an adaptive OOK threshold.
 */
#ifndef OOK_EDGE_DETECTOR_H
#define OOK_EDGE_DETECTOR_H

#include "esp_err.h" // For esp_err_t return codes
#include <stdbool.h>
#include <stdint.h>

// Structure to hold the state of the edge detector
typedef struct {
  bool below_threshold;      // Current state: true if last sample was below threshold
  uint32_t samples_in_state; // Counter for consecutive samples in the current state
} ook_edge_detector_t;

/**
 * @brief Initializes the OOK edge detector state.
 *
 * @param[out] edge_state Pointer to the ook_edge_detector_t struct to initialize. Must not be NULL.
 *
 * @return
 * - ESP_OK on success
 * - ESP_ERR_INVALID_ARG if edge_state or threshold_state is NULL.
 */
esp_err_t ook_edge_detector_init(ook_edge_detector_t *edge_state);

/**
 * @brief Updates the edge detector with a new sample and detects edges.
 *
 * @param[in,out] edge_state Pointer to the initialized ook_edge_detector_t struct. Must not be NULL.
 * @param[in] sample The new input sample, normalized to fill full uint32_t range
 *
 * @return int32_t
 * - Positive value: Rising edge detected. Value is the count of samples previously below threshold (clamped to
 * INT32_MAX).
 * - Negative value: Falling edge detected. Value is the negative count of samples previously above threshold (clamped
 * to INT32_MIN).
 * - 0: No edge detected in this sample.
 */
int32_t ook_edge_detector_update(ook_edge_detector_t *edge_state, uint32_t sample);

#endif // OOK_EDGE_DETECTOR_H
