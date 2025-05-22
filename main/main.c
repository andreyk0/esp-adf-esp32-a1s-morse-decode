#include "audio_dsp.h"
#include "audio_pipeline.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "i2s_stream.h"

#include "configure_es8388.h"
#include "lcd.h"
#include "leds.h"
#include "morse.h"

static const char *TAG = "MAIN";

void app_main(void) {
  // esp_log_level_set("*", ESP_LOG_INFO);
  // esp_log_level_set(TAG, ESP_LOG_DEBUG);

  leds_init();
  lcd_init();

  ESP_ERROR_CHECK(morse_init());

  audio_pipeline_handle_t pipeline;
  audio_element_handle_t i2s_stream_writer, i2s_stream_reader, audio_dsp_el;

  ESP_LOGI(TAG, "Start codec chip");
  configure_es8388();
  // ESP_ERROR_CHECK(es8388_pa_power(true)); // enable speaker amp power

  ESP_LOGI(TAG, "Create audio pipeline for playback");
  audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
  pipeline = audio_pipeline_init(&pipeline_cfg);

  ESP_LOGI(TAG, "Create i2s stream to write data to codec chip");
  i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
  i2s_cfg.type = AUDIO_STREAM_WRITER;
  i2s_stream_writer = i2s_stream_init(&i2s_cfg);

  ESP_LOGI(TAG, "Create i2s stream to read data from codec chip");
  i2s_stream_cfg_t i2s_cfg_read = I2S_STREAM_CFG_DEFAULT();
  i2s_cfg_read.type = AUDIO_STREAM_READER;
  i2s_stream_reader = i2s_stream_init(&i2s_cfg_read);

  ESP_LOGI(TAG, "Create audio dsp element");
  audio_dsp_cfg_t dsp_cfg = DEFAULT_AUDIO_DSP_CONFIG();
  audio_dsp_el = audio_dsp_init(&dsp_cfg);
  mem_assert(audio_dsp_el);

  ESP_LOGI(TAG, "Register all elements to audio pipeline");
  audio_pipeline_register(pipeline, i2s_stream_reader, "i2s_read");
  audio_pipeline_register(pipeline, audio_dsp_el, "dsp");
  audio_pipeline_register(pipeline, i2s_stream_writer, "i2s_write");

  ESP_LOGI(TAG, "Link elements: "
                "[codec]-->i2s_read-->audio_dsp-->i2s_write-->[codec]");
  // Define the tags for linking in the correct order
  const char *link_tag[3] = {"i2s_read", "dsp", "i2s_write"};
  audio_pipeline_link(pipeline, &link_tag[0], 3);

  ESP_LOGI(TAG, "Set up  event listener");
  audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
  audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

  ESP_LOGI(TAG, "Listening event from all elements of pipeline");
  audio_pipeline_set_listener(pipeline, evt);

  ESP_LOGI(TAG, "Start audio_pipeline");
  audio_pipeline_run(pipeline);

  ESP_LOGI(TAG, "Listen for all pipeline events");
  while (1) {
    audio_event_iface_msg_t msg;
    esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
      continue;
    }

    /* Stop when the last pipeline element (i2s_stream_writer in this case)
     * receives stop event */
    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *)i2s_stream_writer &&
        msg.cmd == AEL_MSG_CMD_REPORT_STATUS &&
        (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED))) {
      ESP_LOGW(TAG, "[ * ] Stop event received");
      break;
    }
  }

  ESP_LOGI(TAG, "Stop audio_pipeline");
  audio_pipeline_stop(pipeline);
  audio_pipeline_wait_for_stop(pipeline);
  audio_pipeline_terminate(pipeline);

  audio_pipeline_unregister(pipeline, i2s_stream_reader);
  audio_pipeline_unregister(pipeline, audio_dsp_el);
  audio_pipeline_unregister(pipeline, i2s_stream_writer);

  /* Terminate the pipeline before removing the listener */
  audio_pipeline_remove_listener(pipeline);

  /* Make sure audio_pipeline_remove_listener &
   * audio_event_iface_remove_listener are called before destroying event_iface
   */
  audio_event_iface_destroy(evt);

  /* Release all resources */
  audio_pipeline_deinit(pipeline);
  audio_element_deinit(i2s_stream_reader);
  audio_element_deinit(i2s_stream_writer);
}
