#include "StateMachine.h"
LOG_MODULE_REGISTER(state_machine);

struct s_object *s_obj_ptr;

struct smf_state *s0_ptr;
struct smf_state *s_idle_state_ptr;
struct smf_state *s_interactive_move_ptr;
struct smf_state *s0_stack_ptr;
struct smf_state *s_stack_ptr;
struct smf_state *s_stack_move_ptr;
struct smf_state *s_stack_img_ptr;

static void s0_entry(void *o)
{
  LOG_DBG("s0_entry");
}
static void s0_run(void *o)
{
  LOG_DBG("s0_run");
  smf_set_state(SMF_CTX(s_obj_ptr), s_idle_state_ptr);
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
    smf_set_state(SMF_CTX(s_obj_ptr), s_interactive_move_ptr);
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
    smf_set_state(SMF_CTX(s_obj_ptr), s_idle_state_ptr);
  }
}

static void s_parent_stacking_entry(void *o)
{
  LOG_DBG("%s", __FUNCTION__);
  struct s_object *s = (struct s_object *)o;
  s->stack->start_stack();
}

static void s_parent_stacking_exit(void *o)
{
  LOG_DBG("%s", __FUNCTION__);
}

static void s_stack_run(void *o)
{
  LOG_DBG("%s", __FUNCTION__);
  struct s_object *s = (struct s_object *)o;
  s->stack->log_state();
  if(s->stack->stack_in_progress()) {
    smf_set_state(SMF_CTX(s_obj_ptr), s_stack_move_ptr);
  } else {
    smf_set_state(SMF_CTX(s_obj_ptr), s_idle_state_ptr);
  }
}

static void s_stack_move_run(void *o)
{
  LOG_DBG("%s", __FUNCTION__);
  smf_set_state(SMF_CTX(s_obj_ptr), s_stack_img_ptr);
}

static void s_stack_img_run(void *o)
{
  LOG_DBG("%s", __FUNCTION__);
  struct s_object *s = (struct s_object *)o;
  s->stack->increment_step();
  smf_set_state(SMF_CTX(s_obj_ptr), s_stack_ptr);
}

static const struct smf_state stack_states[] = {
  [S0] = SMF_CREATE_STATE(s0_entry, s0_run, s0_exit, NULL),
  [S_IDLE] = SMF_CREATE_STATE(s_idle_entry, s_idle_run, s_idle_exit, NULL),
  [S_INTERACTIVE_MOVE] = SMF_CREATE_STATE(NULL, s_interactive_move_run, NULL, NULL),
  [S_PARENT_STACKING] = SMF_CREATE_STATE(s_parent_stacking_entry, NULL, s_parent_stacking_exit, NULL),
  [S_STACK] = SMF_CREATE_STATE(NULL, s_stack_run, NULL, &stack_states[S_PARENT_STACKING]),
  [S_STACK_MOVE] = SMF_CREATE_STATE(NULL, s_stack_move_run, NULL, &stack_states[S_PARENT_STACKING]),
  [S_STACK_IMG] = SMF_CREATE_STATE(NULL, s_stack_img_run, NULL, &stack_states[S_PARENT_STACKING]),
};


static struct s_object init_state_machine(const StepperWithTarget *stepper, const Stack * stack, const Gui * gui)
{
  struct s_object s_obj;
  s_obj_ptr = &s_obj;

  s0_ptr = &stack_states[S0];
  s_idle_state_ptr = &stack_states[S_IDLE];
  s_interactive_move_ptr = &stack_states[S_INTERACTIVE_MOVE];
  s_stack_ptr = &stack_states[S_STACK];
  s_stack_move_ptr = &stack_states[S_STACK_MOVE];
  s_stack_img_ptr = &stack_states[S_STACK_IMG];

  s_obj.stepper = stepper;
  s_obj.stack = stack;
  s_obj.gui = gui;

  smf_set_initial(SMF_CTX(s_obj_ptr), s0_ptr);

  return s_obj;
}

static int32_t run_state_machine()
{
  return smf_run_state(SMF_CTX(s_obj_ptr));
}

// ############################################################################
// initialize ZBus

ZBUS_CHAN_DEFINE(controller_msg_chan,   /* Name */
                 struct controller_msg, /* Message type */
                 NULL,
                 NULL,
                 ZBUS_OBSERVERS(controller_action_listener),
                 ZBUS_MSG_INIT(.action = NOOP_CONTROLLER_ACTION, .value = 0));
static void controller_action_listener_cb(const struct zbus_channel *chan)
{
  if (s_obj_ptr == NULL)
  {
    return;
  }

  smf_state* current = s_obj_ptr->ctx.current;
  if (current != s_idle_state_ptr && current != s_interactive_move_ptr)
  {
    LOG_INF("In non-interactive mode, ignore msg");
    return;
  }

  const struct controller_msg *msg = (const struct controller_msg *)zbus_chan_const_msg(chan);
  switch (msg->action)
  {
  case NOOP_CONTROLLER_ACTION:
    break;
  case GO_CONTROLLER_ACTION:
    s_obj_ptr->stepper->go_relative(msg->value);
    break;
  case GO_TO_CONTROLLER_ACTION:
    s_obj_ptr->stepper->set_target_position(msg->value);
    break;
  case SET_NEW_LOWER_BOUND_ACTION:
    s_obj_ptr->stack->set_lower_bound(s_obj_ptr->stepper->get_target_position());
    break;
  case SET_NEW_UPPER_BOUND_ACTION:
    s_obj_ptr->stack->set_upper_bound(s_obj_ptr->stepper->get_target_position());
    break;
  default:
    LOG_ERR("unknown action: %i", msg->action);
    break;
  }
}

ZBUS_LISTENER_DEFINE(controller_action_listener, controller_action_listener_cb);

static int controller_action_pub(controller_msg *msg) 
{
  return  zbus_chan_pub(&controller_msg_chan, msg, K_MSEC(200));
}

