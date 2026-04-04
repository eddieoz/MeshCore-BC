#pragma once

/**
 * Story 4.4: Hashtag Room Key Derivation
 * 
 * BitChat-Compatible Hashtag Key Derivation
 * - Derives channel key from hashtag name using SHA256
 * - Handles with/without # prefix
 * - Matches BitChat reference implementation
 */

#ifdef ENABLE_BITCHAT

#include <cstdint>
#include <cstring>

namespace mesh {
namespace bitchat {

#define HASHTAG_KEY_SIZE 32

/**
 * Derive channel key from hashtag name
 * 
 * Uses SHA256 of the hashtag string (including # prefix)
 * If input doesn't start with #, it will be added
 * 
 * @param hashtag Channel name (e.g., "#general" or "general")
 * @param outKey 32-byte output buffer for derived key
 * @return true if derivation successful
 */
bool deriveHashtagKey(const char* hashtag, uint8_t outKey[HASHTAG_KEY_SIZE]);

/**
 * Derive channel key from hashtag name (normalized)
 * 
 * Same as deriveHashtagKey but ensures # prefix is present
 * 
 * @param name Channel name (with or without #)
 * @param outKey 32-byte output buffer
 * @return true if derivation successful
 */
bool deriveHashtagKeyNormalized(const char* name, uint8_t outKey[HASHTAG_KEY_SIZE]);

/**
 * Normalize hashtag name
 * 
 * Ensures name starts with # prefix
 * 
 * @param input Input name
 * @param output Output buffer (must be at least 33 bytes)
 * @param outputSize Size of output buffer
 * @return true if normalization successful
 */
bool normalizeHashtag(const char* input, char* output, size_t outputSize);

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
