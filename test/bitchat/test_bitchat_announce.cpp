/**
 * Story 3.3: MeshCore Identity to BitChat Mapping - BDD Tests
 * 
 * Feature: Advertise MeshCore Identity to BitChat
 */

#include <unity.h>
#include "helpers/bitchat/BitchatAnnounce.h"
#include "helpers/bitchat/BitchatIdentity.h"

#ifdef ENABLE_BITCHAT

using namespace mesh::bitchat;

namespace test {
namespace bitchat {

// Use constants from test_bitchat_identity.cpp
extern const uint8_t TEST_PUBKEY[32];
extern const uint64_t EXPECTED_PEER_ID;

/**
 * Scenario: Build ANNOUNCE payload from MeshCore identity
 * 
 * Given MeshCore identity (name, pubkey)
 * When building ANNOUNCE payload
 * Then TLV format is correct
 */
void test_build_announce_payload(void) {
    BitchatAnnounceBuilder builder;
    builder.setNodeName("MeshNode");
    builder.setEd25519PubKey(TEST_PUBKEY);
    
    uint8_t buffer[256];
    size_t len = builder.buildPayload(buffer, sizeof(buffer));
    
    // Then: Payload has content
    TEST_ASSERT_TRUE(len > 0);
    
    // And: Contains nickname TLV (type 0x01)
    TEST_ASSERT_EQUAL_HEX8(BITCHAT_TLV_NICKNAME, buffer[0]);
    TEST_ASSERT_EQUAL(8, buffer[1]);  // "MeshNode" length
    TEST_ASSERT_EQUAL_STRING_LEN("MeshNode", &buffer[2], 8);
    
    // And: Contains Ed25519 pubkey TLV (type 0x03)
    size_t offset = 2 + 8;
    TEST_ASSERT_EQUAL_HEX8(BITCHAT_TLV_ED25519_PUBKEY, buffer[offset]);
    TEST_ASSERT_EQUAL(32, buffer[offset + 1]);
}

/**
 * Scenario: Build complete ANNOUNCE message
 * 
 * Given MeshCore identity
 * When building ANNOUNCE message
 * Then format matches BitChat spec
 */
void test_build_announce_message(void) {
    BitchatAnnounceBuilder builder;
    builder.setNodeName("TestNode");
    builder.setEd25519PubKey(TEST_PUBKEY);
    
    uint8_t buffer[256];
    size_t len = builder.buildMessage(buffer, sizeof(buffer));
    
    // Then: Message created
    TEST_ASSERT_TRUE(len >= BITCHAT_HEADER_SIZE);
    
    // And: Header correct
    TEST_ASSERT_EQUAL_HEX8(BITCHAT_VERSION, buffer[0]);
    TEST_ASSERT_EQUAL_HEX8(BITCHAT_MSG_ANNOUNCE, buffer[1]);
    TEST_ASSERT_EQUAL_HEX8(8, buffer[2]);  // ttl
    
    // And: Sender ID is peer ID (little-endian)
    size_t senderIdOffset = 14;  // After version(1) + type(1) + ttl(1) + timestamp(8) + flags(1) + payload_len(2)
    uint64_t senderId = 0;
    for (int i = 0; i < 8; i++) {
        senderId |= ((uint64_t)buffer[senderIdOffset + i]) << (i * 8);
    }
    TEST_ASSERT_EQUAL_HEX64(EXPECTED_PEER_ID, senderId);
}

/**
 * Scenario: Derive peer ID from Ed25519 key
 * 
 * Given Ed25519 public key
 * When getting peer ID
 * Then returns first 8 bytes as little-endian
 */
void test_announce_builder_peer_id(void) {
    BitchatAnnounceBuilder builder;
    builder.setEd25519PubKey(TEST_PUBKEY);
    
    uint64_t peerId = builder.getPeerId();
    
    TEST_ASSERT_EQUAL_HEX64(EXPECTED_PEER_ID, peerId);
}

/**
 * Scenario: Valid when required fields set
 * 
 * Given builder with name and pubkey
 * When checking validity
 * Then returns true
 */
void test_builder_valid_with_required_fields(void) {
    BitchatAnnounceBuilder builder;
    
    // Initially invalid
    TEST_ASSERT_FALSE(builder.isValid());
    
    // Add name only - still invalid
    builder.setNodeName("Node");
    TEST_ASSERT_FALSE(builder.isValid());
    
    // Add pubkey - now valid
    builder.setEd25519PubKey(TEST_PUBKEY);
    TEST_ASSERT_TRUE(builder.isValid());
}

/**
 * Scenario: Include optional Noise pubkey
 * 
 * Given builder with Noise key
 * When building payload
 * Then includes Noise pubkey TLV
 */
void test_builder_includes_noise_pubkey(void) {
    uint8_t noiseKey[32];
    memset(noiseKey, 0xAB, 32);
    
    BitchatAnnounceBuilder builder;
    builder.setNodeName("Node");
    builder.setEd25519PubKey(TEST_PUBKEY);
    builder.setNoisePubKey(noiseKey);
    
    uint8_t buffer[256];
    size_t len = builder.buildPayload(buffer, sizeof(buffer));
    
    // Should find Noise pubkey TLV (type 0x02)
    bool found = false;
    size_t offset = 0;
    while (offset < len) {
        uint8_t type = buffer[offset];
        uint8_t tlen = buffer[offset + 1];
        
        if (type == BITCHAT_TLV_NOISE_PUBKEY && tlen == 32) {
            found = true;
            // Verify content
            TEST_ASSERT_EQUAL_HEX8(0xAB, buffer[offset + 2]);
            break;
        }
        offset += 2 + tlen;
    }
    
    TEST_ASSERT_TRUE(found);
}

/**
 * Scenario: Returns 0 for invalid build
 * 
 * Given incomplete builder
 * When building message
 * Then returns 0
 */
void test_builder_returns_zero_for_invalid(void) {
    BitchatAnnounceBuilder builder;
    // Missing required fields
    
    uint8_t buffer[256];
    size_t len = builder.buildMessage(buffer, sizeof(buffer));
    
    TEST_ASSERT_EQUAL(0, len);
}

/**
 * Scenario: Truncate long node names
 * 
 * Given very long node name
 * When setting name
 * Then truncated to fit
 */
void test_builder_truncates_long_name(void) {
    BitchatAnnounceBuilder builder;
    
    char longName[100];
    memset(longName, 'A', sizeof(longName));
    longName[sizeof(longName) - 1] = '\0';
    
    builder.setNodeName(longName);
    builder.setEd25519PubKey(TEST_PUBKEY);
    
    uint8_t buffer[256];
    size_t len = builder.buildPayload(buffer, sizeof(buffer));
    
    // Name TLV length should be max 31 (not 99)
    TEST_ASSERT_TRUE(buffer[1] <= 31);
}

} // namespace bitchat
} // namespace test

#else // !ENABLE_BITCHAT

namespace test {
namespace bitchat {
void test_announce_placeholder(void) {
    TEST_IGNORE_MESSAGE("BitChat disabled - ENABLE_BITCHAT not defined");
}
} // namespace bitchat
} // namespace test

#endif // ENABLE_BITCHAT
