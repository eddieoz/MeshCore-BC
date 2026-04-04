/**
 * Story 5.3: Multi-Channel Encapsulation Routing - BDD Tests
 * 
 * Feature: Route Encapsulated BitChat to Multiple Channels
 * 
 * TDD Approach:
 * 1. Write failing tests (RED)
 * 2. Implement to pass (GREEN)
 * 3. Refactor (REFACTOR)
 */

#include <unity.h>
#include "helpers/bitchat/MultiChannelRouter.h"
#include "helpers/bitchat/ChannelRegistry.h"

#ifdef ENABLE_BITCHAT

using namespace mesh::bitchat;

namespace test {
namespace bitchat {

// Test channel keys
static const uint8_t MESH_CHANNEL_KEY[32] = {
    0x5B, 0x66, 0x93, 0x46, 0xD7, 0x55, 0xE8, 0xA4,
    0x23, 0xB8, 0xC9, 0x12, 0x45, 0x67, 0x89, 0xAB,
    0xCD, 0xEF, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB,
    0xCD, 0xEF, 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54
};

static const uint8_t GENERAL_CHANNEL_KEY[32] = {
    0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11,
    0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
    0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,
    0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA
};

static const uint8_t EMERGENCY_CHANNEL_KEY[32] = {
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00,
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00
};

/**
 * Scenario: BitChat #general message encapsulation
 * 
 * Given BitChat message for "#general"
 * And "general" maps to MeshCore channel X with secret S
 * When encapsulating for routing
 * Then create PAYLOAD_TYPE_GRP_DATA with channel X
 * And encrypt payload with secret S
 */
void test_route_to_registered_channel(void) {
    // Given: Channel registry with multiple channels
    ChannelRegistry registry;
    registry.registerChannel("mesh", MESH_CHANNEL_KEY, ChannelType::HASHTAG);
    registry.registerChannel("general", GENERAL_CHANNEL_KEY, ChannelType::HASHTAG);
    registry.registerChannel("emergency", EMERGENCY_CHANNEL_KEY, ChannelType::HASHTAG);
    
    // And: BitChat message for #general
    BitchatMessage msg;
    msg.version = BITCHAT_VERSION;
    msg.type = BITCHAT_MSG_MESSAGE;
    msg.ttl = 8;
    msg.timestamp = 1234567890;
    msg.setSenderId64(0x0807060504030201ULL);
    msg.payloadLength = 12;
    memcpy(msg.payload, "Hello World!", 12);
    
    MultiChannelRouter router(registry);
    RoutingResult result;
    
    // When: Route message for #general
    bool success = router.routeMessage(msg, "general", result);
    
    // Then: Should route successfully
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(RoutingStatus::ROUTED, result.status);
    TEST_ASSERT_EQUAL(1, result.packetCount);
    TEST_ASSERT_TRUE(result.packets[0].isValid);
    
    // And: Should use correct channel key (verify by successful decapsulation)
    BitchatMessage decapsulated;
    BitchatMessageEncapsulator encapsulator;
    bool decapsulated_ok = encapsulator.decapsulate(
        result.packets[0], 
        GENERAL_CHANNEL_KEY,  // Should work with general key
        decapsulated
    );
    TEST_ASSERT_TRUE(decapsulated_ok);
    TEST_ASSERT_EQUAL(msg.payloadLength, decapsulated.payloadLength);
    TEST_ASSERT_EQUAL_MEMORY(msg.payload, decapsulated.payload, msg.payloadLength);
}

/**
 * Scenario: Route to different channels
 * 
 * Given multiple registered channels
 * When routing messages to each
 * Then each should use its own channel key
 */
void test_route_to_different_channels(void) {
    // Given: Channel registry with multiple channels
    ChannelRegistry registry;
    registry.registerChannel("mesh", MESH_CHANNEL_KEY, ChannelType::HASHTAG);
    registry.registerChannel("general", GENERAL_CHANNEL_KEY, ChannelType::HASHTAG);
    
    MultiChannelRouter router(registry);
    
    // Message 1 for #mesh
    BitchatMessage msg1;
    msg1.version = BITCHAT_VERSION;
    msg1.type = BITCHAT_MSG_MESSAGE;
    msg1.ttl = 8;
    msg1.timestamp = 1234567890;
    msg1.setSenderId64(0x0807060504030201ULL);
    msg1.payloadLength = 10;
    memcpy(msg1.payload, "MeshMsg", 7);
    
    RoutingResult result1;
    bool success1 = router.routeMessage(msg1, "mesh", result1);
    TEST_ASSERT_TRUE(success1);
    TEST_ASSERT_EQUAL(RoutingStatus::ROUTED, result1.status);
    
    // Verify it was encrypted with mesh key
    BitchatMessage decapsulated1;
    BitchatMessageEncapsulator encapsulator;
    TEST_ASSERT_TRUE(encapsulator.decapsulate(result1.packets[0], MESH_CHANNEL_KEY, decapsulated1));
    
    // Message 2 for #general
    BitchatMessage msg2;
    msg2.version = BITCHAT_VERSION;
    msg2.type = BITCHAT_MSG_MESSAGE;
    msg2.ttl = 8;
    msg2.timestamp = 1234567891;
    msg2.setSenderId64(0x0807060504030201ULL);
    msg2.payloadLength = 10;
    memcpy(msg2.payload, "GeneralMsg", 10);
    
    RoutingResult result2;
    bool success2 = router.routeMessage(msg2, "general", result2);
    TEST_ASSERT_TRUE(success2);
    TEST_ASSERT_EQUAL(RoutingStatus::ROUTED, result2.status);
    
    // Verify it was encrypted with general key
    BitchatMessage decapsulated2;
    TEST_ASSERT_TRUE(encapsulator.decapsulate(result2.packets[0], GENERAL_CHANNEL_KEY, decapsulated2));
}

/**
 * Scenario: Unknown channel handling
 * 
 * Given BitChat message for "#unknown"
 * And "unknown" is not registered
 * Then return NOT_FOUND status
 * And drop message
 */
void test_unknown_channel_handling(void) {
    // Given: Channel registry with only #mesh
    ChannelRegistry registry;
    registry.registerChannel("mesh", MESH_CHANNEL_KEY, ChannelType::HASHTAG);
    
    // And: BitChat message for unknown channel
    BitchatMessage msg;
    msg.version = BITCHAT_VERSION;
    msg.type = BITCHAT_MSG_MESSAGE;
    msg.ttl = 8;
    msg.timestamp = 1234567890;
    msg.setSenderId64(0x0807060504030201ULL);
    msg.payloadLength = 10;
    memcpy(msg.payload, "Test", 4);
    
    MultiChannelRouter router(registry);
    RoutingResult result;
    
    // When: Route message for unknown channel
    bool success = router.routeMessage(msg, "unknown", result);
    
    // Then: Should fail with NOT_FOUND
    TEST_ASSERT_FALSE(success);
    TEST_ASSERT_EQUAL(RoutingStatus::CHANNEL_NOT_FOUND, result.status);
    TEST_ASSERT_EQUAL(0, result.packetCount);
}

/**
 * Scenario: Default channel for unknown channels
 * 
 * Given default channel is configured
 * When routing to unknown channel
 * Then route to default channel
 */
void test_default_channel_for_unknown(void) {
    // Given: Channel registry with mesh and general
    ChannelRegistry registry;
    registry.registerChannel("mesh", MESH_CHANNEL_KEY, ChannelType::HASHTAG);
    registry.registerChannel("general", GENERAL_CHANNEL_KEY, ChannelType::HASHTAG);
    
    // And: Router with default channel set to "mesh"
    MultiChannelRouter router(registry);
    router.setDefaultChannel("mesh");
    
    // And: BitChat message for unknown channel
    BitchatMessage msg;
    msg.version = BITCHAT_VERSION;
    msg.type = BITCHAT_MSG_MESSAGE;
    msg.ttl = 8;
    msg.timestamp = 1234567890;
    msg.setSenderId64(0x0807060504030201ULL);
    msg.payloadLength = 10;
    memcpy(msg.payload, "TestMessage", 11);
    
    RoutingResult result;
    
    // When: Route message for unknown channel
    bool success = router.routeMessage(msg, "unknownchannel", result);
    
    // Then: Should route to default (mesh)
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(RoutingStatus::ROUTED_TO_DEFAULT, result.status);
    TEST_ASSERT_EQUAL(1, result.packetCount);
    
    // Verify it was encrypted with mesh key (the default)
    BitchatMessage decapsulated;
    BitchatMessageEncapsulator encapsulator;
    TEST_ASSERT_TRUE(encapsulator.decapsulate(result.packets[0], MESH_CHANNEL_KEY, decapsulated));
    TEST_ASSERT_EQUAL(msg.payloadLength, decapsulated.payloadLength);
}

/**
 * Scenario: Large message fragmentation across channels
 * 
 * Given large BitChat message for #general
 * When routing
 * Then create multiple fragments
 * All using correct channel key
 */
void test_fragmentation_with_channel_routing(void) {
    // Given: Channel registry
    ChannelRegistry registry;
    registry.registerChannel("general", GENERAL_CHANNEL_KEY, ChannelType::HASHTAG);
    
    // And: Large BitChat message (250 bytes payload)
    BitchatMessage msg;
    msg.version = BITCHAT_VERSION;
    msg.type = BITCHAT_MSG_MESSAGE;
    msg.ttl = 8;
    msg.timestamp = 1234567890;
    msg.setSenderId64(0x0807060504030201ULL);
    msg.payloadLength = 250;
    for (int i = 0; i < 250; i++) {
        msg.payload[i] = 'A' + (i % 26);
    }
    
    MultiChannelRouter router(registry);
    RoutingResult result;
    
    // When: Route large message
    bool success = router.routeMessage(msg, "general", result);
    
    // Then: Should create multiple fragments
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(RoutingStatus::ROUTED, result.status);
    TEST_ASSERT_TRUE(result.packetCount > 1);  // Should be fragmented
    
    // And: All fragments should be valid encapsulated packets
    for (size_t i = 0; i < result.packetCount; i++) {
        // Each fragment should be valid
        TEST_ASSERT_TRUE(result.packets[i].isValid);
        
        // Should have BitChat encapsulation magic
        TEST_ASSERT_EQUAL_HEX8('B', result.packets[i].data[0]);
        TEST_ASSERT_EQUAL_HEX8('C', result.packets[i].data[1]);
        
        // Should have correct version
        TEST_ASSERT_EQUAL_HEX8(BITCHAT_ENCAPSULATION_VERSION, result.packets[i].data[4]);
        
        // Fragment info should be set
        TEST_ASSERT_TRUE(result.packets[i].isFragment || result.packetCount == 1);
    }
    
    // Verify first fragment can be decrypted (contains start of message)
    // Note: Full fragment reassembly is tested separately in Story 5.2
    BitchatMessageEncapsulator encapsulator;
    uint8_t decrypted[300];
    
    // Get first fragment and decrypt just the payload portion
    const EncapsulatedPacket& firstFrag = result.packets[0];
    size_t payloadLen = firstFrag.length - BITCHAT_ENCAPSULATION_HEADER_SIZE;
    
    // Simple XOR decrypt to verify it decrypts with correct key
    for (size_t i = 0; i < payloadLen; i++) {
        decrypted[i] = firstFrag.data[BITCHAT_ENCAPSULATION_HEADER_SIZE + i] ^ 
                       GENERAL_CHANNEL_KEY[i % 32];
    }
    
    // Verify decrypted data starts with BitChat message header (version, type, ttl)
    TEST_ASSERT_EQUAL(msg.version, decrypted[0]);
    TEST_ASSERT_EQUAL(msg.type, decrypted[1]);
    TEST_ASSERT_EQUAL(msg.ttl, decrypted[2]);
}

/**
 * Scenario: Channel name normalization
 * 
 * Given channel name with # prefix
 * When routing
 * Then normalize and find matching channel
 */
void test_channel_name_normalization(void) {
    // Given: Channel registry (stores without #)
    ChannelRegistry registry;
    registry.registerChannel("general", GENERAL_CHANNEL_KEY, ChannelType::HASHTAG);
    
    // And: Message with #general
    BitchatMessage msg;
    msg.version = BITCHAT_VERSION;
    msg.type = BITCHAT_MSG_MESSAGE;
    msg.ttl = 8;
    msg.timestamp = 1234567890;
    msg.setSenderId64(0x0807060504030201ULL);
    msg.payloadLength = 10;
    memcpy(msg.payload, "Test", 4);
    
    MultiChannelRouter router(registry);
    RoutingResult result;
    
    // When: Route with # prefix
    bool success = router.routeMessage(msg, "#general", result);
    
    // Then: Should route successfully (normalizing the name)
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(RoutingStatus::ROUTED, result.status);
}

/**
 * Scenario: Null or empty channel name
 * 
 * Given null or empty channel name
 * When routing
 * Then should fail gracefully
 */
void test_null_empty_channel_name(void) {
    ChannelRegistry registry;
    registry.registerChannel("mesh", MESH_CHANNEL_KEY, ChannelType::HASHTAG);
    
    BitchatMessage msg;
    msg.version = BITCHAT_VERSION;
    msg.type = BITCHAT_MSG_MESSAGE;
    msg.ttl = 8;
    msg.timestamp = 1234567890;
    msg.setSenderId64(0x0807060504030201ULL);
    msg.payloadLength = 10;
    memcpy(msg.payload, "Test", 4);
    
    MultiChannelRouter router(registry);
    router.setDefaultChannel("mesh");
    
    RoutingResult result;
    
    // Test null channel name
    bool success1 = router.routeMessage(msg, nullptr, result);
    TEST_ASSERT_TRUE(success1);  // Should use default
    TEST_ASSERT_EQUAL(RoutingStatus::ROUTED_TO_DEFAULT, result.status);
    
    // Test empty channel name
    bool success2 = router.routeMessage(msg, "", result);
    TEST_ASSERT_TRUE(success2);  // Should use default
    TEST_ASSERT_EQUAL(RoutingStatus::ROUTED_TO_DEFAULT, result.status);
}

/**
 * Scenario: Multiple messages routed correctly
 * 
 * Given multiple messages to different channels
 * When routing each
 * Then each uses correct channel key
 */
void test_multiple_messages_multiple_channels(void) {
    // Given: Registry with 3 channels
    ChannelRegistry registry;
    registry.registerChannel("mesh", MESH_CHANNEL_KEY, ChannelType::HASHTAG);
    registry.registerChannel("general", GENERAL_CHANNEL_KEY, ChannelType::HASHTAG);
    registry.registerChannel("emergency", EMERGENCY_CHANNEL_KEY, ChannelType::HASHTAG);
    
    MultiChannelRouter router(registry);
    BitchatMessageEncapsulator encapsulator;
    
    // Messages to different channels
    const char* channels[] = {"mesh", "general", "emergency"};
    const uint8_t* keys[] = {MESH_CHANNEL_KEY, GENERAL_CHANNEL_KEY, EMERGENCY_CHANNEL_KEY};
    const char* payloads[] = {"MeshMsg", "GeneralMsg", "EmergencyMsg"};
    
    for (int i = 0; i < 3; i++) {
        BitchatMessage msg;
        msg.version = BITCHAT_VERSION;
        msg.type = BITCHAT_MSG_MESSAGE;
        msg.ttl = 8;
        msg.timestamp = 1234567890 + i;
        msg.setSenderId64(0x0807060504030201ULL);
        msg.payloadLength = strlen(payloads[i]);
        memcpy(msg.payload, payloads[i], msg.payloadLength);
        
        RoutingResult result;
        bool success = router.routeMessage(msg, channels[i], result);
        
        TEST_ASSERT_TRUE(success);
        TEST_ASSERT_EQUAL(RoutingStatus::ROUTED, result.status);
        
        // Verify it was encrypted with correct key
        BitchatMessage decapsulated;
        TEST_ASSERT_TRUE(encapsulator.decapsulate(result.packets[0], keys[i], decapsulated));
        TEST_ASSERT_EQUAL(msg.payloadLength, decapsulated.payloadLength);
        TEST_ASSERT_EQUAL_MEMORY(msg.payload, decapsulated.payload, msg.payloadLength);
    }
}

} // namespace bitchat
} // namespace test

#endif // ENABLE_BITCHAT
