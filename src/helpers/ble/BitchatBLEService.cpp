#include "BitchatBLEService.h"

#ifdef ENABLE_BITCHAT

#ifdef NATIVE_TEST
#include <cstdio>
#include <cstring>
#include <ctime>
#else
#ifdef ESP32
#include <Arduino.h>
#elif defined(NRF52_PLATFORM)
#include "../nrf52/SerialBLEInterface.h"

#include <Arduino.h>
#endif // This #endif closes the #ifdef ESP32 / #elif defined(NRF52_PLATFORM) block
#endif // This #endif closes the #ifdef NATIVE_TEST block

// Helper for time - must be defined before use
static uint32_t getCurrentTimeMs() {
#ifdef NATIVE_TEST
  // Use simple counter for native tests
  static uint32_t counter = 0;
  return counter += 100;
#else
  return millis();
#endif
}

namespace mesh {
namespace ble {

// Static instance pointer for nRF52 callback routing
BitchatBLEService *BitchatBLEService::_instance = nullptr;

// Static lock to prevent BLE receive callback during send operations
volatile bool BitchatBLEService::_sendingInProgress = false;

// Static callbacks for SerialBLEInterface
void BitchatBLEService::onConnect(uint16_t conn_handle) {
  if (_instance) {
    ConnectionInfo info;
    info.connectionId = conn_handle;
    _instance->onClientConnect(info);
  }
}

void BitchatBLEService::onDisconnect(uint16_t conn_handle, uint8_t reason) {
  if (_instance) {
    _instance->onClientDisconnect(conn_handle);
  }
}

BitchatBLEService::BitchatBLEService()
    : _peerId(0), _registered(false), _lastAnnounceTime(0), _announcementSent(false), _writeBufferOffset(0),
      _pendingData(false), _lastWriteTime(0), _bitchatClientCount(0), _clientSubscribed(false),
      _hasPendingOutgoing(false), _connectionTime(0)
#ifdef ESP32
      ,
      _platformService(nullptr), _characteristic(nullptr)
#elif defined(NRF52_PLATFORM)
      ,
      _service() // Default constructor, UUID set later in initNRF52
      ,
      _characteristic()
#endif
{
  memset(_nodeName, 0, sizeof(_nodeName));
  strcpy(_nodeName, "MeshCore");
  memset(_peerCache, 0, sizeof(_peerCache));

  // Initialize buffers
  _writeBufferOffset = 0;
  memset(_writeBuffer, 0, sizeof(_writeBuffer));
  memset(_readBuffer, 0, sizeof(_readBuffer));
  _pendingData = false;

  // Store instance for nRF52 callback routing
#ifdef NRF52_PLATFORM
  _instance = this;
#endif
}

BitchatBLEService::~BitchatBLEService() {
  onServiceUnregistered();
}

void BitchatBLEService::setNodeName(const char *name) {
  if (name) {
    strncpy(_nodeName, name, sizeof(_nodeName) - 1);
    _nodeName[sizeof(_nodeName) - 1] = '\0';
  }
}

void BitchatBLEService::setPeerId(uint64_t peerId) {
  _peerId = peerId;
}

const char *BitchatBLEService::getNodeName() const {
  return _nodeName;
}

uint64_t BitchatBLEService::getPeerId() const {
  return _peerId;
}

// Message handling

bool BitchatBLEService::hasReceivedMessages() const {
  return !_recvQueue.empty();
}

size_t BitchatBLEService::getMessageQueueSize() const {
  return _recvQueue.size();
}

bool BitchatBLEService::getNextMessage(BitchatMessage &msg) {
  if (_recvQueue.empty()) {
    return false;
  }
  msg = _recvQueue.front();
  _recvQueue.pop();
  return true;
}

void BitchatBLEService::clearMessageQueue() {
  while (!_recvQueue.empty()) {
    _recvQueue.pop();
  }
}

// Peer cache

size_t BitchatBLEService::getPeerCacheSize() const {
  size_t count = 0;
  for (size_t i = 0; i < PEER_CACHE_SIZE; i++) {
    if (_peerCache[i].valid) {
      count++;
    }
  }
  return count;
}

bool BitchatBLEService::getPeerById(uint64_t peerId, PeerInfo &info) const {
  for (size_t i = 0; i < PEER_CACHE_SIZE; i++) {
    if (_peerCache[i].valid && _peerCache[i].peerId == peerId) {
      info = _peerCache[i];
      return true;
    }
  }
  return false;
}

// Announcement

void BitchatBLEService::sendAnnouncement() {
  if (_connections.empty()) {
    return;
  }

  // Build ANNOUNCE message
  // Use member variable to avoid stack overflow
  memset(&_tempTxMessage, 0, sizeof(BitchatMessage));
  BitchatMessage &msg = _tempTxMessage;

  msg.type = BITCHAT_MSG_ANNOUNCE;
  msg.ttl = 8;
  msg.setSenderId64(_peerId);
  msg.flags = 0;

  // Build TLV payload
  size_t offset = 0;

  // Nickname TLV (type=0x01)
  size_t nickLen = strlen(_nodeName);
  if (nickLen > 0 && offset + 2 + nickLen < BITCHAT_MAX_PAYLOAD_SIZE) {
    msg.payload[offset++] = 0x01; // TLV type
    msg.payload[offset++] = static_cast<uint8_t>(nickLen);
    memcpy(&msg.payload[offset], _nodeName, nickLen);
    offset += nickLen;
  }

  msg.payloadLength = static_cast<uint16_t>(offset);

  // Serialize and send
  uint8_t buffer[512];
  size_t len = serializeMessage(msg, buffer, sizeof(buffer));
  if (len > 0) {
    broadcastNotification(buffer, len);
    _announcementSent = true;
  }

  _lastAnnounceTime = getCurrentTimeMs();
}

bool BitchatBLEService::wasAnnouncementSent() const {
  return _announcementSent;
}

bool BitchatBLEService::sendMessageToClients(const char *senderName, const char *text, uint64_t timestamp) {
  if (!senderName || !text) {
    return false;
  }

  // Check if we have connected clients
  if (!hasConnectedClients()) {
    Serial.println("[BitChat BLE] No clients connected");
    return false;
  }

  // Build the message content in BitChat format: "<senderName> text"
  // This matches the reference implementation in onMeshcoreGroupMessage
  // Use static buffer to avoid stack overflow on nRF52
  static char formattedContent[256];
  snprintf(formattedContent, sizeof(formattedContent), "<%s> %s", senderName, text);

  Serial.printf("[BitChat BLE] Sending message: %s\n", formattedContent);

  // CRITICAL: Acquire lock to prevent BLE receive callbacks during send
  // This prevents the softdevice from triggering receive callbacks that would
  // conflict with the ongoing BLE send operation
  _sendingInProgress = true;

  // Use member buffer to avoid stack overflow
  _tempTxMessage = BitchatMessage(); // Reset
  BitchatMessage &msg = _tempTxMessage;

  msg.version = BITCHAT_VERSION;
  msg.type = BITCHAT_MSG_MESSAGE;
  msg.ttl = 8;
  // Convert timestamp: if it's a Unix timestamp (seconds), convert to milliseconds
  // BitChat protocol uses milliseconds since epoch
  msg.timestamp = (timestamp < 1000000000000ULL) ? timestamp * 1000ULL : timestamp;
  msg.flags = 0; // No recipient (channel message), no signature

  // Set sender ID from node's peer ID
  msg.setSenderId64(_peerId);

  // Set payload to the formatted content
  size_t contentLen = strlen(formattedContent);
  if (contentLen > BITCHAT_MAX_PAYLOAD_SIZE) contentLen = BITCHAT_MAX_PAYLOAD_SIZE;
  msg.payloadLength = (uint16_t)contentLen;
  memcpy(msg.payload, formattedContent, contentLen);

  bool result = sendBitchatMessage(msg);

  // Release lock after send completes
  _sendingInProgress = false;

  return result;
}

bool BitchatBLEService::sendBitchatMessage(const BitchatMessage &msg) {
  if (!hasConnectedClients()) {
    return false;
  }

  // If client connected but not yet subscribed, queue for notification later
  // This matches reference implementation behavior
  if (!_clientSubscribed) {
    Serial.println("[BitChat BLE] Client connected but not subscribed, queuing message");
    _pendingOutgoing = msg;
    _hasPendingOutgoing = true;
    return true; // Return true as we've "sent" it to the queue
  }

  // Use member buffer to avoid stack overflow
  // Buffer size in header is BITCHAT_MAX_PAYLOAD_SIZE + 100
  size_t len = serializeMessage(msg, _tempTxBuffer, sizeof(_tempTxBuffer));

  if (len == 0) {
    Serial.println("[BitChat BLE] Error: Failed to serialize message");
    return false;
  }

  Serial.printf("[BitChat BLE] Broadcasting message type=%d, len=%d\n", msg.type, len);

  return broadcastNotification(_tempTxBuffer, len);
}

// BLEServiceWrapper implementation

bool BitchatBLEService::onServiceRegistered(void *platformService) {
  if (_registered) {
    return false;
  }

#ifdef ESP32
  _platformService = static_cast<BLEService *>(platformService);
  if (!_platformService) {
    return false;
  }

  // Create single characteristic for BitChat (read/write/notify)
  _characteristic = _platformService->createCharacteristic(
      BITCHAT_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE |
                                       BLECharacteristic::PROPERTY_WRITE_NR |
                                       BLECharacteristic::PROPERTY_NOTIFY);

  if (!_characteristic) {
    return false;
  }

  // Open security - no encryption required
  _characteristic->setAccessPermissions(ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE);

  // Add descriptor for notifications
  _characteristic->addDescriptor(new BLE2902());

#elif defined(NRF52_PLATFORM)
  // For nRF52, the service is already created in initNRF52
  // platformService should be nullptr or we ignore it
  (void)platformService;

  _characteristic.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE | CHR_PROPS_WRITE_WO_RESP |
                                CHR_PROPS_NOTIFY);
  _characteristic.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  _characteristic.setMaxLen(512);
  _characteristic.begin();
#endif

  _registered = true;
  _lastAnnounceTime = getCurrentTimeMs();
  return true;
}

void BitchatBLEService::onServiceUnregistered() {
  _registered = false;
  _connections.clear();
  clearMessageQueue();

#ifdef ESP32
  _platformService = nullptr;
  _characteristic = nullptr;
#elif defined(NRF52_PLATFORM)
  // nRF52 services are stopped but objects remain
#endif
}

void BitchatBLEService::onClientConnect(const ConnectionInfo &connInfo) {
  _connections.push_back(connInfo);

#ifdef NRF52_PLATFORM
  _bitchatClientCount++;
  Serial.printf("[BitChat BLE] Client connected, count=%d\n", _bitchatClientCount);

  // Track connection time for stability delay
  _connectionTime = getCurrentTimeMs();
#endif

  // Defer announcement to main loop and add delay for MTU negotiation
  // Trigger after 1000ms
  _lastAnnounceTime = getCurrentTimeMs() - ANNOUNCE_INTERVAL_MS + 1000;
}

void BitchatBLEService::onClientDisconnect(uint16_t connectionId) {
  _connections.erase(
      std::remove_if(_connections.begin(), _connections.end(),
                     [connectionId](const ConnectionInfo &c) { return c.connectionId == connectionId; }),
      _connections.end());

#ifdef NRF52_PLATFORM
  if (_bitchatClientCount > 0) {
    _bitchatClientCount--;
  }
  Serial.printf("[BitChat BLE] Client disconnected, count=%d\n", _bitchatClientCount);

  if (_bitchatClientCount == 0) {
    _clientSubscribed = false;
    _hasPendingOutgoing = false;
    clearWriteBuffer();
  }
#endif
}

void BitchatBLEService::onDataReceived(uint16_t connectionId, const uint8_t *data, size_t len) {
  if (!data || len < BITCHAT_HEADER_SIZE) {
    return;
  }

  parseAndQueueMessage(data, len);
}

void BitchatBLEService::onDataSent(uint16_t connectionId, size_t len) {
  // Track sent data
}

bool BitchatBLEService::sendNotification(uint16_t connectionId, const uint8_t *data, size_t len) {
  if (!data || len == 0) {
    return false;
  }

#ifdef ESP32
  if (_characteristic) {
    _characteristic->setValue(const_cast<uint8_t *>(data), len);
    _characteristic->notify();
    return true;
  }
#elif defined(NRF52_PLATFORM)
  (void)connectionId; // nRF52 characteristic.notify() broadcasts to all subscribed clients
  int result = _characteristic.notify(data, len);
  Serial.printf("[BitChat BLE] notify result: %d\n", result);
  return result > 0;
#endif

  return false;
}

bool BitchatBLEService::broadcastNotification(const uint8_t *data, size_t len) {
  Serial.printf("[BitChat BLE] broadcastNotification: len=%d\n", len);

#ifdef ESP32
  if (_characteristic) {
    _characteristic->setValue(const_cast<uint8_t *>(data), len);
    _characteristic->notify();
    Serial.println("[BitChat BLE] ESP32 notify done");
    return true;
  }
#elif defined(NRF52_PLATFORM)
  // On Bluefruit, notify() broadcasts to all connected clients
  // Add defensive checks
  if (len == 0 || data == nullptr) {
    Serial.println("[BitChat BLE] ERROR: invalid data for notify");
    return false;
  }

  Serial.printf("[BitChat BLE] Calling Bluefruit notify, len=%u, connected=%d\n", len, Bluefruit.connected());

  // CRITICAL: Check connection stability before sending
  // The SoftDevice can crash if we send notifications before MTU negotiation completes
  uint32_t connAge = getCurrentTimeMs() - _connectionTime;
  if (connAge < 1000) {
    Serial.printf("[BitChat BLE] Connection too young (%ums), deferring notification\n", connAge);
    return false; // Connection not stable yet - will retry on next loop
  }

  // Safety check: Bluefruit notify() might crash/fail if len > MTU-3
  // Default MTU is 23, so payload max is 20.
  // We rely on MTU negotiation having completed during the 1000ms delay.

  bool result = _characteristic.notify(data, len);

  if (!result) {
    Serial.println("[BitChat BLE] notify failed (buffer full or len > MTU?)");
  }

  // CRITICAL: Add significant delay after BLE notify to allow softdevice to complete
  // pending events before returning. The softdevice processes BLE events asynchronously
  // and can crash if we return too quickly to the caller.
  // Using delay() instead of vTaskDelay() to avoid context switching complications
  // during this critical section.
  delay(10); // 10ms blocking delay is safer here

  return result;
#else
  (void)data;
  (void)len;
#endif
  return false;
}

bool BitchatBLEService::hasConnectedClients() const {
#ifdef NRF52_PLATFORM
  // Use our tracked client count instead of raw Bluefruit.connected()
  // This is maintained by onConnect/onDisconnect callbacks
  return _bitchatClientCount > 0;
#else
  return !_connections.empty();
#endif
}

size_t BitchatBLEService::getConnectedClientCount() const {
  return _connections.size();
}

void BitchatBLEService::loop() {
  // Process buffered data from ISR (Reference implementation pattern)
  // This handles fragmentation and keeps ISR fast
  uint32_t now = millis();

  if (_pendingData) {
    if (now - _lastWriteTime >= 50) { // 50ms timeout to ensure full message
      _pendingData = false;

      // Atomic transfer from write buffer to read buffer
      size_t processLen = 0;

      {
        // CRITICAL SECTION: Disable interrupts to safely move data
        // This prevents race conditions where ISR modifies buffer while we read it
#ifdef NRF52_PLATFORM
        noInterrupts();
#endif
        if (_writeBufferOffset > 0) {
          memcpy(_readBuffer, _writeBuffer, _writeBufferOffset);
          processLen = _writeBufferOffset;
          _writeBufferOffset = 0;
        }
#ifdef NRF52_PLATFORM
        interrupts();
#endif
      }

      if (processLen > 0) {
        Serial.printf("[BitChat BLE] Processing received data: %u bytes\n", processLen);

        // Parse and queue the message - now running in main loop context (SAFE)
        // using our local read buffer so ISR can continue filling write buffer
        parseAndQueueMessage(_readBuffer, processLen);

        // No need to clear write buffer here, it was done atomically above

        // Clear buffer for next message
        // clearWriteBuffer(); // Removed as it was done in atomic block
      }
    }
  }

  // Check for pending outgoing messages (deferred from ISR or connection event)
  if (_hasPendingOutgoing && _clientSubscribed) {
    Serial.println("[BitChat BLE] Sending pending outgoing message from loop");
    sendPendingOutgoing();
  }

  // Send periodic announcements
  if (now - _lastAnnounceTime >= ANNOUNCE_INTERVAL_MS) {
    sendAnnouncement();
  }
}

#ifdef NRF52_PLATFORM
bool BitchatBLEService::initNRF52(const char *deviceName) {
  Serial.println("[BitChat BLE] initNRF52() called");

  if (_registered) {
    Serial.println("[BitChat BLE] Already registered");
    return true;
  }

  // Set device name
  setNodeName(deviceName);

  // Configure the BLE service
  // Note: We don't call Bluefruit.begin() here - it's already done by SerialBLEInterface

  // Set up the service and characteristic
  // The service UUID needs to be converted from string to BLEUUID
  // BITCHAT_SERVICE_UUID = "F47B5E2D-4A9E-4C5A-9B3F-8E1D2C3A4B5C"

  // For Bluefruit, we need to use the byte array form (little-endian)
  static const uint8_t serviceUUID[] = { 0x5C, 0x4B, 0x3A, 0x2C, 0x1D, 0x8E, 0x3F, 0x9B,
                                         0x5A, 0x4C, 0x9E, 0x4A, 0x2D, 0x5E, 0x7B, 0xF4 };

  static const uint8_t charUUID[] = { 0x5D, 0x4C, 0x3B, 0x2A, 0x1F, 0x0E, 0x9D, 0x8C,
                                      0x5B, 0x4A, 0xF6, 0xE5, 0xD4, 0xC3, 0xB2, 0xA1 };

  // Initialize the service with UUID
  _service = BLEService(serviceUUID);
  _service.begin();

  // Initialize the characteristic
  _characteristic = BLECharacteristic(charUUID);
  _characteristic.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE | CHR_PROPS_WRITE_WO_RESP |
                                CHR_PROPS_NOTIFY);
  _characteristic.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  _characteristic.setMaxLen(512);

  // Set up CCCD callback for tracking subscriptions
  _characteristic.setCccdWriteCallback(onCharacteristicCccdWrite);

  // Register extended callbacks with SerialBLEInterface
  SerialBLEInterface::setExtendedConnectCallback(BitchatBLEService::onConnect);
  SerialBLEInterface::setExtendedDisconnectCallback(BitchatBLEService::onDisconnect);

  // Set up write callback - route to instance
  // Reentrancy protection and buffer safety are handled by:
  // 1. inCallback flag (prevents recursive calls)
  // 2. Atomic buffer transfer in loop() with noInterrupts()
  _characteristic.setWriteCallback(
      [](uint16_t conn_hdl, BLECharacteristic *chr, uint8_t *data, uint16_t len) {
        (void)conn_hdl;
        (void)chr;

        // Reentrancy guard - prevent recursive calls from flooding
        static volatile bool inCallback = false;
        if (inCallback) {
          // Already processing, skip this call
          return;
        }
        inCallback = true;

        if (_instance != nullptr && len > 0) {
          // Buffer the data for processing in the main loop
          // Simplified logic to keep ISR fast and stack usage low

          _instance->_lastWriteTime = millis();
          _instance->_pendingData = true;

          // Append to write buffer with overflow protection
          size_t currentOffset = _instance->_writeBufferOffset;
          size_t copyLen = len;

          if (currentOffset + copyLen > sizeof(_instance->_writeBuffer)) {
            // Buffer overflow - clear and start fresh or truncate?
            // Reference implementation clears buffer on overflow
            _instance->clearWriteBuffer();
            currentOffset = 0;
            if (copyLen > sizeof(_instance->_writeBuffer)) {
              copyLen = sizeof(_instance->_writeBuffer);
            }
          }

          memcpy(&_instance->_writeBuffer[currentOffset], data, copyLen);
          _instance->_writeBufferOffset = currentOffset + copyLen;
        }

        inCallback = false;
      });

  _characteristic.begin();

  _registered = true;
  Serial.println("[BitChat BLE] Service initialized on nRF52");
  return true;
}
#endif

// Internal helpers

void BitchatBLEService::clearWriteBuffer() {
  _writeBufferOffset = 0;
  memset(_writeBuffer, 0, sizeof(_writeBuffer));
}

void BitchatBLEService::parseAndQueueMessage(const uint8_t *data, size_t len) {
  // Use member variable to avoid large stack allocation (2KB+)
  // Use member variable to avoid large stack allocation (2KB+)
  // CRITICAL: Do NOT use _tempRxMessage = BitchatMessage() as it creates a temp object on stack
  memset(&_tempRxMessage, 0, sizeof(BitchatMessage));
  _tempRxMessage.version = BITCHAT_VERSION;

  BitchatMessage &msg = _tempRxMessage;

  bool parsed = parseMessage(data, len, msg);
  Serial.printf("[BitChat BLE] parseMessage returned %d\n", parsed);

  if (!parsed) {
    return;
  }

  Serial.printf("[BitChat BLE] Message type: 0x%02X\n", msg.type);

  // Process based on type
  switch (msg.type) {
  case BITCHAT_MSG_ANNOUNCE:
    Serial.println("[BitChat BLE] Processing ANNOUNCE");
    processAnnounce(msg);
    break;

  case BITCHAT_MSG_MESSAGE:
  case BITCHAT_MSG_PING:
  case BITCHAT_MSG_PONG:
  case BITCHAT_MSG_REQUEST_SYNC:
    // Queue these for processing
    if (_recvQueue.size() < MAX_MESSAGE_QUEUE) {
      _recvQueue.push(msg);
    }
    break;

  default:
    // Ignore unknown types
    break;
  }
}

void BitchatBLEService::processAnnounce(const BitchatMessage &msg) {
  Serial.println("[ANNOUNCE_DEBUG] processAnnounce: entry");

  char nickname[64];
  uint8_t pubKey[32];

  Serial.println("[ANNOUNCE_DEBUG] processAnnounce: calling parseAnnounceTLV");
  if (parseAnnounceTLV(msg.payload, msg.payloadLength, nickname, sizeof(nickname), pubKey)) {
    Serial.printf("[ANNOUNCE_DEBUG] processAnnounce: parseAnnounceTLV success, nickname=%s\n", nickname);
    Serial.println("[ANNOUNCE_DEBUG] processAnnounce: calling cachePeer");
    cachePeer(msg.getSenderId64(), nickname, pubKey);
    Serial.println("[ANNOUNCE_DEBUG] processAnnounce: cachePeer returned");
  } else {
    Serial.println("[ANNOUNCE_DEBUG] processAnnounce: parseAnnounceTLV failed");
  }

  Serial.println("[ANNOUNCE_DEBUG] processAnnounce: exit");
}

void BitchatBLEService::cachePeer(uint64_t peerId, const char *nickname, const uint8_t *pubKey) {
  // Find existing or empty slot
  int emptySlot = -1;
  for (size_t i = 0; i < PEER_CACHE_SIZE; i++) {
    if (_peerCache[i].valid && _peerCache[i].peerId == peerId) {
      // Update existing
      strncpy(_peerCache[i].nickname, nickname, sizeof(_peerCache[i].nickname) - 1);
      if (pubKey) {
        memcpy(_peerCache[i].ed25519PubKey, pubKey, 32);
      }
      _peerCache[i].lastSeen = getCurrentTimeMs();
      return;
    }
    if (!_peerCache[i].valid && emptySlot < 0) {
      emptySlot = static_cast<int>(i);
    }
  }

  // Add new if slot available
  if (emptySlot >= 0) {
    _peerCache[emptySlot].peerId = peerId;
    strncpy(_peerCache[emptySlot].nickname, nickname, sizeof(_peerCache[emptySlot].nickname) - 1);
    if (pubKey) {
      memcpy(_peerCache[emptySlot].ed25519PubKey, pubKey, 32);
    }
    _peerCache[emptySlot].lastSeen = getCurrentTimeMs();
    _peerCache[emptySlot].valid = true;
  }
}

void BitchatBLEService::onCharacteristicCccdWrite(uint16_t conn_handle, BLECharacteristic *chr,
                                                  uint16_t cccd_value) {
#ifdef NRF52_PLATFORM
  // Check if notifications are enabled
  if (_instance != nullptr) {
    bool notificationsEnabled = (cccd_value & BLE_GATT_HVX_NOTIFICATION);
    _instance->_clientSubscribed = notificationsEnabled;

    Serial.printf("[BitChat BLE] CCCD write: notifications %s\n",
                  notificationsEnabled ? "enabled" : "disabled");

    // If client just subscribed and we have pending messages, send them
    // DEFERRED to loop() to avoid calling notify() in ISR
    if (notificationsEnabled && _instance->_hasPendingOutgoing) {
      Serial.println("[BitChat BLE] Client subscribed, pending message will be sent in loop");
    }
  }
#else
  (void)conn_handle;
  (void)chr;
  (void)cccd_value;
#endif
}

void BitchatBLEService::sendPendingOutgoing() {
  if (!_hasPendingOutgoing || !_clientSubscribed) {
    return;
  }

  Serial.println("[BitChat BLE] Sending pending outgoing message");
  _hasPendingOutgoing = false;

  // Use member buffer to avoid stack overflow
  size_t len = serializeMessage(_pendingOutgoing, _tempTxBuffer, sizeof(_tempTxBuffer));

  if (len > 0) {
    broadcastNotification(_tempTxBuffer, len);
  }
}

size_t BitchatBLEService::serializeMessage(const BitchatMessage &msg, uint8_t *buffer, size_t maxLen) {
  if (!buffer || maxLen < BITCHAT_HEADER_SIZE) {
    return 0;
  }

  size_t offset = 0;

  // Header
  buffer[offset++] = msg.version;
  buffer[offset++] = msg.type;
  buffer[offset++] = msg.ttl;

  // Timestamp (8 bytes, big-endian)
  for (int i = 7; i >= 0; i--) {
    buffer[offset++] = static_cast<uint8_t>((msg.timestamp >> (i * 8)) & 0xFF);
  }

  buffer[offset++] = msg.flags;
  buffer[offset++] = static_cast<uint8_t>((msg.payloadLength >> 8) & 0xFF);
  buffer[offset++] = static_cast<uint8_t>(msg.payloadLength & 0xFF);

  // Sender ID
  memcpy(&buffer[offset], msg.senderId, BITCHAT_SENDER_ID_SIZE);
  offset += BITCHAT_SENDER_ID_SIZE;

  // Recipient ID (if present)
  if (msg.hasRecipient()) {
    memcpy(&buffer[offset], msg.recipientId, BITCHAT_RECIPIENT_ID_SIZE);
    offset += BITCHAT_RECIPIENT_ID_SIZE;
  }

  // Payload
  if (msg.payloadLength > 0 && offset + msg.payloadLength <= maxLen) {
    memcpy(&buffer[offset], msg.payload, msg.payloadLength);
    offset += msg.payloadLength;
  }

  // Signature (if present)
  if (msg.hasSignature() && offset + BITCHAT_SIGNATURE_SIZE <= maxLen) {
    memcpy(&buffer[offset], msg.signature, BITCHAT_SIGNATURE_SIZE);
    offset += BITCHAT_SIGNATURE_SIZE;
  }

  return offset;
}

bool BitchatBLEService::parseMessage(const uint8_t *data, size_t len, BitchatMessage &msg) {
  if (!data || len < BITCHAT_HEADER_SIZE) {
    return false;
  }

  size_t offset = 0;

  // Header
  msg.version = data[offset++];
  if (msg.version != BITCHAT_VERSION) {
    return false;
  }

  msg.type = data[offset++];
  msg.ttl = data[offset++];

  // Timestamp (8 bytes, big-endian)
  msg.timestamp = 0;
  for (int i = 0; i < 8; i++) {
    msg.timestamp = (msg.timestamp << 8) | data[offset++];
  }

  msg.flags = data[offset++];
  msg.payloadLength = (static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1];
  offset += 2;

  // Sender ID
  memcpy(msg.senderId, &data[offset], BITCHAT_SENDER_ID_SIZE);
  offset += BITCHAT_SENDER_ID_SIZE;

  // Recipient ID (if present)
  if (msg.hasRecipient()) {
    if (offset + BITCHAT_RECIPIENT_ID_SIZE > len) {
      return false;
    }
    memcpy(msg.recipientId, &data[offset], BITCHAT_RECIPIENT_ID_SIZE);
    offset += BITCHAT_RECIPIENT_ID_SIZE;
  }

  // Payload
  if (msg.payloadLength > 0) {
    if (offset + msg.payloadLength > len || msg.payloadLength > BITCHAT_MAX_PAYLOAD_SIZE) {
      return false;
    }
    memcpy(msg.payload, &data[offset], msg.payloadLength);
    offset += msg.payloadLength;
  }

  // Signature (if present)
  if (msg.hasSignature()) {
    if (offset + BITCHAT_SIGNATURE_SIZE > len) {
      return false;
    }
    memcpy(msg.signature, &data[offset], BITCHAT_SIGNATURE_SIZE);
    offset += BITCHAT_SIGNATURE_SIZE;
  }

  return true;
}

bool BitchatBLEService::parseAnnounceTLV(const uint8_t *payload, size_t len, char *nickname, size_t nickLen,
                                         uint8_t *pubKey) {
  if (!payload || len == 0 || !nickname) {
    return false;
  }

  nickname[0] = '\0';
  if (pubKey) {
    memset(pubKey, 0, 32);
  }

  size_t offset = 0;
  while (offset + 2 <= len) {
    uint8_t type = payload[offset++];
    uint8_t length = payload[offset++];

    if (offset + length > len) {
      break;
    }

    switch (type) {
    case 0x01: // Nickname
      if (length < nickLen) {
        memcpy(nickname, &payload[offset], length);
        nickname[length] = '\0';
      }
      break;

    case 0x03: // Ed25519 public key
      if (pubKey && length == 32) {
        memcpy(pubKey, &payload[offset], 32);
      }
      break;
    }

    offset += length;
  }

  return nickname[0] != '\0';
}

// Test helpers
#ifdef NATIVE_TEST
void BitchatBLEService::queueMessageForTest(const BitchatMessage &msg) {
  if (_recvQueue.size() < MAX_MESSAGE_QUEUE) {
    _recvQueue.push(msg);
  }
}
#endif

} // namespace ble
} // namespace mesh

#endif // ENABLE_BITCHAT
