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

  int photo_count = 0;
  bool first_time = true;

  // Simple test loop
  while (true) {
    if (remote.ready()) {
      if (first_time) {
        LOG_INF("Camera is ready! Testing individual commands first...");

        // Test just focus commands first
        LOG_INF("Testing Focus Down...");
        remote.focusDown();
        k_sleep(K_SECONDS(2));

        LOG_INF("Testing Focus Up...");
        remote.focusUp();
        k_sleep(K_SECONDS(2));

        LOG_INF("Individual command test complete. Starting photo sequence...");
        first_time = false;
      }

      LOG_INF("=== Taking photo #%d ===", ++photo_count);

      // Follow the exact sequence from the spec:
      // 1. Focus Down (0x0107)
      LOG_INF("Step 1: Focus Down");
      remote.focusDown();
      k_msleep(200); // Longer delay

      // 2. Shutter Down (0x0109)
      LOG_INF("Step 2: Shutter Down");
      remote.shutterDown();
      k_msleep(200);

      // 3. Shutter Up (0x0108)
      LOG_INF("Step 3: Shutter Up");
      remote.shutterUp();
      k_msleep(200);

      // 4. Focus Up (0x0106)
      LOG_INF("Step 4: Focus Up");
      remote.focusUp();

      LOG_INF("=== Photo sequence complete ===");
      k_sleep(K_SECONDS(5)); // Wait 5 seconds before next photo
    } else {
      LOG_INF("Waiting for camera connection...");
      k_sleep(K_SECONDS(1));
    }
  }

  return 0;
}