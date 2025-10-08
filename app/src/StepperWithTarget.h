#ifndef STEPPERWITHTARGET_H_
#define STEPPERWITHTARGET_H_

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>

#include <soc.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

#include <optional>

#include "Stepper.h"

struct stepper_with_target_status {
  struct stepper_status stepper_status;
  bool is_moving;
  int target_position;
};

class StepperWithTarget : private Stepper {
  bool is_moving = false;
  int target_position;

public:
  StepperWithTarget(const struct gpio_dt_spec *stepper_pulse,
                    const struct gpio_dt_spec *stepper_dir)
      : Stepper(stepper_pulse, stepper_dir) {
    target_position = Stepper::get_position();
  }

  void log_state();

  void start();
  void pause();
  void wait_and_pause();

  int get_position();

  int go_relative(int dist);
  void set_target_position(int _target_position);
  int get_target_position();

  bool step_towards_target();

  bool is_in_target_position();

  const struct stepper_with_target_status get_status() {
    return {
        .stepper_status = Stepper::get_status(),
        .is_moving = is_moving,
        .target_position = target_position,
    };
  }
};

void start_stepper(StepperWithTarget *_started_stepper_ptr);

#endif // STEPPERWITHTARGET_H_
