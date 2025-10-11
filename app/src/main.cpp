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

#ifdef CONFIG_DISPLAY
// Global GUI instance for thread access
static GUI *g_gui = nullptr;

#ifdef CONFIG_LOG
// Message queue for thread-safe log handling
struct log_msg_item {
  char data[256];
  size_t length;
};

K_MSGQ_DEFINE(log_msgq, sizeof(struct log_msg_item), 200, 4);

// Log output function for GUI backend
static int gui_log_out(uint8_t *data, size_t length, void *ctx) {
  if (g_gui) {
    struct log_msg_item msg;
    size_t copy_len = MIN(length, sizeof(msg.data) - 1);
    memcpy(msg.data, data, copy_len);
    msg.data[copy_len] = '\0';
    msg.length = copy_len;

    // Try to put message in queue (non-blocking)
    k_msgq_put(&log_msgq, &msg, K_NO_WAIT);
  }
  return length;
}

LOG_OUTPUT_DEFINE(gui_log_output, gui_log_out, NULL, 0);

// Log backend process function
static void gui_log_process(const struct log_backend *const backend,
                            union log_msg_generic *msg) {
  log_format_func_t log_output_func = log_format_func_t_get(LOG_OUTPUT_TEXT);
  log_output_func(&gui_log_output, &msg->log, 0);
}

// Log backend definition
static const struct log_backend_api gui_log_backend_api = {
    .process = gui_log_process,
};

LOG_BACKEND_DEFINE(gui_log_backend, gui_log_backend_api, false);
#endif

// GUI thread function
static void gui_thread_func(void *arg1, void *arg2, void *arg3) {
  // LVGL operations must be performed from the main LVGL thread.
  ARG_UNUSED(arg1);
  ARG_UNUSED(arg2);
  ARG_UNUSED(arg3);

  while (g_gui == nullptr) {
    k_sleep(K_MSEC(40));
  }

  LOG_INF("GUI thread started");

#ifdef CONFIG_LOG
  // Enable log backend from GUI thread context
  log_backend_enable(&gui_log_backend, NULL, LOG_LEVEL_INF);
#endif

  while (1) {
#ifdef CONFIG_LOG
    // Process any pending log messages
    struct log_msg_item msg;
    while (k_msgq_get(&log_msgq, &msg, K_NO_WAIT) == 0) {
      g_gui->add_log_line(msg.data, msg.length);
    }
#endif

    g_gui->run_task_handler();
    k_sleep(K_MSEC(20)); // Run at ~50Hz
  }
}

// Define GUI thread with 1KB stack, priority 7
K_THREAD_DEFINE(gui_thread_id, 1024, gui_thread_func, NULL, NULL, NULL, 7, 0,
                0);

#endif

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
  GUI gui(&sm);
  if (!gui.init()) {
    LOG_ERR("Failed to initialize GUI");
    return -1;
  }

  // Set global GUI pointer for thread access
  g_gui = &gui;
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
  g_gui->deinit();
  g_gui = nullptr;
#endif

  return ret;
}