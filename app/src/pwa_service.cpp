#include "pwa_service.h"
#include <cstdlib>
#include <cstring>
#include <strings.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>

#include "StateMachine.h"

LOG_MODULE_REGISTER(pwa_service, LOG_LEVEL_INF);

namespace {

int parseSpeedPreset(const char *token) {
  if (!token) {
    return 0;
  }
  if (strcasecmp(token, "slow") == 0 || strcmp(token, "1") == 0) {
    return 1;
  }
  if (strcasecmp(token, "medium") == 0 || strcmp(token, "2") == 0) {
    return 2;
  }
  if (strcasecmp(token, "fast") == 0 || strcmp(token, "3") == 0) {
    return 3;
  }
  return 0;
}

bool handleRailCommand(const char *subcmd, char **saveptr) {
  if (!subcmd) {
    PwaService::notifyStatus("ERR:RAIL_MISSING_COMMAND");
    return true;
  }

  char *param1 = nullptr;
  char *param2 = nullptr;
  char *param3 = nullptr;
  char response[128];

  if (strcasecmp(subcmd, "go") == 0 || strcasecmp(subcmd, "g") == 0 ||
      strcasecmp(subcmd, "go_nm") == 0) {
    param1 = strtok_r(nullptr, " ", saveptr);
    if (!param1) {
      LOG_WRN("→ rail %s (missing distance)", subcmd);
      PwaService::notifyStatus("ERR:RAIL_GO_MISSING_DISTANCE");
      return true;
    }
    bool is_nm_command = (strcasecmp(subcmd, "go_nm") == 0);
    int multiplier = is_nm_command ? 1 : 1000;
    int distance_nm = atoi(param1) * multiplier;
    LOG_INF("→ Command: rail %s distance=%dnm", subcmd, distance_nm);
    event_pub(EVENT_GO, distance_nm);
    snprintf(response, sizeof(response), "ACK:rail %s %d", subcmd, distance_nm);
    PwaService::notifyStatus(response);
    return true;
  }

  if (strcasecmp(subcmd, "go_to") == 0) {
    param1 = strtok_r(nullptr, " ", saveptr);
    if (!param1) {
      LOG_WRN("→ rail go_to (missing position)");
      PwaService::notifyStatus("ERR:RAIL_GO_TO_MISSING_POSITION");
      return true;
    }
    int position_nm = atoi(param1) * 1000;
    LOG_INF("→ Command: rail go_to position=%dnm", position_nm);
    event_pub(EVENT_GO_TO, position_nm);
    snprintf(response, sizeof(response), "ACK:rail go_to %d", position_nm);
    PwaService::notifyStatus(response);
    return true;
  }

  if (strcasecmp(subcmd, "go_pct") == 0 || strcasecmp(subcmd, "p") == 0) {
    param1 = strtok_r(nullptr, " ", saveptr);
    if (!param1) {
      LOG_WRN("→ rail go_pct (missing percentage)");
      PwaService::notifyStatus("ERR:RAIL_GO_PCT_MISSING_PERCENTAGE");
      return true;
    }
    int percentage = atoi(param1);
    if (percentage < 0 || percentage > 100) {
      LOG_WRN("→ rail go_pct invalid percentage=%d", percentage);
      PwaService::notifyStatus("ERR:RAIL_GO_PCT_INVALID_RANGE");
      return true;
    }
    LOG_INF("→ Command: rail go_pct percentage=%d", percentage);
    event_pub(EVENT_GO_PCT, percentage);
    snprintf(response, sizeof(response), "ACK:rail go_pct %d", percentage);
    PwaService::notifyStatus(response);
    return true;
  }

  if (strcasecmp(subcmd, "lower") == 0) {
    param1 = strtok_r(nullptr, " ", saveptr);
    if (!param1) {
      LOG_INF("→ Command: rail lower (current position)");
      event_pub(EVENT_SET_LOWER_BOUND);
      PwaService::notifyStatus("ACK:rail lower");
    } else {
      int position_nm = atoi(param1) * 1000;
      LOG_INF("→ Command: rail lower position=%dnm", position_nm);
      event_pub(EVENT_SET_LOWER_BOUND_TO, position_nm);
      snprintf(response, sizeof(response), "ACK:rail lower %d", position_nm);
      PwaService::notifyStatus(response);
    }
    return true;
  }

  if (strcasecmp(subcmd, "upper") == 0) {
    param1 = strtok_r(nullptr, " ", saveptr);
    if (!param1) {
      LOG_INF("→ Command: rail upper (current position)");
      event_pub(EVENT_SET_UPPER_BOUND);
      PwaService::notifyStatus("ACK:rail upper");
    } else {
      int position_nm = atoi(param1) * 1000;
      LOG_INF("→ Command: rail upper position=%dnm", position_nm);
      event_pub(EVENT_SET_UPPER_BOUND_TO, position_nm);
      snprintf(response, sizeof(response), "ACK:rail upper %d", position_nm);
      PwaService::notifyStatus(response);
    }
    return true;
  }

  if (strcasecmp(subcmd, "wait_before") == 0) {
    param1 = strtok_r(nullptr, " ", saveptr);
    if (!param1) {
      LOG_WRN("→ rail wait_before (missing milliseconds)");
      PwaService::notifyStatus("ERR:RAIL_WAIT_BEFORE_MISSING_MS");
      return true;
    }
    int wait_ms = atoi(param1);
    LOG_INF("→ Command: rail wait_before ms=%d", wait_ms);
    event_pub(EVENT_SET_WAIT_BEFORE_MS, wait_ms);
    snprintf(response, sizeof(response), "ACK:rail wait_before %d", wait_ms);
    PwaService::notifyStatus(response);
    return true;
  }

  if (strcasecmp(subcmd, "wait_after") == 0) {
    param1 = strtok_r(nullptr, " ", saveptr);
    if (!param1) {
      LOG_WRN("→ rail wait_after (missing milliseconds)");
      PwaService::notifyStatus("ERR:RAIL_WAIT_AFTER_MISSING_MS");
      return true;
    }
    int wait_ms = atoi(param1);
    LOG_INF("→ Command: rail wait_after ms=%d", wait_ms);
    event_pub(EVENT_SET_WAIT_AFTER_MS, wait_ms);
    snprintf(response, sizeof(response), "ACK:rail wait_after %d", wait_ms);
    PwaService::notifyStatus(response);
    return true;
  }

  if (strcasecmp(subcmd, "set_speed") == 0) {
    param1 = strtok_r(nullptr, " ", saveptr);
    int speed_value = parseSpeedPreset(param1);
    if (speed_value == 0) {
      LOG_WRN("→ rail set_speed (unknown preset %s)",
              param1 ? param1 : "<missing>");
      PwaService::notifyStatus("ERR:RAIL_SET_SPEED_UNKNOWN_PRESET");
      return true;
    }
    LOG_INF("→ Command: rail set_speed preset=%d", speed_value);
    event_pub(EVENT_SET_SPEED, speed_value);
    snprintf(response, sizeof(response), "ACK:rail set_speed %d", speed_value);
    PwaService::notifyStatus(response);
    return true;
  }

  if (strcasecmp(subcmd, "set_rpm") == 0) {
    param1 = strtok_r(nullptr, " ", saveptr);
    if (!param1) {
      LOG_WRN("→ rail set_rpm (missing rpm)");
      PwaService::notifyStatus("ERR:RAIL_SET_RPM_MISSING_VALUE");
      return true;
    }
    int rpm = atoi(param1);
    if (rpm < 1) {
      LOG_WRN("→ rail set_rpm invalid rpm=%d", rpm);
      PwaService::notifyStatus("ERR:RAIL_SET_RPM_INVALID");
      return true;
    }
    LOG_INF("→ Command: rail set_rpm rpm=%d", rpm);
    event_pub(EVENT_SET_SPEED_RPM, rpm);
    snprintf(response, sizeof(response), "ACK:rail set_rpm %d", rpm);
    PwaService::notifyStatus(response);
    return true;
  }

  if (strcasecmp(subcmd, "stack") == 0 || strcasecmp(subcmd, "s") == 0 ||
      strcasecmp(subcmd, "stack_nm") == 0) {
    bool is_nm_command = (strcasecmp(subcmd, "stack_nm") == 0);
    int multiplier = is_nm_command ? 1 : 1000;

    param1 = strtok_r(nullptr, " ", saveptr);

    if (param1) {
      int expected_step_size_nm = atoi(param1) * multiplier;
      LOG_INF("→ Command: rail %s step_size=%dnm", subcmd,
              expected_step_size_nm);
      event_pub(EVENT_START_STACK_WITH_STEP_SIZE, expected_step_size_nm);
      snprintf(response, sizeof(response), "ACK:rail %s %d", subcmd,
               expected_step_size_nm);
    } else {
      LOG_INF("→ Command: rail %s step_size=1000nm (default)", subcmd);
      event_pub(EVENT_START_STACK_WITH_STEP_SIZE, 1000);
      snprintf(response, sizeof(response), "ACK:rail %s 1000", subcmd);
    }
    PwaService::notifyStatus(response);
    return true;
  }

  if (strcasecmp(subcmd, "stack_count") == 0) {
    param1 = strtok_r(nullptr, " ", saveptr);

    if (param1) {
      int expected_length = atoi(param1);
      LOG_INF("→ Command: rail stack_count length=%d", expected_length);
      event_pub(EVENT_START_STACK_WITH_LENGTH, expected_length);
      snprintf(response, sizeof(response), "ACK:rail stack_count %d",
               expected_length);
    } else {
      LOG_INF("→ Command: rail stack_count length=100 (default)");
      event_pub(EVENT_START_STACK_WITH_LENGTH, 100);
      snprintf(response, sizeof(response), "ACK:rail stack_count 100");
    }
    PwaService::notifyStatus(response);
    return true;
  }

  if (strcasecmp(subcmd, "status") == 0) {
    LOG_INF("→ Command: rail status");
    event_pub(EVENT_STATUS);
    PwaService::notifyStatus("ACK:rail status");
    return true;
  }

  return false;
}

bool handleCamCommand(const char *subcmd, char **saveptr) {
  ARG_UNUSED(saveptr);
  if (!subcmd) {
    PwaService::notifyStatus("ERR:CAM_MISSING_COMMAND");
    return true;
  }

  if (strcasecmp(subcmd, "shoot") == 0) {
    LOG_INF("→ Command: cam shoot");
    event_pub(EVENT_SHOOT);
    PwaService::notifyStatus("ACK:cam shoot");
    return true;
  }

  if (strcasecmp(subcmd, "record") == 0) {
    LOG_INF("→ Command: cam record");
    event_pub(EVENT_RECORD);
    PwaService::notifyStatus("ACK:cam record");
    return true;
  }

  return false;
}
} // namespace

// Custom UUIDs for PWA service (128-bit)
// Service UUID: 12345634-5678-1234-1234-123456789abc
#define PWA_SERVICE_UUID                                                       \
  BT_UUID_DECLARE_128(                                                         \
      BT_UUID_128_ENCODE(0x12345634, 0x5678, 0x1234, 0x1234, 0x123456789abc))

// Command characteristic UUID: 12345635-5678-1234-1234-123456789abc
#define PWA_CMD_UUID                                                           \
  BT_UUID_DECLARE_128(                                                         \
      BT_UUID_128_ENCODE(0x12345635, 0x5678, 0x1234, 0x1234, 0x123456789abc))

// Status characteristic UUID: 12345636-5678-1234-1234-123456789abc
#define PWA_STATUS_UUID                                                        \
  BT_UUID_DECLARE_128(                                                         \
      BT_UUID_128_ENCODE(0x12345636, 0x5678, 0x1234, 0x1234, 0x123456789abc))

// Static member initialization
struct bt_conn *PwaService::pwa_conn_ = nullptr;
bool PwaService::notify_enabled_ = false;
uint8_t PwaService::status_buffer_[256] = {0};

// Static method implementations
void PwaService::cccChanged(const struct bt_gatt_attr *attr, uint16_t value) {
  notify_enabled_ = (value == BT_GATT_CCC_NOTIFY);
  LOG_INF("PWA notifications %s", notify_enabled_ ? "enabled" : "disabled");
}

ssize_t PwaService::cmdWrite(struct bt_conn *conn,
                             const struct bt_gatt_attr *attr, const void *buf,
                             uint16_t len, uint16_t offset, uint8_t flags) {
  ARG_UNUSED(attr);
  ARG_UNUSED(offset);
  ARG_UNUSED(flags);

  if (len == 0 || len >= 256) {
    LOG_WRN("Invalid command length: %u", len);
    notifyStatus("ERR:INVALID_LENGTH");
    return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
  }

  char cmd[256];
  memcpy(cmd, buf, len);
  cmd[len] = '\0';

  LOG_DBG("PWA Command Received: '%s'", cmd);

  char response[128];
  char *saveptr;
  char *token = strtok_r(cmd, " ", &saveptr);

  if (!token) {
    LOG_WRN("Empty command received");
    notifyStatus("ERR:EMPTY_COMMAND");
    return len;
  }

  if (strcasecmp(token, "rail") == 0) {
    const char *subcmd = strtok_r(nullptr, " ", &saveptr);
    if (handleRailCommand(subcmd, &saveptr)) {
      return len;
    }
  } else if (strcasecmp(token, "cam") == 0) {
    const char *subcmd = strtok_r(nullptr, " ", &saveptr);
    if (handleCamCommand(subcmd, &saveptr)) {
      return len;
    }
  }

  LOG_WRN("→ Unknown command: %s", token);
  snprintf(response, sizeof(response), "ERR:UNKNOWN_CMD:%s", token);
  notifyStatus(response);
  return len;
}

// C-style wrapper functions for GATT callbacks
static void pwa_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value) {
  PwaService::cccChanged(attr, value);
}

static ssize_t pwa_cmd_write(struct bt_conn *conn,
                             const struct bt_gatt_attr *attr, const void *buf,
                             uint16_t len, uint16_t offset, uint8_t flags) {
  return PwaService::cmdWrite(conn, attr, buf, len, offset, flags);
}

// GATT service definition
BT_GATT_SERVICE_DEFINE(
    pwa_gatt_svc, BT_GATT_PRIMARY_SERVICE(PWA_SERVICE_UUID),

    // Status characteristic: Read + Notify
    BT_GATT_CHARACTERISTIC(PWA_STATUS_UUID,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ_ENCRYPT, bt_gatt_attr_read, NULL,
                           PwaService::status_buffer_),

    // Client Characteristic Configuration Descriptor
    BT_GATT_CCC(pwa_ccc_changed,
                BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),

    // Command characteristic: Write + Write Without Response
    BT_GATT_CHARACTERISTIC(PWA_CMD_UUID,
                           BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_WRITE_ENCRYPT, NULL, pwa_cmd_write,
                           NULL));

const struct bt_gatt_attr *PwaService::findStatusAttr() {
  const struct bt_gatt_attr *svc = bt_gatt_attr_next(nullptr);
  while (svc) {
    if (bt_uuid_cmp(svc->uuid, PWA_STATUS_UUID) == 0) {
      return svc;
    }
    svc = bt_gatt_attr_next(svc);
  }
  return nullptr;
}

void PwaService::notifyStatus(const char *status) {
  if (!notify_enabled_ || !pwa_conn_) {
    LOG_DBG("Cannot notify: %s (enabled=%d, conn=%p)", status, notify_enabled_,
            pwa_conn_);
    return;
  }

  size_t len = strlen(status);
  if (len >= sizeof(status_buffer_)) {
    len = sizeof(status_buffer_) - 1;
  }

  memcpy(status_buffer_, status, len);
  status_buffer_[len] = '\0';

  const struct bt_gatt_attr *attr = findStatusAttr();
  if (!attr) {
    LOG_ERR("Status characteristic not found");
    return;
  }

  int err = bt_gatt_notify(pwa_conn_, attr, status_buffer_, len);
  if (err) {
    LOG_ERR("Notify failed: %d", err);
  } else {
    LOG_DBG("Notified: %s", status);
  }
}

bool PwaService::isConnected() { return pwa_conn_ != nullptr; }

void PwaService::onConnected(struct bt_conn *conn, uint8_t err) {
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role != BT_CONN_ROLE_PERIPHERAL) {
    // Not a PWA connection (we are not peripheral/server), ignore
    return;
  }

  if (err) {
    LOG_ERR("PWA connection failed: %u", err);
    return;
  }

  char addr[BT_ADDR_LE_STR_LEN];
  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
  LOG_INF("PWA connected: %s", addr);

  pwa_conn_ = bt_conn_ref(conn);

  struct bt_le_conn_param param = {
      .interval_min = 24, .interval_max = 40, .latency = 0, .timeout = 400};
  bt_conn_le_param_update(conn, &param);

  stopAdvertising();
}

void PwaService::onDisconnected(struct bt_conn *conn, uint8_t reason) {
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role != BT_CONN_ROLE_PERIPHERAL) {
    // Not a PWA connection (we are not peripheral/server), ignore
    return;
  }

  LOG_INF("PWA disconnected: reason 0x%02x", reason);

  if (pwa_conn_) {
    bt_conn_unref(pwa_conn_);
    pwa_conn_ = nullptr;
  }
  notify_enabled_ = false;

  LOG_INF("Restarting advertising after disconnect...");
  if (int err = startAdvertising(); err) {
    LOG_ERR("Failed to restart advertising: %d", err);
  }
}

int PwaService::startAdvertising() {
  const struct bt_data ad[] = {
      BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
  };

  const struct bt_data sd[] = {
      BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
              sizeof(CONFIG_BT_DEVICE_NAME) - 1),
  };

  int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd,
                            ARRAY_SIZE(sd));
  if (err) {
    LOG_ERR("Advertising failed to start (err %d)", err);
    return err;
  }

  LOG_INF("PWA advertising started - device name: %s", CONFIG_BT_DEVICE_NAME);
  return 0;
}

int PwaService::stopAdvertising() {
  int err = bt_le_adv_stop();
  if (err) {
    LOG_ERR("Advertising failed to stop (err %d)", err);
    return err;
  }

  LOG_INF("PWA advertising stopped");
  return 0;
}
