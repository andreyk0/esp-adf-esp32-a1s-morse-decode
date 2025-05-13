#include "audio_dsp.h" // Include our header

#include "audio_element.h"
#include "audio_mem.h"
#include "esp_err.h"
#include "esp_log.h"
#include <dsp_common.h>
#include <dsps_biquad.h>
#include <dsps_biquad_gen.h>
#include <dsps_d_gen.h>
#include <dsps_fft2r.h>
#include <dsps_view.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "morse.h"
#include "ook_edge_detector.h"

static const char *TAG = "AUD";

typedef struct audio_dsp {
  uint32_t cnt;
  float coeffs_bpf[AUDIO_DSP_FILTER_LEN];
  float coeffs_lpf_envelope[AUDIO_DSP_FILTER_LEN];
  float wfb[AUDIO_DSP_FILTER_LEN];
  float wfe[AUDIO_DSP_FILTER_LEN];

  ook_edge_detector_t ook_edge;
} audio_dsp_t;

static float smax = -MAXFLOAT / 2;
static float smin = MAXFLOAT / 2;

/**
 * Audio DSP.
 * Reads stereo samples, passes one channel through for reference/debugging.
 * Second channels is processed through
 *   BPF(750Hz) =>
 *   Envelope detector =>
 *   LPF =>
 *   Rescaling => Audio output (for debugging)
 *             => OOK edge detector => Morse decoder
 *
 */
static int _dsp_process(audio_element_handle_t self, char *in_buffer, int in_len) {
  __attribute__((aligned(16))) static float input[AUDIO_DSP_N_SAMPLES];
  __attribute__((aligned(16))) static float output[AUDIO_DSP_N_SAMPLES];

  audio_dsp_t *mod = (audio_dsp_t *)audio_element_getdata(self);

  int r_size = audio_element_input(self, in_buffer, in_len); // Read data

  // Handle errors or EOF/DONE from input first
  if (r_size <= 0) {
    if (r_size == AEL_IO_TIMEOUT) {
      ESP_LOGD(TAG, "Input timeout, continue processing");
      return ESP_OK;                                           // Timeout is okay, the element task should continue
                                                               // running
    } else if (r_size == AEL_IO_DONE || r_size == AEL_IO_OK) { // AEL_IO_OK (0) can indicate DONE
      ESP_LOGI(TAG, "Input stream ended (AEL_IO_DONE received: %d)", r_size);
      // Signal DONE downstream. Important to pass NULL buffer and 0 length.
      audio_element_output(self, NULL, 0);
      // Return ESP_OK to indicate the element finished its work cleanly,
      // the framework will handle stopping the task appropriately.
      return ESP_OK;
    } else {
      // Handle actual input errors
      ESP_LOGE(TAG, "Error reading from input ringbuffer: %d", r_size);
      // Report error state to the pipeline listener
      audio_element_report_status(self, AEL_STATUS_ERROR_INPUT);
      // Return ESP_FAIL to stop the element task immediately
      return ESP_FAIL;
    }
  }

  mod->cnt++;

  // If we got here, r_size > 0, process the audio data
  // Assuming 16-bit signed integer samples
  int16_t *samples = (int16_t *)in_buffer;
  int num_samples = r_size / sizeof(int16_t);
  int num_samples_filter = num_samples / 2; // 1 channel only

  /* ESP_LOGW(TAG, "cnt: %ul, num_samples: %d, in_len: %d", (unsigned
   * int)mod->cnt, */
  /*          num_samples, in_len); */

  if (num_samples_filter > AUDIO_DSP_N_SAMPLES) {
    return ESP_FAIL;
  }

  for (int i = 0; i < num_samples_filter; i++) {
    input[i] = (float)samples[i * 2];
  }

  // BPF
  ESP_ERROR_CHECK(dsps_biquad_f32(input, output, num_samples_filter, mod->coeffs_bpf, mod->wfb));

  // Envelope
  for (int i = 0; i < num_samples_filter; i++) {
    input[i] = fabs(output[i]);
  }

  // LPF over envelope
  ESP_ERROR_CHECK(dsps_biquad_f32(input, output, num_samples_filter, mod->coeffs_lpf_envelope,
                                  mod->wfe)); // memcpy(output, input, num_samples_filter * sizeof(float));

  // Shrinking min/max to account for signal fade in/out
  smax = smax - 0.010 * fabs(smax);
  smin = smin + 0.010 * fabs(smin);

  for (int i = 0; i < num_samples_filter; i++) {
    if (smin > output[i]) {
      smin = output[i];
    }
    if (smax < output[i]) {
      smax = output[i];
    }
  }

  if (smin >= smax) {
    smin = smax - 0.1;
  }

  float range = smax - smin;
  float scale = range / (float)UINT32_MAX;

  // ESP_LOGV(TAG, "Smin: %0.3f, Smax: %0.3f, Range: %0.3f, Scale: %0.7f", smin, smax, range, scale);

  // Convert back to stereo output and run OOK decoder
  for (int i = 0; i < num_samples_filter; i++) {
    uint32_t s = (uint32_t)fmin((float)(UINT32_MAX - UINT32_MAX / 128), fmax(0, (output[i] - smin) / scale));
    samples[i * 2] = (s >> 16) + INT16_MIN;

    int32_t e = ook_edge_detector_update(&mod->ook_edge, s);

    if (e != 0) {
      ESP_ERROR_CHECK(morse_sample(e, range));
    }
  }

  // Write the modified data to the output ringbuffer
  int w_size = audio_element_output(self, in_buffer, r_size);

  // Handle output results
  if (w_size == r_size) {
    // Successfully wrote all data
    return w_size;
  } else if (w_size == AEL_IO_TIMEOUT) {
    ESP_LOGW(TAG, "Output ringbuffer timeout (wrote %d/%d bytes)", w_size, r_size);
    // Output buffer likely full. Element should wait and retry, which
    // returning ESP_OK effectively does (the element's task loop will call
    // process again). The output_wait_time in cfg helps prevent this from
    // happening too often.
    return ESP_OK;
  } else {
    // Handle actual output errors (w_size < 0 but not AEL_IO_TIMEOUT, or w_size
    // < r_size)
    ESP_LOGE(TAG, "Error writing to output ringbuffer: %d (expected %d)", w_size, r_size);
    // Report error state to the pipeline listener
    audio_element_report_status(self, AEL_STATUS_ERROR_OUTPUT);
    // Return ESP_FAIL to stop the element task immediately
    return ESP_FAIL;
  }
}

/**
 * @brief Open function for the audio element (called when pipeline starts).
 */
static esp_err_t _dsp_open(audio_element_handle_t self) {
  ESP_LOGI(TAG, "Dsp element opened");
  return ESP_OK;
}

/**
 * @brief Close function for the audio element (called when pipeline
 * stops/terminates).
 */
static esp_err_t _dsp_close(audio_element_handle_t self) {
  ESP_LOGI(TAG, "Dsp element closed");
  audio_dsp_t *mod = (audio_dsp_t *)audio_element_getdata(self);
  ESP_LOGI(TAG, "Dsp CNT: %ul", (unsigned int)mod->cnt);

  // Check status, might be useful for debugging why it closed
  if (audio_element_is_stopping(self)) {
    ESP_LOGI(TAG, "Element closing due to stop command");
  }
  // Note: If the _process function returns ESP_FAIL, the task exits,
  // and _close might be called after the task has already stopped.
  return ESP_OK;
}

/**
 * @brief Destroy function for the audio element (called when element is
 * deinitialized).
 */
static esp_err_t _dsp_destroy(audio_element_handle_t self) {
  ESP_LOGD(TAG, "Dsp element destroying");

  audio_dsp_t *mod = (audio_dsp_t *)audio_element_getdata(self);

  if (mod) {
    audio_free(mod);
  }
  ESP_LOGD(TAG, "Dsp element destroyed");
  return ESP_OK;
}

/**
 * @brief Initialization function for the audio dsp element.
 */
audio_element_handle_t audio_dsp_init(audio_dsp_cfg_t *config) {
  // Allocate memory for the element's specific data
  audio_dsp_t *mod = audio_calloc(1, sizeof(audio_dsp_t));
  AUDIO_MEM_CHECK(TAG, mod, {
    ESP_LOGE(TAG, "Failed to allocate memory for audio_dsp_t");
    return NULL;
  });

  ook_edge_detector_init(&mod->ook_edge);

  // Init filters
  // 44100 750Hz
  ESP_ERROR_CHECK(dsps_biquad_gen_bpf_f32(mod->coeffs_bpf, 0.017, 20.0f));
  ESP_ERROR_CHECK(dsps_biquad_gen_lpf_f32(mod->coeffs_lpf_envelope, 0.00050, 0.707f));

  // Basic audio element configuration
  audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
  cfg.open = _dsp_open;
  cfg.close = _dsp_close;
  cfg.process = _dsp_process;
  cfg.destroy = _dsp_destroy;
  cfg.tag = "dsp"; // Element tag for logging
  cfg.task_stack = config->task_stack;
  cfg.task_prio = config->task_prio;
  cfg.task_core = config->task_core;
  cfg.out_rb_size = config->out_rb_size;
  cfg.buffer_len = AUDIO_DSP_BUF_SIZE;

  // Initialize the base audio element
  audio_element_handle_t el = audio_element_init(&cfg);
  AUDIO_MEM_CHECK(TAG, el, {
    ESP_LOGE(TAG, "Failed to initialize audio element");
    audio_free(mod);
    return NULL;
  });

  // Store the pointer to our specific data structure
  audio_element_setdata(el, mod);

  audio_element_info_t info = {0};
  audio_element_setinfo(el, &info);

  ESP_LOGD(TAG, "Audio DSP element initialized successfully");
  return el;
}
