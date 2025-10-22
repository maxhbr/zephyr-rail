# ZephyrRail PWA Control

This application now includes Web Bluetooth Progressive Web App (PWA) control in addition to the UART shell interface.

## Features

- **Dual Interface**: Control the rail via UART shell OR Web Bluetooth PWA
- **Web Bluetooth**: Connect from any compatible browser without cables
- **Focus Stacking**: Automated focus stacking with configurable parameters
- **Sony Camera Control**: Integrated Sony camera remote control via Bluetooth
- **Real-time Status**: Live feedback from the device to the web interface

## How to Use the PWA

### 1. Flash the Firmware

Build and flash the application to your nRF54L15 board:

```bash
cd app
west build -b xiao_nrf54l15/nrf54l15/cpuapp
west flash
```

### 2. Open the Web Interface

Open `web_interface.html` in a compatible browser:
- Chrome/Edge/Opera (Windows, macOS, Linux, Android)
- Samsung Internet (Android)

**Note**: Firefox, Safari, and iOS browsers do NOT support Web Bluetooth.

### 3. Connect

1. Click "Connect to ZephyrRail"
2. Select your device from the Bluetooth pairing dialog
3. The device will appear as "ZephyrRail"
4. Once connected, control interface will appear

## Available Commands

### Position Control
- **GO** `<distance>` - Move relative distance in steps
- **GO_TO** `<position>` - Move to absolute position
- **NOOP** - No operation (idle state)

### Bounds & Limits
- **SET_LOWER_BOUND** - Set lower bound to current position
- **SET_UPPER_BOUND** - Set upper bound to current position
- **SET_LOWER_BOUND** `<position>` - Set lower bound to specific position
- **SET_UPPER_BOUND** `<position>` - Set upper bound to specific position

### Timing Settings
- **SET_WAIT_BEFORE** `<milliseconds>` - Delay before taking photo
- **SET_WAIT_AFTER** `<milliseconds>` - Delay after taking photo

### Focus Stacking
- **START_STACK** `<expected_length>` - Start automated focus stacking
- **START_STACK** `<expected_length> <lower> <upper>` - Start with specific bounds

### Camera Control
- **SHOOT** - Trigger camera to take a photo

### System
- **PING** - Test connection (responds with PONG)
- **STATUS** - Get current system status

## Architecture

### Bluetooth Roles

The device operates in **dual role**:

1. **Central (Client)**: Connects to Sony camera for remote control
2. **Peripheral (Server)**: Advertises PWA service for web control

### Connection Handling

The connection callbacks in `main.cpp` automatically route connections:

```cpp
static void conn_connected(struct bt_conn *conn, uint8_t err) {
  struct bt_conn_info info;
  bt_conn_get_info(conn, &info);

  if (info.role == BT_CONN_ROLE_PERIPHERAL) {
    // PWA connection
    PwaService::onConnected(conn, err);
  } else {
    // Sony camera connection
    SonyRemote::on_connected(conn, err);
  }
}
```

### Command Flow

1. Web interface sends command via BLE characteristic write
2. `PwaService::cmdWrite()` receives command
3. Command is parsed and translated to `event_pub()` call
4. StateMachine processes event
5. Status updates are sent back via BLE notifications

## GATT Service Definition

**Service UUID**: `12345634-5678-1234-1234-123456789abc`

**Characteristics**:
- **Command** (Write): `12345635-5678-1234-1234-123456789abc`
  - Write commands to control the device
  - Requires encryption (BLE pairing)
  
- **Status** (Read + Notify): `12345636-5678-1234-1234-123456789abc`
  - Read current status
  - Subscribe for real-time notifications
  - Requires encryption (BLE pairing)

## Security

- **Encryption Required**: All characteristics require BLE pairing
- **Secure Simple Pairing**: Uses BT_SMP_SC_PAIR_ONLY
- **Bondable**: Device can remember paired devices (if CONFIG_BT_SETTINGS enabled)

## Configuration

Key configuration options in `prj.conf`:

```ini
# Enable both central and peripheral roles
CONFIG_BT_CENTRAL=y
CONFIG_BT_PERIPHERAL=y

# Support 2 simultaneous connections (PWA + Sony camera)
CONFIG_BT_MAX_CONN=2

# Security
CONFIG_BT_SMP=y
CONFIG_BT_BONDABLE=y
CONFIG_BT_SMP_SC_PAIR_ONLY=y
```

## Development

### File Structure

```
app/
├── src/
│   ├── main.cpp           # Main application, BT initialization
│   ├── pwa_service.cpp    # PWA GATT service implementation
│   ├── pwa_service.h      # PWA service interface
│   ├── StateMachine.cpp   # State machine for rail control
│   └── StateMachine.h     # State machine header
├── web_interface.html     # PWA web interface
├── prj.conf              # Zephyr configuration
└── CMakeLists.txt        # Build configuration
```

### Adding New Commands

1. Add event to `enum event` in `StateMachine.h`
2. Handle event in `StateMachine.cpp`
3. Add command parsing in `pwa_service.cpp` `cmdWrite()`
4. Add button/control to `web_interface.html`

## Troubleshooting

### Can't Connect from Browser
- Ensure device is powered on and advertising
- Check browser compatibility (Chrome/Edge/Opera only)
- Try disabling/re-enabling Bluetooth on your device
- Clear browser cache and reload page

### Connection Drops
- Ensure device is within range (~10m)
- Check for Bluetooth interference
- Verify battery/power supply is sufficient

### Commands Not Working
- Verify pairing/bonding completed successfully
- Check device logs via UART for error messages
- Ensure shell commands work via UART first

## UART Shell (Alternative Interface)

All commands are also available via UART shell:

```bash
rail go 100
rail go_to 0
rail lower -1000
rail upper 1000
rail stack 50
rail shoot
```

Use `minicom`, `screen`, or VS Code serial monitor to access the shell.

## License

See main project LICENSE file.
