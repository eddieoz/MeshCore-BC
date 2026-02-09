#include "BitchatIdentity.h"

#ifdef ENABLE_BITCHAT

namespace mesh {
namespace bitchat {

uint64_t deriveBitChatPeerId(const uint8_t* ed25519PubKey) {
    if (!ed25519PubKey) {
        return 0;
    }
    
    // Take first 8 bytes of Ed25519 public key
    // Interpret as little-endian uint64_t (BitChat convention)
    uint64_t peerId = 0;
    for (int i = 0; i < BITCHAT_PEER_ID_SIZE; i++) {
        peerId |= (static_cast<uint64_t>(ed25519PubKey[i]) << (i * 8));
    }
    
    return peerId;
}

void peerIdToBytes(uint64_t peerId, uint8_t* outBytes) {
    if (!outBytes) {
        return;
    }
    
    // Write as little-endian
    for (int i = 0; i < BITCHAT_PEER_ID_SIZE; i++) {
        outBytes[i] = static_cast<uint8_t>((peerId >> (i * 8)) & 0xFF);
    }
}

uint64_t bytesToPeerId(const uint8_t* bytes) {
    if (!bytes) {
        return 0;
    }
    
    // Read as little-endian
    uint64_t peerId = 0;
    for (int i = 0; i < BITCHAT_PEER_ID_SIZE; i++) {
        peerId |= (static_cast<uint64_t>(bytes[i]) << (i * 8));
    }
    
    return peerId;
}

bool verifyPeerId(uint64_t peerId, const uint8_t* ed25519PubKey) {
    if (!ed25519PubKey) {
        return false;
    }
    
    uint64_t expectedPeerId = deriveBitChatPeerId(ed25519PubKey);
    return peerId == expectedPeerId;
}

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
