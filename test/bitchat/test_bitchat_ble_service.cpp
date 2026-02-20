/**
 * Story 1.3: BitChat BLE Service Addition - BDD Tests
 * 
 * Feature: BitChat BLE Service Coexistence
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
 * Scenario: BitChat service added to existing BLE server
 * 
 * Given MeshCore UART service is running
 * When BitChat service is attached to same server
 * Then BitChat UUID (F47B5E2D-...) should be visible in scan response
 * And BitChat should not require PIN
 * And MeshCore UART should still require PIN
 */
void test_bitchat_service_adds_to_shared_server(void) {
    // Given: SharedBLEServer with MeshCore UART service
    SharedBLEServer server;
    TEST_ASSERT_TRUE(server.begin("MeshCoreBitChatDevice"));
    
    server.addService(
        MESHCORE_UART_SERVICE_UUID,
        std::make_unique<MeshCoreUARTService>(),
        BLESecurityLevel::PIN_REQUIRED,
        123456
    );
    
    // When: BitChat service added
    auto bitchatService = std::make_unique<BitchatBLEService>();
    bool added = server.addService(
        BITCHAT_SERVICE_UUID,
        std::move(bitchatService),
        BLESecurityLevel::OPEN
    );
    
    // Then: BitChat service added successfully
    TEST_ASSERT_TRUE(added);
    TEST_ASSERT_TRUE(server.hasService(BITCHAT_SERVICE_UUID));
    
    // And: Correct BitChat UUID
    TEST_ASSERT_EQUAL_STRING(
        "F47B5E2D-4A9E-4C5A-9B3F-8E1D2C3A4B5C",
        BITCHAT_SERVICE_UUID
    );
    
    // And: BitChat has OPEN security
    TEST_ASSERT_EQUAL(
        BLESecurityLevel::OPEN,
        server.getServiceSecurityLevel(BITCHAT_SERVICE_UUID)
    );
    TEST_ASSERT_FALSE(server.serviceRequiresPIN(BITCHAT_SERVICE_UUID));
    
    // And: MeshCore still has PIN_REQUIRED
    TEST_ASSERT_EQUAL(
        BLESecurityLevel::PIN_REQUIRED,
        server.getServiceSecurityLevel(MESHCORE_UART_SERVICE_UUID)
    );
    TEST_ASSERT_TRUE(server.serviceRequiresPIN(MESHCORE_UART_SERVICE_UUID));
    
    server.end();
}

/**
 * Scenario: BitChat service has open security
 * 
 * Given BitChat service is registered
 * When a client attempts to connect
 * Then no authentication should be required
 * And connection should succeed immediately
 */
void test_bitchat_service_has_open_security(void) {
    SharedBLEServer server;
    server.begin("MeshCoreBitChatDevice");
    
    // Given: BitChat service with OPEN security
    server.addService(
        BITCHAT_SERVICE_UUID,
        std::make_unique<BitchatBLEService>(),
        BLESecurityLevel::OPEN
    );
    
    // Then: Security level is OPEN
    TEST_ASSERT_EQUAL(
        BLESecurityLevel::OPEN,
        server.getServiceSecurityLevel(BITCHAT_SERVICE_UUID)
    );
    
    // When: Client connects
    server.simulateClientConnect(BITCHAT_SERVICE_UUID, /*conn_id=*/1);
    
    // Then: Connection succeeds (no PIN needed)
    TEST_ASSERT_TRUE(server.isServiceConnected(BITCHAT_SERVICE_UUID));
    
    server.end();
}

/**
 * Scenario: BitChat service handles MESSAGE type
 * 
 * Given BitChat service is connected
 * When BitChat MESSAGE packet is received
 * Then it should parse the message correctly
 * And extract sender, content, and channel
 */
void test_bitchat_service_handles_message_type(void) {
    SharedBLEServer server;
    server.begin("MeshCoreBitChatDevice");
    
    auto bitchatService = std::make_unique<BitchatBLEService>();
    auto* bitchatPtr = bitchatService.get();
    
    server.addService(
        BITCHAT_SERVICE_UUID,
        std::move(bitchatService),
        BLESecurityLevel::OPEN
    );
    
    server.simulateClientConnect(BITCHAT_SERVICE_UUID, /*conn_id=*/1);
    
    // When: BitChat MESSAGE received
    // Format: header(14) + sender(8) + payload(channel:text)
    uint8_t message[] = {
        // Header
        0x01,  // version
        0x02,  // type = MESSAGE
        0x08,  // ttl
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // timestamp
        0x00,  // flags
        0x00, 0x0A,  // payload length = 10 (big-endian)
        // Sender ID (8 bytes)
        0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0,
        // Payload: "#mesh:test"
        0x23, 0x6D, 0x65, 0x73, 0x68, 0x3A, 0x74, 0x65, 0x73, 0x74
    };
    
    server.simulateDataWrite(BITCHAT_SERVICE_UUID, message, sizeof(message));
    
    // Then: Message was received and parsed
    TEST_ASSERT_TRUE(bitchatPtr->hasReceivedMessages());
    
    BitchatMessage msg;
    TEST_ASSERT_TRUE(bitchatPtr->getNextMessage(msg));
    
    // And: Fields extracted correctly
    TEST_ASSERT_EQUAL(0x02, msg.type);  // MESSAGE type
    TEST_ASSERT_EQUAL(0x0A, msg.payloadLength);  // 10 bytes
    
    server.end();
}

/**
 * Scenario: BitChat service handles ANNOUNCE type
 * 
 * Given BitChat service is connected
 * When BitChat ANNOUNCE packet is received
 * Then it should parse peer info
 * And cache peer nickname and public key
 */
void test_bitchat_service_handles_announce_type(void) {
    SharedBLEServer server;
    server.begin("MeshCoreBitChatDevice");
    
    auto bitchatService = std::make_unique<BitchatBLEService>();
    auto* bitchatPtr = bitchatService.get();
    
    server.addService(
        BITCHAT_SERVICE_UUID,
        std::move(bitchatService),
        BLESecurityLevel::OPEN
    );
    
    server.simulateClientConnect(BITCHAT_SERVICE_UUID, /*conn_id=*/1);
    
    // When: BitChat ANNOUNCE received
    uint8_t announce[] = {
        // Header
        0x01,  // version
        0x01,  // type = ANNOUNCE
        0x08,  // ttl
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // timestamp
        0x00,  // flags = 0 (no signature for this test)
        0x00, 0x29,  // payload length = 41 (nickname TLV + pubkey TLV) (big-endian)
        // Sender ID (8 bytes)
        0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0,
        // Payload: TLV encoded
        // Nickname TLV: type=0x01, len=5, "Alice"
        0x01, 0x05, 0x41, 0x6C, 0x69, 0x63, 0x65,
        // Ed25519 pubkey TLV: type=0x03, len=32
        0x03, 0x20,
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
    };
    
    server.simulateDataWrite(BITCHAT_SERVICE_UUID, announce, sizeof(announce));
    
    // Then: Peer info cached
    TEST_ASSERT_EQUAL(1, bitchatPtr->getPeerCacheSize());
    
    PeerInfo peer;
    TEST_ASSERT_TRUE(bitchatPtr->getPeerById(0xF0DEBC9A78563412, peer));
    TEST_ASSERT_EQUAL_STRING("Alice", peer.nickname);
    
    server.end();
}

/**
 * Scenario: BitChat service sends announcements
 * 
 * Given BitChat service is initialized
 * When periodic announcement timer fires
 * Then it should send ANNOUNCE message to connected clients
 */
void test_bitchat_service_sends_announcements(void) {
    SharedBLEServer server;
    server.begin("MeshCoreBitChatDevice");
    
    auto bitchatService = std::make_unique<BitchatBLEService>();
    auto* bitchatPtr = bitchatService.get();
    
    // Configure the service
    bitchatPtr->setNodeName("TestNode");
    bitchatPtr->setPeerId(0x123456789ABCDEF0);
    
    server.addService(
        BITCHAT_SERVICE_UUID,
        std::move(bitchatService),
        BLESecurityLevel::OPEN
    );
    
    server.simulateClientConnect(BITCHAT_SERVICE_UUID, /*conn_id=*/1);
    
    // When: Trigger announcement
    bitchatPtr->sendAnnouncement();
    
    // Then: Announcement was sent
    TEST_ASSERT_TRUE(bitchatPtr->wasAnnouncementSent());
    
    server.end();
}

/**
 * Scenario: Both services advertise correctly
 * 
 * Given SharedBLEServer with both MeshCore and BitChat
 * When advertising starts
 * Then both service UUIDs should be in advertisement
 * And clients should discover both services
 */
void test_both_services_advertise_correctly(void) {
    SharedBLEServer server;
    server.begin("MeshCoreBitChatDevice");
    
    // Add both services
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
    
    // Then: Both services registered
    TEST_ASSERT_EQUAL(2, server.getServiceCount());
    TEST_ASSERT_TRUE(server.hasService(MESHCORE_UART_SERVICE_UUID));
    TEST_ASSERT_TRUE(server.hasService(BITCHAT_SERVICE_UUID));
    
    // When: Start advertising
    TEST_ASSERT_TRUE(server.startAdvertising());
    TEST_ASSERT_TRUE(server.isAdvertising());
    
    server.end();
}

/**
 * Scenario: BitChat message queue handling
 * 
 * Given multiple BitChat messages received
 * When messages are retrieved
 * Then they should be returned in FIFO order
 */
void test_bitchat_message_queue_fifo(void) {
    SharedBLEServer server;
    server.begin("MeshCoreBitChatDevice");
    
    auto bitchatService = std::make_unique<BitchatBLEService>();
    auto* bitchatPtr = bitchatService.get();
    
    server.addService(
        BITCHAT_SERVICE_UUID,
        std::move(bitchatService),
        BLESecurityLevel::OPEN
    );
    
    server.simulateClientConnect(BITCHAT_SERVICE_UUID, /*conn_id=*/1);
    
    // Given: Multiple messages received
    BitchatMessage msg1;
    msg1.type = BITCHAT_MSG_MESSAGE;
    msg1.setSenderId64(0x1111111111111111);
    memcpy(msg1.payload, "Message1", 8);
    msg1.payloadLength = 8;
    bitchatPtr->queueMessageForTest(msg1);
    
    BitchatMessage msg2;
    msg2.type = BITCHAT_MSG_MESSAGE;
    msg2.setSenderId64(0x2222222222222222);
    memcpy(msg2.payload, "Message2", 8);
    msg2.payloadLength = 8;
    bitchatPtr->queueMessageForTest(msg2);
    
    BitchatMessage msg3;
    msg3.type = BITCHAT_MSG_MESSAGE;
    msg3.setSenderId64(0x3333333333333333);
    memcpy(msg3.payload, "Message3", 8);
    msg3.payloadLength = 8;
    bitchatPtr->queueMessageForTest(msg3);
    
    // Then: Queue has 3 messages
    TEST_ASSERT_EQUAL(3, bitchatPtr->getMessageQueueSize());
    
    // When: Retrieve messages
    BitchatMessage received;
    
    // Then: FIFO order
    TEST_ASSERT_TRUE(bitchatPtr->getNextMessage(received));
    TEST_ASSERT_EQUAL(0x1111111111111111, received.getSenderId64());
    
    TEST_ASSERT_TRUE(bitchatPtr->getNextMessage(received));
    TEST_ASSERT_EQUAL(0x2222222222222222, received.getSenderId64());
    
    TEST_ASSERT_TRUE(bitchatPtr->getNextMessage(received));
    TEST_ASSERT_EQUAL(0x3333333333333333, received.getSenderId64());
    
    // And: Queue empty
    TEST_ASSERT_EQUAL(0, bitchatPtr->getMessageQueueSize());
    
    server.end();
}

// Tests are called from test_main.cpp
} // namespace bitchat
} // namespace test
