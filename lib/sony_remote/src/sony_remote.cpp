// mostly written by GPT5, based on
// https://gregleeds.com/reverse-engineering-sony-camera-bluetooth/
#include "sony_remote/sony_remote.h"
#include <cstring>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(sony_remote, LOG_LEVEL_DBG);

SonyRemote *SonyRemote::self_ = nullptr;

namespace {
// Sony command payloads (write to FF01, Write Without Response)
constexpr uint8_t FOCUS_DOWN[] = {0x01, 0x07};
constexpr uint8_t FOCUS_UP[] = {0x01, 0x06};
constexpr uint8_t SHUTTER_DOWN[] = {0x01, 0x09};
constexpr uint8_t SHUTTER_UP[] = {0x01, 0x08};
constexpr uint8_t C1_DOWN[] = {0x01, 0x21};
constexpr uint8_t C1_UP[] = {0x01, 0x20};
constexpr uint8_t REC_TOGGLE[] = {0x01, 0x0E};
constexpr uint8_t ZOOM_T_PRESS[] = {0x02, 0x45, 0x10};
constexpr uint8_t ZOOM_T_RELEASE[] = {0x02, 0x44, 0x00};
constexpr uint8_t ZOOM_W_PRESS[] = {0x02, 0x47, 0x10};
constexpr uint8_t ZOOM_W_RELEASE[] = {0x02, 0x46, 0x00};

// Little-endian bytes for 8000FF00-FF00-FFFF-FFFF-FFFFFFFFFFFF
static bt_uuid_128 kSonySvcUuid =
    BT_UUID_INIT_128(0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                     0x00, 0xff, 0x00, 0xff, 0x00, 0x80);

static bt_uuid_16 kFF01Uuid = BT_UUID_INIT_16(0xFF01);

static bt_conn_cb kConnCbs = {
    .connected = SonyRemote::on_connected,
    .disconnected = SonyRemote::on_disconnected,
};

static bool accept_any(bt_data *data, void * /*user*/) {
  // Look for Sony devices by checking for Sony manufacturer data or device name
  if (data->type == BT_DATA_MANUFACTURER_DATA && data->data_len >= 2) {
    uint16_t company_id = sys_get_le16(data->data);
    if (company_id == 0x012D) { // Sony's company ID
      return true;
    }
  }

  if (data->type == BT_DATA_NAME_COMPLETE ||
      data->type == BT_DATA_NAME_SHORTENED) {
    // Check if device name contains "DSC", "ILCE", "FX", or other Sony camera
    // prefixes
    const char *name = (const char *)data->data;
    if (data->data_len >= 3) {
      if (strncmp(name, "DSC", 3) == 0 || strncmp(name, "ILCE", 4) == 0 ||
          strncmp(name, "FX", 2) == 0) {
        return true;
      }
    }
  }

  return true; // For now, still accept any device for broader compatibility
}

} // namespace

SonyRemote::SonyRemote() {
  self_ = this;
  k_work_init_delayable(&discovery_work_, discovery_work_handler);
  has_target_addr_ = false;
  is_paired_ = false;
}

SonyRemote::SonyRemote(const char *target_address) {
  self_ = this;
  k_work_init_delayable(&discovery_work_, discovery_work_handler);
  is_paired_ = false;

  // Parse the target address string and set has_target_addr_
  if (target_address) {
    // Try parsing as public address first (most common for cameras)
    if (bt_addr_le_from_str(target_address, "public", &target_addr_) == 0) {
      has_target_addr_ = true;
      LOG_INF("Target camera address set to: %s (public)", target_address);
    } else if (bt_addr_le_from_str(target_address, "random", &target_addr_) ==
               0) {
      has_target_addr_ = true;
      LOG_INF("Target camera address set to: %s (random)", target_address);
    } else {
      has_target_addr_ = false;
      LOG_ERR("Invalid target address format: %s", target_address);
    }
  } else {
    has_target_addr_ = false;
    LOG_ERR("NULL target address provided");
  }
}

void SonyRemote::begin() { bt_conn_cb_register(&kConnCbs); }

void SonyRemote::startScan() {
  static struct bt_le_scan_param scan_param =
      BT_LE_SCAN_PARAM_INIT(BT_LE_SCAN_TYPE_ACTIVE, BT_LE_SCAN_OPT_NONE,
                            BT_GAP_SCAN_FAST_INTERVAL, BT_GAP_SCAN_FAST_WINDOW);
  int err = bt_le_scan_start(&scan_param, SonyRemote::on_scan);
  if (err) {
    LOG_ERR("Scan start failed (%d)", err);
  } else {
    if (self_->has_target_addr_) {
      char addr_str[BT_ADDR_LE_STR_LEN];
      bt_addr_le_to_str(&self_->target_addr_, addr_str, sizeof(addr_str));
      LOG_INF("Scanning for specific Sony camera: %s", addr_str);
    } else {
      LOG_INF("Scanning for any Sony camera...");
    }
  }
}

bool SonyRemote::ready() const {
  if (conn_ != nullptr && ff01_handle_ != 0 && !is_paired_) {
    LOG_DBG("connected but not paired");
  }
  return conn_ != nullptr && ff01_handle_ != 0 && is_paired_;
}

void SonyRemote::focusDown() { send_cmd(FOCUS_DOWN, sizeof(FOCUS_DOWN)); }
void SonyRemote::focusUp() { send_cmd(FOCUS_UP, sizeof(FOCUS_UP)); }
void SonyRemote::shutterDown() { send_cmd(SHUTTER_DOWN, sizeof(SHUTTER_DOWN)); }
void SonyRemote::shutterUp() { send_cmd(SHUTTER_UP, sizeof(SHUTTER_UP)); }
void SonyRemote::cOneDown() { send_cmd(C1_DOWN, sizeof(C1_DOWN)); }
void SonyRemote::cOneUp() { send_cmd(C1_UP, sizeof(C1_UP)); }
void SonyRemote::recToggle() { send_cmd(REC_TOGGLE, sizeof(REC_TOGGLE)); }
void SonyRemote::zoomTPress() { send_cmd(ZOOM_T_PRESS, sizeof(ZOOM_T_PRESS)); }
void SonyRemote::zoomTRelease() {
  send_cmd(ZOOM_T_RELEASE, sizeof(ZOOM_T_RELEASE));
}
void SonyRemote::zoomWPress() { send_cmd(ZOOM_W_PRESS, sizeof(ZOOM_W_PRESS)); }
void SonyRemote::zoomWRelease() {
  send_cmd(ZOOM_W_RELEASE, sizeof(ZOOM_W_RELEASE));
}

void SonyRemote::on_connected(bt_conn *conn, uint8_t err) {
  if (!self_) {
    LOG_ERR("self_ pointer is NULL in on_connected!");
    return;
  }

  if (err) {
    LOG_ERR("Connect failed (%u)", err);
    return;
  }
  LOG_INF("Connected");
  if (self_->conn_)
    bt_conn_unref(self_->conn_);
  self_->conn_ = bt_conn_ref(conn);

  // Start pairing process - this will trigger the security procedure
  LOG_INF("Starting security/pairing...");
  int pairing_err = bt_conn_set_security(conn, BT_SECURITY_L2);
  if (pairing_err) {
    LOG_ERR("Failed to start pairing (%d)", pairing_err);
    // Still try discovery in case device is already bonded
    self_->start_discovery();
  } else {
    LOG_INF("Pairing initiated successfully");
    // Discovery will be started after pairing is complete
    // For now, let's try discovery after a short delay
    k_work_schedule(&self_->discovery_work_, K_MSEC(2000));
  }
}

void SonyRemote::on_disconnected(bt_conn * /*conn*/, uint8_t reason) {
  if (!self_) {
    LOG_ERR("self_ pointer is NULL in on_disconnected!");
    return;
  }

  LOG_INF("Disconnected (0x%02x)", reason);
  if (self_->conn_) {
    bt_conn_unref(self_->conn_);
    self_->conn_ = nullptr;
  }
  self_->ff01_handle_ = 0;
  self_->is_paired_ = false; // Reset pairing status on disconnect
  // resume scanning
  static struct bt_le_scan_param scan_param =
      BT_LE_SCAN_PARAM_INIT(BT_LE_SCAN_TYPE_ACTIVE, BT_LE_SCAN_OPT_NONE,
                            BT_GAP_SCAN_FAST_INTERVAL, BT_GAP_SCAN_FAST_WINDOW);
  int err = bt_le_scan_start(&scan_param, SonyRemote::on_scan);
  if (err)
    LOG_ERR("Scan restart failed (%d)", err);
}

uint8_t SonyRemote::on_discover(bt_conn * /*conn*/, const bt_gatt_attr *attr,
                                bt_gatt_discover_params *params) {
  // Add safety check for self_ pointer
  if (!self_) {
    LOG_ERR("self_ pointer is NULL!");
    return BT_GATT_ITER_STOP;
  }

  if (!attr) {
    LOG_DBG("Discovery complete");
    std::memset(params, 0, sizeof(*params));
    return BT_GATT_ITER_STOP;
  }

  if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
    const auto *chrc = static_cast<const bt_gatt_chrc *>(attr->user_data);
    if (chrc && chrc->uuid && chrc->uuid->type == BT_UUID_TYPE_16) {
      uint16_t uuid_val = BT_UUID_16(chrc->uuid)->val;
      LOG_DBG("Found characteristic UUID: 0x%04x (handle 0x%04x)", uuid_val,
              chrc->value_handle);

      if (uuid_val == 0xFF01) {
        self_->ff01_handle_ = chrc->value_handle;
        self_->is_paired_ = true; // Set paired flag when FF01 is found
        LOG_INF("Found FF01 (handle 0x%04x) - camera is ready!",
                self_->ff01_handle_);

        // Log the characteristic properties
        LOG_INF("FF01 properties: 0x%02x", chrc->properties);
        if (chrc->properties & BT_GATT_CHRC_WRITE_WITHOUT_RESP) {
          LOG_INF("✓ Write Without Response supported");
        } else {
          LOG_WRN("⚠ Write Without Response NOT supported");
        }

        std::memset(params, 0, sizeof(*params));
        return BT_GATT_ITER_STOP;
      }
    }
  }
  return BT_GATT_ITER_CONTINUE;
}

void SonyRemote::start_discovery() {
  // Check if we have proper security level before discovering services
  bt_security_t sec_level = bt_conn_get_security(conn_);
  LOG_INF("Connection security level: %d", sec_level);

  if (sec_level < BT_SECURITY_L2) {
    LOG_WRN("Security level too low (%d), retrying after delay...", sec_level);
    // Retry discovery after a delay if security is not established
    k_work_schedule(&discovery_work_, K_MSEC(1000));
    return;
  }

  // Only discover the FF01 characteristic directly
  // Remove the unnecessary primary service discovery to avoid conflicts
  disc_params_ = {};
  disc_params_.uuid = &kFF01Uuid.uuid;
  disc_params_.type = BT_GATT_DISCOVER_CHARACTERISTIC;
  disc_params_.func = SonyRemote::on_discover;
  disc_params_.start_handle = 0x0001;
  disc_params_.end_handle = 0xffff;

  int err = bt_gatt_discover(conn_, &disc_params_);
  if (err) {
    LOG_ERR("Char discover err: %d", err);
  }
}

void SonyRemote::on_scan(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                         net_buf_simple *ad) {
  if (!self_) {
    LOG_ERR("self_ pointer is NULL in on_scan!");
    return;
  }

  if (self_->has_target_addr_) {
    char found_addr_str[BT_ADDR_LE_STR_LEN];
    char target_addr_str[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(addr, found_addr_str, sizeof(found_addr_str));
    bt_addr_le_to_str(&self_->target_addr_, target_addr_str,
                      sizeof(target_addr_str));

    LOG_DBG("Comparing found: %s vs target: %s", found_addr_str,
            target_addr_str);

    if (!bt_addr_le_eq(addr, &self_->target_addr_)) {
      LOG_DBG("Ignoring non-target device: %s", found_addr_str);
      return;
    }
  }

  bt_data_parse(ad, accept_any, nullptr);

  if (type == BT_GAP_ADV_TYPE_ADV_IND ||
      type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
    char s[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(addr, s, sizeof(s));

    if (self_->has_target_addr_) {
      LOG_INF("Found target camera %s (RSSI %d), connecting...", s, rssi);
    } else {
      LOG_DBG("Trying to connect %s (RSSI %d)", s, rssi);
    }

    if (bt_le_scan_stop() == 0) {
      static struct bt_conn_le_create_param create_param =
          BT_CONN_LE_CREATE_PARAM_INIT(BT_CONN_LE_OPT_NONE,
                                       BT_GAP_SCAN_FAST_INTERVAL,
                                       BT_GAP_SCAN_FAST_WINDOW);
      static struct bt_le_conn_param conn_param = BT_LE_CONN_PARAM_INIT(
          BT_GAP_INIT_CONN_INT_MIN, BT_GAP_INIT_CONN_INT_MAX, 0, 400);
      int err =
          bt_conn_le_create(addr, &create_param, &conn_param, &self_->conn_);
      if (err) {
        LOG_ERR("Create conn failed (%d)", err);
        static struct bt_le_scan_param scan_param = BT_LE_SCAN_PARAM_INIT(
            BT_LE_SCAN_TYPE_ACTIVE, BT_LE_SCAN_OPT_NONE,
            BT_GAP_SCAN_FAST_INTERVAL, BT_GAP_SCAN_FAST_WINDOW);
        bt_le_scan_start(&scan_param, SonyRemote::on_scan);
      }
    }
  }
}

void SonyRemote::send_cmd(const uint8_t *buf, size_t len) {
  if (!ready()) {
    LOG_WRN(
        "Camera not ready, command ignored (conn=%p, ff01=0x%04x, paired=%d)",
        conn_, ff01_handle_, is_paired_);
    return;
  }

  if (!conn_ || ff01_handle_ == 0) {
    LOG_ERR("No connection or handle for sending command");
    return;
  }

  // Log the command being sent
  LOG_INF("Sending command of %d bytes:", len);
  for (size_t i = 0; i < len; i++) {
    LOG_INF("  [%d]: 0x%02x", i, buf[i]);
  }

  int err =
      bt_gatt_write_without_response(conn_, ff01_handle_, buf, len, false);
  if (err) {
    LOG_ERR("GATT write failed (%d)", err);
  } else {
    LOG_INF("Command sent successfully to handle 0x%04x", ff01_handle_);
  }
}

void SonyRemote::discovery_work_handler(struct k_work *work) {
  if (!self_) {
    LOG_ERR("self_ pointer is NULL in discovery_work_handler!");
    return;
  }

  LOG_INF("Starting delayed service discovery...");
  self_->start_discovery();
}