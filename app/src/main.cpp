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
#include <zephyr/smf.h>

#include <zephyr/console/console.h>

#include "stepper/StepperWithTarget.h"
#include "Gui.h"
#include "state/StateMachine.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rail);

static const struct gpio_dt_spec stepper_pulse = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 0);
static const struct gpio_dt_spec stepper_dir = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 1);

static void input_cb(struct input_event *evt)
{
  LOG_DBG("type: %d, code: %d, value: %d", evt->type, evt->code, evt->value);

  if (evt->type != INPUT_EV_KEY)
  {
    return;
  }
  if (evt->value == 0)
  {
    return;
  }
  struct state_msg msg;
  int err;

  switch (evt->code)
  {
  case INPUT_KEY_0:
    msg = {GO_CONTROLLER_ACTION,-1000};
    break;
  case INPUT_KEY_1:
    msg = {GO_CONTROLLER_ACTION,1000};
    break;
  case INPUT_KEY_2:
    msg = {SET_NEW_LOWER_BOUND_ACTION,0};
    break;
  case INPUT_KEY_3:
    msg = {SET_NEW_UPPER_BOUND_ACTION,0};
    break;
  default:
    LOG_DBG("Unrecognized input code %u value %d",
            evt->code, evt->value);
    return;
  }
  err = state_action_pub(&msg);
  if (err == -ENOMSG)
  {
    LOG_INF("Pub an invalid value to a channel with validator successfully.");
  }
}

INPUT_CALLBACK_DEFINE(NULL, input_cb);

int main(void)
{

  LOG_INF("CONFIG_BOARD=%s", CONFIG_BOARD);
  StepperWithTarget stepper(&stepper_pulse, &stepper_dir);
  struct s_object s_obj = init_state_machine(&stepper);

  Gui gui;

  start_stepper(&stepper);
  int32_t ret;
  while(1) {
    ret = run_state_machine(&s_obj);
    if (ret) {
      break;
    }

    /* update GUI */
    gui.update(&s_obj.stepper->get_status(), &s_obj.stack.get_status());
    gui.run_task_handler();

    /* sleep */
    k_msleep(50);
  }

  return ret;
}

