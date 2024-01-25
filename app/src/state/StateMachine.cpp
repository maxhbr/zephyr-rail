#include "StateMachine.h"
LOG_MODULE_REGISTER(state_machine);


// ############################################################################
// initialize ZBus

ZBUS_CHAN_DEFINE(state_msg_chan,   /* Name */
                 struct state_msg, /* Message type */
                 NULL,
                 NULL,
                 ZBUS_OBSERVERS(state_sub),
                 ZBUS_MSG_INIT(.action = NOOP_CONTROLLER_ACTION, .value = 0));


static int state_action_pub(state_msg *msg) 
{
  LOG_INF("send msg: action=%d value=%d", msg->action, msg->value);
  return  zbus_chan_pub(&state_msg_chan, msg, K_MSEC(200));
}

ZBUS_SUBSCRIBER_DEFINE(state_sub, 20);

// ############################################################################
// initialize StateMachine

struct smf_state *s0_ptr;
struct smf_state *s_interactive_state_ptr;
struct smf_state *s_stack_ptr;
struct smf_state *s_stack_move_ptr;
struct smf_state *s_stack_img_ptr;

static bool s_handle_state_msg(s_object *s, struct state_msg *msg)
{
  switch (msg->action)
  {
  case NOOP_CONTROLLER_ACTION:
    break;
  case GO_CONTROLLER_ACTION:
    s->stepper->go_relative(msg->value);
    break;
  case GO_TO_CONTROLLER_ACTION:
    s->stepper->set_target_position(msg->value);
    break;
  case SET_NEW_LOWER_BOUND_ACTION:
    s->stack.set_lower_bound(s->stepper->get_target_position());
    break;
  case SET_NEW_UPPER_BOUND_ACTION:
    s->stack.set_upper_bound(s->stepper->get_target_position());
    break;
  case START_STACK:
    smf_set_state(SMF_CTX(s), s_stack_ptr);
    break;
  default:
    LOG_ERR("unknown action: %i", msg->action);
    return false;
    break;
  }
  return true;
}

static bool s_read_from_sub(s_object *s)
{
	const struct zbus_channel *chan;

	if (!zbus_sub_wait(&state_sub, &chan, K_MSEC(100))) {
		if (&state_msg_chan == chan) {
      struct state_msg msg;
			zbus_chan_read(&state_msg_chan, &msg, K_MSEC(100));
			LOG_INF("msg: action=%d value=%d", msg.action, msg.value);

      return s_handle_state_msg(s, &msg);
		}
	}
  return false;
}

static void s0_run(void *o)
{
  smf_set_state(SMF_CTX(o), s_interactive_state_ptr);
}

static void s_log_state(void *o)
{
  struct s_object *s = (struct s_object *)o;
  s->stepper->log_state();
}

static void s_interactive_run(void *o)
{
  struct s_object *s = (struct s_object *)o;
  s_read_from_sub(s);
}

static void s_parent_stacking_entry(void *o)
{
  struct s_object *s = (struct s_object *)o;
  s->stack.start_stack();
}

static void s_parent_stacking_exit(void *o)
{
}

static void s_stack_run(void *o)
{
  struct s_object *s = (struct s_object *)o;
  s->stack.log_state();
  if(s->stack.stack_in_progress()) {
    smf_set_state(SMF_CTX(o), s_stack_move_ptr);
  } else {
    smf_set_state(SMF_CTX(o), s_interactive_state_ptr);
  }
}

static void s_stack_move_run(void *o)
{
  smf_set_state(SMF_CTX(o), s_stack_img_ptr);
}

static void s_stack_img_run(void *o)
{
  struct s_object *s = (struct s_object *)o;
  s->stack.increment_step();
  smf_set_state(SMF_CTX(o), s_stack_ptr);
}

static const struct smf_state stack_states[] = {
  [S0] = SMF_CREATE_STATE(NULL, s0_run, NULL, NULL),
  [S_INTERACTIVE] = SMF_CREATE_STATE(s_log_state, s_interactive_run, NULL, NULL),
  [S_PARENT_STACKING] = SMF_CREATE_STATE(s_parent_stacking_entry, NULL, s_parent_stacking_exit, NULL),
  [S_STACK] = SMF_CREATE_STATE(NULL, s_stack_run, NULL, &stack_states[S_PARENT_STACKING]),
  [S_STACK_MOVE] = SMF_CREATE_STATE(NULL, s_stack_move_run, NULL, &stack_states[S_PARENT_STACKING]),
  [S_STACK_IMG] = SMF_CREATE_STATE(NULL, s_stack_img_run, NULL, &stack_states[S_PARENT_STACKING]),
};

StateMachine::StateMachine(const StepperWithTarget *stepper)
{
  s0_ptr = &stack_states[S0];
  s_interactive_state_ptr = &stack_states[S_INTERACTIVE];
  s_stack_ptr = &stack_states[S_STACK];
  s_stack_move_ptr = &stack_states[S_STACK_MOVE];
  s_stack_img_ptr = &stack_states[S_STACK_IMG];

  s_obj.stepper = stepper;
  Stack stack;
  s_obj.stack = stack;

  smf_set_initial(SMF_CTX(&s_obj), s0_ptr);
}

int32_t StateMachine::run_state_machine()
{
  LOG_DBG("%s", __FUNCTION__);
  return smf_run_state(SMF_CTX(&s_obj));
}


const struct stepper_with_target_status StateMachine::get_stepper_status() {
  return s_obj.stepper->get_status();
}
const struct stack_status StateMachine::get_stack_status() {
  return s_obj.stack.get_status();
}
