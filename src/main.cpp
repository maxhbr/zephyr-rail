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
  stepper.go_relative(data->diff);
}

ZBUS_LISTENER_DEFINE(sample_diff_lis, sample_diff_listener_cb);

// status -> ...
ZBUS_CHAN_DEFINE(stepper_status_chan,               /* Name */
                 struct stepper_with_target_status, /* Message type */

                 NULL,                            /* Validator */
                 NULL,                            /* User data */
                 ZBUS_OBSERVERS(sample_diff_lis), /* observers */
                 ZBUS_MSG_INIT(.stepper_status = {.direction = 0, .step_jump = 0, .position = 0},
                               .is_moving = false,
                               .target_position = 0) /* Initial value */
);

// ############################################################################
// initialize Display
Display display(&stepper_status_chan);

// ############################################################################
// initialize IrSony
IrSony irsony;

// // ############################################################################
// // initialize Button
// #define SW0_GPIO_LABEL DT_GPIO_LABEL(SW0_NODE, gpios)
// #define SW0_GPIO_PIN DT_GPIO_PIN(SW0_NODE, gpios)
// #define SW0_GPIO_FLAGS (GPIO_INPUT | DT_GPIO_FLAGS(SW0_NODE, gpios))
// static struct gpio_callback button_cb_data;
// void button_pressed(const struct device *dev, struct gpio_callback *cb,
// uint32_t pins)
// {
// ARG_UNUSED(dev);
// ARG_UNUSED(cb);
// ARG_UNUSED(pins);

// printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
// }
// void init_button()
// {
// const struct device *button;
// button = device_get_binding(SW0_GPIO_LABEL);
// gpio_pin_configure(button, SW0_GPIO_PIN, SW0_GPIO_FLAGS);
// gpio_pin_interrupt_configure(button, SW0_GPIO_PIN, GPIO_INT_EDGE_TO_ACTIVE);
// gpio_init_callback(&button_cb_data, button_pressed, BIT(SW0_GPIO_PIN));
// gpio_add_callback(button, &button_cb_data);
// }

// ############################################################################
// initialize Display

// Display get_display()
// {
//   const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
//   __ASSERT(display_dev != NULL, "display device not found.");
//   if (!device_is_ready(display_dev))
//   {
//     LOG_ERR("Device not ready");
//     // return 1;
//   }
//   Display display(display_dev);
//   return display;
// }

// ############################################################################
// Main

int main(void)
{

  // const struct device *display_dev;
  // lv_obj_t *hello_world_label;

  // display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
  // if (!device_is_ready(display_dev))
  // {
  //   LOG_ERR("Device not ready, aborting test");
  //   return 0;
  // }
  // hello_world_label = lv_label_create(lv_scr_act());
  // lv_label_set_text(hello_world_label, "Hello world!");
  // lv_obj_align(hello_world_label, LV_ALIGN_CENTER, 0, 0);

  // lv_task_handler();
  // display_blanking_off(display_dev);
  // lv_task_handler();

  LOG_INF("stepper = %p", &stepper);

  // init_button();
  Model model(&stepper);
  model.set_upper_bound(12800);
  model.set_step_number(300);
  LOG_INF("model = %p", &model);
  model.log_state();

  Controller controller(&model, &irsony);
  controller.prepare_stack();
  LOG_INF("controller = %p", &controller);

  start_stepper();
  while (true)
  {
    display.update_status();
    lv_task_handler();
    controller.work();
    k_sleep(K_MSEC(100));
  }

  return 0;
}
