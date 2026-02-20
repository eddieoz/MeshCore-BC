/**
 * Story 6.2: MeshCore DM Decapsulation - BDD Tests
 * 
 * Feature: Decapsulate BitChat DMs from MeshCore
 * 
 * TDD Approach:
 * 1. Write failing tests (RED)
 * 2. Implement to pass (GREEN)
 * 3. Refactor (REFACTOR)
 */

#include <unity.h>
#include "helpers/bitchat/DMDecapsulator.h"
#include "helpers/bitchat/DMEncapsulator.h"
#include "helpers/bitchat/BitchatIdentity.h"

#ifdef ENABLE_BITCHAT

using namespace mesh::bitchat;
using namespace mesh::ble;

namespace test {
namespace bitchat {

// DM decapsulation test keys
static const uint8_t DM_DECAP_SENDER_KEY[32] = {
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
    0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80,
    0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88,
    0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90
};

static const uint8_t DM_DECAP_RECIPIENT_KEY[32] = {
    0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
    0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0xA0,
    0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8,
    0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0
};

/**
 * Scenario: Receive encapsulated BitChat DM
 * 
 * Given MeshCore receives PAYLOAD_TYPE_TXT_MSG from contact
 * And decrypted payload has BitChat encapsulation header
 * When message is from known contact with BitChat mapping
 * Then extract BitChat DM from encapsulated payload
 * And forward to BitChat app via BLE
 */
void test_receive_dm_from_known_contact(void) {
    // Given: Contact registry with sender
    DMContactRegistry contacts;
    uint64_t senderId = deriveBitChatPeerId(DM_DECAP_SENDER_KEY);
    uint64_t recipientId = deriveBitChatPeerId(DM_DECAP_RECIPIENT_KEY);
    contacts.addContact(senderId, DM_DECAP_SENDER_KEY, "Alice");
    contacts.addContact(recipientId, DM_DECAP_RECIPIENT_KEY, "Bob");
    
    // And: Original BitChat DM message
    BitchatMessage originalMsg;
    originalMsg.version = BITCHAT_VERSION;
    originalMsg.type = BITCHAT_MSG_MESSAGE;
    originalMsg.ttl = 8;
    originalMsg.timestamp = 1234567890;
    originalMsg.setSenderId64(senderId);
    originalMsg.flags = BITCHAT_FLAG_HAS_RECIPIENT;
    originalMsg.setRecipientId64(recipientId);
    originalMsg.payloadLength = 20;
    memcpy(originalMsg.payload, "Hello Bob, it's Alice!", 20);
    
    // And: Encapsulated DM packet
    DMEncapsulator encapsulator(contacts);
    DMEncapsulationResult encapResult;
    encapsulator.encapsulateDM(originalMsg, senderId, encapResult);
    
    TEST_ASSERT_TRUE(encapResult.packets[0].isValid);
    
    // When: Decapsulate the DM
    DMDecapsulator decapsulator(contacts);
    DMDecapsulationResult result;
    
    bool success = decapsulator.decapsulateDM(
        encapResult.packets[0],
        senderId,
        DM_DECAP_RECIPIENT_KEY,  // Recipient decrypts with their key
        result
    );
    
    // Then: Should succeed
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(DMDecapsulationStatus::SUCCESS, result.status);
    
    // And: Should extract original message
    TEST_ASSERT_EQUAL(originalMsg.version, result.message.version);
    TEST_ASSERT_EQUAL(originalMsg.type, result.message.type);
    TEST_ASSERT_EQUAL_UINT64(senderId, result.message.getSenderId64());
    TEST_ASSERT_EQUAL(originalMsg.payloadLength, result.message.payloadLength);
    TEST_ASSERT_EQUAL_MEMORY(originalMsg.payload, result.message.payload, originalMsg.payloadLength);
    
    // And: Should identify sender
    TEST_ASSERT_EQUAL_UINT64(senderId, result.senderId);
    TEST_ASSERT_TRUE(result.isKnownContact);
}

/**
 * Scenario: DM from non-BitChat source
 * 
 * Given MeshCore receives PAYLOAD_TYPE_TXT_MSG
 * And payload does NOT have BitChat encapsulation
 * Then process as normal MeshCore DM
 * And do not forward to BitChat
 */
void test_dm_from_non_bitchat_source(void) {
    // Given: Contact registry
    DMContactRegistry contacts;
    uint64_t senderId = deriveBitChatPeerId(DM_DECAP_SENDER_KEY);
    contacts.addContact(senderId, DM_DECAP_SENDER_KEY, "Alice");
    
    // And: Non-BitChat packet (random data)
    EncapsulatedPacket packet;
    packet.length = 50;
    memset(packet.data, 0xAB, packet.length);
    packet.isValid = true;
    packet.payloadType = MeshCorePayloadType::MESHCORE_TXT_MSG;
    
    // When: Decapsulate
    DMDecapsulator decapsulator(contacts);
    DMDecapsulationResult result;
    
    bool success = decapsulator.decapsulateDM(
        packet,
        senderId,
        DM_DECAP_SENDER_KEY,
        result
    );
    
    // Then: Should fail with NOT_BITCHAT
    TEST_ASSERT_FALSE(success);
    TEST_ASSERT_EQUAL(DMDecapsulationStatus::NOT_BITCHAT, result.status);
}

/**
 * Scenario: DM from unknown contact
 * 
 * Given DM received from unknown sender
 * When decapsulating
 * Then mark as unknown contact but still extract message
 */
void test_dm_from_unknown_contact(void) {
    // Given: Empty contact registry (sender unknown)
    DMContactRegistry contacts;
    uint64_t senderId = deriveBitChatPeerId(DM_DECAP_SENDER_KEY);
    uint64_t recipientId = deriveBitChatPeerId(DM_DECAP_RECIPIENT_KEY);
    // Note: NOT adding sender to registry
    contacts.addContact(recipientId, DM_DECAP_RECIPIENT_KEY, "Bob");
    
    // And: Original message
    BitchatMessage originalMsg;
    originalMsg.version = BITCHAT_VERSION;
    originalMsg.type = BITCHAT_MSG_MESSAGE;
    originalMsg.ttl = 8;
    originalMsg.timestamp = 1234567890;
    originalMsg.setSenderId64(senderId);
    originalMsg.flags = BITCHAT_FLAG_HAS_RECIPIENT;
    originalMsg.setRecipientId64(recipientId);
    originalMsg.payloadLength = 15;
    memcpy(originalMsg.payload, "Anonymous msg!", 15);
    
    // And: Encapsulated packet (encrypt with sender's key as placeholder)
    DMEncapsulator encap(contacts);
    DMEncapsulationResult encapResult;
    encap.encapsulateDM(originalMsg, senderId, encapResult);
    
    // When: Decapsulate (use recipient key since that's what was used to encrypt)
    DMDecapsulator decapsulator(contacts);
    DMDecapsulationResult result;
    
    bool success = decapsulator.decapsulateDM(
        encapResult.packets[0],
        senderId,
        DM_DECAP_RECIPIENT_KEY,  // Bob's key was used to encrypt (recipient)
        result
    );
    
    // Then: Should succeed extracting message
    TEST_ASSERT_TRUE(success);
    
    // But: Mark as unknown contact
    TEST_ASSERT_FALSE(result.isKnownContact);
    TEST_ASSERT_EQUAL_UINT64(senderId, result.senderId);
}

/**
 * Scenario: Wrong recipient key
 * 
 * Given DM encrypted with recipient key A
 * When decrypting with recipient key B
 * Then decryption should fail or produce garbage
 */
void test_dm_wrong_recipient_key(void) {
    // Given: Contact registry
    DMContactRegistry contacts;
    uint64_t senderId = deriveBitChatPeerId(DM_DECAP_SENDER_KEY);
    uint64_t recipientId = deriveBitChatPeerId(DM_DECAP_RECIPIENT_KEY);
    contacts.addContact(senderId, DM_DECAP_SENDER_KEY, "Alice");
    contacts.addContact(recipientId, DM_DECAP_RECIPIENT_KEY, "Bob");
    
    // And: Original message
    BitchatMessage originalMsg;
    originalMsg.version = BITCHAT_VERSION;
    originalMsg.type = BITCHAT_MSG_MESSAGE;
    originalMsg.ttl = 8;
    originalMsg.timestamp = 1234567890;
    originalMsg.setSenderId64(senderId);
    originalMsg.flags = BITCHAT_FLAG_HAS_RECIPIENT;
    originalMsg.setRecipientId64(recipientId);
    originalMsg.payloadLength = 10;
    memcpy(originalMsg.payload, "Secret DM!", 10);
    
    // And: Encapsulated
    DMEncapsulator encap(contacts);
    DMEncapsulationResult encapResult;
    encap.encapsulateDM(originalMsg, senderId, encapResult);
    
    // Wrong key
    uint8_t wrongKey[32] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };
    
    // When: Decrypt with wrong key
    DMDecapsulator decapsulator(contacts);
    DMDecapsulationResult result;
    
    bool success = decapsulator.decapsulateDM(
        encapResult.packets[0],
        senderId,
        wrongKey,
        result
    );
    
    // Then: With XOR cipher, will decrypt but produce garbage
    // The version check should catch this
    if (success) {
        TEST_ASSERT_NOT_EQUAL(originalMsg.version, result.message.version);
    }
}

/**
 * Scenario: DM packet too short
 * 
 * Given packet smaller than encapsulation header
 * When decapsulating
 * Then fail with INVALID_PACKET
 */
void test_dm_packet_too_short(void) {
    // Given: Contact registry
    DMContactRegistry contacts;
    uint64_t senderId = deriveBitChatPeerId(DM_DECAP_SENDER_KEY);
    contacts.addContact(senderId, DM_DECAP_SENDER_KEY, "Alice");
    
    // And: Too-short packet
    EncapsulatedPacket packet;
    packet.length = 5;
    memset(packet.data, 0xBC, packet.length);
    packet.isValid = true;
    packet.payloadType = MeshCorePayloadType::MESHCORE_TXT_MSG;
    
    // When: Decapsulate
    DMDecapsulator decapsulator(contacts);
    DMDecapsulationResult result;
    
    bool success = decapsulator.decapsulateDM(
        packet,
        senderId,
        DM_DECAP_SENDER_KEY,
        result
    );
    
    // Then: Should fail
    TEST_ASSERT_FALSE(success);
    TEST_ASSERT_EQUAL(DMDecapsulationStatus::INVALID_PACKET, result.status);
}

/**
 * Scenario: Verify sender info extraction
 * 
 * Given DM from known contact
 * When decapsulating
 * Then correctly identify sender nickname
 */
void test_dm_sender_info_extraction(void) {
    // Given: Contact registry with sender info
    DMContactRegistry contacts;
    uint64_t senderId = deriveBitChatPeerId(DM_DECAP_SENDER_KEY);
    contacts.addContact(senderId, DM_DECAP_SENDER_KEY, "AliceTheSender");
    
    uint64_t recipientId = deriveBitChatPeerId(DM_DECAP_RECIPIENT_KEY);
    contacts.addContact(recipientId, DM_DECAP_RECIPIENT_KEY, "Bob");
    
    // And: Original message
    BitchatMessage originalMsg;
    originalMsg.version = BITCHAT_VERSION;
    originalMsg.type = BITCHAT_MSG_MESSAGE;
    originalMsg.ttl = 8;
    originalMsg.timestamp = 1234567890;
    originalMsg.setSenderId64(senderId);
    originalMsg.flags = BITCHAT_FLAG_HAS_RECIPIENT;
    originalMsg.setRecipientId64(recipientId);
    originalMsg.payloadLength = 12;
    memcpy(originalMsg.payload, "Hello there!", 12);
    
    // And: Encapsulated
    DMEncapsulator encap(contacts);
    DMEncapsulationResult encapResult;
    encap.encapsulateDM(originalMsg, senderId, encapResult);
    
    // When: Decapsulate
    DMDecapsulator decapsulator(contacts);
    DMDecapsulationResult result;
    
    bool success = decapsulator.decapsulateDM(
        encapResult.packets[0],
        senderId,
        DM_DECAP_RECIPIENT_KEY,
        result
    );
    
    // Then: Should succeed
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_TRUE(result.isKnownContact);
    TEST_ASSERT_EQUAL_UINT64(senderId, result.senderId);
}

/**
 * Scenario: Multiple DMs from different contacts
 * 
 * Given multiple contacts
 * When receiving DMs from each
 * Then correctly identify each sender
 */
void test_multiple_dm_senders(void) {
    // Given: Contact registry with multiple contacts
    DMContactRegistry contacts;
    
    uint64_t aliceId = deriveBitChatPeerId(DM_DECAP_SENDER_KEY);
    contacts.addContact(aliceId, DM_DECAP_SENDER_KEY, "Alice");
    
    uint8_t bobKey[32] = {
        0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8,
        0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0,
        0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8,
        0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 0xE0
    };
    uint64_t bobId = deriveBitChatPeerId(bobKey);
    contacts.addContact(bobId, bobKey, "Bob");
    
    uint64_t recipientId = deriveBitChatPeerId(DM_DECAP_RECIPIENT_KEY);
    contacts.addContact(recipientId, DM_DECAP_RECIPIENT_KEY, "Charlie");
    
    DMDecapsulator decapsulator(contacts);
    DMEncapsulator encap(contacts);
    
    // DM from Alice
    BitchatMessage msgFromAlice;
    msgFromAlice.version = BITCHAT_VERSION;
    msgFromAlice.type = BITCHAT_MSG_MESSAGE;
    msgFromAlice.ttl = 8;
    msgFromAlice.timestamp = 1234567890;
    msgFromAlice.setSenderId64(aliceId);
    msgFromAlice.flags = BITCHAT_FLAG_HAS_RECIPIENT;
    msgFromAlice.setRecipientId64(recipientId);
    msgFromAlice.payloadLength = 10;
    memcpy(msgFromAlice.payload, "From Alice", 10);
    
    DMEncapsulationResult encapAlice;
    encap.encapsulateDM(msgFromAlice, aliceId, encapAlice);
    
    DMDecapsulationResult resultAlice;
    bool ok1 = decapsulator.decapsulateDM(
        encapAlice.packets[0],
        aliceId,
        DM_DECAP_RECIPIENT_KEY,
        resultAlice
    );
    
    TEST_ASSERT_TRUE(ok1);
    TEST_ASSERT_TRUE(resultAlice.isKnownContact);
    TEST_ASSERT_EQUAL_UINT64(aliceId, resultAlice.senderId);
    
    // DM from Bob
    BitchatMessage msgFromBob;
    msgFromBob.version = BITCHAT_VERSION;
    msgFromBob.type = BITCHAT_MSG_MESSAGE;
    msgFromBob.ttl = 8;
    msgFromBob.timestamp = 1234567891;
    msgFromBob.setSenderId64(bobId);
    msgFromBob.flags = BITCHAT_FLAG_HAS_RECIPIENT;
    msgFromBob.setRecipientId64(recipientId);
    msgFromBob.payloadLength = 8;
    memcpy(msgFromBob.payload, "From Bob", 8);
    
    DMEncapsulationResult encapBob;
    encap.encapsulateDM(msgFromBob, bobId, encapBob);
    
    DMDecapsulationResult resultBob;
    bool ok2 = decapsulator.decapsulateDM(
        encapBob.packets[0],
        bobId,
        DM_DECAP_RECIPIENT_KEY,
        resultBob
    );
    
    TEST_ASSERT_TRUE(ok2);
    TEST_ASSERT_TRUE(resultBob.isKnownContact);
    TEST_ASSERT_EQUAL_UINT64(bobId, resultBob.senderId);
}

} // namespace bitchat
} // namespace test

#endif // ENABLE_BITCHAT
