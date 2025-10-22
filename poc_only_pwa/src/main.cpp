#include "pwa_service.h"
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#ifdef CONFIG_BT_SETTINGS
#include <zephyr/settings/settings.h>
#endif

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

// Connection callback
static void conn_connected(struct bt_conn *conn, uint8_t err) {
  PwaService::onConnected(conn, err);
}

static void conn_disconnected(struct bt_conn *conn, uint8_t reason) {
  PwaService::onDisconnected(conn, reason);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = conn_connected,
    .disconnected = conn_disconnected,
};

int main(void) {
  LOG_INF("═══════════════════════════════════════════════════════");
  LOG_INF("PWA Proof of Concept - Web Bluetooth Test");
  LOG_INF("═══════════════════════════════════════════════════════");

  // Initialize Bluetooth
  if (int err = bt_enable(nullptr); err) {
    LOG_ERR("bt_enable failed (%d)", err);
    return err;
  }

#ifdef CONFIG_BT_SETTINGS
  if (int err = settings_load(); err) {
    LOG_ERR("settings_load failed (%d)", err);
    return err;
  }
#endif

  LOG_INF("Bluetooth initialized");

  // Initialize PWA service
  PwaService::init();

  // Start advertising
  if (int err = PwaService::startAdvertising(); err) {
    LOG_ERR("Failed to start advertising: %d", err);
    return err;
  }

  LOG_INF("═══════════════════════════════════════════════════════");
  LOG_INF("Ready! Connect from Web Bluetooth capable browser");
  LOG_INF("Device Name: %s", CONFIG_BT_DEVICE_NAME);
  LOG_INF("═══════════════════════════════════════════════════════");

  // Main loop - just keep running
  while (true) {
    if (PwaService::isConnected()) {
      LOG_DBG("PWA client connected - waiting for commands...");
    }
    k_sleep(K_SECONDS(10));
  }

  return 0;
}