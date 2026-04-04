# BitChat Device Compatibility

> This document lists device compatibility for the BitChat BLE feature in MeshCore-BC firmware.

## ⚠️ Important: RAM Requirements

BitChat requires **BLE support** and sufficient RAM for the BLE stack and message buffering:

| Platform | Total RAM | BitChat Compatible? |
|----------|-----------|---------------------|
| ESP32 | 320KB | ❌ NO - Insufficient RAM |
| ESP32-S3 | 512KB | ✅ YES |
| nRF52 | 256KB | ✅ YES* |

*nRF52 uses a different memory architecture with external flash for storage, making more RAM available for the application.

## Overview

BitChat support requires either **on-device UI navigation** (screen + buttons) or **button-based mode switching** to toggle between MeshCore and BitChat BLE modes. This page lists all compatible and incompatible devices.

## ✅ Compatible Devices (43 Total)

These devices have been **tested on real hardware** or have sufficient RAM to support BitChat.

### Real-World Tested

| Device | Platform | BitChat BLE | Notes |
|--------|----------|:-----------:|:------|
| **Seeed Wio Tracker 1110** | nRF52 | ✅ | Verified working |
| **LilyGO T1000-E** | nRF52 | ✅ | Verified working (button-only) |

### ESP32-S3 Devices (19)

These devices have sufficient RAM (512KB) to support BitChat BLE:

| # | Device | Display | Build Environment | Notes |
|---|--------|---------|-------------------|-------|
| 1 | **Ebyte EoRa-S3** | Yes | `Ebyte_EoRa-S3_companion_radio_ble` | |
| 2 | **Heltec LoRa V3** | SSD1306 | `Heltec_v3_companion_radio_ble` | Most popular |
| 3 | **Heltec WSL3** | SSD1306 | `Heltec_WSL3_companion_radio_ble` | V3 variant |
| 4 | **Heltec LoRa V4** | Multiple | `heltec_v4_companion_radio_ble` | |
| 5 | **Heltec V4 TFT** | TFT | `heltec_v4_tft_companion_radio_ble` | Color display |
| 6 | **Heltec Wireless Tracker** | Yes | `Heltec_Wireless_Tracker_companion_radio_ble` | GPS built-in |
| 7 | **Heltec Tracker V2** | Yes | `heltec_tracker_v2_companion_radio_ble` | |
| 8 | **Heltec Wireless Paper** | E-paper | `Heltec_Wireless_Paper_companion_radio_ble` | |
| 9 | **LilyGo T3-S3 (SX1262)** | Yes | `LilyGo_T3S3_sx1262_companion_radio_ble` | SX1262 |
| 10 | **LilyGo T3-S3 (SX1276)** | Yes | `LilyGo_T3S3_sx1276_companion_radio_ble` | SX1276 variant |
| 11 | **LilyGo T-Beam Supreme** | Yes | `T_Beam_S3_Supreme_SX1262_companion_radio_ble` | S3 variant (512KB RAM) |
| 12 | **LilyGo T-Deck** | Yes | `LilyGo_TDeck_companion_radio_ble` | Keyboard |
| 13 | **Nibble Screen Connect** | Yes | `nibble_screen_connect_companion_radio_ble` | |
| 14 | **ProMicro** | Yes | `ProMicro_companion_radio_ble` | |
| 15 | **Station G2** | Yes | `Station_G2_companion_radio_ble` | |
| 16 | **ThinkNode M1** | Yes | `ThinkNode_M1_companion_radio_ble` | |
| 17 | **ThinkNode M2** | Yes | `ThinkNode_M2_companion_radio_ble` | |
| 18 | **ThinkNode M5** | Yes | `ThinkNode_M5_companion_radio_ble` | |
| 19 | **Xiao S3 WIO** | Yes | `Xiao_S3_WIO_companion_radio_ble` | |

*Note: Original ESP32 devices with 320KB RAM (Heltec v2, T-Beam SX1262/SX1276, TLora V2.1, MeshAdventurer) are NOT compatible due to insufficient memory for BitChat BLE*

### nRF52 Devices (20)

| # | Device | Display | Build Environment | Notes |
|---|--------|---------|-------------------|-------|
| 1 | **Heltec T114** | ST7789 | `Heltec_t114_companion_radio_ble` | Color LCD |
| 2 | **Heltec Mesh Solar** | No | `Heltec_mesh_solar_companion_radio_ble` | Solar-powered |
| 3 | **Ikoka Nano nRF (22dBm)** | No | `ikoka_nano_nrf_22dbm_companion_radio_ble` | Compact |
| 4 | **Ikoka Nano nRF (30dBm)** | No | `ikoka_nano_nrf_30dbm_companion_radio_ble` | Compact |
| 5 | **Ikoka Stick nRF (22dBm)** | SSD1306 | `ikoka_stick_nrf_22dbm_companion_radio_ble` | |
| 6 | **Ikoka Stick nRF (30dBm)** | SSD1306 | `ikoka_stick_nrf_30dbm_companion_radio_ble` | |
| 7 | **Ikoka Stick nRF (33dBm)** | SSD1306 | `ikoka_stick_nrf_33dbm_companion_radio_ble` | |
| 8 | **Keepteen LT1** | Yes | `KeepteenLT1_companion_radio_ble` | |
| 9 | **LilyGo T-Echo** | Yes | `LilyGo_T-Echo_companion_radio_ble` | nRF52840 |
| 10 | **LilyGo T-Echo Lite** | Yes | `LilyGo_T-Echo-Lite_companion_radio_ble` | |
| 11 | **Mesh Pocket** | Yes | `Mesh_pocket_companion_radio_ble` | |
| 12 | **Meshtiny** | Yes | `Meshtiny_companion_radio_ble` | Compact |
| 13 | **Nano G2 Ultra** | Yes | `Nano_G2_Ultra_companion_radio_ble` | |
| 14 | **RAK 3112** | Yes | `RAK_3112_companion_radio_ble` | WisDuo |
| 15 | **RAK 3401** | Yes | `RAK_3401_companion_radio_ble` | WisBlock Mini |
| 16 | **RAK 4631** | Yes | `RAK_4631_companion_radio_ble` | Most popular nRF52 |
| 17 | **SenseCap Solar** | No | `SenseCap_Solar_companion_radio_ble` | Solar-powered |
| 18 | **Wio Tracker L1** | SH1106 | `WioTrackerL1_companion_radio_ble` | GPS + LCD |
| 19 | **Wio Tracker L1 E-ink** | E-ink | `WioTrackerL1Eink_companion_radio_ble` | E-paper variant |
| 20 | **Xiao nRF52** | No | `Xiao_nrf52_companion_radio_ble` | Compact |

*Note: Some nRF52 variants have multiple build environments for different power levels*

## Button-Only Devices (No Display)

These devices use **button-based mode switching** (5x press) with LED/buzzer feedback instead of menu navigation. The mode switching is handled by UITask's quintuple press handler (not a separate controller). See [Button BLE Controller](./button_ble_controller.md) for details.

### nRF52 (Button-Only)

| # | Device | Variant | Button | LED | Buzzer | Build Environment | Implementation |
|---|--------|---------|--------|-----|--------|-------------------|----------------|
| 1 | **T1000-E** | `t1000-e` | ✓ | ✓ Green | ✓ | `t1000e_companion_radio_ble` | UITask (NullDisplayDriver) |
| 2 | **RAK WisMesh Tag** | `rak_wismesh_tag` | ✓ | ✓ RGB | ✓ | `RAK_WisMesh_Tag_companion_radio_ble` | UITask (NullDisplayDriver) |

**Usage:** Press the user button **5 times rapidly** (within ~3 seconds) to toggle between MeshCore and Bitchat modes.
- 3 slow LED blinks (500ms) = MeshCore mode
- 3 fast LED blinks (150ms) = Bitchat mode
- Buzzer acknowledgment tone on mode switch (if available)

**Note:** T1000-E uses `USER_BTN_PRESSED=LOW` as the button is active LOW.

## Incompatible Devices

### ESP32 with Insufficient RAM (320KB)

These devices have the original ESP32 with only 320KB SRAM. BitChat requires ~25-30KB additional RAM for BLE services and message buffering, which exceeds available memory:

| Device | Platform | RAM | Reason |
|--------|----------|-----|--------|
| **Heltec LoRa V2** | ESP32 | 320KB | Insufficient RAM (+28KB overflow) |
| **LilyGo T-Beam (SX1262)** | ESP32 | 320KB | Insufficient RAM (+29KB overflow) |
| **LilyGo T-Beam (SX1276)** | ESP32 | 320KB | Insufficient RAM (+29KB overflow) |
| **LilyGo TLora V2.1** | ESP32 | 320KB | Insufficient RAM (+30KB overflow) |
| **Mesh Adventurer (SX1262)** | ESP32 | 320KB | Insufficient RAM (+20KB overflow) |
| **Mesh Adventurer (SX1268)** | ESP32 | 320KB | Insufficient RAM (+20KB overflow) |

**Workaround:** Use USB interface (`_companion_radio_usb` variant) instead of BLE.

### Why ESP32 320KB Devices Are Incompatible

The original ESP32 has 320KB total SRAM (~280KB available after system reservation). BitChat requires:
- BLE service code and state: ~8KB
- Message buffering for protocol translation: ~12KB
- Encryption state and session management: ~6KB
- Additional BLE stack overhead: ~4KB

**Total BitChat overhead: ~30KB** - pushing these boards over their memory limit.

### No Display/No Button

These devices **cannot** currently use Bitchat due to lack of screen and buttons:

### ESP32 (No Display)

| Device | Variant | Reason |
|--------|---------|--------|
| Heltec CT62 | `heltec_ct62` | No display |
| Heltec Wireless Paper (USB only) | `heltec_wireless_paper` | Some variants no display |
| SenseCap Indicator ESPNow | `sensecap_indicator-espnow` | ESPNow only |
| Xiao C3 | `xiao_c3` | Compact, no display |

### nRF52 (No Display)

| Device | Variant | Reason |
|--------|---------|--------|
| Heltec T114 (No Display) | `Heltec_t114_without_display` | Variant without screen |
| Wio WM1110 | `wio_wm1110` | Compact module |

### STM32/RP2040 (Not Supported)

Bitchat is currently only supported on ESP32-S3 (512KB RAM) and nRF52 platforms.

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
# Try building your device with Bitchat
pio run -e YOUR_DEVICE_companion_radio_ble

# If it builds successfully and you see Bitchat symbols, it's compatible
nm .pio/build/YOUR_DEVICE_companion_radio_ble/firmware.elf | grep -i bitchat
```

### Method 3: Check This Table

1. Find your device in the **Compatible** or **Incompatible** lists above
2. If your device has a screen and buttons, it's likely compatible
3. If your device has no screen, it cannot use menu-based switching

## Memory Requirements

| Platform | Total RAM | Available | BitChat | Compatible? |
|----------|-----------|-----------|---------|-------------|
| ESP32 | 320KB | ~280KB | ~30KB | ❌ No - insufficient RAM |
| ESP32-S3 | 512KB | ~470KB | ~30KB | ✅ Yes |
| nRF52 | 256KB | ~220KB | ~25KB | ✅ Yes* |

*nRF52 uses a different memory architecture with external flash for storage, making more RAM available for the application.

## Requesting Support for New Devices

If your device has a display and buttons but is not listed:

1. Open an issue on GitHub
2. Provide the device name and variant
3. Confirm it has:
   - ESP32-S3 (512KB RAM) or nRF52 processor
   - A display (any type)
   - At least one button for navigation

## Future Support for Display-Less Devices

Devices without displays or buttons may receive Bitchat support through:

- **CLI Configuration**: `bitchat enable` command via serial
- **Startup Mode**: Default to Bitchat mode on boot
- **Companion App**: Configure via USB/BLE from phone

Planned for future releases. Priority will be given to popular devices like:
- Xiao series
- Heltec Mesh Solar

## Testers Wanted!

If you have any of the **build-verified** devices above, please test and report your results:

1. Flash the firmware to your device
2. Test core functionality:
   - BLE pairing with BitChat app
   - Sending/receiving messages
   - Display navigation (if applicable)
   - Button controls
3. Report results in [GitHub Issues](../../issues)

Your feedback helps move devices from "build-verified" to "real-world tested"!

## See Also

- [Build Configuration](./build_configuration.md) - How to enable Bitchat
- [BLE Service](./ble_service.md) - Technical BLE specifications
- [Architecture](./architecture.md) - How Bitchat integration works
