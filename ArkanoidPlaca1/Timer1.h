#include "Arduino.h"

#ifndef TIMER1_H
#define TIMER1_H

typedef enum {PS_1, PS_8, PS_64, PS_256, PS_1024} pscaler_t;

class Timer1 {
  public:
    Timer1();

    void init(pscaler_t pscaler, uint16_t ticks);
    void start();
    void pause();
    void stop();

  private:
    pscaler_t pscaler;
    uint16_t ticks;
};

#endif
