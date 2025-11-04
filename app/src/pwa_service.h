#pragma once

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>

/**
 * @brief PWA GATT Service for Web Bluetooth control
 *
 * This service exposes two characteristics:
 * - Command (write): Receives commands from the PWA
 * - Status (read + notify): Sends status updates to the PWA
 */
class PwaService {
public:
  /**
   * @brief Start advertising the PWA service
   * @return 0 on success, negative error code on failure
   */
  static int startAdvertising();

  /**
   * @brief Stop advertising
   * @return 0 on success, negative error code on failure
   */
  static int stopAdvertising();

  /**
   * @brief Send a status notification to connected PWA clients
   * @param status Status string to send
   */
  static void notifyStatus(const char *status);

  /**
   * @brief Check if a PWA client is connected
   * @return true if connected, false otherwise
   */
  static bool isConnected();

  // Connection callbacks
  static void onConnected(struct bt_conn *conn, uint8_t err);
  static void onDisconnected(struct bt_conn *conn, uint8_t reason);

  // GATT callbacks (public so C wrappers can access them)
  static ssize_t cmdWrite(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                          const void *buf, uint16_t len, uint16_t offset,
                          uint8_t flags);
  static void cccChanged(const struct bt_gatt_attr *attr, uint16_t value);

  // Public access to status buffer for GATT service
  static uint8_t status_buffer_[256];

private:
  static struct bt_conn *pwa_conn_;
  static bool notify_enabled_;

  // Helper to find status attribute for notifications
  static const struct bt_gatt_attr *findStatusAttr();
};
