/**
 * Story 6.1: MeshCore Group Message Decapsulation - Implementation
 */

#ifdef ENABLE_BITCHAT

#include "MessageDecapsulator.h"

#include "../ble/BitchatBLEService.h"

#include <cstring>

namespace mesh {
namespace bitchat {

MessageDecapsulator::MessageDecapsulator(ChannelRegistry &registry)
    : _registry(registry), _lastStatus(DecapsulationStatus::NOT_BITCHAT) {}

bool MessageDecapsulator::decapsulate(const EncapsulatedPacket &packet, const uint8_t *channelKey,
                                      DecapsulationResult &result) {
  // Reset result
  result = DecapsulationResult();

  // Validate packet
  if (!packet.isValid || packet.length < BITCHAT_ENCAPSULATION_HEADER_SIZE) {
    _lastStatus = DecapsulationStatus::INVALID_PACKET;
    result.status = _lastStatus;
    return false;
  }

  // Check for BitChat magic header
  if (!isBitChatEncapsulated(packet.data, packet.length)) {
    _lastStatus = DecapsulationStatus::NOT_BITCHAT;
    result.status = _lastStatus;
    return false;
  }

  // Verify version
  if (packet.data[4] != BITCHAT_ENCAPSULATION_VERSION) {
    _lastStatus = DecapsulationStatus::VERSION_MISMATCH;
    result.status = _lastStatus;
    return false;
  }

  // Extract header info
  uint16_t originalLength = (static_cast<uint16_t>(packet.data[6]) << 8) | packet.data[7];
  result.messageId = (static_cast<uint32_t>(packet.data[8]) << 24) |
                     (static_cast<uint32_t>(packet.data[9]) << 16) |
                     (static_cast<uint32_t>(packet.data[10]) << 8) | packet.data[11];
  result.fragmentNum = packet.data[12];
  result.totalFragments = packet.data[13];
  result.isFragment = (result.totalFragments > 1);

  // Decrypt payload
  size_t payloadLen = packet.length - BITCHAT_ENCAPSULATION_HEADER_SIZE;
  // Use member buffer to avoid stack overflow (~2KB)
  // uint8_t decrypted[BITCHAT_MAX_PAYLOAD_SIZE + BITCHAT_HEADER_SIZE];
  uint8_t *decrypted = _decryptionBuffer;

  if (payloadLen > sizeof(decrypted)) {
    payloadLen = sizeof(decrypted);
  }

  decryptPayload(&packet.data[BITCHAT_ENCAPSULATION_HEADER_SIZE], payloadLen, channelKey, decrypted);

  // Deserialize message
  if (!deserializeMessage(decrypted, originalLength, result.message)) {
    _lastStatus = DecapsulationStatus::DESERIALIZATION_FAILED;
    result.status = _lastStatus;
    return false;
  }

  _lastStatus = DecapsulationStatus::SUCCESS;
  result.status = _lastStatus;
  return true;
}

bool MessageDecapsulator::isBitChatEncapsulated(const uint8_t *data, size_t len) const {
  if (data == nullptr || len < 4) {
    return false;
  }

  // Check magic: "BC\x00\x00"
  return (data[0] == 'B' && data[1] == 'C' && data[2] == 0x00 && data[3] == 0x00);
}

void MessageDecapsulator::decryptPayload(const uint8_t *input, size_t len, const uint8_t *key,
                                         uint8_t *output) const {
  // Simple XOR decryption (same as encryption)
  for (size_t i = 0; i < len; i++) {
    output[i] = input[i] ^ key[i % 32];
  }
}

bool MessageDecapsulator::deserializeMessage(const uint8_t *data, size_t len,
                                             mesh::ble::BitchatMessage &msgOut) {
  if (data == nullptr || len < BITCHAT_HEADER_SIZE) {
    return false;
  }

  size_t offset = 0;

  // Version
  msgOut.version = data[offset++];

  // Type
  msgOut.type = data[offset++];

  // TTL
  msgOut.ttl = data[offset++];

  // Timestamp (8 bytes, big-endian)
  msgOut.timestamp = 0;
  for (int i = 0; i < 8; i++) {
    msgOut.timestamp = (msgOut.timestamp << 8) | data[offset++];
  }

  // Flags
  msgOut.flags = data[offset++];

  // Payload length
  msgOut.payloadLength = (static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1];
  offset += 2;

  // Sender ID (8 bytes, little-endian)
  uint64_t senderId = 0;
  for (int i = 0; i < 8; i++) {
    senderId |= (static_cast<uint64_t>(data[offset + i]) << (i * 8));
  }
  msgOut.setSenderId64(senderId);
  offset += 8;

  // Validate payload length
  if (msgOut.payloadLength > BITCHAT_MAX_PAYLOAD_SIZE) {
    return false;
  }

  // Check if we have enough data for payload
  if (offset + msgOut.payloadLength > len) {
    return false;
  }

  // Payload
  if (msgOut.payloadLength > 0) {
    memcpy(msgOut.payload, &data[offset], msgOut.payloadLength);
    offset += msgOut.payloadLength;
  }

  // Recipient ID (if HAS_RECIPIENT flag)
  if (msgOut.hasRecipient() && offset + 8 <= len) {
    uint64_t recipientId = 0;
    for (int i = 0; i < 8; i++) {
      recipientId |= (static_cast<uint64_t>(data[offset + i]) << (i * 8));
    }
    msgOut.setRecipientId64(recipientId);
    offset += 8;
  }

  // Signature (if HAS_SIGNATURE flag)
  if (msgOut.hasSignature() && offset + BITCHAT_SIGNATURE_SIZE <= len) {
    memcpy(msgOut.signature, &data[offset], BITCHAT_SIGNATURE_SIZE);
  }

  return true;
}

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
