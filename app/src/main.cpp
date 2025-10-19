#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#ifdef CONFIG_STEPPER
#include <zephyr/drivers/stepper.h>
#endif
#ifdef CONFIG_BT
#include <zephyr/bluetooth/bluetooth.h>
#endif
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#ifdef CONFIG_BT_SETTINGS
#include <zephyr/settings/settings.h>
#endif

#ifdef CONFIG_BT
#include "sony_remote/sony_remote.h"
#endif
#ifdef CONFIG_STEPPER
#include "stepper_with_target/StepperWithTarget.h"
#endif

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#ifdef CONFIG_STEPPER
#define STEPPER_NODE DT_NODELABEL(stepper_motor)
#define LED0_NODE DT_ALIAS(led0)

static StepperWithTarget *init_stepper(void) {
  int ret;
  const struct device *stepper_dev = DEVICE_DT_GET(STEPPER_NODE);

  if (!device_is_ready(stepper_dev)) {
    LOG_ERR("Stepper device is not ready");
    return nullptr;
  }

  LOG_INF("Stepper device is ready");

  static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
  const struct device *led_dev = nullptr;

  if (gpio_is_ready_dt(&led)) {
    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    if (ret >= 0) {
      led_dev = led.port;
    }
  }

  static StepperWithTarget stepper(stepper_dev, led_dev);

  ret = stepper.enable();
  if (ret < 0) {
    LOG_ERR("Failed to enable stepper: %d", ret);
    return nullptr;
  }

  return &stepper;
}
#endif

int main(void) {
#ifdef CONFIG_STEPPER
  LOG_DBG("main: initialize stepper");
  StepperWithTarget *stepper = init_stepper();
  if (stepper == nullptr) {
    LOG_ERR("Failed to initialize stepper");
    return -1;
  }
  LOG_DBG("main: initialize Bluetooth");
  if (int err = bt_enable(nullptr); err) {
    LOG_ERR("bt_enable failed (%d)", err);
    return err;
  }
#endif

#ifdef CONFIG_BT_SETTINGS
  // Load Bluetooth settings (bonding info, etc.)
  if (int err = settings_load(); err) {
    LOG_ERR("settings_load failed (%d)", err);
    return err;
  }
#endif

#ifdef CONFIG_BT
  LOG_DBG("main: initialize Sony Remote");
  SonyRemote remote("9C:50:D1:AF:76:5F");
  remote.begin();
  k_msleep(100);
  LOG_DBG("main: start Sony Remote scan");
  remote.startScan();
  k_msleep(100);
#endif

  while (1) {
    LOG_DBG("loop...");
#ifdef CONFIG_BT
    if (remote.ready()) {
      LOG_INF("Connected to camera...");
    } else {
      LOG_DBG("Waiting for camera connection...");
    }
#endif
#ifdef CONFIG_STEPPER
    stepper->go_relative(20000);
    stepper->log_state();
    stepper->step_towards_target();
    stepper->wait_and_pause();
#endif

    k_sleep(K_SECONDS(1));

#ifdef CONFIG_STEPPER
    stepper->go_relative(-20000);
    stepper->log_state();
    stepper->step_towards_target();
    stepper->wait_and_pause();
#endif

    k_sleep(K_SECONDS(1));
  }

  return 0;
}
