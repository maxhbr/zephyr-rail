#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/stepper.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "stepper_with_target/StepperWithTarget.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define STEPPER_NODE DT_NODELABEL(stepper_motor)
#define LED0_NODE DT_ALIAS(led0)

int main(void) {
  const struct device *stepper_dev = DEVICE_DT_GET(STEPPER_NODE);

  if (!device_is_ready(stepper_dev)) {
    LOG_ERR("Stepper device is not ready");
    return -1;
  }

  LOG_INF("Stepper device is ready");

  // Create StepperWithTarget instance
  StepperWithTarget stepper(stepper_dev);

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

  // Enable the stepper
  ret = stepper.enable();
  if (ret < 0) {
    LOG_ERR("Failed to enable stepper: %d", ret);
    return ret;
  }

  // Set step interval to 5ms (5000 microseconds) = 200 steps/second
  // Previous value of 20000000 (20 seconds!) was way too slow
  ret = stepper_set_microstep_interval(stepper_dev,
                                       117188); // ~117 µs , for 10 RPM
  if (ret < 0) {
    LOG_WRN("Failed to set step interval: %d", ret);
    return ret;
  }

  LOG_INF("Testing stepper motor movements with StepperWithTarget...");

  while (1) {
    // Move clockwise (forward direction - LED ON)
    LOG_INF("Moving 20000 steps clockwise");
    gpio_pin_set_dt(&led, 1); // Turn LED on for forward direction

    stepper.go_relative(20000);
    stepper.log_state();
    stepper.step_towards_target();

    // Wait for movement to complete
    stepper.wait_and_pause();

    LOG_INF("Clockwise movement finished, pausing...");
    k_sleep(K_SECONDS(1));

    // Move counter-clockwise (reverse direction - LED OFF)
    LOG_INF("Moving 20000 steps counter-clockwise");
    gpio_pin_set_dt(&led, 0); // Turn LED off for reverse direction

    stepper.go_relative(-20000);
    stepper.log_state();
    stepper.step_towards_target();

    // Wait for movement to complete
    stepper.wait_and_pause();

    LOG_INF("Counter-clockwise movement finished, pausing...");
    k_sleep(K_SECONDS(1));
  }

  return 0;
}