#include "maybe_bluetooth.h"
#include <zephyr/logging/log.h>

#ifdef CONFIG_BT
#include "bluetooth.h"
#else
#include "sony_remote/fake_sony_remote.h"
#endif

LOG_MODULE_REGISTER(maybe_bluetooth, LOG_LEVEL_INF);

SonyRemote *init_maybe_bluetooth(void) {
#ifdef CONFIG_BT
  return init_bluetooth();
#else
  static SonyRemote remote;
  LOG_INF("initialize Dummy Sony Remote ...");
  return &remote;
#endif
}