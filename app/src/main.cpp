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
#include <zephyr/smf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/zbus/zbus.h>

#include <zephyr/console/console.h>

#include "GUI.h"
#include "StateMachine.h"
#include "StepperWithTarget.h"
#include "sony_remote.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rail);

// Global GUI instance for thread access
static GUI *g_gui = nullptr;

// GUI thread function
static void gui_thread_func(void *arg1, void *arg2, void *arg3) {
  ARG_UNUSED(arg1);
  ARG_UNUSED(arg2);
  ARG_UNUSED(arg3);

  while (g_gui == nullptr) {
    k_sleep(K_MSEC(10));
  }

  LOG_INF("GUI thread started");

  while (1) {
    g_gui->run_task_handler();
    k_sleep(K_MSEC(5)); // Run at ~200Hz
  }
}

// Define GUI thread with 1KB stack, priority 7
K_THREAD_DEFINE(gui_thread_id, 1024, gui_thread_func, NULL, NULL, NULL, 7, 0,
                0);

#if 1
static const struct gpio_dt_spec stepper_pulse =
    GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 0);
static const struct gpio_dt_spec stepper_dir =
    GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 1);

INPUT_CALLBACK_DEFINE(NULL, input_cb, NULL);

int main(void) {

#ifdef CONFIG_BT
  if (int err = bt_enable(nullptr); err) {
    LOG_ERR("bt_enable failed (%d)\n", err);
    return err;
  }

  LOG_INF("BLE on. Enable 'Bluetooth Rmt Ctrl' on the A7R V and pair on first "
          "connect.\n");

  SonyRemote remote;
  remote.begin();
  remote.startScan();
#endif

  LOG_INF("CONFIG_BOARD=%s", CONFIG_BOARD);
  StepperWithTarget stepper(&stepper_pulse, &stepper_dir);
  StateMachine sm(&stepper);

  GUI gui(&sm);
  if (!gui.init()) {
    LOG_ERR("Failed to initialize GUI");
    return -1;
  }

  // Set global GUI pointer for thread access
  g_gui = &gui;

  start_stepper(&stepper);

  int32_t ret;

  LOG_INF("Start Loop ...");
  while (1) {
    ret = sm.run_state_machine();
    if (ret) {
      break;
    }
  }

  LOG_ERR("Exited the infinite loop...");

  // Clear global GUI pointer on exit
  g_gui = nullptr;

  return ret;
}

#else

int main(void) {
  if (int err = bt_enable(nullptr); err) {
    printk("bt_enable failed (%d)\n", err);
    return err;
  }

  printk("BLE on. Enable 'Bluetooth Rmt Ctrl' on the A7R V and pair on first "
         "connect.\n");

  SonyRemote remote;
  remote.begin();
  remote.startScan();

  bool shot_once = false;

  while (true) {
    if (!shot_once && remote.ready()) {
      k_sleep(K_SECONDS(2));
      remote.focusDown();
      k_msleep(80);
      remote.shutterDown();
      k_msleep(80);
      remote.shutterUp();
      k_msleep(50);
      remote.focusUp();
      printk("One still shot triggered.\n");
      shot_once = true;
    }
    k_sleep(K_MSEC(200));
  }
}

#endif