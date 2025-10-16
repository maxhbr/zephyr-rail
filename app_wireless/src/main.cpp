#include "pwa_service.h"
#include "sony_remote/sony_remote.h"
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#ifdef CONFIG_BT_SETTINGS
#include <zephyr/settings/settings.h>
#endif

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

// Connection callback dispatcher
static void conn_connected(struct bt_conn *conn, uint8_t err) {
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role == BT_CONN_ROLE_PERIPHERAL) {
    // This is a PWA connection (we are peripheral)
    PwaService::onConnected(conn, err);
  } else {
    // This is a Sony camera connection (we are central)
    SonyRemote::on_connected(conn, err);
  }
}

static void conn_disconnected(struct bt_conn *conn, uint8_t reason) {
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role == BT_CONN_ROLE_PERIPHERAL) {
    // PWA disconnected
    PwaService::onDisconnected(conn, reason);
  } else {
    // Sony camera disconnected
    SonyRemote::on_disconnected(conn, reason);
  }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = conn_connected,
    .disconnected = conn_disconnected,
};

int main(void) {
  LOG_INF("ZephyrRail: Sony Camera Remote + PWA Control");

  // Initialize Bluetooth
  if (int err = bt_enable(nullptr); err) {
    LOG_ERR("bt_enable failed (%d)", err);
    return err;
  }

#ifdef CONFIG_BT_SETTINGS
  // Load Bluetooth settings (bonding info, etc.)
  if (int err = settings_load(); err) {
    LOG_ERR("settings_load failed (%d)", err);
    return err;
  }
#endif

  LOG_INF("Bluetooth enabled");

  // Initialize Sony Remote with specific camera address
  SonyRemote remote("9C:50:D1:AF:76:5F");
  remote.begin();

  // Initialize PWA service
  PwaService::init(&remote);

  // Start advertising for PWA connections (peripheral role)
  if (int err = PwaService::startAdvertising(); err) {
    LOG_ERR("Failed to start advertising: %d", err);
    return err;
  }

  // Start scanning for Sony camera (central role)
  k_msleep(100);
  remote.startScan();

  LOG_INF("System ready: Advertising for PWA and scanning for Sony camera");

  // Main loop - keep both connections alive
  while (true) {
    if (remote.ready()) {
      LOG_DBG("Sony camera connected and ready");
    } else {
      LOG_DBG("Waiting for Sony camera connection...");
    }

    if (PwaService::isConnected()) {
      LOG_DBG("PWA client connected");
    }

    k_sleep(K_SECONDS(5));
  }

  return 0;
}