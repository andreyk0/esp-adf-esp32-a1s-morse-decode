#include "configure_es8388.h"
#include "es8388_register_values.h"

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

  // Start

  // Step 2: Set Chip to Master / Slave Mode
  ESP_ERROR_CHECK(es8388_write(ES8388_MASTERMODE, ES8388_REG08_MASTER_MODE_CTRL));

  // Step 3: Power down DEM and STM (Initial state as per user guide)
  // User guide suggests 0xF3 for initial power down of digital blocks and state machines
  ESP_ERROR_CHECK(es8388_write(ES8388_CHIPPOWER, 0xF3));

  // Step 5: Set Chip to Play&Record Mode (Part of general chip control)
  ESP_ERROR_CHECK(es8388_write(ES8388_CONTROL1, ES8388_REG00_CHIP_CONTROL1));

  // Step 6: Power Up Analog and Ibias (Chip Control 2)
  ESP_ERROR_CHECK(es8388_write(ES8388_CONTROL2, ES8388_REG01_CHIP_CONTROL2));

  // General Chip Control: Analog Voltage Management
  ESP_ERROR_CHECK(es8388_write(ES8388_ANAVOLMANAG, ES8388_REG07_ANALOG_VOLT_MGMT));

  // General Chip Control: Chip Low Power settings
  ESP_ERROR_CHECK(es8388_write(ES8388_CHIPLOPOW1, ES8388_REG05_CHIP_LOW_POWER1));
  ESP_ERROR_CHECK(es8388_write(ES8388_CHIPLOPOW2, ES8388_REG06_CHIP_LOW_POWER2));

  // Step 4: Set same LRCK (Related to clocking)
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL21, ES8388_REG2B_DAC_CTRL21_LRCKOFFSET));

  // Step 7: Power up DAC and Enable LOUT/ROUIT
  ESP_ERROR_CHECK(es8388_write(ES8388_DACPOWER, ES8388_REG04_DAC_POWER_MGMT));

  // Step 8: Select Analog input channel for ADC
  ESP_ERROR_CHECK(es8388_write(ES8388_ADCCONTROL2, ES8388_REG0A_ADC_CTRL2_INSEL));
  // Step 9: Select Analog Input PGA Gain for ADC
  ESP_ERROR_CHECK(es8388_write(ES8388_ADCCONTROL1, ES8388_REG09_ADC_CTRL1_MICAMP));

  // Step 10: Set SFI for ADC
  ESP_ERROR_CHECK(es8388_write(ES8388_ADCCONTROL4, ES8388_REG0C_ADC_CTRL4_FORMAT));
  // Step 11: Select MCLK/ LRCK ratio for ADC
  ESP_ERROR_CHECK(es8388_write(ES8388_ADCCONTROL5, ES8388_REG0D_ADC_CTRL5_FSRATIO));

  // Step 12: Set ADC Digital Volume
  ESP_ERROR_CHECK(es8388_write(ES8388_ADCCONTROL8, ES8388_REG10_ADC_CTRL8_LVOL));
  ESP_ERROR_CHECK(es8388_write(ES8388_ADCCONTROL9, ES8388_REG11_ADC_CTRL9_RVOL));

  // Step 13: Select ALC for ADC Record
  ESP_ERROR_CHECK(es8388_write(ES8388_ADCCONTROL10, ES8388_REG12_ADC_CTRL10_ALCGAIN));
  ESP_ERROR_CHECK(es8388_write(ES8388_ADCCONTROL11, ES8388_REG13_ADC_CTRL11_ALCLVLHLD));
  ESP_ERROR_CHECK(es8388_write(ES8388_ADCCONTROL12, ES8388_REG14_ADC_CTRL12_ALCDCYATK));
  ESP_ERROR_CHECK(es8388_write(ES8388_ADCCONTROL13, ES8388_REG15_ADC_CTRL13_ALCZCWIN));
  ESP_ERROR_CHECK(es8388_write(ES8388_ADCCONTROL14, ES8388_REG16_ADC_CTRL14_NG));

  // Step 14: Power up ADC/ Analog Input/ Micbias for Record
  ESP_ERROR_CHECK(es8388_write(ES8388_ADCPOWER, ES8388_REG03_ADC_POWER_MGMT));

  // Step 15: Set SFI for DAC
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL1, ES8388_REG17_DAC_CTRL1_FORMAT));
  // Step 16: Select MCLK/LRCK ratio for DAC
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL2, ES8388_REG18_DAC_CTRL2_FSRATIO));

  // Step 17: Set DAC Digital Volume
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL4, ES8388_REG1A_DAC_CTRL4_LVOL));
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL5, ES8388_REG1B_DAC_CTRL5_RVOL));

  // Step 18: UnMute DAC
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL3, ES8388_REG19_DAC_CTRL3_RAMPMUTE));

  // DAC Output Configuration (before Mixer)
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL6, ES8388_REG1C_DAC_CTRL6_DEEMPHINV));
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL7, ES8388_REG1D_DAC_CTRL7_ZEROSE));

  // Shelving Filter Coefficients (Equalizer)
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL8, ES8388_REG1E_DAC_CTRL8_SHELVINGA29_24));
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL9, ES8388_REG1F_DAC_CTRL9_SHELVINGA23_16));
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL10, ES8388_REG20_DAC_CTRL10_SHELVINGA15_8));
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL11, ES8388_REG21_DAC_CTRL11_SHELVINGA7_0));
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL12, ES8388_REG22_DAC_CTRL12_SHELVINGB29_24));
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL13, ES8388_REG23_DAC_CTRL13_SHELVINGB23_16));
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL14, ES8388_REG24_DAC_CTRL14_SHELVINGB15_8));
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL15, ES8388_REG25_DAC_CTRL15_SHELVINGB7_0));

  // DAC Analog Stage Control (Offset, VROI)
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL22, ES8388_REG2C_DAC_CTRL22_OFFSETVAL));
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL23, ES8388_REG2D_DAC_CTRL23_VROI));

  // Step 19: Setup Mixer
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL16, ES8388_REG26_DAC_CTRL16_MIXSEL));
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL17, ES8388_REG27_DAC_CTRL17_LD2LOVOL));
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL18, ES8388_REG28_DAC_CTRL18));
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL19, ES8388_REG29_DAC_CTRL19));
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL20, ES8388_REG2A_DAC_CTRL20_RD2ROVOL));

  // Step 20: Set LOUT/ROUT Volume
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL24, ES8388_REG2E_DAC_CTRL24_LOUT1VOL));
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL25, ES8388_REG2F_DAC_CTRL25_ROUT1VOL));
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL26, ES8388_REG30_DAC_CTRL26_LOUT2VOL));
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL27, ES8388_REG31_DAC_CTRL27_ROUT2VOL));

  // Headphone/Speaker Reference Configuration
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL28, ES8388_REG32_DAC_CTRL28));
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL29, ES8388_REG33_DAC_CTRL29_HPLOUT1REF));
  ESP_ERROR_CHECK(es8388_write(ES8388_DACCONTROL30, ES8388_REG34_DAC_CTRL30_SPKLOUT2REF));

  // Step 21: Power up DEM and STM (Final power up)
  // User's intended final power up value (assuming 0x00 for general startup)
  ESP_ERROR_CHECK(es8388_write(ES8388_CHIPPOWER, ES8388_REG02_CHIP_POWER_MGMT));

  // End

  /* es8388 PA gpio_config */
  gpio_config_t io_conf;
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
                             .master.clk_speed = 10000};
  ESP_ERROR_CHECK(get_i2c_pins(I2C_NUM_0, &es_i2c_cfg));
  i2c_handle = i2c_bus_create(I2C_NUM_0, &es_i2c_cfg);
  return ESP_OK;
}

esp_err_t es8388_write(uint8_t reg_add, uint8_t data) {
  return i2c_bus_write_bytes(i2c_handle, ES8388_ADDR, &reg_add, sizeof(reg_add), &data, sizeof(data));
}
