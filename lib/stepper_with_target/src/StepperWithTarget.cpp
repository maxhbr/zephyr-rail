#include "stepper_with_target/StepperWithTarget.h"

LOG_MODULE_REGISTER(stepper_with_target, LOG_LEVEL_INF);

StepperWithTarget::StepperWithTarget(const struct device *dev,
                                     int _pitch_per_rev, int _pulses_per_rev) {
  stepper_dev = dev;

  if (!device_is_ready(stepper_dev)) {
    LOG_ERR("Stepper device is not ready");
    return;
  }

  // Set up event callback
  int ret =
      stepper_set_event_callback(stepper_dev, event_callback_wrapper, this);
  if (ret < 0) {
    LOG_WRN("Failed to set event callback: %d", ret);
  }

  // Get initial position
  int32_t pos;
  ret = stepper_get_actual_position(stepper_dev, &pos);
  if (ret == 0) {
    target_position = pos;
  }

  ret = set_speed(StepperSpeed::MEDIUM);
  if (ret < 0) {
    LOG_WRN("Failed to set step interval: %d", ret);
  }

  pitch_per_rev = _pitch_per_rev;
  pulses_per_rev = _pulses_per_rev;
  LOG_INF("%s initialized with pitch_per_rev=%dmm, pulses_per_rev=%d",
          __FUNCTION__, pitch_per_rev, pulses_per_rev);
}

int StepperWithTarget::set_speed(StepperSpeed speed) {
  uint64_t interval_ns;
  switch (speed) {
  case StepperSpeed::FAST:
    interval_ns = 78125; // ~78 µs , for 15 RPM
    break;
  case StepperSpeed::MEDIUM:
    interval_ns = 117188; // ~117 µs , for 10 RPM
    break;
  case StepperSpeed::SLOW:
    interval_ns = 234375; // ~234 µs , for 5 RPM
    break;
  default:
    interval_ns = 117188; // Default to medium
    break;
  }

  int ret = stepper_set_microstep_interval(stepper_dev, interval_ns);
  if (ret < 0) {
    LOG_WRN("Failed to set step interval: %d", ret);
  } else {
    LOG_DBG("Stepper speed set to %d (interval %llu ns)",
            static_cast<int>(speed), interval_ns);
  }
  return ret;
}

void StepperWithTarget::event_callback_wrapper(const struct device *dev,
                                               const enum stepper_event event,
                                               void *user_data) {
  StepperWithTarget *instance = static_cast<StepperWithTarget *>(user_data);

  switch (event) {
  case STEPPER_EVENT_STEPS_COMPLETED:
    LOG_DBG("Movement completed!");
    instance->is_moving = false;
    break;
  case STEPPER_EVENT_STALL_DETECTED:
    LOG_WRN("Stall detected!");
    instance->is_moving = false;
    break;
  case STEPPER_EVENT_STOPPED:
    LOG_DBG("Stepper stopped");
    instance->is_moving = false;
    break;
  default:
    LOG_DBG("Stepper event: %d", event);
    break;
  }
}

void StepperWithTarget::log_state() {
  int32_t pos = get_position();
  LOG_INF("Enabled: %s, Position: %.3gum @ %d, Target: %.3gum, Moving: %s",
          enabled ? "true" : "false", nm_as_um(steps_to_nm(pos)), pos,
          nm_as_um(steps_to_nm(target_position)), is_moving ? "true" : "false");
}

int StepperWithTarget::enable() {
  int ret = stepper_enable(stepper_dev);
  if (ret == 0) {
    enabled = true;
  }
  return ret;
}

int StepperWithTarget::disable() {
  int ret = stepper_disable(stepper_dev);
  if (ret == 0) {
    enabled = false;
  }
  return ret;
}

void StepperWithTarget::start() {
  if (!enabled) {
    enable();
  }
}

void StepperWithTarget::pause() {
  stepper_stop(stepper_dev);
  is_moving = false;
}

void StepperWithTarget::wait_and_pause() {
  LOG_DBG("wait..., currently at %d -> %d", get_position(),
          get_target_position());
  while (!is_in_target_position() && is_moving) {
    k_sleep(K_MSEC(100));
  }
  pause();
  LOG_DBG("...pause");
}

int StepperWithTarget::get_position() {
  int32_t pos;
  int ret = stepper_get_actual_position(stepper_dev, &pos);
  if (ret < 0) {
    LOG_ERR("Failed to get position: %d", ret);
    return 0;
  }
  return pos;
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
  return (steps * pitch_per_rev * 1000000) / pulses_per_rev;
}

int32_t StepperWithTarget::nm_to_steps(int32_t nm) {
  return (nm * pulses_per_rev) / (pitch_per_rev * 1000000);
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
  if (!enabled) {
    LOG_WRN("Stepper not enabled");
    return false;
  }

  int32_t current_pos = get_position();
  int32_t steps_to_move = target_position - current_pos;

  if (steps_to_move == 0) {
    is_moving = false;
    return true; // Already at target
  }

  LOG_DBG("step_towards_target: current=%d, target=%d, delta=%d", current_pos,
          target_position, steps_to_move);

  is_moving = true;
  int ret = stepper_move_by(stepper_dev, steps_to_move);
  if (ret < 0) {
    LOG_ERR("Failed to move stepper: %d", ret);
    is_moving = false;
    return false;
  }

  return false; // Not yet at target
}

bool StepperWithTarget::is_in_target_position() {
  return get_position() == get_target_position();
}

const struct stepper_with_target_status StepperWithTarget::get_status() {
  return {
      .actual_position = get_position(),
      .is_moving = is_moving,
      .target_position = target_position,
  };
}

StepperWithTarget *started_stepper_ptr;

void stepper_work_handler(struct k_work *work) {
  ARG_UNUSED(work);
  if (started_stepper_ptr == NULL) {
    return;
  }
  started_stepper_ptr->step_towards_target();
}
K_WORK_DEFINE(stepper_work, stepper_work_handler);

void stepper_expiry_function(struct k_timer *timer_id) {
  ARG_UNUSED(timer_id);
  k_work_submit(&stepper_work);
}
K_TIMER_DEFINE(stepper_timer, stepper_expiry_function, NULL);

void start_stepper(StepperWithTarget *_started_stepper_ptr) {
  if (_started_stepper_ptr == NULL) {
    return;
  }
  started_stepper_ptr = _started_stepper_ptr;
  started_stepper_ptr->start();
  k_timer_start(&stepper_timer, K_USEC(20), K_USEC(20));
}