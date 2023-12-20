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

#define SW0_NODE DT_ALIAS(sw0)

// ############################################################################
// initialize Stepper

StepperWithTarget stepper;

void stepper_work_handler(struct k_work *work)
{
  ARG_UNUSED(work);
  stepper.step_towards_target();
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
  stepper.start();
  k_timer_start(&stepper_timer, K_USEC(20), K_USEC(20));
}

// ############################################################################
// initialize ZBus for Stepper
#define MOVE_RELATIVE 0
#define MOVE_ABSOLUTE 1

struct stepper_msg
{
  int diff;
  int mode;
};

// diff -> stepper
ZBUS_CHAN_DEFINE(stepper_diff_chan,  /* Name */
                 struct stepper_msg, /* Message type */

                 NULL,                                           /* Validator */
                 NULL,                                           /* User data */
                 ZBUS_OBSERVERS(sample_diff_lis),                /* observers */
                 ZBUS_MSG_INIT(.diff = 0, .mode = MOVE_RELATIVE) /* Initial value */
);

// ZBUS_SUBSCRIBER_DEFINE(sample_start_sub, 4);

static void sample_diff_listener_cb(const struct zbus_channel *chan)
{
  const struct stepper_msg *data = (const struct stepper_msg *)zbus_chan_const_msg(chan);
  LOG_INF("diff = %d", data->diff);
  switch (data->mode)
  {
  case MOVE_ABSOLUTE:
    stepper.set_target_position(data->diff);
    break;
  case MOVE_RELATIVE:
    stepper.go_relative(data->diff);
    break;
  default:
    LOG_INF("Unrecognized mode %u", data->mode);
  }
}

ZBUS_LISTENER_DEFINE(sample_diff_lis, sample_diff_listener_cb);

// status -> ...
ZBUS_CHAN_DEFINE(model_status_chan,   /* Name */
                 struct model_status, /* Message type */

                 NULL,                 /* Validator */
                 NULL,                 /* User data */
                 ZBUS_OBSERVERS_EMPTY, /* observers */
                 ZBUS_MSG_INIT(.stepper_with_target_status = {.stepper_status = {.direction = 0, .step_jump = 0, .position = 0},
                                                              .is_moving = false,
                                                              .target_position = 0},
                               .upper_bound = 0,
                               .lower_bound = 0,
                               .step_number = 0,
                               .cur_step_index = 0,
                               .stack_in_progress = false) /* Initial value */
);

// ############################################################################
// initialize Display

Display *display_ptr;

// ############################################################################
// initialize Button

static void input_cb(struct input_event *evt)
{

  char buf[1000];
  snprintf(buf, sizeof(buf), "type: %d, code: %d, value: %d", evt->type, evt->code, evt->value);
  display_ptr->set_debug_text(buf);

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

// ############################################################################
// Main

int main(void)
{
  Display display(&model_status_chan);
  display_ptr = &display;
  IrSony irsony;
  Model model(&model_status_chan, &stepper);
  Controller controller(&model, &irsony);

  LOG_INF("stepper = %p", &stepper);

  model.set_upper_bound(12800);
  model.set_step_number(300);
  LOG_INF("model = %p", &model);
  model.log_state();

  controller.prepare_stack();
  LOG_INF("controller = %p", &controller);

  start_stepper();
  while (true)
  {
    model.pub_status();
    display.update_status();
    lv_task_handler();
    controller.work();
    k_sleep(K_MSEC(100));
  }

  return 0;
}
