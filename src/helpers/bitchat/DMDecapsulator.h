#pragma once

/**
 * Story 6.2: MeshCore DM Decapsulation
 * 
 * Decapsulates BitChat DMs from MeshCore PAYLOAD_TYPE_TXT_MSG packets
 * Detects BitChat encapsulation in DM payload
 * Maps MeshCore contact to BitChat peer_id
 */

#ifdef ENABLE_BITCHAT

#include "DMEncapsulator.h"
#include "MessageDecapsulator.h"
#include <cstdint>
#include <cstring>

namespace mesh {
namespace bitchat {

/**
 * DM Decapsulation status codes
 */
enum class DMDecapsulationStatus : uint8_t {
    SUCCESS = 0,            // Successfully decapsulated
    NOT_BITCHAT = 1,        // Not a BitChat encapsulated DM
    INVALID_PACKET = 2,     // Packet too short or malformed
    VERSION_MISMATCH = 3,   // Unsupported encapsulation version
    UNKNOWN_SENDER = 4,     // Sender not in contact registry
    DECRYPTION_FAILED = 5   // Decryption error
};

/**
 * DM Decapsulation result
 */
struct DMDecapsulationResult {
    DMDecapsulationStatus status;
    mesh::ble::BitchatMessage message;
    uint64_t senderId;       // Sender peer ID
    bool isKnownContact;     // Sender is in contact registry
    char senderNickname[BITCHAT_MAX_CONTACT_NAME_LEN];
    uint32_t timestamp;
    
    DMDecapsulationResult()
        : status(DMDecapsulationStatus::NOT_BITCHAT)
        , senderId(0)
        , isKnownContact(false)
        , timestamp(0)
    {
        memset(senderNickname, 0, sizeof(senderNickname));
    }
};

/**
 * DM Decapsulator
 * 
 * Decapsulates BitChat DMs from MeshCore TXT_MSG packets
 */
class DMDecapsulator {
public:
    /**
     * Constructor
     * @param contacts Contact registry for sender lookups
     */
    explicit DMDecapsulator(DMContactRegistry& contacts);
    
    /**
     * Decapsulate a DM packet
     * 
     * @param packet Encapsulated packet from mesh
     * @param senderId Sender peer ID
     * @param recipientKey Recipient's decryption key
     * @param result Output decapsulation result
     * @return true if successfully decapsulated
     * @return false if not a BitChat DM or error
     */
    bool decapsulateDM(const EncapsulatedPacket& packet,
                       uint64_t senderId,
                       const uint8_t* recipientKey,
                       DMDecapsulationResult& result);
    
    /**
     * Get last status
     */
    DMDecapsulationStatus getLastStatus() const { return _lastStatus; }

private:
    DMContactRegistry& _contacts;
    DMDecapsulationStatus _lastStatus;
    
    /**
     * Check if data has BitChat encapsulation magic
     */
    bool isBitChatEncapsulated(const uint8_t* data, size_t len) const;
    
    /**
     * Decrypt payload
     */
    void decryptPayload(const uint8_t* input, size_t len,
                        const uint8_t* key,
                        uint8_t* output) const;
    
    /**
     * Deserialize BitChat message
     */
    bool deserializeMessage(const uint8_t* data, size_t len,
                            mesh::ble::BitchatMessage& msgOut);
};

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
