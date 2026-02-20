/**
 * Story 5.1: BitChat Message Reception - BDD Tests
 * 
 * Feature: Receive and Parse BitChat Messages
 */

#include <unity.h>
#include "helpers/bitchat/BitchatMessageParser.h"

#ifdef ENABLE_BITCHAT

using namespace mesh::bitchat;

namespace test {
namespace bitchat {

// Test sender ID
static const uint64_t TEST_SENDER_ID = 0x0807060504030201ULL;

// Build a test MESSAGE packet
// Returns length of packet
size_t buildTestMessage(uint8_t* buffer, size_t maxLen, 
                        const char* channel, const char* text) {
    size_t offset = 0;
    
    // Header
    buffer[offset++] = BITCHAT_VERSION;
    buffer[offset++] = BITCHAT_MSG_MESSAGE;
    buffer[offset++] = 8;  // ttl
    
    // Timestamp (8 bytes, big-endian)
    for (int i = 0; i < 8; i++) buffer[offset++] = 0;
    
    // Flags
    buffer[offset++] = 0;
    
    // Calculate payload length
    size_t payloadLen = 2 + strlen(channel) + 2 + strlen(text);
    
    // Payload length (2 bytes, big-endian)
    buffer[offset++] = (payloadLen >> 8) & 0xFF;
    buffer[offset++] = payloadLen & 0xFF;
    
    // Sender ID (8 bytes, little-endian)
    for (int i = 0; i < 8; i++) {
        buffer[offset++] = (TEST_SENDER_ID >> (i * 8)) & 0xFF;
    }
    
    // TLV: Channel name
    buffer[offset++] = BITCHAT_TLV_CHANNEL_NAME;
    buffer[offset++] = strlen(channel);
    memcpy(&buffer[offset], channel, strlen(channel));
    offset += strlen(channel);
    
    // TLV: Message text
    buffer[offset++] = BITCHAT_TLV_MESSAGE_TEXT;
    buffer[offset++] = strlen(text);
    memcpy(&buffer[offset], text, strlen(text));
    offset += strlen(text);
    
    return offset;
}

/**
 * Scenario: Receive MESSAGE type from BitChat
 * 
 * Given BitChat BLE client is connected
 * When MESSAGE packet arrives
 * Then parse TLV payload correctly
 */
void test_receive_message_packet(void) {
    uint8_t packet[256];
    size_t len = buildTestMessage(packet, sizeof(packet), "#mesh", "Hello everyone");
    
    BitchatMessageParser parser;
    ParsedBitchatMessage msg;
    
    // When: Parse message
    bool parsed = parser.parseMessage(packet, len, msg);
    
    // Then: Parsed successfully
    TEST_ASSERT_TRUE(parsed);
    TEST_ASSERT_EQUAL(BITCHAT_VERSION, msg.version);
    TEST_ASSERT_EQUAL(BITCHAT_MSG_MESSAGE, msg.type);
    TEST_ASSERT_EQUAL_HEX64(TEST_SENDER_ID, msg.senderId);
    
    // And: Fields extracted
    TEST_ASSERT_EQUAL_STRING("#mesh", msg.channel);
    TEST_ASSERT_EQUAL_STRING("Hello everyone", msg.text);
}

/**
 * Scenario: Parse MESSAGE with all TLV fields
 * 
 * Given MESSAGE with multiple TLVs
 * When parsed
 * Then all fields extracted
 */
void test_parse_message_with_all_tlvs(void) {
    uint8_t packet[256];
    size_t offset = 0;
    
    // Header
    packet[offset++] = BITCHAT_VERSION;
    packet[offset++] = BITCHAT_MSG_MESSAGE;
    packet[offset++] = 8;  // ttl
    
    // Timestamp (8 bytes, big-endian) - 0x1234567890ABCDEF
    packet[offset++] = 0x12;
    packet[offset++] = 0x34;
    packet[offset++] = 0x56;
    packet[offset++] = 0x78;
    packet[offset++] = 0x90;
    packet[offset++] = 0xAB;
    packet[offset++] = 0xCD;
    packet[offset++] = 0xEF;
    
    // Flags
    packet[offset++] = 0;
    
    // Calculate payload
    const char* channel = "#general";
    const char* text = "Test message";
    uint64_t replyId = 0xDEADBEEFCAFEBABEULL;
    
    size_t payloadLen = 2 + strlen(channel) + 2 + strlen(text) + 2 + 8;
    
    // Payload length
    packet[offset++] = (payloadLen >> 8) & 0xFF;
    packet[offset++] = payloadLen & 0xFF;
    
    // Sender ID
    for (int i = 0; i < 8; i++) {
        packet[offset++] = (TEST_SENDER_ID >> (i * 8)) & 0xFF;
    }
    
    // TLV: Channel
    packet[offset++] = BITCHAT_TLV_CHANNEL_NAME;
    packet[offset++] = strlen(channel);
    memcpy(&packet[offset], channel, strlen(channel));
    offset += strlen(channel);
    
    // TLV: Text
    packet[offset++] = BITCHAT_TLV_MESSAGE_TEXT;
    packet[offset++] = strlen(text);
    memcpy(&packet[offset], text, strlen(text));
    offset += strlen(text);
    
    // TLV: Reply-to
    packet[offset++] = BITCHAT_TLV_REPLY_TO;
    packet[offset++] = 8;
    for (int i = 0; i < 8; i++) {
        packet[offset++] = (replyId >> (i * 8)) & 0xFF;
    }
    
    BitchatMessageParser parser;
    ParsedBitchatMessage msg;
    
    // When: Parse
    bool parsed = parser.parseMessage(packet, offset, msg);
    
    // Then: Parsed
    TEST_ASSERT_TRUE(parsed);
    TEST_ASSERT_EQUAL_STRING("#general", msg.channel);
    TEST_ASSERT_EQUAL_STRING("Test message", msg.text);
    TEST_ASSERT_TRUE(msg.hasReplyTo);
    TEST_ASSERT_EQUAL_HEX64(replyId, msg.replyToMsgId);
}

/**
 * Scenario: Parse unsupported version
 * 
 * Given packet with wrong version
 * When parsed
 * Then returns error
 */
void test_parse_unsupported_version(void) {
    uint8_t packet[64] = {
        0x99,  // Invalid version
        BITCHAT_MSG_MESSAGE,
        8,     // ttl
        0, 0, 0, 0, 0, 0, 0, 0,  // timestamp
        0,     // flags
        0, 0,  // payload len
        0, 0, 0, 0, 0, 0, 0, 0   // sender id
    };
    
    BitchatMessageParser parser;
    ParsedBitchatMessage msg;
    
    bool parsed = parser.parseMessage(packet, sizeof(packet), msg);
    
    TEST_ASSERT_FALSE(parsed);
}

/**
 * Scenario: Parse too short message
 * 
 * Given truncated packet
 * When parsed
 * Then returns error
 */
void test_parse_truncated_message(void) {
    uint8_t packet[5] = {BITCHAT_VERSION, BITCHAT_MSG_MESSAGE, 8, 0, 0};
    
    BitchatMessageParser parser;
    ParsedBitchatMessage msg;
    
    bool parsed = parser.parseMessage(packet, sizeof(packet), msg);
    
    TEST_ASSERT_FALSE(parsed);
}

/**
 * Scenario: Check supported message types
 * 
 * Given various message types
 * When checking support
 * Then correct result
 */
void test_supported_message_types(void) {
    TEST_ASSERT_TRUE(BitchatMessageParser::isSupportedType(BITCHAT_MSG_MESSAGE));
    TEST_ASSERT_TRUE(BitchatMessageParser::isSupportedType(BITCHAT_MSG_ANNOUNCE));
    TEST_ASSERT_TRUE(BitchatMessageParser::isSupportedType(BITCHAT_MSG_PING));
    TEST_ASSERT_TRUE(BitchatMessageParser::isSupportedType(BITCHAT_MSG_PONG));
    TEST_ASSERT_TRUE(BitchatMessageParser::isSupportedType(BITCHAT_MSG_REQUEST_SYNC));
    
    TEST_ASSERT_FALSE(BitchatMessageParser::isSupportedType(0xFF));
    TEST_ASSERT_FALSE(BitchatMessageParser::isSupportedType(0x00));
}

/**
 * Scenario: Parse empty payload
 * 
 * Given message with no TLVs
 * When parsed
 * Then header fields still extracted
 */
void test_parse_empty_payload(void) {
    uint8_t packet[64];
    size_t offset = 0;
    
    packet[offset++] = BITCHAT_VERSION;
    packet[offset++] = BITCHAT_MSG_MESSAGE;
    packet[offset++] = 8;
    for (int i = 0; i < 8; i++) packet[offset++] = 0;  // timestamp
    packet[offset++] = 0;  // flags
    packet[offset++] = 0;  // payload len high
    packet[offset++] = 0;  // payload len low
    for (int i = 0; i < 8; i++) packet[offset++] = (TEST_SENDER_ID >> (i * 8)) & 0xFF;
    
    BitchatMessageParser parser;
    ParsedBitchatMessage msg;
    
    bool parsed = parser.parseMessage(packet, offset, msg);
    
    TEST_ASSERT_TRUE(parsed);
    TEST_ASSERT_EQUAL(BITCHAT_MSG_MESSAGE, msg.type);
    TEST_ASSERT_EQUAL_HEX64(TEST_SENDER_ID, msg.senderId);
    TEST_ASSERT_EQUAL_STRING("", msg.channel);  // No channel TLV
    TEST_ASSERT_EQUAL_STRING("", msg.text);     // No text TLV
}

/**
 * Scenario: Parse truncated TLV
 * 
 * Given TLV with length exceeding data
 * When parsed
 * Then returns error
 */
void test_parse_truncated_tlv(void) {
    uint8_t packet[64];
    size_t offset = 0;
    
    // Header
    packet[offset++] = BITCHAT_VERSION;
    packet[offset++] = BITCHAT_MSG_MESSAGE;
    packet[offset++] = 8;
    for (int i = 0; i < 8; i++) packet[offset++] = 0;
    packet[offset++] = 0;
    
    // Payload length says 10 but we only provide 3
    packet[offset++] = 0;
    packet[offset++] = 10;
    
    // Sender ID
    for (int i = 0; i < 8; i++) packet[offset++] = 0;
    
    // Incomplete TLV (only 3 bytes, but length says more)
    packet[offset++] = BITCHAT_TLV_MESSAGE_TEXT;
    packet[offset++] = 100;  // Claims 100 bytes
    packet[offset++] = 0x41; // 'A'
    
    BitchatMessageParser parser;
    ParsedBitchatMessage msg;
    
    bool parsed = parser.parseMessage(packet, offset, msg);
    
    // Should fail due to truncated TLV
    TEST_ASSERT_FALSE(parsed);
}

} // namespace bitchat
} // namespace test

#else // !ENABLE_BITCHAT

namespace test {
namespace bitchat {
void test_message_reception_placeholder(void) {
    TEST_IGNORE_MESSAGE("BitChat disabled - ENABLE_BITCHAT not defined");
}
} // namespace bitchat
} // namespace test

#endif // ENABLE_BITCHAT
