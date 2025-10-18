#include "pwa_service.h"
#include "sony_remote/sony_remote.h"
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
SonyRemote *PwaService::remote_ = nullptr;
struct bt_conn *PwaService::pwa_conn_ = nullptr;
bool PwaService::notify_enabled_ = false;
uint8_t PwaService::status_buffer_[256] = {0};

// Static method implementations (must be before GATT service definition)
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
  char cmd[64] = {0};
  size_t cmd_len = len < sizeof(cmd) - 1 ? len : sizeof(cmd) - 1;
  memcpy(cmd, buf, cmd_len);
  cmd[cmd_len] = '\0';

  LOG_INF("PWA Command: %s", cmd);

  // Handle commands
  if (strcmp(cmd, "PING") == 0) {
    notifyStatus("PONG");
    return len;
  }

  if (strcmp(cmd, "STATUS") == 0) {
    if (remote_ && remote_->ready()) {
      notifyStatus("CAMERA_READY");
    } else {
      notifyStatus("CAMERA_NOT_READY");
    }
    return len;
  }

  // Camera control commands
  if (!remote_) {
    notifyStatus("ERR:NO_CAMERA_INSTANCE");
    return len;
  }

  if (!remote_->ready()) {
    notifyStatus("ERR:CAMERA_NOT_READY");
    return len;
  }

  // Sony camera commands
  if (strcmp(cmd, "SHOOT") == 0) {
    remote_->shoot();
    notifyStatus("ACK:SHOOT");
  } else if (strcmp(cmd, "FOCUS_DOWN") == 0) {
    remote_->focusDown();
    notifyStatus("ACK:FOCUS_DOWN");
  } else if (strcmp(cmd, "FOCUS_UP") == 0) {
    remote_->focusUp();
    notifyStatus("ACK:FOCUS_UP");
  } else if (strcmp(cmd, "SHUTTER_DOWN") == 0) {
    remote_->shutterDown();
    notifyStatus("ACK:SHUTTER_DOWN");
  } else if (strcmp(cmd, "SHUTTER_UP") == 0) {
    remote_->shutterUp();
    notifyStatus("ACK:SHUTTER_UP");
  } else if (strcmp(cmd, "C1_DOWN") == 0) {
    remote_->cOneDown();
    notifyStatus("ACK:C1_DOWN");
  } else if (strcmp(cmd, "C1_UP") == 0) {
    remote_->cOneUp();
    notifyStatus("ACK:C1_UP");
  } else if (strcmp(cmd, "REC_TOGGLE") == 0) {
    remote_->recToggle();
    notifyStatus("ACK:REC_TOGGLE");
  } else if (strcmp(cmd, "ZOOM_T_PRESS") == 0) {
    remote_->zoomTPress();
    notifyStatus("ACK:ZOOM_T_PRESS");
  } else if (strcmp(cmd, "ZOOM_T_RELEASE") == 0) {
    remote_->zoomTRelease();
    notifyStatus("ACK:ZOOM_T_RELEASE");
  } else if (strcmp(cmd, "ZOOM_W_PRESS") == 0) {
    remote_->zoomWPress();
    notifyStatus("ACK:ZOOM_W_PRESS");
  } else if (strcmp(cmd, "ZOOM_W_RELEASE") == 0) {
    remote_->zoomWRelease();
    notifyStatus("ACK:ZOOM_W_RELEASE");
  } else {
    LOG_WRN("Unknown command: %s", cmd);
    notifyStatus("ERR:UNKNOWN_CMD");
  }

  return len;
}

// C-style wrapper functions for GATT callbacks (required by Zephyr macros)
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

void PwaService::init(SonyRemote *remote) {
  remote_ = remote;
  strcpy((char *)status_buffer_, "INIT");
  LOG_INF("PWA Service initialized");
}

const struct bt_gatt_attr *PwaService::findStatusAttr() {
  // Find the status characteristic attribute in the service
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

  // Request connection parameter update for better performance
  struct bt_le_conn_param param = {
      .interval_min = 24, // 30ms (24 * 1.25ms)
      .interval_max = 40, // 50ms (40 * 1.25ms)
      .latency = 0,
      .timeout = 400 // 4s (400 * 10ms)
  };
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
      // Include the service UUID in advertising data
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
