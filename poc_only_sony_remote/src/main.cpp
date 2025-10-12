#include "sony_remote/sony_remote.h"
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

int main(void) {
  LOG_INF("Sony Remote Proof-of-Concept Application");

  // Initialize Bluetooth
  if (int err = bt_enable(nullptr); err) {
    LOG_ERR("bt_enable failed (%d)", err);
    return err;
  }

  LOG_INF("BLE on. Enable 'Bluetooth Rmt Ctrl' on the Sony camera and pair on "
          "first connect.");

  // Initialize Sony Remote with specific camera address
  SonyRemote remote("9C:50:D1:AF:76:5F");
  remote.begin();

  // Add a small delay before starting scan
  k_msleep(100);
  remote.startScan();

  k_msleep(100);

  // Simple test loop
  while (true) {
    if (remote.ready()) {
      LOG_INF("Connected to camera, take one picture...");
      remote.shoot();
      return;
    } else {
      LOG_INF("Waiting for camera connection...");
      k_sleep(K_SECONDS(1));
    }
  }

  return 0;
}