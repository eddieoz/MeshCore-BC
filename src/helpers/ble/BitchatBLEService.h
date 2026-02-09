#pragma once

/**
 * Story 1.3: BitChat BLE Service Addition
 *
 * BitChat BLE Service - Implements BLEServiceWrapper for BitChat protocol
 *
 * Provides:
 * - BitChat UUID (F47B5E2D-4A9E-4C5A-9B3F-8E1D2C3A4B5C)
 * - Open security (no PIN)
 * - BitChat message parsing and generation
 * - Peer discovery and caching
 * - Coexistence with MeshCoreUARTService
 *
 * Story 2.3: Compile-time control via ENABLE_BITCHAT flag
 */

#ifdef ENABLE_BITCHAT

#include "SharedBLEServer.h"

#include <cstring>
#include <memory>
#include <queue>

// BitChat Protocol Constants
#define BITCHAT_HEADER_SIZE       14
#define BITCHAT_SIGNATURE_SIZE    64
#define BITCHAT_SENDER_ID_SIZE    8
#define BITCHAT_RECIPIENT_ID_SIZE 8
#define BITCHAT_MAX_PAYLOAD_SIZE  2048
#define BITCHAT_VERSION           1

// BitChat Message Types
enum BitchatMessageType : uint8_t {
  BITCHAT_MSG_ANNOUNCE = 0x01,
  BITCHAT_MSG_MESSAGE = 0x02,
  BITCHAT_MSG_LEAVE = 0x03,
  BITCHAT_MSG_IDENTITY = 0x04,
  BITCHAT_MSG_CHANNEL = 0x05,
  BITCHAT_MSG_PING = 0x06,
  BITCHAT_MSG_PONG = 0x07,
  BITCHAT_MSG_NOISE_HANDSHAKE = 0x10,
  BITCHAT_MSG_NOISE_ENCRYPTED = 0x11,
  BITCHAT_MSG_FRAGMENT_NEW = 0x20,
  BITCHAT_MSG_REQUEST_SYNC = 0x21,
  BITCHAT_MSG_FILE_TRANSFER = 0x22,
  BITCHAT_MSG_FRAGMENT = 0xFF
};

// BitChat Flags
#define BITCHAT_FLAG_HAS_RECIPIENT 0x01
#define BITCHAT_FLAG_HAS_SIGNATURE 0x02
#define BITCHAT_FLAG_IS_COMPRESSED 0x04

namespace mesh {
namespace ble {

/**
 * BitChat Message Structure
 */
struct BitchatMessage {
  uint8_t version;
  uint8_t type;
  uint8_t ttl;
  uint64_t timestamp;
  uint8_t flags;
  uint16_t payloadLength;
  uint16_t wirePayloadLength;
  uint8_t senderId[BITCHAT_SENDER_ID_SIZE];
  uint8_t recipientId[BITCHAT_RECIPIENT_ID_SIZE];
  uint8_t payload[BITCHAT_MAX_PAYLOAD_SIZE];
  uint8_t signature[BITCHAT_SIGNATURE_SIZE];

  BitchatMessage()
      : version(BITCHAT_VERSION), type(0), ttl(0), timestamp(0), flags(0), payloadLength(0),
        wirePayloadLength(0) {
    memset(senderId, 0, sizeof(senderId));
    memset(recipientId, 0, sizeof(recipientId));
    memset(payload, 0, sizeof(payload));
    memset(signature, 0, sizeof(signature));
  }

  bool hasRecipient() const { return (flags & BITCHAT_FLAG_HAS_RECIPIENT) != 0; }
  bool hasSignature() const { return (flags & BITCHAT_FLAG_HAS_SIGNATURE) != 0; }
  bool isCompressed() const { return (flags & BITCHAT_FLAG_IS_COMPRESSED) != 0; }

  void setHasRecipient(bool val) {
    if (val)
      flags |= BITCHAT_FLAG_HAS_RECIPIENT;
    else
      flags &= ~BITCHAT_FLAG_HAS_RECIPIENT;
  }

  void setHasSignature(bool val) {
    if (val)
      flags |= BITCHAT_FLAG_HAS_SIGNATURE;
    else
      flags &= ~BITCHAT_FLAG_HAS_SIGNATURE;
  }

  uint64_t getSenderId64() const {
    uint64_t id = 0;
    for (int i = 0; i < 8; i++) {
      id |= (static_cast<uint64_t>(senderId[i]) << (i * 8));
    }
    return id;
  }

  void setSenderId64(uint64_t id) {
    for (int i = 0; i < 8; i++) {
      senderId[i] = static_cast<uint8_t>((id >> (i * 8)) & 0xFF);
    }
  }

  uint64_t getRecipientId64() const {
    uint64_t id = 0;
    for (int i = 0; i < 8; i++) {
      id |= (static_cast<uint64_t>(recipientId[i]) << (i * 8));
    }
    return id;
  }

  void setRecipientId64(uint64_t id) {
    for (int i = 0; i < 8; i++) {
      recipientId[i] = static_cast<uint8_t>((id >> (i * 8)) & 0xFF);
    }
  }
};

/**
 * Peer information cache entry
 */
struct PeerInfo {
  uint64_t peerId;
  char nickname[32];
  uint8_t ed25519PubKey[32];
  uint32_t lastSeen;
  bool valid;

  PeerInfo() : peerId(0), lastSeen(0), valid(false) {
    memset(nickname, 0, sizeof(nickname));
    memset(ed25519PubKey, 0, sizeof(ed25519PubKey));
  }
};

/**
 * BitChat BLE Service
 */
class BitchatBLEService : public BLEServiceWrapper {
public:
  static constexpr size_t MAX_MESSAGE_QUEUE = 8;
  static constexpr size_t PEER_CACHE_SIZE = 32;
  static constexpr uint32_t ANNOUNCE_INTERVAL_MS = 5000;

  BitchatBLEService();
  ~BitchatBLEService() override;

  // Configuration
  void setNodeName(const char *name);
  void setPeerId(uint64_t peerId);
  const char *getNodeName() const;
  uint64_t getPeerId() const;

  // Message handling
  bool hasReceivedMessages() const;
  size_t getMessageQueueSize() const;
  bool getNextMessage(BitchatMessage &msg);
  void clearMessageQueue();

  // Peer cache
  size_t getPeerCacheSize() const;
  bool getPeerById(uint64_t peerId, PeerInfo &info) const;

  // Announcement
  void sendAnnouncement();
  bool wasAnnouncementSent() const;

  // Send message to BitChat clients (MeshCore → BitChat)
  bool sendMessageToClients(const char *senderName, const char *text, uint64_t timestamp);

  /**
   * @brief Send a raw BitChat message to connected clients
   * @param msg The message to send
   * @return true if sent successfully
   */
  bool sendBitchatMessage(const BitchatMessage &msg);

  // BLEServiceWrapper implementation
  bool onServiceRegistered(void *platformService) override;
  void onServiceUnregistered() override;
  void onClientConnect(const ConnectionInfo &connInfo) override;
  void onClientDisconnect(uint16_t connectionId) override;
  void onDataReceived(uint16_t connectionId, const uint8_t *data, size_t len) override;
  void onDataSent(uint16_t connectionId, size_t len) override;
  bool sendNotification(uint16_t connectionId, const uint8_t *data, size_t len) override;
  bool broadcastNotification(const uint8_t *data, size_t len) override;
  bool hasConnectedClients() const override;
  size_t getConnectedClientCount() const override;

  // Main loop - call periodically
  void loop();

#ifdef NRF52_PLATFORM
  /**
   * @brief Initialize nRF52 Bluefruit BLE service
   * This must be called AFTER Bluefruit.begin() has been called
   * (typically by SerialBLEInterface)
   * @param deviceName Device name for advertising
   * @return true if initialization successful
   */
  bool initNRF52(const char *deviceName);

  /**
   * @brief Get the nRF52 BLEService object for advertising
   * @return Reference to the BLEService object
   */
  BLEService &getNRF52Service() { return _service; }
#endif

// Test helpers
#ifdef NATIVE_TEST
  void queueMessageForTest(const BitchatMessage &msg);
#endif

private:
  // Static instance for nRF52 callback routing
  static BitchatBLEService *_instance;

  // Static callbacks for SerialBLEInterface
  static void onConnect(uint16_t conn_handle);
  static void onDisconnect(uint16_t conn_handle, uint8_t reason);

  // CRITICAL: Global processing lock to prevent BLE callback reentrance during send operations
  // The nRF52 softdevice can trigger receive callbacks during BLE operations, causing crashes.
  // This lock prevents the receive callback from processing while we're sending.
  static volatile bool _sendingInProgress;

  // Configuration
  char _nodeName[32];
  uint64_t _peerId;

  // State
  bool _registered;
  std::vector<ConnectionInfo> _connections;

  // Message queue
  std::queue<BitchatMessage> _recvQueue;

  // TX buffers (avoid stack usage)
  BitchatMessage _tempTxMessage;
  uint8_t _tempTxBuffer[BITCHAT_MAX_PAYLOAD_SIZE + 100];

  // RX Buffer for deferred processing (Reference Implementation Pattern)
  // Incoming BLE writes are appended here in ISR, processed in loop()
  BitchatMessage _tempRxMessage;
  uint8_t _writeBuffer[1024];
  size_t _writeBufferOffset;
  uint8_t _readBuffer[1024]; // Buffer for processing in loop (to avoid race with ISR)
  volatile bool _pendingData;
  uint32_t _lastWriteTime;

  // Connection state tracking (Reference Implementation Pattern)
  uint8_t _bitchatClientCount;
  bool _clientSubscribed;
  BitchatMessage _pendingOutgoing;
  bool _hasPendingOutgoing;
  uint32_t _connectionTime; // Time when last client connected (for stability delay)

  // Peer cache
  PeerInfo _peerCache[PEER_CACHE_SIZE];

  // Announcement timing
  uint32_t _lastAnnounceTime;
  bool _announcementSent;

  // Platform handles
#ifdef ESP32
  BLEService *_platformService;
  BLECharacteristic *_characteristic;
#elif defined(NRF52_PLATFORM)
  BLEService _service;
  BLECharacteristic _characteristic;
#endif

  // Internal helpers
  void clearWriteBuffer();
  void parseAndQueueMessage(const uint8_t *data, size_t len);
  void processAnnounce(const BitchatMessage &msg);
  void cachePeer(uint64_t peerId, const char *nickname, const uint8_t *pubKey);

  // Reference implementation helpers
  void sendPendingOutgoing();
  static void onCharacteristicCccdWrite(uint16_t conn_handle, BLECharacteristic *chr, uint16_t cccd_value);
  size_t serializeMessage(const BitchatMessage &msg, uint8_t *buffer, size_t maxLen);
  bool parseMessage(const uint8_t *data, size_t len, BitchatMessage &msg);
  bool parseAnnounceTLV(const uint8_t *payload, size_t len, char *nickname, size_t nickLen, uint8_t *pubKey);
};

} // namespace ble
} // namespace mesh

#endif // ENABLE_BITCHAT
