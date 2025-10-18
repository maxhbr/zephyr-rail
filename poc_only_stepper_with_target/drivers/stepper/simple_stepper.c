/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Simple Step/Direction Stepper Motor Driver with Enable Pin
 */

#define DT_DRV_COMPAT simple_stepper

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/stepper.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/* Use Zephyr's step_dir common infrastructure */
#include "../../../zephyr/drivers/stepper/step_dir/step_dir_stepper_common.h"

LOG_MODULE_REGISTER(simple_stepper, CONFIG_STEPPER_LOG_LEVEL);

/**
 * @brief Simple stepper driver configuration data.
 */
struct simple_stepper_config {
  struct step_dir_stepper_common_config common;
  struct gpio_dt_spec en_pin;
  bool invert_pins;
};

/**
 * @brief Simple stepper driver runtime data.
 */
struct simple_stepper_data {
  struct step_dir_stepper_common_data common;
  bool enabled;
};

/* Verify that common structs are first in our extended structs */
STEP_DIR_STEPPER_STRUCT_CHECK(struct simple_stepper_config,
                              struct simple_stepper_data);

static int simple_stepper_enable(const struct device *dev) {
  const struct simple_stepper_config *config = dev->config;
  struct simple_stepper_data *data = dev->data;
  int ret;

  if (!config->en_pin.port) {
    LOG_WRN("Enable pin not configured");
    return -ENOTSUP;
  }

  /* Set enable pin based on invert_pins flag */
  /* Normal mode (active low): set to 0 to enable */
  /* Inverted mode (active high): set to 1 to enable */
  int enable_value = config->invert_pins ? 1 : 0;
  ret = gpio_pin_set_dt(&config->en_pin, enable_value);
  if (ret != 0) {
    LOG_ERR("Failed to enable stepper (error: %d)", ret);
    return ret;
  }

  data->enabled = true;
  LOG_DBG("Stepper enabled");

  /* Small delay to allow driver to stabilize */
  k_sleep(K_USEC(10));

  return 0;
}

static int simple_stepper_disable(const struct device *dev) {
  const struct simple_stepper_config *config = dev->config;
  struct simple_stepper_data *data = dev->data;
  int ret;

  if (!config->en_pin.port) {
    LOG_WRN("Enable pin not configured");
    return -ENOTSUP;
  }

  /* Set disable pin based on invert_pins flag */
  /* Normal mode (active low): set to 1 to disable */
  /* Inverted mode (active high): set to 0 to disable */
  int disable_value = config->invert_pins ? 0 : 1;
  ret = gpio_pin_set_dt(&config->en_pin, disable_value);
  if (ret != 0) {
    LOG_ERR("Failed to disable stepper (error: %d)", ret);
    return ret;
  }

  data->enabled = false;
  LOG_DBG("Stepper disabled");

  return 0;
}

static int simple_stepper_move_by(const struct device *dev,
                                  int32_t micro_steps) {
  return step_dir_stepper_common_move_by(dev, micro_steps);
}

static int simple_stepper_move_to(const struct device *dev, int32_t value) {
  return step_dir_stepper_common_move_to(dev, value);
}

static int simple_stepper_set_reference_position(const struct device *dev,
                                                 int32_t value) {
  return step_dir_stepper_common_set_reference_position(dev, value);
}

static int simple_stepper_get_actual_position(const struct device *dev,
                                              int32_t *value) {
  return step_dir_stepper_common_get_actual_position(dev, value);
}

static int simple_stepper_set_event_callback(const struct device *dev,
                                             stepper_event_callback_t callback,
                                             void *user_data) {
  struct simple_stepper_data *data = dev->data;

  data->common.callback = callback;
  data->common.event_cb_user_data = user_data;

  return 0;
}

static int
simple_stepper_set_microstep_interval(const struct device *dev,
                                      uint64_t microstep_interval_ns) {
  return step_dir_stepper_common_set_microstep_interval(dev,
                                                        microstep_interval_ns);
}

static int simple_stepper_run(const struct device *dev,
                              enum stepper_direction direction) {
  struct simple_stepper_data *data = dev->data;
  const struct simple_stepper_config *config = dev->config;

  if (data->common.microstep_interval_ns == 0) {
    LOG_ERR("Step interval not set");
    return -EINVAL;
  }

  K_SPINLOCK(&data->common.lock) {
    data->common.run_mode = STEPPER_RUN_MODE_VELOCITY;
    data->common.direction = direction;
    config->common.timing_source->update(dev,
                                         data->common.microstep_interval_ns);
    config->common.timing_source->start(dev);
  }

  return 0;
}

static int simple_stepper_stop(const struct device *dev) {
  struct simple_stepper_data *data = dev->data;
  const struct simple_stepper_config *config = dev->config;

  K_SPINLOCK(&data->common.lock) {
    config->common.timing_source->stop(dev);
    data->common.run_mode = STEPPER_RUN_MODE_HOLD;
  }

  return 0;
}

static int simple_stepper_init(const struct device *dev) {
  const struct simple_stepper_config *config = dev->config;
  struct simple_stepper_data *data = dev->data;
  int ret;

  /* Store device pointer in data */
  data->common.dev = dev;

  /* Initialize step_dir common functionality (step and dir pins) */
  ret = step_dir_stepper_common_init(dev);
  if (ret < 0) {
    LOG_ERR("Failed to initialize step_dir common: %d", ret);
    return ret;
  }

  /* Configure enable pin if present */
  if (config->en_pin.port) {
    if (!gpio_is_ready_dt(&config->en_pin)) {
      LOG_ERR("Enable pin GPIO is not ready");
      return -ENODEV;
    }

    ret = gpio_pin_configure_dt(&config->en_pin, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
      LOG_ERR("Failed to configure enable pin: %d", ret);
      return ret;
    }

    /* Start with stepper disabled */
    /* Normal mode (active low): set to 1 to disable */
    /* Inverted mode (active high): set to 0 to disable */
    int disable_value = config->invert_pins ? 0 : 1;
    ret = gpio_pin_set_dt(&config->en_pin, disable_value);
    if (ret < 0) {
      LOG_ERR("Failed to set enable pin: %d", ret);
      return ret;
    }

    data->enabled = false;
  } else {
    LOG_INF("Enable pin not configured, stepper always enabled");
    data->enabled = true;
  }

  LOG_INF("Simple stepper initialized");

  return 0;
}

/* Stepper API implementation */
static const struct stepper_driver_api simple_stepper_api = {
    .enable = simple_stepper_enable,
    .disable = simple_stepper_disable,
    .move_by = simple_stepper_move_by,
    .move_to = simple_stepper_move_to,
    .set_reference_position = simple_stepper_set_reference_position,
    .get_actual_position = simple_stepper_get_actual_position,
    .set_event_callback = simple_stepper_set_event_callback,
    .set_microstep_interval = simple_stepper_set_microstep_interval,
    .run = simple_stepper_run,
    .stop = simple_stepper_stop,
};

/* Device instantiation macro */
#define SIMPLE_STEPPER_DEVICE(inst)                                            \
  static struct simple_stepper_data simple_stepper_data_##inst = {             \
      .enabled = false,                                                        \
  };                                                                           \
                                                                               \
  static const struct simple_stepper_config simple_stepper_config_##inst = {   \
      .common = STEP_DIR_STEPPER_DT_INST_COMMON_CONFIG_INIT(inst),             \
      .en_pin = GPIO_DT_SPEC_INST_GET_OR(inst, en_gpios, {0}),                 \
      .invert_pins = DT_INST_PROP(inst, invert_pins),                          \
  };                                                                           \
                                                                               \
  DEVICE_DT_INST_DEFINE(inst, simple_stepper_init, NULL,                       \
                        &simple_stepper_data_##inst,                           \
                        &simple_stepper_config_##inst, POST_KERNEL,            \
                        CONFIG_STEPPER_INIT_PRIORITY, &simple_stepper_api);

DT_INST_FOREACH_STATUS_OKAY(SIMPLE_STEPPER_DEVICE)
