#ifndef __CONTROLLER_H_
#define __CONTROLLER_H_

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/zephyr.h>
#include <zephyr/types.h>

#include <zephyr/drivers/display.h>
#include <lvgl.h>

#include <zephyr/logging/log.h>

#include "IrSony.h"
#include "Model.h"

class Controller {
  Model *model;
  IrSony *irsony;
  struct k_sem *work_in_progress_sem;
public:
  Controller(Model *_model, IrSony *_irsony);
  void work();

  void go(int dist);
  void go_to(int target_position);
  void go_to_upper_bound();
  void go_to_lower_bound();
  void set_new_upper_bound();
  void set_new_lower_bound();
  void set_step_number(int step_number);

  void synchronize_and_sleep(k_timeout_t timeout);

  void prepare_stack();
  void start_stack();
  void stop_stack();
  void do_next_stack_step();
};

#endif // __CONTROLLER_H_
