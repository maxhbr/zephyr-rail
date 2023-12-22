#ifndef __STEPPER_H_
#define __STEPPER_H_

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/zbus/zbus.h>

#include <zephyr/logging/log.h>

#include "GPIOs.h"

#if !DT_NODE_EXISTS(DT_NODELABEL(stepper))
#error "Overlay for stepper node not properly defined."
#endif

class PULSE : public GPIO
{
public:
  PULSE(const struct gpio_dt_spec *spec) : GPIO(spec, GPIO_OUTPUT_ACTIVE){};
  void pulse()
  {
    LOG_MODULE_DECLARE(stepper);
    set(true);
    k_usleep(2); // puse width no less then 7.5us
    set(false);
    k_usleep(2); // puse width no less then 7.5us
  };
};

#define STEPS_PER_REV 12800
#define DIR_RIGHT 1
#define DIR_LEFT -1

static const struct gpio_dt_spec stepper_pulse = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 0);
static const struct gpio_dt_spec stepper_dir = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 1);

struct stepper_status
{
  int direction;
  int step_jump;
  int position;
};

class Stepper
{
  struct k_sem *stepper_sem;
  int direction = DIR_RIGHT;
  int step_jump = 1;
  int position = 0;

  PULSE pulse = PULSE(&stepper_pulse);
  GPIO dir = GPIO(&stepper_dir, GPIO_OUTPUT_ACTIVE);

  void set_direction_towards(int target);

public:
  Stepper();

  void log_state();

  void start();
  void pause();
  bool step_towards(int target);
  int get_position();

  const struct stepper_status get_status()
  {
    return {
        .direction = direction,
        .step_jump = step_jump,
        .position = position};
  };
};

#endif // __STEPPER_H_
