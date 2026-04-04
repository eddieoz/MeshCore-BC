# Bitchat BLE Service Specification

## Overview

The Bitchat BLE Service provides a GATT interface for the Bitchat Android app to communicate with MeshCore devices. This service operates independently from the MeshCore UART service and can be activated via **menu-based switching** (devices with displays) or **button-based switching** (T1000-E and button-only devices).

## Service UUID

| Attribute | Value |
|-----------|-------|
| Service UUID | `F47B5E2D-4A9E-4C5A-9B3F-8E1D2C3A4B5C` |
| Service Name | Bitchat Service |

## GATT Characteristics

### Primary Communication Characteristic

| Attribute | Value |
|-----------|-------|
| UUID | `F47B5E2E-4A9E-4C5A-9B3F-8E1D2C3A4B5C` (TX/RX) |
| Properties | Read, Write, Notify |
| Security | None (open access) |

## BLE Mode Switching

Due to BLE advertising size limitations (31 bytes), both nRF52 and ESP32 platforms use **menu-based switching** between BLE modes:

```
MeshCore Mode          Bitchat Mode
    │                      │
    ▼                      ▼
┌──────────┐           ┌──────────┐
│ Nordic   │           │ Bitchat  │
│ UART     │           │ Service  │
│ 6E40...  │    ◄──►   │ F47B...  │
│ PIN auth │           │ Open     │
└──────────┘           └──────────┘
        
        (Menu-based toggle on both platforms)
```

### Mode Selection Menu

**Access via device UI:**
1. Press **LEFT/RIGHT** to navigate to the **BITCHAT** page
2. Display shows current mode:
   - **"M"** = MeshCore mode (Nordic UART service on nRF52, UART on ESP32)
   - **"B"** = Bitchat mode (Bitchat service)
3. Press **ENTER** to toggle between modes
4. Alert displays "MeshCore Mode" or "Bitchat Mode" for 1 second

### Button-Based Switching (T1000-E)

For button-only devices without displays:

**Action**: Press user button **5 times rapidly** (within ~3 seconds)

**Feedback**:
- LED: 3 blinks (fast 150ms = Bitchat, slow 500ms = MeshCore)
- Buzzer: Acknowledgment tone (if available)
- Serial: "[BLE] Switched to Bitchat/MeshCore mode"

### Platform-Specific Switching

#### nRF52

On nRF52, mode switching disconnects clients and changes the advertised service UUID:

```cpp
// Disconnect connected clients first
if (Bluefruit.connected() > 0) {
    for (uint8_t idx = 0; idx < Bluefruit.connected(); idx++) {
        BLEConnection* conn = Bluefruit.Connection(idx);
        if (conn && conn->connected()) {
            conn->disconnect();
        }
    }
    // Wait for disconnection to complete
    delay(100);
}

// Bluefruit API
Bluefruit.Advertising.stop();
Bluefruit.Advertising.clearServices();
if (bitchatMode) {
    Bluefruit.Advertising.addService(bitchatService);
} else {
    Bluefruit.Advertising.addService(bleuart);
}
Bluefruit.Advertising.start();
```

#### ESP32

On ESP32, mode switching uses `SerialBLEInterface::setBitchatMode()`:

```cpp
void SerialBLEInterface::setBitchatMode(bool enable) {
    if (_bitchatMode == enable) return;
    _bitchatMode = enable;
    
    // Disconnect any connected clients before switching modes
    // This ensures the connected smartphone immediately sees the mode change
    if (pServer->getConnectedCount() > 0) {
        // Disconnect all connected clients
        for (int i = 0; i < pServer->getConnectedCount(); i++) {
            uint16_t connId = pServer->getConnIdByIndex(0);
            if (connId != BLE_HS_CONN_HANDLE_NONE) {
                pServer->disconnect(connId);
            }
        }
        // Wait for disconnection to complete
        delay(100);
    }
    
    // Stop advertising
    pServer->getAdvertising()->stop();
    
    // Change advertised UUID
    BLEAdvertisementData oAdvertisementData;
    if (_bitchatMode && _bitchatService != nullptr) {
        oAdvertisementData.setCompleteServices(BLEUUID(_bitchatService->getUUID()));
    } else {
        oAdvertisementData.setCompleteServices(BLEUUID(SERVICE_UUID));
    }
    
    // Restart with new UUID
    pServer->getAdvertising()->setAdvertisementData(oAdvertisementData);
    pServer->getAdvertising()->start();
}
```

### Auto-Disconnect on Mode Switch

When switching modes, the device **automatically disconnects all connected BLE clients** before changing the advertised service. This ensures:

1. **Immediate mode enforcement**: Connected phones cannot continue using the old mode
2. **Clean service transition**: Phone must reconnect to the new service
3. **No cross-mode contamination**: MeshCore app can't send after switching to Bitchat

**Implementation details:**
- Disconnect happens **before** advertising stops
- Up to 2 second timeout waiting for disconnect
- Force clears internal connection state
- Advertising restarts with new service UUID

**User experience:**
- Phone immediately loses connection
- Must reconnect to use new mode
- No manual disconnect required

## Connection Security

| Mode | Authentication | Bonding |
|------|---------------|---------|
| MeshCore | PIN required | Optional |
| Bitchat | None | None |

Note: Bitchat uses its own cryptographic signatures for message authenticity, not BLE security.

## Message Transport

### Receiving Messages (Android → Device)

1. Android app writes Bitchat message to characteristic
2. Device receives write callback
3. Data appended to receive buffer
4. Processed in main loop (deferred processing)
5. Parsed and queued for `BitchatBridge`

### Sending Messages (Device → Android)

1. `BitchatBridge` creates Bitchat MESSAGE
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

Initialization:
```cpp
bool initESP32(const char* deviceName) {
    // Get existing BLE server from SerialBLEInterface
    BLEServer* server = BLEDevice::getServer();
    
    // Create Bitchat service
    _platformService = server->createService(BITCHAT_SERVICE_UUID);
    
    // Create characteristic with READ, WRITE, NOTIFY properties
    _characteristic = _platformService->createCharacteristic(
        BITCHAT_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    
    // Add CCCD descriptor for notifications
    _characteristic->addDescriptor(new BLE2902());
    
    // Set callbacks for write/read events
    _characteristic->setCallbacks(this);
    
    // Start the service
    _platformService->start();
    return true;
}
```

#### ESP32 BLECharacteristicCallbacks

The ESP32 implementation uses `BLECharacteristicCallbacks` for event handling:

```cpp
class BitchatBLEService : public BLEServiceWrapper, 
                          public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) override;
    void onRead(BLECharacteristic* pCharacteristic) override;
    void onNotify(BLECharacteristic* pCharacteristic) override;
    void onStatus(BLECharacteristic* pCharacteristic, Status s, uint32_t code) override;
};
```

#### ESP32 Write Handling

```cpp
void BitchatBLEService::onWrite(BLECharacteristic* pCharacteristic) {
    // MINIMAL WORK IN CALLBACK - BLE stack has limited stack space!
    std::string value = pCharacteristic->getValue();
    if (value.empty()) return;
    
    const uint8_t* data = reinterpret_cast<const uint8_t*>(value.data());
    size_t length = value.length();
    
    _lastWriteTime = millis();
    _pendingData = true;  // Flag for loop() to process
    
    // Append to write buffer with overflow protection
    if (_writeBufferOffset + length > sizeof(_writeBuffer)) {
        clearWriteBuffer();
    }
    memcpy(&_writeBuffer[_writeBufferOffset], data, length);
    _writeBufferOffset += length;
}
```

#### ESP32 Notification Sending

```cpp
bool BitchatBLEService::sendNotification(uint16_t connectionId, 
                                          const uint8_t* data, 
                                          size_t len) {
    if (_characteristic) {
        _characteristic->setValue(const_cast<uint8_t*>(data), len);
        _characteristic->notify();
        return true;
    }
    return false;
}
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
# Scan for Bitchat service
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
