# Bitchat Integration for MeshCore

## Overview

The Bitchat integration enables MeshCore devices to communicate with the Bitchat Android app through a bridge layer that translates between Bitchat protocol and MeshCore mesh networking. This is an **additive feature** that preserves all existing MeshCore functionality while adding Bitchat compatibility.

## Key Principles

1. **Additive, Not Substitutive**: Bitchat support is added alongside MeshCore, not replacing it
2. **No Infrastructure Changes**: Repeaters and room servers require no modifications
3. **Encapsulation Strategy**: Bitchat messages are encapsulated in standard MeshCore packets
4. **Backward Compatible**: Existing MeshCore nodes continue to work normally

## Architecture

```
┌──────────────────────────────────────────────────────────────────────────┐
│                            MeshCore Device                               │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌─────────────────────────────┐    ┌────────────────────────────────┐  │
│  │    WITH DISPLAY (Menu)      │    │     BUTTON-ONLY (T1000-E)      │  │
│  │   ┌─────────────────────┐   │    │   ┌────────────────────────┐   │  │
│  │   │   BITCHAT Page      │   │    │   │   5x Button Press      │   │  │
│  │   │  ┌─────┐  ┌─────┐   │   │    │   │   (Quintuple)          │   │  │
│  │   │  │  M  │  │  B  │   │   │    │   │  ┌──────────────────┐  │   │  │
│  │   │  │Mesh │  │BitC │   │   │    │   │  │ LED: 3 blinks    │  │   │  │
│  │   │  └──┬──┘  └──┬──┘   │   │    │   │  │ Buzzer: tone     │  │   │  │
│  │   │     └────┬───┘      │   │    │   │  └────────┬─────────┘  │   │  │
│  │   └──────────┼──────────┘   │    │   └───────────┼────────────┘   │  │
│  └──────────────┼──────────────┘    └───────────────┼────────────────┘  │
│                 │                                   │                    │
│                 └───────────────────┬───────────────┘                    │
│                                     ▼                                    │
│  ┌────────────────────────────────────────────────────────────────────┐ │
│  │                    SerialBLEInterface                               │ │
│  │  • MeshCore UART Service (6E400001-B5A3-F393-E0A9-E50E24DCCA9E)    │ │
│  │  • Bitchat Service (F47B5E2D-4A9E-4C5A-9B3F-8E1D2C3A4B5C)        │ │
│  │  • Only ONE service advertised at a time                           │ │
│  │  • Auto-disconnects clients on mode switch                         │ │
│  │  • PIN auth (MeshCore) / Open access (Bitchat)                    │ │
│  └────────────────────────────────┬───────────────────────────────────┘ │
│                                   │                                      │
│                       ┌───────────▼────────────┐                        │
│                       │    BitchatBridge       │                        │
│                       │  • Encapsulate         │──► MeshCore GRP/TXT    │
│                       │  • Decapsulate         │◄── BC magic header     │
│                       │  • Loop prevention     │                        │
│                       └───────────┬────────────┘                        │
│                                   │                                      │
│                       ┌───────────▼────────────┐                        │
│                       │       MyMesh           │                        │
│                       │     (MeshCore)         │                        │
│                       └───────────┬────────────┘                        │
│                                   │                                      │
│                       ┌───────────▼────────────┐                        │
│                       │     LoRa Radio         │                        │
│                       │    (SX1262/etc)        │                        │
│                       └────────────────────────┘                        │
└──────────────────────────────────────────────────────────────────────────┘
```

### Platform Support

| Feature | nRF52 | ESP32 |
|---------|-------|-------|
| Menu-based BLE mode switching | ✅ | ✅ |
| Button-based BLE mode switching | ✅ | ✅ |
| Visual mode indicator (M/B) | ✅ | ✅ |
| LED/buzzer feedback (button-only) | ✅ | ✅ |
| Bitchat BLE service | ✅ | ✅ |
| MeshCore UART service | ✅ | ✅ |
| PIN authentication (MeshCore mode) | ✅ | ✅ |
| Open access (Bitchat mode) | ✅ | ✅ |

### With Display (Menu-Based)
Navigate to the **BITCHAT** page and press **ENTER** to toggle between modes. Display shows large **M** (MeshCore) or **B** (Bitchat).

### Button-Only (T1000-E)
Press the user button **5 times rapidly** (within ~3 seconds) to toggle modes. See [Button-Based Mode Switching](./button_ble_controller.md) for details.

## Documentation Structure

| Document | Description |
|----------|-------------|
| [Build Configuration](./build_configuration.md) | **ENABLE_BITCHAT flag**, build options, compilation settings |
| [Device Compatibility](./compatibility_devices.md) | **Complete list** of compatible/incompatible devices |
| [Button-Based Mode Switching](./button_ble_controller.md) | T1000-E and button-only device guide |
| [Protocol Specification](./protocol_specification.md) | Bitchat wire protocol format |
| [Encapsulation Format](./encapsulation_format.md) | MeshCore encapsulation header and format |
| [Payloads](./payloads.md) | Bitchat payload types and structures |
| [BLE Service](./ble_service.md) | BLE GATT service specification |
| [Architecture](./architecture.md) | Component architecture and data flow |

## Quick Start

### Enable Bitchat (For Developers)

Add to your `platformio.ini`:

```ini
build_flags =
    -D ENABLE_BITCHAT=1
    -D BLE_MODE_SWITCHING=1
```

### Check Device Compatibility

Bitchat requires **either a display with buttons** OR **button-only with LED feedback** for mode switching.

| Status | Device Examples | Count |
|--------|-----------------|-------|
| ✅ **Compatible (with display)** | Heltec V3, Wio Tracker L1, RAK4631, LilyGo T-Deck, etc. | **43 devices** |
| ✅ **Compatible (button-only)** | T1000-E | **1 device** |
| ❌ **Not Compatible** | Xiao C3/nRF52, devices without screens or buttons | Future CLI support |

**[📋 See Complete Device List](./compatibility_devices.md)** - Find your specific device

## Quick Start

### Enable Bitchat Support

```bash
# Build with Bitchat support
export ENABLE_BITCHAT=1
pio run -e WioTrackerL1_companion_radio_ble
```

### Runtime Control

Bitchat mode is controlled **on-device** via the UI or button presses:

**With Display:**
1. Navigate to **BITCHAT** page using LEFT/RIGHT buttons
2. Press **ENTER** to toggle between MeshCore and Bitchat modes
3. Display shows large **M** (MeshCore) or **B** (Bitchat)

**Button-Only (T1000-E):**
1. Press user button **5 times rapidly** (within ~3 seconds)
2. LED blinks 3 times (fast=Bitchat 150ms, slow=MeshCore 500ms)
3. Buzzer plays acknowledgment tone (if available)

**Note:** The device always boots in **MeshCore mode**. Mode is not persisted across reboots.

## Message Flow

### Bitchat → MeshCore

1. Bitchat app sends MESSAGE via BLE to `#mesh` channel
2. `BitchatBLEService` receives and parses message
3. `BitchatBridge` formats for MeshCore with `📱` prefix
4. Message sent as `PAYLOAD_TYPE_GRP_TXT` via mesh on `#mesh` channel

### MeshCore → Bitchat

1. MeshCore receives group message on `#mesh` channel
2. `BitchatBridge` verifies channel secret matches `SHA256("#mesh")`
3. `BitchatBridge` detects non-Bitchat origin (no `📱` prefix)
4. Message formatted as Bitchat MESSAGE with TLV payload
5. Sent to Bitchat app via BLE notification for `#mesh` channel

## #mesh Channel

The `#mesh` hashtag channel is the primary interoperability channel between Bitchat and MeshCore.

### Channel Key Derivation

```
#mesh secret = first_16_bytes(SHA256("#mesh"))
             = 0x5B664CDE0B08B220612113DB980650F3
```

The channel secret is the **first 16 bytes** of the SHA256 hash of the UTF-8 encoded channel name string (including the `#` prefix).

Both Bitchat Android app and MeshCore firmware derive the same channel secret using this mechanism, enabling seamless group messaging.

### Firmware Implementation

- **Secret Computation**: `BitchatBridge::computeMeshSecret()` derives the secret at initialization
- **Channel Verification**: `BitchatBridge::isMeshChannel()` verifies messages belong to `#mesh`
- **Channel Initialization**: `MyMesh::addHashtagChannel("mesh")` creates the channel on startup

See [Protocol Specification](./protocol_specification.md#hashtag-channels) for technical details.

## Known Limitations

| Issue | Status | Workaround |
|-------|--------|------------|
| BLE Notification Freeze | ⚠️ Partial | MeshCore→Bitchat notifications currently disabled on nRF52 |
| Channel Hash Mismatch | ✅ Fixed | #mesh channel verified by full 32-byte secret |
| Simultaneous BLE Services | ❌ HW Limit | Menu-based switching (both nRF52 and ESP32) |
| DM Support | 🚧 Planned | Basic infrastructure in place |
| Multiple Hashtag Channels | 🚧 Planned | Currently #mesh only |

### Menu-Based BLE Mode Switching

Due to BLE advertising size constraints, both nRF52 and ESP32 platforms use **menu-based switching** rather than simultaneous services:

**Navigation:**
1. Use **LEFT/RIGHT** keys to navigate to the **BITCHAT** page
2. Display shows:
   - **"M"** with "MeshCore" text → MeshCore mode active
   - **"B"** with "Bitchat" text → Bitchat mode active
3. Press **ENTER** to toggle between modes

**Platform-Specific Implementation:**
- **nRF52**: Switches advertisement data between Nordic UART and Bitchat service UUIDs
- **ESP32**: Uses `setBitchatMode()` to dynamically change advertised UUID via `SerialBLEInterface`

Both platforms provide identical user experience for mode switching.

## Compatibility

| Component | Compatibility |
|-----------|--------------|
| MeshCore Android App | ✅ Full (when in MeshCore BLE mode) |
| Bitchat Android App | ✅ Full (when in Bitchat BLE mode) |
| MeshCore Repeaters | ✅ No changes required |
| MeshCore Room Servers | ✅ No changes required |
| Other MeshCore Nodes | ✅ Backward compatible |

## References

- [Bitchat Android App](https://github.com/bitchat/bitchat-android)
- MeshCore Companion Protocol: [../companion_protocol.md](../companion_protocol.md)
- MeshCore Packet Structure: [../packet_structure.md](../packet_structure.md)
