# Sony Remote Library

This library provides Sony camera remote control functionality via Bluetooth for Zephyr RTOS.

## Usage

Include the header:
```cpp
#include "sony_remote/sony_remote.h"
```

Basic usage:
```cpp
// Initialize Bluetooth first
bt_enable(nullptr);

// Create and initialize Sony Remote (scans for any camera)
SonyRemote remote;
remote.begin();
remote.startScan();

// OR: Create with specific camera address (recommended)
SonyRemote remote("9C:50:D1:AF:76:5F")// ;
remote.begin();
remote.startScan();

// Wait for connection and take a photo
if (remote.ready()) {
    remote.focusDown();
    k_msleep(100);
    remote.shutterDown();
    k_msleep(100);
    remote.shutterUp();
    k_msleep(100);
    remote.focusUp();
}
```

## Building

Add this library to your CMakeLists.txt:
```cmake
add_subdirectory(path/to/lib/sony_remote ${CMAKE_CURRENT_BINARY_DIR}/sony_remote)
target_link_libraries(app PRIVATE sony_remote)
```

## Requirements

- Bluetooth support (`CONFIG_BT=y`)
- Bluetooth Central role (`CONFIG_BT_CENTRAL=y`)
- GATT Client (`CONFIG_BT_GATT_CLIENT=y`)

## Examples

See `../app_only_sony_remote/` for a minimal working example.