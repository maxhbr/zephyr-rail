const SERVICE_UUID = '12345634-5678-1234-1234-123456789abc';
const STATUS_UUID = '12345636-5678-1234-1234-123456789abc';
const COMMAND_UUID = '12345635-5678-1234-1234-123456789abc';
const STORAGE_PREFIX = 'zephyrRail.';
const DEBUG_MODE = window.location.hash.toLowerCase() === '#debug';
const PERSISTED_FIELDS = [
  'go-distance', 'goto-position', 'bound', 'wait-before', 'wait-after',
  'stack-length', 'speed-rpm'
];

let device, server, service, commandChar, statusChar;
let connectionIndicatorEl, connectionLabelEl;
let tabButtons = [];
let tabPanels = [];
let activeTabId = 'tab-connection';
const bluetoothSupported = !!navigator.bluetooth;

// Check Web Bluetooth support
if (!bluetoothSupported && !DEBUG_MODE) {
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
  if (!bluetoothSupported && !DEBUG_MODE) {
    return;
  }

  connectionIndicatorEl = document.getElementById('connection-indicator');
  connectionLabelEl = document.getElementById('connection-label');

  // Setup collapsible sections
  const collapsibleHeaders =
      document.querySelectorAll('[data-collapsible-target]');
  collapsibleHeaders.forEach((header) => {
    const targetSelectors =
        (header.getAttribute('data-collapsible-target') || '')
            .split(',')
            .map((selector) => selector.trim())
            .filter((selector) => selector.length > 0);
    if (!targetSelectors.length) {
      return;
    }
    const targetElements =
        targetSelectors.map((selector) => document.querySelector(selector))
            .filter((element) => !!element);
    if (!targetElements.length) {
      return;
    }

    header.addEventListener('click', () => {
      header.classList.toggle('collapsed');
      targetElements.forEach(
          (content) => { content.classList.toggle('collapsed'); });
    });
  });

  restorePersistedInputs();
  setupSlider();
  setupCustomCommandInput();
  setupTabs();

  const connectButton = document.getElementById('connect');
  const pairButton = document.getElementById('pair-camera');
  const disconnectButton = document.getElementById('disconnect');

  toggleControls(false);
  setConnectionState('disconnected', 'Not connected');

  if (DEBUG_MODE) {
    updateStatus('Debug mode: controls forced visible', 'connected');
  }

  if (pairButton) {
    pairButton.addEventListener('click', () => sendCommand('PAIR'));
  }

  if (!bluetoothSupported && connectButton) {
    connectButton.disabled = true;
    connectButton.title = 'Web Bluetooth not supported in this browser';
  }

  connectButton && bluetoothSupported &&
      connectButton.addEventListener('click', async () => {
        try {
          setConnectionState('connecting', 'Searching for ZephyrRail...');
          updateStatus('Searching for ZephyrRail device...', 'disconnected');

          // Request device
          device = await navigator.bluetooth.requestDevice({
            filters : [ {namePrefix : 'ZephyrRail'} ],
            optionalServices : [ SERVICE_UUID ]
          });
          console.log('Device selected:', device);

          setConnectionState('connecting',
                             'Connecting to ' + device.name + '...');
          updateStatus('Connecting to ' + device.name + '...', 'connecting');

          // Connect to GATT server
          server = await device.gatt.connect();
          console.log('Connected to GATT server:', server);

          // Get primary service
          service = await server.getPrimaryService(SERVICE_UUID);
          console.log('Got service:', service);

          // Get characteristics
          commandChar = await service.getCharacteristic(COMMAND_UUID);
          console.log('Got command characteristic:', commandChar);
          statusChar = await service.getCharacteristic(STATUS_UUID);
          console.log('Got status characteristic:', statusChar);

          // Subscribe to notifications
          await statusChar.startNotifications();
          statusChar.addEventListener('characteristicvaluechanged',
                                      handleStatusNotification);

          // Handle disconnection
          device.addEventListener('gattserverdisconnected',
                                  handleDisconnection);

          updateStatus('Connected to ' + device.name + '!', 'connected');
          setConnectionState('connected', 'Connected to ' + device.name);
          toggleControls(true);

        } catch (error) {
          handleError(error);
        }
      });

  disconnectButton && disconnectButton.addEventListener('click', () => {
    if (device && device.gatt.connected) {
      device.gatt.disconnect();
    }
  });
});

function handleStatusNotification(event) {
  const value = new TextDecoder().decode(event.target.value);
  const timestamp = new Date().toLocaleTimeString();

  updateStatus(`[${timestamp}] ${value}`, 'connected');
}

function handleDisconnection() {
  updateStatus('Disconnected from device', 'disconnected');
  setConnectionState('disconnected', 'Not connected');
  toggleControls(false);

  // Reset variables
  device = null;
  server = null;
  service = null;
  commandChar = null;
  statusChar = null;
}

function handleError(error) {
  console.error('Bluetooth Error:', error);
  updateStatus('Error: ' + error, 'disconnected');
  setConnectionState('disconnected', 'Error while connecting');
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
  const distance = readNumber('go-distance');
  sendCommand('GO ' + Math.ceil(distance * times * 1000));
}

function sendGoPct(pct) { sendCommand('GO_PCT ' + pct); }

function sendGoTo() {
  const position = readNumber('goto-position');
  sendCommand('GO_TO ' + Math.ceil(position * 1000));
}

function sendSetLowerBound() {
  const value = readNumber('bound');
  sendCommand('SET_LOWER_BOUND ' + Math.ceil(value * 1000));
}

function sendSetUpperBound() {
  const value = readNumber('bound');
  sendCommand('SET_UPPER_BOUND ' + Math.ceil(value * 1000));
}

function sendSetWaitBefore() {
  const value = Math.max(0, readNumber('wait-before'));
  sendCommand('SET_WAIT_BEFORE ' + Math.round(value));
}

function sendSetWaitAfter() {
  const value = Math.max(0, readNumber('wait-after'));
  sendCommand('SET_WAIT_AFTER ' + Math.round(value));
}

function sendWaitSettings() {
  sendSetWaitBefore();
  sendSetWaitAfter();
}

function sendSetSpeedPreset(preset) {
  if (!preset) {
    return;
  }
  sendCommand('SET_SPEED ' + String(preset).toUpperCase());
}

function sendSetSpeedRpm() {
  const rpm = Math.max(1, Math.round(readNumber('speed-rpm', 1)));
  sendCommand('SET_SPEED_RPM ' + rpm);
}

function sendStartStack(expected_step_size_nm) {
  sendWaitSettings();
  sendCommand('START_STACK ' + expected_step_size_nm);
}

function sendStartStackCount(length) {
  const stackLength = length !== undefined
                          ? length
                          : Math.max(1, readNumber('stack-length', 1));
  sendWaitSettings();
  sendCommand('START_STACK_COUNT ' + Math.round(stackLength));
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

function setConnectionState(state, labelText) {
  if (connectionIndicatorEl) {
    connectionIndicatorEl.classList.remove('connected', 'connecting',
                                           'disconnected');
    connectionIndicatorEl.classList.add(state);
  }

  if (connectionLabelEl && labelText) {
    connectionLabelEl.textContent = labelText;
  }

  const connectButton = document.getElementById('connect');
  if (connectButton) {
    const shouldDisable =
        state === 'connecting' || (state === 'connected' && !DEBUG_MODE);
    connectButton.disabled = shouldDisable;
  }
}

function toggleControls(isConnected) {
  const allowExtended = isConnected || DEBUG_MODE;
  setTabAvailability(allowExtended);

  const setupControls = document.getElementById('controls-setup');
  const disconnectButton = document.getElementById('disconnect');
  const pairButton = document.getElementById('pair-camera');

  if (setupControls) {
    setupControls.style.display = allowExtended ? 'block' : 'none';
  }
  if (disconnectButton) {
    disconnectButton.disabled = !isConnected;
  }
  if (pairButton) {
    pairButton.disabled = !isConnected;
  }
}

function restorePersistedInputs() {
  if (!supportsLocalStorage()) {
    return;
  }

  PERSISTED_FIELDS.forEach((id) => {
    const element = document.getElementById(id);
    if (!element) {
      return;
    }

    const storedValue = localStorage.getItem(STORAGE_PREFIX + id);
    if (storedValue !== null) {
      element.value = storedValue;
    }

    const eventName = element.tagName === 'SELECT' ? 'change' : 'input';
    element.addEventListener(
        eventName,
        () => { localStorage.setItem(STORAGE_PREFIX + id, element.value); });
  });
}

function supportsLocalStorage() {
  try {
    const key = STORAGE_PREFIX + 'probe';
    localStorage.setItem(key, '1');
    localStorage.removeItem(key);
    return true;
  } catch (err) {
    console.warn('LocalStorage disabled, values will not persist', err);
    return false;
  }
}

function setupSlider() {
  const slider = document.getElementById('go-slider');
  if (!slider) {
    return;
  }

  const valueLabel = document.getElementById('go-slider-value');
  const tickButtons = document.querySelectorAll('.slider-tick');
  const min = Number(slider.min || 0);
  const max = Number(slider.max || 100);
  const sliderRange = Math.max(max - min, 1);
  const throttledSend = throttle((value) => sendGoPct(value), 150);

  const formatPercent = (value) => `${value}%`;
  const clampValue = (value) => {
    const numericValue = Number(value);
    if (!Number.isFinite(numericValue)) {
      return min;
    }
    return Math.min(max, Math.max(min, Math.round(numericValue)));
  };

  const updateSliderVisuals = (value) => {
    if (valueLabel) {
      valueLabel.textContent = formatPercent(value);
    }

    const percentage = ((value - min) / sliderRange) * 100;
    slider.style.setProperty('--slider-progress', `${percentage}%`);
    slider.style.background =
        `linear-gradient(90deg, var(--slider-fill, #4299e1) 0%, var(--slider-fill, #4299e1) ${
            percentage}%, var(--slider-bg, #e2e8f0) ${
            percentage}%, var(--slider-bg, #e2e8f0) 100%)`;
  };

  const applyValue = (value, shouldSendCommand = true) => {
    const clampedValue = clampValue(value);
    slider.value = clampedValue;
    updateSliderVisuals(clampedValue);
    if (shouldSendCommand) {
      throttledSend(clampedValue);
    }
  };

  slider.addEventListener('input',
                          (event) => { applyValue(event.target.value); });

  tickButtons.forEach((button) => {
    button.addEventListener('click', () => {
      const targetValue = button.dataset.value;
      applyValue(targetValue);
      button.blur();
    });
  });

  applyValue(slider.value, false);
}

function setupCustomCommandInput() {
  const customInput = document.getElementById('custom-command-input');
  const sendButton = document.getElementById('send-custom-command');
  if (!customInput || !sendButton) {
    return;
  }

  const send = () => {
    const value = customInput.value.trim();
    if (value.length === 0) {
      return;
    }
    sendCommand(value.toUpperCase());
    customInput.value = '';
  };

  sendButton.addEventListener('click', send);
  customInput.addEventListener('keydown', (event) => {
    if (event.key === 'Enter') {
      event.preventDefault();
      send();
    }
  });
}

function setupTabs() {
  tabButtons = Array.from(document.querySelectorAll('.tab-button'));
  tabPanels = Array.from(document.querySelectorAll('.tab-panel'));

  if (!tabButtons.length || !tabPanels.length) {
    return;
  }

  tabButtons.forEach((button) => {
    button.addEventListener('click', () => {
      if (button.disabled) {
        return;
      }
      activateTab(button.dataset.tabTarget);
    });
  });

  activateTab(activeTabId);
}

function activateTab(targetId) {
  if (!targetId) {
    return;
  }
  activeTabId = targetId;
  tabButtons.forEach((button) => {
    const isActive = button.dataset.tabTarget === targetId;
    button.classList.toggle('active', isActive);
  });
  tabPanels.forEach(
      (panel) => { panel.classList.toggle('active', panel.id === targetId); });
}

function setTabAvailability(enableConnectedTabs) {
  if (!tabButtons.length) {
    return;
  }
  let shouldReset = false;
  tabButtons.forEach((button) => {
    const requiresConnection = button.dataset.requiresConnection === 'true';
    if (!requiresConnection) {
      return;
    }
    button.disabled = !enableConnectedTabs;
    if (!enableConnectedTabs && button.dataset.tabTarget === activeTabId) {
      shouldReset = true;
    }
  });
  if (shouldReset) {
    activateTab('tab-connection');
  }
}

function throttle(fn, delay) {
  let lastCall = 0;
  let timeoutId;
  return (...args) => {
    const now = Date.now();
    const remaining = delay - (now - lastCall);

    if (remaining <= 0) {
      if (timeoutId) {
        clearTimeout(timeoutId);
        timeoutId = null;
      }
      lastCall = now;
      fn(...args);
    } else if (!timeoutId) {
      timeoutId = setTimeout(() => {
        lastCall = Date.now();
        timeoutId = null;
        fn(...args);
      }, remaining);
    }
  };
}

function readNumber(id, defaultValue = 0) {
  const element = document.getElementById(id);
  if (!element) {
    return defaultValue;
  }
  const parsed = Number(element.value);
  return Number.isFinite(parsed) ? parsed : defaultValue;
}

// Log to console for debugging
console.log('ZephyrRail Focus Stacking Control');
console.log('Service UUID:', SERVICE_UUID);
console.log('Command UUID:', COMMAND_UUID);
console.log('Status UUID:', STATUS_UUID);
