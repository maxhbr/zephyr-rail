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

  // Set up LED
  static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
  const struct device *led_dev = nullptr;

  if (gpio_is_ready_dt(&led)) {
    int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
      LOG_ERR("Failed to configure LED: %d", ret);
    } else {
      led_dev = led.port;
      LOG_INF("LED is ready and will be controlled by stepper");
    }
  } else {
    LOG_WRN("LED device is not ready, stepper will run without LED indicator");
  }

  // Create StepperWithTarget instance with LED
  StepperWithTarget stepper(stepper_dev, led_dev);

  // Enable the stepper
  int ret = stepper.enable();
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
  LOG_INF("LED will automatically turn ON when moving and OFF when at target");

  while (1) {
    // Move clockwise (LED will turn ON automatically)
    LOG_INF("Moving 20000 steps clockwise");

    stepper.go_relative(20000);
    stepper.log_state();
    stepper.step_towards_target();

    // Wait for movement to complete (LED will turn OFF automatically)
    stepper.wait_and_pause();

    LOG_INF("Clockwise movement finished, pausing...");
    k_sleep(K_SECONDS(1));

    // Move counter-clockwise (LED will turn ON automatically)
    LOG_INF("Moving 20000 steps counter-clockwise");

    stepper.go_relative(-20000);
    stepper.log_state();
    stepper.step_towards_target();

    // Wait for movement to complete (LED will turn OFF automatically)
    stepper.wait_and_pause();

    LOG_INF("Counter-clockwise movement finished, pausing...");
    k_sleep(K_SECONDS(1));
  }

  return 0;
}