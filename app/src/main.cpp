#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/stepper.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#ifdef CONFIG_SHELL
#include <zephyr/shell/shell.h>
#endif
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
  }

  remote.end();

  return 0;
}

#ifdef CONFIG_SHELL
static int cmd_rail_go(const struct shell *sh, size_t argc, char **argv) {

  if (argc != 2) {
    shell_print(sh, "Usage: rail go <distance>");
    return -EINVAL;
  }

  int distance = atoi(argv[1]);

  event_pub(EVENT_GO, distance);

  return 0;
}

static int cmd_rail_go_to(const struct shell *sh, size_t argc, char **argv) {
  if (argc != 2) {
    shell_print(sh, "Usage: rail go_to <absolute_position>");
    return -EINVAL;
  }

  int position = atoi(argv[1]);

  event_pub(EVENT_GO_TO, position);

  return 0;
}

static int cmd_rail_go_setLowerBound(const struct shell *sh, size_t argc,
                                     char **argv) {
  if (argc == 2) {
    int position = atoi(argv[1]);
    event_pub(EVENT_SET_LOWER_BOUND_TO, position);
  } else {
    event_pub(EVENT_SET_LOWER_BOUND);
  }
  return 0;
}

static int cmd_rail_go_setUpperBound(const struct shell *sh, size_t argc,
                                     char **argv) {
  if (argc == 2) {
    int position = atoi(argv[1]);
    event_pub(EVENT_SET_UPPER_BOUND_TO, position);
  } else {
    event_pub(EVENT_SET_UPPER_BOUND);
  }
  return 0;
}

static int cmd_rail_go_startStack(const struct shell *sh, size_t argc,
                                  char **argv) {
  if (argc != 2) {
    shell_print(sh, "Usage: rail startStack <expected_length_of_stack>");
    return -EINVAL;
  }

  int expected_length_of_stack = atoi(argv[1]);

  event_pub(EVENT_START_STACK, expected_length_of_stack);

  return 0;
}

static int cmd_rail_params(const struct shell *sh, size_t argc, char **argv) {
  int cnt;

  shell_print(sh, "argc = %d", argc);
  for (cnt = 0; cnt < argc; cnt++) {
    shell_print(sh, "  argv[%d] = %s", cnt, argv[cnt]);
  }
  return 0;
}

/* Creating subcommands (level 1 command) array for command "rail". */
SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_rail, SHELL_CMD(go, NULL, "Go relative.", cmd_rail_go),
    SHELL_CMD(go_to, NULL, "Go to absolute position.", cmd_rail_go_to),
    SHELL_CMD(lower, NULL, "Set lower bound.", cmd_rail_go_setLowerBound),
    SHELL_CMD(upper, NULL, "Set upper bound.", cmd_rail_go_setUpperBound),
    SHELL_CMD(stack, NULL, "Start stacking.", cmd_rail_go_startStack),
    SHELL_SUBCMD_SET_END);
/* Creating root (level 0) command "rail" without a handler */
SHELL_CMD_REGISTER(rail, &sub_rail, "rail commands", NULL);
#endif