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

#include "StepperWithTarget.h"
#include "StateMachine.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rail);

static const struct gpio_dt_spec stepper_pulse = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 0);
static const struct gpio_dt_spec stepper_dir = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 1);

INPUT_CALLBACK_DEFINE(NULL, input_cb);

int main(void)
{

  LOG_INF("CONFIG_BOARD=%s", CONFIG_BOARD);
  StepperWithTarget stepper(&stepper_pulse, &stepper_dir);
  StateMachine sm(&stepper);

  start_stepper(&stepper);

  int32_t ret;
  
  struct stepper_with_target_status stepper_status;
  struct stack_status stack_status;
  while(1) {
    ret = sm.run_state_machine();
    if (ret) {
      break;
    }

    /* /1* update GUI *1/ */
    /* stepper_status = sm.get_stepper_status(); */
    /* stack_status = sm.get_stack_status(); */ 
    /* gui.update(&stepper_status, &stack_status); */
    /* gui.run_task_handler(); */
  }

  LOG_ERR("Exited the infinite loop...");

  return ret;
}

