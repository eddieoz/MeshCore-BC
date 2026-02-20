/**
 * Story 7.4: Signature Verification and Generation - BDD Tests
 * 
 * Feature: BitChat Ed25519 Signatures
 * 
 * TDD Approach:
 * 1. Write failing tests (RED)
 * 2. Implement to pass (GREEN)
 * 3. Refactor (REFACTOR)
 */

#include <unity.h>
#include "helpers/bitchat/Signature.h"

#ifdef ENABLE_BITCHAT

using namespace mesh::bitchat;

namespace test {
namespace bitchat {

// Signature test keys (unique to this test file)
static const uint8_t SIG_TEST_PUBKEY[32] = {
    0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
    0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0xA0,
    0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8,
    0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0
};

static const uint8_t SIG_TEST_PRIVKEY[64] = {
    0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
    0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0xA0,
    0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8,
    0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0,
    0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8,
    0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0,
    0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8,
    0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0
};

/**
 * Scenario: Sign outgoing BitChat message
 * 
 * Given creating BitChat MESSAGE
 * When signing
 * Then use TTL=0 for signature data
 * And sign with Ed25519 private key
 * And set signature flag
 */
void test_sign_message(void) {
    // Given: Message data
    uint8_t message[] = "Hello, this is a test message for signing!";
    size_t messageLen = sizeof(message) - 1;
    
    // When: Sign
    SignatureManager signer;
    uint8_t signature[SIGNATURE_SIZE];
    
    bool signed_ok = signer.sign(message, messageLen, SIG_TEST_PRIVKEY, signature);
    
    // Then: Should sign successfully
    TEST_ASSERT_TRUE(signed_ok);
    
    // And: Signature should be non-zero
    bool allZero = true;
    for (size_t i = 0; i < SIGNATURE_SIZE; i++) {
        if (signature[i] != 0) {
            allZero = false;
            break;
        }
    }
    TEST_ASSERT_FALSE(allZero);
}

/**
 * Scenario: Verify incoming BitChat signature
 * 
 * Given BitChat message with signature
 * When verifying
 * Then extract Ed25519 pubkey
 * And verify signature
 * And accept if valid
 */
void test_verify_valid_signature(void) {
    // Given: Message and signature
    uint8_t message[] = "Test message for verification";
    size_t messageLen = sizeof(message) - 1;
    
    SignatureManager signer;
    uint8_t signature[SIGNATURE_SIZE];
    
    bool signed_ok = signer.sign(message, messageLen, SIG_TEST_PRIVKEY, signature);
    TEST_ASSERT_TRUE(signed_ok);
    
    // When: Verify with correct public key
    bool verified = signer.verify(message, messageLen, signature, SIG_TEST_PUBKEY);
    
    // Then: Should verify successfully
    TEST_ASSERT_TRUE(verified);
}

/**
 * Scenario: Reject invalid signature
 * 
 * Given BitChat message
 * And wrong signature
 * When verifying
 * Then reject
 */
void test_reject_invalid_signature(void) {
    // Given: Message
    uint8_t message[] = "Test message";
    size_t messageLen = sizeof(message) - 1;
    
    // And: Wrong signature (random bytes)
    uint8_t badSignature[SIGNATURE_SIZE];
    memset(badSignature, 0xAB, sizeof(badSignature));
    
    // When: Verify
    SignatureManager signer;
    bool verified = signer.verify(message, messageLen, badSignature, SIG_TEST_PUBKEY);
    
    // Then: Should reject
    TEST_ASSERT_FALSE(verified);
}

/**
 * Scenario: Sign and verify roundtrip
 * 
 * Given message
 * When signing then verifying
 * Then verify succeeds
 */
void test_sign_verify_roundtrip(void) {
    // Given: Message
    uint8_t message[] = "Roundtrip test message for signature!";
    size_t messageLen = sizeof(message) - 1;
    
    SignatureManager signer;
    uint8_t signature[SIGNATURE_SIZE];
    
    // When: Sign
    bool signed_ok = signer.sign(message, messageLen, SIG_TEST_PRIVKEY, signature);
    TEST_ASSERT_TRUE(signed_ok);
    
    // And: Verify
    bool verified = signer.verify(message, messageLen, signature, SIG_TEST_PUBKEY);
    TEST_ASSERT_TRUE(verified);
}

/**
 * Scenario: Different message same key
 * 
 * Given different messages
 * When signing
 * Then produce different signatures
 */
void test_different_messages_different_signatures(void) {
    SignatureManager signer;
    
    uint8_t msg1[] = "Message one";
    uint8_t msg2[] = "Message two";
    
    uint8_t sig1[SIGNATURE_SIZE];
    uint8_t sig2[SIGNATURE_SIZE];
    
    signer.sign(msg1, sizeof(msg1) - 1, SIG_TEST_PRIVKEY, sig1);
    signer.sign(msg2, sizeof(msg2) - 1, SIG_TEST_PRIVKEY, sig2);
    
    // Signatures should be different
    TEST_ASSERT_TRUE(memcmp(sig1, sig2, SIGNATURE_SIZE) != 0);
    
    // But both should verify
    TEST_ASSERT_TRUE(signer.verify(msg1, sizeof(msg1) - 1, sig1, SIG_TEST_PUBKEY));
    TEST_ASSERT_TRUE(signer.verify(msg2, sizeof(msg2) - 1, sig2, SIG_TEST_PUBKEY));
}

/**
 * Scenario: Same message different keys
 * 
 * Given same message
 * When signing with different keys
 * Then produce different signatures
 */
void test_same_message_different_keys(void) {
    SignatureManager signer;
    
    uint8_t message[] = "Same message";
    size_t messageLen = sizeof(message) - 1;
    
    uint8_t sig1[SIGNATURE_SIZE];
    uint8_t sig2[SIGNATURE_SIZE];
    
    // Different private key
    uint8_t privkey2[64];
    memcpy(privkey2, SIG_TEST_PRIVKEY, 64);
    privkey2[0] = 0xFF;  // Change first byte
    
    signer.sign(message, messageLen, SIG_TEST_PRIVKEY, sig1);
    signer.sign(message, messageLen, privkey2, sig2);
    
    // Signatures should be different
    TEST_ASSERT_TRUE(memcmp(sig1, sig2, SIGNATURE_SIZE) != 0);
}

/**
 * Scenario: Null parameters handling
 * 
 * Given null parameters
 * When signing/verifying
 * Then fail gracefully
 */
void test_signature_null_parameters(void) {
    SignatureManager signer;
    uint8_t message[] = "Test";
    uint8_t signature[SIGNATURE_SIZE];
    
    // Null message
    TEST_ASSERT_FALSE(signer.sign(nullptr, 4, SIG_TEST_PRIVKEY, signature));
    TEST_ASSERT_FALSE(signer.verify(nullptr, 4, signature, SIG_TEST_PUBKEY));
    
    // Null key
    TEST_ASSERT_FALSE(signer.sign(message, 4, nullptr, signature));
    TEST_ASSERT_FALSE(signer.verify(message, 4, signature, nullptr));
    
    // Null signature output
    TEST_ASSERT_FALSE(signer.sign(message, 4, SIG_TEST_PRIVKEY, nullptr));
}

/**
 * Scenario: Empty message handling
 * 
 * Given empty message
 * When signing
 * Then succeed (sign empty data)
 */
void test_empty_message(void) {
    SignatureManager signer;
    uint8_t signature[SIGNATURE_SIZE];
    
    // Empty message should be signable
    bool signed_ok = signer.sign((uint8_t*)"", 0, SIG_TEST_PRIVKEY, signature);
    TEST_ASSERT_TRUE(signed_ok);
}

} // namespace bitchat
} // namespace test

#endif // ENABLE_BITCHAT
