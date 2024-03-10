#include "StepperWithTarget.h"

void StepperWithTarget::log_state() {
  LOG_MODULE_DECLARE(stepper);
  Stepper::log_state();
  LOG_INF("target_position: %i", target_position);
};

void StepperWithTarget::start() { Stepper::start(); }
void StepperWithTarget::pause() { Stepper::pause(); }
void StepperWithTarget::wait_and_pause() {
  LOG_MODULE_DECLARE(stepper);
  LOG_INF("wait...");
  while (!is_in_target_position()) {
    k_sleep(K_MSEC(100));
  }
  pause();
  LOG_INF("...pause");
}

int StepperWithTarget::get_position() { return Stepper::get_position(); }

int StepperWithTarget::go_relative(int dist) {
  target_position += dist;
  return target_position;
}

void StepperWithTarget::set_target_position(int _target_position) {
  target_position = _target_position;
}
int StepperWithTarget::get_target_position() { return target_position; }

bool StepperWithTarget::step_towards_target() {
  LOG_MODULE_DECLARE(stepper);
  LOG_DBG("step_towards_target");
  is_moving = !step_towards(get_target_position());
  return !is_moving;
}

bool StepperWithTarget::is_in_target_position() {
  return get_position() == get_target_position();
}


StepperWithTarget *started_stepper_ptr;

void stepper_work_handler(struct k_work *work)
{
  ARG_UNUSED(work);
  if (started_stepper_ptr == NULL)
  {
    return;
  }
  started_stepper_ptr->step_towards_target();
}
K_WORK_DEFINE(stepper_work, stepper_work_handler);
void stepper_expiry_function(struct k_timer *timer_id)
{
  ARG_UNUSED(timer_id);
  k_work_submit(&stepper_work);
}
K_TIMER_DEFINE(stepper_timer, stepper_expiry_function, NULL);
void start_stepper(StepperWithTarget *_started_stepper_ptr)
{
  if (_started_stepper_ptr == NULL)
  {
    return;
  }
  started_stepper_ptr = _started_stepper_ptr;
  started_stepper_ptr->start();
  k_timer_start(&stepper_timer, K_USEC(20), K_USEC(20));
}
