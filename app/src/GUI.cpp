#include "GUI.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_output.h>

LOG_MODULE_REGISTER(gui, LOG_LEVEL_DBG);

#ifdef CONFIG_DISPLAY
// Global GUI instance for thread access
static GUI *g_gui = nullptr;

#ifdef CONFIG_LOG
// Message queue for thread-safe log handling
struct log_msg_item {
  char data[256];
  size_t length;
};

K_MSGQ_DEFINE(log_msgq, sizeof(struct log_msg_item), 200, 4);

// Log output function for GUI backend
static int gui_log_out(uint8_t *data, size_t length, void *ctx) {
  if (g_gui) {
    struct log_msg_item msg;
    size_t copy_len = MIN(length, sizeof(msg.data) - 1);
    memcpy(msg.data, data, copy_len);
    msg.data[copy_len] = '\0';
    msg.length = copy_len;

    // Try to put message in queue (non-blocking)
    k_msgq_put(&log_msgq, &msg, K_NO_WAIT);
  }
  return length;
}

LOG_OUTPUT_DEFINE(gui_log_output, gui_log_out, NULL, 0);

// Log backend process function
static void gui_log_process(const struct log_backend *const backend,
                            union log_msg_generic *msg) {
  log_format_func_t log_output_func = log_format_func_t_get(LOG_OUTPUT_TEXT);
  log_output_func(&gui_log_output, &msg->log, 0);
}

// Log backend definition
static const struct log_backend_api gui_log_backend_api = {
    .process = gui_log_process,
};

LOG_BACKEND_DEFINE(gui_log_backend, gui_log_backend_api, false);
#endif

// GUI thread function
static void gui_thread_func(void *arg1, void *arg2, void *arg3) {
  // LVGL operations must be performed from the main LVGL thread.
  ARG_UNUSED(arg1);
  ARG_UNUSED(arg2);
  ARG_UNUSED(arg3);

  while (g_gui == nullptr) {
    k_sleep(K_MSEC(40));
  }

  LOG_INF("GUI thread started");

#ifdef CONFIG_LOG
  // Enable log backend from GUI thread context
  log_backend_enable(&gui_log_backend, NULL, LOG_LEVEL_INF);
#endif

  while (1) {
#ifdef CONFIG_LOG
    // Process any pending log messages
    struct log_msg_item msg;
    while (k_msgq_get(&log_msgq, &msg, K_NO_WAIT) == 0) {
      g_gui->add_log_line(msg.data, msg.length);
    }
#endif

    g_gui->run_task_handler();
    k_sleep(K_MSEC(20)); // Run at ~50Hz
  }
}

// Define GUI thread with 1KB stack, priority 7
K_THREAD_DEFINE(gui_thread_id, 1024, gui_thread_func, NULL, NULL, NULL, 7, 0,
                0);

#endif

// Constructor
GUI::GUI(const StateMachine *sm)
    : display_dev(nullptr), main_screen(nullptr), status_label(nullptr),
      log_textarea(nullptr) {
  this->sm = sm;
}

// Destructor
GUI::~GUI() { deinit(); }

void GUI::init_tabview(lv_obj_t *parent) {
  tabview = lv_tabview_create(parent);
  lv_obj_set_size(tabview, LV_HOR_RES, LV_VER_RES - 30);
  lv_obj_align(tabview, LV_ALIGN_BOTTOM_MID, 0, 0);

  // Create tabs
  move_tab = lv_tabview_add_tab(tabview, "Move");
  stack_tab = lv_tabview_add_tab(tabview, "Stack");

  // // Fill each tab with content
  // fill_move_tab(move_tab);
  // fill_stack_tab(stack_tab);
  // fill_status_tab(status_tab);
}

void GUI::add_log_tab() {
  lv_obj_t *log_tab = lv_tabview_add_tab(tabview, "Log");
  log_textarea = lv_textarea_create(log_tab);
  lv_obj_set_size(log_textarea, LV_PCT(90), LV_PCT(80));
  lv_obj_align(log_textarea, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_textarea_set_text(log_textarea, "");
  lv_textarea_set_placeholder_text(log_textarea,
                                   "Log messages will appear here...");
  lv_obj_add_state(log_textarea, LV_STATE_DISABLED); // Make it read-only

  lv_obj_set_scrollbar_mode(log_textarea, LV_SCROLLBAR_MODE_AUTO);
}

// Initialize the GUI
bool GUI::init() {
  LOG_INF("Initializing GUI");

  // Get display device
  display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
  if (!device_is_ready(display_dev)) {
    LOG_ERR("Display device not ready");
    return false;
  }

  // Get current screen
  main_screen = lv_scr_act();

  init_tabview(main_screen);
  add_log_tab();

  // Set background color
  // lv_obj_set_style_bg_color(main_screen, lv_color_black(), 0);

  status_label = lv_label_create(main_screen);
  lv_label_set_text(status_label, "Zephyr Rail Ready");
  lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 10);
  // lv_obj_set_style_text_color(status_label, lv_color_white(), 0);

  lv_timer_handler();
  k_sleep(K_MSEC(20));
  display_blanking_off(display_dev);

  return true;
}

// Static method to initialize the gui
GUI *GUI::initialize_gui(const StateMachine *sm) {
#ifdef CONFIG_DISPLAY
  static GUI gui(sm);
  if (!gui.init()) {
    LOG_ERR("Failed to initialize GUI");
    return nullptr;
  }

  // Set global GUI pointer for thread access
  g_gui = &gui;

  LOG_INF("GUI initialized successfully");
  return &gui;
#else
  return nullptr;
#endif
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
    LOG_DBG("Status: %s", buf);
  } else {
    snprintf(buf, sizeof(buf), "@%dnm", position_nm);
  }
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

void GUI::run_task_handler() {
  struct stepper_with_target_status stepper_status =
      this->sm->get_stepper_status();
  struct stack_status stack_status = this->sm->get_stack_status();

  this->update(&stepper_status, &stack_status);
  lv_task_handler();
  lv_timer_handler();
}

int GUI::position_as_nm(int pitch_per_rev, int pulses_per_rev, int position) {
  if (pulses_per_rev == 0)
    return 0;
  return (position * pitch_per_rev * 1000000) / pulses_per_rev;
}

void GUI::add_log_line(const char *log_data, size_t length) {
  if (!log_textarea) {
    return;
  }
  char log_line[256];
  size_t copy_len = MIN(length, 256 - 1);
  memcpy(log_line, log_data, copy_len);

  log_line[copy_len] = '\0';

  const char *current_text = lv_textarea_get_text(log_textarea);
  static const int MAX_LOG_LINES = 10;
  // Walk backwards from the end, counting newlines
  size_t text_len = strlen(current_text);
  if (text_len > 0) {
    int newline_count = 0;
    const char *p = current_text + text_len - 1;
    while (p > current_text && newline_count < (MAX_LOG_LINES - 1)) {
      if (*p == '\n') {
        newline_count++;
      }
      p--;
    }
    if (newline_count >= (MAX_LOG_LINES - 1)) {
      if (*p == '\n') {
        p++;
      } else {
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
  lv_textarea_add_text(log_textarea, log_line);
  lv_obj_scroll_to_y(log_textarea, LV_COORD_MAX, LV_ANIM_OFF);
}