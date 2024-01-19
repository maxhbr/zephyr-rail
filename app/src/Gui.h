#pragma once

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/zephyr.h>
#include <zephyr/types.h>

#include <zephyr/drivers/display.h>
#include <lvgl.h>

/* #include <map> */

#include "stepper/StepperWithTarget.h"
#include "display/Display.h"
#include "Stack.h"


class Gui : public Display
{
private:

  int position_as_nm(const int pitch_per_rev, const int pulses_per_rev, const int position);

  
  lv_obj_t *move_tab;
  lv_obj_t *stack_tab;
  lv_obj_t *config_tab;
  lv_obj_t *status_tab;

  /* void fill_move_panel(lv_obj_t *parent); */
  /* lv_obj_t *pos_label = NULL; */
  /* lv_obj_t *lower_label = NULL; */
  /* lv_obj_t *upper_label = NULL; */
  /* lv_obj_t *step_size_roller = NULL; */
  /* void fill_stack_panel(lv_obj_t *parent); */
  /* lv_obj_t *plan_label = NULL; */
  /* lv_obj_t *step_number_roller = NULL; */
  /* void fill_config_panel(lv_obj_t *parent); */
  /* void fill_status_panel(lv_obj_t *parent); */
  /* lv_obj_t *status_label; */

  /* void update_status_label(const struct model_status status); */

public:
  Gui();

  void update(const struct stepper_with_target_status *stepper_with_target_status,
      const struct stack_status *stack_status);
};
