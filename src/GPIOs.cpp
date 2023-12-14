#include "GPIOs.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpios);

GPIO::GPIO(const struct gpio_dt_spec *spec, gpio_flags_t extra_flags)
{
  LOG_MODULE_DECLARE(gpios);
	int ret;

	if (!gpio_is_ready_dt(spec)) {
    LOG_ERR("gpio %s not ready", spec->port);
    return;
	}

	ret = gpio_pin_configure_dt(spec, extra_flags);
	if (ret < 0) {
    LOG_ERR("Failed to configure gpio %s (err %i)", spec->port, ret);
    return;
	}
}
void GPIO::set(bool value)
{
  LOG_MODULE_DECLARE(gpios);
  if (ret == 0)
  {
    if (value != cur_value)
    {
      cur_value = value;

      gpio_pin_set(dev, pin, value);
    }
  }
  else
  {
    LOG_WRN("Do nothing, since ret=%i", ret);
  }
}
