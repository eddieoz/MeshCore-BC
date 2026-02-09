#pragma once

/**
 * Story 5.2: BitChat Message Encapsulation (BitChat → MeshCore)
 *
 * Encapsulates BitChat messages in MeshCore PAYLOAD_TYPE_GRP_DATA packets
 *
 * Encapsulation Format:
 * - Header (14 bytes): magic(4) + version(1) + flags(1) + orig_len(2) + msg_id(4) + frag(2)
 * - Encrypted BitChat message data
 *
 * Supports fragmentation for messages > 170 bytes
 */

#ifdef ENABLE_BITCHAT

#include "../ble/BitchatBLEService.h"

#include <cstdint>
#include <cstring>

namespace mesh {
namespace bitchat {

// Import BitchatMessage from ble namespace
using mesh::ble::BitchatMessage;

// Encapsulation header constants
#define BITCHAT_ENCAPSULATION_MAGIC       0x42430000 // "BC\x00\x00"
#define BITCHAT_ENCAPSULATION_VERSION     1
#define BITCHAT_ENCAPSULATION_HEADER_SIZE 14

// Maximum payload per MeshCore packet
#define BITCHAT_MAX_ENCAPSULATED_SIZE     170

// MeshCore payload types (mirror from MeshCore)
// Note: Names prefixed with MESHCORE_ to avoid macro collision with Packet.h
enum class MeshCorePayloadType : uint8_t {
  MESHCORE_TXT_MSG = 0x02, // Direct text message
  MESHCORE_GRP_DATA = 0x06 // Raw group data for encapsulation
};

/**
 * Encapsulated packet structure
 */
struct EncapsulatedPacket {
  uint8_t data[256];      // Full packet data
  size_t length;          // Total length
  bool isValid;           // Packet is valid
  bool isFragment;        // Is a fragment
  uint8_t fragmentNum;    // Fragment number (0-based)
  uint8_t totalFragments; // Total fragments
  uint32_t messageId;     // Message ID for reassembly
  MeshCorePayloadType payloadType;

  EncapsulatedPacket()
      : length(0), isValid(false), isFragment(false), fragmentNum(0), totalFragments(1), messageId(0),
        payloadType(MeshCorePayloadType::MESHCORE_GRP_DATA) {
    memset(data, 0, sizeof(data));
  }
};

/**
 * BitChat Message Encapsulator
 *
 * Encapsulates BitChat messages for MeshCore transmission
 * and decapsulates received packets back to BitChat messages
 */
class BitchatMessageEncapsulator {
public:
  BitchatMessageEncapsulator();

  // Encapsulate a single BitChat message
  // Returns true if encapsulation successful
  bool encapsulate(const BitchatMessage &msg, const uint8_t *channelKey, EncapsulatedPacket &outPacket);

  // Encapsulate with fragmentation (for large messages)
  // Returns true if all fragments created
  bool encapsulateWithFragmentation(const BitchatMessage &msg, const uint8_t *channelKey,
                                    EncapsulatedPacket *outPackets, size_t maxPackets,
                                    size_t &outPacketCount);

  // Decapsulate a received packet back to BitChat message
  bool decapsulate(const EncapsulatedPacket &packet, const uint8_t *channelKey, BitchatMessage &msgOut);

  // Check if message needs fragmentation
  static bool needsFragmentation(size_t messageSize);

  // Calculate number of fragments needed
  static size_t calculateFragmentCount(size_t messageSize);

private:
  uint32_t _nextMessageId;

  // Build encapsulation header
  size_t buildHeader(uint8_t *buffer, size_t bufferSize, uint16_t originalLength, uint8_t flags,
                     uint32_t msgId, uint8_t fragNum, uint8_t totalFrags);

  // Simple XOR encryption (placeholder for AES)
  void encryptPayload(const uint8_t *input, size_t len, const uint8_t *key, uint8_t *output);

  void decryptPayload(const uint8_t *input, size_t len, const uint8_t *key, uint8_t *output);

  // Serialize BitChat message to bytes
  size_t serializeMessage(const BitchatMessage &msg, uint8_t *buffer, size_t bufferSize);
};

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
