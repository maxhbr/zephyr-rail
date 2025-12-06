// mostly written by GPT5, based on
// https://gregleeds.com/reverse-engineering-sony-camera-bluetooth/
#include "sony_remote/sony_remote.h"
#include <cstring>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(sony_remote, LOG_LEVEL_INF);

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

// // Little-endian bytes for 8000FF00-FF00-FFFF-FFFF-FFFFFFFFFFFF
// static bt_uuid_128 kSonySvcUuid =
//     BT_UUID_INIT_128(0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
//     0xff,
//                      0x00, 0xff, 0x00, 0xff, 0x00, 0x80);

static bt_uuid_16 kFF01Uuid = BT_UUID_INIT_16(0xFF01);

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

  LOG_WRN("Device that actually should be rejected by filter, type: %u",
          data->type);
  return true; // For now, still accept any device for broader compatibility
}

} // namespace

SonyRemote::SonyRemote() {
  self_ = this;
  k_work_init_delayable(&discovery_work_, discovery_work_handler);
  has_target_addr_ = false;
  is_paired_ = false;
  discovery_retry_count_ = 0;
}

SonyRemote::SonyRemote(const char *target_address) {
  self_ = this;
  k_work_init_delayable(&discovery_work_, discovery_work_handler);
  is_paired_ = false;
  discovery_retry_count_ = 0;

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

void SonyRemote::end() {
  if (conn_) {
    bt_conn_unref(conn_);
    conn_ = nullptr;
  }
  ff01_handle_ = 0;
  k_work_cancel_delayable(&discovery_work_);
}

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

void SonyRemote::stopScan() {
  int err = bt_le_scan_stop();
  if (err) {
    LOG_ERR("Scan stop failed (%d)", err);
  } else {
    LOG_INF("Bluetooth LE scan stopped");
  }
  k_work_cancel_delayable(&discovery_work_);
}

bool SonyRemote::ready() const {
  if (conn_ != nullptr && ff01_handle_ != 0 && !is_paired_) {
    LOG_DBG("connected but not paired");
  }
  return conn_ != nullptr && ff01_handle_ != 0 && is_paired_;
}

char *SonyRemote::state() {
  static char buffer[128];
  snprintf(buffer, sizeof(buffer),
           "SonyRemote: connected=%s, paired=%s, ff01_handle=0x%04x",
           conn_ ? "true" : "false", is_paired_ ? "true" : "false",
           ff01_handle_);
  return buffer;
}

void SonyRemote::log_state() { LOG_INF("%s", state()); }

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

void SonyRemote::shoot() {
  int delay = 50;
  LOG_DBG("shoot ...");
  focusDown();
  k_msleep(delay);
  shutterDown();
  k_msleep(delay);
  shutterUp();
  k_msleep(delay);
  focusUp();
  LOG_DBG("... done shooting");
}

void SonyRemote::on_connected(bt_conn *conn, uint8_t err) {
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role != BT_CONN_ROLE_CENTRAL) {
    // Not a Sony camera connection, ignore
    return;
  }

  if (!self_) {
    LOG_ERR("self_ pointer is NULL in on_connected!");
    return;
  }

  if (err) {
    LOG_ERR("Connect failed (%u)", err);
    return;
  }

  char addr_str[BT_ADDR_LE_STR_LEN];
  bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));
  LOG_INF("Connected to %s", addr_str);

  if (self_->conn_)
    bt_conn_unref(self_->conn_);
  self_->conn_ = bt_conn_ref(conn);

  // Check current security level
  bt_security_t sec_level = bt_conn_get_security(conn);
  LOG_INF("Current security level: %d", sec_level);

  if (sec_level < BT_SECURITY_L2) {
    // Start pairing process - security_changed callback will handle discovery
    LOG_INF("Starting security/pairing...");
    int pairing_err = bt_conn_set_security(conn, BT_SECURITY_L2);
    if (pairing_err) {
      LOG_ERR("Failed to start pairing (%d)", pairing_err);
      // Try discovery anyway in case device is already bonded
      k_work_schedule(&self_->discovery_work_, K_MSEC(1000));
    }
  } else {
    // Already paired, start discovery immediately
    LOG_INF("Already paired (security level %d), starting discovery...",
            sec_level);
    self_->is_paired_ = true;
    self_->start_discovery();
  }
}

void SonyRemote::on_disconnected(bt_conn *conn, uint8_t reason) {
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role != BT_CONN_ROLE_CENTRAL) {
    // Not a Sony camera connection, ignore
    return;
  }

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
  self_->ff01_properties_ = 0;       // Reset properties
  self_->is_paired_ = false;         // Reset pairing status on disconnect
  self_->discovery_retry_count_ = 0; // Reset retry counter on disconnect
  // resume scanning
  self_->startScan();
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
        self_->ff01_properties_ = chrc->properties; // Store properties
        self_->is_paired_ = true; // Set paired flag when FF01 is found
        LOG_INF("Found FF01 (handle 0x%04x) - camera is ready!",
                self_->ff01_handle_);

        // Log the characteristic properties
        LOG_DBG("FF01 properties: 0x%02x", chrc->properties);
        if (chrc->properties & BT_GATT_CHRC_WRITE_WITHOUT_RESP) {
          LOG_DBG("Write Without Response supported");
        } else if (chrc->properties & BT_GATT_CHRC_WRITE) {
          LOG_DBG("Write (with Response) supported");
        } else {
          LOG_WRN("Neither Write nor Write Without Response supported");
        }

        std::memset(params, 0, sizeof(*params));
        return BT_GATT_ITER_STOP;
      }
    }
  }
  return BT_GATT_ITER_CONTINUE;
}

uint8_t SonyRemote::on_discover_service(bt_conn * /*conn*/,
                                        const bt_gatt_attr *attr,
                                        bt_gatt_discover_params *params) {
  if (!self_) {
    LOG_ERR("self_ pointer is NULL!");
    return BT_GATT_ITER_STOP;
  }

  if (!attr) {
    LOG_WRN("Sony service not found!");
    std::memset(params, 0, sizeof(*params));
    return BT_GATT_ITER_STOP;
  }

  if (params->type == BT_GATT_DISCOVER_PRIMARY) {
    const auto *service =
        static_cast<const bt_gatt_service_val *>(attr->user_data);
    if (service) {
      LOG_INF("Found Sony service (handle range: 0x%04x - 0x%04x)",
              attr->handle, service->end_handle);

      // Now discover characteristics within this service
      self_->disc_params_ = {};
      self_->disc_params_.uuid = &kFF01Uuid.uuid;
      self_->disc_params_.type = BT_GATT_DISCOVER_CHARACTERISTIC;
      self_->disc_params_.func = SonyRemote::on_discover;
      self_->disc_params_.start_handle = attr->handle;
      self_->disc_params_.end_handle = service->end_handle;

      int err = bt_gatt_discover(self_->conn_, &self_->disc_params_);
      if (err) {
        LOG_ERR("Characteristic discovery failed: %d", err);
      }

      std::memset(params, 0, sizeof(*params));
      return BT_GATT_ITER_STOP;
    }
  }

  return BT_GATT_ITER_CONTINUE;
}

void SonyRemote::start_discovery() {
  LOG_DBG("Starting service discovery...");
  // Check if we have proper security level before discovering services
  bt_security_t sec_level = bt_conn_get_security(conn_);

  if (sec_level < BT_SECURITY_L2) {
    if (discovery_retry_count_ >= MAX_DISCOVERY_RETRIES) {
      LOG_ERR("Maximum discovery retries (%d) reached, giving up",
              MAX_DISCOVERY_RETRIES);
      return;
    }

    LOG_WRN("Security level too low (%d < %d), retrying after delay... "
            "(attempt %d/%d)",
            sec_level, BT_SECURITY_L2, discovery_retry_count_ + 1,
            MAX_DISCOVERY_RETRIES);
    discovery_retry_count_++;
    bt_conn_set_security(conn_, BT_SECURITY_L2);

    k_work_schedule(&discovery_work_, K_MSEC(1000));
    return;
  }

  // Reset retry counter on successful security level
  discovery_retry_count_ = 0;

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

    if (!bt_addr_le_eq(addr, &self_->target_addr_)) {
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
        bt_conn_unref(self_->conn_);
        self_->conn_ = nullptr;
        k_msleep(300);

        self_->startScan();
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
  LOG_DBG("Sending command of %d bytes:", len);
  for (size_t i = 0; i < len; i++) {
    LOG_DBG("  [%d]: 0x%02x", i, buf[i]);
  }

  // Ensure command fits in our buffer
  if (len > sizeof(write_buffer_)) {
    LOG_ERR("Command too large (%d bytes, max %d)", len, sizeof(write_buffer_));
    return;
  }

  int err;
  if (ff01_properties_ & BT_GATT_CHRC_WRITE_WITHOUT_RESP) {
    LOG_DBG("Using Write Without Response");
    err = bt_gatt_write_without_response(conn_, ff01_handle_, buf, len, false);
  } else if (ff01_properties_ & BT_GATT_CHRC_WRITE) {
    LOG_DBG("Using Write (with Response)");

    // Copy data to our persistent buffer
    memcpy(write_buffer_, buf, len);

    // Set up write parameters for bt_gatt_write using member variable
    write_params_.func = SonyRemote::on_write_complete;
    write_params_.handle = ff01_handle_;
    write_params_.offset = 0;
    write_params_.data = write_buffer_;
    write_params_.length = len;

    err = bt_gatt_write(conn_, &write_params_);
  } else {
    LOG_ERR("FF01 characteristic doesn't support write operations");
    return;
  }

  if (err) {
    LOG_ERR("GATT write failed (%d)", err);
  } else {
    LOG_DBG("Command sent successfully to handle 0x%04x", ff01_handle_);
  }
}

void SonyRemote::on_write_complete(bt_conn *conn, uint8_t err,
                                   bt_gatt_write_params *params) {
  if (err) {
    LOG_ERR("GATT write completed with error (%d)", err);
  } else {
    LOG_DBG("GATT write completed successfully");
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

void SonyRemote::on_security_changed(bt_conn *conn, bt_security_t level,
                                     enum bt_security_err err) {
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role != BT_CONN_ROLE_CENTRAL) {
    // Not a Sony camera connection, ignore
    return;
  }

  if (!self_) {
    LOG_ERR("self_ pointer is NULL in on_security_changed!");
    return;
  }

  char addr_str[BT_ADDR_LE_STR_LEN];
  bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));

  if (err) {
    LOG_ERR("Security failed: %s level %u err %d", addr_str, level, err);
    self_->is_paired_ = false;
  } else {
    LOG_INF("Security changed: %s level %u", addr_str, level);
    if (level >= BT_SECURITY_L2) {
      LOG_INF("Pairing successful! Starting service discovery...");
      self_->is_paired_ = true;
      self_->start_discovery();
    }
  }
}

void SonyRemote::auth_cancel(struct bt_conn *conn) {
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role != BT_CONN_ROLE_CENTRAL) {
    // Not a Sony camera connection, ignore
    return;
  }

  char addr[BT_ADDR_LE_STR_LEN];
  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
  LOG_WRN("Pairing cancelled: %s", addr);
}

void SonyRemote::auth_passkey_display(struct bt_conn *conn,
                                      unsigned int passkey) {
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role != BT_CONN_ROLE_CENTRAL) {
    // Not a Sony camera connection, ignore
    return;
  }

  char addr[BT_ADDR_LE_STR_LEN];
  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
  LOG_INF("Passkey for %s: %06u", addr, passkey);
}

void SonyRemote::auth_passkey_confirm(struct bt_conn *conn,
                                      unsigned int passkey) {
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role != BT_CONN_ROLE_CENTRAL) {
    // Not a Sony camera connection, ignore
    return;
  }

  char addr[BT_ADDR_LE_STR_LEN];
  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
  LOG_INF("Confirm passkey for %s: %06u", addr, passkey);
  // Auto-confirm the passkey
  bt_conn_auth_passkey_confirm(conn);
}

void SonyRemote::auth_passkey_entry(struct bt_conn *conn) {
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role != BT_CONN_ROLE_CENTRAL) {
    // Not a Sony camera connection, ignore
    return;
  }

  char addr[BT_ADDR_LE_STR_LEN];
  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
  LOG_INF("Enter passkey for %s", addr);
  // Sony cameras typically don't require passkey entry
}

enum bt_security_err SonyRemote::auth_pairing_confirm(struct bt_conn *conn) {
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role != BT_CONN_ROLE_CENTRAL) {
    // Not a Sony camera connection, reject pairing
    return BT_SECURITY_ERR_UNSPECIFIED;
  }

  char addr[BT_ADDR_LE_STR_LEN];
  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
  LOG_INF("Confirm pairing for %s", addr);
  // Auto-accept pairing
  return BT_SECURITY_ERR_SUCCESS;
}