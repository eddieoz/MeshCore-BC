# BitChat Device Compatibility

## Overview

BitChat support requires either **on-device UI navigation** (screen + buttons) or **button-based mode switching** to toggle between MeshCore and BitChat BLE modes. This page lists all compatible and incompatible devices.

## Compatible Devices (35 Total)

These devices have displays and buttons for menu-based BLE mode switching.

### ESP32 Devices (20)

| # | Device | Display | Build Environment | Notes |
|---|--------|---------|-------------------|-------|
| 1 | **Ebyte EoRa-S3** | Yes | `Ebyte_EoRa-S3_companion_radio_ble` | |
| 2 | **Heltec LoRa V2** | SSD1306 | `Heltec_v2_companion_radio_ble` | 160 contacts |
| 3 | **Heltec LoRa V3** | SSD1306 | `Heltec_v3_companion_radio_ble` | Most popular |
| 4 | **Heltec WSL3** | SSD1306 | `Heltec_WSL3_companion_radio_ble` | V3 variant |
| 5 | **Heltec LoRa V4** | Multiple | `heltec_v4_companion_radio_ble` | |
| 6 | **Heltec V4 TFT** | TFT | `heltec_v4_tft_companion_radio_ble` | Color display |
| 7 | **Heltec Wireless Tracker** | Yes | `Heltec_Wireless_Tracker_companion_radio_ble` | GPS built-in |
| 8 | **Heltec Tracker V2** | Yes | `heltec_tracker_v2_companion_radio_ble` | |
| 9 | **Heltec Wireless Paper** | E-paper | `Heltec_Wireless_Paper_companion_radio_ble` | |
| 10 | **Keepteen LT1** | Yes | `KeepteenLT1_companion_radio_ble` | |
| 11 | **LilyGo T3-S3** | Yes | `LilyGo_T3S3_sx1262_companion_radio_ble` | SX1262 |
| 12 | **LilyGo T3-S3 (SX1276)** | Yes | `LilyGo_T3S3_sx1276_companion_radio_ble` | SX1276 variant |
| 13 | **LilyGo T-Beam (SX1262)** | Yes | `Tbeam_SX1262_companion_radio_ble` | GPS + 18650 |
| 14 | **LilyGo T-Beam (SX1276)** | Yes | `Tbeam_SX1276_companion_radio_ble` | GPS + 18650 |
| 15 | **LilyGo T-Beam Supreme** | Yes | `T_Beam_S3_Supreme_SX1262_companion_radio_ble` | S3 variant |
| 16 | **LilyGo T-Deck** | Yes | `LilyGo_TDeck_companion_radio_ble` | Keyboard |
| 17 | **LilyGo T-Echo** | Yes | `LilyGo_T-Echo_companion_radio_ble` | nRF52840+ESP32 |
| 18 | **LilyGo T-Echo Lite** | Yes | `LilyGo_T-Echo-Lite_companion_radio_ble` | |
| 19 | **LilyGo TLora V2.1** | Yes | `LilyGo_TLora_V2_1_1_6_companion_radio_ble` | |
| 20 | **Mesh Pocket** | Yes | `Mesh_pocket_companion_radio_ble` | |
| 21 | **Mesh Adventurer (SX1262)** | Yes | `Meshadventurer_sx1262_companion_radio_ble` | |
| 22 | **Mesh Adventurer (SX1268)** | Yes | `Meshadventurer_sx1268_companion_radio_ble` | China freq |
| 23 | **Meshtiny** | Yes | `Meshtiny_companion_radio_ble` | Compact |
| 24 | **Nano G2 Ultra** | Yes | `Nano_G2_Ultra_companion_radio_ble` | |
| 25 | **Nibble Screen Connect** | Yes | `nibble_screen_connect_companion_radio_ble` | |
| 26 | **ProMicro** | Yes | `ProMicro_companion_radio_ble` | |
| 27 | **RAK3112** | Yes | `RAK3112_companion_radio_ble` | WisDuo |
| 28 | **RAK3401** | Yes | `RAK_3401_companion_radio_ble` | WisBlock Mini |
| 29 | **Station G2** | Yes | `Station_G2_companion_radio_ble` | |
| 30 | **ThinkNode M1** | Yes | `ThinkNode_M1_companion_radio_ble` | |
| 31 | **ThinkNode M2** | Yes | `ThinkNode_M2_companion_radio_ble` | |
| 32 | **ThinkNode M5** | Yes | `ThinkNode_M5_companion_radio_ble` | |
| 33 | **Xiao S3 WIO** | Yes | `Xiao_S3_WIO_companion_radio_ble` | |

*Note: Some ESP32 variants have multiple build environments for different radio modules*

### nRF52 Devices (15)

| # | Device | Display | Build Environment | Notes |
|---|--------|---------|-------------------|-------|
| 1 | **Heltec T114** | ST7789 | `Heltec_t114_companion_radio_ble` | Color LCD |
| 2 | **Ikoka Handheld nRF (22dBm)** | SSD1306 | `ikoka_handheld_nrf_e22_22dbm_companion_radio_ble` | |
| 3 | **Ikoka Handheld nRF (30dBm)** | SSD1306 | `ikoka_handheld_nrf_e22_30dbm_096_companion_radio_ble` | |
| 4 | **Ikoka Handheld nRF (30dBm Rotated)** | SSD1306 | `ikoka_handheld_nrf_e22_30dbm_096_rotated_companion_radio_ble` | |
| 5 | **Ikoka Stick nRF (22dBm)** | SSD1306 | `ikoka_stick_nrf_22dbm_companion_radio_ble` | |
| 6 | **Ikoka Stick nRF (30dBm)** | SSD1306 | `ikoka_stick_nrf_30dbm_companion_radio_ble` | |
| 7 | **Ikoka Stick nRF (33dBm)** | SSD1306 | `ikoka_stick_nrf_33dbm_companion_radio_ble` | |
| 8 | **RAK4631** | Yes | `RAK_4631_companion_radio_ble` | Most popular nRF52 |
| 9 | **Wio Tracker L1** | SH1106 | `WioTrackerL1_companion_radio_ble` | GPS + LCD |
| 10 | **Wio Tracker L1 E-ink** | E-ink | `WioTrackerL1Eink_companion_radio_ble` | E-paper variant |

*Note: Some nRF52 variants have multiple build environments for different power levels*

## Button-Only Devices (No Display)

These devices use **button-based mode switching** (5x press) with LED/buzzer feedback instead of menu navigation. The mode switching is handled by UITask's quintuple press handler (not a separate controller). See [Button BLE Controller](../button_ble_controller.md) for details.

### nRF52 (Button-Only)

| # | Device | Variant | Button | LED | Buzzer | Build Environment | Implementation |
|---|--------|---------|--------|-----|--------|-------------------|----------------|
| 1 | **T1000-E** | `t1000-e` | ✓ | ✓ Green | ✓ | `t1000e_companion_radio_ble` | UITask (NullDisplayDriver) |
| 2 | **RAK WisMesh Tag** | `rak_wismesh_tag` | ✓ | ✓ RGB | ✓ | Planned | - |
| 3 | MinewSemi ME25LS01 | `minewsemi_me25ls01` | ✓ | ✓ RGB | ✗ | Planned | - |

**Usage:** Press the user button **5 times rapidly** (within ~3 seconds) to toggle between MeshCore and BitChat modes.
- 3 slow LED blinks (500ms) = MeshCore mode
- 3 fast LED blinks (150ms) = BitChat mode
- Buzzer acknowledgment tone on mode switch (if available)

**Note:** T1000-E uses `USER_BTN_PRESSED=LOW` as the button is active LOW.

## Incompatible Devices (No Display/No Button)

These devices **cannot** currently use BitChat due to lack of screen and buttons. They may receive CLI-based configuration in a future release.

### ESP32 (No Display)

| Device | Variant | Reason |
|--------|---------|--------|
| Heltec CT62 | `heltec_ct62` | No display |
| Heltec Mesh Solar | `heltec_mesh_solar` | No display, solar-only |
| Heltec Wireless Paper (USB only) | `heltec_wireless_paper` | Some variants no display |
| SenseCap Indicator ESPNow | `sensecap_indicator-espnow` | ESPNow only |
| SenseCap Solar | `sensecap_solar` | No display |
| Xiao C3 | `xiao_c3` | Compact, no display |

### nRF52 (No Display)

| Device | Variant | Reason |
|--------|---------|--------|
| Heltec T114 (No Display) | `Heltec_t114_without_display` | Variant without screen |
| Ikoka Nano nRF | `ikoka_nano_nrf` | Compact, no display |
| Wio WM1110 | `wio_wm1110` | Compact module |
| Xiao nRF52 | `xiao_nrf52` | Compact, no display |

### STM32/RP2040 (Not Supported)

BitChat is currently only supported on ESP32 and nRF52 platforms.

| Device | Platform | Reason |
|--------|----------|--------|
| Various STM32 devices | STM32 | Platform not supported |
| Various RP2040 devices | RP2040 | Platform not supported |

## How to Check Your Device

### Method 1: Check the Variant Directory

```bash
cd /path/to/MeshCore
ls variants/YOUR_DEVICE/platformio.ini
grep -E "DISPLAY_CLASS|ENABLE_BITCHAT" variants/YOUR_DEVICE/platformio.ini
```

If you see:
- `DISPLAY_CLASS=` that is NOT `NullDisplayDriver` → **Compatible**
- `ENABLE_BITCHAT=1` in companion_radio_ble env → **Enabled**

### Method 2: Build and Check

```bash
# Try building your device with BitChat
pio run -e YOUR_DEVICE_companion_radio_ble

# If it builds successfully and you see BitChat symbols, it's compatible
nm .pio/build/YOUR_DEVICE_companion_radio_ble/firmware.elf | grep -i bitchat
```

### Method 3: Check This Table

1. Find your device in the **Compatible** or **Incompatible** lists above
2. If your device has a screen and buttons, it's likely compatible
3. If your device has no screen, it cannot use menu-based switching

## Requesting Support for New Devices

If your device has a display and buttons but is not listed:

1. Open an issue on GitHub
2. Provide the device name and variant
3. Confirm it has:
   - A display (any type)
   - At least one button for navigation
   - ESP32 or nRF52 processor

## Future Support for Display-Less Devices

Devices without displays or buttons may receive BitChat support through:

- **CLI Configuration**: `bitchat enable` command via serial
- **Startup Mode**: Default to BitChat mode on boot
- **Companion App**: Configure via USB/BLE from phone

Planned for future releases. Priority will be given to popular devices like:
- Xiao series
- Heltec Mesh Solar

## See Also

- [Build Configuration](./build_configuration.md) - How to enable BitChat
- [BLE Service](./ble_service.md) - Technical BLE specifications
- [Architecture](./architecture.md) - How BitChat integration works
