#include "StateMachine.h"
LOG_MODULE_REGISTER(state_machine, LOG_LEVEL_INF);

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

void log_stack_state(void *o) {}

static void s_log_state(void *o) {
  struct s_object *s = (struct s_object *)o;
  s->stepper->log_state();
  s->remote->log_state();
  s->stack.log_state();
  LOG_INF("wait_before_ms=%d, wait_after_ms=%d", s->wait_before_ms,
          s->wait_after_ms);
}

static void s_parent_interactive_entry(void *o) { LOG_INF("%s", __FUNCTION__); }

static void s_parent_interactive_exit(void *o) { LOG_INF("%s", __FUNCTION__); }

static enum smf_state_result s_interactive_run(void *o) {
  struct s_object *s = (struct s_object *)o;

  const struct zbus_channel *chan;

  LOG_DBG("%s, wait for input...", __FUNCTION__);
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
        LOG_INF("go to position %.3fum", nm_as_um(msg.value));
        s->stepper->go_relative_nm(msg.value);
        s->stepper->step_towards_target();
        break;
      case EVENT_GO_TO:
        LOG_INF("go to absolute position %.3fum", nm_as_um(msg.value));
        s->stepper->set_target_position_nm(msg.value);
        s->stepper->step_towards_target();
        break;
      case EVENT_GO_PCT: {
        if (msg.value < 0 || msg.value > 100) {
          LOG_ERR("cannot go to pct, value %d out of range [0-100]", msg.value);
          break;
        }
        int lower = s->stack.get_lower_bound();
        int upper = s->stack.get_upper_bound();
        int range = upper - lower;
        int target = lower + (range * msg.value) / 100;
        LOG_INF("go to relative position %d%% between upper and lower @ %.3fum",
                msg.value, nm_as_um(target));
        s->stepper->set_target_position_nm(target);
        s->stepper->step_towards_target();
        break;
      }
      case EVENT_SET_LOWER_BOUND: {
        int lower_bound = s->stepper->get_target_position_nm();
        LOG_INF("set lower bound to %.3fum", nm_as_um(lower_bound),
                nm_as_um(s->stack.get_upper_bound()));
        s->stack.set_lower_bound(lower_bound);
        s->stack.log_state();
        break;
      }
      case EVENT_SET_UPPER_BOUND: {
        int upper_bound = s->stepper->get_target_position_nm();
        LOG_INF("set upper bound to %.3fum", nm_as_um(upper_bound));
        s->stack.set_upper_bound(upper_bound);
        s->stack.log_state();
        break;
      }
      case EVENT_SET_LOWER_BOUND_TO:
        LOG_INF("set lower bound to %.3fum", nm_as_um(msg.value));
        s->stack.set_lower_bound(msg.value);
        s->stack.log_state();
        break;
      case EVENT_SET_UPPER_BOUND_TO:
        LOG_INF("set upper bound to %.3fum", nm_as_um(msg.value));
        s->stack.set_upper_bound(msg.value);
        s->stack.log_state();
        break;
      case EVENT_SET_WAIT_BEFORE_MS:
        LOG_INF("set wait before ms to %d", msg.value);
        s->wait_before_ms = msg.value;
        break;
      case EVENT_SET_WAIT_AFTER_MS:
        LOG_INF("set wait after ms to %d", msg.value);
        s->wait_after_ms = msg.value;
        break;
      case EVENT_SET_SPEED:
        switch (msg.value) {
        case 1:
          LOG_INF("Setting movement speed to SLOW");
          s->stepper->set_speed(StepperSpeed::SLOW);
          break;
        case 2:
          LOG_INF("Setting movement speed to MEDIUM");
          s->stepper->set_speed(StepperSpeed::MEDIUM);
          break;
        case 3:
          LOG_INF("Setting movement speed to FAST");
          s->stepper->set_speed(StepperSpeed::FAST);
          break;
        default:
          LOG_WRN("Unsupported speed preset %d (expected 1-3)", msg.value);
          break;
        }
        break;
      case EVENT_SET_SPEED_RPM:
        if (msg.value < 1) {
          LOG_WRN("Ignoring RPM update with invalid value %d", msg.value);
          break;
        }
        LOG_INF("Setting movement speed to %d RPM", msg.value);
        s->stepper->set_speed_rpm(msg.value);
        break;
      case EVENT_PAIR_CAMERA:
        LOG_INF("Starting camera pairing");
        smf_set_state(SMF_CTX(o), s_wait_for_camera_ptr);
        break;
      case EVENT_START_STACK:
        LOG_INF("Starting stack..., %d images", msg.value);
        s->stack.set_expected_step_size(msg.value);
        smf_set_state(SMF_CTX(o), s_stack_ptr);
        break;
      case EVENT_START_STACK_WITH_LENGTH:
        LOG_INF("Starting stack..., %d images", msg.value);
        s->stack.set_expected_length_of_stack(msg.value);
        smf_set_state(SMF_CTX(o), s_stack_ptr);
        break;
      case EVENT_SHOOT:
        LOG_INF("Triggering camera shoot");
        s->remote->shoot();
        break;
      case EVENT_RECORD:
        LOG_INF("Toggling camera recording");
        s->remote->recToggle();
        break;
      case EVENT_STATUS:
        s_log_state(s);
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

static void s_parent_camera_pairing_entry(void *o) {
  LOG_INF("Entering camera pairing mode");

  struct s_object *s = (struct s_object *)o;
  if (!s->remote) {
    LOG_WRN("No remote available for pairing");
    smf_set_state(SMF_CTX(o), s_interactive_ptr);
    return;
  }
  s->retry_counter = 0;

  k_msleep(100);
  s->remote->startScan();
  k_msleep(100);
}

static void s_parent_camera_pairing_exit(void *o) {
  LOG_INF("Exiting camera pairing mode");
  struct s_object *s = (struct s_object *)o;
  s->remote->stopScan();
  k_msleep(100);
}

static enum smf_state_result s_wait_for_camera_run(void *o) {
  struct s_object *s = (struct s_object *)o;

  if (s->remote->ready()) {
    LOG_INF("Camera paired successfully");
    s->remote->log_state();
    smf_set_state(SMF_CTX(o), s_interactive_ptr);
    return SMF_EVENT_HANDLED;
  }

  if (s->retry_counter++ > 10) {
    LOG_WRN("Camera pairing failed");
    smf_set_state(SMF_CTX(o), s_interactive_ptr);
    return SMF_EVENT_HANDLED;
  }

  LOG_DBG("Waiting for camera to pair (%d)...", s->retry_counter);
  k_sleep(K_MSEC(1000));

  return SMF_EVENT_HANDLED;
}

static void s_parent_stacking_entry(void *o) {
  struct s_object *s = (struct s_object *)o;

  if (!s->remote->ready()) {
    LOG_WRN("Cannot start stacking - camera not connected");
    smf_set_state(SMF_CTX(o), s_interactive_ptr);
    return;
  }

  // wait for camera to be ready
  while (!s->remote->ready()) {
    LOG_INF("Waiting for camera to be ready...");
    k_sleep(K_SECONDS(1));
  }
  LOG_DBG("Camera is ready, starting stack");

  s->stepper->set_speed(StepperSpeed::SLOW);
  s->stack.start_stack();
}

static void s_parent_stacking_exit(void *o) {
  struct s_object *s = (struct s_object *)o;

  s->stepper->set_speed(StepperSpeed::MEDIUM);
}

static enum smf_state_result s_stack_run(void *o) {
  struct s_object *s = (struct s_object *)o;
  if (s->stack.stack_in_progress()) {
    LOG_INF("Stacking:");
    s->stack.log_state();
    int current_target = s->stack.get_current_target().value();
    s->stepper->set_target_position_nm(current_target);
    smf_set_state(SMF_CTX(o), s_stack_move_ptr);
  } else {
    LOG_INF("Stacking DONE");
    s->stack.flip_start_at();
    smf_set_state(SMF_CTX(o), s_interactive_ptr);
  }
  return SMF_EVENT_HANDLED;
}

static enum smf_state_result s_stack_move_run(void *o) {
  LOG_DBG("%s", __FUNCTION__);
  struct s_object *s = (struct s_object *)o;
  s->stepper->step_towards_target();
  s->stepper->wait_and_pause();
  smf_set_state(SMF_CTX(o), s_stack_settle_ptr);
  return SMF_EVENT_HANDLED;
}

static enum smf_state_result s_stack_settle_run(void *o) {
  LOG_DBG("%s", __FUNCTION__);
  struct s_object *s = (struct s_object *)o;
  k_sleep(K_MSEC(s->wait_before_ms));
  smf_set_state(SMF_CTX(o), s_stack_img_ptr);
  return SMF_EVENT_HANDLED;
}

static enum smf_state_result s_stack_img_run(void *o) {
  struct s_object *s = (struct s_object *)o;
  LOG_DBG("%s", __FUNCTION__);
  s->remote->shoot();
  k_sleep(K_MSEC(s->wait_after_ms));
  s->stack.increment_target();
  smf_set_state(SMF_CTX(o), s_stack_ptr);
  return SMF_EVENT_HANDLED;
}

// Note: Array order must match enum stack_state order
static struct smf_state stack_states[] = {
    SMF_CREATE_STATE(NULL, s0_run, NULL, NULL, NULL), // S0
    SMF_CREATE_STATE(s_parent_interactive_entry, NULL,
                     s_parent_interactive_exit, NULL,
                     NULL), // S_PARENT_INTERACTIVE
    SMF_CREATE_STATE(s_log_state, s_interactive_run, NULL, NULL,
                     NULL), // S_INTERACTIVE
    SMF_CREATE_STATE(s_parent_camera_pairing_entry, NULL,
                     s_parent_camera_pairing_exit, NULL,
                     NULL), // S_PARENT_CAMERA_PAIRING
    SMF_CREATE_STATE(NULL, s_wait_for_camera_run, NULL, NULL,
                     NULL), // S_WAIT_FOR_CAMERA
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
  stack_states[S_WAIT_FOR_CAMERA].parent =
      &stack_states[S_PARENT_CAMERA_PAIRING];
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
