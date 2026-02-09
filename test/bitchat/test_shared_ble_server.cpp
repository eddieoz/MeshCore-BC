/**
 * Story 1.1: Shared BLE Server Foundation - BDD Tests
 * 
 * Feature: Shared BLE Server for Multiple Services
 */

#include <unity.h>
#include "helpers/ble/SharedBLEServer.h"
#include "helpers/ble/BitchatBLEService.h"

// Mock BLE classes for testing (when hardware not available)
#ifdef NATIVE_TEST
  #include "../mocks/MockBLE.h"
#else
  // Real hardware test
  #include <BLEDevice.h>
#endif

using namespace mesh::ble;

namespace test {
namespace bitchat {

/**
 * Scenario: BLE server can host multiple services
 * 
 * Given the device is an ESP32 or NRF52
 * And BLE is initialized
 * When the BLE server is created
 * Then it should support adding multiple GATT services
 * And each service should have a unique UUID
 */
void test_shared_ble_server_can_host_multiple_services(void) {
    // Given: BLE is initialized
    SharedBLEServer server;
    TEST_ASSERT_TRUE(server.begin("TestDevice"));
    
    // When: Creating first service (MeshCore UART)
    auto meshcoreService = std::make_unique<MeshCoreUARTService>();
    bool meshcoreAdded = server.addService(
        MESHCORE_UART_SERVICE_UUID,
        std::move(meshcoreService),
        BLESecurityLevel::PIN_REQUIRED
    );
    
    // Then: First service added successfully
    TEST_ASSERT_TRUE(meshcoreAdded);
    TEST_ASSERT_TRUE(server.hasService(MESHCORE_UART_SERVICE_UUID));
    
    // When: Creating second service (BitChat)
    auto bitchatService = std::make_unique<BitchatBLEService>();
    bool bitchatAdded = server.addService(
        BITCHAT_SERVICE_UUID,
        std::move(bitchatService),
        BLESecurityLevel::OPEN
    );
    
    // Then: Second service also added successfully
    TEST_ASSERT_TRUE(bitchatAdded);
    TEST_ASSERT_TRUE(server.hasService(BITCHAT_SERVICE_UUID));
    
    // And: Both services have unique UUIDs
    TEST_ASSERT_NOT_EQUAL(
        server.getServiceUUID(MESHCORE_UART_SERVICE_UUID),
        server.getServiceUUID(BITCHAT_SERVICE_UUID)
    );
    
    server.end();
}

/**
 * Scenario: Services maintain independent connection state
 * 
 * Given two BLE services are registered
 * When a client connects to one service
 * Then the other service should not be affected
 * And connection counts should be tracked per-service
 */
void test_services_maintain_independent_connection_state(void) {
    SharedBLEServer server;
    server.begin("TestDevice");
    
    // Add two services
    server.addService(MESHCORE_UART_SERVICE_UUID, 
                      std::make_unique<MeshCoreUARTService>(),
                      BLESecurityLevel::PIN_REQUIRED);
    server.addService(BITCHAT_SERVICE_UUID,
                      std::make_unique<BitchatBLEService>(),
                      BLESecurityLevel::OPEN);
    
    // When: Client connects to MeshCore service
    server.simulateClientConnect(MESHCORE_UART_SERVICE_UUID, /*conn_id=*/1);
    
    // Then: MeshCore shows connected
    TEST_ASSERT_TRUE(server.isServiceConnected(MESHCORE_UART_SERVICE_UUID));
    TEST_ASSERT_EQUAL(1, server.getServiceConnectionCount(MESHCORE_UART_SERVICE_UUID));
    
    // And: BitChat shows NOT connected
    TEST_ASSERT_FALSE(server.isServiceConnected(BITCHAT_SERVICE_UUID));
    TEST_ASSERT_EQUAL(0, server.getServiceConnectionCount(BITCHAT_SERVICE_UUID));
    
    // When: Another client connects to BitChat service
    server.simulateClientConnect(BITCHAT_SERVICE_UUID, /*conn_id=*/2);
    
    // Then: Both show connected independently
    TEST_ASSERT_TRUE(server.isServiceConnected(MESHCORE_UART_SERVICE_UUID));
    TEST_ASSERT_TRUE(server.isServiceConnected(BITCHAT_SERVICE_UUID));
    TEST_ASSERT_EQUAL(1, server.getServiceConnectionCount(MESHCORE_UART_SERVICE_UUID));
    TEST_ASSERT_EQUAL(1, server.getServiceConnectionCount(BITCHAT_SERVICE_UUID));
    
    server.end();
}

/**
 * Scenario: Different security levels per service
 * 
 * Given MeshCore service requires PIN
 * And BitChat service requires no authentication
 * When clients attempt to connect
 * Then MeshCore should require PIN authentication
 * And BitChat should connect without authentication
 */
void test_different_security_levels_per_service(void) {
    SharedBLEServer server;
    server.begin("TestDevice");
    
    // Add services with different security levels
    server.addService(MESHCORE_UART_SERVICE_UUID,
                      std::make_unique<MeshCoreUARTService>(),
                      BLESecurityLevel::PIN_REQUIRED,  // PIN required
                      /*pin_code=*/123456);
    
    server.addService(BITCHAT_SERVICE_UUID,
                      std::make_unique<BitchatBLEService>(),
                      BLESecurityLevel::OPEN);  // No auth
    
    // Then: MeshCore service has PIN security
    TEST_ASSERT_EQUAL(BLESecurityLevel::PIN_REQUIRED,
                      server.getServiceSecurityLevel(MESHCORE_UART_SERVICE_UUID));
    TEST_ASSERT_TRUE(server.serviceRequiresPIN(MESHCORE_UART_SERVICE_UUID));
    
    // And: BitChat service has open security
    TEST_ASSERT_EQUAL(BLESecurityLevel::OPEN,
                      server.getServiceSecurityLevel(BITCHAT_SERVICE_UUID));
    TEST_ASSERT_FALSE(server.serviceRequiresPIN(BITCHAT_SERVICE_UUID));
    
    server.end();
}

/**
 * Scenario: Service data isolation
 * 
 * Given two services with different characteristics
 * When data is written to one service
 * Then the other service should not receive that data
 * And each service processes only its own data
 */
void test_service_data_isolation(void) {
    SharedBLEServer server;
    server.begin("TestDevice");
    
    // Create mock services that track received data
    auto meshcoreService = std::make_unique<MockMeshCoreUARTService>();
    auto* meshcorePtr = meshcoreService.get();
    
    auto bitchatService = std::make_unique<MockBitchatBLEService>();
    auto* bitchatPtr = bitchatService.get();
    
    server.addService(MESHCORE_UART_SERVICE_UUID,
                      std::move(meshcoreService),
                      BLESecurityLevel::PIN_REQUIRED);
    server.addService(BITCHAT_SERVICE_UUID,
                      std::move(bitchatService),
                      BLESecurityLevel::OPEN);
    
    // Given: Clients connected to both services
    server.simulateClientConnect(MESHCORE_UART_SERVICE_UUID, /*conn_id=*/1);
    server.simulateClientConnect(BITCHAT_SERVICE_UUID, /*conn_id=*/2);
    
    // When: Data written to MeshCore service
    uint8_t meshcoreData[] = {0x01, 0x02, 0x03};
    server.simulateDataWrite(MESHCORE_UART_SERVICE_UUID, meshcoreData, sizeof(meshcoreData));
    
    // Then: Only MeshCore service received data
    TEST_ASSERT_TRUE(meshcorePtr->wasDataReceived());
    TEST_ASSERT_FALSE(bitchatPtr->wasDataReceived());
    
    // Reset
    meshcorePtr->clearReceivedData();
    bitchatPtr->clearReceivedData();
    
    // When: Data written to BitChat service
    uint8_t bitchatData[] = {0xAA, 0xBB, 0xCC};
    server.simulateDataWrite(BITCHAT_SERVICE_UUID, bitchatData, sizeof(bitchatData));
    
    // Then: Only BitChat service received data
    TEST_ASSERT_FALSE(meshcorePtr->wasDataReceived());
    TEST_ASSERT_TRUE(bitchatPtr->wasDataReceived());
    
    server.end();
}

/**
 * Scenario: Service dynamic registration
 * 
 * Given BLE server is running with one service
 * When a second service is added dynamically
 * Then the second service should be discoverable
 * And existing connections should not be disrupted
 */
void test_dynamic_service_registration(void) {
    SharedBLEServer server;
    server.begin("TestDevice");
    
    // Start with only MeshCore
    server.addService(MESHCORE_UART_SERVICE_UUID,
                      std::make_unique<MeshCoreUARTService>(),
                      BLESecurityLevel::PIN_REQUIRED);
    
    // Client connects to MeshCore
    server.simulateClientConnect(MESHCORE_UART_SERVICE_UUID, /*conn_id=*/1);
    TEST_ASSERT_TRUE(server.isServiceConnected(MESHCORE_UART_SERVICE_UUID));
    
    // When: Dynamically add BitChat service
    server.addService(BITCHAT_SERVICE_UUID,
                      std::make_unique<BitchatBLEService>(),
                      BLESecurityLevel::OPEN);
    
    // Then: BitChat is discoverable
    TEST_ASSERT_TRUE(server.hasService(BITCHAT_SERVICE_UUID));
    
    // And: MeshCore connection still active
    TEST_ASSERT_TRUE(server.isServiceConnected(MESHCORE_UART_SERVICE_UUID));
    
    server.end();
}

// Tests are called from test_main.cpp
} // namespace bitchat
} // namespace test
