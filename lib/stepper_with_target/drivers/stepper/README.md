# Simple Stepper Motor Driver

This is a minimal stepper motor driver implementation for basic step/direction stepper motor controllers with an enable pin.

## Features

- **Three GPIO pins control**: step, direction, and enable
- **Leverages Zephyr's step_dir common infrastructure** for timing and stepping logic
- **Simple and minimal**: No complex microstep configuration or additional features
- **Compatible with common stepper drivers** like A4988, DRV8825, TMC2208, etc.

## Hardware Requirements

The driver requires three GPIO pins:

1. **STEP pin**: Generates pulses to step the motor (GPIO_ACTIVE_HIGH)
2. **DIR pin**: Controls direction (HIGH = positive, LOW = negative)
3. **EN pin**: Enables/disables the motor driver (typically GPIO_ACTIVE_LOW)

## Device Tree Configuration

Add the stepper motor node to your device tree overlay:

```dts
/ {
    stepper_motor: stepper_motor {
        compatible = "simple-stepper";
        status = "okay";
        step-gpios = <&gpio1 5 GPIO_ACTIVE_HIGH>;
        dir-gpios = <&gpio1 7 GPIO_ACTIVE_HIGH>;
        en-gpios = <&gpio1 9 GPIO_ACTIVE_LOW>;
        micro-step-res = <16>;
    };
};
```

### Properties

- `step-gpios`: GPIO specification for step signal
- `dir-gpios`: GPIO specification for direction signal
- `en-gpios`: GPIO specification for enable signal (optional)
- `micro-step-res`: Microstep resolution (default: 1) - this is informational for the application
- `invert-direction`: Invert motor direction (optional, default: false)
- `counter`: Counter device for hardware timing (optional, uses work queue if not specified)

## Kconfig

Enable the driver in your `prj.conf`:

```
CONFIG_STEPPER=y
CONFIG_SIMPLE_STEPPER=y
CONFIG_GPIO=y
```

## Usage Example

```cpp
#include <zephyr/drivers/stepper.h>

#define STEPPER_NODE DT_NODELABEL(stepper_motor)

int main(void) {
    const struct device *stepper_dev = DEVICE_DT_GET(STEPPER_NODE);
    
    if (!device_is_ready(stepper_dev)) {
        printk("Stepper device not ready\n");
        return -1;
    }
    
    // Enable the stepper motor
    stepper_enable(stepper_dev);
    
    // Set step interval (e.g., 1ms between steps)
    stepper_set_microstep_interval(stepper_dev, 1000000); // 1ms in nanoseconds
    
    // Move 200 steps forward
    stepper_move_by(stepper_dev, 200);
    
    // Wait for movement to complete
    k_sleep(K_SECONDS(1));
    
    // Move 200 steps backward
    stepper_move_by(stepper_dev, -200);
    
    return 0;
}
```

## Implementation Details

The driver uses Zephyr's `step_dir_stepper_common` infrastructure which provides:
- Position tracking
- Velocity and position mode support
- Event callbacks
- Timing via work queue or hardware counter

The driver implements the minimal Zephyr stepper API:
- `stepper_enable()` / `stepper_disable()`: Control the enable pin
- `stepper_move_by()`: Relative movement
- `stepper_move_to()`: Absolute positioning
- `stepper_set_microstep_interval()`: Set step timing
- `stepper_run()`: Continuous rotation
- `stepper_stop()`: Stop movement
- `stepper_get_actual_position()`: Read current position
- `stepper_set_reference_position()`: Set position reference

## Differences from ti,drv84xx

The `simple-stepper` driver is much simpler than `ti,drv84xx`:

| Feature | simple-stepper | ti,drv84xx |
|---------|---------------|------------|
| GPIO pins | 3 (step, dir, en) | 7+ (step, dir, en, sleep, fault, m0, m1) |
| Microstep config | Software only | Hardware pins (M0, M1) |
| Fault detection | No | Yes |
| Sleep mode | Via enable pin | Separate sleep pin |
| Complexity | Minimal | Feature-rich |

Use `simple-stepper` when you have a basic stepper driver without configuration pins, or when you don't need the advanced features of the TI DRV84xx family.

## Compatible Hardware

This driver should work with most basic step/direction stepper drivers:
- A4988
- DRV8825
- TMC2208 (UART/standalone mode)
- TMC2209 (UART/standalone mode)
- TB67S128FTG
- Any other step/dir/enable driver

## Building

The driver is automatically included when you add it to your project and enable it in Kconfig.

Make sure your project CMakeLists.txt includes the drivers subdirectory:

```cmake
add_subdirectory(drivers)
```

## License

SPDX-License-Identifier: Apache-2.0
