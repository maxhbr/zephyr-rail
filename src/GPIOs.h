#ifndef __GPIOS_H_
#define __GPIOS_H_

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/zephyr.h>
#include <zephyr/types.h>

#include <lvgl.h>

#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>

class GPIO {
  const struct device *dev;
  int pin = -1;
  int ret = -1;
  bool cur_value = false;

public:
  GPIO(const char *label, int _pin, int flags);
  void set(bool value);
};

class LED : public GPIO {
public:
  LED(const char *label, int _pin, int flags) : GPIO(label, _pin, flags) {
    set(true);
    k_msleep(100);
    set(false);
  };
};

#endif // __GPIOS_H_
