#include "StateMachine.h"
LOG_MODULE_REGISTER(state_machine, LOG_LEVEL_INF);

// ############################################################################
// initialize ZBus

ZBUS_CHAN_DEFINE(event_msg_chan,   /* Name */
                 struct event_msg, /* Message type */
                 NULL, NULL, ZBUS_OBSERVERS(event_sub),
                 ZBUS_MSG_INIT(.evt = {}));

static int event_pub(event event) {
  LOG_DBG("send msg: event=%d", event);
  struct event_msg msg = {event};
  return zbus_chan_pub(&event_msg_chan, &msg, K_MSEC(200));
}

ZBUS_SUBSCRIBER_DEFINE(event_sub, 20);

static void input_cb(struct input_event *evt, void *user_data) {
  LOG_DBG("type: %d, code: %d, value: %d", evt->type, evt->code, evt->value);

  if (evt->type != INPUT_EV_KEY) {
    return;
  }
  if (evt->value == 0) {
    return;
  }
  int err;

  switch (evt->code) {
  case INPUT_KEY_0:
    err = event_pub(EVENT_INPUT_KEY_0);
    break;
  case INPUT_KEY_1:
    err = event_pub(EVENT_INPUT_KEY_1);
    break;
  case INPUT_KEY_2:
    err = event_pub(EVENT_INPUT_KEY_2);
    break;
  case INPUT_KEY_ENTER:
    err = event_pub(EVENT_INPUT_KEY_ENTER);
    break;
  case INPUT_KEY_DOWN:
    err = event_pub(EVENT_INPUT_KEY_DOWN);
    break;
  case INPUT_KEY_UP:
    err = event_pub(EVENT_INPUT_KEY_UP);
    break;
  case INPUT_KEY_LEFT:
    err = event_pub(EVENT_INPUT_KEY_LEFT);
    break;
  case INPUT_KEY_RIGHT:
    err = event_pub(EVENT_INPUT_KEY_RIGHT);
    break;
  default:
    LOG_DBG("Unrecognized input code %u value %d", evt->code, evt->value);
    return;
  }
  if (err == -ENOMSG) {
    LOG_INF("Pub an invalid value to a channel with validator successfully.");
  }
}

// ############################################################################
// initialize StateMachine

struct smf_state *s0_ptr;
struct smf_state *s_interactive_move_ptr;
struct smf_state *s_interactive_pre_stacking_ptr;
struct smf_state *s_stack_ptr;
struct smf_state *s_stack_move_ptr;
struct smf_state *s_stack_img_ptr;

/* static bool s_handle_state_msg(s_object *s, struct state_msg *msg) */
/* { */
/*   switch (msg->action) */
/*   { */
/*   case NOOP_CONTROLLER_ACTION: */
/*     break; */
/*   case GO_CONTROLLER_ACTION: */
/*     s->stepper->go_relative(msg->value); */
/*     break; */
/*   case GO_TO_CONTROLLER_ACTION: */
/*     s->stepper->set_target_position(msg->value); */
/*     break; */
/*   case SET_NEW_LOWER_BOUND_ACTION: */
/*     s->stack.set_lower_bound(s->stepper->get_target_position()); */
/*     break; */
/*   case SET_NEW_UPPER_BOUND_ACTION: */
/*     s->stack.set_upper_bound(s->stepper->get_target_position()); */
/*     break; */
/*   case START_STACK: */
/*     smf_set_state(SMF_CTX(s), s_stack_ptr); */
/*     break; */
/*   default: */
/*     LOG_ERR("unknown action: %i", msg->action); */
/*     return false; */
/*     break; */
/*   } */
/*   return true; */
/* } */

/* static bool s_read_from_sub(s_object *s) */
/* { */
/* 	const struct zbus_channel *chan; */

/* 	if (!zbus_sub_wait(&state_sub, &chan, K_MSEC(100))) { */
/* 		if (&state_msg_chan == chan) { */
/*       struct state_msg msg; */
/* 			zbus_chan_read(&state_msg_chan, &msg, K_MSEC(100)); */
/* 			LOG_INF("msg: action=%d value=%d", msg.action,
 * msg.value); */

/*       return s_handle_state_msg(s, &msg); */
/* 		} */
/* 	} */
/*   return false; */
/* } */

static enum smf_state_result s0_run(void *o) {
  smf_set_state(SMF_CTX(o), s_interactive_move_ptr);
  return SMF_EVENT_HANDLED;
}

static void s_log_state(void *o) {
  struct s_object *s = (struct s_object *)o;
  s->stepper->log_state();
}

static void s_parent_interactive_entry(void *o) { LOG_INF("%s", __FUNCTION__); }

static void s_parent_interactive_exit(void *o) { LOG_INF("%s", __FUNCTION__); }

static enum smf_state_result s_interactive_move_run(void *o) {
  struct s_object *s = (struct s_object *)o;

  const struct zbus_channel *chan;

  LOG_INF("%s, wait for input...", __FUNCTION__);
  if (!zbus_sub_wait(&event_sub, &chan, K_FOREVER)) {
    if (&event_msg_chan == chan) {
      struct event_msg msg;
      zbus_chan_read(&event_msg_chan, &msg, K_MSEC(100));

      if (!msg.evt.has_value()) {
        LOG_INF("no value in event_msg");
        return SMF_EVENT_HANDLED;
      }

      switch (msg.evt.value()) {
      case EVENT_INPUT_KEY_RIGHT:
        smf_set_state(SMF_CTX(o), s_interactive_pre_stacking_ptr);
        break;
      case EVENT_INPUT_KEY_0:
        LOG_INF("go_right");
        s->stepper->go_relative(100);
        break;
      case EVENT_INPUT_KEY_2:
        LOG_INF("go_left");
        s->stepper->go_relative(-100);
        break;
      default:
        LOG_INF("unsupported event: %d", msg.evt.value());
      }
    }
  } else {
    LOG_ERR("failed to wait for zbus");
  }
  return SMF_EVENT_HANDLED;
}

static enum smf_state_result s_interactive_pre_stacking_run(void *o) {
  struct s_object *s = (struct s_object *)o;

  const struct zbus_channel *chan;

  LOG_INF("%s, wait for input...", __FUNCTION__);
  if (!zbus_sub_wait(&event_sub, &chan, K_FOREVER)) {
    if (&event_msg_chan == chan) {
      struct event_msg msg;
      zbus_chan_read(&event_msg_chan, &msg, K_MSEC(100));

      if (!msg.evt.has_value()) {
        LOG_INF("no value in event_msg");
        return SMF_EVENT_HANDLED;
      }

      switch (msg.evt.value()) {
      case EVENT_INPUT_KEY_LEFT:
        smf_set_state(SMF_CTX(o), s_interactive_move_ptr);
        break;
      default:
        LOG_INF("unsupported event: %d", msg.evt.value());
      }
    }
  } else {
    LOG_ERR("failed to wait for zbus");
  }
  return SMF_EVENT_HANDLED;
}

static void s_parent_stacking_entry(void *o) {
  struct s_object *s = (struct s_object *)o;
  s->stack.start_stack();
}

static void s_parent_stacking_exit(void *o) {}

static enum smf_state_result s_stack_run(void *o) {
  struct s_object *s = (struct s_object *)o;
  s->stack.log_state();
  if (s->stack.stack_in_progress()) {
    smf_set_state(SMF_CTX(o), s_stack_move_ptr);
  } else {
    smf_set_state(SMF_CTX(o), s_interactive_move_ptr);
  }
  return SMF_EVENT_HANDLED;
}

static enum smf_state_result s_stack_move_run(void *o) {
  smf_set_state(SMF_CTX(o), s_stack_img_ptr);
  return SMF_EVENT_HANDLED;
}

static enum smf_state_result s_stack_img_run(void *o) {
  struct s_object *s = (struct s_object *)o;
  s->stack.increment_step();
  smf_set_state(SMF_CTX(o), s_stack_ptr);
  return SMF_EVENT_HANDLED;
}

static const struct smf_state stack_states[] = {
    [S0] = SMF_CREATE_STATE(NULL, s0_run, NULL, NULL, NULL),

    [S_PARENT_INTERACTIVE] =
        SMF_CREATE_STATE(s_parent_interactive_entry, NULL,
                         s_parent_interactive_exit, NULL, NULL),
    [S_INTERACTIVE_MOVE] =
        SMF_CREATE_STATE(s_log_state, s_interactive_move_run, NULL,
                         &stack_states[S_PARENT_INTERACTIVE], NULL),
    [S_INTERACTIVE_PRE_STACKING] =
        SMF_CREATE_STATE(s_log_state, s_interactive_pre_stacking_run, NULL,
                         &stack_states[S_PARENT_INTERACTIVE], NULL),

    [S_PARENT_STACKING] = SMF_CREATE_STATE(s_parent_stacking_entry, NULL,
                                           s_parent_stacking_exit, NULL, NULL),
    [S_STACK] = SMF_CREATE_STATE(NULL, s_stack_run, NULL,
                                 &stack_states[S_PARENT_STACKING], NULL),
    [S_STACK_MOVE] = SMF_CREATE_STATE(NULL, s_stack_move_run, NULL,
                                      &stack_states[S_PARENT_STACKING], NULL),
    [S_STACK_IMG] = SMF_CREATE_STATE(NULL, s_stack_img_run, NULL,
                                     &stack_states[S_PARENT_STACKING], NULL),
};

StateMachine::StateMachine(const StepperWithTarget *stepper) {
  s0_ptr = &stack_states[S0];
  s_interactive_move_ptr = &stack_states[S_INTERACTIVE_MOVE];
  s_interactive_pre_stacking_ptr = &stack_states[S_INTERACTIVE_PRE_STACKING];
  s_stack_ptr = &stack_states[S_STACK];
  s_stack_move_ptr = &stack_states[S_STACK_MOVE];
  s_stack_img_ptr = &stack_states[S_STACK_IMG];

  s_obj.stepper = stepper;
  Stack stack;
  s_obj.stack = stack;

  smf_set_initial(SMF_CTX(&s_obj), s0_ptr);
}

int32_t StateMachine::run_state_machine() {
  LOG_DBG("%s", __FUNCTION__);
  return smf_run_state(SMF_CTX(&s_obj));
}

const struct stepper_with_target_status
StateMachine::get_stepper_status() const {
  return s_obj.stepper->get_status();
}
const struct stack_status StateMachine::get_stack_status() const {
  return s_obj.stack.get_status();
}
