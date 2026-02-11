# BitChat Integration for MeshCore

## Overview

The BitChat integration enables MeshCore devices to communicate with the BitChat Android app through a bridge layer that translates between BitChat protocol and MeshCore mesh networking. This is an **additive feature** that preserves all existing MeshCore functionality while adding BitChat compatibility.

## Key Principles

1. **Additive, Not Substitutive**: BitChat support is added alongside MeshCore, not replacing it
2. **No Infrastructure Changes**: Repeaters and room servers require no modifications
3. **Encapsulation Strategy**: BitChat messages are encapsulated in standard MeshCore packets
4. **Backward Compatible**: Existing MeshCore nodes continue to work normally

## Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                         MeshCore Device                             │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                    BLE MODE MENU                             │   │
│  │  ┌─────────────┐    ┌─────────────┐                         │   │
│  │  │  MeshCore   │ or │   BitChat   │  (switchable)           │   │
│  │  │    Mode     │◄──►│    Mode     │                         │   │
│  │  │  (PIN auth) │    │  (no auth)  │                         │   │
│  │  └──────┬──────┘    └──────┬──────┘                         │   │
│  │         │                   │                                │   │
│  │         └─────────┬─────────┘                                │   │
│  │                   ▼                                          │   │
│  │  ┌─────────────────────────────────────┐                     │   │
│  │  │      SerialBLEInterface             │                     │   │
│  │  │  • MeshCore UART (6E400001-...)    │                     │   │
│  │  │  • BitChat Service (F47B5E2D-...)  │                     │   │
│  │  │  • Only ONE active at a time       │                     │   │
│  │  └────────────────┬──────────────────┘                     │   │
│  └───────────────────┼──────────────────────────────────────────┘   │
│                      │                                              │
│              ┌───────▼────────┐                                    │
│              │ BitchatBridge  │                                    │
│              │ • Encapsulate  │──► MeshCore GRP_TXT/GRP_DATA       │
│              │ • Decapsulate  │◄── BC magic header                  │
│              │ • Loop prevent │                                    │
│              └───────┬────────┘                                    │
│                      │                                              │
│              ┌───────▼────────┐                                    │
│              │    MyMesh      │                                    │
│              │  (MeshCore)    │                                    │
│              └───────┬────────┘                                    │
│                      │                                              │
│              ┌───────▼────────┐                                    │
│              │  LoRa Radio    │                                    │
│              └────────────────┘                                    │
└─────────────────────────────────────────────────────────────────────┘
```

### Platform Support

| Feature | nRF52 | ESP32 |
|---------|-------|-------|
| Menu-based BLE mode switching | ✅ | ✅ |
| Visual mode indicator (M/B) | ✅ | ✅ |
| BitChat BLE service | ✅ | ✅ |
| MeshCore UART service | ✅ | ✅ |
| PIN authentication (MeshCore mode) | ✅ | ✅ |
| Open access (BitChat mode) | ✅ | ✅ |

Both platforms support identical menu-based switching between MeshCore and BitChat BLE modes. Navigate to the **BLE_MODE** page and press **ENTER** to toggle between modes.

## Documentation Structure

| Document | Description |
|----------|-------------|
| [Protocol Specification](./protocol_specification.md) | BitChat wire protocol format |
| [Encapsulation Format](./encapsulation_format.md) | MeshCore encapsulation header and format |
| [Payloads](./payloads.md) | BitChat payload types and structures |
| [BLE Service](./ble_service.md) | BLE GATT service specification |
| [Architecture](./architecture.md) | Component architecture and data flow |
| [Build Configuration](./build_configuration.md) | Build flags and platform configuration |

## Quick Start

### Enable BitChat Support

```bash
# Build with BitChat support
export ENABLE_BITCHAT=1
pio run -e WioTrackerL1_companion_radio_ble
```

### Runtime Control

```
# Enable BitChat
bitchat enable

# Disable BitChat
bitchat disable

# Check status
bitchat status

# List channels
bitchat channel list

# Add channel
bitchat channel add mychannel 0xABCD...
```

## Message Flow

### BitChat → MeshCore

1. BitChat app sends MESSAGE via BLE to `#mesh` channel
2. `BitchatBLEService` receives and parses message
3. `BitchatBridge` formats for MeshCore with `📱` prefix
4. Message sent as `PAYLOAD_TYPE_GRP_TXT` via mesh on `#mesh` channel

### MeshCore → BitChat

1. MeshCore receives group message on `#mesh` channel
2. `BitchatBridge` verifies channel secret matches `SHA256("#mesh")`
3. `BitchatBridge` detects non-BitChat origin (no `📱` prefix)
4. Message formatted as BitChat MESSAGE with TLV payload
5. Sent to BitChat app via BLE notification for `#mesh` channel

## #mesh Channel

The `#mesh` hashtag channel is the primary interoperability channel between BitChat and MeshCore.

### Channel Key Derivation

```
#mesh secret = first_16_bytes(SHA256("#mesh"))
             = 0x5B664CDE0B08B220612113DB980650F3
```

The channel secret is the **first 16 bytes** of the SHA256 hash of the UTF-8 encoded channel name string (including the `#` prefix).

Both BitChat Android app and MeshCore firmware derive the same channel secret using this mechanism, enabling seamless group messaging.

### Firmware Implementation

- **Secret Computation**: `BitchatBridge::computeMeshSecret()` derives the secret at initialization
- **Channel Verification**: `BitchatBridge::isMeshChannel()` verifies messages belong to `#mesh`
- **Channel Initialization**: `MyMesh::addHashtagChannel("mesh")` creates the channel on startup

See [Protocol Specification](./protocol_specification.md#hashtag-channels) for technical details.

## Known Limitations

| Issue | Status | Workaround |
|-------|--------|------------|
| BLE Notification Freeze | ⚠️ Partial | MeshCore→BitChat notifications currently disabled on nRF52 |
| Channel Hash Mismatch | ✅ Fixed | #mesh channel verified by full 32-byte secret |
| Simultaneous BLE Services | ❌ HW Limit | Menu-based switching (both nRF52 and ESP32) |
| DM Support | 🚧 Planned | Basic infrastructure in place |
| Multiple Hashtag Channels | 🚧 Planned | Currently #mesh only |

### Menu-Based BLE Mode Switching

Due to BLE advertising size constraints, both nRF52 and ESP32 platforms use **menu-based switching** rather than simultaneous services:

**Navigation:**
1. Use **LEFT/RIGHT** keys to navigate to the **BLE_MODE** page
2. Display shows:
   - **"M"** with "MeshCore" text → MeshCore mode active
   - **""** with "BitChat" text → BitChat mode active
3. Press **ENTER** to toggle between modes

**Platform-Specific Implementation:**
- **nRF52**: Switches advertisement data between Nordic UART and BitChat service UUIDs
- **ESP32**: Uses `setBitChatMode()` to dynamically change advertised UUID via `SerialBLEInterface`

Both platforms provide identical user experience for mode switching.

## Compatibility

| Component | Compatibility |
|-----------|--------------|
| MeshCore Android App | ✅ Full (when in MeshCore BLE mode) |
| BitChat Android App | ✅ Full (when in BitChat BLE mode) |
| MeshCore Repeaters | ✅ No changes required |
| MeshCore Room Servers | ✅ No changes required |
| Other MeshCore Nodes | ✅ Backward compatible |

## References

- [BitChat Android App](https://github.com/bitchat/bitchat-android)
- MeshCore Companion Protocol: [../companion_protocol.md](../companion_protocol.md)
- MeshCore Packet Structure: [../packet_structure.md](../packet_structure.md)
