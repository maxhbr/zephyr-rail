/*
 * Copyright (c) 2018 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/types.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>

#include <zephyr/console/console.h>

#include <lvgl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rail);

#include "ir/IrSony.h"
#include "stepper/StepperWithTarget.h"
#include "display/Display.h"
#include "mvc/Controller.h"
#include "mvc/Model.h"
#include "mvc/View.h"

#define SW0_NODE DT_ALIAS(sw0)

StepperWithTarget *stepper_ptr;
Controller *controller_ptr;
Model *model_ptr;
View *view_ptr;

static const struct gpio_dt_spec stepper_pulse = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 0);
static const struct gpio_dt_spec stepper_dir = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 1);

// ############################################################################
// initialize Button

#if 0
static bool key_1_pressed = false;
static void input_cb(struct input_event *evt)
{

  char buf[1000];
  /* LOG_DBG("type: %d, code: %d, value: %d", evt->type, evt->code, evt->value); */
  LOG_INF("type: %d, code: %d, value: %d", evt->type, evt->code, evt->value);

  if (evt->type != INPUT_EV_KEY)
  {
    return;
  }
  if (evt->value == 0)
  {
    if(evt->code == INPUT_KEY_1) {
      key_1_pressed = false;
    }
    return;
  }
  int err;
  struct controller_msg msg;

  switch (evt->code)
  {
  case INPUT_KEY_2:
    if(key_1_pressed) {
      msg = {SET_NEW_LOWER_BOUND_ACTION,0};
    } else {
      msg = {GO_CONTROLLER_ACTION,-1000};
    }
    err = zbus_chan_pub(&controller_msg_chan, &msg, K_MSEC(200));
    break;
  case INPUT_KEY_1:
    key_1_pressed = true;
    break;
  case INPUT_KEY_0:
    if(key_1_pressed) {
      msg = {SET_NEW_UPPER_BOUND_ACTION,0};
    } else {
      msg = {GO_CONTROLLER_ACTION, 1000};
    }
    err = zbus_chan_pub(&controller_msg_chan, &msg, K_MSEC(200));
    break;
  case INPUT_KEY_ENTER:
    break;
  case INPUT_KEY_DOWN:
    break;
  case INPUT_KEY_UP:
    break;
  case INPUT_KEY_LEFT:
    break;
  case INPUT_KEY_RIGHT:
    break;
  default:
    LOG_DBG("Unrecognized input code %u value %d",
            evt->code, evt->value);
    return;
  }
  if (err == -ENOMSG)
  {
    LOG_INF("Pub an invalid value to a channel with validator successfully.");
  }
}

INPUT_CALLBACK_DEFINE(NULL, input_cb);
#endif

// ############################################################################
// Main

int main(void)
{
  LOG_INF("CONFIG_BOARD=%s", CONFIG_BOARD);
  StepperWithTarget stepper;
  stepper_ptr = &stepper;
  IrSony irsony;
  Model model(stepper_ptr);
  model_ptr = &model;
  Controller controller(&model, &irsony);
  controller_ptr = &controller;
  set_controller_ptr_for_zbus(controller);
  View view(&model, &controller);

  LOG_INF("Initialize model");
  controller.prepare_stack();
  model.log_state();

  LOG_INF("Start main loop");
  k_sleep(K_MSEC(100));
  /* start_stepper(stepper_ptr); */
  while (true)
  {
    k_sleep(K_MSEC(100));
    view.update();
    /* controller.work(); */
  }

  return 0;
}
