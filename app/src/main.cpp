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

#ifdef CONFIG_BT
#include <zephyr/bluetooth/bluetooth.h>
#endif
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/smf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/zbus/zbus.h>

#include <zephyr/console/console.h>

#ifdef CONFIG_DISPLAY
#include "GUI.h"
#endif
#include "StateMachine.h"
#include "StepperWithTarget.h"
#ifdef CONFIG_BT
#include "sony_remote.h"
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rail, LOG_LEVEL_DBG);

static const struct gpio_dt_spec stepper_pulse =
    GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 0);
static const struct gpio_dt_spec stepper_dir =
    GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 1);

INPUT_CALLBACK_DEFINE(NULL, input_cb, NULL);

int main(void) {

  // #ifdef CONFIG_BT
  //   if (int err = bt_enable(nullptr); err) {
  //     LOG_ERR("bt_enable failed (%d)\n", err);
  //     return err;
  //   }

  //   LOG_INF("BLE on. Enable 'Bluetooth Rmt Ctrl' on the A7R V and pair on
  //   first "
  //           "connect.\n");

  //   SonyRemote remote;
  //   remote.begin();
  //   remote.startScan();

  //   // bool shot_once = false;

  //   // while (true) {
  //   //   if (!shot_once && remote.ready()) {
  //   //     k_sleep(K_SECONDS(2));
  //   //     remote.focusDown();
  //   //     k_msleep(80);
  //   //     remote.shutterDown();
  //   //     k_msleep(80);
  //   //     remote.shutterUp();
  //   //     k_msleep(50);
  //   //     remote.focusUp();
  //   //     printk("One still shot triggered.\n");
  //   //     shot_once = true;
  //   //   }
  //   //   k_sleep(K_MSEC(200));
  //   // }
  // #endif

  LOG_INF("CONFIG_BOARD=%s", CONFIG_BOARD);
  StepperWithTarget stepper(&stepper_pulse, &stepper_dir);
  StateMachine sm(&stepper);

#ifdef CONFIG_DISPLAY
  GUI *gui = GUI::initialize_gui(&sm);
  if (!gui) {
    LOG_ERR("Failed to initialize GUI");
    return -1;
  }
#endif

  start_stepper(&stepper);

  int32_t ret;

  k_sleep(K_MSEC(200));
  LOG_INF("Start Loop ...");
  while (1) {
    LOG_INF("Iter Loop ...");
    ret = sm.run_state_machine();
    if (ret) {
      break;
    }
  }

  LOG_ERR("Exited the infinite loop...");

#ifdef CONFIG_DISPLAY
  // Clear global GUI pointer on exit
  gui->deinit();
  gui = nullptr;
#endif

  return ret;
}