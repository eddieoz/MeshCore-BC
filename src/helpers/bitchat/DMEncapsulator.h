#pragma once

/**
 * Story 5.4: BitChat DM Encapsulation (BitChat → MeshCore)
 * 
 * Encapsulates BitChat direct messages for MeshCore transport
 * using PAYLOAD_TYPE_TXT_MSG with E2E encryption
 * 
 * Features:
 * - Map BitChat recipient_id to MeshCore identity
 * - Use PAYLOAD_TYPE_TXT_MSG (not GRP_DATA)
 * - E2E encryption with recipient's shared secret
 * - Fragmentation for large DMs
 */

#ifdef ENABLE_BITCHAT

#include "BitchatIdentity.h"
#include "BitchatMessageEncapsulator.h"
#include <cstdint>
#include <cstring>

namespace mesh {
namespace bitchat {

// Maximum DM contacts in registry
#define BITCHAT_MAX_DM_CONTACTS 32

// Maximum DM packets (for fragmentation)
#define BITCHAT_MAX_DM_PACKETS 4

// Contact name max length
#define BITCHAT_MAX_CONTACT_NAME_LEN 32

/**
 * DM status codes
 */
enum class DMStatus : uint8_t {
    ENCAPSULATED = 0,       // Successfully encapsulated
    RECIPIENT_UNKNOWN = 1,  // Recipient not in contact registry
    INVALID_MESSAGE = 2,    // Not a valid DM (no recipient flag)
    ENCAPSULATION_FAILED = 3, // Encryption/packing failed
    CONTACT_REGISTRY_FULL = 4 // Cannot add more contacts
};

/**
 * DM Contact entry
 */
struct DMContact {
    uint64_t peerId;
    uint8_t ed25519PubKey[32];
    char nickname[BITCHAT_MAX_CONTACT_NAME_LEN];
    bool active;
    uint32_t lastSeen;
    
    DMContact()
        : peerId(0)
        , active(false)
        , lastSeen(0)
    {
        memset(ed25519PubKey, 0, sizeof(ed25519PubKey));
        memset(nickname, 0, sizeof(nickname));
    }
};

/**
 * DM Contact Registry
 * 
 * Maps peer IDs to contact information for DM routing
 */
class DMContactRegistry {
public:
    DMContactRegistry();
    
    /**
     * Add a contact to the registry
     * 
     * @param peerId 8-byte BitChat peer ID
     * @param ed25519PubKey 32-byte Ed25519 public key
     * @param nickname Contact nickname
     * @return true if added, false if registry full
     */
    bool addContact(uint64_t peerId, const uint8_t* ed25519PubKey, const char* nickname);
    
    /**
     * Find contact by peer ID
     * 
     * @param peerId 8-byte peer ID
     * @return Pointer to contact or nullptr if not found
     */
    const DMContact* findContact(uint64_t peerId) const;
    
    /**
     * Remove a contact
     * 
     * @param peerId 8-byte peer ID
     * @return true if removed, false if not found
     */
    bool removeContact(uint64_t peerId);
    
    /**
     * Check if contact exists
     */
    bool hasContact(uint64_t peerId) const;
    
    /**
     * Get contact count
     */
    size_t getContactCount() const;
    
    /**
     * Clear all contacts
     */
    void clear();

private:
    DMContact _contacts[BITCHAT_MAX_DM_CONTACTS];
    size_t _count;
    
    int findSlotByPeerId(uint64_t peerId) const;
    int findEmptySlot() const;
};

/**
 * DM Encapsulation Result
 */
struct DMEncapsulationResult {
    DMStatus status;
    EncapsulatedPacket packets[BITCHAT_MAX_DM_PACKETS];
    size_t packetCount;
    uint64_t recipientId;
    uint64_t senderId;
    uint32_t timestamp;
    
    DMEncapsulationResult()
        : status(DMStatus::RECIPIENT_UNKNOWN)
        , packetCount(0)
        , recipientId(0)
        , senderId(0)
        , timestamp(0)
    {
    }
};

/**
 * DM Encapsulator
 * 
 * Encapsulates BitChat DMs for MeshCore transport
 */
class DMEncapsulator {
public:
    /**
     * Constructor
     * @param contacts Contact registry for lookups
     */
    explicit DMEncapsulator(DMContactRegistry& contacts);
    
    /**
     * Encapsulate a BitChat DM for MeshCore transport
     * 
     * @param msg BitChat DM message (must have HAS_RECIPIENT flag)
     * @param senderId Verified sender peer ID
     * @param result Output encapsulation result
     * @return true if encapsulated successfully
     * @return false if failed (recipient unknown, invalid message)
     */
    bool encapsulateDM(const mesh::ble::BitchatMessage& msg,
                       uint64_t senderId,
                       DMEncapsulationResult& result);
    
    /**
     * Get last status code
     */
    DMStatus getLastStatus() const { return _lastStatus; }

private:
    DMContactRegistry& _contacts;
    DMStatus _lastStatus;
    BitchatMessageEncapsulator _encapsulator;
    
    /**
     * Validate DM message
     */
    bool validateDM(const mesh::ble::BitchatMessage& msg) const;
    
    /**
     * Get recipient ID from message
     */
    uint64_t getRecipientId(const mesh::ble::BitchatMessage& msg) const;
};

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
