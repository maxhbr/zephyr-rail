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
#if 0
#include "ir/IrSony.h"
#include "stepper/StepperWithTarget.h"
#include "display/Display.h"
#include "mvc/Controller.h"
#include "mvc/Model.h"
#include "mvc/View.h"

static const struct gpio_dt_spec stepper_pulse = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 0);
static const struct gpio_dt_spec stepper_dir = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 1);

// ############################################################################
// initialize Button

// ############################################################################
// Main

int main(void)
{

  StepperWithTarget *stepper_ptr;
  Controller *controller_ptr;
  Model *model_ptr;
  View *view_ptr;

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
#endif



enum stack_state { S0, S_IDLE, S_INTERACTIVE_MOVE };

struct s_object {
    struct smf_ctx ctx;
    /* Other state specific data add here */
    StepperWithTarget *stepper;
    Stack *stack;
    Gui *gui;
} s_obj;


struct smf_state *s0_state_ptr;
struct smf_state *s_idle_state_ptr;
struct smf_state *s_interactive_move_ptr;

static void s0_entry(void *o)
{
  LOG_DBG("s0_entry");
}
static void s0_run(void *o)
{
  LOG_DBG("s0_run");
  smf_set_state(SMF_CTX(&s_obj), s_idle_state_ptr);
}
static void s0_exit(void *o)
{
  LOG_DBG("s0_exit");
}

static void s_idle_entry(void *o)
{
  LOG_INF("S_IDLE...");
  struct s_object *s = (struct s_object *)o;
  s->stepper->log_state();
}
static void s_idle_run(void *o)
{
  LOG_DBG("s_idle_run");
  struct s_object *s = (struct s_object *)o;
  if (!s->stepper->is_in_target_position()) {
    smf_set_state(SMF_CTX(&s_obj), s_interactive_move_ptr);
  }
}
static void s_idle_exit(void *o)
{
  LOG_DBG("s_idle_exit");
}

static void s_interactive_move_run(void *o)
{
  LOG_DBG("s_interactive_move_run");
  struct s_object *s = (struct s_object *)o;
  if (s->stepper->is_in_target_position()) {
    smf_set_state(SMF_CTX(&s_obj), s_idle_state_ptr);
  }
}

static const struct smf_state stack_states[] = {
        [S0] = SMF_CREATE_STATE(s0_entry, s0_run, s0_exit),
        [S_IDLE] = SMF_CREATE_STATE(s_idle_entry, s_idle_run, s_idle_exit),
        [S_INTERACTIVE_MOVE] = SMF_CREATE_STATE(NULL, s_interactive_move_run, NULL),
};

static void set_state_ptrs() {
  s0_state_ptr = &stack_states[S0];
  s_idle_state_ptr = &stack_states[S_IDLE];
  s_interactive_move_ptr = &stack_states[S_INTERACTIVE_MOVE];
}

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

static const struct gpio_dt_spec stepper_pulse = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 0);
static const struct gpio_dt_spec stepper_dir = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 1);

int main(void)
{
  set_state_ptrs();


  StepperWithTarget stepper(&stepper_pulse, &stepper_dir);
  s_obj.stepper = &stepper;

  Stack stack;
  s_obj.stack = &stack;

  Gui gui;
  s_obj.gui = &gui;

  LOG_INF("CONFIG_BOARD=%s", CONFIG_BOARD);
  int32_t ret;

  smf_set_initial(SMF_CTX(&s_obj), s0_state_ptr);

  start_stepper(&stepper);
  while(1) {
    /* Run the state machine */
    ret = smf_run_state(SMF_CTX(&s_obj));
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

