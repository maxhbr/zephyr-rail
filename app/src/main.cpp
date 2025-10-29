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
#include <zephyr/bluetooth/conn.h>
#ifdef CONFIG_BT_SETTINGS
#include <zephyr/settings/settings.h>
#endif
#endif

#include "StateMachine.h"
#ifdef CONFIG_BT
#include "pwa_service.h"
#include "sony_remote/sony_remote.h"
#else
#include "sony_remote/fake_sony_remote.h"
#endif
#include "stepper_with_target/StepperWithTarget.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

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

  uint32_t micro_step_res = DT_PROP(STEPPER_NODE, micro_step_res);

  static StepperWithTarget stepper(stepper_dev, 1, 200 * micro_step_res);

  ret = stepper.enable();
  if (ret < 0) {
    LOG_ERR("Failed to enable stepper: %d", ret);
    return nullptr;
  }

  return &stepper;
}

#ifdef CONFIG_BT
static void conn_connected(struct bt_conn *conn, uint8_t err) {
  if (err) {
    LOG_ERR("Connection failed: %u", err);
    return;
  }

  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role == BT_CONN_ROLE_PERIPHERAL) {
    // This is a PWA connection (we are peripheral/server)
    PwaService::onConnected(conn, err);
  }
}

static void conn_disconnected(struct bt_conn *conn, uint8_t reason) {
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role == BT_CONN_ROLE_PERIPHERAL) {
    // PWA disconnected
    PwaService::onDisconnected(conn, reason);
  }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {.connected = conn_connected,
                                     .disconnected = conn_disconnected};
#endif

void led_blink(const struct gpio_dt_spec *led, int times, int on_ms,
               int off_ms) {
  if (!gpio_is_ready_dt(led)) {
    return;
  }

  for (int i = 0; i < times; i++) {
    gpio_pin_set(led->port, led->pin, 1);
    k_msleep(on_ms);
    gpio_pin_set(led->port, led->pin, 0);
    k_msleep(off_ms);
  }
}

bool led_toggle(const struct gpio_dt_spec *led, bool was_on) {
  if (!gpio_is_ready_dt(led)) {
    return;
  }

  bool led_on = !was_on;
  gpio_pin_set(led->port, led->pin, led_on ? 1 : 0);
  return led_on;
}

int main(void) {
  int32_t ret;
  static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
  const struct device *led_dev = nullptr;

  if (gpio_is_ready_dt(&led)) {
    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    if (ret >= 0) {
      led_dev = led.port;
    }
  }

  led_blink(&led, 1, 100, 100);

#ifdef CONFIG_BT
  LOG_INF("initialize Bluetooth ...");
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

  // Initialize PWA service
  LOG_INF("initialize PWA service ...");
  PwaService::init();

  // Start advertising for PWA connections (peripheral role)
  if (int err = PwaService::startAdvertising(); err) {
    LOG_ERR("Failed to start PWA advertising: %d", err);
    return err;
  }
  LOG_DBG("PWA service ready - Web Bluetooth interface available");

  LOG_INF("initialize Sony Remote ...");
  SonyRemote remote("9C:50:D1:AF:76:5F");
#else
  LOG_INF("initialize Dummy Sony Remote ...");
  SonyRemote remote;
#endif
  led_blink(&led, 2, 100, 100);
  k_msleep(100); // Match POC timing
  remote.begin();

  k_msleep(100); // Match POC timing
  remote.startScan();

  led_blink(&led, 3, 100, 100);

  LOG_INF("initialize stepper ...");
  StepperWithTarget *stepper = init_stepper();
  if (stepper == nullptr) {
    LOG_ERR("Failed to initialize stepper");
    return -1;
  }

  StateMachine sm(stepper, &remote);
  bool led_on = false;
  LOG_INF("entering main loop ...");
  while (1) {
    LOG_DBG("loop...");
    led_on = led_toggle(&led, led_on);
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

static int cmd_rail_go_pct(const struct shell *sh, size_t argc, char **argv) {
  if (argc != 2) {
    shell_print(sh, "Usage: rail go_pct <percentage>");
    return -EINVAL;
  }

  int percentage = atoi(argv[1]);
  if (percentage < 0 || percentage > 100) {
    shell_print(sh, "Percentage must be between 0 and 100");
    return -EINVAL;
  }

  event_pub(EVENT_GO_PCT, percentage);

  return 0;
}

static int cmd_rail_setLowerBound(const struct shell *sh, size_t argc,
                                  char **argv) {
  if (argc == 2) {
    int position = atoi(argv[1]);
    event_pub(EVENT_SET_LOWER_BOUND_TO, position);
  } else {
    event_pub(EVENT_SET_LOWER_BOUND);
  }
  return 0;
}

static int cmd_rail_setUpperBound(const struct shell *sh, size_t argc,
                                  char **argv) {
  if (argc == 2) {
    int position = atoi(argv[1]);
    event_pub(EVENT_SET_UPPER_BOUND_TO, position);
  } else {
    event_pub(EVENT_SET_UPPER_BOUND);
  }
  return 0;
}

static int cmd_rail_setWaitBefore(const struct shell *sh, size_t argc,
                                  char **argv) {
  if (argc != 2) {
    shell_print(sh, "Usage: rail wait_before <milliseconds>");
    return -EINVAL;
  }

  int wait_ms = atoi(argv[1]);
  event_pub(EVENT_SET_WAIT_BEFORE_MS, wait_ms);
  return 0;
}

static int cmd_rail_setWaitAfter(const struct shell *sh, size_t argc,
                                 char **argv) {
  if (argc != 2) {
    shell_print(sh, "Usage: rail wait_after <milliseconds>");
    return -EINVAL;
  }

  int wait_ms = atoi(argv[1]);
  event_pub(EVENT_SET_WAIT_AFTER_MS, wait_ms);
  return 0;
}

static int cmd_rail_startStack(const struct shell *sh, size_t argc,
                               char **argv) {
  if (argc == 2) {
    int expected_length_of_stack = atoi(argv[1]);
    event_pub(EVENT_START_STACK, expected_length_of_stack);
  } else if (argc == 4) {
    int expected_length_of_stack = atoi(argv[1]);
    int lower = atoi(argv[2]);
    int upper = atoi(argv[3]);
    event_pub(EVENT_SET_LOWER_BOUND_TO, lower);
    event_pub(EVENT_SET_UPPER_BOUND_TO, upper);
    event_pub(EVENT_START_STACK, expected_length_of_stack);
  } else if (argc > 2) {
    shell_print(sh, "Usage: rail startStack <expected_length_of_stack>");
    return -EINVAL;
  } else {
    event_pub(EVENT_START_STACK, 100);
  }
  return 0;
}

static int cmd_rail_shoot(const struct shell *sh, size_t argc, char **argv) {
  event_pub(EVENT_SHOOT);
  return 0;
}

/* Creating subcommands (level 1 command) array for command "rail". */
SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_rail, SHELL_CMD(go, NULL, "Go relative.", cmd_rail_go),
    SHELL_CMD(go_to, NULL, "Go to absolute position.", cmd_rail_go_to),
    SHELL_CMD(go_pct, NULL, "Go to percentage between upper and lower bound.",
              cmd_rail_go_pct),
    SHELL_CMD(lower, NULL, "Set lower bound.", cmd_rail_setLowerBound),
    SHELL_CMD(upper, NULL, "Set upper bound.", cmd_rail_setUpperBound),
    SHELL_CMD(wait_before, NULL, "Set wait before ms.", cmd_rail_setWaitBefore),
    SHELL_CMD(wait_after, NULL, "Set wait after ms.", cmd_rail_setWaitAfter),
    SHELL_CMD(stack, NULL, "Start stacking.", cmd_rail_startStack),
    SHELL_CMD(shoot, NULL, "Trigger camera shoot.", cmd_rail_shoot),
    SHELL_SUBCMD_SET_END);
/* Creating root (level 0) command "rail" without a handler */
SHELL_CMD_REGISTER(rail, &sub_rail, "rail commands", NULL);
#endif