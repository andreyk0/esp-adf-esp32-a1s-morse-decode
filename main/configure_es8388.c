#include "configure_es8388.h"

#include "board.h"
#include "esp_err.h"
#include "esp_log.h"
#include "i2c_bus.h"
#include <es8388.h>
#include <string.h>

static const char *TAG = "ES8388";
static i2c_bus_handle_t i2c_handle;

static int i2c_init();
static esp_err_t es8388_write(uint8_t reg_add, uint8_t data);

/**
 * Configures the ES8388 codec for LineIn -> ADC -> I2S -> DAC -> LineOut.
 */
void configure_es8388(void) {
  ESP_LOGI(TAG, "Configuring ES8388 for ADC->I2S->DAC path (no bypass)...");
  i2c_init();

  // --- Power Management and Basic Setup ---
  ESP_ERROR_CHECK(es8388_write(ES8388_CONTROL1, 0x12));

  // Configure I2S clocking mode
  ESP_ERROR_CHECK(es8388_write(ES8388_MASTERMODE,
                               0x00)); // Set ES8388 to I2S Slave mode (MSC=0)

  // Power up essential analog blocks
  ESP_ERROR_CHECK(es8388_write(
      ES8388_CONTROL2, 0x00)); // Power up analog blocks, ibiasgen, VrefBuf

  // Power up digital sections and references
  ESP_ERROR_CHECK(es8388_write(ES8388_CHIPPOWER,
                               0x00)); // Power up digital ADC/DAC, ADC/DAC Vref

  ESP_ERROR_CHECK(es8388_write(0x35, 0xA0));
  ESP_ERROR_CHECK(es8388_write(0x37, 0xD0));
  ESP_ERROR_CHECK(es8388_write(0x39, 0xD0));

  // Power up ADC specific blocks
  ESP_ERROR_CHECK(es8388_write(
      ES8388_ADCPOWER,
      0x08)); // Power up ADC analog inputs (AINL/R), core (ADCL/R), biasgen

  // Power up DAC specific blocks and enable outputs
  ESP_ERROR_CHECK(
      es8388_write(ES8388_DACPOWER,
                   0x0F)); // Power up DAC core (DACL/R), enable outputs

  // --- ADC Configuration ---
  // Select physical input pins
  ESP_ERROR_CHECK(es8388_write(
      ES8388_ADCCONTROL2,
      0x50)); // Select LINPUT2 for Left ADC, RINPUT2 for Right ADC
              // 0x00)); // Select LINPUT1 for Left ADC, RINPUT1 for Right ADC

  ESP_ERROR_CHECK(es8388_write(ES8388_ADCCONTROL10,
                               0xF8)); // ALC gain

  ESP_ERROR_CHECK(es8388_write(ES8388_ADCCONTROL11,
                               0xF0)); // ALC target

  // Set ADC I2S format and word length
  ESP_ERROR_CHECK(
      es8388_write(ES8388_ADCCONTROL4,
                   0x0C)); // ADC: 16-bit I2S format (ADCFORMAT=00, ADCWL=011)
                           // Use 0x00 for 24-bit, 0x18 for 32-bit etc.

  // Set ADC digital volume
  ESP_ERROR_CHECK(es8388_write(ES8388_ADCCONTROL8,
                               0x00)); // ADC Left Volume 0dB (LADCVOL=0)
  ESP_ERROR_CHECK(es8388_write(ES8388_ADCCONTROL9,
                               0x00)); // ADC Right Volume 0dB (RADCVOL=0)

  // --- DAC Configuration ---
  // Set DAC I2S format and word length (should match ADC)
  ESP_ERROR_CHECK(
      es8388_write(ES8388_DACCONTROL1,
                   0x18)); // DAC: 16-bit I2S format (DACFORMAT=00, DACWL=011)
                           // Use 0x00 for 24-bit, 0x30 for 32-bit etc.
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL2,
                               0x02)); // DAC: single speed 256 MCLK;

  // Set DAC digital volume
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL4,
                               0x00)); // DAC Left Volume 0dB (LDACVOL=0)
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL5,
                               0x00)); // DAC Right Volume 0dB (RDACVOL=0)

  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL12,
                               0x99)); // ALC attack/decay
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL13,
                               0x1F)); // ALC mode, samples

  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL16,
                               0x24)); // DAC LMIXSEL / RMIXSEL

  // --- Output Mixer Configuration (Disable Bypass) ---
  // Configure Left Output Mixer (mixL)
  ESP_ERROR_CHECK(es8388_write(
      ES8388_DACCONTROL17,
      0x80)); // Left Mixer: Connect DACL (LD2LO=1), DISCONNECT LineIn (LI2LO=0)

  // Configure Right Output Mixer (mixR)
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL20,
                               0x80)); // Right Mixer: Connect DACR (RD2RO=1),
                                       // DISCONNECT LineIn (RI2RO=0)

  // Set Analog Output Volume (LOUT1/ROUT1)
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL24,
                               0x1E)); // LOUT1 Volume 0dB (011110b)
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL25,
                               0x1E)); // ROUT1 Volume 0dB (011110b)

  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL26,
                               0x1E)); // LOUT1 Volume 0dB (011110b)
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL27,
                               0x1E)); // ROUT1 Volume 0dB (011110b)

  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL21,
                               0x80)); // set internal ADC and DAC use the same
                                       // LRCK clock, ADC LRCK as internal LRCK


  /* es8388 PA gpio_config */
  gpio_config_t  io_conf;
  memset(&io_conf, 0, sizeof(io_conf));
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = BIT64(get_pa_enable_gpio());
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 0;
  gpio_config(&io_conf);
  es8388_pa_power(false); // enable explicitly as needed

  ESP_LOGI("ES8388_CONFIG", "ES8388 configuration complete.");
}

static int i2c_init() {
  i2c_config_t es_i2c_cfg = {.mode = I2C_MODE_MASTER,
                             .sda_pullup_en = GPIO_PULLUP_ENABLE,
                             .scl_pullup_en = GPIO_PULLUP_ENABLE,
                             .master.clk_speed = 40000};
  ESP_ERROR_CHECK(get_i2c_pins(I2C_NUM_0, &es_i2c_cfg));
  i2c_handle = i2c_bus_create(I2C_NUM_0, &es_i2c_cfg);
  return ESP_OK;
}

esp_err_t es8388_write(uint8_t reg_add, uint8_t data) {
  return i2c_bus_write_bytes(i2c_handle, ES8388_ADDR, &reg_add, sizeof(reg_add),
                             &data, sizeof(data));
}
