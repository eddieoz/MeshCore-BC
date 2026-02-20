#include "SharedBLEServer.h"

#ifdef NATIVE_TEST
  #include <iostream>
  #include <cstring>
  // Minimal stubs for native testing
#else
  #ifdef ESP32
    #include <Arduino.h>
  #elif defined(NRF52_PLATFORM)
    #include <Arduino.h>
  #endif
#endif

namespace mesh {
namespace ble {

// Platform-specific implementation details
class SharedBLEServer::Impl {
public:
    struct ServiceEntry {
        std::string uuid;
        std::unique_ptr<BLEServiceWrapper> service;
        ServiceConfig config;
        std::vector<ConnectionInfo> connections;
#ifdef ESP32
        BLEService* platformService = nullptr;
#elif defined(NRF52_PLATFORM)
        BLEService platformService;
#endif
    };
    
    std::string deviceName;
    bool initialized = false;
    bool advertising = false;
    std::unordered_map<std::string, ServiceEntry> services;
    
#ifdef ESP32
    BLEServer* server = nullptr;
#elif defined(NRF52_PLATFORM)
    // Bluefruit uses global state
#endif

    Impl() = default;
    ~Impl() {
        end();
    }
    
    bool begin(const char* name) {
        if (initialized) {
            return false;
        }
        
        deviceName = name;
        
#ifdef ESP32
        BLEDevice::init(name);
        server = BLEDevice::createServer();
        if (!server) {
            return false;
        }
#elif defined(NRF52_PLATFORM)
        Bluefruit.begin();
        Bluefruit.setName(name);
        Bluefruit.Periph.setConnectCallback(onConnectStatic);
        Bluefruit.Periph.setDisconnectCallback(onDisconnectStatic);
#endif
        
        initialized = true;
        return true;
    }
    
    void end() {
        if (!initialized) {
            return;
        }
        
        stopAdvertising();
        
        // Notify all services
        for (auto& [uuid, entry] : services) {
            if (entry.service) {
                entry.service->onServiceUnregistered();
            }
        }
        services.clear();
        
#ifdef ESP32
        if (server) {
            // BLEServer cleanup handled by BLEDevice
            server = nullptr;
        }
#elif defined(NRF52_PLATFORM)
        // Bluefruit cleanup
#endif
        
        initialized = false;
    }
    
    bool addService(const char* uuid, std::unique_ptr<BLEServiceWrapper> service,
                    BLESecurityLevel security, uint32_t pinCode) {
        if (!initialized || !uuid || !service) {
            return false;
        }
        
        std::string uuidStr(uuid);
        if (services.find(uuidStr) != services.end()) {
            return false; // Already exists
        }
        
        ServiceEntry entry;
        entry.uuid = uuidStr;
        entry.service = std::move(service);
        entry.config.uuid = uuidStr;
        entry.config.securityLevel = security;
        entry.config.pinCode = pinCode;
        
#ifdef ESP32
        if (server) {
            entry.platformService = server->createService(uuid);
            if (!entry.platformService) {
                return false;
            }
            
            // Configure security
            if (security == BLESecurityLevel::PIN_REQUIRED) {
                BLESecurity sec;
                sec.setStaticPIN(pinCode);
                sec.setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);
            }
            
            // Notify service implementation
            entry.service->onServiceRegistered(entry.platformService);
            
            entry.platformService->start();
        }
#elif defined(NRF52_PLATFORM)
        entry.platformService.begin();
        
        // Configure security
        if (security == BLESecurityLevel::PIN_REQUIRED) {
            char pinStr[20];
            sprintf(pinStr, "%d", pinCode);
            Bluefruit.Security.setPIN(pinStr);
            Bluefruit.Security.setMITM(true);
        } else {
            Bluefruit.Security.setMITM(false);
        }
        
        entry.service->onServiceRegistered(&entry.platformService);
#endif
        
        services[uuidStr] = std::move(entry);
        return true;
    }
    
    bool removeService(const char* uuid) {
        if (!uuid) {
            return false;
        }
        
        std::string uuidStr(uuid);
        auto it = services.find(uuidStr);
        if (it == services.end()) {
            return false;
        }
        
        if (it->second.service) {
            it->second.service->onServiceUnregistered();
        }
        
        services.erase(it);
        return true;
    }
    
    bool hasService(const char* uuid) const {
        if (!uuid) return false;
        return services.find(std::string(uuid)) != services.end();
    }
    
    bool startAdvertising() {
        if (!initialized) {
            return false;
        }
        
#ifdef ESP32
        if (server) {
            BLEAdvertising* adv = server->getAdvertising();
            if (adv) {
                // Add all service UUIDs to advertisement
                for (const auto& [uuid, entry] : services) {
                    adv->addServiceUUID(uuid.c_str());
                }
                adv->start();
                advertising = true;
                return true;
            }
        }
#elif defined(NRF52_PLATFORM)
        Bluefruit.Advertising.clearData();
        Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
        
        // Add all services
        for (auto& [uuid, entry] : services) {
            Bluefruit.Advertising.addService(entry.platformService);
        }
        
        Bluefruit.ScanResponse.addName();
        Bluefruit.Advertising.start(0);
        advertising = true;
        return true;
#endif
        return false;
    }
    
    void stopAdvertising() {
        if (!initialized || !advertising) {
            return;
        }
        
#ifdef ESP32
        if (server) {
            server->getAdvertising()->stop();
        }
#elif defined(NRF52_PLATFORM)
        Bluefruit.Advertising.stop();
#endif
        advertising = false;
    }
    
    // Static callbacks for platform
#ifdef ESP32
    static void onConnectStatic(BLEServer* server) {
        // Handle in instance
    }
    
    static void onDisconnectStatic(BLEServer* server) {
        // Handle in instance
    }
#elif defined(NRF52_PLATFORM)
    static void onConnectStatic(uint16_t connHandle) {
        // Handle in instance
    }
    
    static void onDisconnectStatic(uint16_t connHandle, uint8_t reason) {
        // Handle in instance
    }
#endif

#ifdef NATIVE_TEST
    // Test helpers
    void simulateClientConnect(const char* serviceUuid, uint16_t connId) {
        auto it = services.find(std::string(serviceUuid));
        if (it != services.end()) {
            ConnectionInfo conn;
            conn.connectionId = connId;
            conn.connectTime = 0; // Would use millis() on real hardware
            conn.isAuthenticated = (it->second.config.securityLevel == BLESecurityLevel::OPEN);
            it->second.connections.push_back(conn);
            it->second.service->onClientConnect(conn);
        }
    }
    
    void simulateClientDisconnect(uint16_t connId) {
        for (auto& [uuid, entry] : services) {
            for (auto it = entry.connections.begin(); it != entry.connections.end(); ++it) {
                if (it->connectionId == connId) {
                    entry.service->onClientDisconnect(connId);
                    entry.connections.erase(it);
                    return;
                }
            }
        }
    }
    
    void simulateDataWrite(const char* serviceUuid, const uint8_t* data, size_t len) {
        auto it = services.find(std::string(serviceUuid));
        if (it != services.end() && !it->second.connections.empty()) {
            uint16_t connId = it->second.connections.front().connectionId;
            it->second.service->onDataReceived(connId, data, len);
        }
    }
    
    std::string getServiceUUID(const char* uuid) const {
        return std::string(uuid);
    }
#endif
};

// Public interface implementation
SharedBLEServer::SharedBLEServer() : _impl(std::make_unique<Impl>()) {}
SharedBLEServer::~SharedBLEServer() = default;

bool SharedBLEServer::begin(const char* deviceName) {
    return _impl->begin(deviceName);
}

void SharedBLEServer::end() {
    _impl->end();
}

bool SharedBLEServer::isInitialized() const {
    return _impl->initialized;
}

bool SharedBLEServer::addService(const char* uuid,
                                 std::unique_ptr<BLEServiceWrapper> service,
                                 BLESecurityLevel security,
                                 uint32_t pinCode) {
    return _impl->addService(uuid, std::move(service), security, pinCode);
}

bool SharedBLEServer::removeService(const char* uuid) {
    return _impl->removeService(uuid);
}

bool SharedBLEServer::hasService(const char* uuid) const {
    return _impl->hasService(uuid);
}

BLEServiceWrapper* SharedBLEServer::getService(const char* uuid) const {
    auto it = _impl->services.find(std::string(uuid));
    if (it != _impl->services.end()) {
        return it->second.service.get();
    }
    return nullptr;
}

size_t SharedBLEServer::getServiceCount() const {
    return _impl->services.size();
}

bool SharedBLEServer::startAdvertising() {
    return _impl->startAdvertising();
}

void SharedBLEServer::stopAdvertising() {
    _impl->stopAdvertising();
}

bool SharedBLEServer::isAdvertising() const {
    return _impl->advertising;
}

bool SharedBLEServer::isServiceConnected(const char* uuid) const {
    auto it = _impl->services.find(std::string(uuid));
    if (it != _impl->services.end()) {
        return !it->second.connections.empty();
    }
    return false;
}

size_t SharedBLEServer::getServiceConnectionCount(const char* uuid) const {
    auto it = _impl->services.find(std::string(uuid));
    if (it != _impl->services.end()) {
        return it->second.connections.size();
    }
    return 0;
}

BLESecurityLevel SharedBLEServer::getServiceSecurityLevel(const char* uuid) const {
    auto it = _impl->services.find(std::string(uuid));
    if (it != _impl->services.end()) {
        return it->second.config.securityLevel;
    }
    return BLESecurityLevel::OPEN;
}

bool SharedBLEServer::serviceRequiresPIN(const char* uuid) const {
    return getServiceSecurityLevel(uuid) == BLESecurityLevel::PIN_REQUIRED;
}

size_t SharedBLEServer::getTotalConnectionCount() const {
    size_t total = 0;
    for (const auto& [uuid, entry] : _impl->services) {
        total += entry.connections.size();
    }
    return total;
}

void SharedBLEServer::disconnectAll() {
    // Implementation depends on platform
}

void SharedBLEServer::loop() {
    // Process any pending operations
}

#ifdef NATIVE_TEST
void SharedBLEServer::simulateClientConnect(const char* serviceUuid, uint16_t connId) {
    _impl->simulateClientConnect(serviceUuid, connId);
}

void SharedBLEServer::simulateClientDisconnect(uint16_t connId) {
    _impl->simulateClientDisconnect(connId);
}

void SharedBLEServer::simulateDataWrite(const char* serviceUuid, const uint8_t* data, size_t len) {
    _impl->simulateDataWrite(serviceUuid, data, len);
}

std::string SharedBLEServer::getServiceUUID(const char* uuid) const {
    return _impl->getServiceUUID(uuid);
}
#endif

} // namespace ble
} // namespace mesh
