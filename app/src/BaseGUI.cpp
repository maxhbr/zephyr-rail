#include "BaseGUI.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(base_gui, LOG_LEVEL_INF);

static BaseGUI *g_gui = nullptr;

static void gui_thread_func(void *arg1, void *arg2, void *arg3) {
  // LVGL operations must be performed from the main LVGL thread.
  ARG_UNUSED(arg1);
  ARG_UNUSED(arg2);
  ARG_UNUSED(arg3);

  LOG_INF("GUI thread waiting...");
  k_sleep(K_MSEC(100));

  while (g_gui == nullptr) {
    k_sleep(K_MSEC(40));
    LOG_INF("GUI thread still waiting...");
  }
  k_sleep(K_MSEC(100));

  LOG_INF("GUI thread started");

  while (1) {
    g_gui->run_task_handler();
    k_sleep(K_MSEC(20)); // Run at ~50Hz
  }
}

K_THREAD_DEFINE(logging_gui_thread_id, 8096, gui_thread_func, NULL, NULL, NULL,
                7, 0, 0);

// Constructor
BaseGUI::BaseGUI()
    : display_dev(nullptr), main_screen(nullptr), status_label(nullptr),
      tabview(nullptr) {}

// Destructor
BaseGUI::~BaseGUI() { deinit(); }

void BaseGUI::init_tabview(lv_obj_t *parent) {
  tabview = lv_tabview_create(parent);
  lv_obj_set_size(tabview, LV_HOR_RES, LV_VER_RES - 30);
  lv_obj_align(tabview, LV_ALIGN_BOTTOM_MID, 0, 0);
  // lv_tabview_set_tab_bar_size(tabview, 22);
  lv_tabview_set_tab_bar_position(tabview, LV_DIR_LEFT);
  lv_tabview_set_tab_bar_size(tabview, 60);
}

// Initialize the GUI
bool BaseGUI::init() {
  LOG_INF("Initializing BaseGUI");

  // Get display device
  display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
  if (!device_is_ready(display_dev)) {
    LOG_ERR("Display device not ready");
    return false;
  }

  // Get current screen
  main_screen = lv_scr_act();

  init_tabview(main_screen);

  // Set background color
  // lv_obj_set_style_bg_color(main_screen, lv_color_black(), 0);

  status_label = lv_label_create(main_screen);
  lv_label_set_text(status_label, "Zephyr Rail Ready");
  lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 10);
  // lv_obj_set_style_text_color(status_label, lv_color_white(), 0);

  g_gui = this;

  LOG_INF("BaseGUI initialized successfully");

  return true;
}

void BaseGUI::start() {
  lv_timer_handler();
  k_sleep(K_MSEC(20));
  display_blanking_off(display_dev);
}

// Deinitialize the GUI
void BaseGUI::deinit() {
  // Objects are automatically cleaned up by LVGL
  main_screen = nullptr;
  status_label = nullptr;
  tabview = nullptr;
  g_gui = nullptr;
}
// Set main status
void BaseGUI::set_status(const char *status) {
  if (!status)
    return;

  if (status_label) {
    lv_label_set_text(status_label, status);
  }
  LOG_DBG("Status: %s", status);
}

void BaseGUI::run_task_handler() {
  lv_task_handler();
  lv_timer_handler();
}