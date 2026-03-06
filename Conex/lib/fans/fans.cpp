#include "fans.h"
#include "../../include/config.h"

uint8_t Fans::clamp_percent(uint8_t percent) {
  if (percent > 100) return 100;
  return percent;
}

void Fans::begin() {

  ledcSetup(fan_pwm_ch_exhaust,
            fan_pwm_freq_hz,
            fan_pwm_res_bits);

  ledcAttachPin(pin_pwm_exhaust,
                fan_pwm_ch_exhaust);

  set_exhaust_percent(fan_fixed_percent);
}

void Fans::set_exhaust_percent(uint8_t percent) {

  percent = clamp_percent(percent);

  const uint32_t max_duty =
      (1UL << fan_pwm_res_bits) - 1UL;

  const uint32_t duty =
      (percent * max_duty) / 100;

  ledcWrite(fan_pwm_ch_exhaust,
            duty);
}