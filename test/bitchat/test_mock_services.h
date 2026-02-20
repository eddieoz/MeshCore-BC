/**
 * @file test_mock_services.h
 * @brief Mock BLE services for testing
 * 
 * Include this file AFTER including SharedBLEServer.h or files that include it.
 */

#pragma once

#include "helpers/ble/SharedBLEServer.h"

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
    
    bool wasDataReceived() const { return dataReceived; }
    void clearReceivedData() { dataReceived = false; lastData.clear(); }
};

} // namespace ble
} // namespace mesh
