/**
 * Story 1.2: MeshCore BLE Service Preservation - BDD Tests
 * 
 * Feature: MeshCore UART BLE Service Unchanged
 */

#include <unity.h>
#include "helpers/ble/SharedBLEServer.h"
#include "helpers/ble/MeshCoreUARTService.h"

#ifdef NATIVE_TEST
  #include "../mocks/MockBLE.h"
  // No SerialBLEInterface mock needed for native tests
#else
  #include <helpers/esp32/SerialBLEInterface.h>
  // or #include <helpers/nrf52/SerialBLEInterface.h>
#endif

using namespace mesh::ble;

namespace test {
namespace bitchat {

/**
 * Scenario: MeshCore companion app connects as before
 * 
 * Given the device has ENABLE_BITCHAT defined
 * When MeshCore app scans for devices
 * Then Nordic UART Service (6E400001-...) should be visible
 * And PIN authentication should be required
 * And all MeshCore protocol frames should work
 */
void test_meshcore_service_preserves_nordic_uart_uuid(void) {
    // Given: SharedBLEServer initialized
    mesh::ble::SharedBLEServer server;
    TEST_ASSERT_TRUE(server.begin("MeshCoreDevice"));
    
    // When: MeshCore UART service added
    auto meshcoreService = std::make_unique<mesh::ble::MeshCoreUARTService>();
    bool added = server.addService(
        mesh::ble::MESHCORE_UART_SERVICE_UUID,
        std::move(meshcoreService),
        mesh::ble::BLESecurityLevel::PIN_REQUIRED,
        123456  // PIN code
    );
    
    // Then: Service added successfully
    TEST_ASSERT_TRUE(added);
    TEST_ASSERT_TRUE(server.hasService(mesh::ble::MESHCORE_UART_SERVICE_UUID));
    
    // And: Correct UUID used
    TEST_ASSERT_EQUAL_STRING(
        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E",
        mesh::ble::MESHCORE_UART_SERVICE_UUID
    );
    
    server.end();
}

/**
 * Scenario: PIN authentication still required
 * 
 * Given MeshCore UART service is registered with PIN
 * When a client attempts to connect
 * Then PIN authentication should be required
 * And connection should only succeed with correct PIN
 */
void test_meshcore_service_requires_pin_authentication(void) {
    mesh::ble::SharedBLEServer server;
    server.begin("MeshCoreDevice");
    
    uint32_t pinCode = 123456;
    
    // Given: MeshCore service with PIN
    server.addService(
        mesh::ble::MESHCORE_UART_SERVICE_UUID,
        std::make_unique<mesh::ble::MeshCoreUARTService>(),
        mesh::ble::BLESecurityLevel::PIN_REQUIRED,
        pinCode
    );
    
    // Then: Security level is PIN_REQUIRED
    TEST_ASSERT_EQUAL(
        mesh::ble::BLESecurityLevel::PIN_REQUIRED,
        server.getServiceSecurityLevel(mesh::ble::MESHCORE_UART_SERVICE_UUID)
    );
    
    // And: Service requires PIN
    TEST_ASSERT_TRUE(
        server.serviceRequiresPIN(mesh::ble::MESHCORE_UART_SERVICE_UUID)
    );
    
    server.end();
}

/**
 * Scenario: Service works with SharedBLEServer
 * 
 * Given SharedBLEServer has MeshCore UART service
 * And SharedBLEServer has BitChat service
 * When MeshCore app connects to UART service
 * Then connection should succeed with PIN
 * And data should flow correctly
 * And BitChat service should be unaffected
 */
void test_meshcore_service_works_with_shared_server(void) {
    mesh::ble::SharedBLEServer server;
    server.begin("MeshCoreDevice");
    
    // Given: Both services registered
    auto meshcoreService = std::make_unique<mesh::ble::MeshCoreUARTService>();
    auto* meshcorePtr = meshcoreService.get();
    
    server.addService(
        mesh::ble::MESHCORE_UART_SERVICE_UUID,
        std::move(meshcoreService),
        mesh::ble::BLESecurityLevel::PIN_REQUIRED,
        123456
    );
    
    server.addService(
        mesh::ble::BITCHAT_SERVICE_UUID,
        std::make_unique<mesh::ble::BitchatBLEService>(),
        mesh::ble::BLESecurityLevel::OPEN
    );
    
    // When: Client connects to MeshCore service
    server.simulateClientConnect(mesh::ble::MESHCORE_UART_SERVICE_UUID, /*conn_id=*/1);
    
    // Then: MeshCore service shows connected
    TEST_ASSERT_TRUE(server.isServiceConnected(mesh::ble::MESHCORE_UART_SERVICE_UUID));
    TEST_ASSERT_EQUAL(1, server.getServiceConnectionCount(mesh::ble::MESHCORE_UART_SERVICE_UUID));
    
    // And: MeshCore service received connect callback
    TEST_ASSERT_TRUE(meshcorePtr->hasConnectedClients());
    
    // And: BitChat service is NOT affected
    TEST_ASSERT_FALSE(server.isServiceConnected(mesh::ble::BITCHAT_SERVICE_UUID));
    
    server.end();
}

/**
 * Scenario: MeshCore protocol frames work correctly
 * 
 * Given MeshCore UART service is connected
 * When MeshCore protocol frames are sent/received
 * Then frames should be processed correctly
 * And frame queue should work as expected
 */
void test_meshcore_service_protocol_frames(void) {
    mesh::ble::SharedBLEServer server;
    server.begin("MeshCoreDevice");
    
    auto meshcoreService = std::make_unique<mesh::ble::MeshCoreUARTService>();
    auto* meshcorePtr = meshcoreService.get();
    
    server.addService(
        mesh::ble::MESHCORE_UART_SERVICE_UUID,
        std::move(meshcoreService),
        mesh::ble::BLESecurityLevel::PIN_REQUIRED,
        123456
    );
    
    server.simulateClientConnect(mesh::ble::MESHCORE_UART_SERVICE_UUID, /*conn_id=*/1);
    
    // When: MeshCore frame received (header + data)
    // Frame format: [header][length][payload...]
    uint8_t frame[] = {
        0x01,  // header byte
        0x04,  // length
        0xAA, 0xBB, 0xCC, 0xDD  // payload
    };
    
    server.simulateDataWrite(mesh::ble::MESHCORE_UART_SERVICE_UUID, frame, sizeof(frame));
    
    // Then: Frame was received by service
    TEST_ASSERT_TRUE(meshcorePtr->wasDataReceived());
    
    // And: Frame is in receive queue
    TEST_ASSERT_TRUE(meshcorePtr->hasReceiveData());
    
    // When: Frame retrieved
    uint8_t receivedFrame[256];
    size_t len = meshcorePtr->readFrame(receivedFrame, sizeof(receivedFrame));
    
    // Then: Frame matches what was sent
    TEST_ASSERT_EQUAL(sizeof(frame), len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(frame, receivedFrame, len);
    
    server.end();
}

/**
 * Scenario: Backward compatibility with existing code
 * 
 * Given existing code uses SerialBLEInterface
 * When MeshCoreUARTService wraps it
 * Then existing functionality should still work
 * And no changes needed to upper layers
 */
void test_meshcore_service_backward_compatibility(void) {
    mesh::ble::SharedBLEServer server;
    server.begin("MeshCoreDevice");
    
    // Create service with typical MeshCore settings
    auto meshcoreService = std::make_unique<mesh::ble::MeshCoreUARTService>();
    
    // Configure with typical MeshCore parameters
    meshcoreService->setDeviceName("MeshCore Node");
    meshcoreService->setPIN(123456);
    
    server.addService(
        mesh::ble::MESHCORE_UART_SERVICE_UUID,
        std::move(meshcoreService),
        mesh::ble::BLESecurityLevel::PIN_REQUIRED,
        123456
    );
    
    // Then: Service provides same interface as before
    auto* service = dynamic_cast<mesh::ble::MeshCoreUARTService*>(
        server.getService(mesh::ble::MESHCORE_UART_SERVICE_UUID)
    );
    
    TEST_ASSERT_NOT_NULL(service);
    TEST_ASSERT_EQUAL_STRING("MeshCore Node", service->getDeviceName());
    TEST_ASSERT_EQUAL(123456, service->getPIN());
    
    // And: Can check connection status
    TEST_ASSERT_FALSE(service->isConnected());
    
    server.simulateClientConnect(mesh::ble::MESHCORE_UART_SERVICE_UUID, /*conn_id=*/1);
    TEST_ASSERT_TRUE(service->isConnected());
    
    server.end();
}

/**
 * Scenario: Service maintains frame queue
 * 
 * Given multiple frames received
 * When frames are read
 * Then they should be returned in FIFO order
 * And queue should not overflow
 */
void test_meshcore_service_frame_queue(void) {
    mesh::ble::SharedBLEServer server;
    server.begin("MeshCoreDevice");
    
    auto meshcoreService = std::make_unique<mesh::ble::MeshCoreUARTService>();
    auto* meshcorePtr = meshcoreService.get();
    
    server.addService(
        mesh::ble::MESHCORE_UART_SERVICE_UUID,
        std::move(meshcoreService),
        mesh::ble::BLESecurityLevel::PIN_REQUIRED,
        123456
    );
    
    server.simulateClientConnect(mesh::ble::MESHCORE_UART_SERVICE_UUID, /*conn_id=*/1);
    
    // Given: Multiple frames received
    uint8_t frame1[] = {0x01, 0x02, 0x03};
    uint8_t frame2[] = {0x04, 0x05, 0x06};
    uint8_t frame3[] = {0x07, 0x08, 0x09};
    
    server.simulateDataWrite(mesh::ble::MESHCORE_UART_SERVICE_UUID, frame1, sizeof(frame1));
    server.simulateDataWrite(mesh::ble::MESHCORE_UART_SERVICE_UUID, frame2, sizeof(frame2));
    server.simulateDataWrite(mesh::ble::MESHCORE_UART_SERVICE_UUID, frame3, sizeof(frame3));
    
    // Then: All frames in queue
    TEST_ASSERT_EQUAL(3, meshcorePtr->getReceiveQueueSize());
    
    // When: Read frames
    uint8_t buf[256];
    size_t len;
    
    // Then: FIFO order preserved
    len = meshcorePtr->readFrame(buf, sizeof(buf));
    TEST_ASSERT_EQUAL(sizeof(frame1), len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(frame1, buf, len);
    
    len = meshcorePtr->readFrame(buf, sizeof(buf));
    TEST_ASSERT_EQUAL(sizeof(frame2), len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(frame2, buf, len);
    
    len = meshcorePtr->readFrame(buf, sizeof(buf));
    TEST_ASSERT_EQUAL(sizeof(frame3), len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(frame3, buf, len);
    
    // And: Queue empty
    TEST_ASSERT_EQUAL(0, meshcorePtr->getReceiveQueueSize());
    
    server.end();
}

// Tests are called from test_main.cpp
} // namespace bitchat
} // namespace test
