#include "MeshCoreUARTService.h"

#ifdef NATIVE_TEST
  #include <cstring>
  #include <cstdio>
  // Forward declaration for native tests (no real SerialBLEInterface)
  class SerialBLEInterface {};
#else
  #ifdef ESP32
    #include <Arduino.h>
    #include <helpers/esp32/SerialBLEInterface.h>
  #elif defined(NRF52_PLATFORM)
    #include <Arduino.h>
    #include <helpers/nrf52/SerialBLEInterface.h>
  #endif
#endif

namespace mesh {
namespace ble {

MeshCoreUARTService::MeshCoreUARTService()
    : _pinCode(123456)
    , _mtu(244)
    , _registered(false)
    , _serviceStarted(false)
#ifdef ESP32
    , _platformService(nullptr)
    , _rxCharacteristic(nullptr)
    , _txCharacteristic(nullptr)
#elif defined(NRF52_PLATFORM)
    , _platformService(nullptr)
#endif
{
    memset(_deviceName, 0, sizeof(_deviceName));
    strcpy(_deviceName, "MeshCore");
    
#ifdef NATIVE_TEST
    _dataReceived = false;
#endif
}

MeshCoreUARTService::~MeshCoreUARTService() {
    onServiceUnregistered();
}

void MeshCoreUARTService::setDeviceName(const char* name) {
    if (name) {
        strncpy(_deviceName, name, sizeof(_deviceName) - 1);
        _deviceName[sizeof(_deviceName) - 1] = '\0';
    }
}

void MeshCoreUARTService::setPIN(uint32_t pin) {
    _pinCode = pin;
}

void MeshCoreUARTService::setMTU(size_t mtu) {
    _mtu = mtu;
}

const char* MeshCoreUARTService::getDeviceName() const {
    return _deviceName;
}

uint32_t MeshCoreUARTService::getPIN() const {
    return _pinCode;
}

bool MeshCoreUARTService::isConnected() const {
    return !_connections.empty();
}

// Frame I/O implementation

size_t MeshCoreUARTService::writeFrame(const uint8_t* data, size_t len) {
    if (!data || len == 0 || len > MAX_FRAME_SIZE) {
        return 0;
    }
    
    if (_sendQueue.size() >= FRAME_QUEUE_SIZE) {
        return 0; // Queue full
    }
    
    if (queueFrame(_sendQueue, data, len)) {
        return len;
    }
    return 0;
}

size_t MeshCoreUARTService::readFrame(uint8_t* buffer, size_t maxLen) {
    if (!buffer || maxLen == 0) {
        return 0;
    }
    
    if (_recvQueue.empty()) {
        return 0; // No data
    }
    
    // Get length BEFORE popping
    size_t len = _recvQueue.front().len;
    return dequeueFrame(_recvQueue, buffer, maxLen) ? len : 0;
}

bool MeshCoreUARTService::hasReceiveData() const {
    return !_recvQueue.empty();
}

size_t MeshCoreUARTService::getReceiveQueueSize() const {
    return _recvQueue.size();
}

void MeshCoreUARTService::clearReceiveQueue() {
    while (!_recvQueue.empty()) {
        _recvQueue.pop();
    }
}

bool MeshCoreUARTService::notify(uint16_t connectionId, const uint8_t* data, size_t len) {
    return sendNotification(connectionId, data, len);
}

// BLEServiceWrapper implementation

bool MeshCoreUARTService::onServiceRegistered(void* platformService) {
    if (_registered) {
        return false;
    }
    
#ifdef ESP32
    _platformService = static_cast<BLEService*>(platformService);
    if (!_platformService) {
        return false;
    }
    
    // Create RX characteristic (write)
    _rxCharacteristic = _platformService->createCharacteristic(
        CHAR_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE
    );
    if (!_rxCharacteristic) {
        return false;
    }
    
    // Create TX characteristic (read/notify)
    _txCharacteristic = _platformService->createCharacteristic(
        CHAR_UUID_TX,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    if (!_txCharacteristic) {
        return false;
    }
    
    // Set security - requires encryption for PIN-protected
    _rxCharacteristic->setAccessPermissions(ESP_GATT_PERM_WRITE_ENC_MITM);
    _txCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENC_MITM);
    
    // Add descriptor for notifications
    _txCharacteristic->addDescriptor(new BLE2902());
    
#elif defined(NRF52_PLATFORM)
    // NRF52 implementation
    _platformService = static_cast<BLEService*>(platformService);
    
    // Configure characteristics
    _rxCharacteristic.setProperties(CHR_PROPS_WRITE);
    _rxCharacteristic.setPermission(SECMODE_ENC_WITH_MITM, SECMODE_ENC_WITH_MITM);
    _rxCharacteristic.setMaxLen(MAX_FRAME_SIZE);
    _rxCharacteristic.begin();
    
    _txCharacteristic.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
    _txCharacteristic.setPermission(SECMODE_ENC_WITH_MITM, SECMODE_ENC_WITH_MITM);
    _txCharacteristic.setMaxLen(MAX_FRAME_SIZE);
    _txCharacteristic.begin();
#endif
    
    _registered = true;
    return true;
}

void MeshCoreUARTService::onServiceUnregistered() {
    _registered = false;
    _serviceStarted = false;
    
    // Clear all connections
    _connections.clear();
    
    // Clear queues
    clearReceiveQueue();
    while (!_sendQueue.empty()) {
        _sendQueue.pop();
    }
    
    // Platform cleanup handled by SharedBLEServer
#ifdef ESP32
    _platformService = nullptr;
    _rxCharacteristic = nullptr;
    _txCharacteristic = nullptr;
#elif defined(NRF52_PLATFORM)
    _platformService = nullptr;
#endif
}

void MeshCoreUARTService::onClientConnect(const ConnectionInfo& connInfo) {
    _connections.push_back(connInfo);
    
#ifdef NATIVE_TEST
    printf("[MeshCoreUARTService] Client %d connected\n", connInfo.connectionId);
#endif
}

void MeshCoreUARTService::onClientDisconnect(uint16_t connectionId) {
    // Remove connection
    _connections.erase(
        std::remove_if(_connections.begin(), _connections.end(),
            [connectionId](const ConnectionInfo& c) { 
                return c.connectionId == connectionId; 
            }),
        _connections.end()
    );
    
#ifdef NATIVE_TEST
    printf("[MeshCoreUARTService] Client %d disconnected\n", connectionId);
#endif
}

void MeshCoreUARTService::onDataReceived(uint16_t connectionId, const uint8_t* data, size_t len) {
    if (!data || len == 0) {
        return;
    }
    
#ifdef NATIVE_TEST
    _dataReceived = true;
    printf("[MeshCoreUARTService] Received %zu bytes from client %d\n", len, connectionId);
#endif
    
    processReceivedData(connectionId, data, len);
}

void MeshCoreUARTService::onDataSent(uint16_t connectionId, size_t len) {
    // Track sent data
}

bool MeshCoreUARTService::sendNotification(uint16_t connectionId, const uint8_t* data, size_t len) {
    if (!data || len == 0) {
        return false;
    }
    
#ifdef ESP32
    if (_txCharacteristic) {
        _txCharacteristic->setValue(const_cast<uint8_t*>(data), len);
        _txCharacteristic->notify();
        return true;
    }
#elif defined(NRF52_PLATFORM)
    // NRF52 notify implementation
    return _txCharacteristic.notify(data, len) > 0;
#endif
    
    return false;
}

bool MeshCoreUARTService::broadcastNotification(const uint8_t* data, size_t len) {
    // Send to all connected clients
    bool anySent = false;
    for (const auto& conn : _connections) {
        if (sendNotification(conn.connectionId, data, len)) {
            anySent = true;
        }
    }
    return anySent;
}

bool MeshCoreUARTService::hasConnectedClients() const {
    return !_connections.empty();
}

size_t MeshCoreUARTService::getConnectedClientCount() const {
    return _connections.size();
}

// Private helpers

void MeshCoreUARTService::processReceivedData(uint16_t connectionId, const uint8_t* data, size_t len) {
    // Queue the received frame
    if (_recvQueue.size() < FRAME_QUEUE_SIZE) {
        queueFrame(_recvQueue, data, len);
    }
    // If queue full, drop oldest
    else {
        _recvQueue.pop();
        queueFrame(_recvQueue, data, len);
    }
}

bool MeshCoreUARTService::queueFrame(std::queue<Frame>& queue, const uint8_t* data, size_t len) {
    if (len > MAX_FRAME_SIZE) {
        return false;
    }
    
    Frame frame;
    memcpy(frame.data, data, len);
    frame.len = len;
    queue.push(frame);
    return true;
}

bool MeshCoreUARTService::dequeueFrame(std::queue<Frame>& queue, uint8_t* buffer, size_t maxLen) {
    if (queue.empty()) {
        return false;
    }
    
    const Frame& frame = queue.front();
    size_t copyLen = (frame.len < maxLen) ? frame.len : maxLen;
    memcpy(buffer, frame.data, copyLen);
    queue.pop();
    return true;
}

} // namespace ble
} // namespace mesh
