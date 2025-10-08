#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>

// Include the status structures
#include "Stack.h"
#include "StepperWithTarget.h"

// Conditional LVGL includes
#ifdef CONFIG_LVGL
#include <lvgl.h>
#else
// Define dummy types when LVGL is not available
typedef void *lv_obj_t;
#endif

class GUI {
private:
  // Display device
  const struct device *display_dev;

  // Simple UI components
  lv_obj_t *main_screen;
  lv_obj_t *status_label;

  // Helper methods
  int position_as_nm(int pitch_per_rev, int pulses_per_rev, int position);

public:
  GUI();
  ~GUI();

  // Initialization and lifecycle
  bool init();
  void deinit();

  // Main update method
  void update(const struct stepper_with_target_status *stepper_status,
              const struct stack_status *stack_status);

  // Status methods
  void set_status(const char *status);

  // Task handler for LVGL
  void run_task_handler();
};