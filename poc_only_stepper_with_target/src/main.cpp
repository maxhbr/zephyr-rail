#include <zephyr/device.h>
#include <zephyr/drivers/stepper.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define STEPPER_NODE DT_NODELABEL(stepper_motor)

int main(void) {
  const struct device *stepper_dev = DEVICE_DT_GET(STEPPER_NODE);

  if (!device_is_ready(stepper_dev)) {
    LOG_ERR("Stepper device is not ready");
    return -1;
  }

  LOG_INF("Stepper device is ready");

  int ret = stepper_enable(stepper_dev);
  if (ret < 0) {
    LOG_ERR("Failed to enable stepper: %d", ret);
    return ret;
  }

  //   // Set micro stepping resolution (if supported)
  //   ret = stepper_set_micro_step_res(stepper_dev, STEPPER_MICRO_STEP_16);
  //   if (ret < 0) {
  //     LOG_WRN("Failed to set micro step resolution: %d", ret);
  //   }

  ret = stepper_set_microstep_interval(stepper_dev,
                                       20000000); // 20ms between steps
  if (ret < 0) {
    LOG_WRN("Failed to set step interval: %d", ret);
  }

  LOG_INF("Testing stepper motor movements...");

  while (1) {
    LOG_INF("Moving 200 steps clockwise");
    ret = stepper_move_by(stepper_dev, 200);
    if (ret < 0) {
      LOG_ERR("Failed to move stepper: %d", ret);
    }

    k_sleep(K_SECONDS(2));

    LOG_INF("Moving 200 steps counter-clockwise");
    ret = stepper_move_by(stepper_dev, -200);
    if (ret < 0) {
      LOG_ERR("Failed to move stepper: %d", ret);
    }

    k_sleep(K_SECONDS(2));
  }

  return 0;
}