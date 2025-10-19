#include "stepper_with_target/StepperWithTarget.h"

LOG_MODULE_REGISTER(stepper_with_target, LOG_LEVEL_DBG);

StepperWithTarget::StepperWithTarget(const struct device *dev,
                                     const struct device *led_dev) {
  stepper_dev = dev;
  this->led_dev = led_dev;

  if (!device_is_ready(stepper_dev)) {
    LOG_ERR("Stepper device is not ready");
    return;
  }

  // Check if LED device is provided and ready
  if (led_dev != nullptr) {
    if (!device_is_ready(led_dev)) {
      LOG_WRN("LED device is not ready, disabling LED support");
      this->led_dev = nullptr;
    }
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

  ret = stepper_set_microstep_interval(stepper_dev,
                                       117188); // ~117 µs , for 10 RPM
  if (ret < 0) {
    LOG_WRN("Failed to set step interval: %d", ret);
  }

  // Initialize LED to off (not moving)
  update_led();
}

void StepperWithTarget::update_led() {
  if (led_dev == nullptr) {
    return;
  }

  // LED on when moving, off when at target
  int ret = gpio_pin_set(led_dev, 0, is_moving ? 1 : 0);
  if (ret < 0) {
    LOG_ERR("Failed to set LED state: %d", ret);
  }
}

void StepperWithTarget::event_callback_wrapper(const struct device *dev,
                                               const enum stepper_event event,
                                               void *user_data) {
  StepperWithTarget *instance = static_cast<StepperWithTarget *>(user_data);

  switch (event) {
  case STEPPER_EVENT_STEPS_COMPLETED:
    LOG_DBG("Movement completed!");
    instance->is_moving = false;
    instance->update_led();
    break;
  case STEPPER_EVENT_STALL_DETECTED:
    LOG_WRN("Stall detected!");
    instance->is_moving = false;
    instance->update_led();
    break;
  case STEPPER_EVENT_STOPPED:
    LOG_DBG("Stepper stopped");
    instance->is_moving = false;
    instance->update_led();
    break;
  default:
    LOG_DBG("Stepper event: %d", event);
    break;
  }
}

void StepperWithTarget::log_state() {
  int32_t pos = get_position();
  LOG_INF("Enabled: %s, Position: %d, Target: %d, Moving: %s",
          enabled ? "true" : "false", pos, target_position,
          is_moving ? "true" : "false");
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
  update_led();
}

void StepperWithTarget::wait_and_pause() {
  LOG_INF("wait...");
  while (!is_in_target_position() && is_moving) {
    k_sleep(K_MSEC(100));
  }
  pause();
  LOG_INF("...pause");
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

int StepperWithTarget::go_relative(int32_t dist) {
  target_position += dist;
  return target_position;
}

void StepperWithTarget::set_target_position(int32_t _target_position) {
  target_position = _target_position;
}

int32_t StepperWithTarget::get_target_position() { return target_position; }

bool StepperWithTarget::step_towards_target() {
  if (!enabled) {
    LOG_WRN("Stepper not enabled");
    return false;
  }

  int32_t current_pos = get_position();
  int32_t steps_to_move = target_position - current_pos;

  if (steps_to_move == 0) {
    is_moving = false;
    update_led();
    return true; // Already at target
  }

  LOG_DBG("step_towards_target: current=%d, target=%d, steps=%d", current_pos,
          target_position, steps_to_move);

  is_moving = true;
  update_led();
  int ret = stepper_move_by(stepper_dev, steps_to_move);
  if (ret < 0) {
    LOG_ERR("Failed to move stepper: %d", ret);
    is_moving = false;
    update_led();
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