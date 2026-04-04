/**
 * Story 5.4: BitChat DM Encapsulation - Implementation
 */

#ifdef ENABLE_BITCHAT

#include "DMEncapsulator.h"
#include "../ble/BitchatBLEService.h"
#include <cstring>

namespace mesh {
namespace bitchat {

// ============================================================================
// DM Contact Registry Implementation
// ============================================================================

DMContactRegistry::DMContactRegistry()
    : _count(0)
{
    clear();
}

bool DMContactRegistry::addContact(uint64_t peerId, const uint8_t* ed25519PubKey, const char* nickname)
{
    if (peerId == 0 || ed25519PubKey == nullptr) {
        return false;
    }
    
    // Check if already exists
    int existingSlot = findSlotByPeerId(peerId);
    if (existingSlot >= 0) {
        // Update existing
        DMContact& contact = _contacts[existingSlot];
        memcpy(contact.ed25519PubKey, ed25519PubKey, 32);
        if (nickname != nullptr) {
            strncpy(contact.nickname, nickname, BITCHAT_MAX_CONTACT_NAME_LEN - 1);
            contact.nickname[BITCHAT_MAX_CONTACT_NAME_LEN - 1] = '\0';
        }
        contact.active = true;
        contact.lastSeen = 1234567890;  // Would use actual timestamp
        return true;
    }
    
    // Find empty slot
    int slot = findEmptySlot();
    if (slot < 0) {
        return false;  // Registry full
    }
    
    // Add new contact
    DMContact& contact = _contacts[slot];
    contact.peerId = peerId;
    memcpy(contact.ed25519PubKey, ed25519PubKey, 32);
    if (nickname != nullptr) {
        strncpy(contact.nickname, nickname, BITCHAT_MAX_CONTACT_NAME_LEN - 1);
        contact.nickname[BITCHAT_MAX_CONTACT_NAME_LEN - 1] = '\0';
    } else {
        contact.nickname[0] = '\0';
    }
    contact.active = true;
    contact.lastSeen = 1234567890;  // Would use actual timestamp
    _count++;
    
    return true;
}

const DMContact* DMContactRegistry::findContact(uint64_t peerId) const
{
    int slot = findSlotByPeerId(peerId);
    if (slot >= 0 && _contacts[slot].active) {
        return &_contacts[slot];
    }
    return nullptr;
}

bool DMContactRegistry::removeContact(uint64_t peerId)
{
    int slot = findSlotByPeerId(peerId);
    if (slot >= 0 && _contacts[slot].active) {
        _contacts[slot].active = false;
        _contacts[slot].peerId = 0;
        _count--;
        return true;
    }
    return false;
}

bool DMContactRegistry::hasContact(uint64_t peerId) const
{
    return findContact(peerId) != nullptr;
}

size_t DMContactRegistry::getContactCount() const
{
    return _count;
}

void DMContactRegistry::clear()
{
    for (int i = 0; i < BITCHAT_MAX_DM_CONTACTS; i++) {
        _contacts[i] = DMContact();
    }
    _count = 0;
}

int DMContactRegistry::findSlotByPeerId(uint64_t peerId) const
{
    for (int i = 0; i < BITCHAT_MAX_DM_CONTACTS; i++) {
        if (_contacts[i].active && _contacts[i].peerId == peerId) {
            return i;
        }
    }
    return -1;
}

int DMContactRegistry::findEmptySlot() const
{
    for (int i = 0; i < BITCHAT_MAX_DM_CONTACTS; i++) {
        if (!_contacts[i].active) {
            return i;
        }
    }
    return -1;
}

// ============================================================================
// DM Encapsulator Implementation
// ============================================================================

DMEncapsulator::DMEncapsulator(DMContactRegistry& contacts)
    : _contacts(contacts)
    , _lastStatus(DMStatus::RECIPIENT_UNKNOWN)
{
}

bool DMEncapsulator::encapsulateDM(const mesh::ble::BitchatMessage& msg,
                                    uint64_t senderId,
                                    DMEncapsulationResult& result)
{
    // Reset result
    result = DMEncapsulationResult();
    result.senderId = senderId;
    result.timestamp = 1234567890;  // Would use actual timestamp
    
    // Validate it's a proper DM
    if (!validateDM(msg)) {
        _lastStatus = DMStatus::INVALID_MESSAGE;
        result.status = _lastStatus;
        return false;
    }
    
    // Get recipient ID
    uint64_t recipientId = getRecipientId(msg);
    result.recipientId = recipientId;
    
    // Look up recipient in contact registry
    const DMContact* recipient = _contacts.findContact(recipientId);
    if (recipient == nullptr) {
        _lastStatus = DMStatus::RECIPIENT_UNKNOWN;
        result.status = _lastStatus;
        return false;
    }
    
    // For DM encapsulation, we use the same encapsulation mechanism
    // but with PAYLOAD_TYPE_TXT_MSG and recipient-specific encryption
    // For now, we use the channel encapsulator with a placeholder key
    // In production, this would derive a shared secret via ECDH
    
    // Use recipient's pubkey hash as encryption key (placeholder for ECDH)
    uint8_t sharedSecret[32];
    memcpy(sharedSecret, recipient->ed25519PubKey, 32);
    
    // Determine if fragmentation needed
    size_t messageSize = BITCHAT_HEADER_SIZE + msg.payloadLength;
    bool needsFrag = BitchatMessageEncapsulator::needsFragmentation(messageSize);
    
    bool encapsulationOk;
    if (needsFrag) {
        encapsulationOk = _encapsulator.encapsulateWithFragmentation(
            msg,
            sharedSecret,
            result.packets,
            BITCHAT_MAX_DM_PACKETS,
            result.packetCount
        );
    } else {
        result.packetCount = 1;
        encapsulationOk = _encapsulator.encapsulate(msg, sharedSecret, result.packets[0]);
    }
    
    if (!encapsulationOk) {
        _lastStatus = DMStatus::ENCAPSULATION_FAILED;
        result.status = _lastStatus;
        result.packetCount = 0;
        return false;
    }
    
    // Mark all packets as TXT_MSG type (not GRP_DATA)
    for (size_t i = 0; i < result.packetCount; i++) {
        result.packets[i].payloadType = MeshCorePayloadType::MESHCORE_TXT_MSG;
    }
    
    _lastStatus = DMStatus::ENCAPSULATED;
    result.status = _lastStatus;
    return true;
}

bool DMEncapsulator::validateDM(const mesh::ble::BitchatMessage& msg) const
{
    // Must have recipient flag
    if (!msg.hasRecipient()) {
        return false;
    }
    
    // Must have valid payload
    if (msg.payloadLength == 0 || msg.payloadLength > BITCHAT_MAX_PAYLOAD_SIZE) {
        return false;
    }
    
    // Must be MESSAGE type
    if (msg.type != BITCHAT_MSG_MESSAGE) {
        return false;
    }
    
    return true;
}

uint64_t DMEncapsulator::getRecipientId(const mesh::ble::BitchatMessage& msg) const
{
    return msg.getRecipientId64();
}

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
