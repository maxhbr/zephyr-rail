#include "bluetooth.h"
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_BT_SETTINGS
#include <zephyr/settings/settings.h>
#endif

#include "pwa_service.h"
#include "sony_remote/sony_remote.h"

LOG_MODULE_REGISTER(bluetooth, LOG_LEVEL_INF);

static void unified_connected(struct bt_conn *conn, uint8_t err) {
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role == BT_CONN_ROLE_PERIPHERAL) {
    PwaService::onConnected(conn, err);
  } else if (info.role == BT_CONN_ROLE_CENTRAL) {
    SonyRemote::on_connected(conn, err);
  }
}

static void unified_disconnected(struct bt_conn *conn, uint8_t reason) {
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role == BT_CONN_ROLE_PERIPHERAL) {
    PwaService::onDisconnected(conn, reason);
  } else if (info.role == BT_CONN_ROLE_CENTRAL) {
    SonyRemote::on_disconnected(conn, reason);
  }
}

static void unified_security_changed(struct bt_conn *conn, bt_security_t level,
                                     enum bt_security_err err) {
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role == BT_CONN_ROLE_CENTRAL) {
    SonyRemote::on_security_changed(conn, level, err);
  } else if (info.role == BT_CONN_ROLE_PERIPHERAL) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (err) {
      LOG_ERR("PWA security failed: %s level %u err %d", addr, level, err);
    } else {
      LOG_INF("PWA security changed: %s level %u", addr, level);
    }
  }
}

static bt_conn_cb kConnCbs = {
    .connected = unified_connected,
    .disconnected = unified_disconnected,
    .security_changed = unified_security_changed,
};

static void unified_auth_cancel(struct bt_conn *conn) {
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role == BT_CONN_ROLE_PERIPHERAL) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_WRN("PWA pairing cancelled: %s", addr);
    return;
  }

  SonyRemote::auth_cancel(conn);
}

static void unified_auth_passkey_display(struct bt_conn *conn,
                                         unsigned int passkey) {
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role == BT_CONN_ROLE_PERIPHERAL) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("PWA passkey display for %s: %06u", addr, passkey);
    return;
  }

  SonyRemote::auth_passkey_display(conn, passkey);
}

static void unified_auth_passkey_confirm(struct bt_conn *conn,
                                         unsigned int passkey) {
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role == BT_CONN_ROLE_PERIPHERAL) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("PWA confirm passkey for %s: %06u", addr, passkey);
    bt_conn_auth_passkey_confirm(conn);
    return;
  }

  SonyRemote::auth_passkey_confirm(conn, passkey);
}

static void unified_auth_passkey_entry(struct bt_conn *conn) {
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role == BT_CONN_ROLE_PERIPHERAL) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("PWA passkey entry for %s", addr);
    return;
  }

  SonyRemote::auth_passkey_entry(conn);
}

static enum bt_security_err unified_auth_pairing_confirm(struct bt_conn *conn) {
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role == BT_CONN_ROLE_PERIPHERAL) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("PWA pairing confirm for %s - accepting", addr);
    return BT_SECURITY_ERR_SUCCESS;
  }

  return SonyRemote::auth_pairing_confirm(conn);
}

static bt_conn_auth_cb kAuthCbs = {
    .passkey_display = unified_auth_passkey_display,
    .passkey_entry = unified_auth_passkey_entry,
    .passkey_confirm = unified_auth_passkey_confirm,
    .cancel = unified_auth_cancel,
    .pairing_confirm = unified_auth_pairing_confirm,
};

SonyRemote *init_bluetooth(void) {
  LOG_INF("initialize Bluetooth ...");
  if (int err = bt_enable(nullptr); err) {
    LOG_ERR("bt_enable failed (%d)", err);
    return nullptr;
  }
#ifdef CONFIG_BT_SETTINGS
  if (int err = settings_load(); err) {
    LOG_ERR("settings_load failed (%d), but continue...", err);
  }
#endif

  LOG_INF("initialize PWA service ...");
  if (int err = PwaService::startAdvertising(); err) {
    LOG_ERR("Failed to start PWA advertising: %d", err);
    return nullptr;
  }
  LOG_DBG("PWA service ready - Web Bluetooth interface available");

  LOG_INF("initialize Sony Remote ...");
  static SonyRemote remote("9C:50:D1:AF:76:5F");
  // SonyRemote remote("CC:C0:79:DA:94:B6");

  k_msleep(100);
  remote.startScan();
  k_msleep(100);

  bt_conn_cb_register(&kConnCbs);
  bt_conn_auth_cb_register(&kAuthCbs);

  k_msleep(100);
  return &remote;
}