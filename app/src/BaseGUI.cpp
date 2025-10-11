#include "BaseGUI.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(base_gui, LOG_LEVEL_DBG);

#ifdef CONFIG_DISPLAY

// Constructor
BaseGUI::BaseGUI(const StateMachine *sm)
    : display_dev(nullptr), main_screen(nullptr), status_label(nullptr),
      tabview(nullptr) {
  this->sm = sm;
}

// Destructor
BaseGUI::~BaseGUI() { deinit(); }

void BaseGUI::init_tabview(lv_obj_t *parent) {
  tabview = lv_tabview_create(parent);
  lv_obj_set_size(tabview, LV_HOR_RES, LV_VER_RES - 30);
  lv_obj_align(tabview, LV_ALIGN_BOTTOM_MID, 0, 0);

  // Create basic tabs
  lv_obj_t *move_tab = lv_tabview_add_tab(tabview, "Move");
  lv_obj_t *stack_tab = lv_tabview_add_tab(tabview, "Stack");

  // // Fill each tab with content
  // fill_move_tab(move_tab);
  // fill_stack_tab(stack_tab);
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

  lv_timer_handler();
  k_sleep(K_MSEC(20));
  display_blanking_off(display_dev);

  return true;
}

// Deinitialize the GUI
void BaseGUI::deinit() {
  // Objects are automatically cleaned up by LVGL
  main_screen = nullptr;
  status_label = nullptr;
  tabview = nullptr;
}

// Update GUI with current status
void BaseGUI::update(const struct stepper_with_target_status *stepper_status,
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
void BaseGUI::set_status(const char *status) {
  if (!status)
    return;

  if (status_label) {
    lv_label_set_text(status_label, status);
  }

  LOG_INF("Status: %s", status);
}

void BaseGUI::run_task_handler() {
  struct stepper_with_target_status stepper_status =
      this->sm->get_stepper_status();
  struct stack_status stack_status = this->sm->get_stack_status();

  this->update(&stepper_status, &stack_status);
  lv_task_handler();
  lv_timer_handler();
}

int BaseGUI::position_as_nm(int pitch_per_rev, int pulses_per_rev,
                            int position) {
  if (pulses_per_rev == 0)
    return 0;
  return (position * pitch_per_rev * 1000000) / pulses_per_rev;
}

#endif