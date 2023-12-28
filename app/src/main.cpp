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

#include "IrSony.h"
#include "StepperWithTarget.h"
#include "Controller.h"
#include "Model.h"
#include "Display.h"
#include "View.h"

#define SW0_NODE DT_ALIAS(sw0)

StepperWithTarget *stepper_ptr;
Controller *controller_ptr;
Model *model_ptr;
View *view_ptr;

// ############################################################################
// initialize Stepper

void stepper_work_handler(struct k_work *work)
{
  ARG_UNUSED(work);
  if (stepper_ptr == NULL)
  {
    return;
  }
  stepper_ptr->step_towards_target();
}
K_WORK_DEFINE(stepper_work, stepper_work_handler);
void stepper_expiry_function(struct k_timer *timer_id)
{
  ARG_UNUSED(timer_id);
  k_work_submit(&stepper_work);
}
K_TIMER_DEFINE(stepper_timer, stepper_expiry_function, NULL);
void start_stepper()
{
  if (stepper_ptr == NULL)
  {
    return;
  }
  stepper_ptr->start();
  k_timer_start(&stepper_timer, K_USEC(20), K_USEC(20));
}

// ############################################################################
// initialize ZBus for Stepper
ZBUS_CHAN_DEFINE(controller_msg_chan,   /* Name */
                 struct controller_msg, /* Message type */
                 NULL,
                 NULL,
                 ZBUS_OBSERVERS(controller_action_listener),
                 ZBUS_MSG_INIT(.action = NOOP_CONTROLLER_ACTION, .value = 0));

static void controller_action_listener_cb(const struct zbus_channel *chan)
{
  if (controller_ptr == NULL)
  {
    return;
  }
  const struct controller_msg *msg = (const struct controller_msg *)zbus_chan_const_msg(chan);
  controller_ptr->handle_controller_msg(msg);
}

ZBUS_LISTENER_DEFINE(controller_action_listener, controller_action_listener_cb);

// ############################################################################
// initialize Button

#if 0
static void input_cb(struct input_event *evt)
{

  char buf[1000];
  snprintf(buf, sizeof(buf), "type: %d, code: %d, value: %d", evt->type, evt->code, evt->value);
  // display_ptr->set_debug_text(buf);

  if (evt->type != INPUT_EV_KEY)
  {
    return;
  }
  if (evt->value == 0)
  {
    return;
  }
  int err;
  struct stepper_msg msg;

  switch (evt->code)
  {
  case INPUT_KEY_2:
    msg = {-1000, MOVE_RELATIVE};
    err = zbus_chan_pub(&stepper_diff_chan, &msg, K_MSEC(200));
    break;
  case INPUT_KEY_1:
    msg = {0, MOVE_ABSOLUTE};
    err = zbus_chan_pub(&stepper_diff_chan, &msg, K_MSEC(200));
    break;
  case INPUT_KEY_0:
    msg = {1000, MOVE_RELATIVE};
    err = zbus_chan_pub(&stepper_diff_chan, &msg, K_MSEC(200));
    break;
  case INPUT_KEY_ENTER:
    msg = {0, MOVE_ABSOLUTE};
    err = zbus_chan_pub(&stepper_diff_chan, &msg, K_MSEC(200));
    break;
  case INPUT_KEY_DOWN:
    msg = {102, MOVE_RELATIVE};
    err = zbus_chan_pub(&stepper_diff_chan, &msg, K_MSEC(200));
    break;
  case INPUT_KEY_UP:
    msg = {103, MOVE_RELATIVE};
    err = zbus_chan_pub(&stepper_diff_chan, &msg, K_MSEC(200));
    break;
  case INPUT_KEY_LEFT:
    msg = {104, MOVE_RELATIVE};
    err = zbus_chan_pub(&stepper_diff_chan, &msg, K_MSEC(200));
    break;
  case INPUT_KEY_RIGHT:
    msg = {105, MOVE_RELATIVE};
    err = zbus_chan_pub(&stepper_diff_chan, &msg, K_MSEC(200));
    break;
  default:
    LOG_INF("Unrecognized input code %u value %d",
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
  // Display display(&model_status_chan);
  // display_ptr = &display;
  IrSony irsony;
  Model model(stepper_ptr);
  model_ptr = &model;
  Controller controller(&model, &irsony);
  controller_ptr = &controller;
  View view(&model, &controller);

  LOG_INF("Initialize model");
  model.set_upper_bound(25600);
  model.set_step_number(300);
  controller.prepare_stack();
  model.log_state();

  LOG_INF("Start main loop");
  k_sleep(K_MSEC(100));
  start_stepper();
  while (true)
  {
    k_sleep(K_MSEC(100));
    view.update();
    controller.work();
  }

  return 0;
}
