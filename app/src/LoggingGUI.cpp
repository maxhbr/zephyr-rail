#include "LoggingGUI.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_output.h>

LOG_MODULE_REGISTER(logging_gui, LOG_LEVEL_DBG);

#ifdef CONFIG_DISPLAY

// Global LoggingGUI instance for thread access
static LoggingGUI *g_logging_gui = nullptr;

#ifdef CONFIG_LOG
// Message queue for thread-safe log handling
struct log_msg_item {
  char data[256];
  size_t length;
};

K_MSGQ_DEFINE(log_msgq, sizeof(struct log_msg_item), 200, 4);

// Log output function for GUI backend
static int gui_log_out(uint8_t *data, size_t length, void *ctx) {
  if (g_logging_gui) {
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

// GUI thread function
static void logging_gui_thread_func(void *arg1, void *arg2, void *arg3) {
  // LVGL operations must be performed from the main LVGL thread.
  ARG_UNUSED(arg1);
  ARG_UNUSED(arg2);
  ARG_UNUSED(arg3);

  while (g_logging_gui == nullptr) {
    k_sleep(K_MSEC(40));
  }

  LOG_INF("LoggingGUI thread started");

  // Enable log backend from GUI thread context
  log_backend_enable(&gui_log_backend, NULL, LOG_LEVEL_INF);

  while (1) {
    // Process any pending log messages
    struct log_msg_item msg;
    while (k_msgq_get(&log_msgq, &msg, K_NO_WAIT) == 0) {
      g_logging_gui->add_log_line(msg.data, msg.length);
    }

    g_logging_gui->run_task_handler();
    k_sleep(K_MSEC(20)); // Run at ~50Hz
  }
}

// Define GUI thread with 1KB stack, priority 7
K_THREAD_DEFINE(logging_gui_thread_id, 1024, logging_gui_thread_func, NULL,
                NULL, NULL, 7, 0, 0);
#endif

// Constructor
LoggingGUI::LoggingGUI(const StateMachine *sm)
    : BaseGUI(sm), log_textarea(nullptr) {}

bool LoggingGUI::init() {
  LOG_INF("Initializing LoggingGUI");

  // Initialize base GUI first
  if (!BaseGUI::init()) {
    return false;
  }

  // Add logging functionality
  add_log_tab();

#ifdef CONFIG_LOG
  // Set global logging GUI pointer for thread access
  g_logging_gui = this;
#endif

  LOG_INF("LoggingGUI initialized successfully");
  return true;
}

void LoggingGUI::deinit() {
#ifdef CONFIG_LOG
  g_logging_gui = nullptr;
#endif
  log_textarea = nullptr;
  BaseGUI::deinit();
}

void LoggingGUI::add_log_tab() {
  if (!tabview) {
    LOG_ERR("Tabview not initialized, cannot add log tab");
    return;
  }

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

void LoggingGUI::add_log_line(const char *log_data, size_t length) {
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

#endif