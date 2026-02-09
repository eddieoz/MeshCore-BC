#pragma once

/**
 * Story 3.1: BitChat Peer ID Derivation
 * 
 * Derives BitChat peer IDs from MeshCore Ed25519 keys
 * - 8-byte peer ID from first 8 bytes of public key (little-endian)
 * - Consistent with BitChat app expectations
 */

#ifdef ENABLE_BITCHAT

#include <cstdint>
#include <cstring>

namespace mesh {
namespace bitchat {

// BitChat uses 8-byte peer IDs
#define BITCHAT_PEER_ID_SIZE 8

/**
 * Derive BitChat peer ID from Ed25519 public key
 * 
 * Takes first 8 bytes of 32-byte Ed25519 public key
 * Returns as little-endian uint64_t
 * 
 * @param ed25519PubKey 32-byte Ed25519 public key
 * @return 8-byte peer ID as uint64_t (little-endian)
 */
uint64_t deriveBitChatPeerId(const uint8_t* ed25519PubKey);

/**
 * Convert peer ID to bytes (little-endian)
 * 
 * @param peerId 8-byte peer ID
 * @param outBytes 8-byte output buffer
 */
void peerIdToBytes(uint64_t peerId, uint8_t* outBytes);

/**
 * Convert bytes to peer ID (little-endian)
 * 
 * @param bytes 8-byte input buffer
 * @return 8-byte peer ID as uint64_t
 */
uint64_t bytesToPeerId(const uint8_t* bytes);

/**
 * Verify peer ID matches public key
 * 
 * @param peerId 8-byte peer ID
 * @param ed25519PubKey 32-byte Ed25519 public key
 * @return true if peer ID matches first 8 bytes of pubkey
 */
bool verifyPeerId(uint64_t peerId, const uint8_t* ed25519PubKey);

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
