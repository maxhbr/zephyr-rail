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

  int step_number;
  int cur_step_index;

  bool stack_in_progress;
};

class Model
{
  StepperWithTarget *stepper;
  int upper_bound = 12800;
  int lower_bound = 0;

  int step_number = 0;
  int cur_step_index = 0;
  int *stepps;
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
  int get_step_number() { return step_number; };
  int get_step_jump_size()
  {
    if (step_number > 1)
    {
      return stepps[1] - stepps[0];
    }
    else
    {
      return 0;
    }
  };
  int get_cur_step_index() { return cur_step_index; };
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
        .step_number = step_number,
        .cur_step_index = cur_step_index,
        .stack_in_progress = stack_in_progress,
    };
  }
  void pub_status();
};

#endif // __MODEL_H_
