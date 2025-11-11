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

#ifdef CONFIG_BT

static void debug_log_bluetooth_information(struct bt_conn *conn) {
  char addr[BT_ADDR_LE_STR_LEN];
  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
  LOG_DBG("Bluetooth info for %s:", addr);

  bt_conn_info info;
  bt_conn_get_info(conn, &info);
  LOG_DBG("  Role: %s",
          info.role == BT_CONN_ROLE_CENTRAL ? "Central" : "Peripheral");

  bt_security_t sec_level = bt_conn_get_security(conn);
  switch (sec_level) {
  case BT_SECURITY_L0:
    LOG_DBG("  Current security level: BT_SECURITY_L0 (no security)");
    break;
  case BT_SECURITY_L1:
    LOG_DBG("  Current security level: BT_SECURITY_L1 (unencrypted)");
    break;
  case BT_SECURITY_L2:
    LOG_DBG("  Current security level: BT_SECURITY_L2 (encrypted, no MITM)");
    break;
  case BT_SECURITY_L3:
    LOG_DBG("  Current security level: BT_SECURITY_L3 (encrypted with MITM)");
    break;
  case BT_SECURITY_L4:
    LOG_DBG("  Current security level: BT_SECURITY_L4 (LE Secure Connections)");
    break;
  default:
    LOG_DBG("  Current security level: Unknown (%d)", sec_level);
    break;
  }
}

// Unified connection callbacks that delegate to appropriate implementation
static void unified_connected(struct bt_conn *conn, uint8_t err) {
  debug_log_bluetooth_information(conn);
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role == BT_CONN_ROLE_PERIPHERAL) {
    // We are acting as peripheral/server - this is a PWA connection
    PwaService::onConnected(conn, err);
  } else if (info.role == BT_CONN_ROLE_CENTRAL) {
    // We are acting as central/client - this is a Sony camera connection
    SonyRemote::on_connected(conn, err);
  }
}

static void unified_disconnected(struct bt_conn *conn, uint8_t reason) {
  debug_log_bluetooth_information(conn);
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role == BT_CONN_ROLE_PERIPHERAL) {
    PwaService::onDisconnected(conn, reason);
  } else if (info.role == BT_CONN_ROLE_CENTRAL) {
    SonyRemote::on_disconnected(conn, reason);
  }
}

static void unified_security_changed(struct bt_conn *conn, bt_security_t level,
                                     enum bt_security_err err) {
  debug_log_bluetooth_information(conn);
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role == BT_CONN_ROLE_CENTRAL) {
    // Sony camera connection
    SonyRemote::on_security_changed(conn, level, err);
  } else if (info.role == BT_CONN_ROLE_PERIPHERAL) {
    // PWA connection - just log the security change
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (err) {
      LOG_ERR("PWA security failed: %s level %u err %d", addr, level, err);
    } else {
      LOG_INF("PWA security changed: %s level %u", addr, level);
    }
  }
}

static bt_conn_cb kConnCbs = {
    .connected = unified_connected,
    .disconnected = unified_disconnected,
    .security_changed = unified_security_changed,
};

// Auth callbacks delegate to Sony Remote (only used for camera pairing)
static void unified_auth_cancel(struct bt_conn *conn) {
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role == BT_CONN_ROLE_PERIPHERAL) {
    // PWA connection - just log
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_WRN("PWA pairing cancelled: %s", addr);
    return;
  }

  // Delegate to Sony Remote for central connections
  SonyRemote::auth_cancel(conn);
}

static void unified_auth_passkey_display(struct bt_conn *conn,
                                         unsigned int passkey) {
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role == BT_CONN_ROLE_PERIPHERAL) {
    // PWA connection - just log
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("PWA passkey display for %s: %06u", addr, passkey);
    return;
  }

  // Delegate to Sony Remote for central connections
  SonyRemote::auth_passkey_display(conn, passkey);
}

static void unified_auth_passkey_confirm(struct bt_conn *conn,
                                         unsigned int passkey) {
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role == BT_CONN_ROLE_PERIPHERAL) {
    // PWA connection - auto-confirm
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("PWA confirm passkey for %s: %06u", addr, passkey);
    bt_conn_auth_passkey_confirm(conn);
    return;
  }

  // Delegate to Sony Remote for central connections
  SonyRemote::auth_passkey_confirm(conn, passkey);
}

static void unified_auth_passkey_entry(struct bt_conn *conn) {
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role == BT_CONN_ROLE_PERIPHERAL) {
    // PWA connection - just log
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("PWA passkey entry for %s", addr);
    return;
  }

  // Delegate to Sony Remote for central connections
  SonyRemote::auth_passkey_entry(conn);
}

static enum bt_security_err unified_auth_pairing_confirm(struct bt_conn *conn) {
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role == BT_CONN_ROLE_PERIPHERAL) {
    // PWA connection - auto-accept
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("PWA pairing confirm for %s - accepting", addr);
    return BT_SECURITY_ERR_SUCCESS;
  }

  // Delegate to Sony Remote for central connections
  return SonyRemote::auth_pairing_confirm(conn);
}

static bt_conn_auth_cb kAuthCbs = {
    .passkey_display = unified_auth_passkey_display,
    .passkey_entry = unified_auth_passkey_entry,
    .passkey_confirm = unified_auth_passkey_confirm,
    .cancel = unified_auth_cancel,
    .pairing_confirm = unified_auth_pairing_confirm,
};

#endif

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

static struct k_timer led_blink_timer;
static const struct gpio_dt_spec *async_led = nullptr;
static int blink_phase = 0;

static void led_blink_timer_handler(struct k_timer *timer) {
  if (async_led && gpio_is_ready_dt(async_led)) {
    if (blink_phase == 1) {
      gpio_pin_set(async_led->port, async_led->pin, 1);
      blink_phase = 0;
      k_timer_stop(&led_blink_timer);
    }
  }
}

static void init_async_led(const struct gpio_dt_spec *led) {
  async_led = led;
  blink_phase = 0;
  k_timer_init(&led_blink_timer, led_blink_timer_handler, nullptr);
  if (async_led && gpio_is_ready_dt(async_led)) {
    gpio_pin_set(async_led->port, async_led->pin, 1);
  }
}

static void trigger_async_led_blink(int blink_duration_ms) {
  if (async_led && gpio_is_ready_dt(async_led) && blink_phase == 0) {
    gpio_pin_set(async_led->port, async_led->pin, 0);
    blink_phase = 1;
    k_timer_start(&led_blink_timer, K_MSEC(blink_duration_ms), K_NO_WAIT);
  }
}

static void stop_async_led() {
  k_timer_stop(&led_blink_timer);
  if (async_led && gpio_is_ready_dt(async_led)) {
    gpio_pin_set(async_led->port, async_led->pin, 0);
  }
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

  init_async_led(&led);
  trigger_async_led_blink(100);
  k_msleep(200);

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
  // PwaService::init();

  // Start advertising for PWA connections (peripheral role)
  if (int err = PwaService::startAdvertising(); err) {
    LOG_ERR("Failed to start PWA advertising: %d", err);
    return err;
  }
  LOG_DBG("PWA service ready - Web Bluetooth interface available");

  LOG_INF("initialize Sony Remote ...");
  SonyRemote remote("9C:50:D1:AF:76:5F"); // A7Riv
  // SonyRemote remote("CC:C0:79:DA:94:B6"); // A7iii

  bt_conn_cb_register(&kConnCbs);
  bt_conn_auth_cb_register(&kAuthCbs);

  k_msleep(100);
#else
  LOG_INF("initialize Dummy Sony Remote ...");
  SonyRemote remote;
#endif

  trigger_async_led_blink(100);
  k_msleep(200);
  trigger_async_led_blink(100);
  k_msleep(200);

  LOG_INF("initialize stepper ...");
  StepperWithTarget *stepper = init_stepper();
  if (stepper == nullptr) {
    LOG_ERR("Failed to initialize stepper");
    return -1;
  }

  StateMachine sm(stepper, &remote);

#ifdef BUILD_COMMIT
  LOG_INF("Build commit: %s", BUILD_COMMIT);
#endif

  LOG_INF("entering main loop ...");
  while (1) {
    LOG_DBG("loop...");
    trigger_async_led_blink(50);
    ret = sm.run_state_machine();
    if (ret) {
      break;
    }
  }

  stop_async_led();
  remote.end();

  return 0;
}

#ifdef CONFIG_SHELL
static int cmd_rail_go(const struct shell *sh, size_t argc, char **argv) {

  if (argc != 2) {
    shell_print(sh, "Usage: rail %s <distance>", argv[0]);
    return -EINVAL;
  }

  // Check if argv[0] is `go` or `go_nm`:
  bool is_nm_command = (strcmp(argv[0], "go_nm") == 0);
  int multiplier =
      is_nm_command ? 1 : 1000; // go_nm uses nm directly, go uses um

  int distance_nm = atoi(argv[1]) * multiplier;
  event_pub(EVENT_GO, distance_nm);

  return 0;
}

static int cmd_rail_go_to(const struct shell *sh, size_t argc, char **argv) {
  if (argc != 2) {
    shell_print(sh, "Usage: rail go_to <absolute_position>");
    return -EINVAL;
  }

  int position_um = atoi(argv[1]);
  int position_nm = position_um * 1000;
  event_pub(EVENT_GO_TO, position_nm);

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
    int position_um = atoi(argv[1]);
    int position_nm = position_um * 1000;
    event_pub(EVENT_SET_LOWER_BOUND_TO, position_nm);
  } else {
    event_pub(EVENT_SET_LOWER_BOUND);
  }
  return 0;
}

static int cmd_rail_setUpperBound(const struct shell *sh, size_t argc,
                                  char **argv) {
  if (argc == 2) {
    int position_um = atoi(argv[1]);
    int position_nm = position_um * 1000;
    event_pub(EVENT_SET_UPPER_BOUND_TO, position_nm);
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

static int cmd_rail_startStackWithStepSize(const struct shell *sh, size_t argc,
                                           char **argv) {
  bool is_nm_command = (strcmp(argv[0], "stack_nm") == 0);
  int multiplier =
      is_nm_command ? 1 : 1000; // stack_nm uses nm directly, stack uses um

  if (argc == 2) {
    int expected_step_size_nm = atoi(argv[1]) * multiplier;
    event_pub(EVENT_START_STACK, expected_step_size_nm);
  } else if (argc == 4) {
    int expected_step_size_nm = atoi(argv[1]) * multiplier;
    int lower_nm = atoi(argv[2]) * multiplier;
    int upper_nm = atoi(argv[3]) * multiplier;
    event_pub(EVENT_SET_UPPER_BOUND_TO, upper_nm);
    event_pub(EVENT_SET_LOWER_BOUND_TO, lower_nm);
    event_pub(EVENT_START_STACK, expected_step_size_nm);
  } else if (argc > 2) {
    shell_print(sh, "Usage: rail %s <expected_step_size> [lower upper]",
                argv[0]);
    return -EINVAL;
  } else {
    event_pub(EVENT_START_STACK, 1000);
  }
  return 0;
}
static int cmd_rail_startStackWithLength(const struct shell *sh, size_t argc,
                                         char **argv) {
  if (argc == 2) {
    int expected_length_of_stack = atoi(argv[1]);
    event_pub(EVENT_START_STACK_WITH_LENGTH, expected_length_of_stack);
  } else if (argc == 4) {
    int expected_length_of_stack = atoi(argv[1]);
    int lower_nm = atoi(argv[2]) * 1000;
    int upper_nm = atoi(argv[3]) * 1000;
    event_pub(EVENT_SET_UPPER_BOUND_TO, upper_nm);
    event_pub(EVENT_SET_LOWER_BOUND_TO, lower_nm);
    event_pub(EVENT_START_STACK_WITH_LENGTH, expected_length_of_stack);
  } else if (argc > 2) {
    shell_print(sh, "Usage: rail startStack <expected_length_of_stack>");
    return -EINVAL;
  } else {
    event_pub(EVENT_START_STACK_WITH_LENGTH, 100);
  }
  return 0;
}

static int cmd_rail_status(const struct shell *sh, size_t argc, char **argv) {
  event_pub(EVENT_STATUS);
  return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_rail, SHELL_CMD(go, NULL, "Go relative.", cmd_rail_go),
    SHELL_CMD(g, NULL, "Go relative.", cmd_rail_go),
    SHELL_CMD(go_nm, NULL, "Go relative (nm).", cmd_rail_go),
    SHELL_CMD(go_to, NULL, "Go to absolute position.", cmd_rail_go_to),
    SHELL_CMD(go_pct, NULL, "Go to percentage between upper and lower bound.",
              cmd_rail_go_pct),
    SHELL_CMD(p, NULL, "Go to percentage between upper and lower bound.",
              cmd_rail_go_pct),
    SHELL_CMD(lower, NULL, "Set lower bound.", cmd_rail_setLowerBound),
    SHELL_CMD(upper, NULL, "Set upper bound.", cmd_rail_setUpperBound),
    SHELL_CMD(wait_before, NULL, "Set wait before ms.", cmd_rail_setWaitBefore),
    SHELL_CMD(wait_after, NULL, "Set wait after ms.", cmd_rail_setWaitAfter),
    SHELL_CMD(s, NULL, "Start stacking with step size.",
              cmd_rail_startStackWithStepSize),
    SHELL_CMD(stack, NULL, "Start stacking with step size.",
              cmd_rail_startStackWithStepSize),
    SHELL_CMD(stack_nm, NULL, "Start stacking with step size (nm).",
              cmd_rail_startStackWithStepSize),
    SHELL_CMD(stack_count, NULL, "Start stacking with length.",
              cmd_rail_startStackWithLength),
    SHELL_CMD(status, NULL, "Get current status.", cmd_rail_status),
    SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(rail, &sub_rail, "rail commands", NULL);
SHELL_CMD_REGISTER(r, &sub_rail, "rail commands", NULL);

static int cmd_cam_shoot(const struct shell *sh, size_t argc, char **argv) {
  event_pub(EVENT_SHOOT);
  return 0;
}

static int cmd_cam_record(const struct shell *sh, size_t argc, char **argv) {
  event_pub(EVENT_RECORD);
  return 0;
}

static int cmd_cam_pair(const struct shell *sh, size_t argc, char **argv) {
  event_pub(EVENT_PAIR_CAMERA);
  return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_cam, SHELL_CMD(shoot, NULL, "Trigger camera shoot.", cmd_cam_shoot),
    SHELL_CMD(record, NULL, "Toggle camera recording.", cmd_cam_record),
    SHELL_CMD(pair, NULL, "Pair camera", cmd_cam_pair), SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(cam, &sub_cam, "cam commands", cmd_cam_pair);
SHELL_CMD_REGISTER(c, &sub_cam, "cam commands", cmd_cam_pair);
#endif