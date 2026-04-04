#include "BitchatAnnounce.h"

#ifdef ENABLE_BITCHAT

namespace mesh {
namespace bitchat {

BitchatAnnounceBuilder::BitchatAnnounceBuilder()
    : _hasEd25519Key(false)
    , _hasNoiseKey(false)
{
    memset(_nodeName, 0, sizeof(_nodeName));
    memset(_ed25519PubKey, 0, sizeof(_ed25519PubKey));
    memset(_noisePubKey, 0, sizeof(_noisePubKey));
}

void BitchatAnnounceBuilder::setNodeName(const char* name) {
    if (name) {
        strncpy(_nodeName, name, sizeof(_nodeName) - 1);
        _nodeName[sizeof(_nodeName) - 1] = '\0';
    }
}

void BitchatAnnounceBuilder::setEd25519PubKey(const uint8_t* pubkey) {
    if (pubkey) {
        memcpy(_ed25519PubKey, pubkey, 32);
        _hasEd25519Key = true;
    }
}

void BitchatAnnounceBuilder::setNoisePubKey(const uint8_t* pubkey) {
    if (pubkey) {
        memcpy(_noisePubKey, pubkey, 32);
        _hasNoiseKey = true;
    }
}

uint64_t BitchatAnnounceBuilder::getPeerId() const {
    if (!_hasEd25519Key) {
        return 0;
    }
    return deriveBitChatPeerId(_ed25519PubKey);
}

bool BitchatAnnounceBuilder::isValid() const {
    return _hasEd25519Key && _nodeName[0] != '\0';
}

size_t BitchatAnnounceBuilder::buildPayload(uint8_t* buffer, size_t maxLen) {
    if (!buffer || maxLen < 3) {
        return 0;
    }
    
    size_t offset = 0;
    
    // TLV: Nickname (type 0x01)
    size_t nickLen = strlen(_nodeName);
    if (nickLen > 0 && offset + 2 + nickLen <= maxLen) {
        offset += writeTLV(&buffer[offset], BITCHAT_TLV_NICKNAME, 
                          (const uint8_t*)_nodeName, (uint8_t)nickLen);
    }
    
    // TLV: Ed25519 Public Key (type 0x03)
    if (_hasEd25519Key && offset + 2 + 32 <= maxLen) {
        offset += writeTLV(&buffer[offset], BITCHAT_TLV_ED25519_PUBKEY, 
                          _ed25519PubKey, 32);
    }
    
    // TLV: Noise Public Key (type 0x02) - optional
    if (_hasNoiseKey && offset + 2 + 32 <= maxLen) {
        offset += writeTLV(&buffer[offset], BITCHAT_TLV_NOISE_PUBKEY, 
                          _noisePubKey, 32);
    }
    
    return offset;
}

size_t BitchatAnnounceBuilder::buildMessage(uint8_t* buffer, size_t maxLen, uint8_t ttl) {
    if (!buffer || maxLen < BITCHAT_HEADER_SIZE + 1) {
        return 0;
    }
    
    if (!isValid()) {
        return 0;
    }
    
    size_t offset = 0;
    
    // Header (14 bytes)
    buffer[offset++] = BITCHAT_VERSION;        // version
    buffer[offset++] = BITCHAT_MSG_ANNOUNCE;   // type
    buffer[offset++] = ttl;                    // ttl
    
    // Timestamp (8 bytes, big-endian) - 0 for now
    for (int i = 0; i < 8; i++) {
        buffer[offset++] = 0;
    }
    
    // Flags (no signature for now)
    buffer[offset++] = 0;
    
    // Payload length placeholder (2 bytes, big-endian)
    size_t payloadLenOffset = offset;
    offset += 2;
    
    // Sender ID (8 bytes, little-endian)
    uint64_t peerId = getPeerId();
    for (int i = 0; i < 8; i++) {
        buffer[offset++] = (peerId >> (i * 8)) & 0xFF;
    }
    
    // Payload
    size_t payloadStart = offset;
    size_t payloadLen = buildPayload(&buffer[offset], maxLen - offset);
    offset += payloadLen;
    
    // Fill in payload length
    buffer[payloadLenOffset] = (payloadLen >> 8) & 0xFF;
    buffer[payloadLenOffset + 1] = payloadLen & 0xFF;
    
    return offset;
}

size_t BitchatAnnounceBuilder::writeTLV(uint8_t* buffer, uint8_t type, const uint8_t* data, uint8_t len) {
    buffer[0] = type;
    buffer[1] = len;
    memcpy(&buffer[2], data, len);
    return 2 + len;
}

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
