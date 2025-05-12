/**
 * @file ook_edge_detector.h
 * @brief Detects edges (state changes) based on an adaptive OOK threshold.
 */
#ifndef OOK_EDGE_DETECTOR_H
#define OOK_EDGE_DETECTOR_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h" // For esp_err_t return codes
#include "ook_adaptive_threshold.h" // Include the threshold module header

#ifdef __cplusplus
extern "C" {
#endif

// Structure to hold the state of the edge detector
typedef struct
{
    ook_adaptive_threshold_t *threshold_state; // Reference to the adaptive threshold state
    bool below_threshold;                      // Current state: true if last sample was below threshold
    uint32_t samples_in_state;                 // Counter for consecutive samples in the current state
} ook_edge_detector_t;

/**
 * @brief Initializes the OOK edge detector state.
 *
 * @param[out] edge_state Pointer to the ook_edge_detector_t struct to initialize. Must not be NULL.
 * @param[in] threshold_state Pointer to an already initialized ook_adaptive_threshold_t struct. Must not be NULL.
 * @param[in] initial_sample An initial sample value used to determine the starting state (above/below threshold).
 *
 * @return
 * - ESP_OK on success
 * - ESP_ERR_INVALID_ARG if edge_state or threshold_state is NULL.
 */
esp_err_t ook_edge_detector_init(ook_edge_detector_t *edge_state, ook_adaptive_threshold_t *threshold_state, int32_t initial_sample);

/**
 * @brief Updates the edge detector with a new sample and detects edges.
 *
 * This function performs the following steps:
 * 1. Updates the referenced adaptive threshold using the new sample.
 * 2. Gets the current threshold value.
 * 3. Compares the sample to the threshold to determine the new state.
 * 4. If the state changed (an edge occurred):
 * - Returns the number of samples spent in the *previous* state.
 * - Positive value for a rising edge (below -> above).
 * - Negative value for a falling edge (above -> below).
 * - Resets the sample counter.
 * - Updates the internal state.
 * 5. If the state did not change:
 * - Increments the sample counter.
 * - Returns 0.
 *
 * Assumes 'edge_state' and its referenced 'threshold_state' are valid pointers
 * (initialized via their respective init functions).
 *
 * @param[in,out] edge_state Pointer to the initialized ook_edge_detector_t struct. Must not be NULL.
 * @param[in] sample The new input sample (post-LPF), int32_t.
 *
 * @return int32_t
 * - Positive value: Rising edge detected. Value is the count of samples previously below threshold (clamped to INT32_MAX).
 * - Negative value: Falling edge detected. Value is the negative count of samples previously above threshold (clamped to INT32_MIN).
 * - 0: No edge detected in this sample.
 */
int32_t ook_edge_detector_update(ook_edge_detector_t *edge_state, int32_t sample);


#ifdef __cplusplus
}
#endif

#endif // OOK_EDGE_DETECTOR_H
