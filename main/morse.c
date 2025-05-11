#include "morse.h"

#include "char_buffer.h"
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

#include "char_buffer.h"
#include "decaying_histogram.h"
#include "morse_decoder.h"

static const char *TAG = "MORSE";

// pulses shorted than this will be merged with the longer pulse, value is in units of time/sample
static const int32_t PULSE_WIDTH_MIN = 200;
static const int32_t PULSE_WIDTH_MAX = 12000;

// keep ook range for display, larger is better
static uint32_t range = 0;

// Queue of decoded edge transitions, uint32_t elements
static QueueHandle_t morse_ook_queue;

// "dit/dah" pulse length histogram
static decaying_histogram_t dit_dah_len_his;

static char_buffer_t *dit_dah_buf = NULL;
static char_buffer_t *text_buf = NULL;

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

  ESP_ERROR_CHECK(decaying_histogram_init(&dit_dah_len_his, PULSE_WIDTH_MIN, PULSE_WIDTH_MAX, 256, 0.8f));

  dit_dah_buf = char_buffer_init(64);
  text_buf = char_buffer_init(32);

  morse_decoder_init();

  ESP_LOGI(TAG, "Initialization complete");
  return ESP_OK;
}

static void log_buffers() {
  ESP_LOGW(TAG, "%s", char_buffer_get_string(dit_dah_buf));
  ESP_LOGW(TAG, "%s", char_buffer_get_string(text_buf));
  char_buffer_reset(dit_dah_buf);
  char_buffer_reset(text_buf);
}

void morse_sample_handler_task(void *pvParameters) {
  int32_t e = 0;
  int32_t abse = 0;
  int32_t dit_th = 0;

  while (1) {
    if (xQueueReceive(morse_ook_queue, &e, portMAX_DELAY) == pdTRUE) {
      abse = abs(e);

      if (e < 0) {
        if (abse >= PULSE_WIDTH_MIN && abse < PULSE_WIDTH_MAX) {
          decaying_histogram_add_sample(&dit_dah_len_his, abse);
          dit_th = decaying_histogram_get_threshold(&dit_dah_len_his);

          if (abse > dit_th) {
            ESP_LOGI(TAG, "-");
            decode_morse_signal('-');
            char_buffer_append_char(dit_dah_buf, '-');
          } else {
            ESP_LOGI(TAG, "*");
            decode_morse_signal('*');
            char_buffer_append_char(dit_dah_buf, '*');
          }
        }
      } else {
        if (abse >= 2 * dit_th) {
          char c = decode_morse_signal(' ');

          if (c) {
            if (!char_buffer_append_char(text_buf, c)) {
              log_buffers();
              char_buffer_append_char(text_buf, c);
            }
            char_buffer_append_char(dit_dah_buf, ' ');
            ESP_LOGI(TAG, "%c", c);
          } else {
            decaying_histogram_dump(&dit_dah_len_his);
            ESP_LOGI(TAG, "? r: %u ; dit: %d", (unsigned int)range, (int)dit_th);
          }

          if (abse > 3 * dit_th) {
            log_buffers();
          }
        }
      }
    }
  }
}

esp_err_t morse_sample(int32_t e, uint32_t r) {
  xQueueSend(morse_ook_queue, (void *)&e, (TickType_t)0);
  return ESP_OK;
}
