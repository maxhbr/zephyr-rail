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

#include "stepper/StepperWithTarget.h"
#include "Gui.h"
#include "Stack.h"


enum stack_state { S0, S_IDLE, S_INTERACTIVE_MOVE };

struct s_object {
    struct smf_ctx ctx;
    /* Other state specific data add here */
    StepperWithTarget *stepper;
    Stack *stack;
    Gui *gui;
};


struct s_object init_state_machine(const StepperWithTarget *stepper, const Stack * stack, const Gui * gui);
int32_t run_state_machine();


#define NOOP_CONTROLLER_ACTION 0
#define GO_CONTROLLER_ACTION 1
#define GO_TO_CONTROLLER_ACTION 2
#define SET_NEW_LOWER_BOUND_ACTION 3
#define SET_NEW_UPPER_BOUND_ACTION 4

struct controller_msg
{
  int action;
  int value;
};

int controller_action_pub(controller_msg *msg);
