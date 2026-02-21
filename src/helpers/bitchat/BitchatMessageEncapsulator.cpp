#include "BitchatMessageEncapsulator.h"

#ifdef ENABLE_BITCHAT

namespace mesh {
namespace bitchat {

BitchatMessageEncapsulator::BitchatMessageEncapsulator() : _nextMessageId(1) {}

bool BitchatMessageEncapsulator::encapsulate(const BitchatMessage &msg, const uint8_t *channelKey,
                                             EncapsulatedPacket &outPacket) {
  if (!channelKey) {
    return false;
  }

  // Serialize the BitChat message
  size_t serializedLen = serializeMessage(msg, _serializationBuffer, sizeof(_serializationBuffer));
  if (serializedLen == 0) {
    return false;
  }

  // Check if fragmentation needed
  if (needsFragmentation(serializedLen)) {
    return false; // Use encapsulateWithFragmentation instead
  }

  // Build header
  uint8_t flags = 0;
  size_t headerLen = buildHeader(outPacket.data, sizeof(outPacket.data), static_cast<uint16_t>(serializedLen),
                                 flags, _nextMessageId++, 0, 1);

  if (headerLen == 0) {
    return false;
  }

  // Encrypt payload
  encryptPayload(_serializationBuffer, serializedLen, channelKey, &outPacket.data[headerLen]);

  outPacket.length = headerLen + serializedLen;
  outPacket.isValid = true;
  outPacket.isFragment = false;
  outPacket.fragmentNum = 0;
  outPacket.totalFragments = 1;
  outPacket.messageId = _nextMessageId - 1;
  outPacket.payloadType = MeshCorePayloadType::MESHCORE_GRP_DATA;

  return true;
}

bool BitchatMessageEncapsulator::encapsulateWithFragmentation(const BitchatMessage &msg,
                                                              const uint8_t *channelKey,
                                                              EncapsulatedPacket *outPackets,
                                                              size_t maxPackets, size_t &outPacketCount) {

  if (!channelKey || !outPackets || maxPackets == 0) {
    return false;
  }

  // Serialize the message
  size_t serializedLen = serializeMessage(msg, _serializationBuffer, sizeof(_serializationBuffer));
  if (serializedLen == 0) {
    return false;
  }

  // Calculate fragments needed
  size_t fragCount = calculateFragmentCount(serializedLen);
  if (fragCount > maxPackets) {
    return false; // Not enough buffer space
  }

  uint32_t msgId = _nextMessageId++;
  size_t bytesPerFragment =
      BITCHAT_MAX_ENCAPSULATED_SIZE - BITCHAT_ENCAPSULATION_HEADER_SIZE - 6; // Extra header for frag info
  size_t offset = 0;

  for (size_t fragNum = 0; fragNum < fragCount; fragNum++) {
    size_t remaining = serializedLen - offset;
    size_t thisFragmentSize = (remaining > bytesPerFragment) ? bytesPerFragment : remaining;

    EncapsulatedPacket &pkt = outPackets[fragNum];

    // Build header with fragmentation flag
    uint8_t flags = 0x01; // Fragment flag
    size_t headerLen = buildHeader(pkt.data, sizeof(pkt.data), static_cast<uint16_t>(serializedLen), flags,
                                   msgId, static_cast<uint8_t>(fragNum), static_cast<uint8_t>(fragCount));

    if (headerLen == 0) {
      return false;
    }

    // Encrypt this fragment
    encryptPayload(&_serializationBuffer[offset], thisFragmentSize, channelKey, &pkt.data[headerLen]);

    pkt.length = headerLen + thisFragmentSize;
    pkt.isValid = true;
    pkt.isFragment = true;
    pkt.fragmentNum = static_cast<uint8_t>(fragNum);
    pkt.totalFragments = static_cast<uint8_t>(fragCount);
    pkt.messageId = msgId;
    pkt.payloadType = MeshCorePayloadType::MESHCORE_GRP_DATA;

    offset += thisFragmentSize;
  }

  outPacketCount = fragCount;
  return true;
}

bool BitchatMessageEncapsulator::decapsulate(const EncapsulatedPacket &packet, const uint8_t *channelKey,
                                             BitchatMessage &msgOut) {
  if (!packet.isValid || !channelKey || packet.length < BITCHAT_ENCAPSULATION_HEADER_SIZE) {
    return false;
  }

  // Verify magic
  uint32_t magic = (static_cast<uint32_t>(packet.data[0]) << 24) |
                   (static_cast<uint32_t>(packet.data[1]) << 16) |
                   (static_cast<uint32_t>(packet.data[2]) << 8) | packet.data[3];

  if (magic != BITCHAT_ENCAPSULATION_MAGIC) {
    return false;
  }

  // Verify version
  if (packet.data[4] != BITCHAT_ENCAPSULATION_VERSION) {
    return false;
  }

  // Get original length
  uint16_t originalLength = (static_cast<uint16_t>(packet.data[6]) << 8) | packet.data[7];

  // Get message ID and fragment info
  uint32_t msgId = (static_cast<uint32_t>(packet.data[8]) << 24) |
                   (static_cast<uint32_t>(packet.data[9]) << 16) |
                   (static_cast<uint32_t>(packet.data[10]) << 8) | packet.data[11];
  (void)msgId; // Used for reassembly in future

  uint8_t fragNum = packet.data[12];
  uint8_t totalFrags = packet.data[13];
  (void)fragNum; // Used for reassembly
  (void)totalFrags;

  // Decrypt payload
  size_t payloadLen = packet.length - BITCHAT_ENCAPSULATION_HEADER_SIZE;
  decryptPayload(&packet.data[BITCHAT_ENCAPSULATION_HEADER_SIZE], payloadLen, channelKey, _decryptionBuffer);

  // For single-fragment messages, deserialize directly
  // For multi-fragment, would need reassembly (not fully implemented)
  if (!packet.isFragment || totalFrags == 1) {
    // Inline deserialization
    if (originalLength < BITCHAT_HEADER_SIZE) {
      return false;
    }

    size_t offset = 0;

    msgOut.version = _decryptionBuffer[offset++];
    msgOut.type = _decryptionBuffer[offset++];
    msgOut.ttl = _decryptionBuffer[offset++];

    // Timestamp (8 bytes, big-endian)
    msgOut.timestamp = 0;
    for (int i = 0; i < 8; i++) {
      msgOut.timestamp = (msgOut.timestamp << 8) | _decryptionBuffer[offset++];
    }

    msgOut.flags = _decryptionBuffer[offset++];

    // Payload length
    msgOut.payloadLength =
        (static_cast<uint16_t>(_decryptionBuffer[offset]) << 8) | _decryptionBuffer[offset + 1];

    offset += 2;

    // Sender ID (8 bytes, little-endian)
    uint64_t senderId = 0;
    for (int i = 0; i < 8; i++) {
      senderId |= (static_cast<uint64_t>(_decryptionBuffer[offset + i]) << (i * 8));
    }
    msgOut.setSenderId64(senderId);
    offset += 8;

    // Payload
    if (msgOut.payloadLength > 0) {
      if (offset + msgOut.payloadLength > originalLength || msgOut.payloadLength > BITCHAT_MAX_PAYLOAD_SIZE) {
        return false;
      }
      memcpy(msgOut.payload, &_decryptionBuffer[offset], msgOut.payloadLength);
    }

    return true;
  }

  // Multi-fragment reassembly not yet implemented
  // Would need to buffer fragments and reassemble
  return false;
}

bool BitchatMessageEncapsulator::needsFragmentation(size_t messageSize) {
  // If message + header > max size, needs fragmentation
  return (messageSize + BITCHAT_ENCAPSULATION_HEADER_SIZE) > BITCHAT_MAX_ENCAPSULATED_SIZE;
}

size_t BitchatMessageEncapsulator::calculateFragmentCount(size_t messageSize) {
  size_t payloadPerFragment = BITCHAT_MAX_ENCAPSULATED_SIZE - BITCHAT_ENCAPSULATION_HEADER_SIZE;
  return (messageSize + payloadPerFragment - 1) / payloadPerFragment;
}

size_t BitchatMessageEncapsulator::buildHeader(uint8_t *buffer, size_t bufferSize, uint16_t originalLength,
                                               uint8_t flags, uint32_t msgId, uint8_t fragNum,
                                               uint8_t totalFrags) {
  if (bufferSize < BITCHAT_ENCAPSULATION_HEADER_SIZE) {
    return 0;
  }

  size_t offset = 0;

  // Magic: "BC\x00\x00"
  buffer[offset++] = 'B';
  buffer[offset++] = 'C';
  buffer[offset++] = 0x00;
  buffer[offset++] = 0x00;

  // Version
  buffer[offset++] = BITCHAT_ENCAPSULATION_VERSION;

  // Flags
  buffer[offset++] = flags;

  // Original length (2 bytes, big-endian)
  buffer[offset++] = (originalLength >> 8) & 0xFF;
  buffer[offset++] = originalLength & 0xFF;

  // Message ID (4 bytes, big-endian)
  buffer[offset++] = (msgId >> 24) & 0xFF;
  buffer[offset++] = (msgId >> 16) & 0xFF;
  buffer[offset++] = (msgId >> 8) & 0xFF;
  buffer[offset++] = msgId & 0xFF;

  // Fragment number and total
  buffer[offset++] = fragNum;
  buffer[offset++] = totalFrags;

  return offset;
}

void BitchatMessageEncapsulator::encryptPayload(const uint8_t *input, size_t len, const uint8_t *key,
                                                uint8_t *output) {
  // Simple XOR encryption with key cycling
  // In production, replace with AES-CTR or ChaCha20
  for (size_t i = 0; i < len; i++) {
    output[i] = input[i] ^ key[i % 32];
  }
}

void BitchatMessageEncapsulator::decryptPayload(const uint8_t *input, size_t len, const uint8_t *key,
                                                uint8_t *output) {
  // XOR is symmetric
  encryptPayload(input, len, key, output);
}

size_t BitchatMessageEncapsulator::serializeMessage(const BitchatMessage &msg, uint8_t *buffer,
                                                    size_t bufferSize) {
  if (bufferSize < BITCHAT_HEADER_SIZE + msg.payloadLength) {
    return 0;
  }

  size_t offset = 0;

  // Header
  buffer[offset++] = msg.version;
  buffer[offset++] = msg.type;
  buffer[offset++] = msg.ttl;

  // Timestamp (8 bytes, big-endian)
  for (int i = 7; i >= 0; i--) {
    buffer[offset++] = (msg.timestamp >> (i * 8)) & 0xFF;
  }

  // Flags
  buffer[offset++] = msg.flags;

  // Payload length (2 bytes, big-endian)
  buffer[offset++] = (msg.payloadLength >> 8) & 0xFF;
  buffer[offset++] = msg.payloadLength & 0xFF;

  // Sender ID (8 bytes, little-endian)
  for (int i = 0; i < 8; i++) {
    buffer[offset++] = (msg.getSenderId64() >> (i * 8)) & 0xFF;
  }

  // Payload
  if (msg.payloadLength > 0) {
    memcpy(&buffer[offset], msg.payload, msg.payloadLength);
    offset += msg.payloadLength;
  }

  return offset;
}

size_t BitchatMessageEncapsulator::serializeForSigning(const BitchatMessage &msg, uint8_t *buffer,
                                                       size_t bufferSize) {
  if (bufferSize < BITCHAT_HEADER_SIZE + msg.payloadLength) {
    return 0;
  }

  size_t offset = 0;

  buffer[offset++] = msg.version;
  buffer[offset++] = msg.type;
  buffer[offset++] = 0; // Force TTL to 0 to match Android AppConstants.SYNC_TTL_HOPS

  // Timestamp (Big Endian)
  for (int i = 7; i >= 0; i--) {
    buffer[offset++] = static_cast<uint8_t>((msg.timestamp >> (i * 8)) & 0xFF);
  }

  // Flags: Mask out HAS_SIGNATURE
  buffer[offset++] = msg.flags & ~BITCHAT_FLAG_HAS_SIGNATURE;

  // Payload Length (Big Endian)
  buffer[offset++] = static_cast<uint8_t>((msg.payloadLength >> 8) & 0xFF);
  buffer[offset++] = static_cast<uint8_t>(msg.payloadLength & 0xFF);

  // Sender ID (big-endian for BitChat protocol compatibility)
  for (int i = 7; i >= 0; i--) {
    buffer[offset++] = msg.senderId[i];
  }

  // CRITICAL: Aligned with iOS/Android - recipientID field (8 bytes) is ONLY present if flag is set
  if (msg.hasRecipient()) {
    for (int i = 7; i >= 0; i--) {
      buffer[offset++] = msg.recipientId[i];
    }
  }

  if (msg.payloadLength > 0) {
    memcpy(&buffer[offset], msg.payload, msg.payloadLength);
    offset += msg.payloadLength;
  }

  return offset;
}

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
