/**
	@file LCD.cpp
	@author
	@date
	@brief LCD library.
*/

#include "LCD.h"

/*******************************************************************************
 ** Initializations
 ******************************************************************************/

LCD::LCD(
  uint8_t pin_rs, uint8_t pin_rw, uint8_t pin_en,
  uint8_t pin_db0, uint8_t pin_db1, uint8_t pin_db2, uint8_t pin_db3,
  uint8_t pin_db4, uint8_t pin_db5, uint8_t pin_db6, uint8_t pin_db7
) {
  // save pins
  this->pin_rs = pin_rs;
  this->pin_rw = pin_rw;
  this->pin_en = pin_en;

  pinMode(pin_rs, OUTPUT);
  pinMode(pin_rw, OUTPUT);
  pinMode(pin_en, OUTPUT); digitalWrite(pin_en, LOW);

  pin_db[0] = pin_db0;
  pin_db[1] = pin_db1;
  pin_db[2] = pin_db2;
  pin_db[3] = pin_db3;
  pin_db[4] = pin_db4;
  pin_db[5] = pin_db5;
  pin_db[6] = pin_db6;
  pin_db[7] = pin_db7;
}

void LCD::init()
{
  /* See page 45 */

  delay(40);

  // Function set: 8 bits
  send(INSTR, 0b00110000);
  delayMicroseconds(4100);

  // Function set: 8 bits
  send(INSTR, 0b00110000);
  delayMicroseconds(100);

  // Function set: 8 bits
  send(INSTR, 0b00110000);
  wait();

  // Function set: 8 bits, 2 lines and character font 5x8
  send(INSTR, 0b00111000);
  wait();

  // Display off
  off();

  // Display clear
  clear();

  // Entry mode set: Cursor move direction increment and no shifting
  send(INSTR, 0b00000110);
  wait();


  /* INITIALIZATION FINISHED */

  // Display on
  on(CURSOR_ON, BLINK_OFF);
}


/*******************************************************************************
 ** Print
 ******************************************************************************/

void LCD::print(char c)
{
  send(DATA, c);
  wait();
}

void LCD::print(char *str)
{
  while (*str != '\0') {
    print(*str);
    str++;
  }
}


/*******************************************************************************
 ** Display control
 ******************************************************************************/

void LCD::on(lcd_cursor_state_t cursor_state, lcd_blink_state_t blink_state)
{
  send(INSTR, 0b00001110);
  wait();
}

void LCD::off()
{
  send(INSTR, 0b00001000);
  wait();
}

void LCD::clear()
{
  send(INSTR, 0b00000001);
  wait();
}


/*******************************************************************************
 ** Cursor control
 ******************************************************************************/

void LCD::moveCursor(uint8_t row, uint8_t col)
{
  uint8_t instr = 0b10000000;

  if (row == 1) {
    instr |= 0b01000000;
  }
  instr |= col;

  send(INSTR, instr);
}

void LCD::moveCursorRight()
{

}

void LCD::moveCursorLeft()
{

}


/*******************************************************************************
 ** Shift control
 ******************************************************************************/

void LCD::shiftDisplayRight()
{

}

void LCD::shiftDisplayLeft()
{

}


/******************************************************************************/
/** PRIVATE FUNCTIONS                                                        **/
/******************************************************************************/

void LCD::send(lcd_send_mode_t send_mode, uint8_t value)
{
  // Set data pins as output
  for (uint8_t pin_id = 0; pin_id < 8; pin_id++) {
    pinMode(pin_db[pin_id], OUTPUT);
  }

  // Set RS and RW to send instruction/data depending on mode
  if (send_mode == INSTR) {
    digitalWrite(pin_rs, LOW);
  } else if (send_mode == DATA) {
    digitalWrite(pin_rs, HIGH);
  }
  digitalWrite(pin_rw, LOW);

  // Write from value to data pins
  digitalWrite(pin_db[7], (value & 0b10000000) >> 7);
  digitalWrite(pin_db[6], (value & 0b01000000) >> 6);
  digitalWrite(pin_db[5], (value & 0b00100000) >> 5);
  digitalWrite(pin_db[4], (value & 0b00010000) >> 4);
  digitalWrite(pin_db[3], (value & 0b00001000) >> 3);
  digitalWrite(pin_db[2], (value & 0b00000100) >> 2);
  digitalWrite(pin_db[1], (value & 0b00000010) >> 1);
  digitalWrite(pin_db[0], (value & 0b00000001) >> 0);

  do_enable_pulse();
}

void LCD::wait()
{
  uint8_t busy;

  // Set data pins as input
  for (uint8_t pin_id = 0; pin_id < 8; pin_id++) {
    pinMode(pin_db[pin_id], INPUT);
  }

  // Set RS and RW to read busy flag
  digitalWrite(pin_rs, LOW);
  digitalWrite(pin_rw, HIGH);

  do {
    digitalWrite(pin_en, HIGH);
    
    // Read busy flag (db7)
    busy = digitalRead(pin_db[7]);

    digitalWrite(pin_en, LOW);
  } while (busy == HIGH);
}

void LCD::do_enable_pulse()
{
  digitalWrite(pin_en, LOW);
  delayMicroseconds(1);
  digitalWrite(pin_en, HIGH);
  delayMicroseconds(1);
  digitalWrite(pin_en, LOW);
  delayMicroseconds(100);
}
