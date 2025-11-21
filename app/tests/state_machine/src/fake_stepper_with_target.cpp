#include <stepper_with_target/StepperWithTarget.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(test_stepper_with_target, LOG_LEVEL_INF);

StepperWithTarget::StepperWithTarget(const struct device *dev,
                                     int _pitch_per_rev,
                                     int _pulses_per_rev_mm) {
  stepper_dev = dev;
  pitch_per_rev_nm = _pitch_per_rev * 1000000;
  pulses_per_rev = _pulses_per_rev_mm;
  target_position = 0;
  is_moving = false;
  enabled = true;
}

char *StepperWithTarget::state() {
  static char buffer[96];
  snprintf(buffer, sizeof(buffer), "TestStepper enabled=%d target=%d steps",
           enabled ? 1 : 0, target_position);
  return buffer;
}

void StepperWithTarget::log_state() { LOG_INF("%s", state()); }

int StepperWithTarget::enable() {
  enabled = true;
  return 0;
}

int StepperWithTarget::disable() {
  enabled = false;
  return 0;
}

void StepperWithTarget::start() { enable(); }

void StepperWithTarget::pause() { is_moving = false; }

void StepperWithTarget::wait_and_pause() { is_moving = false; }

int StepperWithTarget::get_position() { return target_position; }

int StepperWithTarget::set_speed(StepperSpeed speed) {
  ARG_UNUSED(speed);
  return 0;
}

void StepperWithTarget::event_callback_wrapper(const struct device *dev,
                                               const enum stepper_event event,
                                               void *user_data) {
  ARG_UNUSED(dev);
  ARG_UNUSED(event);
  ARG_UNUSED(user_data);
}

int32_t StepperWithTarget::go_relative(int32_t dist) {
  target_position += dist;
  return target_position;
}

void StepperWithTarget::set_target_position(int32_t _target_position) {
  target_position = _target_position;
}

int32_t StepperWithTarget::get_target_position() { return target_position; }

int32_t StepperWithTarget::steps_to_nm(int32_t steps) {
  if (pulses_per_rev == 0) {
    return steps;
  }
  int64_t result =
      ((int64_t)steps * (int64_t)pitch_per_rev_nm) / (int64_t)pulses_per_rev;
  return (int32_t)result;
}

int32_t StepperWithTarget::nm_to_steps(int32_t nm) {
  if (pitch_per_rev_nm == 0) {
    return nm;
  }
  int64_t result =
      ((int64_t)nm * (int64_t)pulses_per_rev) / (int64_t)pitch_per_rev_nm;
  return (int32_t)result;
}

int32_t StepperWithTarget::go_relative_nm(int32_t dist) {
  int32_t steps = nm_to_steps(dist);
  return go_relative(steps);
}

void StepperWithTarget::set_target_position_nm(int32_t _target_position) {
  int32_t steps = nm_to_steps(_target_position);
  set_target_position(steps);
}

int32_t StepperWithTarget::get_target_position_nm() {
  int32_t steps = get_target_position();
  return steps_to_nm(steps);
}

bool StepperWithTarget::step_towards_target() {
  is_moving = false;
  return true;
}

bool StepperWithTarget::is_in_target_position() { return true; }

const struct stepper_with_target_status StepperWithTarget::get_status() {
  return {
      .actual_position = get_target_position(),
      .is_moving = is_moving,
      .target_position = target_position,
  };
}

void start_stepper(StepperWithTarget *_started_stepper_ptr) {
  ARG_UNUSED(_started_stepper_ptr);
}
