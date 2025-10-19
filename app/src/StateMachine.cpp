#include "StateMachine.h"
LOG_MODULE_REGISTER(state_machine, LOG_LEVEL_DBG);

// ############################################################################
// initialize ZBus

ZBUS_CHAN_DEFINE(event_msg_chan,   /* Name */
                 struct event_msg, /* Message type */
                 NULL, NULL, ZBUS_OBSERVERS(event_sub),
                 ZBUS_MSG_INIT(.evt = {}));

static int event_pub(event event, int value) {
  LOG_DBG("send msg: event=%d with value=%d", event, value);
  struct event_msg msg = {event, value};
  return zbus_chan_pub(&event_msg_chan, &msg, K_MSEC(200));
}

static int event_pub(event event) { return event_pub(event, 0); }

ZBUS_SUBSCRIBER_DEFINE(event_sub, 20);

// ############################################################################
// initialize StateMachine

struct smf_state *s0_ptr;
struct smf_state *s_wait_for_camera_ptr;
struct smf_state *s_interactive_ptr;
struct smf_state *s_stack_ptr;
struct smf_state *s_stack_move_ptr;
struct smf_state *s_stack_settle_ptr;
struct smf_state *s_stack_img_ptr;

static enum smf_state_result s0_run(void *o) {
  smf_set_state(SMF_CTX(o), s_interactive_ptr);
  return SMF_EVENT_HANDLED;
}

static enum smf_state_result s_wait_for_camera_run(void *o) {
  struct s_object *s = (struct s_object *)o;
  if (s->remote->ready()) {
    // If we're in stacking mode, go to stack, otherwise go to interactive
    if (s->stack.stack_in_progress()) {
      smf_set_state(SMF_CTX(o), s_stack_ptr);
    } else {
      smf_set_state(SMF_CTX(o), s_interactive_ptr);
    }
  } else {
    LOG_INF("%s, sleeping for 2 seconds...", __FUNCTION__);
    k_sleep(K_SECONDS(2));
    smf_set_state(SMF_CTX(o), s_wait_for_camera_ptr);
  }

  return SMF_EVENT_HANDLED;
}

static void s_log_state(void *o) {
  struct s_object *s = (struct s_object *)o;
  s->stepper->log_state();
}

static void s_parent_interactive_entry(void *o) { LOG_INF("%s", __FUNCTION__); }

static void s_parent_interactive_exit(void *o) { LOG_INF("%s", __FUNCTION__); }

static enum smf_state_result s_interactive_run(void *o) {
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
      case EVENT_GO:
        LOG_INF("go to position %d", msg.value);
        s->stepper->go_relative(msg.value);
        break;
      case EVENT_GO_TO:
        LOG_INF("go to absolute position %d", msg.value);
        s->stepper->set_target_position(msg.value);
        break;
      case EVENT_SET_LOWER_BOUND:
        int lower_bound = s->stepper->get_target_position();
        LOG_INF("set lower bound to %d", lower_bound);
        s->stack.set_lower_bound(lower_bound);
        break;
      case EVENT_SET_UPPER_BOUND:
        int upper_bound = s->stepper->get_target_position();
        LOG_INF("set upper bound to %d", upper_bound);
        s->stack.set_upper_bound(upper_bound);
        break;
      case EVENT_SET_LOWER_BOUND_TO:
        LOG_INF("set lower bound to %d", msg.value);
        s->stack.set_lower_bound(msg.value);
        break;
      case EVENT_SET_UPPER_BOUND_TO:
        LOG_INF("set upper bound to %d", msg.value);
        s->stack.set_upper_bound(msg.value);
        break;
      case EVENT_START_STACK:
        // Start stack
        LOG_INF("Starting stack..., %d images", msg.value);
        s->stack.set_expected_length_of_stack(msg.value);
        smf_set_state(SMF_CTX(o), s_wait_for_camera_ptr);
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
    smf_set_state(SMF_CTX(o), s_interactive_ptr);
  }
  return SMF_EVENT_HANDLED;
}

static enum smf_state_result s_stack_move_run(void *o) {
  LOG_INF("%s", __FUNCTION__);
  // Move to next position
  smf_set_state(SMF_CTX(o), s_stack_settle_ptr);
  return SMF_EVENT_HANDLED;
}

static enum smf_state_result s_stack_settle_run(void *o) {
  LOG_INF("%s", __FUNCTION__);
  // Allow time for vibrations to settle
  smf_set_state(SMF_CTX(o), s_stack_img_ptr);
  return SMF_EVENT_HANDLED;
}

static enum smf_state_result s_stack_img_run(void *o) {
  struct s_object *s = (struct s_object *)o;
  LOG_INF("%s", __FUNCTION__);
  s->stack.increment_step();
  smf_set_state(SMF_CTX(o), s_stack_ptr);
  return SMF_EVENT_HANDLED;
}

// Note: Array order must match enum stack_state order
static struct smf_state stack_states[] = {
    SMF_CREATE_STATE(NULL, s0_run, NULL, NULL, NULL), // S0
    SMF_CREATE_STATE(NULL, s_wait_for_camera_run, NULL, NULL,
                     NULL), // S_WAIT_FOR_CAMERA
    SMF_CREATE_STATE(s_parent_interactive_entry, NULL,
                     s_parent_interactive_exit, NULL,
                     NULL), // S_PARENT_INTERACTIVE
    SMF_CREATE_STATE(s_log_state, s_interactive_run, NULL, NULL,
                     NULL), // S_INTERACTIVE
    SMF_CREATE_STATE(s_parent_stacking_entry, NULL, s_parent_stacking_exit,
                     NULL, NULL),                          // S_PARENT_STACKING
    SMF_CREATE_STATE(NULL, s_stack_run, NULL, NULL, NULL), // S_STACK
    SMF_CREATE_STATE(NULL, s_stack_move_run, NULL, NULL, NULL), // S_STACK_MOVE
    SMF_CREATE_STATE(NULL, s_stack_settle_run, NULL, NULL,
                     NULL),                                    // S_STACK_SETTLE
    SMF_CREATE_STATE(NULL, s_stack_img_run, NULL, NULL, NULL), // S_STACK_IMG
};

StateMachine::StateMachine(const StepperWithTarget *stepper,
                           const SonyRemote *remote) {
  LOG_INF("%s", __FUNCTION__);

  // Set up parent pointers after array initialization
  stack_states[S_INTERACTIVE].parent = &stack_states[S_PARENT_INTERACTIVE];
  stack_states[S_STACK].parent = &stack_states[S_PARENT_STACKING];
  stack_states[S_STACK_MOVE].parent = &stack_states[S_PARENT_STACKING];
  stack_states[S_STACK_SETTLE].parent = &stack_states[S_PARENT_STACKING];
  stack_states[S_STACK_IMG].parent = &stack_states[S_PARENT_STACKING];

  s0_ptr = &stack_states[S0];
  s_wait_for_camera_ptr = &stack_states[S_WAIT_FOR_CAMERA];
  s_interactive_ptr = &stack_states[S_INTERACTIVE];
  s_stack_ptr = &stack_states[S_STACK];
  s_stack_move_ptr = &stack_states[S_STACK_MOVE];
  s_stack_settle_ptr = &stack_states[S_STACK_SETTLE];
  s_stack_img_ptr = &stack_states[S_STACK_IMG];

  s_obj.stepper = stepper;
  s_obj.remote = remote;
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
