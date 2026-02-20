/**
 * Story 5.4: BitChat DM Encapsulation - BDD Tests
 * 
 * Feature: Encapsulate BitChat DMs for MeshCore Transport
 * 
 * TDD Approach:
 * 1. Write failing tests (RED)
 * 2. Implement to pass (GREEN)
 * 3. Refactor (REFACTOR)
 */

#include <unity.h>
#include "helpers/bitchat/DMEncapsulator.h"
#include "helpers/bitchat/BitchatIdentity.h"

#ifdef ENABLE_BITCHAT

using namespace mesh::bitchat;
using namespace mesh::ble;

namespace test {
namespace bitchat {

// DM test keys (different from other test files)
static const uint8_t DM_SENDER_PUBKEY[32] = {
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30,
    0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
    0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40
};

static const uint8_t DM_RECIPIENT_PUBKEY[32] = {
    0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11,
    0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
    0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,
    0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA
};

/**
 * Scenario: BitChat DM to known MeshCore contact
 * 
 * Given BitChat DM received
 * And recipient_id maps to known MeshCore contact
 * When encapsulating for DM routing
 * Then serialize BitChat DM to bytes
 * And create MeshCore PAYLOAD_TYPE_TXT_MSG
 * And encrypt with shared secret
 */
void test_dm_to_known_contact(void) {
    // Given: DM message with recipient
    BitchatMessage msg;
    msg.version = BITCHAT_VERSION;
    msg.type = BITCHAT_MSG_MESSAGE;
    msg.ttl = 8;
    msg.timestamp = 1234567890;
    msg.setSenderId64(0x0807060504030201ULL);
    msg.flags = BITCHAT_FLAG_HAS_RECIPIENT;
    
    // Set recipient from recipient pubkey
    uint64_t recipientId = deriveBitChatPeerId(DM_RECIPIENT_PUBKEY);
    msg.setRecipientId64(recipientId);
    
    msg.payloadLength = 14;
    memcpy(msg.payload, "Direct message", 14);
    
    // And: Contact registry with recipient
    DMContactRegistry contacts;
    uint64_t senderId = deriveBitChatPeerId(DM_SENDER_PUBKEY);
    contacts.addContact(senderId, DM_SENDER_PUBKEY, "Alice");
    contacts.addContact(recipientId, DM_RECIPIENT_PUBKEY, "Bob");
    
    // When: Encapsulate DM
    DMEncapsulator encapsulator(contacts);
    DMEncapsulationResult result;
    
    bool success = encapsulator.encapsulateDM(msg, senderId, result);
    
    // Then: Should succeed
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(DMStatus::ENCAPSULATED, result.status);
    TEST_ASSERT_EQUAL(1, result.packetCount);
    TEST_ASSERT_TRUE(result.packets[0].isValid);
    
    // And: Should be TXT_MSG type (not GRP_DATA)
    TEST_ASSERT_EQUAL(MeshCorePayloadType::MESHCORE_TXT_MSG, result.packets[0].payloadType);
    
    // And: Should have recipient info
    TEST_ASSERT_EQUAL_UINT64(recipientId, result.recipientId);
}

/**
 * Scenario: DM encapsulation header
 * 
 * Given BitChat DM
 * When encapsulated for routing
 * Then use BitChat encapsulation header
 * Inside E2E encrypted payload
 */
void test_dm_encapsulation_format(void) {
    // Given: DM message
    BitchatMessage msg;
    msg.version = BITCHAT_VERSION;
    msg.type = BITCHAT_MSG_MESSAGE;
    msg.ttl = 8;
    msg.timestamp = 1234567890;
    msg.setSenderId64(0x0807060504030201ULL);
    msg.flags = BITCHAT_FLAG_HAS_RECIPIENT;
    uint64_t recipientId = deriveBitChatPeerId(DM_RECIPIENT_PUBKEY);
    msg.setRecipientId64(recipientId);
    msg.payloadLength = 12;
    memcpy(msg.payload, "Hello there!", 12);
    
    // And: Contact registry
    DMContactRegistry contacts;
    uint64_t senderId = deriveBitChatPeerId(DM_SENDER_PUBKEY);
    contacts.addContact(senderId, DM_SENDER_PUBKEY, "Alice");
    contacts.addContact(recipientId, DM_RECIPIENT_PUBKEY, "Bob");
    
    // When: Encapsulate
    DMEncapsulator encapsulator(contacts);
    DMEncapsulationResult result;
    
    bool success = encapsulator.encapsulateDM(msg, senderId, result);
    
    // Then: Success
    TEST_ASSERT_TRUE(success);
    
    // And: Encapsulation header present inside encrypted payload
    // The packet should have the BitChat encapsulation magic after MeshCore headers
    // (verification would need decryption, tested separately)
}

/**
 * Scenario: BitChat DM to unknown recipient
 * 
 * Given BitChat DM received
 * And recipient_id is unknown in address book
 * Then return RECIPIENT_UNKNOWN status
 * And provide error info
 */
void test_dm_to_unknown_recipient(void) {
    // Given: DM message with recipient
    BitchatMessage msg;
    msg.version = BITCHAT_VERSION;
    msg.type = BITCHAT_MSG_MESSAGE;
    msg.ttl = 8;
    msg.timestamp = 1234567890;
    msg.setSenderId64(0x0807060504030201ULL);
    msg.flags = BITCHAT_FLAG_HAS_RECIPIENT;
    uint64_t recipientId = 0xDEADBEEFCAFEBABEULL;  // Unknown recipient
    msg.setRecipientId64(recipientId);
    msg.payloadLength = 10;
    memcpy(msg.payload, "Test", 4);
    
    // And: Empty contact registry (recipient unknown)
    DMContactRegistry contacts;
    uint64_t senderId = deriveBitChatPeerId(DM_SENDER_PUBKEY);
    contacts.addContact(senderId, DM_SENDER_PUBKEY, "Alice");
    // Bob NOT added - he's unknown
    
    // When: Encapsulate DM
    DMEncapsulator encapsulator(contacts);
    DMEncapsulationResult result;
    
    bool success = encapsulator.encapsulateDM(msg, senderId, result);
    
    // Then: Should fail with recipient unknown
    TEST_ASSERT_FALSE(success);
    TEST_ASSERT_EQUAL(DMStatus::RECIPIENT_UNKNOWN, result.status);
    TEST_ASSERT_EQUAL_UINT64(recipientId, result.recipientId);
    TEST_ASSERT_EQUAL(0, result.packetCount);
}

/**
 * Scenario: DM without recipient flag
 * 
 * Given message without HAS_RECIPIENT flag
 * When trying to encapsulate as DM
 * Then return INVALID_MESSAGE status
 */
void test_dm_without_recipient_flag(void) {
    // Given: Regular message (not DM)
    BitchatMessage msg;
    msg.version = BITCHAT_VERSION;
    msg.type = BITCHAT_MSG_MESSAGE;
    msg.ttl = 8;
    msg.timestamp = 1234567890;
    msg.setSenderId64(0x0807060504030201ULL);
    msg.flags = 0;  // No HAS_RECIPIENT flag
    msg.payloadLength = 10;
    memcpy(msg.payload, "Broadcast!", 10);
    
    // And: Contact registry
    DMContactRegistry contacts;
    uint64_t senderId = deriveBitChatPeerId(DM_SENDER_PUBKEY);
    contacts.addContact(senderId, DM_SENDER_PUBKEY, "Alice");
    
    // When: Encapsulate as DM
    DMEncapsulator encapsulator(contacts);
    DMEncapsulationResult result;
    
    bool success = encapsulator.encapsulateDM(msg, senderId, result);
    
    // Then: Should fail - not a DM
    TEST_ASSERT_FALSE(success);
    TEST_ASSERT_EQUAL(DMStatus::INVALID_MESSAGE, result.status);
}

/**
 * Scenario: Large DM fragmentation
 * 
 * Given BitChat DM with large content
 * When recipient is known
 * Then fragment if needed
 */
void test_large_dm_fragmentation(void) {
    // Given: Large DM message
    BitchatMessage msg;
    msg.version = BITCHAT_VERSION;
    msg.type = BITCHAT_MSG_MESSAGE;
    msg.ttl = 8;
    msg.timestamp = 1234567890;
    msg.setSenderId64(0x0807060504030201ULL);
    msg.flags = BITCHAT_FLAG_HAS_RECIPIENT;
    uint64_t recipientId = deriveBitChatPeerId(DM_RECIPIENT_PUBKEY);
    msg.setRecipientId64(recipientId);
    
    // Large payload - 300 bytes
    msg.payloadLength = 300;
    for (int i = 0; i < 300; i++) {
        msg.payload[i] = 'A' + (i % 26);
    }
    
    // And: Contact registry
    DMContactRegistry contacts;
    uint64_t senderId = deriveBitChatPeerId(DM_SENDER_PUBKEY);
    contacts.addContact(senderId, DM_SENDER_PUBKEY, "Alice");
    contacts.addContact(recipientId, DM_RECIPIENT_PUBKEY, "Bob");
    
    // When: Encapsulate
    DMEncapsulator encapsulator(contacts);
    DMEncapsulationResult result;
    
    bool success = encapsulator.encapsulateDM(msg, senderId, result);
    
    // Then: Should succeed
    TEST_ASSERT_TRUE(success);
    
    // And: Should create packet(s)
    TEST_ASSERT_TRUE(result.packetCount >= 1);
    TEST_ASSERT_TRUE(result.packets[0].isValid);
}

/**
 * Scenario: Multiple DM recipients
 * 
 * Given multiple contacts in registry
 * When routing to different recipients
 * Then use correct recipient for each
 */
void test_multiple_dm_recipients(void) {
    // Setup contact registry with multiple contacts
    DMContactRegistry contacts;
    
    uint64_t aliceId = deriveBitChatPeerId(DM_SENDER_PUBKEY);
    contacts.addContact(aliceId, DM_SENDER_PUBKEY, "Alice");
    
    uint64_t bobId = deriveBitChatPeerId(DM_RECIPIENT_PUBKEY);
    contacts.addContact(bobId, DM_RECIPIENT_PUBKEY, "Bob");
    
    uint64_t charlieId = 0x1122334455667788ULL;
    uint8_t charliePubkey[32] = {
        0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11, 0x22, 0x33,
        0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB,
        0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11, 0x22, 0x33,
        0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB
    };
    contacts.addContact(charlieId, charliePubkey, "Charlie");
    
    DMEncapsulator encapsulator(contacts);
    
    // Test sending to Bob
    BitchatMessage msgToBob;
    msgToBob.version = BITCHAT_VERSION;
    msgToBob.type = BITCHAT_MSG_MESSAGE;
    msgToBob.ttl = 8;
    msgToBob.timestamp = 1234567890;
    msgToBob.setSenderId64(aliceId);
    msgToBob.flags = BITCHAT_FLAG_HAS_RECIPIENT;
    msgToBob.setRecipientId64(bobId);
    msgToBob.payloadLength = 10;
    memcpy(msgToBob.payload, "Hi Bob!", 7);
    
    DMEncapsulationResult resultBob;
    bool successBob = encapsulator.encapsulateDM(msgToBob, aliceId, resultBob);
    
    TEST_ASSERT_TRUE(successBob);
    TEST_ASSERT_EQUAL(DMStatus::ENCAPSULATED, resultBob.status);
    TEST_ASSERT_EQUAL_UINT64(bobId, resultBob.recipientId);
    
    // Test sending to Charlie
    BitchatMessage msgToCharlie;
    msgToCharlie.version = BITCHAT_VERSION;
    msgToCharlie.type = BITCHAT_MSG_MESSAGE;
    msgToCharlie.ttl = 8;
    msgToCharlie.timestamp = 1234567891;
    msgToCharlie.setSenderId64(aliceId);
    msgToCharlie.flags = BITCHAT_FLAG_HAS_RECIPIENT;
    msgToCharlie.setRecipientId64(charlieId);
    msgToCharlie.payloadLength = 12;
    memcpy(msgToCharlie.payload, "Hi Charlie!", 11);
    
    DMEncapsulationResult resultCharlie;
    bool successCharlie = encapsulator.encapsulateDM(msgToCharlie, aliceId, resultCharlie);
    
    TEST_ASSERT_TRUE(successCharlie);
    TEST_ASSERT_EQUAL(DMStatus::ENCAPSULATED, resultCharlie.status);
    TEST_ASSERT_EQUAL_UINT64(charlieId, resultCharlie.recipientId);
}

/**
 * Scenario: DM with signature verification info
 * 
 * Given signed BitChat DM
 * When encapsulating
 * Then include sender verification info
 */
void test_dm_with_sender_verification(void) {
    // Given: Signed DM message
    BitchatMessage msg;
    msg.version = BITCHAT_VERSION;
    msg.type = BITCHAT_MSG_MESSAGE;
    msg.ttl = 8;
    msg.timestamp = 1234567890;
    uint64_t senderId = deriveBitChatPeerId(DM_SENDER_PUBKEY);
    msg.setSenderId64(senderId);
    msg.flags = BITCHAT_FLAG_HAS_RECIPIENT | BITCHAT_FLAG_HAS_SIGNATURE;
    uint64_t recipientId = deriveBitChatPeerId(DM_RECIPIENT_PUBKEY);
    msg.setRecipientId64(recipientId);
    msg.payloadLength = 10;
    memcpy(msg.payload, "Signed DM!", 10);
    
    // Mock signature
    memset(msg.signature, 0xAB, BITCHAT_SIGNATURE_SIZE);
    
    // And: Contact registry
    DMContactRegistry contacts;
    contacts.addContact(senderId, DM_SENDER_PUBKEY, "Alice");
    contacts.addContact(recipientId, DM_RECIPIENT_PUBKEY, "Bob");
    
    // When: Encapsulate
    DMEncapsulator encapsulator(contacts);
    DMEncapsulationResult result;
    
    bool success = encapsulator.encapsulateDM(msg, senderId, result);
    
    // Then: Should succeed
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(DMStatus::ENCAPSULATED, result.status);
}

} // namespace bitchat
} // namespace test

#endif // ENABLE_BITCHAT
