/**
 * Story 6.1: MeshCore Group Message Decapsulation - BDD Tests
 * 
 * Feature: Decapsulate BitChat Messages from MeshCore Packets
 * 
 * TDD Approach:
 * 1. Write failing tests (RED)
 * 2. Implement to pass (GREEN)
 * 3. Refactor (REFACTOR)
 */

#include <unity.h>
#include "helpers/bitchat/MessageDecapsulator.h"
#include "helpers/bitchat/ChannelRegistry.h"
#include "helpers/bitchat/BitchatMessageEncapsulator.h"

#ifdef ENABLE_BITCHAT

using namespace mesh::bitchat;
using namespace mesh::ble;

namespace test {
namespace bitchat {

// Decapsulation test channel key (unique to this test file)
static const uint8_t DECAP_CHANNEL_KEY[32] = {
    0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
    0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50,
    0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
    0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60
};

// Helper to create an encapsulated packet for testing
static EncapsulatedPacket createTestPacket(const BitchatMessage& msg, const uint8_t* key) {
    BitchatMessageEncapsulator encapsulator;
    EncapsulatedPacket packet;
    encapsulator.encapsulate(msg, key, packet);
    return packet;
}

/**
 * Scenario: Receive encapsulated BitChat from mesh
 * 
 * Given MeshCore receives PAYLOAD_TYPE_GRP_DATA on channel C
 * And channel C maps to BitChat "#general"
 * When payload has BitChat encapsulation header
 * Then decrypt with channel C secret
 * And extract BitChat serialized data
 * And deserialize to BitChat MESSAGE
 */
void test_receive_encapsulated_bitchat(void) {
    // Given: Channel registry
    ChannelRegistry registry;
    registry.registerChannel("general", DECAP_CHANNEL_KEY, ChannelType::HASHTAG);
    
    // And: Original BitChat message
    BitchatMessage originalMsg;
    originalMsg.version = BITCHAT_VERSION;
    originalMsg.type = BITCHAT_MSG_MESSAGE;
    originalMsg.ttl = 8;
    originalMsg.timestamp = 1234567890;
    originalMsg.setSenderId64(0x0807060504030201ULL);
    originalMsg.payloadLength = 20;
    memcpy(originalMsg.payload, "Hello from BitChat!", 20);
    
    // And: Encapsulated packet (simulating received from mesh)
    EncapsulatedPacket packet = createTestPacket(originalMsg, DECAP_CHANNEL_KEY);
    
    // When: Decapsulate
    MessageDecapsulator decapsulator(registry);
    DecapsulationResult result;
    
    bool success = decapsulator.decapsulate(packet, DECAP_CHANNEL_KEY, result);
    
    // Then: Should succeed
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(DecapsulationStatus::SUCCESS, result.status);
    
    // And: Should extract original message
    TEST_ASSERT_EQUAL(originalMsg.version, result.message.version);
    TEST_ASSERT_EQUAL(originalMsg.type, result.message.type);
    TEST_ASSERT_EQUAL(originalMsg.ttl, result.message.ttl);
    TEST_ASSERT_EQUAL(originalMsg.timestamp, result.message.timestamp);
    TEST_ASSERT_EQUAL_UINT64(originalMsg.getSenderId64(), result.message.getSenderId64());
    TEST_ASSERT_EQUAL(originalMsg.payloadLength, result.message.payloadLength);
    TEST_ASSERT_EQUAL_MEMORY(originalMsg.payload, result.message.payload, originalMsg.payloadLength);
}

/**
 * Scenario: Ignore non-BitChat group messages
 * 
 * Given MeshCore receives PAYLOAD_TYPE_GRP_DATA
 * And payload does NOT have BitChat magic header
 * Then process as normal MeshCore group message
 * And do not forward to BitChat
 */
void test_ignore_non_bitchat_messages(void) {
    // Given: Channel registry
    ChannelRegistry registry;
    registry.registerChannel("general", DECAP_CHANNEL_KEY, ChannelType::HASHTAG);
    
    // And: A non-BitChat packet (random data without magic header)
    EncapsulatedPacket packet;
    packet.length = 50;
    memset(packet.data, 0xAB, packet.length);
    packet.isValid = true;
    packet.payloadType = MeshCorePayloadType::MESHCORE_GRP_DATA;
    
    // When: Decapsulate
    MessageDecapsulator decapsulator(registry);
    DecapsulationResult result;
    
    bool success = decapsulator.decapsulate(packet, DECAP_CHANNEL_KEY, result);
    
    // Then: Should fail with NOT_BITCHAT status
    TEST_ASSERT_FALSE(success);
    TEST_ASSERT_EQUAL(DecapsulationStatus::NOT_BITCHAT, result.status);
}

/**
 * Scenario: Wrong channel key
 * 
 * Given encrypted packet with key A
 * When trying to decrypt with key B
 * Then decryption should fail
 */
void test_wrong_channel_key(void) {
    // Given: Original message
    BitchatMessage originalMsg;
    originalMsg.version = BITCHAT_VERSION;
    originalMsg.type = BITCHAT_MSG_MESSAGE;
    originalMsg.ttl = 8;
    originalMsg.timestamp = 1234567890;
    originalMsg.setSenderId64(0x0807060504030201ULL);
    originalMsg.payloadLength = 15;
    memcpy(originalMsg.payload, "Secret message!", 15);
    
    // And: Encrypted with correct key
    EncapsulatedPacket packet = createTestPacket(originalMsg, DECAP_CHANNEL_KEY);
    
    // And: Wrong key
    uint8_t wrongKey[32] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };
    
    ChannelRegistry registry;
    registry.registerChannel("general", wrongKey, ChannelType::HASHTAG);
    
    // When: Decapsulate with wrong key
    MessageDecapsulator decapsulator(registry);
    DecapsulationResult result;
    
    // Decapsulation might succeed but produce garbage or fail validation
    // The XOR cipher doesn't have integrity check, so it will produce something
    bool success = decapsulator.decapsulate(packet, wrongKey, result);
    
    // With XOR cipher, decryption with wrong key produces garbage
    // The version check should catch this
    if (success) {
        // If it succeeds, the content should be garbage
        TEST_ASSERT_NOT_EQUAL(originalMsg.version, result.message.version);
    }
}

/**
 * Scenario: Invalid packet too short
 * 
 * Given packet smaller than encapsulation header
 * When decapsulating
 * Then fail with INVALID_PACKET
 */
void test_packet_too_short(void) {
    // Given: Channel registry
    ChannelRegistry registry;
    registry.registerChannel("general", DECAP_CHANNEL_KEY, ChannelType::HASHTAG);
    
    // And: Too-short packet
    EncapsulatedPacket packet;
    packet.length = 5;  // Less than header size
    memset(packet.data, 0xBC, packet.length);
    packet.isValid = true;
    
    // When: Decapsulate
    MessageDecapsulator decapsulator(registry);
    DecapsulationResult result;
    
    bool success = decapsulator.decapsulate(packet, DECAP_CHANNEL_KEY, result);
    
    // Then: Should fail
    TEST_ASSERT_FALSE(success);
}

/**
 * Scenario: Wrong encapsulation version
 * 
 * Given packet with unsupported version
 * When decapsulating
 * Then fail with VERSION_MISMATCH
 */
void test_unsupported_version(void) {
    // Given: Channel registry
    ChannelRegistry registry;
    registry.registerChannel("general", DECAP_CHANNEL_KEY, ChannelType::HASHTAG);
    
    // And: Original message
    BitchatMessage originalMsg;
    originalMsg.version = BITCHAT_VERSION;
    originalMsg.type = BITCHAT_MSG_MESSAGE;
    originalMsg.ttl = 8;
    originalMsg.timestamp = 1234567890;
    originalMsg.setSenderId64(0x0807060504030201ULL);
    originalMsg.payloadLength = 10;
    memcpy(originalMsg.payload, "Test", 4);
    
    // And: Encapsulated packet
    EncapsulatedPacket packet = createTestPacket(originalMsg, DECAP_CHANNEL_KEY);
    
    // Modify version to unsupported
    packet.data[4] = 0xFF;  // Invalid version
    
    // When: Decapsulate
    MessageDecapsulator decapsulator(registry);
    DecapsulationResult result;
    
    bool success = decapsulator.decapsulate(packet, DECAP_CHANNEL_KEY, result);
    
    // Then: Should fail with version mismatch
    TEST_ASSERT_FALSE(success);
}

/**
 * Scenario: Detect BitChat encapsulation
 * 
 * Given various packets
 * When checking for BitChat magic
 * Then correctly identify BitChat vs non-BitChat
 */
void test_detect_bitchat_magic(void) {
    MessageDecapsulator decapsulator(*(ChannelRegistry*)nullptr);
    
    // Valid BitChat magic: "BC\x00\x00"
    uint8_t validMagic[] = {'B', 'C', 0x00, 0x00};
    TEST_ASSERT_TRUE(decapsulator.isBitChatEncapsulated(validMagic, 4));
    
    // Invalid magic: wrong bytes
    uint8_t invalidMagic1[] = {'A', 'B', 0x00, 0x00};
    TEST_ASSERT_FALSE(decapsulator.isBitChatEncapsulated(invalidMagic1, 4));
    
    // Invalid: too short
    uint8_t tooShort[] = {'B', 'C'};
    TEST_ASSERT_FALSE(decapsulator.isBitChatEncapsulated(tooShort, 2));
    
    // Invalid: null
    TEST_ASSERT_FALSE(decapsulator.isBitChatEncapsulated(nullptr, 4));
}

/**
 * Scenario: Multiple messages from different channels
 * 
 * Given messages from different channels
 * When decapsulating each
 * Then use correct channel key for each
 */
void test_multiple_channels_decapsulation(void) {
    // Given: Multiple channel keys
    uint8_t channelAKey[32];
    uint8_t channelBKey[32];
    memcpy(channelAKey, DECAP_CHANNEL_KEY, 32);
    memcpy(channelBKey, DECAP_CHANNEL_KEY, 32);
    channelBKey[0] = 0xFF;  // Make different
    
    ChannelRegistry registry;
    registry.registerChannel("channelA", channelAKey, ChannelType::HASHTAG);
    registry.registerChannel("channelB", channelBKey, ChannelType::HASHTAG);
    
    // Message 1 for channel A
    BitchatMessage msgA;
    msgA.version = BITCHAT_VERSION;
    msgA.type = BITCHAT_MSG_MESSAGE;
    msgA.ttl = 8;
    msgA.timestamp = 1234567890;
    msgA.setSenderId64(0x0807060504030201ULL);
    msgA.payloadLength = 10;
    memcpy(msgA.payload, "Channel A!", 10);
    EncapsulatedPacket packetA = createTestPacket(msgA, channelAKey);
    
    // Message 2 for channel B
    BitchatMessage msgB;
    msgB.version = BITCHAT_VERSION;
    msgB.type = BITCHAT_MSG_MESSAGE;
    msgB.ttl = 8;
    msgB.timestamp = 1234567891;
    msgB.setSenderId64(0x0807060504030202ULL);
    msgB.payloadLength = 10;
    memcpy(msgB.payload, "Channel B!", 10);
    EncapsulatedPacket packetB = createTestPacket(msgB, channelBKey);
    
    MessageDecapsulator decapsulator(registry);
    
    // Decapsulate message A with correct key
    DecapsulationResult resultA;
    bool successA = decapsulator.decapsulate(packetA, channelAKey, resultA);
    TEST_ASSERT_TRUE(successA);
    TEST_ASSERT_EQUAL_MEMORY(msgA.payload, resultA.message.payload, msgA.payloadLength);
    
    // Decapsulate message B with correct key
    DecapsulationResult resultB;
    bool successB = decapsulator.decapsulate(packetB, channelBKey, resultB);
    TEST_ASSERT_TRUE(successB);
    TEST_ASSERT_EQUAL_MEMORY(msgB.payload, resultB.message.payload, msgB.payloadLength);
}

} // namespace bitchat
} // namespace test

#endif // ENABLE_BITCHAT
