#include "esp_err.h"
#include "esp_log.h"
#include "esp_types.h"
#include "soc/gpio_num.h"
#include "u8g2.h"
#include "u8g2_esp32_hal.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "lcd.h"

static const char *TAG = "LCD";

#define PIN_DC GPIO_NUM_12   // JT_MTDI
#define PIN_CS GPIO_NUM_15   // JT_MTDO
#define PIN_CLK GPIO_NUM_14  // JT_MTMS
#define PIN_MOSI GPIO_NUM_13 // JT_MTCK
#define PIN_RESET GPIO_NUM_NC

// 48 × 84 pixels matrix LCD
// https://github.com/olikraus/u8g2/wiki/fntlistmono#6-pixel-height
#define FONT (u8g2_font_5x8_mf)
// top line of pixels is clear, 7 lines, -1 y offset, 7*7-1=48
#define CHAR_HEIGHT (7)
#define TEXT_LINES (7)
// right line of pixels is clear, 17 columns, 17*5-1=84
// #define CHAR_WIDTH (5)
#define TEXT_COLUMNS (17)

static u8g2_t u8g2;
// text_buf stores TEXT_BUF_LEN characters, 0-terminated lines
static char text_buf[TEXT_LINES][TEXT_COLUMNS + 1] = {0};
static int current_line = 0;
static int current_column = 0;

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

void lcd_test() {
  for (int i = 0; i < 5; i++) {
    lcd_print_str("Hello! ");
    lcd_flush();

    lcd_print_flush('A');
    lcd_print_flush('B');
    lcd_print_flush('C');
    lcd_print_flush(' ');

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void lcd_flush() {
  u8g2_ClearBuffer(&u8g2);

  // current line is always at the bottom
  int y;
  int buf_line = current_line;

  for (int l = TEXT_LINES - 1; l >= 0; l--) {
    y = (CHAR_HEIGHT - 1) + (CHAR_HEIGHT)*l;
    u8g2_DrawStr(&u8g2, 0, y, text_buf[buf_line]);
    buf_line--;
    if (buf_line < 0) {
      buf_line = TEXT_LINES - 1;
    }
  }

  u8g2_SendBuffer(&u8g2);
}

void lcd_print_str(const char *cp) {
  while (*cp) {
    lcd_print(*cp);
    cp++;
  }
}

void lcd_print(char ch) {
  if (!(isascii(ch) && ch > 0)) {
    ch = '?';
  }

  if (current_column >= TEXT_COLUMNS) {
    current_column = 0;
    current_line++;
  }

  if (current_line >= TEXT_LINES) {
    current_line = 0;
  }

  text_buf[current_line][current_column] = ch;
  current_column++;
  text_buf[current_line][current_column] = 0;
}

void lcd_print_flush(char ch) {
  lcd_print(ch);
  lcd_flush();
}
