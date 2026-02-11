# BitChat Build Configuration

## Overview

This document describes the build configuration options for enabling and customizing BitChat integration in MeshCore firmware.

## Compile-Time Configuration

### ENABLE_BITCHAT Flag

The `ENABLE_BITCHAT` preprocessor flag controls whether BitChat support is included in the build.

```ini
; platformio.ini
build_flags =
    -D ENABLE_BITCHAT=1
```

| Value | Description |
|-------|-------------|
| `1` or defined | Include BitChat code |
| undefined | Exclude BitChat code (smaller binary) |

### Code Guard Pattern

All BitChat code is wrapped with conditional compilation:

```cpp
#ifdef ENABLE_BITCHAT
// BitChat implementation
#endif
```

## PlatformIO Configuration

### Build Environments

#### With BitChat Support

```ini
[env:WioTrackerL1_companion_radio_ble]
extends = nrf52_base
board = wio_tracker_l1
build_flags = 
    ${nrf52_base.build_flags}
    -D ENABLE_BITCHAT=1
    -D BLE_MODE_SWITCHING=1
```

#### Without BitChat (Standard)

```ini
[env:WioTrackerL1_companion_radio_ble]
extends = nrf52_base
board = wio_tracker_l1
; No ENABLE_BITCHAT flag - BitChat disabled
```

### Native Test Environment

```ini
[env:native_test]
platform = native
test_framework = unity
build_flags =
    -D NATIVE_TEST
    -D ENABLE_BITCHAT=1
    -std=c++11
```

## Platform-Specific Configuration

### nRF52 Platform

Additional flags for nRF52 builds:

```ini
build_flags =
    -D ENABLE_BITCHAT=1
    -D NRF52_PLATFORM
    -D BLE_MODE_SWITCHING=1  ; Enable menu-based BLE mode switching
    -D BITCHAT_MAX_PAYLOAD_SIZE=2048
```

#### nRF52 Menu-Based Switching

nRF52 uses the same menu-based switching UI as ESP32:

```cpp
// In UITask.cpp - Identical for both platforms
if (c == KEY_ENTER && _page == HomePage::BLE_MODE) {
  if (_task->isBitChatMode()) {
    _task->setBitChatMode(false);
    _task->showAlert("MeshCore Mode", 1000);
  } else {
    _task->setBitChatMode(true);
    _task->showAlert("BitChat Mode", 1000);
    the_mesh.initBitchatMeshChannel();
  }
}
```

The mode is stored in `Bluefruit` advertising data and switched via the UI menu system.

#### nRF52 Memory Considerations

| Component | Flash | RAM |
|-----------|-------|-----|
| Base MeshCore | ~400KB | ~60KB |
| BitChat Addition | ~80KB | ~25KB |
| Total with BitChat | ~480KB | ~85KB |

**nRF52840 Limits**: 1MB Flash, 256KB RAM

### ESP32 Platform

ESP32 supports BitChat through the `BitchatBLEService` class which attaches to the existing BLE server managed by `SerialBLEInterface`. Both ESP32 and nRF52 now support identical **menu-based mode switching**.

```ini
build_flags =
    -D ENABLE_BITCHAT=1
    -D ESP32
    -D BLE_MODE_SWITCHING=1  ; Enable menu-based BLE mode switching
```

#### ESP32 BLE Initialization

On ESP32, BitChat integrates with the existing BLE stack:

```cpp
// In main.cpp - ESP32 initialization flow
#ifdef ENABLE_BITCHAT
  // Initialize BitChat bridge
  bitchat_bridge.begin();
  
  // Attach to existing BLE server
  if (serial_interface.getBLEServer() != nullptr) {
    mesh::ble::BitchatBLEService &bitchatService = bitchat_bridge.getBLEService();
    bitchatService.initESP32(the_mesh.getNodeName());
    serial_interface.setBitChatService(bitchatService.getESP32Service());
  }
  
  the_mesh.initBitchat(&bitchat_bridge);
#endif
```

#### ESP32 Mode Switching

ESP32 supports **menu-based runtime mode switching** identical to nRF52:

```cpp
// In UITask.cpp - Menu-based switching
if (c == KEY_ENTER && _page == HomePage::BLE_MODE) {
  if (_task->isBitChatMode()) {
    _task->setBitChatMode(false);  // Switch to MeshCore
    _task->showAlert("MeshCore Mode", 1000);
  } else {
    _task->setBitChatMode(true);   // Switch to BitChat
    _task->showAlert("BitChat Mode", 1000);
    the_mesh.initBitchatMeshChannel();  // Prepare #mesh channel
  }
}
```

**Implementation in `SerialBLEInterface`:**

```cpp
void SerialBLEInterface::setBitChatMode(bool enable) {
  if (_bitchatMode == enable) return;
  _bitchatMode = enable;
  
  // Stop advertising
  pServer->getAdvertising()->stop();
  
  // Change advertised UUID
  BLEAdvertisementData oAdvertisementData;
  if (_bitchatMode && _bitchatService != nullptr) {
    oAdvertisementData.setCompleteServices(BLEUUID(_bitchatService->getUUID()));
  } else {
    oAdvertisementData.setCompleteServices(BLEUUID(SERVICE_UUID));
  }
  
  // Restart advertising with new UUID
  pServer->getAdvertising()->setAdvertisementData(oAdvertisementData);
  pServer->getAdvertising()->start();
}
```

**UI Display:**
- Navigate to **BLE_MODE** page using LEFT/RIGHT keys
- Display shows **"M"** (MeshCore) or **""** (BitChat) with mode name
- Press **ENTER** to toggle
- Alert shows "MeshCore Mode" or "BitChat Mode" for 1 second

#### ESP32 Memory Considerations

| Component | Flash | RAM |
|-----------|-------|-----|
| Base MeshCore | ~800KB | ~100KB |
| BitChat Addition | ~80KB | ~25KB |
| Total with BitChat | ~880KB | ~125KB |

**ESP32 Limits**: 4MB+ Flash, 520KB RAM

#### ESP32 vs nRF52 Comparison

| Feature | ESP32 | nRF52 |
|---------|-------|-------|
| BLE Stack | ESP-IDF NimBLE | Nordic SoftDevice |
| Service Creation | `initESP32()` attaches to existing server | `initNRF52()` creates standalone |
| Mode Switching | Menu-based `setBitChatMode()` | Menu-based `setBitChatMode()` |
| Menu Navigation | LEFT/RIGHT to BLE_MODE, ENTER to toggle | LEFT/RIGHT to BLE_MODE, ENTER to toggle |
| Visual Indicator | "M" / "" on display | "M" / "" on display |
| MTU Handling | Automatic | Manual negotiation |
| Security | Open (BitChat) / PIN (MeshCore) | Open (BitChat) / PIN (MeshCore) |

**Platform Parity**: Both ESP32 and nRF52 now support identical menu-based BLE mode switching UX. The only difference is the underlying BLE stack implementation (NimBLE vs SoftDevice).

## Debug Configuration

### Debug Logging

Enable verbose debug output:

```ini
build_flags =
    -D ENABLE_BITCHAT=1
    -D BITCHAT_DEBUG=1
    -D MESH_DEBUG=1
```

Debug levels:

| Flag | Output |
|------|--------|
| `BITCHAT_DEBUG` | BitChat-specific messages |
| `MESH_DEBUG` | General mesh debug |
| `BLE_DEBUG_LOGGING` | BLE communication debug |

### Debug Output Example

```
[BitChat] Bridge initialized
[BitChat] Peer ID set: 0xA1B2C3D4E5F6A7B8
[BitChat] Received message from BLE, type=2
[BitChat] Processing message from BLE queue
[BitChat] Sending GRP_TXT to mesh, len=42
[BitChat] Message sent to mesh as GRP_TXT!
```

## Feature Flags

### Core Features

| Flag | Default | Description |
|------|---------|-------------|
| `ENABLE_BITCHAT` | undefined | Master enable flag |
| `BITCHAT_SUPPORT_ENCAPSULATION` | 1 | Use PAYLOAD_TYPE_GRP_DATA encapsulation |
| `BITCHAT_SUPPORT_PLAINTEXT` | 1 | Use PAYLOAD_TYPE_GRP_TXT with prefix |
| `BITCHAT_MAX_CHANNELS` | 8 | Maximum channel mappings |
| `BITCHAT_MAX_PEERS` | 32 | Maximum peer cache entries |

### Optional Features

| Flag | Default | Description |
|------|---------|-------------|
| `BITCHAT_SUPPORT_COMPRESSION` | 1 | Enable zlib compression |
| `BITCHAT_SUPPORT_DM` | undefined | Enable direct message support |
| `BITCHAT_SUPPORT_FRAGMENTATION` | 1 | Enable message fragmentation |
| `BITCHAT_SUPPORT_NOISE` | undefined | Enable Noise protocol encryption |

### BLE Configuration

| Flag | Default | Description |
|------|---------|-------------|
| `BLE_MODE_SWITCHING` | undefined | Enable menu-based mode switching |
| `BITCHAT_BLE_MTU` | 185 | BLE MTU size |
| `BITCHAT_ANNOUNCE_INTERVAL_MS` | 5000 | Announcement interval |

## Variant-Specific Configuration

### Wio Tracker L1 Pro

```ini
[env:WioTrackerL1_companion_radio_ble]
extends = nrf52_base
board = wio_tracker_l1
build_flags = 
    ${nrf52_base.build_flags}
    -D ENABLE_BITCHAT=1
    -D BLE_MODE_SWITCHING=1
    -D P_LORA_RST=16
    -D P_LORA_CS=24
    ; ... other pin definitions
```

### RAK4631

```ini
[env:RAK4631_companion_radio_ble]
extends = nrf52_base
board = wiscore_rak4631
build_flags = 
    ${nrf52_base.build_flags}
    -D ENABLE_BITCHAT=1
    -D BLE_MODE_SWITCHING=1
```

### Heltec V3 (ESP32)

```ini
[env:Heltec_v3_companion_radio_ble]
extends = esp32_base
board = heltec_wifi_lora_32_V3
build_flags = 
    ${esp32_base.build_flags}
    -D ENABLE_BITCHAT=1
    -D BLE_MODE_SWITCHING=1
```

**ESP32-specific features:**
- Menu-based BLE mode switching (identical to nRF52)
- `BLECharacteristicCallbacks` for event handling
- Dynamic UUID switching via `setBitChatMode()`
- Compatible with Heltec V3 OLED display for mode indication

## Build Commands

### Build with BitChat

```bash
# Build for specific board with BitChat
pio run -e WioTrackerL1_companion_radio_ble

# Build with custom flags
PLATFORMIO_BUILD_FLAGS="-D ENABLE_BITCHAT=1 -D BITCHAT_DEBUG=1" pio run
```

### Run Tests

```bash
# Run all unit tests
pio test -e native_test

# Run specific test
pio test -e native_test --filter test_bitchat_identity
```

### Build Sizes

```bash
# Check firmware size
pio run -e WioTrackerL1_companion_radio_ble --target size

# Memory usage report
pio run -e WioTrackerL1_companion_radio_ble --target inspect
```

## Runtime Configuration

### Configuration File

BitChat runtime configuration is stored in `/config/bitchat.cfg`:

```cpp
struct BitchatConfig {
    uint16_t magic;        // 0x4243 ("BC")
    uint8_t version;       // 1
    uint8_t enabled;       // 0 or 1
    uint32_t lastModified; // Unix timestamp
    uint8_t reserved[56];  // Future expansion
};
```

### CLI Configuration

```
bitchat enable          ; Enable BitChat
bitchat disable         ; Disable BitChat
bitchat status          ; Show current status
bitchat channel list    ; List channel mappings
bitchat channel add <name> <key>  ; Add channel
bitchat channel remove <name>     ; Remove channel
```

## Troubleshooting Builds

### Common Issues

#### Flash Overflow

**Error**: `region 'flash' overflowed`

**Solutions**:
1. Disable debug features
2. Remove unused features
3. Use higher optimization (`-Os`)

```ini
build_flags =
    -D ENABLE_BITCHAT=1
    -Os  ; Optimize for size
    ; Remove: -D BITCHAT_DEBUG=1
```

#### RAM Overflow

**Error**: `region 'ram' overflowed`

**Solutions**:
1. Reduce buffer sizes
2. Disable fragmentation
3. Reduce peer cache size

```ini
build_flags =
    -D ENABLE_BITCHAT=1
    -D BITCHAT_MAX_PEERS=16  ; Reduce from 32
```

#### Undefined References

**Error**: `undefined reference to 'mesh::bitchat::...'`

**Solution**: Ensure `ENABLE_BITCHAT` is defined in all relevant environments.

### Build Verification

Check that BitChat is properly included:

```bash
# Check preprocessor defines
pio run -e WioTrackerL1_companion_radio_ble --target compiledb

# Search for BitChat symbols
nm .pio/build/*/firmware.elf | grep Bitchat
```

## CI/CD Configuration

### GitHub Actions

```yaml
name: Build BitChat Firmware

on:
  push:
    branches: [ feature/bitchat-encapsulation ]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      
      - name: Setup PlatformIO
        uses: platformio/action@v1
        
      - name: Build with BitChat
        run: |
          pio run -e WioTrackerL1_companion_radio_ble
          
      - name: Run Tests
        run: |
          pio test -e native_test
```

## Release Configuration

### Production Build

```ini
[env:release]
extends = env:WioTrackerL1_companion_radio_ble
build_flags = 
    ${env:WioTrackerL1_companion_radio_ble.build_flags}
    -D ENABLE_BITCHAT=1
    -Os
    -D NDEBUG
    ; No debug flags
```

### Development Build

```ini
[env:development]
extends = env:WioTrackerL1_companion_radio_ble
build_flags = 
    ${env:WioTrackerL1_companion_radio_ble.build_flags}
    -D ENABLE_BITCHAT=1
    -D BITCHAT_DEBUG=1
    -D MESH_DEBUG=1
    -g3
    -O0
```
