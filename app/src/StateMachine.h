#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/zbus/zbus.h>

#include <zephyr/smf.h>

#include <zephyr/logging/log.h>

#include "Stack.h"
#ifdef CONFIG_BT
#include "sony_remote/sony_remote.h"
#else
#include "sony_remote/fake_sony_remote.h"
#endif
#include "stepper_with_target/StepperWithTarget.h"

enum event {
  EVENT_GO,
  EVENT_GO_TO,
  EVENT_GO_PCT,
  EVENT_SET_LOWER_BOUND,
  EVENT_SET_UPPER_BOUND,
  EVENT_SET_LOWER_BOUND_TO,
  EVENT_SET_UPPER_BOUND_TO,
  EVENT_SET_WAIT_BEFORE_MS,
  EVENT_SET_WAIT_AFTER_MS,
  EVENT_SET_SPEED,
  EVENT_SET_SPEED_RPM,
  EVENT_DISABLE,
  EVENT_CAMERA_START_SCAN,
  EVENT_CAMERA_STOP_SCAN,
  EVENT_START_STACK_WITH_STEP_SIZE,
  EVENT_START_STACK_WITH_LENGTH,
  EVENT_SHOOT,
  EVENT_RECORD,
  EVENT_STATUS,
};
struct event_msg {
  std::optional<event> evt;
  int value;
};
int event_pub(event event);
int event_pub(event event, int value);
void input_cb(struct input_event *evt, void *user_data);

enum stack_state {
  S0,

  S_PARENT_INTERACTIVE,
  S_INTERACTIVE,

  S_PARENT_STACKING,
  S_STACK,
  S_STACK_MOVE,
  S_STACK_SETTLE,
  S_STACK_IMG
};

struct s_object {
  struct smf_ctx ctx;
  /* Other state specific data add here */
  const StepperWithTarget *stepper;
  const SonyRemote *remote;
  const Stack stack;
  int wait_before_ms = 1000;
  int wait_after_ms = 500;
  int64_t last_event_ms = 0;
};

class StateMachine {
  struct s_object s_obj;

public:
  StateMachine(const StepperWithTarget *stepper, const SonyRemote *remote);

  int32_t run_state_machine();

  const struct stepper_with_target_status get_stepper_status() const;
  const struct stack_status get_stack_status() const;
};
