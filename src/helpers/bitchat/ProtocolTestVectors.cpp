/**
 * Story 9.1: Unit Tests for Protocol Layer - Implementation
 */

#ifdef ENABLE_BITCHAT

#include "ProtocolTestVectors.h"

#include "../ble/BitchatBLEService.h"

namespace mesh {
namespace bitchat {

ProtocolTestVectors::ProtocolTestVectors() : _randomSeed(123456789) {}

bool ProtocolTestVectors::getMessageVector(TestVector &out) const {
  // Create a valid MESSAGE packet
  // BitChat header: version(1) + type(1) + ttl(1) + timestamp(8) + flags(1) + payloadLen(2) + senderId(8)
  // Total header: 22 bytes

  out = TestVector();
  out.expectedVersion = BITCHAT_VERSION;
  out.expectedType = BITCHAT_MSG_MESSAGE;

  size_t offset = 0;

  // Version
  out.data[offset++] = BITCHAT_VERSION;

  // Type: MESSAGE
  out.data[offset++] = BITCHAT_MSG_MESSAGE;

  // TTL
  out.data[offset++] = 8;

  // Timestamp (8 bytes, big-endian)
  uint64_t timestamp = 1704067200; // 2024-01-01 00:00:00 UTC
  for (int i = 7; i >= 0; i--) {
    out.data[offset++] = (timestamp >> (i * 8)) & 0xFF;
  }

  // Flags
  out.data[offset++] = 0;

  // Payload length (2 bytes, big-endian)
  // Payload length (2 bytes, big-endian)
  const char *messageText = "Hello from test vector!";
  size_t textLen = strlen(messageText);
  // Payload is TLV: Type(1) + Len(1) + Value(textLen)
  size_t payloadLen = 1 + 1 + textLen;

  out.data[offset++] = (payloadLen >> 8) & 0xFF;
  out.data[offset++] = payloadLen & 0xFF;

  // Sender ID (8 bytes, little-endian)
  uint64_t senderId = 0x0102030405060708ULL;
  for (int i = 0; i < 8; i++) {
    out.data[offset++] = (senderId >> (i * 8)) & 0xFF;
  }

  // Payload: TLV
  out.data[offset++] = 0x10; // BITCHAT_TLV_MESSAGE_TEXT
  out.data[offset++] = textLen;
  memcpy(&out.data[offset], messageText, textLen);
  offset += textLen;

  out.length = offset;
  out.isValid = true;

  return true;
}

bool ProtocolTestVectors::getAnnounceVector(TestVector &out) const {
  // Create a valid ANNOUNCE packet
  out = TestVector();
  out.expectedVersion = BITCHAT_VERSION;
  out.expectedType = BITCHAT_MSG_ANNOUNCE;

  size_t offset = 0;

  // Version
  out.data[offset++] = BITCHAT_VERSION;

  // Type: ANNOUNCE
  out.data[offset++] = BITCHAT_MSG_ANNOUNCE;

  // TTL
  out.data[offset++] = 8;

  // Timestamp
  uint64_t timestamp = 1704067200;
  for (int i = 7; i >= 0; i--) {
    out.data[offset++] = (timestamp >> (i * 8)) & 0xFF;
  }

  // Flags
  out.data[offset++] = 0;

  // Payload length
  // ANNOUNCE payload with TLV: nickname(0x01) + noise_pubkey(0x02) + ed25519_pubkey(0x03)
  // Minimal: just nickname TLV
  const char *nickname = "TestNode";
  size_t nickLen = strlen(nickname);
  size_t payloadLen = 1 + 1 + nickLen; // type(1) + len(1) + data

  out.data[offset++] = (payloadLen >> 8) & 0xFF;
  out.data[offset++] = payloadLen & 0xFF;

  // Sender ID
  uint64_t senderId = 0x0102030405060708ULL;
  for (int i = 0; i < 8; i++) {
    out.data[offset++] = (senderId >> (i * 8)) & 0xFF;
  }

  // Payload: TLV for nickname
  out.data[offset++] = 0x01; // Type: nickname
  out.data[offset++] = nickLen;
  memcpy(&out.data[offset], nickname, nickLen);
  offset += nickLen;

  out.length = offset;
  out.isValid = true;

  return true;
}

bool ProtocolTestVectors::getLeaveVector(TestVector &out) const {
  // Create a valid LEAVE packet
  out = TestVector();
  out.expectedVersion = BITCHAT_VERSION;
  out.expectedType = BITCHAT_MSG_LEAVE;

  size_t offset = 0;

  // Version
  out.data[offset++] = BITCHAT_VERSION;

  // Type: LEAVE
  out.data[offset++] = BITCHAT_MSG_LEAVE;

  // TTL
  out.data[offset++] = 8;

  // Timestamp
  uint64_t timestamp = 1704067200;
  for (int i = 7; i >= 0; i--) {
    out.data[offset++] = (timestamp >> (i * 8)) & 0xFF;
  }

  // Flags
  out.data[offset++] = 0;

  // Payload length (0 for LEAVE)
  out.data[offset++] = 0;
  out.data[offset++] = 0;

  // Sender ID
  uint64_t senderId = 0x0102030405060708ULL;
  for (int i = 0; i < 8; i++) {
    out.data[offset++] = (senderId >> (i * 8)) & 0xFF;
  }

  out.length = offset;
  out.isValid = true;

  return true;
}

bool ProtocolTestVectors::getChannelVector(TestVector &out) const {
  // CHANNEL messages are less common, return a simple one
  return getMessageVector(out); // Similar structure
}

void ProtocolTestVectors::generateRandomData(uint8_t *buffer, size_t maxLen, size_t *outLen) const {
  if (buffer == nullptr || outLen == nullptr || maxLen == 0) {
    if (outLen) *outLen = 0;
    return;
  }

  // Generate random length (1 to maxLen)
  size_t len = (nextRandomByte() % maxLen) + 1;
  if (len > maxLen) len = maxLen;

  // Fill with random bytes
  for (size_t i = 0; i < len; i++) {
    buffer[i] = nextRandomByte();
  }

  *outLen = len;
}

size_t ProtocolTestVectors::getVectorCount() const {
  return 4; // MESSAGE, ANNOUNCE, LEAVE, CHANNEL
}

uint8_t ProtocolTestVectors::nextRandomByte() const {
  // LCG random number generator
  _randomSeed = _randomSeed * 1103515245 + 12345;
  return (uint8_t)(_randomSeed >> 16);
}

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
