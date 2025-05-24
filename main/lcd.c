#include "esp_err.h"
#include "esp_log.h"
#include "esp_types.h"
#include "u8g2.h"
#include "u8g2_esp32_hal.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "lcd.h"

static const char *TAG = "LCD";

// Define your GPIO pins for the Nokia 5110
#define PIN_CLK GPIO_NUM_18
#define PIN_MOSI GPIO_NUM_23
#define PIN_CS GPIO_NUM_5
#define PIN_DC GPIO_NUM_0
#define PIN_RESET (-1)

// 48 × 84 pixels matrix LCD
// https://github.com/olikraus/u8g2/wiki/fntlistmono#6-pixel-height
#define FONT (u8g2_font_5x8_mf)
// top line of pixels is clear, 7 lines, -1 y offset, 7*7-1=48
#define CHAR_HEIGHT (7)
#define TEXT_LINES (7)
// right line of pixels is clear, 17 columns, 17*5-1=84
// #define CHAR_WIDTH (5)
#define TEXT_COLUMNS (17)
#define TEXT_BUF_LEN (TEXT_LINES * TEXT_COLUMNS)

static u8g2_t u8g2;
static char text_buf[TEXT_BUF_LEN + 1] = {0};
static char *text_pos = text_buf;

void lcd_init() {
  ESP_LOGI(TAG, "Starting U8g2 PCD8544...");

  u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
  u8g2_esp32_hal.bus.spi.clk = PIN_CLK;
  u8g2_esp32_hal.bus.spi.mosi = PIN_MOSI;
  u8g2_esp32_hal.bus.spi.cs = PIN_CS;
  u8g2_esp32_hal.dc = PIN_DC;
  u8g2_esp32_hal.reset = PIN_RESET;
  u8g2_esp32_hal_init(u8g2_esp32_hal, NULL);

  // https://github.com/olikraus/u8g2/wiki
  u8g2_Setup_pcd8544_84x48_f(&u8g2, U8G2_R0, u8g2_esp32_spi_byte_cb, u8g2_esp32_gpio_and_delay_cb);

  // send init sequence to the display, display is in sleep mode after this
  u8g2_InitDisplay(&u8g2);
  // wake up display
  u8g2_SetPowerSave(&u8g2, 0);

  u8g2_SetFont(&u8g2, FONT);

  ESP_LOGI(TAG, "Initialized PCD8544");
}

void lcd_flush() {
  int y;

  u8g2_ClearBuffer(&u8g2);
  for (int l = 0; l < TEXT_LINES; l++) {
    y = (CHAR_HEIGHT - 1) + (CHAR_HEIGHT)*l;
    u8g2_DrawStr(&u8g2, 0, y, text_buf + (l * TEXT_COLUMNS));
  }
  u8g2_SendBuffer(&u8g2);
}

static void scroll_up() {
  memcpy(text_buf, text_buf + TEXT_COLUMNS, TEXT_BUF_LEN - TEXT_COLUMNS);
  text_pos = text_buf + TEXT_BUF_LEN - TEXT_COLUMNS;
  *text_pos = 0;
}

void print(char ch) {
  if (text_pos >= text_buf + TEXT_BUF_LEN) {
    scroll_up();
  }

  *text_pos = ch;
  text_pos++;
  *text_pos = 0;
}
