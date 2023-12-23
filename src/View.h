#ifndef __VIEW_H_
#define __VIEW_H_

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

#include <map>

#include "Controller.h"
#include "Display.h"
#include "Model.h"
#include "Stepper.h"

class View : public Display
{
private:
  Model *model;
  Controller *controller;

  lv_obj_t *move_tab;
  lv_obj_t *stack_tab;
  lv_obj_t *config_tab;
  lv_obj_t *status_tab;

  void fill_move_panel(lv_obj_t *parent);
  lv_obj_t *pos_label = NULL;
  lv_obj_t *lower_label = NULL;
  lv_obj_t *upper_label = NULL;
  lv_obj_t *step_size_roller = NULL;
  void fill_stack_panel(lv_obj_t *parent);
  lv_obj_t *plan_label = NULL;
  lv_obj_t *step_number_roller = NULL;
  void fill_config_panel(lv_obj_t *parent);
  void fill_status_panel(lv_obj_t *parent);
  lv_obj_t *status_label;

  void update_status_label(const struct model_status status);

public:
  View(Model *_model, Controller *_controller);

  void update();
};

#endif // __VIEW_H_