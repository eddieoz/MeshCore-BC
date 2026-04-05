# BitChat Integration for MeshCore

> **Firmware Version**: 1.14.1  
> **Approach**: Subclassing & Separate Examples
> **Status**: Production Ready - 228/228 Tests Passing

## Overview

The BitChat integration enables MeshCore devices to communicate with the [BitChat Android app](https://github.com/bitchat/bitchat-android) through a bridge layer that translates between BitChat protocol and MeshCore mesh networking. This is an **additive feature** that preserves all existing MeshCore functionality while adding BitChat compatibility.

## Key Principles

1. **Additive, Not Substitutive**: BitChat support is added alongside MeshCore, not replacing it
2. **No Infrastructure Changes**: Repeaters and room servers require no modifications
3. **Encapsulation Strategy**: BitChat messages are encapsulated in standard MeshCore packets
4. **Backward Compatible**: Existing MeshCore nodes continue to work normally
5. **Isolated Implementation**: BitChat logic in separate classes for easy future updates

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
│  │  • BitChat Service (F47B5E2D-4A9E-4C5A-9B3F-8E1D2C3A4B5C)        │ │
│  │  • Only ONE service advertised at a time                           │ │
│  │  • Auto-disconnects clients on mode switch                         │ │
│  │  • PIN auth (MeshCore) / Open access (BitChat)                    │ │
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

### Approach: Subclassing Architecture (1.14.1)

This version uses **Subclassing Architecture** for cleaner integration:

```
┌─────────────────┐
│   BaseChatMesh  │  (MeshCore Core - unchanged)
└────────┬────────┘
         │
┌────────▼────────┐     ┌─────────────────┐
│   BitchatMesh   │◄────┤  BitchatBridge  │  (BitChat Extensions)
└────────┬────────┘     └─────────────────┘
         │
┌────────▼────────┐
│     MyMesh      │  (Application Layer)
└─────────────────┘
```

**Benefits:**
- Only 2 core files modified (backward-compatible additions)
- Easy migration to future MeshCore versions (1.15.0, etc.)
- Comprehensive test suite (228 tests)
- Zero merge conflicts

### Platform Support

| Feature | nRF52 | ESP32 |
|---------|-------|-------|
| Menu-based BLE mode switching | ✅ | ✅ |
| Button-based BLE mode switching | ✅ | ✅ |
| Visual mode indicator (M/B) | ✅ | ✅ |
| LED/buzzer feedback (button-only) | ✅ | ✅ |
| BitChat BLE service | ✅ | ✅ |
| MeshCore UART service | ✅ | ✅ |
| PIN authentication (MeshCore mode) | ✅ | ✅ |
| Open access (BitChat mode) | ✅ | ✅ |

### With Display (Menu-Based)
Navigate to the **BITCHAT** page and press **ENTER** to toggle between modes. Display shows large **M** (MeshCore) or **B** (BitChat).

### Button-Only (T1000-E)
Press the user button **5 times rapidly** (within ~3 seconds) to toggle modes. See [Button-Based Mode Switching](docs/bitchat/button_ble_controller.md) for details.

## Documentation Structure

### User Documentation

| Document | Description |
|----------|-------------|
| [Build Configuration](docs/bitchat/build_configuration.md) | **ENABLE_BITCHAT flag**, build options, compilation settings |
| [Device Compatibility](docs/bitchat/compatibility_devices.md) | **Complete list** of compatible/incompatible devices |
| [Button-Based Mode Switching](docs/bitchat/button_ble_controller.md) | T1000-E and button-only device guide |
| [Protocol Specification](docs/bitchat/protocol_specification.md) | BitChat wire protocol format |
| [Encapsulation Format](docs/bitchat/encapsulation_format.md) | MeshCore encapsulation header and format |
| [Payloads](docs/bitchat/payloads.md) | BitChat payload types and structures |
| [BLE Service](docs/bitchat/ble_service.md) | BLE GATT service specification |
| [Architecture](docs/bitchat/architecture.md) | Component architecture and data flow |

### Developer Documentation

| Document | Description |
|----------|-------------|
| [Integration Guide (1.14.1)](docs/bitchat/developer/integration-guide-1.14.1.md) | **Complete integration guide** for developers |
| [Code Review Report](docs/bitchat/developer/code-review-1.14.1.md) | Code review findings and security analysis |
| [Migration Guide](docs/bitchat/developer/migration-guide.md) | **Porting to new MeshCore versions** |

## Quick Start

### Enable BitChat Support

Add to your `platformio.ini`:

```ini
build_flags =
    -D ENABLE_BITCHAT=1
    -D BLE_MODE_SWITCHING=1
```

### Build for Target Device

```bash
# Build for Wio Tracker L1
pio run -e seeed-wio-tracker-l1

# Run test suite
pio test -e native_bitchat
```

### Runtime Control

BitChat mode is controlled **on-device** via the UI or button presses:

**With Display:**
1. Navigate to **BITCHAT** page using LEFT/RIGHT buttons
2. Press **ENTER** to toggle between MeshCore and BitChat modes
3. Display shows large **M** (MeshCore) or **B** (BitChat)

**Button-Only (T1000-E):**
1. Press user button **5 times rapidly** (within ~3 seconds)
2. LED blinks 3 times (fast=Bitchat 150ms, slow=MeshCore 500ms)
3. Buzzer plays acknowledgment tone (if available)

**Note:** The device always boots in **MeshCore mode**. Mode is not persisted across reboots.

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

## Known Limitations

| Issue | Status | Workaround |
|-------|--------|------------|
| BLE Notification Freeze | ⚠️ Partial | MeshCore→BitChat notifications currently disabled on nRF52 |
| Channel Hash Mismatch | ✅ Fixed | #mesh channel verified by full 32-byte secret |
| Simultaneous BLE Services | ❌ HW Limit | Menu-based switching (both nRF52 and ESP32) |
| DM Support | 🚧 Planned | Basic infrastructure in place |
| Multiple Hashtag Channels | 🚧 Planned | Currently #mesh only |

## Compatibility

| Component | Compatibility |
|-----------|--------------|
| MeshCore Android App | ✅ Full (when in MeshCore BLE mode) |
| BitChat Android App | ✅ Full (when in BitChat BLE mode) |
| MeshCore Repeaters | ✅ No changes required |
| MeshCore Room Servers | ✅ No changes required |
| Other MeshCore Nodes | ✅ Backward compatible |

## Future Support for Display-Less Devices

Devices without displays or buttons may receive BitChat support through:

- **CLI Configuration**: `bitchat enable` command via serial
- **Startup Mode**: Default to BitChat mode on boot
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
3. Report results in [GitHub Issues](https://github.com/eddieoz/MeshCore-BC/issues)

Your feedback helps move devices from "build-verified" to "real-world tested"!

---

# About MeshCore

MeshCore is a lightweight, portable C++ library that enables multi-hop packet routing for embedded projects using LoRa and other packet radios. It is designed for developers who want to create resilient, decentralized communication networks that work without the internet.

## 🔍 What is MeshCore?

MeshCore provides the ability to create wireless mesh networks, similar to Meshtastic and Reticulum but with a focus on lightweight multi-hop packet routing for embedded projects. Unlike Meshtastic, which is tailored for casual LoRa communication, or Reticulum, which offers advanced networking, MeshCore balances simplicity with scalability, making it ideal for custom embedded solutions., where devices (nodes) can communicate over long distances by relaying messages through intermediate nodes. This is especially useful in off-grid, emergency, or tactical situations where traditional communication infrastructure is unavailable.

## ⚡ Key Features

* Multi-Hop Packet Routing
  * Devices can forward messages across multiple nodes, extending range beyond a single radio's reach.
  * Supports up to a configurable number of hops to balance network efficiency and prevent excessive traffic.
  * Nodes use fixed roles where "Companion" nodes are not repeating messages at all to prevent adverse routing paths from being used.
* Supports LoRa Radios – Works with Heltec, RAK Wireless, and other LoRa-based hardware.
* Decentralized & Resilient – No central server or internet required; the network is self-healing.
* Low Power Consumption – Ideal for battery-powered or solar-powered devices.
* Simple to Deploy – Pre-built example applications make it easy to get started.

## 🎯 What Can You Use MeshCore For?

* Off-Grid Communication: Stay connected even in remote areas.
* Emergency Response & Disaster Recovery: Set up instant networks where infrastructure is down.
* Outdoor Activities: Hiking, camping, and adventure racing communication.
* Tactical & Security Applications: Military, law enforcement, and private security use cases.
* IoT & Sensor Networks: Collect data from remote sensors and relay it back to a central location.

## 🚀 How to Get Started

- Watch the [MeshCore Intro Video](https://www.youtube.com/watch?v=t1qne8uJBAc) by Andy Kirby.
- Watch the [MeshCore Technical Presentation](https://www.youtube.com/watch?v=OwmkVkZQTf4) by Liam Cottle.
- Read through our [Frequently Asked Questions](./docs/faq.md) and [Documentation](https://docs.meshcore.io).
- Flash the MeshCore firmware on a supported device.
- Connect with a supported client.

For developers;

- Install [PlatformIO](https://docs.platformio.org) in [Visual Studio Code](https://code.visualstudio.com).
- Clone and open the MeshCore repository in Visual Studio Code.
- See the example applications you can modify and run:
  - [Companion Radio](./examples/companion_radio) - For use with an external chat app, over BLE, USB or WiFi.
  - [KISS Modem](./examples/kiss_modem) - Serial KISS protocol bridge for host applications. ([protocol docs](./docs/kiss_modem_protocol.md))
  - [Simple Repeater](./examples/simple_repeater) - Extends network coverage by relaying messages.
  - [Simple Room Server](./examples/simple_room_server) - A simple BBS server for shared Posts.
  - [Simple Secure Chat](./examples/simple_secure_chat) - Secure terminal based text communication between devices.
  - [Simple Sensor](./examples/simple_sensor) - Remote sensor node with telemetry and alerting.

The Simple Secure Chat example can be interacted with through the Serial Monitor in Visual Studio Code, or with a Serial USB Terminal on Android.

## ⚡️ MeshCore Flasher

We have prebuilt firmware ready to flash on supported devices.

- Launch https://meshcore.io/flasher
- Select a supported device
- Flash one of the firmware types:
  - Companion, Repeater or Room Server
- Once flashing is complete, you can connect with one of the MeshCore clients below.

## 📱 MeshCore Clients

**Companion Firmware**

The companion firmware can be connected to via BLE, USB or WiFi depending on the firmware type you flashed.

- Web: https://app.meshcore.nz
- Android: https://play.google.com/store/apps/details?id=com.liamcottle.meshcore.android
- iOS: https://apps.apple.com/us/app/meshcore/id6742354151?platform=iphone
- NodeJS: https://github.com/liamcottle/meshcore.js
- Python: https://github.com/fdlamotte/meshcore-cli

**Repeater and Room Server Firmware**

The repeater and room server firmwares can be setup via USB in the web config tool.

- https://config.meshcore.io

They can also be managed via LoRa in the mobile app by using the Remote Management feature.

## 🛠 Hardware Compatibility

MeshCore is designed for devices listed in the [MeshCore Flasher](https://meshcore.io/flasher)

## 📜 License

MeshCore is open-source software released under the MIT License. You are free to use, modify, and distribute it for personal and commercial projects.

## Contributing

Please submit PR's using 'dev' as the base branch!
For minor changes just submit your PR and we'll try to review it, but for anything more 'impactful' please open an Issue first and start a discussion. Is better to sound out what it is you want to achieve first, and try to come to a consensus on what the best approach is, especially when it impacts the structure or architecture of this codebase.

Here are some general principals you should try to adhere to:
* Keep it simple. Please, don't think like a high-level lang programmer. Think embedded, and keep code concise, without any unnecessary layers.
* No dynamic memory allocation, except during setup/begin functions.
* Use the same brace and indenting style that's in the core source modules. (A .clang-format is prob going to be added soon, but please do NOT retroactively re-format existing code. This just creates unnecessary diffs that make finding problems harder)

Help us prioritize! Please react with thumbs-up to issues/PRs you care about most. We look at reaction counts when planning work.

## Road-Map / To-Do

There are a number of fairly major features in the pipeline, with no particular time-frames attached yet. In very rough chronological order:
- [X] Companion radio: UI redesign
- [X] Repeater + Room Server: add ACL's (like Sensor Node has)
- [X] Standardise Bridge mode for repeaters
- [X] BitChat integration: BLE bridge for BitChat app compatibility (1.14.1 - Subclassing Architecture)
- [ ] Repeater/Bridge: Standardise the Transport Codes for zoning/filtering
- [X] Core + Repeater: enhanced zero-hop neighbour discovery
- [ ] Core: round-trip manual path support
- [ ] Companion + Apps: support for multiple sub-meshes (and 'off-grid' client repeat mode)
- [ ] Core + Apps: support for LZW message compression
- [ ] Core: dynamic CR (Coding Rate) for weak vs strong hops
- [ ] Core: new framework for hosting multiple virtual nodes on one physical device
- [ ] V2 protocol spec: discussion and consensus around V2 packet protocol, including path hashes, new encryption specs, etc

## 📞 Get Support

- Report bugs and request features on the [GitHub Issues](https://github.com/eddieoz/MeshCore-BC/issues) page.
- Find additional guides and components on [MeshCore Documentation](https://docs.meshcore.io).
- Join [MeshCore Discord](https://discord.gg/BMwCtwHj5V) to chat with the developers and get help from the community.

---

**Version**: 1.14.1  
**Approach**: Subclassing Architecture  
**Test Status**: 228/228 passing  
**Last Updated**: April 2026
