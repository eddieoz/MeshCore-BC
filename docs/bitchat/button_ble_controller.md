# Button BLE Controller

## Overview

The **ButtonBLEController** provides BLE mode switching (MeshCore ↔ BitChat) for devices **without displays** but with physical buttons. This enables users to switch between MeshCore UART mode and BitChat BLE mode using button presses, with LED and buzzer feedback for mode indication.

## Target Devices

This feature is designed for button-only devices such as:

| Device | Platform | Button | LED | Buzzer | Status |
|--------|----------|--------|-----|--------|--------|
| **T1000-E** | nRF52 | ✓ | ✓ Green | ✓ | ✅ Implemented |
| RAK WisMesh Tag | nRF52 | ✓ | ✓ RGB | ✓ | 📋 Planned |
| MinewSemi ME25LS01 | nRF52 | ✓ | ✓ RGB | ✗ | 📋 Planned |
| Heltec Mesh Solar | nRF52 | ✓ | ✓ RGB | ✗ | 📋 Planned |

## How It Works

### Button Action: Quintuple Press

Press the user button **5 times rapidly** (within 500ms between clicks) to toggle between modes.

```
User Action: [PRESS][RELEASE] x 5 times
                    ↓
        Mode Toggles: MeshCore ↔ BitChat
                    ↓
    Feedback: LED blinks 3x + Buzzer tone
```

### Visual Feedback (LED)

The LED provides visual confirmation of the current mode:

| Mode | Pattern | Description |
|------|---------|-------------|
| **MeshCore** | 3 slow blinks | 500ms ON, 500ms OFF |
| **BitChat** | 3 fast blinks | 150ms ON, 150ms OFF |

### Audio Feedback (Buzzer)

If the device has a buzzer, it plays a distinctive tone:

| Mode | Tone | Description |
|------|------|-------------|
| **MeshCore** | Low "C5" | Deeper tone |
| **BitChat** | High "G6" | Higher tone |

### Boot Behavior

- Device always boots in **MeshCore mode** (no persistence)
- LED blinks 3x slowly on boot to indicate MeshCore mode
- No buzzer sound on boot (to avoid annoying users)

## Implementation Details

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    ButtonBLEController                       │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐  │
│  │ Button Class │───→│ Mode Toggle  │───→│   Feedback   │  │
│  │ (5x press)   │    │ setBitChat() │    │ LED + Buzzer │  │
│  └──────────────┘    └──────────────┘    └──────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              │
                              ↓
                   ┌─────────────────────┐
                   │ SerialBLEInterface  │
                   │ - isBitChatMode()   │
                   │ - setBitChatMode()  │
                   └─────────────────────┘
```

### Code Integration

The controller is automatically included in `main.cpp` when:
- `ENABLE_BITCHAT` is defined
- `BLE_PIN_CODE` is defined (BLE enabled)
- `PIN_USER_BTN` is defined (has button)
- No `DISPLAY_CLASS` or `NullDisplayDriver` is used

```cpp
// From main.cpp - automatic inclusion
#if defined(ENABLE_BITCHAT) && defined(BLE_PIN_CODE) && !defined(DISPLAY_CLASS) && defined(PIN_USER_BTN)
#include <helpers/ButtonBLEController.h>
ButtonBLEController button_ble_controller(&serial_interface);
#endif
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
  -D USER_BTN_PRESSED=HIGH
  -D PIN_BUZZER=25
  -D PIN_BUZZER_EN=37
  -D LED_PIN=24
```

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
  -D USER_BTN_PRESSED=HIGH or LOW
  -D PIN_BUZZER=YOUR_BUZZER_PIN    ; Optional
  -D PIN_BUZZER_EN=YOUR_BUZZER_EN  ; Optional, for T1000-E style
  -D LED_PIN=YOUR_LED_PIN

build_src_filter =
  +<helpers/nrf52/SerialBLEInterface.cpp>  ; or esp32 version
  +<helpers/ui/buzzer.cpp>                  ; Optional
  +<helpers/bitchat/*.cpp>
  +<helpers/ble/*.cpp>
  +<helpers/ButtonBLEController.cpp>
  +<../examples/companion_radio/*.cpp>
  +<../examples/companion_radio/ui-orig/*.cpp>
```

3. **Build and test:**

```bash
pio run -e your_device_companion_radio_ble
```

## Troubleshooting

### LED Not Blinking

- Verify `LED_PIN` or `PIN_STATUS_LED` is correctly defined
- Check if `LED_STATE_ON` matches your hardware (HIGH or LOW)

### Buzzer Not Sounding

- Verify `PIN_BUZZER` is defined
- For T1000-E style buzzers, ensure `PIN_BUZZER_EN` is defined
- Check if buzzer enable pin needs HIGH or LOW

### Button Not Responding

- Verify `PIN_USER_BTN` is correctly defined
- Check `USER_BTN_PRESSED` state (HIGH or LOW when pressed)
- Ensure button debounce time (50ms) is sufficient

### Mode Not Switching

- Verify BitChat is enabled: `ENABLE_BITCHAT=1`
- Verify BLE is enabled: `BLE_PIN_CODE` defined
- Check serial output for `[ButtonBLEController] Initialized` message

## User Guide

### Switching to BitChat Mode

1. Press the button **5 times rapidly** (within ~2 seconds total)
2. Wait for feedback:
   - **3 fast LED blinks** = BitChat mode active
   - **High buzzer tone** = BitChat mode active (if buzzer present)
3. Open BitChat app on your phone
4. Connect to the device (it will advertise as BitChat-compatible)

### Switching Back to MeshCore Mode

1. Press the button **5 times rapidly** again
2. Wait for feedback:
   - **3 slow LED blinks** = MeshCore mode active
   - **Low buzzer tone** = MeshCore mode active (if buzzer present)
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
│  │  Buzzer: ♪ Low tone (C5)                             │  │
│  └──────────────────────────────────────────────────────┘  │
│                                                             │
│  BitChat Mode                                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  LED: ▄▀▄▀▄▀  (fast blink: 150ms on/off)             │  │
│  │  Buzzer: ♪ High tone (G6)                            │  │
│  └──────────────────────────────────────────────────────┘  │
│                                                             │
│  Action: 5x button press to toggle                         │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Future Enhancements

- [ ] Add persistence (save mode to flash)
- [ ] Add double-press for quick status check (1 blink = current mode)
- [ ] Support RGB LED color coding
- [ ] Add vibration feedback for devices with haptic motors
- [ ] Long-press action for additional functions

## See Also

- [BitChat Compatibility](./bitchat/compatibility_devices.md)
- [BitChat Build Configuration](./bitchat/build_configuration.md)
- [T1000-E Variant](../../variants/t1000-e/)
