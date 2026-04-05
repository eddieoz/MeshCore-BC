# Bitchat Integration Guide for MeshCore 1.14.1

> **Target Audience**: Firmware developers integrating Bitchat into new MeshCore versions  
> **Version**: 1.14.1  
> **Approach**: Subclassing Architecture & Separate Examples

---

## Overview

This guide documents the complete Bitchat integration implementation on MeshCore firmware version 1.14.1. The integration uses **Subclassing Architecture** which isolates Bitchat functionality to minimize merge conflicts during future firmware updates.

### Key Achievement

By isolating Bitchat logic into subclasses, integrating Bitchat into future versions (e.g., 1.15.0) is as simple as copying isolated directories instead of dealing with complex git merges.

---

## Architecture Philosophy

### Approach: Subclassing & Separate Examples

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         SUBCLASSING ARCHITECTURE                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  CORE MESHCORE (Untouched)                                                   │
│  ├── src/helpers/BaseChatMesh.h          ◄── Only 2 files minimally modified│
│  ├── src/helpers/BaseSerialInterface.h   ◄── Backward-compatible additions  │
│  └── examples/companion_radio/           ◄── Completely untouched           │
│                                                                              │
│  BITCHAT EXTENSIONS (Isolated)                                               │
│  ├── src/helpers/bitchat/                ◄── 25+ source files               │
│  ├── src/helpers/ble/                    ◄── BLE service implementations    │
│  ├── examples/bitchat_companion/         ◄── Standalone example             │
│  └── test/bitchat/                       ◄── 38 test files                  │
│                                                                              │
│  SUBCLASS HIERARCHY                                                          │
│  ┌─────────────────┐                                                         │
│  │   BaseChatMesh  │  (Core - untouched)                                     │
│  └────────┬────────┘                                                         │
│           │                                                                  │
│  ┌────────▼────────┐     ┌─────────────────┐                                 │
│  │   BitchatMesh   │◄────┤  BitchatBridge  │  (Integration layer)           │
│  └────────┬────────┘     └─────────────────┘                                 │
│           │                                                                  │
│  ┌────────▼────────┐                                                         │
│  │     MyMesh      │  (Application)                                          │
│  └─────────────────┘                                                         │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Benefits of `Subclassing Architecture

| Benefit | Description |
|---------|-------------|
| **No Merge Conflicts** | Core files are not modified, only extended |
| **Future-Proof** | Copy `src/helpers/bitchat/` and `examples/bitchat_companion/` to new version |
| **Testable** | Comprehensive test suite in `test/bitchat/` |
| **Safe** | Compile-time control via `ENABLE_BITCHAT` flag |

---

## Directory Structure

### Complete File Tree

```
src/helpers/bitchat/                    # 25 files, ~13K lines
├── BitchatAnnounce.cpp/h              # Peer announcement handling
├── BitchatBridge.cpp/h                # Main integration bridge ⭐
├── BitchatCLI.cpp/h                   # Command-line interface
├── BitchatConfig.cpp/h                # Configuration management
├── BitchatIdentity.cpp/h              # Identity/key management
├── BitchatMesh.cpp/h                  # Mesh subclass ⭐
├── BitchatMessageEncapsulator.cpp/h   # Message encapsulation
├── BitchatMessageParser.cpp/h         # Message parsing
├── BitchatPeerCache.cpp/h             # Peer caching
├── ChannelRegistry.cpp/h              # Hashtag channel management
├── Compression.cpp/h                  # Payload compression
├── DMDecapsulator.cpp/h               # DM message decapsulation
├── DMEncapsulator.cpp/h               # DM message encapsulation
├── FragmentReassembly.cpp/h           # Message fragmentation
├── HashtagKey.cpp/h                   # Hashtag key derivation
├── LoopPrevention.cpp/h               # Duplicate prevention
├── MessageDecapsulator.cpp/h          # Message decapsulation
├── MessageFormatter.cpp/h             # Message formatting
├── MultiChannelRouter.cpp/h           # Multi-channel routing
├── ProtocolTestVectors.cpp/h          # Test vectors
├── PublicMessageHandler.cpp/h         # Public message handling
├── Signature.cpp/h                    # Ed25519 signatures
└── TimestampTranslator.cpp/h          # Time synchronization

src/helpers/ble/                        # BLE service layer
├── BitchatBLEService.cpp/h            # BLE GATT service ⭐
└── SharedBLEServer.h                  # Shared BLE infrastructure

examples/bitchat_companion/             # Standalone example
├── main.cpp                           # Application entry
├── MyMesh.cpp/h                       # Application mesh class
├── DataStore.cpp/h                    # Persistent storage
├── AbstractUITask.h                   # UI abstraction
├── NodePrefs.h                        # Node preferences
└── ui-new/, ui-orig/                  # UI implementations

test/bitchat/                           # 38 test files
├── test_bitchat_mesh.cpp              # Subclass tests
├── test_bitchat_bridge.cpp            # Bridge tests
├── test_bitchat_ble_service.cpp       # BLE service tests
├── test_e2e_message_flow.cpp          # End-to-end tests
└── [34 more test files...]            # Comprehensive coverage
```

---

## Core Integration Points

### 1. BaseChatMesh.h Modifications

**Changes**: Moved `channels` and `num_channels` from `private` to `protected`

```cpp
// Before (private):
class BaseChatMesh : public mesh::Mesh {
  ChannelDetails channels[MAX_GROUP_CHANNELS];  // private
  int num_channels;

// After (protected):
class BaseChatMesh : public mesh::Mesh {
protected:
#ifdef MAX_GROUP_CHANNELS
  ChannelDetails channels[MAX_GROUP_CHANNELS];  // protected
  int num_channels;
#endif
```

**Rationale**: Allows `BitchatMesh` subclass to access channel management for hashtag channel registration.

**Impact**: Zero breaking changes - only increases visibility.

### 2. BaseSerialInterface.h Additions

**Changes**: Added virtual methods for BitChat mode support

```cpp
class BaseSerialInterface {
public:
  // ... existing methods ...
  
  // BitChat mode support (optional, platforms may override)
  virtual bool isBitChatMode() const { return false; }
  virtual void setBitChatMode(bool enable) { (void)enable; }
};
```

**Rationale**: Allows `SerialBLEInterface` to track and report BitChat mode state.

**Impact**: Default implementations are no-ops, fully backward-compatible.

### 3. BitchatMesh Subclass

**Purpose**: Extend BaseChatMesh with BitChat-specific functionality

```cpp
#pragma once
#include "helpers/BaseChatMesh.h"

class BitchatBridge; // Forward declaration

namespace mesh {
namespace bitchat {

class BitchatMesh : public BaseChatMesh {
    bool _bitchatMode;
    BitchatBridge* _bridge;

protected:
    BitchatMesh(mesh::Radio& radio, mesh::MillisecondClock& ms, 
                mesh::RNG& rng, mesh::RTCClock& rtc, 
                mesh::PacketManager& mgr, mesh::MeshTables& tables)
        : BaseChatMesh(radio, ms, rng, rtc, mgr, tables), 
          _bitchatMode(true), _bridge(nullptr) {}

public:
    void setBitchatMode(bool enabled) { _bitchatMode = enabled; }
    bool isBitchatMode() const { return _bitchatMode; }
    void setBitchatBridge(BitchatBridge* bridge) { _bridge = bridge; }
    BitchatBridge* getBitchatBridge() const { return _bridge; }

    // Add hashtag channel with secret
    ChannelDetails* addHashtagChannel(const char* name, 
                                       const uint8_t* secret, 
                                       uint8_t secret_len);

    // Override to intercept group messages
    void onChannelMessageRecv(const mesh::GroupChannel& channel, 
                               mesh::Packet* pkt, 
                               uint32_t timestamp, 
                               const char* text) override;
};

} // namespace bitchat
} // namespace mesh
```

### 4. BitchatBridge Integration

**Purpose**: Bidirectional message routing between MeshCore and BitChat

```cpp
class BitchatBridge {
public:
    BitchatBridge(mesh::Mesh &mesh, mesh::LocalIdentity &identity, 
                  const char *nodeName);
    
    void begin();                    // Initialize bridge
    void loop();                     // Process messages
    
    // MeshCore → BitChat
    void onMeshcoreGroupMessage(const mesh::GroupChannel &channel, 
                                 uint32_t timestamp, 
                                 const char *senderName,
                                 const char *text);
    
    // BitChat → MeshCore
    void onBitchatMessageReceived(const mesh::ble::BitchatMessage &msg);
    
    // Signature handling
    void onBitchatSignRequest(uint8_t *sig, const uint8_t *msg, size_t len);

private:
    mesh::Mesh &_mesh;
    mesh::LocalIdentity &_identity;
    mesh::ble::BitchatBLEService _bleService;
    mesh::bitchat::ChannelRegistry _channelRegistry;
    mesh::bitchat::LoopPrevention _loopPrevention;
    // ... additional components ...
};
```

### 5. Application Integration (main.cpp)

```cpp
#if defined(ENABLE_BITCHAT) && (defined(ESP32) || defined(NRF52_PLATFORM))
#include <helpers/bitchat/BitchatBridge.h>
BitchatBridge bitchat_bridge(the_mesh, the_mesh.self_id, 
                             the_mesh.getNodeName());
#endif

// ... in setup() ...
#ifdef ENABLE_BITCHAT
  bitchat_bridge.begin();
  if (bitchat_bridge.beginStandalone(the_mesh.getNodeName())) {
    Serial.println("[BitChat] BLE advertising started.");
  }
  the_mesh.initBitchat(&bitchat_bridge);
#endif
```

### 6. MyMesh Inheritance

```cpp
#include <helpers/bitchat/BitchatMesh.h>

class MyMesh : public mesh::bitchat::BitchatMesh, 
               public DataStoreHost {
    // Application-specific implementation
};
```

---

## Build Configuration

### PlatformIO Configuration

```ini
[nrf52_base]
extends = arduino_base
platform = nordicnrf52
build_flags = ${arduino_base.build_flags}
  -D NRF52_PLATFORM
  -D ENABLE_BITCHAT=1        ; Enable BitChat support
  -D LFS_NO_ASSERT=1
  -D EXTRAFS=1
```

### Source Filter

```ini
build_src_filter =
  +<*.cpp>
  +<helpers/*.cpp>
  +<helpers/radiolib/*.cpp>
  +<helpers/bitchat/*.cpp>    ; Include BitChat modules
  +<helpers/ble/*.cpp>        ; Include BLE services
  +<helpers/bridges/BridgeBase.cpp>
  +<helpers/ui/MomentaryButton.cpp>
```

---

## Test Suite

### Running Tests

```bash
# Run native BitChat tests
pio test -e native_bitchat
```

### Test Coverage

| Category | Count | Status |
|----------|-------|--------|
| Unit Tests | 228 | ✅ All Passing |
| Integration Tests | 15+ | ✅ All Passing |
| E2E Tests | 6 | ✅ All Passing |

### Key Test Areas

- `test_bitchat_mesh.cpp` - Subclass functionality
- `test_bitchat_bridge.cpp` - Message routing
- `test_bitchat_ble_service.cpp` - BLE protocol
- `test_e2e_message_flow.cpp` - End-to-end flows
- `test_loop_prevention.cpp` - Duplicate detection
- `test_channel_registry.cpp` - Channel management

---

## Memory Considerations

### Stack Overflow Prevention

Critical fixes implemented in 1.14.1:

```cpp
// ❌ BAD: 2KB+ on stack
void processMessage() {
    BitchatMessage msg;  // ~2KB stack allocation
    // ...
}

// ✅ GOOD: Member variable
class BitchatBridge {
    BitchatMessage _tempLoopMessage;  // Heap allocation
    void loop() {
        // Use _tempLoopMessage
    }
};
```

### Memory Usage (Wio Tracker L1)

```
RAM:   60.4% (142,248 / 235,520 bytes)
Flash: 68.0% (482,088 / 708,608 bytes)
```

**Constraints for future updates**:
- Maintain static buffers over stack allocation
- Use deferred processing in main loop
- Keep loop() operations non-blocking

---

## Porting to New Firmware Versions

### Step-by-Step Migration Guide

#### 1. Copy BitChat Source Files

```bash
# From old version to new version
cp -r src/helpers/bitchat/ new-version/src/helpers/
cp -r src/helpers/ble/ new-version/src/helpers/
cp -r examples/bitchat_companion/ new-version/examples/
cp -r test/bitchat/ new-version/test/
```

#### 2. Apply Core Modifications

Modify these 2 files in the new version:

**src/helpers/BaseChatMesh.h**:
```cpp
protected:
#ifdef MAX_GROUP_CHANNELS
  ChannelDetails channels[MAX_GROUP_CHANNELS];
  int num_channels;
#endif
```

**src/helpers/BaseSerialInterface.h**:
```cpp
// BitChat mode support (optional, platforms may override)
virtual bool isBitChatMode() const { return false; }
virtual void setBitChatMode(bool enable) { (void)enable; }
```

#### 3. Update Build Configuration

Add to `platformio.ini`:
```ini
build_src_filter =
  +<helpers/bitchat/*.cpp>
  +<helpers/ble/*.cpp>
```

Add to nRF52 base config:
```ini
[nrf52_base]
build_flags = 
  -D ENABLE_BITCHAT=1
```

#### 4. Verify Build

```bash
pio run -e seeed-wio-tracker-l1
pio test -e native_bitchat
```

#### 5. Handle API Changes

If MeshCore core API changed:
1. Check `BitchatMesh` overrides match new signatures
2. Update `BitchatBridge` if `Mesh` class interface changed
3. Verify `BaseChatMesh` protected members still accessible

---

## Common Integration Issues

### Issue 1: Private Member Access

**Symptom**: Compilation error accessing `channels` or `num_channels`

**Solution**: Ensure `BaseChatMesh.h` has protected section:
```cpp
protected:
  ChannelDetails channels[MAX_GROUP_CHANNELS];
  int num_channels;
```

### Issue 2: Stack Overflow

**Symptom**: Device freezes or crashes during message processing

**Solution**: Verify no large stack allocations:
```cpp
// Check for patterns like:
uint8_t buffer[2048];  // ❌ Too large for stack

// Replace with member variable:
uint8_t _buffer[2048];  // ✅ Class member (heap)
```

### Issue 3: BLE Service Conflicts

**Symptom**: BLE connection issues or advertisement failures

**Solution**: Verify `SharedBLEServer` integration and proper service switching.

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.12.0 | Original | Initial BitChat implementation (hardcoded) |
| 1.13.0 | Previous | Added more devices, ESP32 support, Automated build on Github |
| 1.14.1 | Current | Approach subclass-based refactoring, 228 tests |
| 1.15.0 | Future | TBD - use migration guide above |

---

## References

- [Architecture Overview](../architecture.md)
- [BLE Service Specification](../ble_service.md)
- [Protocol Specification](../protocol_specification.md)
- [Build Configuration](../build_configuration.md)
- [Code Review Report](./code-review-1.14.1.md)
- [Migration Guide](./migration-guide.md)

---

## Contact & Support

For issues with BitChat integration:
1. Check [GitHub Issues](../../../issues)
2. Review test suite: `pio test -e native_bitchat`
3. Verify build: `pio run -e seeed-wio-tracker-l1`

---

*Last Updated: April 2026*  
*Firmware Version: 1.14.1*
