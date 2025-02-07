/**
	@file LCD.h
	@author
	@date
	@brief LCD library.
*/

#include <Arduino.h>
#include <stdint.h>

#ifndef LCD_H
#define LCD_H

typedef enum {CURSOR_ON, CURSOR_OFF} lcd_cursor_state_t;
typedef enum {BLINK_ON,  BLINK_OFF } lcd_blink_state_t;

typedef enum {INSTR, DATA} lcd_send_mode_t;

class LCD {
  public:
    // Initializations
    LCD(uint8_t pin_rs, uint8_t pin_rw, uint8_t pin_en,
        uint8_t pin_db0, uint8_t pin_db1, uint8_t pin_db2, uint8_t pin_db3,
        uint8_t pin_db4, uint8_t pin_db5, uint8_t pin_db6, uint8_t pin_db7);

    void init();

    // Print
    void print(char c);
    void print(char *str);

    // Display control
    void on(lcd_cursor_state_t cursor_state, lcd_blink_state_t blink_state);
    void off();

    void clear();

    // Cursor control
    void moveCursor(uint8_t row, uint8_t col);
    void moveCursorRight();
    void moveCursorLeft();

    // Shift control
    void shiftDisplayRight();
    void shiftDisplayLeft();

  private:
    void send(lcd_send_mode_t send_mode, uint8_t value);
    void wait();

    void do_enable_pulse();

    uint8_t pin_rs;
    uint8_t pin_rw;
    uint8_t pin_en;

    uint8_t pin_db[8];
};

#endif
