/**
 * Story 9.1: Unit Tests for Protocol Layer - BDD Tests
 *
 * Feature: Protocol Parser Tests
 *
 * TDD Approach:
 * 1. Write failing tests (RED)
 * 2. Implement to pass (GREEN)
 * 3. Refactor (REFACTOR)
 */

#include "helpers/bitchat/BitchatMessageEncapsulator.h"
#include "helpers/bitchat/BitchatMessageParser.h"
#include "helpers/bitchat/ProtocolTestVectors.h"

#include <unity.h>

#ifdef ENABLE_BITCHAT

using namespace mesh::bitchat;

namespace test {
namespace bitchat {

/**
 * Scenario: Test vector validation - MESSAGE type
 *
 * Given known-good BitChat MESSAGE packet
 * When parsed
 * Then should match expected structure
 */
void test_vector_message_type(void) {
  // Given: Known-good MESSAGE packet
  ProtocolTestVectors vectors;
  TestVector msgVector;
  bool gotVector = vectors.getMessageVector(msgVector);
  TEST_ASSERT_TRUE(gotVector);

  // When: Parse
  BitchatMessageParser parser;
  ParsedBitchatMessage result;
  bool parsed = parser.parseMessage(msgVector.data, msgVector.length, result);

  // Then: Should parse correctly
  TEST_ASSERT_TRUE(parsed);
  TEST_ASSERT_EQUAL(BITCHAT_MSG_MESSAGE, result.type);
  TEST_ASSERT_EQUAL(msgVector.expectedVersion, result.version);
}

/**
 * Scenario: Test vector validation - ANNOUNCE type
 *
 * Given known-good BitChat ANNOUNCE packet
 * When parsed
 * Then should match expected structure
 */
void test_vector_announce_type(void) {
  // Given: Known-good ANNOUNCE packet
  ProtocolTestVectors vectors;
  TestVector announceVector;
  bool gotVector = vectors.getAnnounceVector(announceVector);
  TEST_ASSERT_TRUE(gotVector);

  // When: Parse
  BitchatMessageParser parser;
  ParsedBitchatMessage result;
  bool parsed = parser.parseMessage(announceVector.data, announceVector.length, result);

  // Then: Should parse correctly
  TEST_ASSERT_TRUE(parsed);
  TEST_ASSERT_EQUAL(BITCHAT_MSG_ANNOUNCE, result.type);
}

/**
 * Scenario: Test vector validation - LEAVE type
 *
 * Given known-good BitChat LEAVE packet
 * When parsed
 * Then should match expected structure
 */
void test_vector_leave_type(void) {
  // Given: Known-good LEAVE packet
  ProtocolTestVectors vectors;
  TestVector leaveVector;
  bool gotVector = vectors.getLeaveVector(leaveVector);
  TEST_ASSERT_TRUE(gotVector);

  // When: Parse
  BitchatMessageParser parser;
  ParsedBitchatMessage result;
  bool parsed = parser.parseMessage(leaveVector.data, leaveVector.length, result);

  // Then: Should parse correctly
  TEST_ASSERT_TRUE(parsed);
  TEST_ASSERT_EQUAL(BITCHAT_MSG_LEAVE, result.type);
}

/**
 * Scenario: Fuzz testing - random data
 *
 * Given random input data
 * When parsed
 * Then should not crash
 * And should return parse error appropriately
 */
void test_fuzz_random_data(void) {
  // Given: Random data
  ProtocolTestVectors fuzzer;
  uint8_t randomData[256];
  size_t randomLen = 0;
  fuzzer.generateRandomData(randomData, sizeof(randomData), &randomLen);

  // When: Parse
  BitchatMessageParser parser;
  ParsedBitchatMessage result;
  bool parsed = parser.parseMessage(randomData, randomLen, result);

  // Then: Should not crash (we got here means no crash)
  // Parse may succeed or fail, but shouldn't crash
  // Result is undefined for random data
  (void)parsed; // Suppress unused warning
}

/**
 * Scenario: Fuzz testing - truncated packets
 *
 * Given truncated packet data
 * When parsed
 * Then should handle gracefully
 */
void test_fuzz_truncated_packets(void) {
  // Given: Truncated versions of valid packets
  ProtocolTestVectors vectors;
  TestVector msgVector;
  vectors.getMessageVector(msgVector);

  BitchatMessageParser parser;

  // Try parsing at various truncation points
  for (size_t len = 1; len < msgVector.length && len < 20; len++) {
    ParsedBitchatMessage result;
    bool parsed = parser.parseMessage(msgVector.data, len, result);

    // Should either parse or fail gracefully
    // Just ensure no crash
    (void)parsed;
  }
}

/**
 * Scenario: Fuzz testing - corrupted data
 *
 * Given corrupted packet data
 * When parsed
 * Then should detect corruption or fail gracefully
 */
void test_fuzz_corrupted_data(void) {
  // Given: Valid packet with corruption
  ProtocolTestVectors vectors;
  TestVector msgVector;
  vectors.getMessageVector(msgVector);

  // Corrupt various bytes
  for (size_t i = 0; i < msgVector.length && i < 10; i++) {
    uint8_t corrupted[256];
    memcpy(corrupted, msgVector.data, msgVector.length);
    corrupted[i] ^= 0xFF; // Flip bits

    BitchatMessageParser parser;
    ParsedBitchatMessage result;
    bool parsed = parser.parseMessage(corrupted, msgVector.length, result);

    // May parse or fail, but shouldn't crash
    (void)parsed;
  }
}

/**
 * Scenario: Encapsulation test vectors
 *
 * Given known input data
 * When encapsulated
 * Then should match expected output
 */
void test_encapsulation_vectors(void) {
  // Given: Test message
  BitchatMessage msg;
  msg.version = BITCHAT_VERSION;
  msg.type = BITCHAT_MSG_MESSAGE;
  msg.ttl = 8;
  msg.timestamp = 1704067200;
  msg.setSenderId64(0x1234567890ABCDEFULL);
  msg.payloadLength = 12;
  memcpy(msg.payload, "Test message", 12);

  // And: Test key
  uint8_t key[32] = { 0 };

  // When: Encapsulate
  BitchatMessageEncapsulator encapsulator;
  EncapsulatedPacket packet;
  bool success = encapsulator.encapsulate(msg, key, packet);

  // Then: Should succeed
  TEST_ASSERT_TRUE(success);
  TEST_ASSERT_TRUE(packet.isValid);

  // And: Should have correct magic
  TEST_ASSERT_EQUAL_HEX8('B', packet.data[0]);
  TEST_ASSERT_EQUAL_HEX8('C', packet.data[1]);
}

/**
 * Scenario: Roundtrip test vectors
 *
 * Given message
 * When encapsulated then decapsulated
 * Then recover original
 */
void test_roundtrip_vectors(void) {
  ProtocolTestVectors vectors;
  TestVector msgVector;
  vectors.getMessageVector(msgVector);

  // Parse original
  BitchatMessageParser parser;
  ParsedBitchatMessage original;
  TEST_ASSERT_TRUE(parser.parseMessage(msgVector.data, msgVector.length, original));

  // Encapsulate
  // Convert Parsed to BitchatMessage for encapsulation
  BitchatMessage msgToEncap;
  msgToEncap.version = original.version;
  msgToEncap.type = original.type;
  msgToEncap.ttl = original.ttl;
  msgToEncap.timestamp = original.timestamp;
  msgToEncap.setSenderId64(original.senderId);
  msgToEncap.payloadLength = original.originalPayloadLen;
  if (original.originalPayload && original.originalPayloadLen > 0) {
    memcpy(msgToEncap.payload, original.originalPayload, original.originalPayloadLen);
  }

  uint8_t key[32] = { 0 };
  BitchatMessageEncapsulator encapsulator;
  EncapsulatedPacket packet;
  TEST_ASSERT_TRUE(encapsulator.encapsulate(msgToEncap, key, packet));

  // Decapsulate
  mesh::ble::BitchatMessage recovered;
  TEST_ASSERT_TRUE(encapsulator.decapsulate(packet, key, recovered));

  // Verify
  TEST_ASSERT_EQUAL(original.type, recovered.type);
  TEST_ASSERT_EQUAL(original.version, recovered.version);
  TEST_ASSERT_EQUAL(original.originalPayloadLen, recovered.payloadLength);
}

/**
 * Scenario: Edge case - zero length payload
 *
 * Given packet with zero-length payload
 * When parsed
 * Then handle appropriately
 */
void test_edge_zero_length_payload(void) {
  // Create minimal valid header with zero payload
  uint8_t data[22] = {
    BITCHAT_VERSION,     // version
    BITCHAT_MSG_MESSAGE, // type
    8,                   // ttl
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, // timestamp (8 bytes)
    0, // flags
    0,
    0, // payload length (2 bytes, big-endian = 0)
    0x01,
    0x02,
    0x03,
    0x04,
    0x05,
    0x06,
    0x07,
    0x08 // sender ID
  };

  BitchatMessageParser parser;
  ParsedBitchatMessage result;
  bool parsed = parser.parseMessage(data, sizeof(data), result);

  // May succeed or fail depending on parser requirements
  // Just ensure no crash
  (void)parsed;
}

} // namespace bitchat
} // namespace test

#endif // ENABLE_BITCHAT
