# BitChat BLE Service Specification

## Overview

The BitChat BLE Service provides a GATT interface for the BitChat Android app to communicate with MeshCore devices. This service operates independently from the MeshCore UART service and can be activated via menu-based switching on nRF52 platforms.

## Service UUID

| Attribute | Value |
|-----------|-------|
| Service UUID | `F47B5E2D-4A9E-4C5A-9B3F-8E1D2C3A4B5C` |
| Service Name | BitChat Service |

## GATT Characteristics

### Primary Communication Characteristic

| Attribute | Value |
|-----------|-------|
| UUID | `F47B5E2E-4A9E-4C5A-9B3F-8E1D2C3A4B5C` (TX/RX) |
| Properties | Read, Write, Notify |
| Security | None (open access) |

## BLE Mode Switching

Due to nRF52 advertising size limitations (31 bytes), the device uses menu-based switching between BLE modes:

```
MeshCore Mode          BitChat Mode
    │                      │
    ▼                      ▼
┌──────────┐           ┌──────────┐
│ Nordic   │           │ BitChat  │
│ UART     │           │ Service  │
│ 6E40...  │           │ F47B...  │
│ PIN auth │           │ Open     │
└──────────┘           └──────────┘
```

### Mode Selection Menu

Access via device UI: Home → BLE Mode → Select Mode

Visual indicators:
- **M** = MeshCore mode (Nordic UART service)
- **B** = BitChat mode (BitChat service)

## Connection Security

| Mode | Authentication | Bonding |
|------|---------------|---------|
| MeshCore | PIN required | Optional |
| BitChat | None | None |

Note: BitChat uses its own cryptographic signatures for message authenticity, not BLE security.

## Message Transport

### Receiving Messages (Android → Device)

1. Android app writes BitChat message to characteristic
2. Device receives write callback
3. Data appended to receive buffer
4. Processed in main loop (deferred processing)
5. Parsed and queued for `BitchatBridge`

### Sending Messages (Device → Android)

1. `BitchatBridge` creates BitChat MESSAGE
2. Message serialized to bytes
3. Notification sent via `characteristic.notify()`
4. Android app receives notification

## Buffer Management

### Receive Buffer

```cpp
uint8_t _writeBuffer[1024];     // Raw BLE write buffer
size_t _writeBufferOffset;      // Current write position
volatile bool _pendingData;     // Flag for loop processing
```

### Deferred Processing

Incoming BLE writes are handled via deferred processing to avoid ISR-level parsing:

1. BLE ISR appends data to `_writeBuffer`
2. Sets `_pendingData` flag
3. Main loop detects flag and processes
4. Copies to `_readBuffer` to avoid race conditions
5. Parses complete messages

## Peer Cache

The service maintains a cache of discovered peers:

```cpp
struct PeerInfo {
    uint64_t peerId;        // 8-byte peer ID
    char nickname[32];      // Display name
    uint8_t ed25519PubKey[32]; // Public key
    uint32_t lastSeen;      // Last announcement timestamp
    bool valid;             // Entry is valid
};
```

**Cache size**: 32 entries
**Eviction**: LRU (Least Recently Used)

## Announcements

### Sending Announcements

Devices periodically send ANNOUNCE messages:

```cpp
// Every 5 seconds when connected
sendAnnouncement(timestamp);
```

### Announcement Content

- Peer ID (derived from Ed25519 pubkey)
- Node nickname (from MeshCore settings)
- Ed25519 public key
- Noise protocol public key (Curve25519)

### Time Synchronization

Announcements are used for time sync:

```cpp
void syncTime(uint64_t unixTimestampMs) {
    _lastSyncTimeMs = unixTimestampMs;
    _syncLocalMillis = millis();
}
```

Time is considered valid if:
- Timestamp > January 1, 2024
- Timestamp not > 30 minutes in the future

## Message Queue

Received messages are queued for processing:

```cpp
std::queue<BitchatMessage> _recvQueue;
static constexpr size_t MAX_MESSAGE_QUEUE = 8;
```

Queue management:
- Messages added on BLE receive
- Messages removed by `BitchatBridge::loop()`
- Overflow drops oldest messages

## Connection State Tracking

```cpp
std::vector<ConnectionInfo> _connections;
uint8_t _bitchatClientCount;
bool _clientSubscribed;
uint32_t _connectionTime;
```

Connection states:
- `DISCONNECTED` - No client connected
- `CONNECTED` - Client connected, not subscribed
- `SUBSCRIBED` - Client subscribed to notifications

## Platform Implementation

### nRF52 (Adafruit Bluefruit)

```cpp
BLEService _service;
BLECharacteristic _characteristic;
```

Initialization:
```cpp
bool initNRF52(const char* deviceName) {
    // Configure BLE service
    _service.begin();
    _characteristic.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE | CHR_PROPS_NOTIFY);
    _characteristic.setPermission(SECMODE_OPEN, SECMODE_OPEN);
    _characteristic.setMaxLen(512);
    _characteristic.begin();
    
    // Set callbacks
    _characteristic.setWriteCallback(onWriteCallback);
    // ...
}
```

### ESP32

```cpp
BLEService* _platformService;
BLECharacteristic* _characteristic;
```

## Reentrance Protection

A global lock prevents BLE callback reentrance during send operations:

```cpp
static volatile bool _sendingInProgress;

// Before notify
_sendingInProgress = true;
_characteristic.notify(data, len);
_sendingInProgress = false;

// In receive callback
if (_sendingInProgress) return;
```

## MTU Considerations

| Platform | Default MTU | Max Payload |
|----------|-------------|-------------|
| nRF52 | 23 bytes | 20 bytes |
| ESP32 | 23 bytes | 20 bytes |
| With negotiation | 185-512 bytes | 182-509 bytes |

Large messages may need fragmentation at the application layer.

## Error Handling

### BLE Write Errors

| Error | Cause | Action |
|-------|-------|--------|
| Buffer overflow | Message > 1024 bytes | Truncate and log |
| Parse error | Invalid format | Drop message |
| Checksum error | Data corruption | Drop message |

### Notification Errors

| Error | Cause | Action |
|-------|-------|--------|
| Not connected | No client | Queue message |
| Not subscribed | Client not ready | Wait for subscribe |
| Buffer full | BLE stack busy | Retry with delay |

## Testing

### Connection Test

```bash
# Scan for BitChat service
hcitool lescan | grep F47B5E2D

# Connect and read characteristic
gatttool -b <MAC> --char-read -a 0x000c
```

### Message Test

```bash
# Write message to characteristic
gatttool -b <MAC> --char-write-req -a 0x000c -n <hex_data>
```

## Implementation Reference

- `src/helpers/ble/BitchatBLEService.h` - Service definition
- `src/helpers/ble/BitchatBLEService.cpp` - Platform implementations
- `src/helpers/ble/SharedBLEServer.h` - Multi-service framework
