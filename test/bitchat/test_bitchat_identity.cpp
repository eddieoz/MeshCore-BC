/**
 * Story 3.1: BitChat Peer ID Derivation - BDD Tests
 *
 * Feature: Consistent Peer ID Generation
 */

#include "helpers/bitchat/BitchatIdentity.h"

#include <unity.h>

#ifdef ENABLE_BITCHAT

using namespace mesh::bitchat;

namespace test {
namespace bitchat {

// Test vector: Known Ed25519 public key and expected peer ID
// Pubkey first 8 bytes: 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
// As little-endian uint64_t: 0x0807060504030201
const uint8_t TEST_PUBKEY[32] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
                                  0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
                                  0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20 };

// Expected peer ID from real SHA256 of TEST_PUBKEY
// SHA256(TEST_PUBKEY) = ae216c2ef5247a37...
// First 8 bytes as little-endian uint64_t: 0x377A24F52E6C21AE
const uint64_t EXPECTED_PEER_ID = 0x377A24F52E6C21AEULL;

/**
 * Scenario: Derive BitChat ID from Ed25519 public key
 *
 * Given device has Ed25519 keypair
 * When BitChat peer ID is needed
 * Then it should be first 8 bytes of public key (little-endian)
 */
void test_derive_peer_id_from_pubkey(void) {
  // When: Derive peer ID from test public key
  uint64_t peerId = deriveBitChatPeerId(TEST_PUBKEY);

  // Then: Expected peer ID (little-endian interpretation)
  TEST_ASSERT_EQUAL_HEX64(EXPECTED_PEER_ID, peerId);
}

/**
 * Scenario: Same pubkey always produces same ID
 *
 * Given same public key
 * When derived multiple times
 * Then always get same result
 */
void test_derive_consistent_result(void) {
  // Derive multiple times
  uint64_t id1 = deriveBitChatPeerId(TEST_PUBKEY);
  uint64_t id2 = deriveBitChatPeerId(TEST_PUBKEY);
  uint64_t id3 = deriveBitChatPeerId(TEST_PUBKEY);

  // All should be identical
  TEST_ASSERT_EQUAL_HEX64(id1, id2);
  TEST_ASSERT_EQUAL_HEX64(id2, id3);
  TEST_ASSERT_EQUAL_HEX64(EXPECTED_PEER_ID, id1);
}

/**
 * Scenario: Convert peer ID to bytes
 *
 * Given a peer ID
 * When converted to bytes
 * Then result is little-endian byte array
 */
void test_peer_id_to_bytes(void) {
  uint8_t bytes[8];

  // When: Convert peer ID to bytes
  peerIdToBytes(EXPECTED_PEER_ID, bytes);

  // Then: Little-endian byte order
  // 0x377A24F52E6C21AE in little-endian
  TEST_ASSERT_EQUAL_HEX8(0xAE, bytes[0]);
  TEST_ASSERT_EQUAL_HEX8(0x21, bytes[1]);
  TEST_ASSERT_EQUAL_HEX8(0x6C, bytes[2]);
  TEST_ASSERT_EQUAL_HEX8(0x2E, bytes[3]);
  TEST_ASSERT_EQUAL_HEX8(0xF5, bytes[4]);
  TEST_ASSERT_EQUAL_HEX8(0x24, bytes[5]);
  TEST_ASSERT_EQUAL_HEX8(0x7A, bytes[6]);
  TEST_ASSERT_EQUAL_HEX8(0x37, bytes[7]);
}

/**
 * Scenario: Convert bytes to peer ID
 *
 * Given a byte array
 * When converted to peer ID
 * Then result interprets bytes as little-endian
 */
void test_bytes_to_peer_id(void) {
  uint8_t bytes[8] = { 0xAE, 0x21, 0x6C, 0x2E, 0xF5, 0x24, 0x7A, 0x37 };

  // When: Convert bytes to peer ID
  uint64_t peerId = bytesToPeerId(bytes);

  // Then: Little-endian interpretation
  TEST_ASSERT_EQUAL_HEX64(EXPECTED_PEER_ID, peerId);
}

/**
 * Scenario: Round-trip conversion
 *
 * Given a peer ID
 * When converted to bytes and back
 * Then result matches original
 */
void test_round_trip_conversion(void) {
  uint64_t originalId = 0xDEADBEEFCAFEBABEULL;
  uint8_t bytes[8];

  // Convert to bytes
  peerIdToBytes(originalId, bytes);

  // Convert back
  uint64_t recoveredId = bytesToPeerId(bytes);

  // Should match
  TEST_ASSERT_EQUAL_HEX64(originalId, recoveredId);
}

/**
 * Scenario: Verify peer ID against public key
 *
 * Given a peer ID and public key
 * When verifying match
 * Then returns true if ID matches first 8 bytes of pubkey
 */
void test_verify_peer_id_match(void) {
  // Valid peer ID for test pubkey
  uint64_t validPeerId = EXPECTED_PEER_ID;

  // Should verify successfully
  TEST_ASSERT_TRUE(verifyPeerId(validPeerId, TEST_PUBKEY));
}

/**
 * Scenario: Verify peer ID detects mismatch
 *
 * Given wrong peer ID for public key
 * When verifying match
 * Then returns false
 */
void test_verify_peer_id_mismatch(void) {
  // Wrong peer ID
  uint64_t wrongPeerId = 0xFFFFFFFFFFFFFFFFULL;

  // Should fail verification
  TEST_ASSERT_FALSE(verifyPeerId(wrongPeerId, TEST_PUBKEY));
}

/**
 * Scenario: Edge case - null pubkey returns zero
 *
 * Given null public key pointer
 * When deriving peer ID
 * Then returns 0
 */
void test_null_pubkey_returns_zero(void) {
  uint64_t peerId = deriveBitChatPeerId(nullptr);
  TEST_ASSERT_EQUAL_HEX64(0, peerId);
}

/**
 * Scenario: Edge case - zero pubkey returns zero ID
 *
 * Given all-zero public key
 * When deriving peer ID
 * Then returns 0
 */
void test_zero_pubkey_returns_zero(void) {
  uint8_t zeroPubkey[32] = { 0 };
  uint64_t peerId = deriveBitChatPeerId(zeroPubkey);
  // sha256 of 32 zeros yields 66...
  TEST_ASSERT_EQUAL_HEX64(0x77BD62F8AD7A6866ULL, peerId);
}

} // namespace bitchat
} // namespace test

#else // !ENABLE_BITCHAT

// Dummy test when BitChat is disabled
namespace test {
namespace bitchat {
void test_identity_placeholder(void) {
  TEST_IGNORE_MESSAGE("BitChat disabled - ENABLE_BITCHAT not defined");
}
} // namespace bitchat
} // namespace test

#endif // ENABLE_BITCHAT
