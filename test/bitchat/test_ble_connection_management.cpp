/**
 * Story 1.4: BLE Connection Management - BDD Tests
 * 
 * Feature: Independent Connection Handling
 */

#include <unity.h>
#include "helpers/ble/SharedBLEServer.h"
#include "helpers/ble/MeshCoreUARTService.h"
#include "helpers/ble/BitchatBLEService.h"

#ifdef NATIVE_TEST
  #include "../mocks/MockBLE.h"
#else
  #include <BLEDevice.h>
#endif

using namespace mesh::ble;

namespace test {
namespace bitchat {

/**
 * Scenario: Both apps connected simultaneously
 * 
 * Given MeshCore app is connected via BLE
 * And BitChat app connects via BLE
 * Then both connections should be active
 * And data from one should not interfere with the other
 */
void test_both_services_connected_simultaneously(void) {
    // Given: SharedBLEServer with both services
    SharedBLEServer server;
    TEST_ASSERT_TRUE(server.begin("MeshCoreBitChatDevice"));
    
    auto meshService = std::make_unique<MeshCoreUARTService>();
    auto bitchatService = std::make_unique<BitchatBLEService>();
    
    server.addService(
        MESHCORE_UART_SERVICE_UUID,
        std::move(meshService),
        BLESecurityLevel::PIN_REQUIRED,
        123456
    );
    
    server.addService(
        BITCHAT_SERVICE_UUID,
        std::move(bitchatService),
        BLESecurityLevel::OPEN
    );
    
    // When: MeshCore app connects first
    server.simulateClientConnect(MESHCORE_UART_SERVICE_UUID, /*conn_id=*/1);
    
    // Then: MeshCore is connected, BitChat is not
    TEST_ASSERT_TRUE(server.isServiceConnected(MESHCORE_UART_SERVICE_UUID));
    TEST_ASSERT_FALSE(server.isServiceConnected(BITCHAT_SERVICE_UUID));
    TEST_ASSERT_EQUAL(1, server.getServiceConnectionCount(MESHCORE_UART_SERVICE_UUID));
    TEST_ASSERT_EQUAL(0, server.getServiceConnectionCount(BITCHAT_SERVICE_UUID));
    
    // When: BitChat app connects
    server.simulateClientConnect(BITCHAT_SERVICE_UUID, /*conn_id=*/2);
    
    // Then: Both are connected
    TEST_ASSERT_TRUE(server.isServiceConnected(MESHCORE_UART_SERVICE_UUID));
    TEST_ASSERT_TRUE(server.isServiceConnected(BITCHAT_SERVICE_UUID));
    TEST_ASSERT_EQUAL(1, server.getServiceConnectionCount(MESHCORE_UART_SERVICE_UUID));
    TEST_ASSERT_EQUAL(1, server.getServiceConnectionCount(BITCHAT_SERVICE_UUID));
    
    // And: Total connections = 2
    TEST_ASSERT_EQUAL(2, server.getTotalConnectionCount());
    
    server.end();
}

/**
 * Scenario: Data isolation between services
 * 
 * Given both services have connected clients
 * When data is sent to MeshCore service
 * Then only MeshCore service receives the data
 * And BitChat service receives nothing
 */
void test_data_isolation_between_services(void) {
    SharedBLEServer server;
    server.begin("MeshCoreBitChatDevice");
    
    auto meshService = std::make_unique<MeshCoreUARTService>();
    auto* meshPtr = meshService.get();
    
    auto bitchatService = std::make_unique<BitchatBLEService>();
    auto* bitchatPtr = bitchatService.get();
    
    server.addService(
        MESHCORE_UART_SERVICE_UUID,
        std::move(meshService),
        BLESecurityLevel::PIN_REQUIRED,
        123456
    );
    
    server.addService(
        BITCHAT_SERVICE_UUID,
        std::move(bitchatService),
        BLESecurityLevel::OPEN
    );
    
    // Given: Both clients connected
    server.simulateClientConnect(MESHCORE_UART_SERVICE_UUID, /*conn_id=*/1);
    server.simulateClientConnect(BITCHAT_SERVICE_UUID, /*conn_id=*/2);
    
    // When: Data sent to MeshCore service
    uint8_t meshData[] = {0x01, 0x02, 0x03, 0x04};
    server.simulateDataWrite(MESHCORE_UART_SERVICE_UUID, meshData, sizeof(meshData));
    
    // Then: MeshCore received it
    TEST_ASSERT_TRUE(meshPtr->hasReceivedData());
    
    // And: BitChat did not
    TEST_ASSERT_FALSE(bitchatPtr->hasReceivedMessages());
    
    server.end();
}

/**
 * Scenario: Disconnect one service doesn't affect other
 * 
 * Given both services have connected clients
 * When MeshCore client disconnects
 * Then BitChat client remains connected
 * And MeshCore shows disconnected
 */
void test_disconnect_isolation(void) {
    SharedBLEServer server;
    server.begin("MeshCoreBitChatDevice");
    
    server.addService(
        MESHCORE_UART_SERVICE_UUID,
        std::make_unique<MeshCoreUARTService>(),
        BLESecurityLevel::PIN_REQUIRED,
        123456
    );
    
    server.addService(
        BITCHAT_SERVICE_UUID,
        std::make_unique<BitchatBLEService>(),
        BLESecurityLevel::OPEN
    );
    
    // Given: Both connected
    server.simulateClientConnect(MESHCORE_UART_SERVICE_UUID, /*conn_id=*/1);
    server.simulateClientConnect(BITCHAT_SERVICE_UUID, /*conn_id=*/2);
    
    TEST_ASSERT_TRUE(server.isServiceConnected(MESHCORE_UART_SERVICE_UUID));
    TEST_ASSERT_TRUE(server.isServiceConnected(BITCHAT_SERVICE_UUID));
    
    // When: MeshCore disconnects
    server.simulateClientDisconnect(/*conn_id=*/1);
    
    // Then: MeshCore is disconnected
    TEST_ASSERT_FALSE(server.isServiceConnected(MESHCORE_UART_SERVICE_UUID));
    TEST_ASSERT_EQUAL(0, server.getServiceConnectionCount(MESHCORE_UART_SERVICE_UUID));
    
    // And: BitChat remains connected
    TEST_ASSERT_TRUE(server.isServiceConnected(BITCHAT_SERVICE_UUID));
    TEST_ASSERT_EQUAL(1, server.getServiceConnectionCount(BITCHAT_SERVICE_UUID));
    
    server.end();
}

/**
 * Scenario: Multiple clients on same service
 * 
 * Given BitChat service is running
 * When two BitChat apps connect
 * Then both connections are tracked
 * And both receive broadcasts
 */
void test_multiple_clients_on_same_service(void) {
    SharedBLEServer server;
    server.begin("MeshCoreBitChatDevice");
    
    auto bitchatService = std::make_unique<BitchatBLEService>();
    auto* bitchatPtr = bitchatService.get();
    
    server.addService(
        BITCHAT_SERVICE_UUID,
        std::move(bitchatService),
        BLESecurityLevel::OPEN
    );
    
    // When: Two clients connect
    server.simulateClientConnect(BITCHAT_SERVICE_UUID, /*conn_id=*/1);
    server.simulateClientConnect(BITCHAT_SERVICE_UUID, /*conn_id=*/2);
    
    // Then: Both tracked
    TEST_ASSERT_EQUAL(2, server.getServiceConnectionCount(BITCHAT_SERVICE_UUID));
    TEST_ASSERT_EQUAL(2, bitchatPtr->getConnectedClientCount());
    
    server.end();
}

/**
 * Scenario: Connection state is tracked per-service
 * 
 * Given both services are running with different clients
 * When checking connection state
 * Then each service reports its own state independently
 */
void test_connection_state_per_service(void) {
    SharedBLEServer server;
    server.begin("MeshCoreBitChatDevice");
    
    auto meshService = std::make_unique<MeshCoreUARTService>();
    auto* meshPtr = meshService.get();
    
    auto bitchatService = std::make_unique<BitchatBLEService>();
    auto* bitchatPtr = bitchatService.get();
    
    server.addService(
        MESHCORE_UART_SERVICE_UUID,
        std::move(meshService),
        BLESecurityLevel::PIN_REQUIRED,
        123456
    );
    
    server.addService(
        BITCHAT_SERVICE_UUID,
        std::move(bitchatService),
        BLESecurityLevel::OPEN
    );
    
    // Initially neither connected
    TEST_ASSERT_FALSE(meshPtr->hasConnectedClients());
    TEST_ASSERT_FALSE(bitchatPtr->hasConnectedClients());
    
    // When: MeshCore client connects
    server.simulateClientConnect(MESHCORE_UART_SERVICE_UUID, /*conn_id=*/1);
    
    // Then: Only MeshCore reports connected
    TEST_ASSERT_TRUE(meshPtr->hasConnectedClients());
    TEST_ASSERT_FALSE(bitchatPtr->hasConnectedClients());
    TEST_ASSERT_EQUAL(1, meshPtr->getConnectedClientCount());
    TEST_ASSERT_EQUAL(0, bitchatPtr->getConnectedClientCount());
    
    // When: BitChat client also connects (different ID)
    server.simulateClientConnect(BITCHAT_SERVICE_UUID, /*conn_id=*/2);
    
    // Then: Both report connected
    TEST_ASSERT_TRUE(meshPtr->hasConnectedClients());
    TEST_ASSERT_TRUE(bitchatPtr->hasConnectedClients());
    TEST_ASSERT_EQUAL(1, meshPtr->getConnectedClientCount());
    TEST_ASSERT_EQUAL(1, bitchatPtr->getConnectedClientCount());
    
    server.end();
}

/**
 * Scenario: Graceful handling of rapid connect/disconnect
 * 
 * Given services are running
 * When clients rapidly connect and disconnect
 * Then state remains consistent
 * And no memory corruption occurs
 */
void test_rapid_connect_disconnect(void) {
    SharedBLEServer server;
    server.begin("MeshCoreBitChatDevice");
    
    server.addService(
        BITCHAT_SERVICE_UUID,
        std::make_unique<BitchatBLEService>(),
        BLESecurityLevel::OPEN
    );
    
    // Rapid connect/disconnect cycles
    for (int i = 0; i < 10; i++) {
        server.simulateClientConnect(BITCHAT_SERVICE_UUID, /*conn_id=*/1);
        TEST_ASSERT_TRUE(server.isServiceConnected(BITCHAT_SERVICE_UUID));
        
        server.simulateClientDisconnect(/*conn_id=*/1);
        TEST_ASSERT_FALSE(server.isServiceConnected(BITCHAT_SERVICE_UUID));
    }
    
    // State should be clean
    TEST_ASSERT_EQUAL(0, server.getServiceConnectionCount(BITCHAT_SERVICE_UUID));
    
    server.end();
}

} // namespace bitchat
} // namespace test
