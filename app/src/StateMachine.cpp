#include "StateMachine.h"
LOG_MODULE_REGISTER(state_machine);

struct s_object *s_obj_ptr;

struct smf_state *s0_state_ptr;
struct smf_state *s_idle_state_ptr;
struct smf_state *s_interactive_move_ptr;

static void s0_entry(void *o)
{
  /* LOG_DBG("s0_entry"); */
}
static void s0_run(void *o)
{
  /* LOG_DBG("s0_run"); */
  smf_set_state(SMF_CTX(s_obj_ptr), s_idle_state_ptr);
}
static void s0_exit(void *o)
{
  /* LOG_DBG("s0_exit"); */
}

static void s_idle_entry(void *o)
{
  /* LOG_INF("S_IDLE..."); */
  struct s_object *s = (struct s_object *)o;
  s->stepper->log_state();
}
static void s_idle_run(void *o)
{
  /* LOG_DBG("s_idle_run"); */
  struct s_object *s = (struct s_object *)o;
  if (!s->stepper->is_in_target_position()) {
    smf_set_state(SMF_CTX(s_obj_ptr), s_interactive_move_ptr);
  }
}
static void s_idle_exit(void *o)
{
  /* LOG_DBG("s_idle_exit"); */
}

static void s_interactive_move_run(void *o)
{
  /* LOG_DBG("s_interactive_move_run"); */
  struct s_object *s = (struct s_object *)o;
  if (s->stepper->is_in_target_position()) {
    smf_set_state(SMF_CTX(s_obj_ptr), s_idle_state_ptr);
  }
}

static const struct smf_state stack_states[] = {
        [S0] = SMF_CREATE_STATE(s0_entry, s0_run, s0_exit),
        [S_IDLE] = SMF_CREATE_STATE(s_idle_entry, s_idle_run, s_idle_exit),
        [S_INTERACTIVE_MOVE] = SMF_CREATE_STATE(NULL, s_interactive_move_run, NULL),
};


static struct s_object init_state_machine(const StepperWithTarget *stepper, const Stack * stack, const Gui * gui)
{
  struct s_object s_obj;
  s_obj_ptr = &s_obj;
  s0_state_ptr = &stack_states[S0];
  s_idle_state_ptr = &stack_states[S_IDLE];
  s_interactive_move_ptr = &stack_states[S_INTERACTIVE_MOVE];
  s_obj.stepper = stepper;
  s_obj.stack = stack;
  s_obj.gui = gui;
  smf_set_initial(SMF_CTX(s_obj_ptr), s0_state_ptr);
  return s_obj;
}

static int32_t run_state_machine()
{
  return smf_run_state(SMF_CTX(s_obj_ptr));
}
