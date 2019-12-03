
#ifndef __TimedBlink__H__
#define __TimedBlink__H__

#include <Arduino.h>

enum blink_t {BLINK_ON, BLINK_OFF};

class TimedBlink {
  private:
    unsigned long m_blinkTime;
    int m_onForTime;
    int m_offForTime;
    blink_t m_blinkState;
    short m_pin;
    int m_resolution;

    void reset();

  public:

    TimedBlink(int pin);
    void blink(int on_for, int off_for);
    void blink();
    void setOnTime(int ms);
    void setOffTime(int ms);
    void setBlinkState(blink_t state);
    void blinkDelay(int d);
    void blinkOff();
    void on();
    void off();
};

#endif // __TimedBlink__H__
