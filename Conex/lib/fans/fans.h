#pragma once
#include <Arduino.h>

class Fans {
public:
  void begin();
  void set_exhaust_percent(uint8_t percent);

private:
  uint8_t clamp_percent(uint8_t percent);
};