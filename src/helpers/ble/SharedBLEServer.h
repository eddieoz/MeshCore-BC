#pragma once

/**
 * Story 1.1: Shared BLE Server Foundation
 * 
 * Provides a unified interface for hosting multiple BLE services
 * on a single BLE server. Supports both ESP32 (ESP-IDF BLE) and
 * NRF52 (Adafruit Bluefruit) platforms.
 * 
 * This enables MeshCore UART service and BitChat service to coexist
 * on the same device with independent:
 * - Connection states
 * - Security levels (PIN vs open)
 * - Data handling
 */

#include <stdint.h>
#include <memory>
#include <functional>
#include <unordered_map>
#include <string>

// Platform-specific includes
#ifdef ESP32
  #include <BLEDevice.h>
  #include <BLEServer.h>
  #include <BLEService.h>
  #include <BLECharacteristic.h>
#elif defined(NRF52_PLATFORM)
  #include <bluefruit.h>
#endif

namespace mesh {
namespace ble {

// Forward declarations
class BLEServiceWrapper;

/**
 * Security levels for BLE services
 */
enum class BLESecurityLevel {
    OPEN,           // No authentication required
    PIN_REQUIRED,   // PIN code required for pairing
    BOND_REQUIRED   // Bonding required
};

/**
 * Service configuration structure
 */
struct ServiceConfig {
    std::string uuid;
    BLESecurityLevel securityLevel;
    uint32_t pinCode;  // Only used if securityLevel == PIN_REQUIRED
    
    ServiceConfig() : securityLevel(BLESecurityLevel::OPEN), pinCode(0) {}
};

/**
 * Connection information
 */
struct ConnectionInfo {
    uint16_t connectionId;
    uint32_t connectTime;
    bool isAuthenticated;
    
    ConnectionInfo() : connectionId(0), connectTime(0), isAuthenticated(false) {}
};

/**
 * Base class for BLE service implementations
 */
class BLEServiceWrapper {
public:
    virtual ~BLEServiceWrapper() = default;
    
    // Lifecycle
    virtual bool onServiceRegistered(void* platformService) = 0;
    virtual void onServiceUnregistered() = 0;
    
    // Connection events
    virtual void onClientConnect(const ConnectionInfo& connInfo) = 0;
    virtual void onClientDisconnect(uint16_t connectionId) = 0;
    
    // Data handling
    virtual void onDataReceived(uint16_t connectionId, const uint8_t* data, size_t len) = 0;
    virtual void onDataSent(uint16_t connectionId, size_t len) = 0;
    
    // Notification helpers
    virtual bool sendNotification(uint16_t connectionId, const uint8_t* data, size_t len) = 0;
    virtual bool broadcastNotification(const uint8_t* data, size_t len) = 0;
    
    // State queries
    virtual bool hasConnectedClients() const = 0;
    virtual size_t getConnectedClientCount() const = 0;
};

/**
 * Shared BLE Server - Manages multiple BLE services on a single server
 * 
 * Usage:
 *   SharedBLEServer server;
 *   server.begin("MyDevice");
 *   
 *   // Add MeshCore service (PIN protected)
 *   server.addService(MESHCORE_UART_UUID, 
 *                     std::make_unique<MeshCoreUARTService>(),
 *                     BLESecurityLevel::PIN_REQUIRED, 123456);
 *   
 *   // Add BitChat service (open)
 *   server.addService(BITCHAT_UUID,
 *                     std::make_unique<BitchatBLEService>(),
 *                     BLESecurityLevel::OPEN);
 *   
 *   server.startAdvertising();
 */
class SharedBLEServer {
public:
    SharedBLEServer();
    ~SharedBLEServer();
    
    // Non-copyable
    SharedBLEServer(const SharedBLEServer&) = delete;
    SharedBLEServer& operator=(const SharedBLEServer&) = delete;
    
    /**
     * Initialize BLE and create server
     * @param deviceName Name for BLE advertising
     * @return true if successful
     */
    bool begin(const char* deviceName);
    
    /**
     * Shutdown BLE server
     */
    void end();
    
    /**
     * Check if server is initialized
     */
    bool isInitialized() const;
    
    /**
     * Add a BLE service to the server
     * @param uuid Service UUID string
     * @param service Service implementation (ownership transferred)
     * @param security Security level for this service
     * @param pinCode PIN code if security is PIN_REQUIRED
     * @return true if service added successfully
     */
    bool addService(const char* uuid,
                    std::unique_ptr<BLEServiceWrapper> service,
                    BLESecurityLevel security = BLESecurityLevel::OPEN,
                    uint32_t pinCode = 0);
    
    /**
     * Remove a service from the server
     * @param uuid Service UUID
     * @return true if service was removed
     */
    bool removeService(const char* uuid);
    
    /**
     * Check if a service is registered
     */
    bool hasService(const char* uuid) const;
    
    /**
     * Get a service by UUID
     * @param uuid Service UUID
     * @return Pointer to service wrapper, or nullptr if not found
     */
    BLEServiceWrapper* getService(const char* uuid) const;
    
    /**
     * Get number of registered services
     */
    size_t getServiceCount() const;
    
    /**
     * Start BLE advertising with all registered services
     */
    bool startAdvertising();
    
    /**
     * Stop BLE advertising
     */
    void stopAdvertising();
    
    /**
     * Check if currently advertising
     */
    bool isAdvertising() const;
    
    // Per-service connection queries
    
    /**
     * Check if a specific service has connected clients
     */
    bool isServiceConnected(const char* uuid) const;
    
    /**
     * Get connection count for a specific service
     */
    size_t getServiceConnectionCount(const char* uuid) const;
    
    /**
     * Get security level for a service
     */
    BLESecurityLevel getServiceSecurityLevel(const char* uuid) const;
    
    /**
     * Check if service requires PIN
     */
    bool serviceRequiresPIN(const char* uuid) const;
    
    // Global connection queries
    
    /**
     * Get total connection count across all services
     */
    size_t getTotalConnectionCount() const;
    
    /**
     * Disconnect all clients
     */
    void disconnectAll();
    
    // Main loop processing - call from loop()
    void loop();
    
    // Platform-specific accessors (for advanced usage)
#ifdef ESP32
    BLEServer* getPlatformServer();
#elif defined(NRF52_PLATFORM)
    BLEService* getPlatformService(const char* uuid);
#endif

#ifdef NATIVE_TEST
    // Test helpers - only available in test builds
    void simulateClientConnect(const char* serviceUuid, uint16_t connId);
    void simulateClientDisconnect(uint16_t connId);
    void simulateDataWrite(const char* serviceUuid, const uint8_t* data, size_t len);
    std::string getServiceUUID(const char* uuid) const;
#endif

private:
    class Impl;
    std::unique_ptr<Impl> _impl;
};

// Common UUIDs
constexpr const char* MESHCORE_UART_SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
constexpr const char* BITCHAT_SERVICE_UUID = "F47B5E2D-4A9E-4C5A-9B3F-8E1D2C3A4B5C";

} // namespace ble
} // namespace mesh
