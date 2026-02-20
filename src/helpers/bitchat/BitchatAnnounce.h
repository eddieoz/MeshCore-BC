#pragma once

/**
 * Story 3.3: MeshCore Identity to BitChat Mapping
 * 
 * Creates BitChat ANNOUNCE messages from MeshCore identity
 * - peer_id: Derived from Ed25519 pubkey
 * - nickname: MeshCore node name
 * - ed25519_pubkey: Full 32-byte public key
 */

#ifdef ENABLE_BITCHAT

#include "BitchatIdentity.h"
#include "../ble/BitchatBLEService.h"
#include <cstdint>
#include <cstring>

namespace mesh {
namespace bitchat {

// ANNOUNCE TLV types
#define BITCHAT_TLV_NICKNAME 0x01
#define BITCHAT_TLV_NOISE_PUBKEY 0x02
#define BITCHAT_TLV_ED25519_PUBKEY 0x03

// Maximum ANNOUNCE payload size
#define BITCHAT_MAX_ANNOUNCE_PAYLOAD 200

/**
 * BitChat ANNOUNCE builder
 * 
 * Creates ANNOUNCE messages from MeshCore identity
 */
class BitchatAnnounceBuilder {
public:
    BitchatAnnounceBuilder();
    
    // Set MeshCore identity info
    void setNodeName(const char* name);
    void setEd25519PubKey(const uint8_t* pubkey);  // 32 bytes
    void setNoisePubKey(const uint8_t* pubkey);    // 32 bytes (optional)
    
    // Build ANNOUNCE payload (TLV format)
    // Returns number of bytes written to buffer
    size_t buildPayload(uint8_t* buffer, size_t maxLen);
    
    // Build complete ANNOUNCE message
    // Returns number of bytes written to buffer
    size_t buildMessage(uint8_t* buffer, size_t maxLen, uint8_t ttl = 8);
    
    // Get derived peer ID
    uint64_t getPeerId() const;
    
    // Check if we have all required fields
    bool isValid() const;

private:
    char _nodeName[32];
    uint8_t _ed25519PubKey[32];
    uint8_t _noisePubKey[32];
    bool _hasEd25519Key;
    bool _hasNoiseKey;
    
    // Write TLV to buffer
    // Returns bytes written
    size_t writeTLV(uint8_t* buffer, uint8_t type, const uint8_t* data, uint8_t len);
};

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
