#include "leds.h"

#include "driver/gpio.h"

void leds_init() {
  // LEDs
  gpio_config_t io_conf = {
      .intr_type = GPIO_INTR_DISABLE,
      .mode = GPIO_MODE_OUTPUT,
      .pin_bit_mask = (1ULL << LED_PIN_1) | (1ULL << LED_PIN_2),
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .pull_up_en = GPIO_PULLUP_DISABLE,
  };

  gpio_config(&io_conf);
}
