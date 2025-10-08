#include "GUI.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gui, LOG_LEVEL_INF);

// Constructor
GUI::GUI()
    : display_dev(nullptr), main_screen(nullptr), status_label(nullptr) {}

// Destructor
GUI::~GUI() { deinit(); }

// Initialize the GUI
bool GUI::init() {
  LOG_INF("Initializing minimal GUI");

  // Get display device
  display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
  if (!device_is_ready(display_dev)) {
    LOG_ERR("Display device not ready");
    return false;
  }

  // Get current screen
  main_screen = lv_scr_act();

  // Set background color
  lv_obj_set_style_bg_color(main_screen, lv_color_black(), 0);

  // Create status label at top
  status_label = lv_label_create(main_screen);
  lv_label_set_text(status_label, "Zephyr Rail Ready");
  lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 10);
  lv_obj_set_style_text_color(status_label, lv_color_white(), 0);

  lv_timer_handler();
  display_blanking_off(display_dev);

  LOG_INF("Minimal GUI initialized successfully");
  return true;
}

// Deinitialize the GUI
void GUI::deinit() {
  // Objects are automatically cleaned up by LVGL
  main_screen = nullptr;
  status_label = nullptr;
}

// Update GUI with current status
void GUI::update(const struct stepper_with_target_status *stepper_status,
                 const struct stack_status *stack_status) {
  if (!stepper_status || !stack_status) {
    return;
  }

  // Update status based on movement
  if (status_label) {
    const char *status_text;
    if (stepper_status->is_moving) {
      status_text = "Moving...";
    } else if (stack_status->index_in_stack.has_value()) {
      status_text = "Stacking Active";
    } else {
      status_text = "Ready";
    }
    LOG_DBG("Status: %s", status_text);
    lv_label_set_text(status_label, status_text);
  }
}

// Set main status
void GUI::set_status(const char *status) {
  if (!status)
    return;

  if (status_label) {
    lv_label_set_text(status_label, status);
  }

  LOG_INF("Status: %s", status);
}

// Run LVGL task handler
void GUI::run_task_handler() {
  lv_task_handler();
  lv_timer_handler();
}

// Helper method for position conversion
int GUI::position_as_nm(int pitch_per_rev, int pulses_per_rev, int position) {
  // Convert stepper position to nanometers
  if (pulses_per_rev == 0)
    return 0;
  return (position * pitch_per_rev * 1000000) / pulses_per_rev;
}