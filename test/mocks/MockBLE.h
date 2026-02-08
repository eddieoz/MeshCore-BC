#pragma once

/**
 * Mock BLE classes for native testing (without hardware)
 */

#include <stdint.h>
#include <string>
#include <vector>
#include <functional>
#include <cstring>

// Mock BLE UUID
class MockBLEUUID {
public:
    std::string uuid;
    MockBLEUUID() = default;
    MockBLEUUID(const char* u) : uuid(u) {}
    MockBLEUUID(const std::string& u) : uuid(u) {}
};

// Mock BLE Service
class MockBLEService {
public:
    std::string uuid;
    bool started = false;
    
    MockBLEService() = default;
    MockBLEService(const char* uuid) : uuid(uuid) {}
    
    void start() { started = true; }
    void stop() { started = false; }
};

// Mock BLE Server
class MockBLEServer {
public:
    std::vector<MockBLEService> services;
    bool advertising = false;
    int connectedCount = 0;
    
    MockBLEService* createService(const char* uuid) {
        services.emplace_back(uuid);
        return &services.back();
    }
    
    MockBLEServer* getAdvertising() { return this; }
    void start() { advertising = true; }
    void stop() { advertising = false; }
    int getConnectedCount() { return connectedCount; }
    void addServiceUUID(const char* uuid) {}
};

// Mock BLE Device
class MockBLEDevice {
public:
    static MockBLEServer server;
    static std::string deviceName;
    
    static void init(const char* name) { deviceName = name; }
    static MockBLEServer* createServer() { return &server; }
    static void setMTU(int mtu) {}
};

// Mock BLE Security
enum {
    ESP_LE_AUTH_REQ_SC_MITM_BOND = 0
};

class MockBLESecurity {
public:
    void setStaticPIN(uint32_t pin) {}
    void setAuthenticationMode(int mode) {}
};

// Static members
inline MockBLEServer MockBLEDevice::server;
inline std::string MockBLEDevice::deviceName;

// Type aliases for compatibility
#define BLEUUID MockBLEUUID
#define BLEService MockBLEService
#define BLEServer MockBLEServer
#define BLEDevice MockBLEDevice
#define BLESecurity MockBLESecurity
#define BLEAdvertising MockBLEServer
#define BLECharacteristicCallbacks void*
#define BLEServerCallbacks void*
#define BLESecurityCallbacks void*

// UUID constants are defined in SharedBLEServer.h - don't redefine here

// Include the real service headers for testing
#include "helpers/ble/MeshCoreUARTService.h"

namespace mesh {
namespace ble {

class MockMeshCoreUARTService : public BLEServiceWrapper {
public:
    bool registered = false;
    bool dataReceived = false;
    std::vector<uint8_t> lastData;
    std::vector<ConnectionInfo> connections;
    
    bool onServiceRegistered(void* platformService) override {
        registered = true;
        return true;
    }
    
    void onServiceUnregistered() override {
        registered = false;
    }
    
    void onClientConnect(const ConnectionInfo& connInfo) override {
        connections.push_back(connInfo);
    }
    
    void onClientDisconnect(uint16_t connectionId) override {
        connections.erase(
            std::remove_if(connections.begin(), connections.end(),
                [connectionId](const ConnectionInfo& c) { return c.connectionId == connectionId; }),
            connections.end()
        );
    }
    
    void onDataReceived(uint16_t connectionId, const uint8_t* data, size_t len) override {
        dataReceived = true;
        lastData.assign(data, data + len);
    }
    
    void onDataSent(uint16_t connectionId, size_t len) override {}
    
    bool sendNotification(uint16_t connectionId, const uint8_t* data, size_t len) override {
        return true;
    }
    
    bool broadcastNotification(const uint8_t* data, size_t len) override {
        return true;
    }
    
    bool hasConnectedClients() const override {
        return !connections.empty();
    }
    
    size_t getConnectedClientCount() const override {
        return connections.size();
    }
    
    // Test helpers
    bool wasDataReceived() const { return dataReceived; }
    void clearReceivedData() { dataReceived = false; lastData.clear(); }
};

class MockBitchatBLEService : public BLEServiceWrapper {
public:
    bool registered = false;
    bool dataReceived = false;
    std::vector<uint8_t> lastData;
    std::vector<ConnectionInfo> connections;
    
    bool onServiceRegistered(void* platformService) override {
        registered = true;
        return true;
    }
    
    void onServiceUnregistered() override {
        registered = false;
    }
    
    void onClientConnect(const ConnectionInfo& connInfo) override {
        connections.push_back(connInfo);
    }
    
    void onClientDisconnect(uint16_t connectionId) override {
        connections.erase(
            std::remove_if(connections.begin(), connections.end(),
                [connectionId](const ConnectionInfo& c) { return c.connectionId == connectionId; }),
            connections.end()
        );
    }
    
    void onDataReceived(uint16_t connectionId, const uint8_t* data, size_t len) override {
        dataReceived = true;
        lastData.assign(data, data + len);
    }
    
    void onDataSent(uint16_t connectionId, size_t len) override {}
    
    bool sendNotification(uint16_t connectionId, const uint8_t* data, size_t len) override {
        return true;
    }
    
    bool broadcastNotification(const uint8_t* data, size_t len) override {
        return true;
    }
    
    bool hasConnectedClients() const override {
        return !connections.empty();
    }
    
    size_t getConnectedClientCount() const override {
        return connections.size();
    }
    
    // Test helpers
    bool wasDataReceived() const { return dataReceived; }
    void clearReceivedData() { dataReceived = false; lastData.clear(); }
};

// Stub BitchatBLEService for testing
class BitchatBLEService : public BLEServiceWrapper {
public:
    bool registered = false;
    bool dataReceived = false;
    std::vector<uint8_t> lastData;
    std::vector<ConnectionInfo> connections;
    
    bool onServiceRegistered(void* platformService) override {
        registered = true;
        return true;
    }
    
    void onServiceUnregistered() override {
        registered = false;
    }
    
    void onClientConnect(const ConnectionInfo& connInfo) override {
        connections.push_back(connInfo);
    }
    
    void onClientDisconnect(uint16_t connectionId) override {
        connections.erase(
            std::remove_if(connections.begin(), connections.end(),
                [connectionId](const ConnectionInfo& c) { return c.connectionId == connectionId; }),
            connections.end()
        );
    }
    
    void onDataReceived(uint16_t connectionId, const uint8_t* data, size_t len) override {
        dataReceived = true;
        lastData.assign(data, data + len);
    }
    
    void onDataSent(uint16_t connectionId, size_t len) override {}
    
    bool sendNotification(uint16_t connectionId, const uint8_t* data, size_t len) override {
        return true;
    }
    
    bool broadcastNotification(const uint8_t* data, size_t len) override {
        return true;
    }
    
    bool hasConnectedClients() const override {
        return !connections.empty();
    }
    
    size_t getConnectedClientCount() const override {
        return connections.size();
    }
    
    bool wasDataReceived() const { return dataReceived; }
    void clearReceivedData() { dataReceived = false; lastData.clear(); }
};

} // namespace ble
} // namespace mesh

// When using real Unity framework (PlatformIO tests), don't redefine macros
#ifndef UNITY_VERSION
  // Standalone mock Unity - only define if real Unity not present
  #define UNITY_BEGIN() 0
  #define UNITY_END() 0
  #define RUN_TEST(testFunc) testFunc()
  #define TEST_ASSERT_TRUE(condition) do { if (!(condition)) { \
      printf("ASSERTION FAILED: %s at %s:%d\n", #condition, __FILE__, __LINE__); \
      return; }} while(0)
  #define TEST_ASSERT_FALSE(condition) TEST_ASSERT_TRUE(!(condition))
  #define TEST_ASSERT_EQUAL(expected, actual) do { if ((expected) != (actual)) { \
      printf("ASSERTION FAILED: expected %d, got %d at %s:%d\n", \
             (int)(expected), (int)(actual), __FILE__, __LINE__); \
      return; }} while(0)
  #define TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, actual, num_elements) do { \
      for (size_t _i = 0; _i < (num_elements); _i++) { \
        if ((expected)[_i] != (actual)[_i]) { \
          printf("ASSERTION FAILED: arrays differ at index %zu at %s:%d\n", _i, __FILE__, __LINE__); \
          return; \
        } \
      }} while(0)
  #define TEST_ASSERT_NOT_EQUAL(expected, actual) TEST_ASSERT_TRUE((expected) != (actual))
  #define TEST_ASSERT_NULL(ptr) TEST_ASSERT_TRUE((ptr) == nullptr)
  #define TEST_ASSERT_NOT_NULL(ptr) TEST_ASSERT_TRUE((ptr) != nullptr)
  
  inline void setUp(void) {}
  inline void tearDown(void) {}
#endif
