#pragma once

/**
 * Story 1.2: MeshCore BLE Service Preservation
 * 
 * Wraps existing SerialBLEInterface functionality to work with
 * SharedBLEServer while maintaining backward compatibility.
 * 
 * This service provides:
 * - Nordic UART Service UUID (6E400001-B5A3-F393-E0A9-E50E24DCCA9E)
 * - PIN-based authentication
 * - Frame-based communication protocol
 * - Backward compatible with existing MeshCore companion app
 */

#include "SharedBLEServer.h"
#include <memory>
#include <queue>

// Forward declaration of existing SerialBLEInterface
class SerialBLEInterface;

namespace mesh {
namespace ble {

/**
 * MeshCore UART Service - Adapter for SharedBLEServer
 * 
 * Wraps the existing SerialBLEInterface to provide:
 * 1. Compatibility with SharedBLEServer architecture
 * 2. Preservation of existing MeshCore BLE protocol
 * 3. PIN authentication as before
 * 4. Frame queueing for reliable communication
 */
class MeshCoreUARTService : public BLEServiceWrapper {
public:
    // Maximum frame size (consistent with existing implementation)
    static constexpr size_t MAX_FRAME_SIZE = 244;
    static constexpr size_t FRAME_QUEUE_SIZE = 4;
    
    // Nordic UART Service UUIDs
    static constexpr const char* SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
    static constexpr const char* CHAR_UUID_RX = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
    static constexpr const char* CHAR_UUID_TX = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

    MeshCoreUARTService();
    ~MeshCoreUARTService() override;

    // Configuration (before registration)
    void setDeviceName(const char* name);
    void setPIN(uint32_t pin);
    void setMTU(size_t mtu);
    
    // Accessors
    const char* getDeviceName() const;
    uint32_t getPIN() const;
    bool isConnected() const;
    
    // Frame I/O (backward compatible interface)
    size_t writeFrame(const uint8_t* data, size_t len);
    size_t readFrame(uint8_t* buffer, size_t maxLen);
    bool hasReceiveData() const;
    size_t getReceiveQueueSize() const;
    void clearReceiveQueue();
    
    // Notification helpers
    bool notify(uint16_t connectionId, const uint8_t* data, size_t len);

    // BLEServiceWrapper implementation
    bool onServiceRegistered(void* platformService) override;
    void onServiceUnregistered() override;
    void onClientConnect(const ConnectionInfo& connInfo) override;
    void onClientDisconnect(uint16_t connectionId) override;
    void onDataReceived(uint16_t connectionId, const uint8_t* data, size_t len) override;
    void onDataSent(uint16_t connectionId, size_t len) override;
    bool sendNotification(uint16_t connectionId, const uint8_t* data, size_t len) override;
    bool broadcastNotification(const uint8_t* data, size_t len) override;
    bool hasConnectedClients() const override;
    size_t getConnectedClientCount() const override;

    // Test helpers (only in test builds)
    #ifdef NATIVE_TEST
    bool wasDataReceived() const { return _dataReceived; }
    void clearDataReceived() { _dataReceived = false; }
    #endif

private:
    struct Frame {
        uint8_t data[MAX_FRAME_SIZE];
        size_t len;
        
        Frame() : len(0) {}
    };
    
    // Configuration
    char _deviceName[48];
    uint32_t _pinCode;
    size_t _mtu;
    
    // State
    bool _registered;
    bool _serviceStarted;
    std::vector<ConnectionInfo> _connections;
    
    // Frame queues
    std::queue<Frame> _recvQueue;
    std::queue<Frame> _sendQueue;
    
    // Platform-specific handles
#ifdef ESP32
    BLEService* _platformService;
    BLECharacteristic* _rxCharacteristic;
    BLECharacteristic* _txCharacteristic;
#elif defined(NRF52_PLATFORM)
    BLEService* _platformService;  // Using pointer for compatibility
    BLECharacteristic _rxCharacteristic;
    BLECharacteristic _txCharacteristic;
#endif
    
    // Legacy interface wrapper (optional, for gradual migration)
    std::unique_ptr<SerialBLEInterface> _legacyInterface;
    
    // Internal helpers
    void processReceivedData(uint16_t connectionId, const uint8_t* data, size_t len);
    bool queueFrame(std::queue<Frame>& queue, const uint8_t* data, size_t len);
    bool dequeueFrame(std::queue<Frame>& queue, uint8_t* buffer, size_t maxLen);
    
    // Test tracking
    #ifdef NATIVE_TEST
    bool _dataReceived;
    #endif
};

} // namespace ble
} // namespace mesh
