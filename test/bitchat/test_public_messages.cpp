/**
 * Story 8.1: Public Message to BitChat - BDD Tests
 * 
 * Feature: Route Public Messages to BitChat
 * 
 * TDD Approach:
 * 1. Write failing tests (RED)
 * 2. Implement to pass (GREEN)
 * 3. Refactor (REFACTOR)
 */

#include <unity.h>
#include "helpers/bitchat/PublicMessageHandler.h"
#include "helpers/bitchat/ChannelRegistry.h"

#ifdef ENABLE_BITCHAT

using namespace mesh::bitchat;
using namespace mesh::ble;

namespace test {
namespace bitchat {

static const uint8_t PUBLIC_CHANNEL_KEY[32] = {
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00,
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00
};

/**
 * Scenario: MeshCore public message
 * 
 * Given MeshCore public message sent
 * When BitChat clients connected
 * Then broadcast to BitChat as channel message
 * And channel could be special "#public" or #mesh
 */
void test_route_public_message_to_bitchat(void) {
    // Given: Channel registry with public channel
    ChannelRegistry registry;
    registry.registerChannel("public", PUBLIC_CHANNEL_KEY, ChannelType::HASHTAG);
    
    // And: Public message handler
    PublicMessageHandler handler(registry);
    handler.setTargetChannel("public");
    
    // And: MeshCore public message
    MeshCorePublicMessage pubMsg;
    pubMsg.senderName = "RadioNode";
    pubMsg.content = "Public announcement for all!";
    pubMsg.timestamp = 1704067200;
    
    // When: Route to BitChat
    BitchatMessage result;
    bool success = handler.routeToBitChat(pubMsg, result);
    
    // Then: Should succeed
    TEST_ASSERT_TRUE(success);
    
    // And: Should be MESSAGE type
    TEST_ASSERT_EQUAL(BITCHAT_MSG_MESSAGE, result.type);
    
    // And: Should have formatted content
    TEST_ASSERT_TRUE(result.payloadLength > 0);
    TEST_ASSERT_EQUAL_STRING("<RadioNode> Public announcement for all!", (char*)result.payload);
}

/**
 * Scenario: Default target channel
 * 
 * Given no target channel configured
 * When routing public message
 * Then use #mesh as default
 */
void test_default_target_channel(void) {
    // Given: Channel registry with mesh channel
    ChannelRegistry registry;
    registry.registerChannel("mesh", PUBLIC_CHANNEL_KEY, ChannelType::HASHTAG);
    
    // And: Handler without explicit target (should default to mesh)
    PublicMessageHandler handler(registry);
    
    // And: Public message
    MeshCorePublicMessage pubMsg;
    pubMsg.senderName = "Node1";
    pubMsg.content = "Hello mesh!";
    pubMsg.timestamp = 1704067200;
    
    // When: Route
    BitchatMessage result;
    bool success = handler.routeToBitChat(pubMsg, result);
    
    // Then: Should route to default mesh channel
    TEST_ASSERT_TRUE(success);
}

/**
 * Scenario: Configurable target channel
 * 
 * Given custom target channel configured
 * When routing public message
 * Then use configured channel
 */
void test_configurable_target_channel(void) {
    // Given: Channel registry with custom channel
    ChannelRegistry registry;
    registry.registerChannel("emergency", PUBLIC_CHANNEL_KEY, ChannelType::HASHTAG);
    
    // And: Handler configured for emergency channel
    PublicMessageHandler handler(registry);
    handler.setTargetChannel("emergency");
    
    // And: Public message
    MeshCorePublicMessage pubMsg;
    pubMsg.senderName = "EmergencyBot";
    pubMsg.content = "Emergency alert!";
    pubMsg.timestamp = 1704067200;
    
    // When: Route
    BitchatMessage result;
    bool success = handler.routeToBitChat(pubMsg, result);
    
    // Then: Should succeed
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL_STRING("<EmergencyBot> Emergency alert!", (char*)result.payload);
}

/**
 * Scenario: Unknown target channel
 * 
 * Given target channel not in registry
 * When routing
 * Then fail gracefully
 */
void test_unknown_target_channel(void) {
    // Given: Channel registry without target channel
    ChannelRegistry registry;
    
    // And: Handler pointing to unknown channel
    PublicMessageHandler handler(registry);
    handler.setTargetChannel("unknownchannel");
    
    // And: Public message
    MeshCorePublicMessage pubMsg;
    pubMsg.senderName = "Node";
    pubMsg.content = "Test";
    pubMsg.timestamp = 1704067200;
    
    // When: Route
    BitchatMessage result;
    bool success = handler.routeToBitChat(pubMsg, result);
    
    // Then: Should fail (unknown channel)
    TEST_ASSERT_FALSE(success);
}

/**
 * Scenario: Empty public message content
 * 
 * Given public message with empty content
 * When routing
 * Then reject
 */
void test_empty_public_message(void) {
    // Given: Handler
    ChannelRegistry registry;
    registry.registerChannel("mesh", PUBLIC_CHANNEL_KEY, ChannelType::HASHTAG);
    PublicMessageHandler handler(registry);
    
    // And: Empty public message
    MeshCorePublicMessage pubMsg;
    pubMsg.senderName = "Node";
    pubMsg.content = "";
    pubMsg.timestamp = 1704067200;
    
    // When: Route
    BitchatMessage result;
    bool success = handler.routeToBitChat(pubMsg, result);
    
    // Then: Should fail
    TEST_ASSERT_FALSE(success);
}

/**
 * Scenario: Long public message
 * 
 * Given long public message
 * When routing
 * Then truncate if needed
 */
void test_long_public_message(void) {
    // Given: Handler
    ChannelRegistry registry;
    registry.registerChannel("mesh", PUBLIC_CHANNEL_KEY, ChannelType::HASHTAG);
    PublicMessageHandler handler(registry);
    
    // And: Long message
    MeshCorePublicMessage pubMsg;
    pubMsg.senderName = "LongTalker";
    char longContent[BITCHAT_MAX_PAYLOAD_SIZE + 100];
    memset(longContent, 'A', sizeof(longContent));
    longContent[sizeof(longContent) - 1] = '\0';
    pubMsg.content = longContent;
    pubMsg.timestamp = 1704067200;
    
    // When: Route
    BitchatMessage result;
    bool success = handler.routeToBitChat(pubMsg, result);
    
    // Then: Should succeed but truncate
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_TRUE(result.payloadLength <= BITCHAT_MAX_PAYLOAD_SIZE);
}

/**
 * Scenario: Get target channel name
 * 
 * Given handler with configured channel
 * When getting target channel
 * Then return correct name
 */
void test_get_target_channel(void) {
    // Given: Handler
    ChannelRegistry registry;
    PublicMessageHandler handler(registry);
    
    // When: Set and get target
    handler.setTargetChannel("customchannel");
    const char* target = handler.getTargetChannel();
    
    // Then: Should match
    TEST_ASSERT_EQUAL_STRING("customchannel", target);
}

/**
 * Scenario: Multiple public messages
 * 
 * Given multiple public messages
 * When routing each
 * Then process all correctly
 */
void test_multiple_public_messages(void) {
    // Given: Handler
    ChannelRegistry registry;
    registry.registerChannel("mesh", PUBLIC_CHANNEL_KEY, ChannelType::HASHTAG);
    PublicMessageHandler handler(registry);
    
    // Multiple messages
    MeshCorePublicMessage msg1;
    msg1.senderName = "Node1";
    msg1.content = "Message one";
    msg1.timestamp = 1704067200;
    
    MeshCorePublicMessage msg2;
    msg2.senderName = "Node2";
    msg2.content = "Message two";
    msg2.timestamp = 1704067201;
    
    // Route first
    BitchatMessage result1;
    TEST_ASSERT_TRUE(handler.routeToBitChat(msg1, result1));
    TEST_ASSERT_EQUAL_STRING("<Node1> Message one", (char*)result1.payload);
    
    // Route second
    BitchatMessage result2;
    TEST_ASSERT_TRUE(handler.routeToBitChat(msg2, result2));
    TEST_ASSERT_EQUAL_STRING("<Node2> Message two", (char*)result2.payload);
}

} // namespace bitchat
} // namespace test

#endif // ENABLE_BITCHAT
