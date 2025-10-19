// mostly written by GPT5, based on
// https://gregleeds.com/reverse-engineering-sony-camera-bluetooth/
#pragma once

#include <cstddef>
#include <cstdint>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

class SonyRemote {
public:
  SonyRemote();
  SonyRemote(
      const char *target_address); // Constructor with specific BT address
  void begin();                    // bt_enable() must be called before this
  void startScan();                // start scanning for the camera
  void end();                      // disconnect and cleanup
  bool ready() const;              // connected + FF01 handle found?

  // Common actions (write to 0xFF01)
  void focusDown();
  void focusUp();
  void shutterDown();
  void shutterUp();
  void cOneDown();
  void cOneUp();
  void recToggle();
  void zoomTPress();
  void zoomTRelease();
  void zoomWPress();
  void zoomWRelease();

  // high level functions:
  void shoot();

  // callbacks (public because they need to be accessed from C code)
  static void on_connected(struct bt_conn *conn, uint8_t err);
  static void on_disconnected(struct bt_conn *conn, uint8_t reason);
  static uint8_t on_discover_service(struct bt_conn *conn,
                                     const struct bt_gatt_attr *attr,
                                     struct bt_gatt_discover_params *params);
  static uint8_t on_discover(struct bt_conn *conn,
                             const struct bt_gatt_attr *attr,
                             struct bt_gatt_discover_params *params);
  static void on_scan(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                      struct net_buf_simple *ad);

  // work handler for delayed discovery
  static void discovery_work_handler(struct k_work *work);

  // write completion callback
  static void on_write_complete(struct bt_conn *conn, uint8_t err,
                                struct bt_gatt_write_params *params);

private:
  // singleton-style bridge for C callbacks
  static SonyRemote *self_;

  bt_conn *conn_ = nullptr;
  uint16_t ff01_handle_ = 0;
  uint8_t ff01_properties_ = 0; // Store FF01 characteristic properties

  bt_gatt_discover_params disc_params_{};
  bt_gatt_subscribe_params sub_params_{}; // reserved if you later use 0xFF02
  bt_gatt_write_params write_params_{};   // Parameters for write operations
  uint8_t write_buffer_[32]; // Buffer to store command data during write

  k_work_delayable discovery_work_;

  // Target camera address (if specified)
  bt_addr_le_t target_addr_;
  bool has_target_addr_ = false;

  // Pairing status
  bool is_paired_ = false;

  void start_discovery();
  void send_cmd(const uint8_t *buf, size_t len);
};