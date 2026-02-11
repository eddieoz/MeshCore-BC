/**
 * @file BitchatBridge.cpp
 * @brief Implementation of BitchatBridge integration layer
 *
 * Implements Story 10.2: BitchatBridge Encapsulation Integration
 * Simplified version compatible with existing BitChat components.
 */

#ifdef ENABLE_BITCHAT

#include "BitchatBridge.h"

#include "BitchatIdentity.h"

#include <Arduino.h>
#include <Identity.h>
#include <Mesh.h>
#include <cstring>

// Crypto headers for BitChat integration
#ifndef NATIVE_TEST
  #include <Crypto.h>
#endif

// For key conversion
extern "C" {
#include "../../../lib/ed25519/fe.h"
}

// Helper to compute optimal block size for padding (matching Android/iOS)
static size_t getOptimalBlockSize(size_t dataSize) {
  // Account for encryption overhead (~16 bytes for AES-GCM tag)
  size_t totalSize = dataSize + 16;
  static const size_t blockSizes[] = { 256, 512, 1024, 2048 };
  for (size_t blockSize : blockSizes) {
    if (totalSize <= blockSize) return blockSize;
  }
  return dataSize;
}

// Helper to apply PKCS#7 padding (matching Android/iOS)
static size_t applyPKCS7Padding(uint8_t *buffer, size_t dataSize, size_t targetSize) {
  if (dataSize >= targetSize) return dataSize;
  size_t paddingNeeded = targetSize - dataSize;
  if (paddingNeeded > 255) return dataSize; // PKCS#7 padding length must fit in 1 byte
  for (size_t i = dataSize; i < targetSize; i++) {
    buffer[i] = static_cast<uint8_t>(paddingNeeded);
  }
  return targetSize;
}

// Static callback for BitchatBLEService signing
static void bridgeBitchatSigningCallback(uint8_t *sig, const uint8_t *msg, size_t len, void *arg) {
  BitchatBridge *bridge = static_cast<BitchatBridge *>(arg);
  if (bridge) {
    bridge->onBitchatSignRequest(sig, msg, len);
  }
}

// Helper to convert Ed25519 public key to Curve25519 (X25519) public key
// Used for the Noise field in BitChat announcements
static void ed25519_pk_to_curve25519(uint8_t *curve25519_pub, const uint8_t *ed25519_pub) {
  fe x1, tmp0, tmp1;
  fe_frombytes(x1, ed25519_pub);
  fe_1(tmp1);
  fe_add(tmp0, x1, tmp1);
  fe_sub(tmp1, tmp1, x1);
  fe_invert(tmp1, tmp1);
  fe_mul(x1, tmp0, tmp1);
  fe_tobytes(curve25519_pub, x1);
}

// ============================================================
// Constructor / Destructor
// ============================================================

BitchatBridge::BitchatBridge(mesh::Mesh &mesh, mesh::LocalIdentity &identity, const char *nodeName)
    : _mesh(mesh), _identity(identity), _nodeName(nodeName), _initialized(false), _bitchatPeerId(0),
      _messagesRelayed(0), _duplicatesDropped(0), _privacyDrops(0), _processingMessage(false), _hasMeshChannel(false),
      _hasMeshSecret(false), _decapsulator(_channelRegistry) {
  memset(_noisePublicKey, 0, sizeof(_noisePublicKey));
  memset(&_meshChannel, 0, sizeof(_meshChannel));
  memset(_meshChannelSecret, 0, sizeof(_meshChannelSecret));
}

BitchatBridge::~BitchatBridge() {}

// ============================================================
// Initialization
// ============================================================

void BitchatBridge::begin() {
  Serial.println("[BitChat] begin() ENTRY");
  
  if (_initialized) {
    Serial.println("[BitChat] Already initialized, returning");
    return;
  }

  // Story 1: Compute #mesh secret for privacy filtering
  Serial.println("[BitChat] Calling computeMeshSecret()...");
  computeMeshSecret();

  // Derive BitChat peer ID from identity
  _bitchatPeerId = derivePeerId(_identity);

  // Derive Noise public key from Identity public key
  ed25519_pk_to_curve25519(_noisePublicKey, _identity.pub_key);

  // Set peer ID on BLE service
  _bleService.setPeerId(_bitchatPeerId);
  _bleService.setNodeName(_nodeName);

  // Provide keys and signing callback to BLE service for announcements
  _bleService.setPublicKeys(_noisePublicKey, _identity.pub_key);
  _bleService.setSigningCallback(bridgeBitchatSigningCallback, this);

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

  // Derive the SHA-256 BitChat Peer ID
  uint64_t peerId = mesh::bitchat::deriveBitChatPeerId(_identity.pub_key);
  _bleService.setPeerId(peerId);
  Serial.printf("[BitChat] Derived SHA-256 Peer ID: %016llX\n", peerId);

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
      // Use member variable to avoid stack overflow
      memset(&_tempLoopMessage, 0, sizeof(mesh::ble::BitchatMessage));
      mesh::ble::BitchatMessage &bMsg = _tempLoopMessage;

      bMsg.version = BITCHAT_VERSION;
      bMsg.type = BITCHAT_MSG_MESSAGE;
      bMsg.ttl = 8;
      // Dynamic Time Sync: use bleService time which is now synced from both Messages and Announcements
      uint64_t baseTime = getCurrentTimeMs();

      // Check for valid time (if > 2024 it's likely synced or at least using the updated fallback)
      if (baseTime > 1700000000000ULL) {
        Serial.printf("[BitChat] Using time: %llu\n", baseTime);
      } else {
        // Should not happen as BitchatBLEService has a fallback, but just in case
        Serial.printf("[BitChat] Warning: Time seems invalid: %llu\n", baseTime);
      }

      bMsg.timestamp = baseTime;

      // Split 64-bit print to avoid printf issues on some platforms
      uint32_t tsHi = (uint32_t)(bMsg.timestamp >> 32);
      uint32_t tsLo = (uint32_t)(bMsg.timestamp & 0xFFFFFFFF);
      Serial.printf("[BitChat] Sending message with timestamp=%08lX%08lX\n", tsHi, tsLo);

      bMsg.setSenderId64(_bleService.getPeerId()); // Use local ID for signing

      // Build plain text payload for broadcast compatibility (Android app expects UTF-8 string)
      // Format: #channel: <sender> <text>
      // Example: #mesh: 🤖 mc/HQ2 Test1
      // Android parsing logic:
      //   if (startWith("#") && contains(":")) {
      //     channel = substring(0, colon);
      //     content = substring(colon + 1);
      //   }

      // Build plain text payload for broadcast compatibility (Android app expects UTF-8 string)
      // Format: <sender>: <text> (Broadcast/No Channel)
      // Example: 🤖 mc/HQ2: Test1
      // We removed #mesh: prefix as it was causing messages to be hidden in the Android app
      // (likely due to channel filtering logic being strict or the view being 'Global').

      // OPTIMIZATION: Write directly to bMsg.payload to avoid 2KB stack allocation (Stack Overflow fix)
      // bMsg.payload is BITCHAT_MAX_PAYLOAD_SIZE (2048) bytes
      char *payloadPtr = (char *)bMsg.payload;
      int len = snprintf(payloadPtr, BITCHAT_MAX_PAYLOAD_SIZE, "%s: %s", msg.sender, msg.text);

      if (len < 0) len = 0;
      if (len >= BITCHAT_MAX_PAYLOAD_SIZE) len = BITCHAT_MAX_PAYLOAD_SIZE - 1; // Truncate if needed

      bMsg.payloadLength = (uint16_t)len;
      // No memcpy needed since we wrote directly!

      Serial.printf("[BitChat] Plain text payload built: %s (len=%d)\n", payloadPtr, len);

      // Sign the message with local identity
      signBitChatMessage(bMsg);

      if (_bleService.sendBitchatMessage(bMsg)) {
        Serial.println("[BitChat] Signed message sent to BitChat app via loop");
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

  // Story 1: Bootstrapping - Periodically send BitChat announcements
  // Android app requires Unix timestamps to avoid "stale announcement" drop.
  // We provide a synthetic timestamp (approx seconds since epoch) if RTC/NTP not available.
  static uint32_t lastAnnounceTime = 0;
  if (now - lastAnnounceTime >= 5000) {
    lastAnnounceTime = now;

    // Use synced time from BitchatBLEService (synced via incoming announcements from phone)
    // getCurrentTimeMs() returns milliseconds; sendAnnouncement expects seconds
    uint64_t currentMs = getCurrentTimeMs();
    uint64_t unixTime = currentMs / 1000ULL;

    Serial.printf("[BitChat] Sending announcement with timestamp: %lu (synced_ms=%llu)\n",
                  (unsigned long)unixTime, currentMs);

    // Ensure service has latest keys
    _bleService.setPublicKeys(_noisePublicKey, _identity.pub_key);

    // Send announcement with valid timestamp
    _bleService.sendAnnouncement(unixTime);
  }
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

  // Story 3: PRIVACY FIX - Only forward messages from #mesh hashtag channel
  if (!isMeshChannel(channel)) {
    Serial.println("[BitChat] Ignoring: Not from #mesh channel (privacy protection)");
    _privacyDrops++;
    _processingMessage = false;
    return;
  }
  Serial.println("[BitChat] Verified: Message from #mesh channel");

  // Cache the channel for outgoing messages
  // IMPORTANT: Only copy hash[0], don't overwrite our secret!
  if (!_hasMeshChannel) {
    _meshChannel.hash[0] = channel.hash[0];  // Only copy 1 byte (PATH_HASH_SIZE)
    // Secret should already be set from initMeshChannel()
    _hasMeshChannel = true;
    Serial.println("[BitChat] Cached mesh channel hash for outgoing messages");
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
  Serial.println("[BitChat] onBitchatMessageReceived() ENTRY");
  
  if (!_initialized) {
    Serial.println("[BitChat] ERROR: Bridge not initialized!");
    return;
  }
  
  if (_processingMessage) {
    Serial.println("[BitChat] ERROR: Already processing message!");
    return;
  }

  _processingMessage = true;

  // Time Sync: Now handled by BitchatBLEService automatically for both Messages and Announcements
  // which updates its internal clock via syncTime()

  // Handle different message types
  Serial.printf("[BitChat] onBitchatMessageReceived: type=%d, payloadLen=%d, _hasMeshChannel=%d\n", 
                msg.type, msg.payloadLength, _hasMeshChannel ? 1 : 0);

  // DEBUG: Check if secret is still valid
  if (_hasMeshChannel) {
    Serial.print("[BitChat] DEBUG _meshChannel.secret: ");
    for (int i = 0; i < 16; i++) {
      Serial.printf("%02X", _meshChannel.secret[i]);
    }
    Serial.println();
    
    // Verify against expected
    if (_meshChannel.secret[0] != 0x5B) {
      Serial.printf("[BitChat] WARNING: Secret corrupted! Expected 0x5B, got 0x%02X\n", _meshChannel.secret[0]);
      // Re-initialize
      initMeshChannel();
    }
  }

  // DEBUG: Check secret before processing
  if (_hasMeshChannel && _meshChannel.secret[0] != 0x5B) {
    Serial.printf("[BitChat] DEBUG: Secret corrupted before switch! first_byte=0x%02X\n", _meshChannel.secret[0]);
  }

  switch (msg.type) {
  case 0x02: { // MESSAGE
    // DEBUG: Check secret at case entry
    if (_meshChannel.secret[0] != 0x5B) {
      Serial.printf("[BitChat] DEBUG: Secret corrupted at case 0x02 entry! first_byte=0x%02X\n", _meshChannel.secret[0]);
    }
    
    // Check if we have a cached mesh channel
    if (!_hasMeshChannel) {
      Serial.println("[BitChat] ERROR: No mesh channel cached yet!");
      Serial.println("[BitChat] Call initMeshChannel() first or switch to BitChat mode");
      Serial.printf("[BitChat] _hasMeshSecret=%d, _initialized=%d\n", _hasMeshSecret ? 1 : 0, _initialized ? 1 : 0);
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

    // Look up sender nickname from ID
    uint64_t senderId = msg.getSenderId64();
    mesh::ble::PeerInfo peerInfo;
    char senderName[33];

    if (_bleService.getPeerById(senderId, peerInfo)) {
      strncpy(senderName, peerInfo.nickname, 32);
      senderName[32] = 0;
    } else {
      // Fallback if not verified/found yet
      snprintf(senderName, sizeof(senderName), "anon%04X", (uint16_t)(senderId & 0xFFFF));
    }

    int prefixLen = sprintf((char *)&temp[5], "📱 %s: ", senderName);
    memcpy(&temp[5 + prefixLen], msg.payload, payloadLen);
    temp[5 + prefixLen + payloadLen] = 0; // null terminator

    size_t totalLen = 5 + prefixLen + payloadLen + 1; // include null

    Serial.printf("[BitChat] Sending GRP_TXT to mesh, len=%d\n", totalLen);
    Serial.print("[BitChat] Full 32-byte secret: ");
    for (int i = 0; i < 32; i++) Serial.printf("%02X", _meshChannel.secret[i]);
    Serial.println();
    Serial.printf("[BitChat] Channel secret first byte: 0x%02X, hash first byte: 0x%02X\n",
                  _meshChannel.secret[0], _meshChannel.hash[0]);

    // Create and send as standard group text message
    auto *pkt = _mesh.createGroupDatagram(0x05, // PAYLOAD_TYPE_GRP_TXT
                                          _meshChannel, temp, totalLen);
    if (pkt) {
      Serial.printf("[BitChat] Packet created, payload_len=%d, path_len=%d\n", 
                    pkt->payload_len, pkt->path_len);
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

void BitchatBridge::signBitChatMessage(mesh::ble::BitchatMessage &msg) {
  // 1. Set the HAS_SIGNATURE flag
  msg.flags |= BITCHAT_FLAG_HAS_SIGNATURE;

  // 2. Serialize the message content for signing (excluding signature field itself)
  // Use member buffer to avoid stack overflow (~2KB allocation on stack was killing it!)
  // uint8_t buffer[BITCHAT_MAX_PAYLOAD_SIZE + 64];
  size_t offset = 0;

  _signingBuffer[offset++] = msg.version;
  _signingBuffer[offset++] = msg.type;
  _signingBuffer[offset++] = 0; // Force TTL to 0 to match Android AppConstants.SYNC_TTL_HOPS

  // Timestamp (Big Endian)
  for (int i = 0; i < 8; i++) {
    _signingBuffer[offset++] = (msg.timestamp >> ((7 - i) * 8)) & 0xFF;
  }

  // Flags: Mask out HAS_SIGNATURE for signing
  // The signature covers the packet AS IF it didn't have a signature yet
  _signingBuffer[offset++] = (msg.flags & ~BITCHAT_FLAG_HAS_SIGNATURE);

  // Payload Length (Big Endian)
  _signingBuffer[offset++] = (msg.payloadLength >> 8) & 0xFF;
  _signingBuffer[offset++] = (msg.payloadLength & 0xFF);

  // Sender ID (big-endian for BitChat protocol compatibility)
  for (int i = 7; i >= 0; i--) {
    _signingBuffer[offset++] = msg.senderId[i];
  }

  // CRITICAL: Aligned with iOS/Android - recipientID field (8 bytes) is ONLY present if flag is set
  if (msg.hasRecipient()) {
    for (int i = 7; i >= 0; i--) {
      _signingBuffer[offset++] = msg.recipientId[i];
    }
  }

  // Payload: starts at offset 30 (1+1+1+8+1+2+8+8)
  memcpy(&_signingBuffer[offset], msg.payload, msg.payloadLength);
  offset += msg.payloadLength;

  // 3. Apply padding and sign
  onBitchatSignRequest(msg.signature, _signingBuffer, offset);
}

// Story 1 & 2: Use hardcoded #mesh secret
// First 16 bytes of SHA256("#mesh") = 5b664cde0b08b220612113db980650f3
// Hardcoded to avoid memory corruption issues during SHA256 computation
void BitchatBridge::computeMeshSecret() {
  Serial.println("[BitChat] computeMeshSecret() ENTRY (hardcoded)");
  
  // Hardcoded secret for #mesh channel (first 16 bytes of SHA256("#mesh"))
  const uint8_t MESH_SECRET[16] = {
    0x5B, 0x66, 0x4C, 0xDE, 0x0B, 0x08, 0xB2, 0x20,
    0x61, 0x21, 0x13, 0xDB, 0x98, 0x06, 0x50, 0xF3
  };
  
  memcpy(_meshChannelSecret, MESH_SECRET, 16);
  _hasMeshSecret = true;

  // Debug: print full 16-byte secret
  Serial.print("[BitChat] #mesh secret (hardcoded): ");
  for (int i = 0; i < 16; i++) {
    Serial.printf("%02X", _meshChannelSecret[i]);
  }
  Serial.println();
}

// Compute channel hash from secret (SHA256 of secret, for routing)
void BitchatBridge::computeChannelHash(uint8_t *hash, const uint8_t *secret, size_t secretLen) {
  mesh::Utils::sha256(hash, MAX_HASH_SIZE, secret, secretLen);
}

// Initialize the #mesh channel for sending messages
// Call this when switching to BitChat mode
bool BitchatBridge::initMeshChannel() {
  Serial.println("[BitChat] initMeshChannel() called");
  
  if (!_hasMeshSecret) {
    Serial.println("[BitChat] ERROR: Cannot init mesh channel - secret not computed");
    return false;
  }
  if (_hasMeshChannel) {
    Serial.println("[BitChat] Mesh channel already initialized");
    return true;
  }

  // Initialize the channel structure
  memset(&_meshChannel, 0, sizeof(_meshChannel));
  
  // Debug: Check source before copy
  Serial.print("[BitChat] initMeshChannel: source secret: ");
  for (int i = 0; i < 16; i++) Serial.printf("%02X", _meshChannelSecret[i]);
  Serial.println();
  
  // Copy the secret (16 bytes) - note: GroupChannel.secret is 32 bytes (PUB_KEY_SIZE)
  // but MeshCore hashtag channels use first 16 bytes of SHA256 as the secret
  memcpy(_meshChannel.secret, _meshChannelSecret, 16);
  
  // Debug: Check destination immediately after copy
  Serial.print("[BitChat] initMeshChannel: after memcpy, secret: ");
  for (int i = 0; i < 32; i++) Serial.printf("%02X", _meshChannel.secret[i]);
  Serial.println();
  
  // Hardcoded channel hash (first byte only - PATH_HASH_SIZE = 1)
  // MeshCore only uses the first byte of hash for routing
  _meshChannel.hash[0] = 0xB0;  // First byte of SHA256(secret)
  
  Serial.printf("[BitChat] Hash (hardcoded): 0x%02X\n", _meshChannel.hash[0]);
  
  _hasMeshChannel = true;
  
  Serial.println("[BitChat] #mesh channel initialized for sending");
  Serial.print("[BitChat] Channel secret (source): ");
  for (int i = 0; i < 16; i++) {
    Serial.printf("%02X", _meshChannelSecret[i]);
  }
  Serial.println();
  Serial.print("[BitChat] Channel secret (in struct, 32 bytes): ");
  for (int i = 0; i < 32; i++) {
    Serial.printf("%02X", _meshChannel.secret[i]);
  }
  Serial.println();
  Serial.print("[BitChat] Channel hash (first byte): 0x");
  Serial.printf("%02X", _meshChannel.hash[0]);
  Serial.println();
  
  return true;
}

// Story 2: Verify channel is #mesh by comparing secrets
bool BitchatBridge::isMeshChannel(const mesh::GroupChannel &channel) const {
  if (!_hasMeshSecret) {
    Serial.println("[BitChat] isMeshChannel: no mesh secret available");
    return false;
  }

  // Compare first 16 bytes for hashtag channel match
  // Per MeshCore spec, hashtag secrets are first 16 bytes of SHA256("#channelname")
  bool match = (memcmp(channel.secret, _meshChannelSecret, 16) == 0);

  if (match) {
    Serial.println("[BitChat] Channel verified: #mesh");
  } else {
    // Debug: show first 16 bytes for troubleshooting
    Serial.println("[BitChat] Channel mismatch:");
    Serial.print("  received=");
    for (int i = 0; i < 16; i++) Serial.printf("%02X", channel.secret[i]);
    Serial.print("\n  expected =");
    for (int i = 0; i < 16; i++) Serial.printf("%02X", _meshChannelSecret[i]);
    Serial.println();
  }

  return match;
}

void BitchatBridge::onBitchatSignRequest(uint8_t *sig, const uint8_t *msg, size_t len) {
  // Use a temporary buffer for padding if needed, or use the member buffer
  // Since this is called from BLE service (announcements) OR locally (messages)
  // we must be careful not to corrupt the input 'msg' if it's the member buffer.

  size_t targetSize = getOptimalBlockSize(len);

  if (targetSize > len && targetSize <= 2048) {
    // If input is not our member buffer, we must copy it first
    if (msg != _signingBuffer) {
      memcpy(_signingBuffer, msg, len);
    }

    size_t paddedLen = applyPKCS7Padding(_signingBuffer, len, targetSize);
    _identity.sign(sig, _signingBuffer, paddedLen);

    Serial.printf("[BitChat] Signed PADDED message: raw=%u, padded=%u\n", (uint32_t)len, (uint32_t)paddedLen);
  } else {
    _identity.sign(sig, msg, len);
    Serial.printf("[BitChat] Signed UNPADDED message: len=%u (padding skipped)\n", (uint32_t)len);
  }
}

uint64_t BitchatBridge::derivePeerId(const mesh::LocalIdentity &identity) {
  return mesh::bitchat::deriveBitChatPeerId(identity.pub_key);
}

uint64_t BitchatBridge::getCurrentTimeMs() const {
  // BitchatBLEService is a member object, not a pointer.
  // It provides synchronized 64-bit time.
  return _bleService.getCurrentTimeMs();
}

mesh::GroupChannel *BitchatBridge::getMeshChannel() {
  if (_hasMeshChannel) {
    return &_meshChannel;
  }
  return nullptr;
}

#endif // ENABLE_BITCHAT
