const SERVICE_UUID = '12345634-5678-1234-1234-123456789abc';
const STATUS_UUID = '12345636-5678-1234-1234-123456789abc';
const COMMAND_UUID = '12345635-5678-1234-1234-123456789abc';
const STORAGE_PREFIX = 'zephyrRail.';
const DEMO_MODE = window.location.hash.toLowerCase() === '#demo';
const PERSISTED_FIELDS = [ 'go-distance', 'wait-before', 'wait-after' ];
const textDecoder = new TextDecoder();

let device, server, service, commandChar, statusChar;
let connectionIndicatorEl, connectionLabelEl;
let tabButtons = [];
let tabPanels = [];
let allTabsButton = null;
let allTabsActive = false;
let activeTabId = 'tab-home';
let railState = {
  position_nm : null,
  target_nm : null,
  lower_nm : null,
  upper_nm : null,
  stack_index : null,
  stack_length : null,
  wait_before_ms : null,
  wait_after_ms : null,
  moving : false,
  last_update : null
};
let railStateEls = {};
const bluetoothSupported = !!navigator.bluetooth;

// Check Web Bluetooth support
if (!bluetoothSupported && !DEMO_MODE) {
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
  if (!bluetoothSupported && !DEMO_MODE) {
    return;
  }

  connectionIndicatorEl = document.getElementById('connection-indicator');
  connectionLabelEl = document.getElementById('connection-label');
  railStateEls = {
    card : document.getElementById('rail-state-card'),
    updated : document.getElementById('rail-state-updated'),
    target : document.getElementById('rail-target-value'),
    lower : document.getElementById('rail-lower-value'),
    upper : document.getElementById('rail-upper-value'),
    stackStep : document.getElementById('rail-stack-step'),
    stackLength : document.getElementById('rail-stack-length'),
    stackStatus : document.getElementById('rail-stack-status'),
    waitBefore : document.getElementById('wait-before-current'),
    waitAfter : document.getElementById('wait-after-current')
  };
  resetRailState();

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

  if (DEMO_MODE) {
    updateStatus('Demo mode: controls forced visible', 'connected');
  }

  if (pairButton) {
    pairButton.addEventListener('click', () => sendCommand('cam pair'));
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
          requestRailStatus();

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
  const value = textDecoder.decode(event.target.value);
  const timestamp = new Date();
  const parsedState = parseRailStateMessage(value);

  if (parsedState) {
    railState =
        Object.assign({}, railState, parsedState, {last_update : timestamp});
    renderRailState();
  }

  updateStatus(`[${timestamp.toLocaleTimeString()}] ${value}`, 'connected');
}

function handleDisconnection() {
  updateStatus('Disconnected from device', 'disconnected');
  setConnectionState('disconnected', 'Not connected');
  toggleControls(false);
  resetRailState();

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

  const trimmed = (cmd || '').toString().trim();
  if (!trimmed.length) {
    return;
  }

  try {
    const encoder = new TextEncoder();
    await commandChar.writeValue(encoder.encode(trimmed));
    console.log('Command sent:', trimmed);
  } catch (error) {
    console.error('Command failed:', error);
    updateStatus('Failed to send command: ' + trimmed, 'connected');
  }
}

function requestRailStatus() { sendCommand('rail status'); }

// Helper functions for commands with parameters
function sendGo(times) {
  const distance = readNumber('go-distance');
  const distanceNm = Math.round(distance * times * 1000);
  sendCommand('rail go_nm ' + distanceNm);
}

function sendGoPct(pct) { sendCommand('rail go_pct ' + pct); }

function sendGoTo() {
  const position = readNumber('goto-position');
  sendCommand('rail go_to ' + Math.round(position));
}

function sendSetLowerBound() {
  const value = readNumber('bound');
  sendCommand('rail lower ' + Math.round(value));
}

function sendSetUpperBound() {
  const value = readNumber('bound');
  sendCommand('rail upper ' + Math.round(value));
}

function sendSetWaitBefore() {
  const value = Math.max(0, readNumber('wait-before'));
  return sendCommand('rail wait_before ' + Math.round(value));
}

function sendSetWaitAfter() {
  const value = Math.max(0, readNumber('wait-after'));
  return sendCommand('rail wait_after ' + Math.round(value));
}

async function sendWaitSettings() {
  await sendSetWaitBefore();
  await new Promise((resolve) => setTimeout(resolve, 300));
  await sendSetWaitAfter();
  await new Promise((resolve) => setTimeout(resolve, 300));
}

function sendSetSpeedPreset(preset) {
  if (!preset) {
    return;
  }
  sendCommand('rail set_speed ' + String(preset).toLowerCase());
}

function sendSetSpeedRpm() {
  const rpm = Math.max(1, Math.round(readNumber('speed-rpm', 1)));
  sendCommand('rail set_rpm ' + rpm);
}

async function sendStartStack(expected_step_size_nm) {
  await sendWaitSettings();
  const stepSize =
      expected_step_size_nm !== undefined ? expected_step_size_nm : 1000;
  await sendCommand('rail stack_nm ' + Math.round(stepSize));
}

async function sendStartStackCount(length) {
  const stackLength = length !== undefined ? length : 100;
  await sendWaitSettings();
  await sendCommand('rail stack_count ' + Math.round(stackLength));
}

function parseStackValue(rawValue) {
  const value = (rawValue || '').toString().trim();
  if (!value.length) {
    return null;
  }
  const [prefix, ...rest] = value.split(':');
  const amount = Number(rest.join(':'));
  if (!rest.length || !Number.isFinite(amount) || amount <= 0) {
    return null;
  }

  const normalizedPrefix = prefix.toLowerCase();
  if (normalizedPrefix === 'step' || normalizedPrefix === 'count') {
    return {type : normalizedPrefix, value : amount};
  }
  return null;
}

function describeStackSelection(selection) {
  if (!selection) {
    return '';
  }
  if (selection.type === 'step') {
    return `Step size ${formatMicrons(selection.value)}`;
  }
  if (selection.type === 'count') {
    return `Count ${selection.value}`;
  }
  return '';
}

function sendSelectedStack() {
  const select = document.getElementById('stack-preset');
  const manualInput = document.getElementById('stack-manual');
  const manualRaw = manualInput ? manualInput.value.trim() : '';
  const parsedManual = parseStackValue(manualRaw);
  if (manualRaw && !parsedManual) {
    alert('Enter custom stack as "step:<nm>" or "count:<shots>".');
    return;
  }

  let selection = parsedManual;
  let label = parsedManual ? describeStackSelection(parsedManual)
              : (select && select.options[select.selectedIndex])
                  ? select.options[select.selectedIndex].textContent
                  : '';

  if (!selection && select) {
    selection = parseStackValue(select.value || '');
  }

  if (!selection) {
    alert('Choose a preset or enter a custom value like "step:750".');
    return;
  }

  const waitBeforeInput = document.getElementById('wait-before');
  const waitAfterInput = document.getElementById('wait-after');
  const waitBefore = waitBeforeInput ? waitBeforeInput.value : '—';
  const waitAfter = waitAfterInput ? waitAfterInput.value : '—';
  const confirmStart =
      confirm(`Start stack with ${parsedManual ? 'custom value' : 'preset'}: ${
          label || describeStackSelection(selection)}\nWait Before: ${
          waitBefore} ms\nWait After: ${waitAfter} ms`);
  if (!confirmStart) {
    return;
  }

  if (selection.type === 'step') {
    sendStartStack(selection.value);
  } else if (selection.type === 'count') {
    sendStartStackCount(selection.value);
  }
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

function resetRailState() {
  railState = {
    position_nm : null,
    target_nm : null,
    lower_nm : null,
    upper_nm : null,
    stack_index : null,
    stack_length : null,
    stack_running : false,
    wait_before_ms : null,
    wait_after_ms : null,
    moving : false,
    last_update : null
  };
  renderRailState();
}

function formatMicrons(nm) {
  if (!Number.isFinite(nm)) {
    return '—';
  }
  return `${(nm / 1000).toFixed(3)} μm`;
}

function formatStackProgress(index, length) {
  if (!Number.isFinite(index) || index < 0 || !Number.isFinite(length) ||
      length <= 0) {
    return 'Stack: idle';
  }
  return `Stack: ${index + 1} / ${length}`;
}

function renderRailState() {
  if (!railStateEls || !railStateEls.target) {
    return;
  }

  railStateEls.target.textContent = formatMicrons(railState.target_nm);
  railStateEls.lower.textContent = formatMicrons(railState.lower_nm);
  railStateEls.upper.textContent = formatMicrons(railState.upper_nm);
  const hasStackIndex =
      Number.isFinite(railState.stack_index) && railState.stack_index >= 0;
  const hasStackLength =
      Number.isFinite(railState.stack_length) && railState.stack_length > 0;
  railStateEls.stackStep.textContent =
      hasStackIndex ? String(railState.stack_index + 1) : '—';
  railStateEls.stackLength.textContent =
      hasStackLength ? String(railState.stack_length) : '—';
  railStateEls.stackStatus.textContent =
      railState.stack_running ? 'Running' : 'Idle';
  if (railStateEls.waitBefore) {
    railStateEls.waitBefore.textContent =
        Number.isFinite(railState.wait_before_ms)
            ? `(device: ${railState.wait_before_ms} ms)`
            : '—';
  }
  if (railStateEls.waitAfter) {
    railStateEls.waitAfter.textContent =
        Number.isFinite(railState.wait_after_ms)
            ? `(device: ${railState.wait_after_ms} ms)`
            : '—';
  }

  if (railStateEls.updated) {
    railStateEls.updated.textContent =
        railState.last_update
            ? `Updated ${railState.last_update.toLocaleTimeString()}`
            : 'Waiting for status...';
  }

  if (railStateEls.card) {
    railStateEls.card.classList.toggle('status-card--active',
                                       !!railState.last_update);
  }
}

function parseRailStateMessage(message) {
  if (!message || !message.startsWith('STATE')) {
    return null;
  }
  const jsonStart = message.indexOf('{');
  if (jsonStart === -1) {
    return null;
  }
  try {
    const data = JSON.parse(message.slice(jsonStart));
    const numberOrNull = (value) => {
      const parsed = Number(value);
      return Number.isFinite(parsed) ? parsed : null;
    };

    const stackIndex = numberOrNull(data.stack_index);
    const stackLength = numberOrNull(data.stack_length);

    return {
      position_nm : numberOrNull(data.position_nm),
      target_nm : numberOrNull(data.target_nm),
      lower_nm : numberOrNull(data.lower_nm),
      upper_nm : numberOrNull(data.upper_nm),
      stack_index : stackIndex !== null && stackIndex >= 0 ? stackIndex : null,
      stack_length : stackLength !== null && stackLength > 0 ? stackLength
                                                             : null,
      stack_running : data.stack_running === true || data.stack_running === 1,
      wait_before_ms : numberOrNull(data.wait_before_ms),
      wait_after_ms : numberOrNull(data.wait_after_ms),
      moving : data.moving === true || data.moving === 1
    };
  } catch (err) {
    console.warn('Failed to parse rail state payload', message, err);
    return null;
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
        state === 'connecting' || (state === 'connected' && !DEMO_MODE);
    connectButton.disabled = shouldDisable;
  }
}

function toggleControls(isConnected) {
  const allowExtended = isConnected || DEMO_MODE;
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
    sendCommand(value);
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
  const allButtonEl = document.querySelector('.tab-button-all');
  allTabsButton = allButtonEl || null;

  tabButtons =
      Array.from(document.querySelectorAll('.tab-button'))
          .filter((button) => !button.classList.contains('tab-button-all'));
  tabPanels = Array.from(document.querySelectorAll('.tab-panel'));

  if (!tabPanels.length) {
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

  if (allTabsButton) {
    allTabsButton.addEventListener('click', () => {
      if (allTabsButton.disabled) {
        return;
      }
      setAllTabsActive(true);
    });
  }

  activateTab(activeTabId);
}

function activateTab(targetId) {
  if (!targetId) {
    return;
  }
  activeTabId = targetId;
  if (allTabsActive) {
    setAllTabsActive(false);
    return;
  }
  updateTabsDisplay();
}

function setTabAvailability(enableConnectedTabs) {
  if (!tabButtons.length && !allTabsButton) {
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

  if (allTabsButton && allTabsButton.dataset.requiresConnection === 'true') {
    allTabsButton.disabled = !enableConnectedTabs;
    if (!enableConnectedTabs && allTabsActive) {
      setAllTabsActive(false);
    }
  }

  if (shouldReset) {
    activateTab('tab-home');
  } else {
    updateTabsDisplay();
  }
}

function setAllTabsActive(value) {
  allTabsActive = value;
  updateTabsDisplay();
}

function updateTabsDisplay() {
  if (allTabsActive) {
    tabPanels.forEach((panel) => panel.classList.add('active'));
    tabButtons.forEach((button) => button.classList.remove('active'));
    if (allTabsButton) {
      allTabsButton.classList.add('active');
    }
  } else {
    tabPanels.forEach((panel) => {
      panel.classList.toggle('active', panel.id === activeTabId);
    });
    tabButtons.forEach((button) => {
      const isActive = button.dataset.tabTarget === activeTabId;
      button.classList.toggle('active', isActive);
    });
    if (allTabsButton) {
      allTabsButton.classList.remove('active');
    }
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
