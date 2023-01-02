#ifndef GYRO_WAITER_H
#define GYRO_WAITER_H
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/zephyr.h>
#include <zephyr/types.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/net/buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/printk.h>

#include <zephyr/console/console.h>

#include <lvgl.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>

#define MPU6050_LABEL DT_LABEL(DT_INST(0, invensense_mpu6050))

class GyroWaiter {
  const struct device *mpu6050 = device_get_binding(MPU6050_LABEL);
  lv_obj_t *label;
  int num_of_samples = 4;
  double boundary = 0.1 * num_of_samples;
  int bail_count = 400;
  int sleep_msec = 50;

  double offset_0 = 0;
  double offset_1 = 0;
  double offset_2 = 0;

public:
  GyroWaiter(){};
  void init();
  int wait();
};
#endif /* GYRO_WAITER_H */
