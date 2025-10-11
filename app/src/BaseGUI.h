#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>

// Include the status structures
#include "Stack.h"
#include "StateMachine.h"
#include "StepperWithTarget.h"

// Conditional LVGL includes
#include <lvgl.h>

class BaseGUI {
protected:
  const StateMachine *sm;
  const struct device *display_dev;

  lv_obj_t *main_screen;
  lv_obj_t *status_label;
  lv_obj_t *tabview;

  // Protected initialization methods
  void init_tabview(lv_obj_t *parent);

  // Helper methods
  int position_as_nm(int pitch_per_rev, int pulses_per_rev, int position);

public:
  BaseGUI(const StateMachine *sm);
  virtual ~BaseGUI();

  // Initialization and lifecycle
  virtual bool init();
  virtual void deinit();

  // Main update method
  virtual void update(const struct stepper_with_target_status *stepper_status,
                      const struct stack_status *stack_status);

  // Status methods
  void set_status(const char *status);

  // Task handler for LVGL
  virtual void run_task_handler();
};