# ZephyrRail Wireless - PWA-Controlled Sony Camera Remote

A dual-role Bluetooth Low Energy application for nRF54L15 that:
- **Acts as a Central** to connect to and control Sony cameras via Bluetooth
- **Acts as a Peripheral** exposing a GATT service for Progressive Web App (PWA) control

This enables remote camera control from any Web Bluetooth-capable browser (Chrome, Edge, Opera on desktop/mobile).

## Features

- ✅ **Multi-role BLE**: Simultaneous central (camera) and peripheral (PWA) connections
- ✅ **Web Bluetooth API**: Control from any modern browser without native apps
- ✅ **Secure**: Requires BLE pairing/bonding for both camera and PWA connections
- ✅ **Sony Camera Control**: Full support for shutter, focus, zoom, recording, and custom buttons
- ✅ **Status Notifications**: Real-time status updates to the web app

## Hardware Requirements

- **nRF54L15 board** (e.g., Xiao nRF54L15)
- **Sony Camera** with Bluetooth remote control capability (e.g., α7 series, α9, ZV-E10)

## Building

```bash
cd app_wireless
west build -b xiao_nrf54l15/nrf54l15/cpuapp
west flash
```

## Configuration

### Camera Address

In `src/main.cpp`, update the Sony camera's Bluetooth address:

```cpp
SonyRemote remote("9C:50:D1:AF:76:5F");  // Replace with your camera's address
```

To find your camera's address:
1. Enable "Bluetooth Rmt Ctrl" on the camera
2. Use a BLE scanner app (nRF Connect, LightBlue) to find the camera
3. Copy the MAC address

### Device Name

In `prj.conf`, customize the advertised name:

```ini
CONFIG_BT_DEVICE_NAME="ZephyrRail"
```

## Web Bluetooth API Usage

### HTML Example

```html
<!DOCTYPE html>
<html>
<head>
    <title>Sony Camera Remote</title>
    <style>
        body { font-family: Arial; padding: 20px; }
        button { margin: 5px; padding: 10px 20px; font-size: 16px; }
        #status { 
            margin: 20px 0; 
            padding: 10px; 
            background: #f0f0f0; 
            border-radius: 5px;
            min-height: 100px;
        }
        .connected { background: #d4edda; }
        .disconnected { background: #f8d7da; }
    </style>
</head>
<body>
    <h1>Sony Camera Remote Control</h1>
    
    <button id="connect">Connect to ZephyrRail</button>
    <button id="disconnect" disabled>Disconnect</button>
    
    <div id="status" class="disconnected">
        Not connected
    </div>
    
    <div id="controls" style="display: none;">
        <h2>Camera Controls</h2>
        <button onclick="sendCommand('SHOOT')">📷 Take Photo</button>
        <button onclick="sendCommand('FOCUS_DOWN')">🎯 Focus Half Press</button>
        <button onclick="sendCommand('FOCUS_UP')">🎯 Focus Release</button>
        <br>
        <button onclick="sendCommand('SHUTTER_DOWN')">📸 Shutter Press</button>
        <button onclick="sendCommand('SHUTTER_UP')">📸 Shutter Release</button>
        <button onclick="sendCommand('REC_TOGGLE')">🎥 Record Toggle</button>
        <br>
        <button onmousedown="sendCommand('ZOOM_T_PRESS')" 
                onmouseup="sendCommand('ZOOM_T_RELEASE')">🔍 Zoom In</button>
        <button onmousedown="sendCommand('ZOOM_W_PRESS')" 
                onmouseup="sendCommand('ZOOM_W_RELEASE')">🔍 Zoom Out</button>
        <br>
        <button onclick="sendCommand('C1_DOWN')">C1 Press</button>
        <button onclick="sendCommand('C1_UP')">C1 Release</button>
        <br>
        <button onclick="sendCommand('STATUS')">📊 Check Status</button>
        <button onclick="sendCommand('PING')">🏓 Ping</button>
    </div>

    <script>
        const SERVICE_UUID = '12345634-5678-1234-1234-123456789abc';
        const STATUS_UUID = '12345636-5678-1234-1234-123456789abc';
        const COMMAND_UUID = '12345635-5678-1234-1234-123456789abc';
        
        let device, server, service, commandChar, statusChar;
        
        document.getElementById('connect').addEventListener('click', async () => {
            try {
                updateStatus('Requesting device...', 'disconnected');
                
                device = await navigator.bluetooth.requestDevice({
                    filters: [{ namePrefix: 'ZephyrRail' }],
                    optionalServices: [SERVICE_UUID]
                });
                
                updateStatus('Connecting...', 'disconnected');
                server = await device.gatt.connect();
                
                updateStatus('Discovering services...', 'disconnected');
                service = await server.getPrimaryService(SERVICE_UUID);
                
                commandChar = await service.getCharacteristic(COMMAND_UUID);
                statusChar = await service.getCharacteristic(STATUS_UUID);
                
                // Subscribe to notifications
                await statusChar.startNotifications();
                statusChar.addEventListener('characteristicvaluechanged', (e) => {
                    const value = new TextDecoder().decode(e.target.value);
                    updateStatus('📡 ' + value, 'connected');
                });
                
                device.addEventListener('gattserverdisconnected', () => {
                    updateStatus('Disconnected from device', 'disconnected');
                    document.getElementById('controls').style.display = 'none';
                    document.getElementById('connect').disabled = false;
                    document.getElementById('disconnect').disabled = true;
                });
                
                updateStatus('✅ Connected to ZephyrRail!', 'connected');
                document.getElementById('controls').style.display = 'block';
                document.getElementById('connect').disabled = true;
                document.getElementById('disconnect').disabled = false;
                
            } catch(error) {
                updateStatus('❌ Error: ' + error, 'disconnected');
                console.error(error);
            }
        });
        
        document.getElementById('disconnect').addEventListener('click', () => {
            if (device && device.gatt.connected) {
                device.gatt.disconnect();
            }
        });
        
        async function sendCommand(cmd) {
            if (!commandChar) {
                alert('Not connected!');
                return;
            }
            
            try {
                const encoder = new TextEncoder();
                await commandChar.writeValue(encoder.encode(cmd));
                console.log('Sent command:', cmd);
            } catch(error) {
                console.error('Error sending command:', error);
                updateStatus('❌ Command failed: ' + error, 'connected');
            }
        }
        
        function updateStatus(text, className) {
            const statusDiv = document.getElementById('status');
            const timestamp = new Date().toLocaleTimeString();
            statusDiv.innerHTML = `[${timestamp}] ${text}<br>` + statusDiv.innerHTML;
            statusDiv.className = className;
        }
        
        // Check if Web Bluetooth is supported
        if (!navigator.bluetooth) {
            document.body.innerHTML = 
                '<h1>⚠️ Web Bluetooth Not Supported</h1>' +
                '<p>Please use Chrome, Edge, or Opera browser on desktop or Android.</p>';
        }
    </script>
</body>
</html>
```

### JavaScript-Only Example

```javascript
// Connection
const device = await navigator.bluetooth.requestDevice({
    filters: [{ namePrefix: 'ZephyrRail' }],
    optionalServices: ['12345634-5678-1234-1234-123456789abc']
});

const server = await device.gatt.connect();
const service = await server.getPrimaryService('12345634-5678-1234-1234-123456789abc');

// Get characteristics
const commandChar = await service.getCharacteristic('12345635-5678-1234-1234-123456789abc');
const statusChar = await service.getCharacteristic('12345636-5678-1234-1234-123456789abc');

// Subscribe to notifications
await statusChar.startNotifications();
statusChar.addEventListener('characteristicvaluechanged', (event) => {
    const status = new TextDecoder().decode(event.target.value);
    console.log('Status:', status);
});

// Send commands
function sendCommand(cmd) {
    const encoder = new TextEncoder();
    return commandChar.writeValue(encoder.encode(cmd));
}

// Example usage
await sendCommand('SHOOT');         // Take a photo
await sendCommand('FOCUS_DOWN');    // Half-press shutter (focus)
await sendCommand('FOCUS_UP');      // Release
await sendCommand('REC_TOGGLE');    // Start/stop recording
await sendCommand('STATUS');        // Check camera connection status
```

## Available Commands

### Basic Control
- `PING` - Test connectivity (responds with `PONG`)
- `STATUS` - Check if camera is connected (`CAMERA_READY` or `CAMERA_NOT_READY`)

### Camera Shutter
- `SHOOT` - Take a photo (full press sequence)
- `SHUTTER_DOWN` - Press shutter button
- `SHUTTER_UP` - Release shutter button

### Focus Control
- `FOCUS_DOWN` - Half-press shutter (autofocus)
- `FOCUS_UP` - Release focus

### Video Recording
- `REC_TOGGLE` - Start/stop video recording

### Zoom Control
- `ZOOM_T_PRESS` - Start zooming in (telephoto)
- `ZOOM_T_RELEASE` - Stop zooming in
- `ZOOM_W_PRESS` - Start zooming out (wide)
- `ZOOM_W_RELEASE` - Stop zooming out

### Custom Button
- `C1_DOWN` - Press C1 button
- `C1_UP` - Release C1 button

## Status Notifications

The device sends these status notifications to the PWA:

- `READY` - Service initialized and ready
- `PONG` - Response to PING
- `CAMERA_READY` - Sony camera connected and ready
- `CAMERA_NOT_READY` - Sony camera not connected
- `ACK:<command>` - Command acknowledged and sent to camera
- `ERR:UNKNOWN_CMD` - Unknown command received
- `ERR:CAMERA_NOT_READY` - Command rejected (camera not connected)

## Architecture

```
┌─────────────────────────────────────────┐
│         nRF54L15 Application            │
├─────────────────────────────────────────┤
│                                         │
│  ┌──────────────┐    ┌──────────────┐  │
│  │  Peripheral  │    │   Central    │  │
│  │  (PWA Svc)   │    │ (Sony Cam)   │  │
│  └──────┬───────┘    └───────┬──────┘  │
│         │                    │         │
│    GATT Service         GATT Client    │
│    ┌────┴─────┐         ┌───┴─────┐   │
│    │Command   │         │ Sony    │   │
│    │Status    │         │ Remote  │   │
│    └──────────┘         └─────────┘   │
│         │                    │         │
└─────────┼────────────────────┼─────────┘
          │                    │
          ↓                    ↓
    ┌──────────┐         ┌──────────┐
    │   PWA    │         │  Sony    │
    │ Browser  │         │  Camera  │
    └──────────┘         └──────────┘
```

## Project Structure

```
app_wireless/
├── CMakeLists.txt           # Build configuration
├── prj.conf                 # Zephyr project configuration
├── README.md                # This file
└── src/
    ├── main.cpp             # Main application & connection dispatcher
    ├── pwa_service.h        # PWA GATT service header
    └── pwa_service.cpp      # PWA GATT service implementation
```

## Security

### Pairing Requirements

Both connections require pairing:

1. **Sony Camera**: 
   - Enable "Bluetooth Rmt Ctrl" on camera
   - Device will auto-pair on first connection
   - Bond is stored for automatic reconnection

2. **PWA Connection**:
   - Browser will prompt for pairing
   - All write operations require encryption (BLE Security Level 2)
   - Bond is stored in the nRF54L15's settings subsystem

### Security Configuration

In `prj.conf`:
```ini
CONFIG_BT_SMP=y                  # Security Manager Protocol
CONFIG_BT_BONDABLE=y             # Allow bonding
CONFIG_BT_SMP_SC_PAIR_ONLY=y     # Require Secure Connections
CONFIG_BT_SETTINGS=y             # Persist bonds
```

## Troubleshooting

### Web Bluetooth Issues

**Problem**: "Web Bluetooth is not supported"
- **Solution**: Use Chrome, Edge, or Opera browser (not Firefox or Safari)
- On iOS: Web Bluetooth is not supported at all
- On desktop: Ensure Bluetooth is enabled in system settings

**Problem**: Device not found during scanning
- **Solution**: 
  - Check that the nRF54L15 is powered and running
  - View serial console to confirm advertising started
  - Try restarting the device
  - Ensure device name filter matches `CONFIG_BT_DEVICE_NAME`

**Problem**: Connection fails immediately
- **Solution**:
  - Clear browser's Bluetooth cache (go to `chrome://bluetooth-internals`)
  - Remove old pairing and re-pair
  - Check that only one connection is active (max 2 total: PWA + camera)

### Camera Connection Issues

**Problem**: Camera not connecting
- **Solution**:
  - Enable "Bluetooth Rmt Ctrl" in camera settings
  - Ensure camera MAC address in `main.cpp` is correct
  - Try deleting existing pairing from camera menu
  - Check serial console for connection status

**Problem**: Commands don't work
- **Solution**:
  - Check `STATUS` command response
  - Ensure camera is in correct mode (not in menu, playback, etc.)
  - Some commands may not work in all camera modes
  - View serial logs for detailed error messages

### Build Issues

**Problem**: Compilation errors
- **Solution**:
  - Ensure you're using Zephyr SDK 0.16+ 
  - Clean build: `rm -rf build && west build`
  - Check that `sony_remote` library is present in `../lib/`

**Problem**: Out of memory errors
- **Solution**:
  - The nRF54L15 has limited RAM
  - Reduce `CONFIG_HEAP_MEM_POOL_SIZE` if needed
  - Adjust buffer counts in `prj.conf`

## Performance Notes

### Connection Intervals

- **PWA connection**: 30-50ms (for responsive UI control)
- **Camera connection**: Per Sony specification (typically 30-45ms)

### Latency

- Command latency: ~50-150ms from web button click to camera action
- Status notification latency: ~10-50ms

### Power Consumption

The device maintains two active BLE connections, which increases power consumption:
- Advertising: ~5mA
- Connected (2 links): ~15-25mA average
- Consider adding sleep modes if battery-powered

## Extending the Project

### Adding New Commands

1. Add the command handler in `pwa_service.cpp`:
```cpp
} else if (strcmp(cmd, "NEW_COMMAND") == 0) {
    // Your logic here
    notifyStatus("ACK:NEW_COMMAND");
}
```

2. Use it from the PWA:
```javascript
await sendCommand('NEW_COMMAND');
```

### Modifying Service UUIDs

In `pwa_service.cpp`, change these constants:
```cpp
#define PWA_SERVICE_UUID  BT_UUID_DECLARE_128(...)
#define PWA_CMD_UUID      BT_UUID_DECLARE_128(...)
#define PWA_STATUS_UUID   BT_UUID_DECLARE_128(...)
```

Update the JavaScript to match.

## Resources

- [Web Bluetooth API Specification](https://webbluetoothcg.github.io/web-bluetooth/)
- [Sony Camera BLE Protocol](https://gregleeds.com/reverse-engineering-sony-camera-bluetooth/)
- [Zephyr Bluetooth Documentation](https://docs.zephyrproject.org/latest/connectivity/bluetooth/index.html)
- [nRF54L15 Documentation](https://docs.nordicsemi.com/bundle/nrf54l15_dk/page/index.html)

## License

This project is based on the Zephyr Project and follows its Apache 2.0 license.

## Credits

- Sony Remote library inspired by [Greg Leeds' reverse engineering work](https://gregleeds.com/reverse-engineering-sony-camera-bluetooth/)
- PWA service design based on best practices from the Web Bluetooth Community Group
