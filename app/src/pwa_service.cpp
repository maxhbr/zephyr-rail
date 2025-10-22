#include "pwa_service.h"
#include <cstdlib>
#include <cstring>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>

#include "StateMachine.h"

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

  // Parse command with parameters
  if (strcmp(token, "GO") == 0) {
    token = strtok_r(nullptr, " ", &saveptr);
    if (!token) {
      LOG_WRN("→ Command: GO (missing distance)");
      notifyStatus("ERR:GO_MISSING_DISTANCE");
      return len;
    }
    int distance = atoi(token);
    LOG_INF("→ Command: GO distance=%d", distance);
    snprintf(response, sizeof(response), "ACK:GO %d", distance);

  } else if (strcmp(token, "GO_TO") == 0) {
    token = strtok_r(nullptr, " ", &saveptr);
    if (!token) {
      LOG_WRN("→ Command: GO_TO (missing position)");
      notifyStatus("ERR:GO_TO_MISSING_POSITION");
      return len;
    }
    int position = atoi(token);
    LOG_INF("→ Command: GO_TO position=%d", position);
    snprintf(response, sizeof(response), "ACK:GO_TO %d", position);

  } else if (strcmp(token, "SET_LOWER_BOUND") == 0) {
    token = strtok_r(nullptr, " ", &saveptr);
    if (!token) {
      LOG_WRN("→ Command: SET_LOWER_BOUND (missing position)");
      notifyStatus("ERR:SET_LOWER_BOUND_MISSING_POSITION");
      return len;
    }
    int position = atoi(token);
    LOG_INF("→ Command: SET_LOWER_BOUND position=%d", position);
    snprintf(response, sizeof(response), "ACK:SET_LOWER_BOUND %d", position);

  } else if (strcmp(token, "SET_UPPER_BOUND") == 0) {
    token = strtok_r(nullptr, " ", &saveptr);
    if (!token) {
      LOG_WRN("→ Command: SET_UPPER_BOUND (missing position)");
      notifyStatus("ERR:SET_UPPER_BOUND_MISSING_POSITION");
      return len;
    }
    int position = atoi(token);
    LOG_INF("→ Command: SET_UPPER_BOUND position=%d", position);
    snprintf(response, sizeof(response), "ACK:SET_UPPER_BOUND %d", position);

  } else if (strcmp(token, "START_STACK") == 0) {
    token = strtok_r(nullptr, " ", &saveptr);
    int expected_length = token ? atoi(token) : 100;
    LOG_INF("→ Command: START_STACK length=%d", expected_length);
    snprintf(response, sizeof(response), "ACK:START_STACK %d", expected_length);

  } else if (strcmp(token, "SHOOT") == 0) {
    LOG_INF("→ Command: SHOOT");
    snprintf(response, sizeof(response), "ACK:SHOOT");

  } else {
    LOG_WRN("→ Unknown command: %s", token);
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
}

void PwaService::onDisconnected(struct bt_conn *conn, uint8_t reason) {
  ARG_UNUSED(conn);
  LOG_INF("PWA disconnected: reason 0x%02x", reason);

  if (pwa_conn_) {
    bt_conn_unref(pwa_conn_);
    pwa_conn_ = nullptr;
  }
  notify_enabled_ = false;
}

int PwaService::startAdvertising() {
  const struct bt_data ad[] = {
      BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
  };

  const struct bt_data sd[] = {
      BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
              sizeof(CONFIG_BT_DEVICE_NAME) - 1),
  };

  int err =
      bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
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
