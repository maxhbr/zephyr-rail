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
#include "stepper_with_target/StepperWithTarget.h"

// Conditional LVGL includes
#include <lvgl.h>

class BaseGUI {
protected:
  const struct device *display_dev;

  lv_obj_t *main_screen;
  lv_obj_t *status_label;
  lv_obj_t *tabview;

  // Protected initialization methods
  void init_tabview(lv_obj_t *parent);

public:
  BaseGUI();
  virtual ~BaseGUI();

  // Initialization and lifecycle
  virtual bool init();
  virtual void start();
  virtual void deinit();
  virtual void run_task_handler();

  void set_status(const char *status);
};