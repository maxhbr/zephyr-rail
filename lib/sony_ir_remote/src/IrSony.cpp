#include "IrSony.h"

/*
   contains parts from:

   ATtiny85 Sony NEX/Alpha Remote Control

   David Johnson-Davies - www.technoblogy.com - 12th April 2015
   ATtiny85 @ 1 MHz (internal oscillator; BOD disabled)

   CC BY 4.0
   Licensed under a Creative Commons Attribution 4.0 International license:
   http://creativecommons.org/licenses/by/4.0/
*/

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(irsony);

#if DT_NODE_EXISTS(DT_NODELABEL(rail_pwmir))

// Remote control
const unsigned long shutter_code = 0x2D;
const unsigned long two_secs_code = 0x37;
const unsigned long video_code = 0x48;

const int BASE = 635;
const int ZERO_DURATION = BASE;
const int ONE_DURATION = BASE * 2;
const int START_DURATION = BASE * 4;

int IrSony::start_carrier() {
  int ret = pwm_set_dt(&pwmir_spec, PERIOD_USEC, PULSE_USEC);
  if (ret < 0) {
    LOG_ERR("failed to start carrier");
  }
  return ret;
}
int IrSony::stop_carrier() {
  int ret = pwm_set_dt(&pwmir_spec, PERIOD_USEC, 0);
  if (ret < 0) {
    LOG_ERR("failed to stop carrier");
  }
  return ret;
}

int IrSony::send_pulse(int duration) {
  int ret = 0;
  ret = start_carrier();
  if (ret < 0) {
    return ret;
  }
  k_busy_wait(duration);
  ret = stop_carrier();
  if (ret < 0) {
    return ret;
  }
  k_busy_wait(BASE);
  return ret;
}

int IrSony::send_start() {
  LOG_MODULE_DECLARE(irsony);
  return send_pulse(START_DURATION);
}
int IrSony::send_bit(bool is_one) {
  LOG_MODULE_DECLARE(irsony);
  int ret = 0;
  if (is_one) {
    ret = send_pulse(ONE_DURATION);
    if (ret < 0) {
      return ret;
    }
  } else {
    ret = send_pulse(ZERO_DURATION);
    if (ret < 0) {
      return ret;
    }
  }
  return ret;
}
int IrSony::send_code(unsigned long code) {
  int ret = 0;
  ret = send_start();
  if (ret < 0) {
    return ret;
  }
  // Send 20 bits
  for (int bit = 0; bit < 20; bit++) {
    ret = send_bit(code & ((unsigned long)1 << bit));
    if (ret < 0) {
      return ret;
    }
  }
  return ret;
}
int IrSony::send_command(unsigned long command) {
  int ret = 0;
  unsigned long address = 0x1E3A;
  LOG_MODULE_DECLARE(irsony);
  unsigned long code = (unsigned long)address << 7 | command;
  LOG_INF("send command=0x%08lu code=0x%08lu", command, code);

  for (int i = 0; i < 6; i++) {
    ret = send_code(code);
    if (ret < 0) {
      return ret;
    }
    k_busy_wait(11000);
  }
  return ret;
}

#endif // if exists

IrSony::IrSony() {
  LOG_MODULE_DECLARE(irsony);
#if DT_NODE_EXISTS(DT_NODELABEL(rail_pwmir))
  int ret;
  if (!pwm_is_ready_dt(&pwmir_spec)) {
    LOG_ERR("Error: PWM device %s is not ready\n", pwmir_spec.dev->name);
    return;
  }
  ret = pwm_set_dt(&pwmir_spec, PERIOD_USEC, 0);
  // pwm_pin_set_usec(pwm, PWM_IR_CHANNEL, PERIOD_USEC, 0, PWM_IR_FLAGS)
  if (ret < 0) {
    LOG_ERR("failed to stop carrier");
    return;
  }
#endif // if exists
}

void IrSony::shoot() {
  LOG_MODULE_DECLARE(irsony);
#if DT_NODE_EXISTS(DT_NODELABEL(rail_pwmir))
  int ret = send_command(shutter_code);
  if (ret < 0) {
    LOG_ERR("failed to send shoot");
  }
#endif // if exists
}