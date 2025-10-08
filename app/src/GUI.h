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
typedef void *lv_style_t;
typedef void *lv_font_t;
typedef void *lv_event_t;
typedef void (*lv_event_cb_t)(lv_event_t *);
#endif

class GUI {
private:
  // Display device
  const struct device *display_dev;

  // Main UI components
  lv_obj_t *main_screen;
  lv_obj_t *tabview;
  lv_obj_t *status_bar;

  // Tabs
  lv_obj_t *move_tab;
  lv_obj_t *stack_tab;
  lv_obj_t *config_tab;
  lv_obj_t *status_tab;

  // Status labels
  lv_obj_t *status_label;
  lv_obj_t *status_label_left;
  lv_obj_t *status_label_right;

  // Move tab components
  lv_obj_t *position_label;
  lv_obj_t *target_label;
  lv_obj_t *step_size_roller;
  lv_obj_t *move_forward_btn;
  lv_obj_t *move_backward_btn;
  lv_obj_t *home_btn;

  // Stack tab components
  lv_obj_t *stack_plan_label;
  lv_obj_t *step_number_roller;
  lv_obj_t *stack_start_btn;
  lv_obj_t *stack_stop_btn;

  // Config tab components
  lv_obj_t *config_table;

  // Status tab components
  lv_obj_t *status_table;

  // Styles
  lv_style_t style_title;
  lv_style_t style_button;
  lv_style_t style_panel;

  // Fonts
  const lv_font_t *font_title;
  const lv_font_t *font_normal;
  const lv_font_t *font_small;

  // Private methods
  void init_display();
  void init_styles();
  void init_main_screen();
  void init_tabview();
  void init_status_bar();

  void setup_move_tab();
  void setup_stack_tab();
  void setup_config_tab();
  void setup_status_tab();

  // Utility methods
  lv_obj_t *create_button(lv_obj_t *parent, const char *text,
                          lv_event_cb_t event_cb);
  lv_obj_t *create_label(lv_obj_t *parent, const char *text);
  lv_obj_t *create_roller(lv_obj_t *parent, const char *options);
  lv_obj_t *create_table(lv_obj_t *parent, int rows, int cols);

  // Event callbacks
  static void move_forward_event_cb(lv_event_t *e);
  static void move_backward_event_cb(lv_event_t *e);
  static void home_event_cb(lv_event_t *e);
  static void stack_start_event_cb(lv_event_t *e);
  static void stack_stop_event_cb(lv_event_t *e);

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
  void set_status_left(const char *status_left);
  void set_status_right(const char *status_right);

  // Display control
  void set_brightness(uint8_t brightness);
  void set_backlight(bool on);

  // Task handler for LVGL
  void run_task_handler();

  // Tab visibility control
  void show_tab(int tab_index);
  void set_header_visible(bool visible);

  // Table operations
  void set_table_cell_value(lv_obj_t *table, int row, int col,
                            const char *value);
  void set_table_cell_value_int(lv_obj_t *table, int row, int col, int value);
};