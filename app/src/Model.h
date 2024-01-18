#ifndef __MODEL_H_
#define __MODEL_H_

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/zbus/zbus.h>

#include <optional>

#include "StepperWithTarget.h"
#include "Stack.h"

struct model_status
{
  struct stepper_with_target_status stepper_with_target_status;
  int upper_bound;
  int lower_bound;
  std::optional<Stack*> stack;
};

class Model
{
  StepperWithTarget *stepper;
  int upper_bound = 0;
  int lower_bound = 0;

  Stack *stack;

public:
  Model(StepperWithTarget *_stepper);

  void log_state();

  void go(int dist);

  void set_target_position(int _target_position);
  int get_target_position();
  void set_upper_bound(int _upper_bound);
  int get_upper_bound();
  void set_lower_bound(int _lower_bound);
  int get_lower_bound();

  int get_cur_position();
  bool is_in_target_position();

  int mk_stack(int step_size);

  const struct model_status get_status()
  {
    return {
        .stepper_with_target_status = stepper->get_status(),
        .upper_bound = upper_bound,
        .lower_bound = lower_bound,
        .stack = {}
    };
  }
  void pub_status();
};

#endif // __MODEL_H_
