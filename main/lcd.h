#ifndef LCD_H_
#define LCD_H_

void lcd_init();

void lcd_flush();

void lcd_print(char ch);
void lcd_print_str(const char *cp);

void lcd_print_flush(char ch);

void lcd_test();

#endif // LCD_H_
