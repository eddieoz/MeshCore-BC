#include "BitchatIdentity.h"

#ifdef NATIVE_TEST
  // Use SHA256 implementation from test/mocks/sha256_impl.c
  extern "C" void sha256_hash(const uint8_t *data, size_t len, uint8_t out[32]);
  #include "../test/mocks/sha256_impl.c"
#else
  #include "../../MeshCore.h"
  #include "../../Utils.h"
#endif

#ifdef ENABLE_BITCHAT

namespace mesh {
namespace bitchat {

uint64_t deriveBitChatPeerId(const uint8_t *ed25519PubKey) {
  if (!ed25519PubKey) {
    return 0;
  }

  // BitChat uses the first 8 bytes of the SHA-256 hash of the public key
  uint8_t hash[32];
#ifdef NATIVE_TEST
  sha256_hash(ed25519PubKey, 32, hash);
#else
  mesh::Utils::sha256(hash, sizeof(hash), ed25519PubKey, 32);
#endif

  // Interpret first 8 bytes as a little-endian uint64_t
  // This ensures that when serialized back to bytes, it matches the original hash order
  uint64_t peerId = 0;
  for (int i = 0; i < BITCHAT_PEER_ID_SIZE; i++) {
    peerId |= (static_cast<uint64_t>(hash[i]) << (i * 8));
  }

  return peerId;
}

void peerIdToBytes(uint64_t peerId, uint8_t *outBytes) {
  if (!outBytes) {
    return;
  }

  // Write as little-endian
  for (int i = 0; i < BITCHAT_PEER_ID_SIZE; i++) {
    outBytes[i] = static_cast<uint8_t>((peerId >> (i * 8)) & 0xFF);
  }
}

uint64_t bytesToPeerId(const uint8_t *bytes) {
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

bool verifyPeerId(uint64_t peerId, const uint8_t *ed25519PubKey) {
  if (!ed25519PubKey) {
    return false;
  }

  uint64_t expectedPeerId = deriveBitChatPeerId(ed25519PubKey);
  return peerId == expectedPeerId;
}

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
