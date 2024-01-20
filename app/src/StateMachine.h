#pragma once

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>

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
