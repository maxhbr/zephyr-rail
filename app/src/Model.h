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

struct model_status
{
  struct stepper_with_target_status stepper_with_target_status;
  int upper_bound;
  int lower_bound;

  int length_of_stack;
  int index_in_stack;

  bool stack_in_progress;
};

class Model
{
  StepperWithTarget *stepper;
  int upper_bound = 12800;
  int lower_bound = 0;

  int length_of_stack = 0;

  int index_in_stack = 0;
  int *stepps_of_stack;
  bool stack_in_progress = false;

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

  void set_step_number(int _step_number);
  int get_step_number() { return length_of_stack; };
  int get_step_jump_size()
  {
    if (length_of_stack > 1)
    {
      return stepps_of_stack[1] - stepps_of_stack[0];
    }
    else
    {
      return 0;
    }
  };
  int get_cur_step_index() { return index_in_stack; };
  void set_step_position(int index, int pos);
  std::optional<int> get_next_step_and_increment();
  void set_stack_in_progress(bool _stack_in_progress);
  bool is_stack_in_progress();

  int get_cur_position();
  bool is_in_target_position();

  const struct model_status get_status()
  {
    return {
        .stepper_with_target_status = stepper->get_status(),
        .upper_bound = upper_bound,
        .lower_bound = lower_bound,
        .length_of_stack = length_of_stack,
        .index_in_stack = index_in_stack,
        .stack_in_progress = stack_in_progress,
    };
  }
  void pub_status();
};

#endif // __MODEL_H_
