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

class StepperWithTarget {
  const struct device *stepper_dev;
  const struct device *led_dev;
  bool is_moving = false;
  int32_t target_position = 0;
  bool enabled = false;

  void update_led();

  static void event_callback_wrapper(const struct device *dev,
                                     const enum stepper_event event,
                                     void *user_data);

public:
  StepperWithTarget(const struct device *dev,
                    const struct device *led_dev = nullptr);

  void log_state();

  int enable();
  int disable();
  void start();
  void pause();
  void wait_and_pause();

  int get_position();

  int go_relative(int32_t dist);
  void set_target_position(int32_t _target_position);
  int32_t get_target_position();

  bool step_towards_target();

  bool is_in_target_position();

  const struct stepper_with_target_status get_status();
};

void start_stepper(StepperWithTarget *_started_stepper_ptr);

#endif // STEPPERWITHTARGET_H_