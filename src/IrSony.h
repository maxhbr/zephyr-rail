#ifndef IRSONY_H_
#define IRSONY_H_

#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/init.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/zephyr.h>
#include <zephyr/types.h>

#include <zephyr/logging/log.h>

#include "GPIOs.h"


// #define PWM_IR_NODE DT_ALIAS(pwmir)
// #define PWM_IR_CTLR DT_PWMS_CTLR(PWM_IR_NODE)
// #define PWM_IR_CHANNEL DT_PWMS_CHANNEL(PWM_IR_NODE)
// #define PWM_IR_FLAGS DT_PWMS_FLAGS(PWM_IR_NODE)

#define MIN_PERIOD_USEC (USEC_PER_SEC / 64U)
#define MAX_PERIOD_USEC USEC_PER_SEC

// 40kHz
#define PERIOD_USEC (USEC_PER_SEC / 40000U)
// pulses with approx 25% mark/space ratio
#define PULSE_USEC (PERIOD_USEC / 4)

class IrSony {
  // const struct device *pwm = DEVICE_DT_GET(PWM_IR_CTLR);
  const struct pwm_dt_spec pwmir_spec = PWM_DT_SPEC_GET(DT_CHOSEN(rail_pwmir));
  uint32_t max_period = MAX_PERIOD_USEC;

  int start_carrier();
  int stop_carrier();
  int send_pulse(int duration);
  int send_start();
  int send_bit(bool is_one);
  int send_code(unsigned long code);
  int send_command(unsigned long command);

public:
  IrSony();
  void shoot();
};

#endif // IRSONY_H_
