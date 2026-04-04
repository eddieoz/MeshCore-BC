/**
 * Story 6.3: BitChat Message Formatting - BDD Tests
 * 
 * Feature: Format MeshCore Messages for BitChat
 * 
 * TDD Approach:
 * 1. Write failing tests (RED)
 * 2. Implement to pass (GREEN)
 * 3. Refactor (REFACTOR)
 */

#include <unity.h>
#include "helpers/bitchat/MessageFormatter.h"
#include "helpers/bitchat/BitchatIdentity.h"

#ifdef ENABLE_BITCHAT

using namespace mesh::bitchat;
using namespace mesh::ble;

namespace test {
namespace bitchat {

// Test sender key
static const uint8_t FMT_SENDER_KEY[32] = {
    0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8,
    0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 0xE0,
    0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8,
    0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF, 0xF0
};

/**
 * Scenario: Format group message for BitChat
 * 
 * Given MeshCore message from "Bob" to #mesh
 * And content is "What's the weather?"
 * When formatting for BitChat
 * Then create BitChat MESSAGE type
 * And payload format: "<Bob> What's the weather?"
 */
void test_format_group_message(void) {
    // Given: MeshCore message info
    MeshCoreMessageInfo info;
    info.type = MeshCoreMessageType::GROUP_MESSAGE;
    strncpy(info.senderName, "Bob", sizeof(info.senderName) - 1);
    strncpy(info.channelName, "mesh", sizeof(info.channelName) - 1);
    info.content = "What's the weather?";
    info.contentLength = 19;
    info.timestamp = 1234567890;
    
    uint64_t senderId = deriveBitChatPeerId(FMT_SENDER_KEY);
    info.senderId = senderId;
    
    // When: Format for BitChat
    MessageFormatter formatter;
    BitchatMessage result;
    
    bool success = formatter.formatForBitChat(info, result);
    
    // Then: Should succeed
    TEST_ASSERT_TRUE(success);
    
    // And: Should be MESSAGE type
    TEST_ASSERT_EQUAL(BITCHAT_MSG_MESSAGE, result.type);
    
    // And: Should have sender ID
    TEST_ASSERT_EQUAL_UINT64(senderId, result.getSenderId64());
    
    // And: Payload should be "<Bob> What's the weather?"
    const char* expectedPayload = "<Bob> What's the weather?";
    size_t expectedLen = strlen(expectedPayload);
    
    TEST_ASSERT_EQUAL(expectedLen, result.payloadLength);
    TEST_ASSERT_EQUAL_STRING(expectedPayload, (const char*)result.payload);
}

/**
 * Scenario: Format with special characters in name
 * 
 * Given sender name with special characters
 * When formatting
 * Then properly escape/include in format
 */
void test_format_with_special_chars(void) {
    // Given: Message with special chars in sender name
    MeshCoreMessageInfo info;
    info.type = MeshCoreMessageType::GROUP_MESSAGE;
    strncpy(info.senderName, "Alice [Admin]", sizeof(info.senderName) - 1);
    strncpy(info.channelName, "general", sizeof(info.channelName) - 1);
    info.content = "Hello everyone!";
    info.contentLength = 15;
    info.timestamp = 1234567890;
    info.senderId = deriveBitChatPeerId(FMT_SENDER_KEY);
    
    // When: Format
    MessageFormatter formatter;
    BitchatMessage result;
    
    bool success = formatter.formatForBitChat(info, result);
    
    // Then: Should format correctly
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL_STRING("<Alice [Admin]> Hello everyone!", (const char*)result.payload);
}

/**
 * Scenario: Format long message
 * 
 * Given long message content
 * When formatting
 * Then truncate if needed
 */
void test_format_long_message(void) {
    // Given: Long message
    MeshCoreMessageInfo info;
    info.type = MeshCoreMessageType::GROUP_MESSAGE;
    strncpy(info.senderName, "LongNameSender", sizeof(info.senderName) - 1);
    strncpy(info.channelName, "mesh", sizeof(info.channelName) - 1);
    
    // Create content that would exceed max payload
    char longContent[BITCHAT_MAX_PAYLOAD_SIZE + 100];
    memset(longContent, 'X', sizeof(longContent));
    longContent[sizeof(longContent) - 1] = '\0';
    
    info.content = longContent;
    info.contentLength = strlen(longContent);
    info.timestamp = 1234567890;
    info.senderId = deriveBitChatPeerId(FMT_SENDER_KEY);
    
    // When: Format
    MessageFormatter formatter;
    BitchatMessage result;
    
    bool success = formatter.formatForBitChat(info, result);
    
    // Then: Should succeed but truncate
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_TRUE(result.payloadLength <= BITCHAT_MAX_PAYLOAD_SIZE);
}

/**
 * Scenario: Format DM for BitChat
 * 
 * Given MeshCore DM
 * When formatting for BitChat
 * Then create proper DM format
 */
void test_format_dm_message(void) {
    // Given: DM message info
    MeshCoreMessageInfo info;
    info.type = MeshCoreMessageType::DIRECT_MESSAGE;
    strncpy(info.senderName, "Alice", sizeof(info.senderName) - 1);
    strncpy(info.recipientName, "Bob", sizeof(info.recipientName) - 1);
    info.content = "Private message here";
    info.contentLength = 20;
    info.timestamp = 1234567890;
    info.senderId = deriveBitChatPeerId(FMT_SENDER_KEY);
    
    // When: Format
    MessageFormatter formatter;
    BitchatMessage result;
    
    bool success = formatter.formatForBitChat(info, result);
    
    // Then: Should succeed
    TEST_ASSERT_TRUE(success);
    
    // And: Should have recipient flag
    TEST_ASSERT_TRUE(result.hasRecipient());
}

/**
 * Scenario: Message timestamp
 * 
 * Given message with timestamp
 * When formatting
 * Then include in BitChat message
 */
void test_format_preserves_timestamp(void) {
    // Given: Message with specific timestamp
    MeshCoreMessageInfo info;
    info.type = MeshCoreMessageType::GROUP_MESSAGE;
    strncpy(info.senderName, "TestUser", sizeof(info.senderName) - 1);
    strncpy(info.channelName, "mesh", sizeof(info.channelName) - 1);
    info.content = "Test";
    info.contentLength = 4;
    info.timestamp = 1640995200;  // 2022-01-01 00:00:00 UTC
    info.senderId = deriveBitChatPeerId(FMT_SENDER_KEY);
    
    // When: Format
    MessageFormatter formatter;
    BitchatMessage result;
    
    bool success = formatter.formatForBitChat(info, result);
    
    // Then: Should preserve timestamp
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(1640995200ULL, result.timestamp);
}

/**
 * Scenario: Empty content handling
 * 
 * Given message with empty content
 * When formatting
 * Then fail gracefully
 */
void test_format_empty_content(void) {
    // Given: Empty message
    MeshCoreMessageInfo info;
    info.type = MeshCoreMessageType::GROUP_MESSAGE;
    strncpy(info.senderName, "User", sizeof(info.senderName) - 1);
    strncpy(info.channelName, "mesh", sizeof(info.channelName) - 1);
    info.content = "";
    info.contentLength = 0;
    info.timestamp = 1234567890;
    info.senderId = deriveBitChatPeerId(FMT_SENDER_KEY);
    
    // When: Format
    MessageFormatter formatter;
    BitchatMessage result;
    
    bool success = formatter.formatForBitChat(info, result);
    
    // Then: Should fail (no content)
    TEST_ASSERT_FALSE(success);
}

/**
 * Scenario: Null content handling
 * 
 * Given message with null content
 * When formatting
 * Then fail gracefully
 */
void test_format_null_content(void) {
    // Given: Null content
    MeshCoreMessageInfo info;
    info.type = MeshCoreMessageType::GROUP_MESSAGE;
    strncpy(info.senderName, "User", sizeof(info.senderName) - 1);
    strncpy(info.channelName, "mesh", sizeof(info.channelName) - 1);
    info.content = nullptr;
    info.contentLength = 0;
    info.timestamp = 1234567890;
    info.senderId = deriveBitChatPeerId(FMT_SENDER_KEY);
    
    // When: Format
    MessageFormatter formatter;
    BitchatMessage result;
    
    bool success = formatter.formatForBitChat(info, result);
    
    // Then: Should fail
    TEST_ASSERT_FALSE(success);
}

/**
 * Scenario: Message with angle brackets in content
 * 
 * Given message content containing angle brackets
 * When formatting
 * Then handle properly without breaking format
 */
void test_format_with_angle_brackets(void) {
    // Given: Message with angle brackets in content
    MeshCoreMessageInfo info;
    info.type = MeshCoreMessageType::GROUP_MESSAGE;
    strncpy(info.senderName, "Coder", sizeof(info.senderName) - 1);
    strncpy(info.channelName, "dev", sizeof(info.channelName) - 1);
    info.content = "Use <stdio.h> for printf";
    info.contentLength = 24;
    info.timestamp = 1234567890;
    info.senderId = deriveBitChatPeerId(FMT_SENDER_KEY);
    
    // When: Format
    MessageFormatter formatter;
    BitchatMessage result;
    
    bool success = formatter.formatForBitChat(info, result);
    
    // Then: Should format with angle brackets preserved
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL_STRING("<Coder> Use <stdio.h> for printf", (const char*)result.payload);
}

} // namespace bitchat
} // namespace test

#endif // ENABLE_BITCHAT
