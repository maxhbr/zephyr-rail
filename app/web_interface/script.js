const SERVICE_UUID = '12345634-5678-1234-1234-123456789abc';
const STATUS_UUID = '12345636-5678-1234-1234-123456789abc';
const COMMAND_UUID = '12345635-5678-1234-1234-123456789abc';

let device, server, service, commandChar, statusChar;

// Check Web Bluetooth support
if (!navigator.bluetooth) {
  document.body.innerHTML = `
        <div class="container">
            <div class="warning">
                <h2>Web Bluetooth Not Supported</h2>
                <p>Your browser doesn't support Web Bluetooth API.</p>
                <p><strong>Supported browsers:</strong></p>
                <ul>
                    <li>Chrome/Edge/Opera on Windows, macOS, Linux, Android</li>
                    <li>Samsung Internet on Android</li>
                </ul>
                <p><strong>Not supported:</strong> Firefox, Safari, iOS browsers</p>
            </div>
        </div>
    `;
}

document.addEventListener('DOMContentLoaded', function() {
  // Setup collapsible section
  const setupHeader = document.getElementById('setup-header');
  const setupContent = document.getElementById('setup-content');

  if (setupHeader && setupContent) {
    setupHeader.addEventListener('click', function() {
      setupHeader.classList.toggle('collapsed');
      setupContent.classList.toggle('collapsed');
    });
  }

  document.getElementById('connect').addEventListener('click', async () => {
    try {
      updateStatus('Searching for ZephyrRail device...', 'disconnected');

      // Request device
      device = await navigator.bluetooth.requestDevice({
        filters : [ {namePrefix : 'ZephyrRail'} ],
        optionalServices : [ SERVICE_UUID ]
      });

      updateStatus('Connecting to ' + device.name + '...', 'disconnected');

      // Connect to GATT server
      server = await device.gatt.connect();

      updateStatus('Discovering services...', 'disconnected');

      // Get primary service
      service = await server.getPrimaryService(SERVICE_UUID);

      // Get characteristics
      commandChar = await service.getCharacteristic(COMMAND_UUID);
      statusChar = await service.getCharacteristic(STATUS_UUID);

      // Subscribe to notifications
      await statusChar.startNotifications();
      statusChar.addEventListener('characteristicvaluechanged',
                                  handleStatusNotification);

      // Handle disconnection
      device.addEventListener('gattserverdisconnected', handleDisconnection);

      updateStatus('Connected to ' + device.name + '!', 'connected');
      document.getElementById('controls').style.display = 'block';
      document.getElementById('controls-setup').style.display = 'block';
      document.getElementById('connect').disabled = true;
      document.getElementById('disconnect').disabled = false;

    } catch (error) {
      handleError(error);
    }
  });

  document.getElementById('disconnect').addEventListener('click', () => {
    if (device && device.gatt.connected) {
      device.gatt.disconnect();
    }
  });
});

function handleStatusNotification(event) {
  const value = new TextDecoder().decode(event.target.value);
  const timestamp = new Date().toLocaleTimeString();

  // Parse status messages
  let icon = '';
  if (value.startsWith('ACK:'))
    icon = '[ACK]';
  if (value.startsWith('ERR:'))
    icon = '[ERR]';
  if (value === 'PONG')
    icon = '[PONG]';
  if (value === 'READY')
    icon = '[READY]';

  updateStatus(`[${timestamp}] ${icon} ${value}`, 'connected');
}

function handleDisconnection() {
  updateStatus('Disconnected from device', 'disconnected');
  document.getElementById('controls').style.display = 'none';
  document.getElementById('controls-setup').style.display = 'none';
  document.getElementById('connect').disabled = false;
  document.getElementById('disconnect').disabled = true;

  // Reset variables
  device = null;
  server = null;
  service = null;
  commandChar = null;
  statusChar = null;
}

function handleError(error) {
  console.error('Bluetooth Error:', error);

  let message = error.message || error.toString();

  // User-friendly error messages
  if (message.includes('User cancelled')) {
    message = 'Connection cancelled by user';
  } else if (message.includes('not found')) {
    message =
        'Device not found. Make sure ZephyrRail is powered on and advertising.';
  } else if (message.includes('GATT')) {
    message = 'Connection failed. Try restarting the device.';
  }

  updateStatus('Error: ' + message, 'disconnected');
}

async function sendCommand(cmd) {
  if (!commandChar) {
    alert('Not connected! Please connect first.');
    return;
  }

  try {
    const encoder = new TextEncoder();
    await commandChar.writeValue(encoder.encode(cmd));
    console.log('Command sent:', cmd);
  } catch (error) {
    console.error('Command failed:', error);
    updateStatus('Failed to send command: ' + cmd, 'connected');
  }
}

// Helper functions for commands with parameters
function sendGo(times) {
  const distance = document.getElementById('go-distance').value;
  sendCommand('GO ' + distance * times);
}

function sendGoPct(pct) { sendCommand('GO_PCT ' + pct); }

function sendGoTo() {
  const position = document.getElementById('goto-position').value;
  sendCommand('GO_TO ' + position);
}

function sendSetLowerBound() {
  const value = document.getElementById('bound').value;
  sendCommand('SET_LOWER_BOUND ' + value);
}

function sendSetUpperBound() {
  const value = document.getElementById('bound').value;
  sendCommand('SET_UPPER_BOUND ' + value);
}

function sendSetWaitBefore() {
  const value = document.getElementById('wait-before').value;
  sendCommand('SET_WAIT_BEFORE ' + value);
}

function sendSetWaitAfter() {
  const value = document.getElementById('wait-after').value;
  sendCommand('SET_WAIT_AFTER ' + value);
}

function sendStartStack() {
  const length = document.getElementById('stack-length').value;
  sendCommand('START_STACK ' + length);
}

function updateStatus(text, className) {
  const statusDiv = document.getElementById('status');
  statusDiv.className = className;

  // Add new message at the top
  const newLine = document.createElement('div');
  newLine.textContent = text;
  statusDiv.insertBefore(newLine, statusDiv.firstChild);

  // Limit to last 20 messages
  while (statusDiv.children.length > 20) {
    statusDiv.removeChild(statusDiv.lastChild);
  }
}

// Log to console for debugging
console.log('ZephyrRail Focus Stacking Control');
console.log('Service UUID:', SERVICE_UUID);
console.log('Command UUID:', COMMAND_UUID);
console.log('Status UUID:', STATUS_UUID);