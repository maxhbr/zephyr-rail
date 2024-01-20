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

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rail);

#include "stepper/StepperWithTarget.h"
#include "Gui.h"
#include "Stack.h"
#include "StateMachine.h"

#if 0
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
  int err;

  switch (evt->code)
  {
  case INPUT_KEY_0:
    LOG_INF("-...");
    s_obj.stepper->go_relative(-10000);
    break;
  case INPUT_KEY_1:
    LOG_INF("+...");
    s_obj.stepper->go_relative(10000);
    break;
  case INPUT_KEY_2:
    s_obj.stack->set_lower_bound(s_obj.stepper->get_target_position());
    break;
  case INPUT_KEY_3:
    s_obj.stack->set_upper_bound(s_obj.stepper->get_target_position());
    break;
  /* case INPUT_KEY_ENTER: */
  /*   break; */
  /* case INPUT_KEY_DOWN: */
  /*   break; */
  /* case INPUT_KEY_UP: */
  /*   break; */
  /* case INPUT_KEY_LEFT: */
  /*   break; */
  /* case INPUT_KEY_RIGHT: */
  /*   break; */
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

static const struct gpio_dt_spec stepper_pulse = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 0);
static const struct gpio_dt_spec stepper_dir = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 1);

int main(void)
{

  LOG_INF("CONFIG_BOARD=%s", CONFIG_BOARD);
  StepperWithTarget stepper(&stepper_pulse, &stepper_dir);
  Stack stack;
  Gui gui;
  struct s_object s_obj = init_state_machine(&stepper, &stack, &gui);

  start_stepper(&stepper);
  int32_t ret;
  while(1) {
    ret = run_state_machine();
    if (ret) {
      break;
    }

    /* update GUI */
    gui.update(&s_obj.stepper->get_status(), &s_obj.stack->get_status());
    gui.run_task_handler();

    /* sleep */
    k_msleep(50);
  }
}

