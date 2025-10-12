// mostly written by GPT5, based on
// https://gregleeds.com/reverse-engineering-sony-camera-bluetooth/
#include "sony_remote/sony_remote.h"
#include <cstring>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(sony_remote, LOG_LEVEL_DBG);

SonyRemote *SonyRemote::self_ = nullptr;

namespace {
// Sony command payloads (write to FF01, Write Without Response)
constexpr uint8_t FOCUS_DOWN[] = {0x01, 0x07};
constexpr uint8_t FOCUS_UP[] = {0x01, 0x06};
constexpr uint8_t SHUTTER_DOWN[] = {0x01, 0x09};
constexpr uint8_t SHUTTER_UP[] = {0x01, 0x08};
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

static bool accept_any(bt_data * /*data*/, void * /*user*/) {
  return true; // make stricter later by parsing Manufacturer ID 0x012D or name
}

} // namespace

SonyRemote::SonyRemote() { self_ = this; }

void SonyRemote::begin() { bt_conn_cb_register(&kConnCbs); }

void SonyRemote::startScan() {
  static struct bt_le_scan_param scan_param =
      BT_LE_SCAN_PARAM_INIT(BT_LE_SCAN_TYPE_ACTIVE, BT_LE_SCAN_OPT_NONE,
                            BT_GAP_SCAN_FAST_INTERVAL, BT_GAP_SCAN_FAST_WINDOW);
  int err = bt_le_scan_start(&scan_param, SonyRemote::on_scan);
  if (err) {
    LOG_ERR("Scan start failed (%d)", err);
  } else {
    LOG_INF("Scanning for Sony camera...");
  }
}

bool SonyRemote::ready() const { return conn_ != nullptr && ff01_handle_ != 0; }

void SonyRemote::focusDown() { send_cmd(FOCUS_DOWN, sizeof(FOCUS_DOWN)); }
void SonyRemote::focusUp() { send_cmd(FOCUS_UP, sizeof(FOCUS_UP)); }
void SonyRemote::shutterDown() { send_cmd(SHUTTER_DOWN, sizeof(SHUTTER_DOWN)); }
void SonyRemote::shutterUp() { send_cmd(SHUTTER_UP, sizeof(SHUTTER_UP)); }
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
  if (err) {
    LOG_ERR("Connect failed (%u)", err);
    return;
  }
  LOG_INF("Connected");
  if (self_->conn_)
    bt_conn_unref(self_->conn_);
  self_->conn_ = bt_conn_ref(conn);
  self_->start_discovery();
}

void SonyRemote::on_disconnected(bt_conn * /*conn*/, uint8_t reason) {
  LOG_INF("Disconnected (0x%02x)", reason);
  if (self_->conn_) {
    bt_conn_unref(self_->conn_);
    self_->conn_ = nullptr;
  }
  self_->ff01_handle_ = 0;
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
  if (!attr) {
    LOG_DBG("Discovery complete");
    std::memset(params, 0, sizeof(*params));
    return BT_GATT_ITER_STOP;
  }

  if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
    const auto *chrc = static_cast<const bt_gatt_chrc *>(attr->user_data);
    if (chrc->uuid->type == BT_UUID_TYPE_16 &&
        BT_UUID_16(chrc->uuid)->val == 0xFF01) {
      self_->ff01_handle_ = chrc->value_handle;
      LOG_DBG("Found FF01 (handle 0x%04x)", self_->ff01_handle_);
      std::memset(params, 0, sizeof(*params));
      return BT_GATT_ITER_STOP;
    }
  }
  return BT_GATT_ITER_CONTINUE;
}

void SonyRemote::start_discovery() {
  // (Optional) discover the vendor service first
  disc_params_ = {};
  disc_params_.uuid = &kSonySvcUuid.uuid;
  disc_params_.type = BT_GATT_DISCOVER_PRIMARY;
  disc_params_.start_handle = 0x0001;
  disc_params_.end_handle = 0xffff;
  int err = bt_gatt_discover(conn_, &disc_params_);
  if (err)
    LOG_WRN("Primary svc discover err: %d (continuing)", err);

  // Robust: search the whole DB for 0xFF01 characteristic
  disc_params_ = {};
  disc_params_.uuid = &kFF01Uuid.uuid;
  disc_params_.type = BT_GATT_DISCOVER_CHARACTERISTIC;
  disc_params_.func = SonyRemote::on_discover;
  disc_params_.start_handle = 0x0001;
  disc_params_.end_handle = 0xffff;
  err = bt_gatt_discover(conn_, &disc_params_);
  if (err)
    LOG_ERR("Char discover err: %d", err);
}

void SonyRemote::on_scan(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                         net_buf_simple *ad) {
  bt_data_parse(ad, accept_any, nullptr);

  if (type == BT_GAP_ADV_TYPE_ADV_IND ||
      type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
    char s[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(addr, s, sizeof(s));
    LOG_DBG("Trying to connect %s (RSSI %d)", s, rssi);

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
    LOG_ERR("Not ready (conn=%p, ff01=0x%04x)", conn_, ff01_handle_);
    return;
  }
  int err =
      bt_gatt_write_without_response(conn_, ff01_handle_, buf, len, false);
  if (err)
    LOG_ERR("WWR failed: %d", err);
}