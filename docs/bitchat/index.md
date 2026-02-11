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

1. BitChat app sends MESSAGE via BLE
2. `BitchatBLEService` receives and parses message
3. `BitchatBridge` formats for MeshCore with `📱` prefix
4. Message sent as `PAYLOAD_TYPE_GRP_TXT` via mesh

### MeshCore → BitChat

1. MeshCore receives group message
2. `BitchatBridge` detects non-BitChat origin (no `📱` prefix)
3. Message formatted as BitChat MESSAGE with TLV payload
4. Sent to BitChat app via BLE notification

## Known Limitations

| Issue | Status | Workaround |
|-------|--------|------------|
| BLE Notification Freeze | ⚠️ Partial | MeshCore→BitChat notifications currently disabled on nRF52 |
| Channel Hash Mismatch | ✅ Fixed | Accept all channel messages (MeshCore uses secret hash) |
| Simultaneous BLE Services | ❌ nRF52 Limit | Menu-based switching instead |
| DM Support | 🚧 Planned | Basic infrastructure in place |

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
