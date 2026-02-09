#pragma once

/**
 * Story 6.1: MeshCore Group Message Decapsulation
 * 
 * Decapsulates BitChat messages from MeshCore packets
 * Detects BitChat encapsulation by magic header
 * Supports fragment reassembly
 */

#ifdef ENABLE_BITCHAT

#include "ChannelRegistry.h"
#include "BitchatMessageEncapsulator.h"
#include <cstdint>
#include <cstring>

namespace mesh {
namespace bitchat {

/**
 * Decapsulation status codes
 */
enum class DecapsulationStatus : uint8_t {
    SUCCESS = 0,           // Successfully decapsulated
    NOT_BITCHAT = 1,       // Not a BitChat encapsulated message
    INVALID_PACKET = 2,    // Packet too short or malformed
    VERSION_MISMATCH = 3,  // Unsupported encapsulation version
    DECRYPTION_FAILED = 4, // Decryption error
    DESERIALIZATION_FAILED = 5 // Failed to deserialize message
};

/**
 * Decapsulation result
 */
struct DecapsulationResult {
    DecapsulationStatus status;
    mesh::ble::BitchatMessage message;
    uint32_t messageId;      // For fragment tracking
    uint8_t fragmentNum;
    uint8_t totalFragments;
    bool isFragment;
    
    DecapsulationResult()
        : status(DecapsulationStatus::NOT_BITCHAT)
        , messageId(0)
        , fragmentNum(0)
        , totalFragments(1)
        , isFragment(false)
    {
    }
};

/**
 * Message Decapsulator
 * 
 * Decapsulates BitChat messages from MeshCore packets
 */
class MessageDecapsulator {
public:
    /**
     * Constructor
     * @param registry Channel registry for channel lookups
     */
    explicit MessageDecapsulator(ChannelRegistry& registry);
    
    /**
     * Decapsulate a received packet
     * 
     * @param packet Encapsulated packet from mesh
     * @param channelKey Channel key for decryption
     * @param result Output decapsulation result
     * @return true if successfully decapsulated
     * @return false if not a BitChat message or error
     */
    bool decapsulate(const EncapsulatedPacket& packet,
                     const uint8_t* channelKey,
                     DecapsulationResult& result);
    
    /**
     * Check if data has BitChat encapsulation magic
     * 
     * @param data Data to check
     * @param len Data length
     * @return true if BitChat encapsulated
     */
    bool isBitChatEncapsulated(const uint8_t* data, size_t len) const;
    
    /**
     * Get last status
     */
    DecapsulationStatus getLastStatus() const { return _lastStatus; }

private:
    ChannelRegistry& _registry;
    DecapsulationStatus _lastStatus;
    
    /**
     * Decrypt payload using channel key
     */
    void decryptPayload(const uint8_t* input, size_t len,
                        const uint8_t* key,
                        uint8_t* output) const;
    
    /**
     * Deserialize BitChat message from decrypted data
     */
    bool deserializeMessage(const uint8_t* data, size_t len,
                            mesh::ble::BitchatMessage& msgOut);
};

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
