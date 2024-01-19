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

#include "../GPIOs.h"

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

#define DIR_RIGHT 1
#define DIR_LEFT -1

struct stepper_status
{
  int direction;
  int step_jump;
  int position;
  int pitch_per_rev; 
	int pulses_per_rev;
};

class Stepper
{
  struct k_sem *stepper_sem;
  int direction = DIR_RIGHT;
  int step_jump = 1;
  int position = 0;

  int pitch_per_rev = 2; 
	int pulses_per_rev = 50000;

  PULSE pulse;
  GPIO dir;

  void set_direction_towards(int target);

public:
  Stepper(const struct gpio_dt_spec *stepper_pulse, const struct gpio_dt_spec *stepper_dir);

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
        .position = position,
        .pitch_per_rev = pitch_per_rev,
        .pulses_per_rev = pulses_per_rev};
  };
};

#endif // __STEPPER_H_
