#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/stepper.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#ifdef CONFIG_BT
#include <zephyr/bluetooth/bluetooth.h>
#ifdef CONFIG_BT_SETTINGS
#include <zephyr/settings/settings.h>
#endif
#endif

#include "StateMachine.h"
#ifdef CONFIG_BT
#include "sony_remote/sony_remote.h"
#else
#include "sony_remote/fake_sony_remote.h"
#endif
#include "stepper_with_target/StepperWithTarget.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

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

int main(void) {
#ifdef CONFIG_BT
  LOG_DBG("main: initialize Bluetooth");
  if (int err = bt_enable(nullptr); err) {
    LOG_ERR("bt_enable failed (%d)", err);
    return err;
  }
#ifdef CONFIG_BT_SETTINGS
  // Load Bluetooth settings (bonding info, etc.)
  if (int err = settings_load(); err) {
    LOG_ERR("settings_load failed (%d)", err);
    return err;
  }
#endif
  LOG_DBG("main: initialize Sony Remote");
  SonyRemote remote("9C:50:D1:AF:76:5F");
#else
  LOG_DBG("main: initialize Dummy Sony Remote");
  SonyRemote remote;
#endif
  k_msleep(100); // Match POC timing
  remote.begin();
  k_msleep(100); // Match POC timing
  remote.startScan();

  LOG_DBG("main: initialize stepper");
  StepperWithTarget *stepper = init_stepper();
  if (stepper == nullptr) {
    LOG_ERR("Failed to initialize stepper");
    return -1;
  }
  StateMachine sm(stepper, &remote);

  int32_t ret;
  while (1) {
    LOG_DBG("loop...");
    ret = sm.run_state_machine();
    if (ret) {
      break;
    }
    // stepper->go_relative(20000);
    // stepper->log_state();
    // stepper->step_towards_target();
    // stepper->wait_and_pause();

    // k_sleep(K_SECONDS(1));

    // stepper->go_relative(-20000);
    // stepper->log_state();
    // stepper->step_towards_target();
    // stepper->wait_and_pause();

    k_sleep(K_SECONDS(1));
  }

  remote.end();

  return 0;
}
