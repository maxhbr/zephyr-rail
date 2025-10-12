#include "sony_remote/sony_remote.h"
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

int main(void) {
  LOG_INF("Sony Remote Test Application");

  // Initialize Bluetooth
  if (int err = bt_enable(nullptr); err) {
    LOG_ERR("bt_enable failed (%d)", err);
    return err;
  }

  LOG_INF("BLE on. Enable 'Bluetooth Rmt Ctrl' on the Sony camera and pair on "
          "first connect.");

  // Initialize Sony Remote
  SonyRemote remote;
  remote.begin();
  remote.startScan();

  // Simple test loop
  while (true) {
    if (remote.ready()) {
      LOG_INF("Camera connected! Taking a photo...");

      // Take a photo
      remote.focusDown();
      k_msleep(100);
      remote.shutterDown();
      k_msleep(100);
      remote.shutterUp();
      k_msleep(100);
      remote.focusUp();

      LOG_INF("Photo taken!");
      k_sleep(K_SECONDS(5)); // Wait 5 seconds before next photo
    } else {
      LOG_INF("Waiting for camera connection...");
      k_sleep(K_SECONDS(1));
    }
  }

  return 0;
}