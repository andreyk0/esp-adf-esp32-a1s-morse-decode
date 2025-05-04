#include "morse.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <esp_check.h> // For ESP_RETURN_ON_FALSE checks
#include <esp_err.h>
#include <esp_log.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ook_adaptive_threshold.h"

static const char *TAG = "MORSE";

// pulses shorted than this will be merged with the longer pulse, value is in units of time/sample
static const int32_t E_THRES = 500;

// carry and combine very short pulses
static int32_t carry = 0;

// keep ook range for display, larger is better
static uint32_t range = 0;

// Queue of decoded edge transitions, uint32_t elements
static QueueHandle_t morse_ook_queue;

esp_err_t morse_init() {
  morse_ook_queue = xQueueCreate(16, sizeof(uint32_t));
  ESP_RETURN_ON_FALSE(morse_ook_queue != NULL, ESP_ERR_INVALID_STATE, TAG, "failed to create queue");

  BaseType_t task_created =
      xTaskCreate(morse_sample_handler_task, "MorseHandler", configMINIMAL_STACK_SIZE * 4, NULL, 5, NULL);

  if (task_created != pdPASS) {
    ESP_LOGE(TAG, "Failed to create morse sample handler task");
    vQueueDelete(morse_ook_queue);
    return ESP_ERR_INVALID_STATE;
  }

  ESP_LOGI(TAG, "Initialization complete");
  return ESP_OK;
}

void morse_sample_handler_task(void *pvParameters) {
  int32_t e;
  while (1) {
    if (xQueueReceive(morse_ook_queue, &e, portMAX_DELAY) == pdTRUE) {
      ESP_LOGI(TAG, "E: %d / R: %u", (int)e, (unsigned int)range);
    }
  }
}

esp_err_t morse_sample(int32_t e, uint32_t r) {
  range = r;

  int32_t abse = abs(e);

  if (abse < E_THRES) {
    if (carry > 0) {
      carry += abse;
    } else {
      carry -= abse;
    }

    return ESP_OK;
  } else {
    if (carry != 0) {
      xQueueSend(morse_ook_queue, (void *)&carry, (TickType_t)0);
      carry = 0;
    }

    xQueueSend(morse_ook_queue, (void *)&e, (TickType_t)0);
  }

  return ESP_OK;
}
