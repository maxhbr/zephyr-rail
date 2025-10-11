#include "GUI.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_output.h>

LOG_MODULE_REGISTER(gui, LOG_LEVEL_INF);

// Constructor
GUI::GUI(const StateMachine *sm)
    : display_dev(nullptr), main_screen(nullptr), status_label(nullptr),
      log_textarea(nullptr) {
  this->sm = sm;
}

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
  // lv_obj_set_style_bg_color(main_screen, lv_color_black(), 0);

  LOG_DBG("Initializing minimal GUI: Create status label at top");
  status_label = lv_label_create(main_screen);
  lv_label_set_text(status_label, "Zephyr Rail Ready");
  lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 10);
  // lv_obj_set_style_text_color(status_label, lv_color_white(), 0);

  LOG_DBG("Initializing minimal GUI: Create log text area");
  log_textarea = lv_textarea_create(main_screen);
  lv_obj_set_size(log_textarea, LV_PCT(90), LV_PCT(60));
  lv_obj_align(log_textarea, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_textarea_set_text(log_textarea, "");
  lv_textarea_set_placeholder_text(log_textarea,
                                   "Log messages will appear here...");
  lv_obj_add_state(log_textarea, LV_STATE_DISABLED); // Make it read-only

  LOG_DBG("Initializing minimal GUI: Configure log text area");
  lv_obj_set_scrollbar_mode(log_textarea, LV_SCROLLBAR_MODE_AUTO);

  LOG_DBG("Initializing minimal GUI: Finalizing LVGL setup");
  lv_timer_handler();
  k_sleep(K_MSEC(20));

  display_blanking_off(display_dev);

  LOG_INF("Minimal GUI initialized successfully");
  return true;
}

// Deinitialize the GUI
void GUI::deinit() {
  // Objects are automatically cleaned up by LVGL
  main_screen = nullptr;
  status_label = nullptr;
  log_textarea = nullptr;
}

// Update GUI with current status
void GUI::update(const struct stepper_with_target_status *stepper_status,
                 const struct stack_status *stack_status) {
  if (!stepper_status || !stack_status) {
    return;
  }

  const struct stepper_status inner_stepper_status =
      stepper_status->stepper_status;

  char buf[1000];
  const int pitch_per_rev = inner_stepper_status.pitch_per_rev;
  const int pulses_per_rev = inner_stepper_status.pulses_per_rev;

  const int position_nm = position_as_nm(pitch_per_rev, pulses_per_rev,
                                         inner_stepper_status.position);

  if (stepper_status->is_moving) {
    const int target_position_nm = position_as_nm(
        pitch_per_rev, pulses_per_rev, stepper_status->target_position);
    if (stepper_status->target_position > inner_stepper_status.position) {
      snprintf(buf, sizeof(buf), "%d (-> %d)", position_nm, target_position_nm);
    } else {
      snprintf(buf, sizeof(buf), "(%d <-) %d", target_position_nm, position_nm);
    }
  } else {
    snprintf(buf, sizeof(buf), "@%dnm", position_nm);
  }
  LOG_DBG("Status: %s", buf);
  lv_label_set_text(status_label, buf);
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
  /* update GUI */
  struct stepper_with_target_status stepper_status =
      this->sm->get_stepper_status();
  struct stack_status stack_status = this->sm->get_stack_status();

  this->update(&stepper_status, &stack_status);
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

// Add new method to handle log lines
void GUI::add_log_line(const char *log_data, size_t length) {
  if (!log_textarea) {
    return;
  }

  // Create a null-terminated string
  char log_line[256];
  size_t copy_len = MIN(length, sizeof(log_line) - 1);
  memcpy(log_line, log_data, copy_len);
  log_line[copy_len] = '\0';

  // Remove trailing newline if present
  if (copy_len > 0 && log_line[copy_len - 1] == '\n') {
    log_line[copy_len - 1] = '\0';
  }

  // Get current text
  const char *current_text = lv_textarea_get_text(log_textarea);

  // Limit the number of lines to prevent memory issues
  static const int MAX_LOG_LINES = 10;

  // Find text length and count lines by walking backwards
  size_t text_len = strlen(current_text);
  if (text_len > 0) {
    int newline_count = 0;
    const char *p = current_text + text_len - 1;

    // Walk backwards from the end, counting newlines
    while (p > current_text && newline_count < (MAX_LOG_LINES - 1)) {
      if (*p == '\n') {
        newline_count++;
      }
      p--;
    }

    // If we found enough newlines, trim to keep only the last MAX_LOG_LINES-1
    // lines
    if (newline_count >= (MAX_LOG_LINES - 1)) {
      // Move forward to the character after the newline we stopped at
      if (*p == '\n') {
        p++;
      } else {
        // We stopped at current_text, find the first newline after it
        while (*p && *p != '\n') {
          p++;
        }
        if (*p == '\n') {
          p++;
        }
      }
      lv_textarea_set_text(log_textarea, p);
    }
  }

  // Add new log line
  lv_textarea_add_text(log_textarea, log_line);

  // Scroll to bottom
  lv_obj_scroll_to_y(log_textarea, LV_COORD_MAX, LV_ANIM_OFF);
}