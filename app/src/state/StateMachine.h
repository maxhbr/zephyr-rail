#pragma once

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/input/input.h>

#include <zephyr/smf.h>

#include <zephyr/logging/log.h>

#include "../stepper/StepperWithTarget.h"
#include "Stack.h"

/* enum state_action { */
/*   NOOP_CONTROLLER_ACTION, */
/*   GO_CONTROLLER_ACTION, */
/*   GO_TO_CONTROLLER_ACTION, */
/*   SET_NEW_LOWER_BOUND_ACTION, */
/*   SET_NEW_UPPER_BOUND_ACTION, */
/*   // Stacking */
/*   START_STACK */
/* }; */

/* struct state_msg */
/* { */
/*   state_action action; */
/*   int value; */
/* }; */
/* int state_action_pub(state_msg *msg); */

enum event {
  EVENT_INPUT_KEY_0,
  EVENT_INPUT_KEY_1,
  EVENT_INPUT_KEY_2,
  EVENT_INPUT_KEY_ENTER,
  EVENT_INPUT_KEY_DOWN,
  EVENT_INPUT_KEY_UP,
  EVENT_INPUT_KEY_LEFT,
  EVENT_INPUT_KEY_RIGHT
};
struct event_msg
{
  std::optional<event> event;
};
int event_pub(event event); 
void input_cb(struct input_event *evt);

enum stack_state { 
  S0,

  S_PARENT_INTERACTIVE,
  S_INTERACTIVE_MOVE,
  S_INTERACTIVE_PRE_STACKING,

  S_PARENT_STACKING,
  S_STACK,
  S_STACK_MOVE,
  S_STACK_IMG
};

struct s_object {
    struct smf_ctx ctx;
    /* Other state specific data add here */
    const StepperWithTarget *stepper;
    const Stack stack;
};

class StateMachine
{
  struct s_object s_obj; 
public:
  StateMachine(const StepperWithTarget *stepper);

  int32_t run_state_machine();

  const struct stepper_with_target_status get_stepper_status();
  const struct stack_status get_stack_status();
};

