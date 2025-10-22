#include "pwa_service.h"
#include "StateMachine.h"
#include <cstring>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwa_service, LOG_LEVEL_INF);

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

  if (notify_enabled_) {
    notifyStatus("READY");
  }
}

ssize_t PwaService::cmdWrite(struct bt_conn *conn,
                             const struct bt_gatt_attr *attr, const void *buf,
                             uint16_t len, uint16_t offset, uint8_t flags) {
  ARG_UNUSED(attr);
  ARG_UNUSED(offset);
  ARG_UNUSED(flags);

  // Require encryption for writes
  if (bt_conn_get_security(conn) < BT_SECURITY_L2) {
    LOG_WRN("Rejecting write: insufficient security");
    return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_ENCRYPTION);
  }

  // Parse command
  char cmd[128] = {0};
  size_t cmd_len = len < sizeof(cmd) - 1 ? len : sizeof(cmd) - 1;
  memcpy(cmd, buf, cmd_len);
  cmd[cmd_len] = '\0';

  LOG_INF("PWA Command: %s", cmd);

  // Parse command and arguments
  char *saveptr = nullptr;
  char *token = strtok_r(cmd, " ", &saveptr);
  if (!token) {
    notifyStatus("ERR:EMPTY_COMMAND");
    return len;
  }

  // Special commands
  if (strcmp(token, "PING") == 0) {
    notifyStatus("PONG");
    return len;
  }

  if (strcmp(token, "STATUS") == 0) {
    notifyStatus("ACK:STATUS");
    return len;
  }

  // Rail control commands - map to StateMachine events
  char response[128];

  if (strcmp(token, "NOOP") == 0) {
    event_pub(EVENT_NOOP);
    snprintf(response, sizeof(response), "ACK:NOOP");

  } else if (strcmp(token, "GO") == 0) {
    // Command format: "GO <distance>"
    token = strtok_r(nullptr, " ", &saveptr);
    if (!token) {
      notifyStatus("ERR:GO_MISSING_DISTANCE");
      return len;
    }
    int distance = atoi(token);
    event_pub(EVENT_GO, distance);
    snprintf(response, sizeof(response), "ACK:GO %d", distance);

  } else if (strcmp(token, "GO_TO") == 0) {
    // Command format: "GO_TO <position>"
    token = strtok_r(nullptr, " ", &saveptr);
    if (!token) {
      notifyStatus("ERR:GO_TO_MISSING_POSITION");
      return len;
    }
    int position = atoi(token);
    event_pub(EVENT_GO_TO, position);
    snprintf(response, sizeof(response), "ACK:GO_TO %d", position);

  } else if (strcmp(token, "SET_LOWER_BOUND") == 0) {
    token = strtok_r(nullptr, " ", &saveptr);
    if (token) {
      int position = atoi(token);
      event_pub(EVENT_SET_LOWER_BOUND_TO, position);
      snprintf(response, sizeof(response), "ACK:SET_LOWER_BOUND %d", position);
    } else {
      event_pub(EVENT_SET_LOWER_BOUND);
      snprintf(response, sizeof(response), "ACK:SET_LOWER_BOUND");
    }

  } else if (strcmp(token, "SET_UPPER_BOUND") == 0) {
    token = strtok_r(nullptr, " ", &saveptr);
    if (token) {
      int position = atoi(token);
      event_pub(EVENT_SET_UPPER_BOUND_TO, position);
      snprintf(response, sizeof(response), "ACK:SET_UPPER_BOUND %d", position);
    } else {
      event_pub(EVENT_SET_UPPER_BOUND);
      snprintf(response, sizeof(response), "ACK:SET_UPPER_BOUND");
    }

  } else if (strcmp(token, "SET_WAIT_BEFORE") == 0) {
    // Command format: "SET_WAIT_BEFORE <milliseconds>"
    token = strtok_r(nullptr, " ", &saveptr);
    if (!token) {
      notifyStatus("ERR:SET_WAIT_BEFORE_MISSING_VALUE");
      return len;
    }
    int wait_ms = atoi(token);
    event_pub(EVENT_SET_WAIT_BEFORE_MS, wait_ms);
    snprintf(response, sizeof(response), "ACK:SET_WAIT_BEFORE %d", wait_ms);

  } else if (strcmp(token, "SET_WAIT_AFTER") == 0) {
    // Command format: "SET_WAIT_AFTER <milliseconds>"
    token = strtok_r(nullptr, " ", &saveptr);
    if (!token) {
      notifyStatus("ERR:SET_WAIT_AFTER_MISSING_VALUE");
      return len;
    }
    int wait_ms = atoi(token);
    event_pub(EVENT_SET_WAIT_AFTER_MS, wait_ms);
    snprintf(response, sizeof(response), "ACK:SET_WAIT_AFTER %d", wait_ms);

  } else if (strcmp(token, "START_STACK") == 0) {
    // Command format: "START_STACK <expected_length>"
    // or "START_STACK <expected_length> <lower> <upper>"
    token = strtok_r(nullptr, " ", &saveptr);
    int expected_length = token ? atoi(token) : 100;

    char *lower_str = strtok_r(nullptr, " ", &saveptr);
    char *upper_str = strtok_r(nullptr, " ", &saveptr);

    if (lower_str && upper_str) {
      int lower = atoi(lower_str);
      int upper = atoi(upper_str);
      event_pub(EVENT_SET_LOWER_BOUND_TO, lower);
      event_pub(EVENT_SET_UPPER_BOUND_TO, upper);
    }

    event_pub(EVENT_START_STACK, expected_length);
    snprintf(response, sizeof(response), "ACK:START_STACK %d", expected_length);

  } else if (strcmp(token, "SHOOT") == 0) {
    event_pub(EVENT_SHOOT);
    snprintf(response, sizeof(response), "ACK:SHOOT");

  } else {
    snprintf(response, sizeof(response), "ERR:UNKNOWN_CMD:%s", token);
  }

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

void PwaService::init() {
  strcpy((char *)status_buffer_, "INIT");
  LOG_INF("PWA Service initialized");
}

const struct bt_gatt_attr *PwaService::findStatusAttr() {
  const struct bt_gatt_attr *attrs = pwa_gatt_svc.attrs;
  size_t attr_count = pwa_gatt_svc.attr_count;

  for (size_t i = 0; i < attr_count; i++) {
    if (attrs[i].uuid && bt_uuid_cmp(attrs[i].uuid, PWA_STATUS_UUID) == 0) {
      return &attrs[i];
    }
  }
  return nullptr;
}

void PwaService::notifyStatus(const char *status) {
  if (!notify_enabled_ || !pwa_conn_) {
    LOG_DBG("Cannot notify: %s (enabled=%d, conn=%p)", status, notify_enabled_,
            pwa_conn_);
    return;
  }

  const struct bt_gatt_attr *attr = findStatusAttr();
  if (!attr) {
    LOG_ERR("Status attribute not found");
    return;
  }

  size_t len = strlen(status);
  if (len > sizeof(status_buffer_) - 1) {
    len = sizeof(status_buffer_) - 1;
  }

  memcpy(status_buffer_, status, len);
  status_buffer_[len] = '\0';

  int err = bt_gatt_notify(pwa_conn_, attr, status_buffer_, len);
  if (err) {
    LOG_ERR("Notify failed: %d", err);
  } else {
    LOG_DBG("Notified: %s", status);
  }
}

bool PwaService::isConnected() { return pwa_conn_ != nullptr; }

void PwaService::onConnected(struct bt_conn *conn, uint8_t err) {
  if (err) {
    LOG_ERR("PWA connection failed: %u", err);
    return;
  }

  char addr[BT_ADDR_LE_STR_LEN];
  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
  LOG_INF("PWA connected: %s", addr);

  pwa_conn_ = bt_conn_ref(conn);

  // Request connection parameter update
  struct bt_le_conn_param param = {
      .interval_min = 24, .interval_max = 40, .latency = 0, .timeout = 400};
  bt_conn_le_param_update(conn, &param);
}

void PwaService::onDisconnected(struct bt_conn *conn, uint8_t reason) {
  ARG_UNUSED(conn);
  LOG_INF("PWA disconnected: reason 0x%02x", reason);

  if (pwa_conn_) {
    bt_conn_unref(pwa_conn_);
    pwa_conn_ = nullptr;
  }
  notify_enabled_ = false;

  // Restart advertising
  startAdvertising();
}

int PwaService::startAdvertising() {
  static const struct bt_data ad[] = {
      BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
      BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
              sizeof(CONFIG_BT_DEVICE_NAME) - 1),
      BT_DATA_BYTES(BT_DATA_UUID128_ALL, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
                    0x34, 0x12, 0x34, 0x12, 0x78, 0x56, 0x34, 0x56, 0x34, 0x12),
  };

  struct bt_le_adv_param adv_param_init = {
      .id = BT_ID_DEFAULT,
      .sid = 0,
      .secondary_max_skip = 0,
      .options = BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_USE_IDENTITY,
      .interval_min = BT_GAP_ADV_FAST_INT_MIN_1,
      .interval_max = BT_GAP_ADV_FAST_INT_MAX_1,
      .peer = NULL,
  };

  int err = bt_le_adv_start(&adv_param_init, ad, ARRAY_SIZE(ad), NULL, 0);
  if (err) {
    LOG_ERR("Advertising start failed: %d", err);
    return err;
  }

  LOG_INF("Advertising started as '%s'", CONFIG_BT_DEVICE_NAME);
  return 0;
}

int PwaService::stopAdvertising() {
  int err = bt_le_adv_stop();
  if (err) {
    LOG_ERR("Advertising stop failed: %d", err);
    return err;
  }

  LOG_INF("Advertising stopped");
  return 0;
}
