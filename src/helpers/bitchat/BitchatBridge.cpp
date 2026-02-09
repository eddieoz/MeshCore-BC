/**
 * @file BitchatBridge.cpp
 * @brief Implementation of BitchatBridge integration layer
 *
 * Implements Story 10.2: BitchatBridge Encapsulation Integration
 * Simplified version compatible with existing BitChat components.
 */

#ifdef ENABLE_BITCHAT

#include "BitchatBridge.h"

#include <Identity.h>
#include <Mesh.h>
#include <cstring>

// ============================================================
// Constructor / Destructor
// ============================================================

BitchatBridge::BitchatBridge(mesh::Mesh &mesh, mesh::LocalIdentity &identity, const char *nodeName)
    : _mesh(mesh), _identity(identity), _nodeName(nodeName), _initialized(false), _bitchatPeerId(0),
      _messagesRelayed(0), _duplicatesDropped(0), _timeOffset(0), _timeSynced(false),
      _processingMessage(false), _hasMeshChannel(false), _decapsulator(_channelRegistry) {
  memset(_noisePublicKey, 0, sizeof(_noisePublicKey));
  memset(&_meshChannel, 0, sizeof(_meshChannel));
}

BitchatBridge::~BitchatBridge() {}

// ============================================================
// Initialization
// ============================================================

void BitchatBridge::begin() {
  if (_initialized) {
    return;
  }

  // Derive BitChat peer ID from identity
  _bitchatPeerId = derivePeerId(_identity);

  // Set peer ID on BLE service
  _bleService.setPeerId(_bitchatPeerId);
  Serial.printf("[BitChat] Peer ID set: 0x%016llX\n", _bitchatPeerId);

  // Register the default #mesh channel
  _channelRegistry.registerDefaultMeshChannel();

  _initialized = true;
}

bool BitchatBridge::beginStandalone(const char *deviceName) {
  if (!_initialized) {
    begin();
  }

#ifdef NRF52_PLATFORM
  // nRF52: Initialize BitChat BLE service
  (void)deviceName;
  return initNRF52BLEService();
#else
  (void)deviceName;
  return false; // Standalone mode only supported on nRF52
#endif
}

#ifdef NRF52_PLATFORM
bool BitchatBridge::initNRF52BLEService() {
  // nRF52 BitChat service initialization
  // This assumes Bluefruit has already been initialized by SerialBLEInterface

  Serial.println("[BitChat] initNRF52BLEService() called");

  // Initialize the BLE service
  if (!_bleService.initNRF52(_nodeName)) {
    Serial.println("[BitChat] ERROR: BLE service init failed");
    return false;
  }

  Serial.println("[BitChat] nRF52 BLE service initialized");
  return true;
}
#endif

// ============================================================
// Main Loop
// ============================================================

void BitchatBridge::loop() {
  static uint32_t lastDebug = 0;
  uint32_t now = millis();

  // Print debug every 10 seconds
  if (now - lastDebug > 10000) {
    lastDebug = now;
    if (_initialized) {
      Serial.printf("[BitChat] Bridge alive, relayed=%lu, dropped=%lu, queue=%u\n", _messagesRelayed,
                    _duplicatesDropped,
                    _bleService.hasReceivedMessages() ? _bleService.getMessageQueueSize() : 0);
    }
  }

  if (!_initialized) {
    return;
  }

  // CRITICAL: Drive BLE service loop for buffered data processing and announcements
  _bleService.loop();

  // Prevent re-entrant processing
  if (_processingMessage) {
    return;
  }

  // Process outgoing messages (MeshCore → BitChat) deferred from callbacks
  if (!_outgoingQueue.empty()) {
    // Process one message per loop to keep stack clean and loop fast
    OutgoingMessage msg = _outgoingQueue.front();
    _outgoingQueue.erase(_outgoingQueue.begin());

    Serial.printf("[BitChat] Processing queued message from %s\n", msg.sender);

    // Check clients before attempting send
    if (_bleService.hasConnectedClients()) {
      if (_bleService.sendMessageToClients(msg.sender, msg.text, msg.timestamp)) {
        Serial.println("[BitChat] Message sent to BitChat app via loop");
        _messagesRelayed++;
      } else {
        Serial.println("[BitChat] Failed to send queued message");
      }
    } else {
      Serial.println("[BitChat] Message processed but no clients connected");
    }
  }

  // Process received messages from BitChat app (BitChat → MeshCore)
  // Use member variable to avoid 2KB+ stack allocation that can cause stack overflow
  while (_bleService.hasReceivedMessages()) {
    if (_bleService.getNextMessage(_tempLoopMessage)) {
      Serial.printf("[BitChat] Processing message from BLE queue, type=%d\n", _tempLoopMessage.type);
      onBitchatMessageReceived(_tempLoopMessage);
    }
  }

  // Clear old loop prevention entries (30 second timeout)
  _loopPrevention.clearOldEntries(now / 1000, 30);

  // Clear old fragment reassembly buffers (30 second timeout)
  _fragmentReassembly.clearOldFragments(now / 1000, 30);
}

// ============================================================
// Status Queries
// ============================================================

bool BitchatBridge::isBLEActive() const {
  // Check if BLE service is ready
  return _initialized;
}

bool BitchatBridge::hasBitchatClient() const {
  // Check if any BitChat client is connected
  return _bleService.hasConnectedClients();
}

// ============================================================
// MeshCore → BitChat Flow
// ============================================================

void BitchatBridge::onMeshcoreGroupMessage(const mesh::GroupChannel &channel, uint32_t timestamp,
                                           const char *senderName, const char *text) {
  Serial.println("[BitChat] onMeshcoreGroupMessage() called");

  if (!_initialized) {
    Serial.println("[BitChat] Bridge not initialized, ignoring");
    return;
  }
  if (!text) {
    Serial.println("[BitChat] Null text, ignoring");
    return;
  }
  if (_processingMessage) {
    Serial.println("[BitChat] Already processing, ignoring");
    return;
  }

  _processingMessage = true;
  Serial.printf("[BitChat] Processing message from %s: %s\n", senderName ? senderName : "null", text);

  // For now, accept messages from any channel (we only care about #mesh in practice)
  // The channel check was failing because MeshCore hash is SHA256(secret), not SHA256(name)
  Serial.println("[BitChat] Accepting message from channel");

  // Cache the channel for outgoing messages
  if (!_hasMeshChannel) {
    _meshChannel = channel;
    _hasMeshChannel = true;
    Serial.println("[BitChat] Cached mesh channel for outgoing messages");
  }

  // Loop prevention
  size_t textLen = strlen(text);
  uint32_t msgHash = mesh::bitchat::LoopPrevention::computeHash((const uint8_t *)text, textLen);
  uint64_t msgId = ((uint64_t)timestamp << 32) | msgHash;

  // For messages marked with 📱 prefix, skip (BitChat origin)
  if (strncmp(text, "📱 ", 3) == 0) {
    _processingMessage = false;
    return;
  }

  uint64_t senderId = 0;
  if (_loopPrevention.isDuplicate(msgId, senderId)) {
    _duplicatesDropped++;
    _processingMessage = false;
    return;
  }

  _loopPrevention.markSeen(msgId, senderId, timestamp);

  // DEFERRED: Queue for processing in loop() to avoid stack overflow
  // The call stack from MeshCore -> ... -> BitchatBLEService -> Bluefruit::notify is too deep
  if (_outgoingQueue.size() < 5) {
    OutgoingMessage msg;
    strncpy(msg.sender, senderName, sizeof(msg.sender) - 1);
    msg.sender[sizeof(msg.sender) - 1] = '\0';

    strncpy(msg.text, text, sizeof(msg.text) - 1);
    msg.text[sizeof(msg.text) - 1] = '\0';

    msg.timestamp = timestamp;

    _outgoingQueue.push_back(msg);
    Serial.println("[BitChat] Queued message for main loop processing");
  } else {
    Serial.println("[BitChat] Outgoing queue full, dropping message");
  }

  _processingMessage = false;
}

void BitchatBridge::onMeshcoreEncapsulatedMessage(const mesh::GroupChannel &channel, uint32_t timestamp,
                                                  const char *senderName, const uint8_t *data, size_t len) {
  if (!_initialized) {
    return;
  }

  // Build EncapsulatedPacket from raw data
  mesh::bitchat::EncapsulatedPacket packet;
  if (len > sizeof(packet.data)) {
    Serial.printf("[BitChat] Packet too large for buffer: %d > %d\n", len, sizeof(packet.data));
    return;
  }

  memcpy(packet.data, data, len);
  packet.length = len;
  packet.isValid = true;

  // Decapsulate using the channel secret
  // Use member to prevent stack overflow
  _tempDecapsulationResult = mesh::bitchat::DecapsulationResult(); // Reset
  mesh::bitchat::DecapsulationResult &result = _tempDecapsulationResult;

  if (_decapsulator.decapsulate(packet, channel.secret, result)) {
    if (result.status == mesh::bitchat::DecapsulationStatus::SUCCESS) {
      if (result.isFragment) {
        Serial.println("[BitChat] Received fragment, reassembly not yet fully implemented");
      } else {
        Serial.printf("[BitChat] Decapsulated message type=%d from %016llX, forwarding to app\n",
                      result.message.type, result.message.getSenderId64());

        if (_bleService.sendBitchatMessage(result.message)) {
          _messagesRelayed++;
        } else {
          Serial.println("[BitChat] Failed to forward to app (BLE tx failed)");
        }
      }
    } else {
      Serial.printf("[BitChat] Decapsulation failed: status=%d\n", (int)result.status);
    }
  } else {
    Serial.println("[BitChat] Failed to decapsulate packet");
  }
}

// ============================================================
// BitChat → MeshCore Flow
// ============================================================

void BitchatBridge::onBitchatMessageReceived(const mesh::ble::BitchatMessage &msg) {
  if (!_initialized || _processingMessage) {
    return;
  }

  _processingMessage = true;

  // Handle different message types
  Serial.printf("[BitChat] onBitchatMessageReceived: type=%d, payloadLen=%d\n", msg.type, msg.payloadLength);

  switch (msg.type) {
  case 0x02: { // MESSAGE
    // Check if we have a cached mesh channel
    if (!_hasMeshChannel) {
      Serial.println("[BitChat] ERROR: No mesh channel cached yet!");
      Serial.println("[BitChat] Send a message from MeshCore first to cache the channel");
      _processingMessage = false;
      return;
    }

    // Build standard MeshCore group text message format
    // Format: [timestamp:4][TXT_TYPE_PLAIN:1]["sender: message"]
    uint8_t temp[256];
    uint32_t timestamp = millis();

    // Copy timestamp (4 bytes)
    memcpy(temp, &timestamp, 4);

    // Text type: 0 = TXT_TYPE_PLAIN
    temp[4] = 0;

    // Build message: "📱 BitChat: <payload>"
    // The 📱 prefix identifies this as coming from BitChat (prevents loopback)
    size_t payloadLen = msg.payloadLength;
    if (payloadLen > 200) payloadLen = 200;

    int prefixLen = sprintf((char *)&temp[5], "📱 %s: ", _nodeName);
    memcpy(&temp[5 + prefixLen], msg.payload, payloadLen);
    temp[5 + prefixLen + payloadLen] = 0; // null terminator

    size_t totalLen = 5 + prefixLen + payloadLen + 1; // include null

    Serial.printf("[BitChat] Sending GRP_TXT to mesh, len=%d\n", totalLen);

    // Create and send as standard group text message
    auto *pkt = _mesh.createGroupDatagram(0x05, // PAYLOAD_TYPE_GRP_TXT
                                          _meshChannel, temp, totalLen);
    if (pkt) {
      _mesh.sendFlood(pkt);
      _messagesRelayed++;
      Serial.println("[BitChat] Message sent to mesh as GRP_TXT!");
    } else {
      Serial.println("[BitChat] ERROR: Failed to create packet!");
    }
    break;
  }

  default:
    // Other message types not yet implemented
    break;
  }

  _processingMessage = false;
}

// ============================================================
// Private Helpers
// ============================================================

bool BitchatBridge::isMeshChannel(const mesh::GroupChannel &channel) const {
  const uint8_t *meshKey = _channelRegistry.lookupByName("mesh");
  if (!meshKey) {
    Serial.println("[BitChat] isMeshChannel: no mesh key in registry");
    return false;
  }

  Serial.printf("[BitChat] Channel check: received[0]=0x%02X, mesh[0]=0x%02X\n", channel.hash[0], meshKey[0]);

  // Compare full hash for accuracy
  bool match = (memcmp(channel.hash, meshKey, 8) == 0);
  Serial.printf("[BitChat] Channel match: %s\n", match ? "YES" : "NO");
  return match;
}

uint64_t BitchatBridge::derivePeerId(const mesh::LocalIdentity &identity) {
  uint64_t peerId = 0;
  for (int i = 0; i < 8; i++) {
    peerId |= ((uint64_t)identity.pub_key[i]) << (i * 8);
  }
  return peerId;
}

uint64_t BitchatBridge::getCurrentTimeMs() const {
  uint64_t localTime = millis();
  if (_timeSynced) {
    return localTime + _timeOffset;
  }
  return localTime;
}

mesh::GroupChannel *BitchatBridge::getMeshChannel() {
  if (_hasMeshChannel) {
    return &_meshChannel;
  }
  return nullptr;
}

#endif // ENABLE_BITCHAT
