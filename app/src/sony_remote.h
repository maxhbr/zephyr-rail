// mostly written by GPT5, based on https://gregleeds.com/reverse-engineering-sony-camera-bluetooth/
#pragma once

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <cstdint>
#include <cstddef>

class SonyRemote {
public:
    SonyRemote();
    void begin();                // bt_enable() must be called before this
    void startScan();            // start scanning for the camera
    bool ready() const;          // connected + FF01 handle found?

    // Common actions (write to 0xFF01)
    void focusDown();
    void focusUp();
    void shutterDown();
    void shutterUp();
    void recToggle();
    void zoomTPress();
    void zoomTRelease();
    void zoomWPress();
    void zoomWRelease();

    // callbacks (public because they need to be accessed from C code)
    static void on_connected(struct bt_conn* conn, uint8_t err);
    static void on_disconnected(struct bt_conn* conn, uint8_t reason);
    static uint8_t on_discover(struct bt_conn* conn,
                               const struct bt_gatt_attr* attr,
                               struct bt_gatt_discover_params* params);
    static void on_scan(const bt_addr_le_t* addr, int8_t rssi, uint8_t type,
                        struct net_buf_simple* ad);

private:
    // singleton-style bridge for C callbacks
    static SonyRemote* self_;

    bt_conn* conn_ = nullptr;
    uint16_t ff01_handle_ = 0;

    bt_gatt_discover_params disc_params_{};
    bt_gatt_subscribe_params sub_params_{}; // reserved if you later use 0xFF02

    // helpers
    void start_discovery();
    void send_cmd(const uint8_t* buf, size_t len);
};
