#ifndef _AUDIO_MODIFIER_H_
#define _AUDIO_MODIFIER_H_

#include "audio_element.h"
#include "audio_error.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Audio Modifier Element configurations
 */
typedef struct {
  int out_rb_size;   /*!< Size of output ringbuffer */
  int task_stack;    /*!< Task stack size */
  int task_core;     /*!< Task running core */
  int task_prio;     /*!< Task priority */
  bool extern_stack; /*!< Allocate stack on extern ram */
} audio_dsp_cfg_t;

// Default configuration values for the modifier element
#define AUDIO_MODIFIER_BUF_SIZE (2048) // Internal buffer size for processing
#define AUDIO_MODIFIER_TASK_STACK (4096)
#define AUDIO_MODIFIER_TASK_CORE (0)
#define AUDIO_MODIFIER_TASK_PRIO (5)
#define AUDIO_MODIFIER_RINGBUFFER_SIZE (8 * 1024) // Output buffer size
#define AUDIO_MODIFIER_N_SAMPLES (1024)
#define AUDIO_MODIFIER_FILTER_LEN (5)

/**
 * @brief Default configuration macro for the audio modifier element.
 */
#define DEFAULT_AUDIO_MODIFIER_CONFIG()                                        \
  {                                                                            \
      .out_rb_size = AUDIO_MODIFIER_RINGBUFFER_SIZE,                           \
      .task_stack = AUDIO_MODIFIER_TASK_STACK,                                 \
      .task_core = AUDIO_MODIFIER_TASK_CORE,                                   \
      .task_prio = AUDIO_MODIFIER_TASK_PRIO,                                   \
      .extern_stack = false,                                                   \
  }

/**
 * @brief      Create an AudioElement handle to process incoming data samples.
 * This element reads audio data, divides each sample by 2, and writes it out.
 *
 * @param      config  The configuration structure for the audio modifier
 * element.
 *
 * @return     The audio element handle, or NULL if initialization fails.
 */
audio_element_handle_t audio_dsp_init(audio_dsp_cfg_t *config);

#ifdef __cplusplus
}
#endif

#endif /* _AUDIO_MODIFIER_H_ */
