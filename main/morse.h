#ifndef MORSE_H_
#define MORSE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"


esp_err_t morse_init();

void morse_sample_handler_task(void *pvParameters);


/** Sample OOK edge.
 *  @param e edge, sign represents transition +/- for positive/negative, absolute value is the number of samples (pulse duration in units of time/sample)
 *  @param range OOK threshold effective range, the bigger the better, for debugging / display
 */
esp_err_t morse_sample(int32_t e, uint32_t range);

#endif // MORSE_H_
