/**
 * @file BitchatBridge.h
 * @brief Integration bridge connecting BitChat BLE service to MeshCore mesh
 *
 * This class orchestrates the BitChat components to enable bidirectional
 * message flow between the BitChat app and the MeshCore mesh network.
 */

#ifndef BITCHAT_BRIDGE_H
#define BITCHAT_BRIDGE_H

#include <map>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

#ifdef ENABLE_BITCHAT

// Forward declarations for MeshCore types
namespace mesh {
class Mesh;
class LocalIdentity;
} // namespace mesh

// Include MeshCore headers for GroupChannel
#include <Mesh.h>

// Include BitChat components
#include "../ble/BitchatBLEService.h"
#include "BitchatMessageEncapsulator.h"
#include "ChannelRegistry.h"
#include "FragmentReassembly.h"
#include "LoopPrevention.h"
#include "MessageDecapsulator.h"

/**
 * @brief Integration bridge between BitChat BLE and MeshCore mesh
 */
class BitchatBridge {
public:
  /**
   * @brief Constructor
   * @param mesh Reference to the Mesh instance
   * @param identity Reference to this node's LocalIdentity
   * @param nodeName This node's display name
   */
  BitchatBridge(mesh::Mesh &mesh, mesh::LocalIdentity &identity, const char *nodeName);

  ~BitchatBridge();

  /**
   * @brief Initialize the bridge
   */
  void begin();

  /**
   * @brief Initialize BLE in standalone mode (for nRF52)
   * @param deviceName BLE device name for advertising
   * @return true if successful
   */
  bool beginStandalone(const char *deviceName);

#ifdef NRF52_PLATFORM
  /**
   * @brief Initialize BitChat BLE service on nRF52 Bluefruit
   * Call this AFTER SerialBLEInterface.begin() has initialized Bluefruit
   * @return true if service was added successfully
   */
  bool initNRF52BLEService();
#endif

  /**
   * @brief Main loop - call from main loop()
   */
  void loop();

  /**
   * @brief Check if bridge is initialized
   */
  bool isInitialized() const { return _initialized; }

  /**
   * @brief Get the BLE service
   */
  mesh::ble::BitchatBLEService &getBLEService() { return _bleService; }

  /**
   * @brief Check if BLE service is active
   */
  bool isBLEActive() const;

  /**
   * @brief Check if a BitChat client is connected
   */
  bool hasBitchatClient() const;

  /**
   * @brief Handle incoming mesh channel message (MeshCore → BitChat)
   */
  void onMeshcoreGroupMessage(const mesh::GroupChannel &channel, uint32_t timestamp, const char *senderName,
                              const char *text);

  /**
   * @brief Handle encapsulated incoming MeshCore channel message (MeshCore → BitChat)
   * Decapsulates and forwards to BitChat app
   */
  void onMeshcoreEncapsulatedMessage(const mesh::GroupChannel &channel, uint32_t timestamp,
                                     const char *senderName, const uint8_t *data, size_t len);

  /**
   * @brief Handle incoming BitChat message (BitChat → MeshCore)
   */
  void onBitchatMessageReceived(const mesh::ble::BitchatMessage &msg);

  /**
   * @brief Sign a BitChat message (callback from BLE service)
   */
  void onBitchatSignRequest(uint8_t *sig, const uint8_t *msg, size_t len);

  /**
   * @brief Get statistics
   */
  uint32_t getMessagesRelayed() const { return _messagesRelayed; }
  uint32_t getDuplicatesDropped() const { return _duplicatesDropped; }

  // Test helpers

private:
  // MeshCore references
  mesh::Mesh &_mesh;
  mesh::LocalIdentity &_identity;
  const char *_nodeName;

  // State
  bool _initialized;
  uint64_t _bitchatPeerId;
  uint8_t _noisePublicKey[32];

  // BitChat components
  mesh::bitchat::ChannelRegistry _channelRegistry;
  mesh::bitchat::LoopPrevention _loopPrevention;
  mesh::bitchat::BitchatMessageEncapsulator _encapsulator;
  mesh::bitchat::MessageDecapsulator _decapsulator;
  mesh::bitchat::FragmentReassembly _fragmentReassembly;

  // Temp buffer for decapsulation to avoid stack overflow
  mesh::bitchat::DecapsulationResult _tempDecapsulationResult;

  // BLE service
  mesh::ble::BitchatBLEService _bleService;

  // Statistics
  uint32_t _messagesRelayed;
  uint32_t _duplicatesDropped;

  // Time synchronization
  int64_t _timeOffset;
  bool _timeSynced;

  // Message guard
  bool _processingMessage;

  // Temp message buffer to avoid stack allocation in loop()
  // BitchatMessage is ~2KB, allocating on stack can cause overflow
  mesh::ble::BitchatMessage _tempLoopMessage;

  // Cached mesh channel (set when receiving from mesh, used when sending to mesh)
  mesh::GroupChannel _meshChannel;
  bool _hasMeshChannel;

  // Queue for outgoing messages (MeshCore -> BitChat) to avoid stack overflow in callback chain
  struct OutgoingMessage {
    char sender[32];
    char text[180]; // Max payload is ~180
    uint32_t timestamp;
  };
  std::vector<OutgoingMessage>
      _outgoingQueue; // Using vector as simple queue to avoid <queue> header dependency if not already there

  // Helpers to sign BitChat messages
  void signBitChatMessage(mesh::ble::BitchatMessage &msg);

  // Buffer for signing (avoid stack allocation of ~2KB)
  uint8_t _signingBuffer[2048 + 64];

  // Helpers
  bool isMeshChannel(const mesh::GroupChannel &channel) const;
  uint64_t derivePeerId(const mesh::LocalIdentity &identity);
  uint64_t getCurrentTimeMs() const;
  mesh::GroupChannel *getMeshChannel();
};

#endif // ENABLE_BITCHAT
#endif // BITCHAT_BRIDGE_H
