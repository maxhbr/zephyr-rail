#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/stepper.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define STEPPER_NODE DT_NODELABEL(stepper_motor)
#define LED0_NODE DT_ALIAS(led0)

// Callback for stepper events
static volatile bool movement_complete = false;

static void stepper_event_callback(const struct device *dev,
                                   const enum stepper_event event,
                                   void *user_data) {
  switch (event) {
  case STEPPER_EVENT_STEPS_COMPLETED:
    LOG_INF("Movement completed!");
    movement_complete = true;
    break;
  case STEPPER_EVENT_STALL_DETECTED:
    LOG_WRN("Stall detected!");
    break;
  case STEPPER_EVENT_STOPPED:
    LOG_INF("Stepper stopped");
    break;
  default:
    LOG_DBG("Stepper event: %d", event);
    break;
  }
}

int main(void) {
  const struct device *stepper_dev = DEVICE_DT_GET(STEPPER_NODE);

  if (!device_is_ready(stepper_dev)) {
    LOG_ERR("Stepper device is not ready");
    return -1;
  }

  LOG_INF("Stepper device is ready");

  // Set up LED
  static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

  if (!gpio_is_ready_dt(&led)) {
    LOG_ERR("LED device is not ready");
    return -1;
  }

  int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
  if (ret < 0) {
    LOG_ERR("Failed to configure LED: %d", ret);
    return ret;
  }

  LOG_INF("LED is ready");

  // Set up event callback
  ret = stepper_set_event_callback(stepper_dev, stepper_event_callback, NULL);
  if (ret < 0) {
    LOG_WRN("Failed to set event callback: %d", ret);
  }

  ret = stepper_enable(stepper_dev);
  if (ret < 0) {
    LOG_ERR("Failed to enable stepper: %d", ret);
    return ret;
  }

  // Set step interval to 5ms (5000 microseconds) = 200 steps/second
  // Previous value of 20000000 (20 seconds!) was way too slow
  ret =
      stepper_set_microstep_interval(stepper_dev, 5000000); // 5ms between steps
  if (ret < 0) {
    LOG_WRN("Failed to set step interval: %d", ret);
    return ret;
  }

  LOG_INF("Testing stepper motor movements...");

  while (1) {
    // Move clockwise (forward direction - LED ON)
    LOG_INF("Moving 200 steps clockwise");
    gpio_pin_set_dt(&led, 1); // Turn LED on for forward direction
    movement_complete = false;
    ret = stepper_move_by(stepper_dev, 20000);
    if (ret < 0) {
      LOG_ERR("Failed to move stepper: %d", ret);
      k_sleep(K_SECONDS(1));
      continue;
    }

    // Wait for movement to complete
    while (!movement_complete) {
      k_sleep(K_MSEC(100));
    }

    LOG_INF("Clockwise movement finished, pausing...");
    k_sleep(K_SECONDS(1));

    // Move counter-clockwise (reverse direction - LED OFF)
    LOG_INF("Moving 200 steps counter-clockwise");
    gpio_pin_set_dt(&led, 0); // Turn LED off for reverse direction
    movement_complete = false;
    ret = stepper_move_by(stepper_dev, -20000);
    if (ret < 0) {
      LOG_ERR("Failed to move stepper: %d", ret);
      k_sleep(K_SECONDS(1));
      continue;
    }

    // Wait for movement to complete
    while (!movement_complete) {
      k_sleep(K_MSEC(100));
    }

    LOG_INF("Counter-clockwise movement finished, pausing...");
    k_sleep(K_SECONDS(1));
  }

  return 0;
}