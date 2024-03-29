#ifndef __CONTROLLER_H_
#define __CONTROLLER_H_

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>

#include <zephyr/drivers/display.h>
#include <lvgl.h>

#include <zephyr/logging/log.h>

#include "../ir/IrSony.h"
#include "Model.h"


#define NOOP_CONTROLLER_ACTION 0
#define GO_CONTROLLER_ACTION 1
#define GO_TO_CONTROLLER_ACTION 2
#define SET_NEW_LOWER_BOUND_ACTION 3
#define SET_NEW_UPPER_BOUND_ACTION 4

struct controller_msg
{
  int action;
  int value;
};

class Controller
{
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

  void handle_controller_msg(const struct controller_msg *msg);

  void synchronize_and_sleep(k_timeout_t timeout);

  void prepare_stack();
  void start_stack();
  void stop_stack();
  void do_next_stack_step();
};

void set_controller_ptr_for_zbus(Controller *_controller_ptr_for_zbus);

#endif // __CONTROLLER_H_
