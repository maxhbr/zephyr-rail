#include "stepper_with_target/StepperWithTarget.h"

LOG_MODULE_REGISTER(stepper_with_target, LOG_LEVEL_INF);

StepperWithTarget::StepperWithTarget(const struct device *dev,
                                     int _pitch_per_rev_mm,
                                     int _pulses_per_rev) {
  stepper_dev = dev;
  pitch_per_rev_nm = _pitch_per_rev_mm * 1000000;
  pulses_per_rev = _pulses_per_rev;

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

  LOG_INF("%s initialized with pitch_per_rev=%.3fum, pulses_per_rev=%d",
          __FUNCTION__, nm_as_um(pitch_per_rev_nm), pulses_per_rev);
}

int StepperWithTarget::set_speed(StepperSpeed speed) {
  int rpm = 10;
  switch (speed) {
  case StepperSpeed::FAST:
    rpm = 15;
    break;
  case StepperSpeed::MEDIUM:
    rpm = 10;
    break;
  case StepperSpeed::SLOW:
    rpm = 5;
    break;
  }
  return set_speed_rpm(rpm);
}

int StepperWithTarget::set_speed_rpm(int rpm) {
  if (rpm < 1) {
    LOG_WRN("Requested RPM %d is too low. Must be >= 1.", rpm);
    return -EINVAL;
  }
  if (pulses_per_rev <= 0) {
    LOG_ERR("Invalid pulses per revolution: %d", pulses_per_rev);
    return -EINVAL;
  }

  const uint64_t nsec_per_min = 60000000000ULL; // 60 seconds * 1e9 ns
  uint64_t steps_per_min =
      static_cast<uint64_t>(rpm) * static_cast<uint64_t>(pulses_per_rev);
  if (steps_per_min == 0) {
    return -EINVAL;
  }

  uint64_t interval_ns = nsec_per_min / steps_per_min;
  if (interval_ns == 0) {
    interval_ns = 1; // best-effort clamp
  }

  int ret = stepper_set_microstep_interval(stepper_dev, interval_ns);
  if (ret < 0) {
    LOG_WRN("Failed to set RPM-based interval: %d", ret);
  } else {
    LOG_INF("Stepper speed set to %d RPM (interval %llu ns)", rpm, interval_ns);
  }
  return ret;
}

void StepperWithTarget::event_callback_wrapper(const struct device *dev,
                                               const enum stepper_event event,
                                               void *user_data) {
  StepperWithTarget *instance = static_cast<StepperWithTarget *>(user_data);

  switch (event) {
  case STEPPER_EVENT_STEPS_COMPLETED:
    LOG_INF("Movement completed!");
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

char *StepperWithTarget::state() {
  int32_t pos = get_position();
  static char buffer[128];
  snprintf(
      buffer, sizeof(buffer),
      "Enabled: %s, Position: %.3fum @ %d, Target: %.3fum @ %d, Moving: %s",
      enabled ? "true" : "false", nm_as_um(steps_to_nm(pos)), pos,
      nm_as_um(steps_to_nm(target_position)), target_position,
      is_moving ? "true" : "false");
  return buffer;
}

void StepperWithTarget::log_state() {
  int32_t pos = get_position();
  LOG_INF("%s", state());
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
  // Use int64_t for calculation to avoid overflow
  int64_t result =
      ((int64_t)steps * (int64_t)pitch_per_rev_nm) / (int64_t)pulses_per_rev;
  return (int32_t)result;
}

int32_t StepperWithTarget::nm_to_steps(int32_t nm) {
  // Use int64_t for calculation to avoid overflow
  int64_t result =
      ((int64_t)nm * (int64_t)pulses_per_rev) / (int64_t)pitch_per_rev_nm;
  return (int32_t)result;
}

int32_t StepperWithTarget::go_relative_nm(int32_t nm) {
  int32_t steps = nm_to_steps(nm);
  return go_relative(steps);
}
void StepperWithTarget::set_target_position_nm(int32_t _target_position) {
  int32_t steps = nm_to_steps(_target_position);
  LOG_DBG("set_target_position_nm: nm=%d -> steps=%d", _target_position, steps);
  set_target_position(steps);
}
int32_t StepperWithTarget::get_target_position_nm() {
  int32_t steps = get_target_position();
  int32_t nm = steps_to_nm(steps);
  LOG_DBG("get_target_position_nm: steps=%d -> nm=%d", steps, nm);
  return nm;
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
