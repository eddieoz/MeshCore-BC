# Button-Based BLE Mode Switching

## Overview

This feature provides button-based BLE mode switching (MeshCore ↔ BitChat) for devices **without displays** but with physical buttons. This enables users to switch between MeshCore UART mode and BitChat BLE mode using button presses, with LED and buzzer feedback for mode indication.

## Target Devices

This feature is designed for button-only devices such as:

| Device | Platform | Button | LED | Buzzer | Status |
|--------|----------|--------|-----|--------|--------|
| **T1000-E** | nRF52 | ✓ | ✓ Green | ✓ | ✅ Implemented (via UITask) |
| RAK WisMesh Tag | nRF52 | ✓ | ✓ RGB | ✓ | 📋 Planned |
| MinewSemi ME25LS01 | nRF52 | ✓ | ✓ RGB | ✗ | 📋 Planned |
| Heltec Mesh Solar | nRF52 | ✓ | ✓ RGB | ✗ | 📋 Planned |

## How It Works

### Button Action: Quintuple Press (5x)

Press the user button **5 times rapidly** (within ~3 seconds total) to toggle between modes.

```
User Action: [PRESS][RELEASE] x 5 times
                    ↓
        Mode Toggles: MeshCore ↔ BitChat
                    ↓
    Feedback: LED blinks 3x + Buzzer tone
```

### Visual Feedback (LED)

The LED provides visual confirmation of the mode change:

| Mode | Pattern | Description |
|------|---------|-------------|
| **MeshCore** | 3 slow blinks | 500ms ON, 500ms OFF |
| **BitChat** | 3 fast blinks | 150ms ON, 150ms OFF |

### Audio Feedback (Buzzer)

If the device has a buzzer, it plays an acknowledgment tone when the mode changes.

### Boot Behavior

- Device always boots in **MeshCore mode** (no persistence)
- BLE advertising starts automatically after boot
- No button feedback on boot (to avoid annoying users)

## Implementation Details

### Architecture

For devices with `NullDisplayDriver` (like T1000-E), the implementation uses **UITask's built-in button handlers** rather than a separate ButtonBLEController. This avoids conflicts with existing button functionality.

```
┌─────────────────────────────────────────────────────────────┐
│                         UITask                               │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────┐    ┌──────────────────┐                  │
│  │ Button Class │───→│ Quintuple Press  │                  │
│  │ (5x press)   │    │   Handler        │                  │
│  └──────────────┘    └────────┬─────────┘                  │
│                               ↓                             │
│                    ┌─────────────────────┐                 │
│                    │ setBitChatMode()    │                 │
│                    │ + LED Feedback      │                 │
│                    └─────────────────────┘                 │
└─────────────────────────────────────────────────────────────┘
                              │
                              ↓
                   ┌─────────────────────┐
                   │ SerialBLEInterface  │
                   │ - Switches BLE      │
                   │   advertising       │
                   └─────────────────────┘
```

### Code Integration

The mode switching is handled in `UITask::handleButtonQuintuplePress()` when:
- `ENABLE_BITCHAT` is defined
- `NullDisplayDriver` is used (display-less device)

```cpp
void UITask::handleButtonQuintuplePress() {
#ifdef ENABLE_BITCHAT
  if (_serial) {
    bool newBitChatMode = !_serial->isBitChatMode();
    _serial->setBitChatMode(newBitChatMode);
    
    // LED feedback - 3 blinks
    #ifdef LED_PIN
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_PIN, HIGH);
      delay(newBitChatMode ? 150 : 500);
      digitalWrite(LED_PIN, LOW);
      delay(newBitChatMode ? 150 : 500);
    }
    #endif
    
    #ifdef PIN_BUZZER
    notify(UIEventType::ack);  // Acknowledgment sound
    #endif
  }
#endif
}
```

## Device-Specific Configuration

### T1000-E Configuration

The T1000-E uses the following pin configuration:

```ini
[env:t1000e_companion_radio_ble]
build_flags =
  -D BLE_PIN_CODE=123456
  -D ENABLE_BITCHAT=1
  -D DISPLAY_CLASS=NullDisplayDriver
  -D PIN_USER_BTN=6
  -D USER_BTN_PRESSED=LOW    ; T1000-E button is active LOW
  -D PIN_BUZZER=25
  -D PIN_BUZZER_EN=37
  -D LED_PIN=24
```

**Note:** The T1000-E button is **active LOW** (goes to LOW when pressed), so `USER_BTN_PRESSED=LOW` must be defined.

### Adding Support for New Devices

To add support for a new button-only device:

1. **Ensure the device has required hardware:**
   - Physical button (`PIN_USER_BTN`)
   - LED (`LED_PIN` or `PIN_STATUS_LED`)
   - Buzzer (optional, `PIN_BUZZER`)

2. **Update platformio.ini:**

```ini
[env:your_device_companion_radio_ble]
build_flags =
  -D BLE_PIN_CODE=123456
  -D ENABLE_BITCHAT=1
  -D DISPLAY_CLASS=NullDisplayDriver
  -D PIN_USER_BTN=YOUR_BUTTON_PIN
  -D USER_BTN_PRESSED=LOW    ; or HIGH depending on hardware
  -D PIN_BUZZER=YOUR_BUZZER_PIN    ; Optional
  -D PIN_BUZZER_EN=YOUR_BUZZER_EN  ; Optional
  -D LED_PIN=YOUR_LED_PIN

build_src_filter =
  +<helpers/nrf52/SerialBLEInterface.cpp>  ; or esp32 version
  +<helpers/ui/buzzer.cpp>                  ; Optional
  +<helpers/bitchat/*.cpp>
  +<helpers/ble/*.cpp>
  +<../examples/companion_radio/*.cpp>
  +<../examples/companion_radio/ui-orig/*.cpp>
```

3. **Build and test:**

```bash
pio run -e your_device_companion_radio_ble
```

## Troubleshooting

### Device Not Appearing in Bluetooth Scan

- Ensure BLE advertising is started (`serial_interface.enable()` is called in main.cpp)
- Check that `BLE_PIN_CODE` is defined
- Verify the device name in serial logs (should show "[BLE] Advertising started")

### LED Not Blinking

- Verify `LED_PIN` or `PIN_STATUS_LED` is correctly defined
- Check if `LED_STATE_ON` matches your hardware (HIGH or LOW)
- For T1000-E: `LED_PIN` is 24 (green LED)

### Buzzer Not Sounding

- Verify `PIN_BUZZER` is defined
- For T1000-E style buzzers, ensure `PIN_BUZZER_EN` is defined and set to 37
- Check that `buzzer.begin()` is called in UITask

### Button Not Responding

- Verify `PIN_USER_BTN` is correctly defined
- Check `USER_BTN_PRESSED` state:
  - T1000-E: `USER_BTN_PRESSED=LOW` (active LOW)
  - Other devices: may vary
- Ensure button debounce time is sufficient

### Mode Not Switching

- Verify BitChat is enabled: `ENABLE_BITCHAT=1`
- Verify BLE is enabled: `BLE_PIN_CODE` defined
- Check serial output for "[BLE] Switched to BitChat mode" or "[BLE] Switched to MeshCore mode"

## User Guide

### Switching to BitChat Mode

1. Press the button **5 times rapidly** (within ~3 seconds total)
2. Wait for feedback:
   - **3 fast LED blinks** = BitChat mode active
   - **Buzzer tone** = Acknowledgment (if buzzer present)
   - Serial log: "[BLE] Switched to BitChat mode"
3. Open BitChat app on your phone
4. Connect to the device (it will advertise as BitChat-compatible)

### Switching Back to MeshCore Mode

1. Press the button **5 times rapidly** again
2. Wait for feedback:
   - **3 slow LED blinks** = MeshCore mode active
   - **Buzzer tone** = Acknowledgment (if buzzer present)
   - Serial log: "[BLE] Switched to MeshCore mode"
3. Use with MeshCore-compatible apps

### Visual Reference

```
┌─────────────────────────────────────────────────────────────┐
│  Mode Indicator Quick Reference                             │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  MeshCore Mode (Default)                                   │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  LED: ▄▀ ▄▀ ▄▀  (slow blink: 500ms on/off)           │  │
│  │  Buzzer: ♪ Acknowledgment tone                      │  │
│  └──────────────────────────────────────────────────────┘  │
│                                                             │
│  BitChat Mode                                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  LED: ▄▀▄▀▄▀  (fast blink: 150ms on/off)             │  │
│  │  Buzzer: ♪ Acknowledgment tone                      │  │
│  └──────────────────────────────────────────────────────┘  │
│                                                             │
│  Action: 5x button press to toggle                         │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Future Enhancements

- [ ] Add persistence (save mode to flash)
- [ ] Add triple-press for quick status check (1 blink = current mode)
- [ ] Support RGB LED color coding
- [ ] Add vibration feedback for devices with haptic motors
- [ ] Reduce required button presses from 5x to 3x with better timing

## See Also

- [BitChat Compatibility](./compatibility_devices.md)
- [BitChat Build Configuration](./build_configuration.md)
- [T1000-E Variant](../../variants/t1000-e/)
