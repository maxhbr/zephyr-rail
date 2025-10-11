#include "GUI.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gui, LOG_LEVEL_INF);

// Constructor
GUI::GUI(const StateMachine *sm) : LoggingGUI() { this->sm = sm; }

bool GUI::init() {
  LOG_INF("Initializing GUI");

  // Initialize base LoggingGUI first
  if (!LoggingGUI::init()) {
    return false;
  }

  add_move_tab();
  add_stack_tab();

  LOG_INF("GUI initialized successfully");
  return true;
}

static const char *btnm_map[] = {"|<", "<",   ">", ">|", "\n",
                                 "v",  "???", "^", ""};

void GUI::add_move_tab() {
  if (!tabview) {
    LOG_ERR("Tabview not initialized, cannot add move tab");
    return;
  }

  lv_obj_t *move_tab = lv_tabview_add_tab(tabview, "Move");

  lv_obj_t *btnm1 = lv_buttonmatrix_create(move_tab);
  lv_buttonmatrix_set_map(btnm1, btnm_map);
  lv_buttonmatrix_set_button_width(btnm1, 1, 2);
  lv_buttonmatrix_set_button_width(btnm1, 2, 2);
  lv_buttonmatrix_set_button_ctrl(btnm1, 5, LV_BUTTONMATRIX_CTRL_DISABLED);
  lv_buttonmatrix_set_button_width(btnm1, 5, 4);
  // lv_buttonmatrix_set_button_ctrl(btnm1, 11, LV_BUTTONMATRIX_CTRL_CHECKED);
  lv_obj_align(btnm1, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_size(btnm1, LV_PCT(100), LV_PCT(100));
}

void GUI::add_stack_tab() {
  if (!tabview) {
    LOG_ERR("Tabview not initialized, cannot add stack tab");
    return;
  }

  lv_obj_t *stack_tab = lv_tabview_add_tab(tabview, "Stack");
  // Add content to stack_tab as needed
  lv_obj_t *label = lv_label_create(stack_tab);
  lv_label_set_text(label, "Stack Tab Content");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

void GUI::deinit() { LoggingGUI::deinit(); }

void GUI::run_task_handler() {
  struct stepper_with_target_status stepper_status =
      this->sm->get_stepper_status();
  struct stack_status stack_status = this->sm->get_stack_status();

  this->update_status(&stepper_status, &stack_status);
  LoggingGUI::run_task_handler();
}

void GUI::start() { LoggingGUI::start(); }

// Update GUI with current status
void GUI::update_status(const struct stepper_with_target_status *stepper_status,
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
  set_status(buf);
}

int GUI::position_as_nm(int pitch_per_rev, int pulses_per_rev, int position) {
  if (pulses_per_rev == 0)
    return 0;
  return (position * pitch_per_rev * 1000000) / pulses_per_rev;
}