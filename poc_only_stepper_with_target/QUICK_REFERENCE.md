# Simple Stepper Driver - Quick Reference

## Device Tree

```dts
stepper_motor: stepper_motor {
    compatible = "simple-stepper";
    status = "okay";
    step-gpios = <&gpio1 5 GPIO_ACTIVE_HIGH>;
    dir-gpios = <&gpio1 7 GPIO_ACTIVE_HIGH>;
    en-gpios = <&gpio1 9 GPIO_ACTIVE_LOW>;
    micro-step-res = <16>;
    /* Optional: */
    /* invert-direction; */
    /* counter = <&counter2>; */
};
```

## Configuration

```properties
CONFIG_STEPPER=y
CONFIG_SIMPLE_STEPPER=y
CONFIG_GPIO=y
```

## API Usage

```c
#include <zephyr/drivers/stepper.h>

const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(stepper_motor));

// Enable/disable
stepper_enable(dev);
stepper_disable(dev);

// Set timing (nanoseconds between microsteps)
stepper_set_microstep_interval(dev, 1000000);  // 1ms = 1000 steps/sec

// Move relative
stepper_move_by(dev, 200);    // 200 steps forward
stepper_move_by(dev, -200);   // 200 steps backward

// Move absolute
stepper_set_reference_position(dev, 0);  // Set current as zero
stepper_move_to(dev, 1000);              // Move to position 1000

// Continuous rotation
stepper_run(dev, STEPPER_DIRECTION_POSITIVE);
stepper_run(dev, STEPPER_DIRECTION_NEGATIVE);
stepper_stop(dev);

// Get position
int32_t pos;
stepper_get_actual_position(dev, &pos);

// Event callback
void callback(const struct device *dev, enum stepper_event event, void *data) {
    if (event == STEPPER_EVENT_STEPS_COMPLETED) {
        printk("Movement complete!\n");
    }
}
stepper_set_event_callback(dev, callback, NULL);
```

## Build Commands

```bash
# Build
west build -b nrf54l15dk/nrf54l15/cpuapp

# Clean build
west build -b nrf54l15dk/nrf54l15/cpuapp --pristine

# Flash
west flash

# Build for different board
west build -b nrf5340dk/nrf5340/cpuapp
```

## Files Created

```
drivers/stepper/simple_stepper.c        # Driver implementation
drivers/stepper/Kconfig                 # CONFIG_SIMPLE_STEPPER
drivers/stepper/CMakeLists.txt          # Build configuration
dts/bindings/stepper/simple-stepper.yaml  # DT binding
```

## Compatible Hardware

- A4988
- DRV8825
- TMC2208/TMC2209
- TB67S128FTG
- Any step/dir/enable driver

## Key Features

✅ 3 GPIO pins (step, dir, enable)
✅ Standard Zephyr stepper API
✅ Position tracking
✅ Software or hardware timing
✅ Event callbacks
✅ Direction inversion
✅ Optional enable pin
