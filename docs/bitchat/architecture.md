# BitChat Integration Architecture

## Overview

The BitChat integration follows a layered architecture that separates BLE communication, protocol translation, and mesh networking concerns. This document describes the component structure and data flow.

## Component Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           APPLICATION LAYER                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌─────────────────┐    ┌─────────────────┐                              │
│  │   MeshCore App  │    │   BitChat App   │                              │
│  │   (Android/iOS) │    │   (Android)     │                              │
│  └────────┬────────┘    └────────┬────────┘                              │
│           │                      │                                      │
│           │ BLE (PIN auth)       │ BLE (open)                           │
│           │ 6E400001-...         │ F47B5E2D-...                         │
│           ▼                      ▼                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                           BRIDGE LAYER                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                        BitchatBridge                                │    │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌────────────┐ │    │
│  │  │ onBitchat   │  │  onMeshcore │  │  signBitChat│  │   begin    │ │    │
│  │  │ MessageReceived│ │ GroupMessage│  │  Message    │  │   loop     │ │    │
│  │  └──────┬──────┘  └──────┬──────┘  └─────────────┘  └────────────┘ │    │
│  │         │                │                                         │    │
│  │  ┌──────▼──────┐  ┌──────▼──────┐                                  │    │
│  │  │  BitChat →  │  │  MeshCore → │                                  │    │
│  │  │  MeshCore   │  │  BitChat    │                                  │    │
│  │  │  (Encaps)   │  │  (Decaps)   │                                  │    │
│  │  └─────────────┘  └─────────────┘                                  │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────────────┐      │
│  │ BitchatBLEService│ │ ChannelRegistry │  │   LoopPrevention        │      │
│  │ (BLE Interface) │  │ (Channel Map)   │  │   (Message Dedupe)      │      │
│  └─────────────────┘  └─────────────────┘  └─────────────────────────┘      │
│                                                                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                         PROTOCOL LAYER                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌──────────────────┐ ┌──────────────────┐ ┌──────────────────────────┐     │
│  │ BitchatMessage   │ │ MessageDecapsulator│ │ BitchatMessageEncapsulator│    │
│  │ Parser           │ │                  │  │                          │     │
│  └──────────────────┘ └──────────────────┘ └──────────────────────────┘     │
│                                                                              │
│  ┌──────────────────┐ ┌──────────────────┐ ┌──────────────────────────┐     │
│  │ MessageFormatter │ │ DMEncapsulator   │ │ FragmentReassembly       │     │
│  │                  │ │ DMDecapsulator   │ │                          │     │
│  └──────────────────┘ └──────────────────┘ └──────────────────────────┘     │
│                                                                              │
│  ┌──────────────────┐ ┌──────────────────┐ ┌──────────────────────────┐     │
│  │ BitchatIdentity  │ │ HashtagKey       │ │ TimestampTranslator      │     │
│  │ (Peer ID Derive) │ │ (Key Derive)     │ │ (Time Sync)              │     │
│  └──────────────────┘ └──────────────────┘ └──────────────────────────┘     │
│                                                                              │
│  ┌──────────────────┐ ┌──────────────────┐                                  │
│  │ Compression      │ │ SignatureManager │                                  │
│  │                  │ │                  │                                  │
│  └──────────────────┘ └──────────────────┘                                  │
│                                                                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                          MESHCORE LAYER                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                              MyMesh                                  │    │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌────────────┐ │    │
│  │  │ sendFlood   │  │ createGroup │  │onChannelMsg │  │ initBitchat│ │    │
│  │  │ sendDirect  │  │ Datagram    │  │ Recv        │  │            │ │    │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └────────────┘ │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────────────┐       │
│  │ LocalIdentity   │  │ Mesh            │  │ GroupChannel            │       │
│  │ (Ed25519 keys)  │  │ (Mesh stack)    │  │ (Channel context)       │       │
│  └─────────────────┘  └─────────────────┘  └─────────────────────────┘       │
│                                                                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                         HARDWARE LAYER                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────────────┐       │
│  │ BLE Radio       │  │ LoRa Radio      │  │ Flash/Filesystem        │       │
│  │ (nRF52/ESP32)   │  │ (SX1262/etc)    │  │ (Config storage)        │       │
│  └─────────────────┘  └─────────────────┘  └─────────────────────────┘       │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Component Descriptions

### BitchatBridge

**File**: `src/helpers/bitchat/BitchatBridge.h/cpp`

Central orchestrator that connects all components. Manages:
- BLE service initialization
- Message flow routing
- Time synchronization
- Statistics tracking

Key methods:
- `begin()` - Initialize bridge components
- `loop()` - Process queued messages
- `onBitchatMessageReceived()` - Handle BitChat → MeshCore
- `onMeshcoreGroupMessage()` - Handle MeshCore → BitChat

### BitchatBLEService

**File**: `src/helpers/ble/BitchatBLEService.h/cpp`

BLE GATT service implementation for BitChat protocol:
- Handles BLE connections
- Parses incoming BitChat messages
- Sends outgoing messages via notifications
- Manages peer cache from ANNOUNCE messages

### BitchatMessageEncapsulator

**File**: `src/helpers/bitchat/BitchatMessageEncapsulator.h/cpp`

Encapsulates BitChat messages for MeshCore transport:
- Adds BitChat magic header (`BC\0\0`)
- Handles fragmentation for large messages
- Encrypts with channel secret

### MessageDecapsulator

**File**: `src/helpers/bitchat/MessageDecapsulator.h/cpp`

Decapsulates BitChat messages from MeshCore packets:
- Detects BitChat magic header
- Decrypts payload
- Handles fragment reassembly

### ChannelRegistry

**File**: `src/helpers/bitchat/ChannelRegistry.h/cpp`

Maps BitChat channel names to MeshCore group channels:
- Hashtag key derivation (first 16 bytes of SHA256)
- Bidirectional lookups (name ↔ key)
- Auto-registration of #mesh channel

### Mesh Channel Secret Storage

**File**: `src/helpers/bitchat/BitchatBridge.h/cpp`

The BitchatBridge maintains the expected `#mesh` channel secret for channel verification:

```cpp
uint8_t _meshChannelSecret[16];  // first_16_bytes(SHA256("#mesh"))
bool _hasMeshSecret;             // true after computation
```

**Methods**:
- `computeMeshSecret()` - Derives `first_16_bytes(SHA256("#mesh"))` during initialization
- `isMeshChannel(const mesh::GroupChannel& channel)` - Verifies message belongs to #mesh channel

This ensures interoperability with BitChat's `#mesh` hashtag channel implementation.

**⚠️ Memory Layout Warning**: The `mesh::GroupChannel` struct has:
- `hash[PATH_HASH_SIZE]` where `PATH_HASH_SIZE = 1` (only 1 byte!)
- `secret[PUB_KEY_SIZE]` where `PUB_KEY_SIZE = 32`

When initializing or caching the channel, be careful not to overflow:
```cpp
// Correct: Only set hash[0]
_meshChannel.hash[0] = 0xB0;

// Correct: Only copy hash when caching
_meshChannel.hash[0] = channel.hash[0];

// Dangerous: Would corrupt secret
// memcpy(_meshChannel.hash, hash, 8);  // ❌ Overflows into secret
// _meshChannel = channel;              // ❌ Overwrites our secret
```

### LoopPrevention

**File**: `src/helpers/bitchat/LoopPrevention.h/cpp`

Prevents message loops between BitChat and MeshCore:
- FNV-1a hash for message identification
- 64-entry cache with timeout
- `📱` prefix detection for BitChat origin

### BitchatIdentity

**File**: `src/helpers/bitchat/BitchatIdentity.h/cpp`

Derives BitChat peer IDs from MeshCore Ed25519 keys:
- First 8 bytes of public key (little-endian)
- Verification functions

### BitchatConfig

**File**: `src/helpers/bitchat/BitchatConfig.h/cpp`

Persistent configuration storage:
- Enable/disable state
- Filesystem persistence
- Auto-load on boot

## Data Flow

### BitChat → MeshCore

```
1. BitChat App (Android)
   └─► Writes MESSAGE to BLE characteristic

2. BitchatBLEService
   └─► Receives data in BLE callback
   └─► Appends to receive buffer
   └─► Sets pending flag

3. BitchatBridge::loop()
   └─► Detects pending data
   └─► Parses complete message
   └─► Calls onBitchatMessageReceived()

4. onBitchatMessageReceived()
   └─► Validates message type (MESSAGE=0x02)
   └─► Looks up sender from peer cache
   └─► Formats for MeshCore:
       "📱 <sender_nickname>: <payload>"
   └─► Checks cached MeshCore channel
   └─► Creates PAYLOAD_TYPE_GRP_TXT packet
   └─► Calls mesh.sendFlood()

5. MyMesh
   └─► Transmits via LoRa radio

6. Receiving Nodes
   └─► Forward via mesh (if not destination)
   └─► Or receive and display (if destination)
```

### MeshCore → BitChat

```
1. LoRa Radio
   └─► Receives packet from mesh

2. MyMesh::onChannelMessageRecv()
   └─► Decrypts packet
   └─► Calls _bitchatBridge->onMeshcoreGroupMessage()

3. BitchatBridge::onMeshcoreGroupMessage()
   └─► Checks for 📱 prefix (loop prevention)
   └─► Verifies channel is #mesh (SHA256 match)
   └─► Computes message hash
   └─► Checks LoopPrevention cache
   └─► Queues for processing (deferred)

4. BitchatBridge::loop() (deferred)
   └─► Processes outgoing queue
   └─► Builds BitChat MESSAGE structure:
       - version: 1
       - type: 0x02 (MESSAGE)
       - timestamp: current time
       - payload: "<sender>: <text>"
   └─► Signs with Ed25519
   └─► Calls _bleService.sendBitchatMessage()

5. BitchatBLEService
   └─► Serializes message to bytes
   └─► Sends BLE notification

6. BitChat App (Android)
   └─► Receives notification
   └─► Displays message in #mesh channel
```

## Class Relationships

```
┌─────────────────────────────────────────┐
│         BitchatBridge                   │
│  (owns/manages all below)               │
├─────────────────────────────────────────┤
│  BitchatBLEService _bleService          │
│  ChannelRegistry _channelRegistry       │
│  LoopPrevention _loopPrevention         │
│  BitchatMessageEncapsulator _encapsulator│
│  MessageDecapsulator _decapsulator      │
│  FragmentReassembly _fragmentReassembly │
└─────────────────────────────────────────┘
                │
                │ uses
                ▼
┌─────────────────────────────────────────┐
│         MyMesh                          │
│  (provided in constructor)              │
├─────────────────────────────────────────┤
│  mesh::Mesh& _mesh                      │
│  mesh::LocalIdentity& _identity         │
└─────────────────────────────────────────┘
```

## Memory Management

### Static Allocation Strategy

All BitChat components use static allocation (no heap):

| Component | Static Size |
|-----------|-------------|
| BitchatMessage | 2048 + 100 bytes |
| BitchatBridge | ~3KB (member buffers) |
| Peer cache | 32 × ~100 bytes |
| Fragment buffers | 16 × 256 bytes |
| Channel registry | 8 × ~64 bytes |

### Stack Safety

Large buffers are allocated as class members to prevent stack overflow:

```cpp
// In class definition (member variable)
uint8_t _signingBuffer[2048 + 64];
BitchatMessage _tempLoopMessage;

// Instead of on-stack allocation
void loop() {
    // BAD: uint8_t buffer[2048]; // Stack overflow risk
    // GOOD: Uses _signingBuffer member
}
```

## Thread Safety

The BitChat bridge operates in a single-threaded environment (Arduino-style loop). However, BLE callbacks occur in interrupt context:

### ISR-Safe Operations

- Setting flags (`volatile bool`)
- Appending to ring buffers
- Incrementing counters

### Deferred Operations

- Message parsing
- Signature verification
- Packet creation
- Mesh transmission

### Reentrance Protection

```cpp
// Global lock for BLE operations
static volatile bool _sendingInProgress;

void onBLEWrite() {
    if (_sendingInProgress) return; // Drop during send
    // ...
}
```

## Platform Abstraction

### Conditional Compilation

```cpp
#ifdef ESP32
    // ESP32-specific implementation
#elif defined(NRF52_PLATFORM)
    // nRF52-specific implementation
#else
    // Stub for native testing
#endif
```

### Platform-Specific Code

| Feature | ESP32 | nRF52 | Native |
|---------|-------|-------|--------|
| BLE | ESP-IDF GATT (NimBLE) | Nordic SoftDevice | Mock |
| BLE Callbacks | `BLECharacteristicCallbacks` | `BLECharacteristic::WriteCallback` | Stub |
| Crypto | Hardware AES | Software | Stub |
| Filesystem | SPIFFS | InternalFS | Mock |
| Mode Switching (Menu) | `setBitChatMode()` - UUID swap | Menu-based UUID swap | N/A |
| Mode Switching (Button) | `handleButtonQuintuplePress()` | `handleButtonQuintuplePress()` | N/A |

### ESP32 BLE Implementation

ESP32 uses the NimBLE stack with `BLECharacteristicCallbacks` for event handling:

```cpp
class BitchatBLEService : public BLEServiceWrapper, 
                          public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) override;
    void onRead(BLECharacteristic* pCharacteristic) override;
    void onNotify(BLECharacteristic* pCharacteristic) override;
    void onStatus(BLECharacteristic* pCharacteristic, Status s, uint32_t code) override;
};
```

**Key ESP32 Characteristics:**
- Service attaches to existing BLE server from `SerialBLEInterface`
- Uses `BLE2902` descriptor for notification enable/disable
- Write handling performs minimal work in callback (deferred processing)
- Mode switching changes advertisement UUID via `setBitChatMode()`

### nRF52 BLE Implementation

nRF52 uses Adafruit Bluefruit with callback-based write handling:

```cpp
// Bluefruit-style callback registration
_characteristic.setWriteCallback(onWriteCallback);

// ISR-safe deferred processing
static volatile bool _pendingData;
void onWriteCallback(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
    // Minimal work: copy to buffer and set flag
    _pendingData = true;
}
```

## Configuration

### Compile-Time Flags

```cpp
ENABLE_BITCHAT          // Include BitChat code
BITCHAT_DEBUG           // Enable debug logging
NRF52_PLATFORM          // nRF52-specific code
ESP32                   // ESP32-specific code
BLE_MODE_SWITCHING      // Enable menu-based BLE mode switching
```

### Button-Based Mode Switching (T1000-E)

For devices without displays (like T1000-E), mode switching is handled via button presses:

```cpp
// In UITask::handleButtonQuintuplePress()
void handleButtonQuintuplePress() {
    if (_serial) {
        bool newBitChatMode = !_serial->isBitChatMode();
        _serial->setBitChatMode(newBitChatMode);
        
        // LED feedback: 3 blinks
        // Fast (150ms) = BitChat, Slow (500ms) = MeshCore
        for (int i = 0; i < 3; i++) {
            digitalWrite(LED_PIN, HIGH);
            delay(newBitChatMode ? 150 : 500);
            digitalWrite(LED_PIN, LOW);
            delay(newBitChatMode ? 150 : 500);
        }
        
        // Buzzer acknowledgment
        notify(UIEventType::ack);
    }
}
```

**Button Action**: 5x rapid press (within ~3 seconds)
**Feedback**:
- LED: 3 blinks (fast = BitChat, slow = MeshCore)
- Buzzer: Acknowledgment tone (if available)

### Runtime Configuration

```cpp
BitchatConfig {
    enabled: bool;           // Feature on/off
    lastModified: uint32_t;  // Change timestamp
};
```

## Testing Architecture

### Unit Tests

- Native platform tests (`pio test -e native_test`)
- Mock BLE implementation
- Deterministic test vectors

### Integration Tests

- Real hardware testing (nRF52)
- Android app compatibility
- Mesh interoperability

### Test Coverage

| Component | Tests | Status |
|-----------|-------|--------|
| Identity | 9 | ✅ Pass |
| Peer Cache | 11 | ✅ Pass |
| Channel Registry | 12 | ✅ Pass |
| Message Parser | 7 | ✅ Pass |
| Encapsulator | 8 | ✅ Pass |
| Decapsulator | 7 | ✅ Pass |
| Loop Prevention | 8 | ✅ Pass |
| Fragmentation | 8 | ✅ Pass |
| Compression | 8 | ✅ Pass |
| Signature | 8 | ✅ Pass |
| Timestamp | 8 | ✅ Pass |
| DM Routing | 7 | ✅ Pass |
| Multi-Channel | 8 | ✅ Pass |
| Public Messages | 8 | ✅ Pass |
| BLE Service | 6 | ✅ Pass |
| **Total** | **125** | ✅ **Pass** |

## Future Extensions

### Planned Features

1. **Simultaneous BLE Services**: Both MeshCore and BitChat active
2. **Full DM Support**: End-to-end encrypted direct messages
3. **Multi-Channel**: Support for arbitrary BitChat channels
4. **File Transfer**: Binary data over mesh

### Extension Points

```cpp
// New message types
enum BitchatMessageType {
    // ... existing types ...
    BITCHAT_MSG_FILE_TRANSFER = 0x22,
    BITCHAT_MSG_LOCATION = 0x30,
    BITCHAT_MSG_REACTION = 0x40,
};

// New TLV types
#define BITCHAT_TLV_FILE_HASH 0x30
#define BITCHAT_TLV_LOCATION 0x31
#define BITCHAT_TLV_REACTION 0x32
```
