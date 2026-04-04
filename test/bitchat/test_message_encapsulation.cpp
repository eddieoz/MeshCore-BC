/**
 * Story 5.2: BitChat Message Encapsulation - BDD Tests
 * 
 * Feature: Encapsulate BitChat Messages in MeshCore Packets
 * 
 * TDD Approach:
 * 1. Write failing tests (RED)
 * 2. Implement to pass (GREEN)
 * 3. Refactor (REFACTOR)
 */

#include <unity.h>
#include "helpers/bitchat/BitchatMessageEncapsulator.h"

#ifdef ENABLE_BITCHAT

using namespace mesh::bitchat;

namespace test {
namespace bitchat {

// Test channel key
static const uint8_t TEST_CHANNEL_KEY[32] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20
};

/**
 * Scenario: Encapsulate BitChat message for mesh transmission
 * 
 * Given BitChat message received for channel "#mesh"
 * When message is encapsulated
 * Then create MeshCore packet with encapsulation header
 */
void test_encapsulate_message_basic(void) {
    // Given: BitChat message
    BitchatMessage msg;
    msg.version = BITCHAT_VERSION;
    msg.type = BITCHAT_MSG_MESSAGE;
    msg.ttl = 8;
    msg.timestamp = 1234567890;
    msg.setSenderId64(0x0807060504030201ULL);
    msg.payloadLength = 12;
    memcpy(msg.payload, "Hello World!", 12);
    
    BitchatMessageEncapsulator encapsulator;
    
    // When: Encapsulate
    EncapsulatedPacket packet;
    bool result = encapsulator.encapsulate(msg, TEST_CHANNEL_KEY, packet);
    
    // Then: Encapsulation successful
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(packet.isValid);
    TEST_ASSERT_EQUAL(MeshCorePayloadType::MESHCORE_GRP_DATA, packet.payloadType);
    
    // And: Encapsulation header present
    TEST_ASSERT_EQUAL_HEX8('B', packet.data[0]);
    TEST_ASSERT_EQUAL_HEX8('C', packet.data[1]);
    TEST_ASSERT_EQUAL_HEX8(0x00, packet.data[2]);
    TEST_ASSERT_EQUAL_HEX8(0x00, packet.data[3]);
    TEST_ASSERT_EQUAL_HEX8(0x01, packet.data[4]);  // version
}

/**
 * Scenario: Encapsulation header format
 * 
 * Given BitChat message
 * When encapsulated
 * Then header is: magic + version + flags + orig_len
 */
void test_encapsulation_header_format(void) {
    BitchatMessage msg;
    msg.version = BITCHAT_VERSION;
    msg.type = BITCHAT_MSG_MESSAGE;
    msg.payloadLength = 10;
    memcpy(msg.payload, "Test data!", 10);
    
    BitchatMessageEncapsulator encapsulator;
    EncapsulatedPacket packet;
    
    TEST_ASSERT_TRUE(encapsulator.encapsulate(msg, TEST_CHANNEL_KEY, packet));
    
    // Verify header (14 bytes)
    // Magic: "BC\x00\x00"
    TEST_ASSERT_EQUAL_HEX8('B', packet.data[0]);
    TEST_ASSERT_EQUAL_HEX8('C', packet.data[1]);
    TEST_ASSERT_EQUAL_HEX8(0x00, packet.data[2]);
    TEST_ASSERT_EQUAL_HEX8(0x00, packet.data[3]);
    // Version
    TEST_ASSERT_EQUAL_HEX8(0x01, packet.data[4]);
    // Flags
    TEST_ASSERT_EQUAL_HEX8(0x00, packet.data[5]);
    // Original length (2 bytes, big-endian) - full serialized message size
    // BITCHAT_HEADER_SIZE (14) + payload (10) + sender_id (8) = ~32
    TEST_ASSERT_EQUAL_HEX8(0x00, packet.data[6]);
    TEST_ASSERT_TRUE(packet.data[7] > 20);  // Should be > header size
}

/**
 * Scenario: Encrypted payload
 * 
 * Given BitChat message and channel key
 * When encapsulated
 * Then payload is encrypted
 */
void test_encapsulation_encrypts_payload(void) {
    BitchatMessage msg;
    msg.version = BITCHAT_VERSION;
    msg.type = BITCHAT_MSG_MESSAGE;
    msg.payloadLength = 12;
    memcpy(msg.payload, "Secret text!", 12);
    
    BitchatMessageEncapsulator encapsulator;
    EncapsulatedPacket packet;
    
    TEST_ASSERT_TRUE(encapsulator.encapsulate(msg, TEST_CHANNEL_KEY, packet));
    
    // Payload should be encrypted (not plaintext)
    // Check that data after header is different from original
    const uint8_t* encryptedPayload = &packet.data[BITCHAT_ENCAPSULATION_HEADER_SIZE];
    
    // Should NOT match original plaintext
    TEST_ASSERT_TRUE(memcmp(encryptedPayload, msg.payload, msg.payloadLength) != 0);
}

/**
 * Scenario: Handle large BitChat messages (fragmentation)
 * 
 * Given BitChat message size > 170 bytes
 * When encapsulating
 * Then fragment into multiple packets
 */
void test_encapsulate_large_message_fragments(void) {
    // Given: Large message (300 bytes)
    BitchatMessage msg;
    msg.version = BITCHAT_VERSION;
    msg.type = BITCHAT_MSG_MESSAGE;
    msg.payloadLength = 300;
    memset(msg.payload, 'A', 300);
    
    BitchatMessageEncapsulator encapsulator;
    
    // When: Encapsulate
    EncapsulatedPacket packets[4];
    size_t packetCount = 0;
    bool result = encapsulator.encapsulateWithFragmentation(
        msg, TEST_CHANNEL_KEY, packets, 4, packetCount
    );
    
    // Then: Multiple fragments created
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(packetCount > 1);
    
    // Each fragment should be valid
    for (size_t i = 0; i < packetCount; i++) {
        TEST_ASSERT_TRUE(packets[i].isValid);
        TEST_ASSERT_EQUAL(MeshCorePayloadType::MESHCORE_GRP_DATA, packets[i].payloadType);
    }
}

/**
 * Scenario: Fragment header contains fragment info
 * 
 * Given fragmented message
 * When examining fragments
 * Then each has msg_id + frag_num + total_frags
 */
void test_fragment_header_format(void) {
    // Create large message to force fragmentation
    BitchatMessage msg;
    msg.version = BITCHAT_VERSION;
    msg.type = BITCHAT_MSG_MESSAGE;
    msg.payloadLength = 250;
    memset(msg.payload, 'X', 250);
    
    BitchatMessageEncapsulator encapsulator;
    EncapsulatedPacket packets[4];
    size_t packetCount = 0;
    
    TEST_ASSERT_TRUE(encapsulator.encapsulateWithFragmentation(
        msg, TEST_CHANNEL_KEY, packets, 4, packetCount
    ));
    
    // Should have fragments
    TEST_ASSERT_TRUE(packetCount >= 2);
    
    // First fragment header format (14 bytes):
    // magic(4) + version(1) + flags(1) + orig_len(2) + msg_id(4) + frag_num(1) + total_frags(1)
    // frag_num is at offset 12, total_frags at offset 13
    
    // frag_num should be 0 for first fragment
    TEST_ASSERT_EQUAL_HEX8(0, packets[0].data[12]);
    // total_frags should match
    TEST_ASSERT_EQUAL_HEX8(packetCount, packets[0].data[13]);
}

/**
 * Scenario: Small messages don't fragment
 * 
 * Given BitChat message < 170 bytes
 * When encapsulating
 * Then single packet created
 */
void test_small_message_no_fragmentation(void) {
    BitchatMessage msg;
    msg.version = BITCHAT_VERSION;
    msg.type = BITCHAT_MSG_MESSAGE;
    msg.payloadLength = 50;
    memset(msg.payload, 'Y', 50);
    
    BitchatMessageEncapsulator encapsulator;
    EncapsulatedPacket packet;
    
    TEST_ASSERT_TRUE(encapsulator.encapsulate(msg, TEST_CHANNEL_KEY, packet));
    
    // Should be single packet, no fragmentation
    TEST_ASSERT_FALSE(packet.isFragment);
    TEST_ASSERT_EQUAL(0, packet.fragmentNum);
    TEST_ASSERT_EQUAL(1, packet.totalFragments);
}

/**
 * Scenario: Null parameters handled
 * 
 * Given null input
 * When encapsulating
 * Then returns error
 */
void test_encapsulate_null_parameters(void) {
    BitchatMessageEncapsulator encapsulator;
    EncapsulatedPacket packet;
    BitchatMessage msg;
    
    // Null message
    TEST_ASSERT_FALSE(encapsulator.encapsulate(msg, nullptr, packet));
}

/**
 * Scenario: Decapsulate returns original message
 * 
 * Given encapsulated packet
 * When decapsulated
 * Then original message recovered
 */
void test_decapsulate_recover_original(void) {
    // Given: Original message
    BitchatMessage originalMsg;
    originalMsg.version = BITCHAT_VERSION;
    originalMsg.type = BITCHAT_MSG_MESSAGE;
    originalMsg.payloadLength = 14;
    memcpy(originalMsg.payload, "Test message!", 14);
    
    // Encapsulate
    BitchatMessageEncapsulator encapsulator;
    EncapsulatedPacket packet;
    TEST_ASSERT_TRUE(encapsulator.encapsulate(originalMsg, TEST_CHANNEL_KEY, packet));
    
    // When: Decapsulate
    BitchatMessage recoveredMsg;
    bool result = encapsulator.decapsulate(packet, TEST_CHANNEL_KEY, recoveredMsg);
    
    // Then: Original recovered
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(originalMsg.version, recoveredMsg.version);
    TEST_ASSERT_EQUAL(originalMsg.type, recoveredMsg.type);
    TEST_ASSERT_EQUAL(originalMsg.payloadLength, recoveredMsg.payloadLength);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(originalMsg.payload, recoveredMsg.payload, originalMsg.payloadLength);
}

} // namespace bitchat
} // namespace test

#else // !ENABLE_BITCHAT

namespace test {
namespace bitchat {
void test_encapsulation_placeholder(void) {
    TEST_IGNORE_MESSAGE("BitChat disabled - ENABLE_BITCHAT not defined");
}
} // namespace bitchat
} // namespace test

#endif // ENABLE_BITCHAT
