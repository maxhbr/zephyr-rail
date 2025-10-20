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
#include <zephyr/drivers/stepper.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>

#include <optional>

struct stepper_with_target_status {
  int32_t actual_position;
  bool is_moving;
  int32_t target_position;
};

/* Enum for speeds, fast, medium, slow */
enum class StepperSpeed { FAST, MEDIUM, SLOW };

class StepperWithTarget {
  const struct device *stepper_dev;
  bool is_moving = false;
  int32_t target_position = 0;
  bool enabled = false;

  int pitch_per_rev = 0;
  int pulses_per_rev = 0;

  static void event_callback_wrapper(const struct device *dev,
                                     const enum stepper_event event,
                                     void *user_data);

public:
  StepperWithTarget(const struct device *dev, int _pitch_per_rev,
                    int _pulses_per_rev);

  void log_state();

  int enable();
  int disable();
  void start();
  void pause();
  void wait_and_pause();

  int get_position();

  int set_speed(StepperSpeed speed);

  int go_relative(int32_t dist);
  void set_target_position(int32_t _target_position);
  int32_t get_target_position();

  bool step_towards_target();

  bool is_in_target_position();

  int position_as_nm(int position);
  const struct stepper_with_target_status get_status();
};

void start_stepper(StepperWithTarget *_started_stepper_ptr);

#endif // STEPPERWITHTARGET_H_