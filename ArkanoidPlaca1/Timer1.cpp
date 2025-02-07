#include "Timer1.h"

Timer1::Timer1() {}

void Timer1::init(pscaler_t pscaler, uint16_t ticks)
{
  // Clear timer counter
  TCNT1 = 0;

  // Reset timer control registers
  TCCR1A = 0;
  TCCR1B = 0;

  // Confige CTC mode (WGMn3:0 = 0100)
  TCCR1B |= 0 << WGM13 | 1 << WGM12 | 0 << WGM11 | 0 << WGM10;

  // Enable OC match interrupt
  TIMSK1 |= (1 << OCIE1A);

  this->pscaler = pscaler;
  this->ticks = ticks;
}

void Timer1::start()
{
  OCR1A = ticks;
  TCCR1B &= ~(1 << CS12 | 1 << CS11 | 1 << CS10);
  switch (pscaler) {
    case PS_1    : TCCR1B |= (0 << CS12 | 0 << CS11 | 1 << CS10); break;
    case PS_8    : TCCR1B |= (0 << CS12 | 1 << CS11 | 0 << CS10); break;
    case PS_64   : TCCR1B |= (0 << CS12 | 1 << CS11 | 1 << CS10); break;
    case PS_256  : TCCR1B |= (1 << CS12 | 0 << CS11 | 0 << CS10); break;
    case PS_1024 : TCCR1B |= (1 << CS12 | 0 << CS11 | 1 << CS10); break;
  }
}

void Timer1::pause()
{
  // Disable prescaler
  TCCR1B &= ~(1 << CS12 | 1 << CS11 | 1 << CS10);
}

void Timer1::stop()
{
  // Clear timer counter
  TCNT1 = 0;

  pause();
}
