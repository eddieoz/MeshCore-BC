#include "HashtagKey.h"

#ifdef ENABLE_BITCHAT

#ifdef NATIVE_TEST
  // Simple SHA256 implementation for native tests (same as ChannelRegistry)
  #include <cstdint>
  #include <cstring>
  
  static const uint32_t K256[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
  };
  
  static inline uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
  static inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (~x & z); }
  static inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
  static inline uint32_t ep0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
  static inline uint32_t ep1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
  static inline uint32_t sig0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
  static inline uint32_t sig1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }
  
  static void sha256_transform(uint32_t state[8], const uint8_t data[64]) {
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t e = state[4], f = state[5], g = state[6], h = state[7];
    uint32_t m[64];
    
    for (int i = 0, j = 0; i < 16; ++i, j += 4)
      m[i] = ((uint32_t)data[j] << 24) | ((uint32_t)data[j+1] << 16) | ((uint32_t)data[j+2] << 8) | ((uint32_t)data[j+3]);
    for (int i = 16; i < 64; ++i)
      m[i] = sig1(m[i-2]) + m[i-7] + sig0(m[i-15]) + m[i-16];
    
    for (int i = 0; i < 64; ++i) {
      uint32_t t1 = h + ep1(e) + ch(e, f, g) + K256[i] + m[i];
      uint32_t t2 = ep0(a) + maj(a, b, c);
      h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
    }
    
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
  }
  
  static void sha256_compute(const uint8_t* data, size_t len, uint8_t hash[32]) {
    uint32_t state[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    uint8_t buf[64];
    size_t i;
    
    for (i = 0; i < len; ++i) {
      size_t idx = i % 64;
      buf[idx] = data[i];
      if (idx == 63) sha256_transform(state, buf);
    }
    
    size_t r = len % 64;
    buf[r] = 0x80;
    for (size_t j = r + 1; j < 64; ++j) buf[j] = 0;
    
    if (r >= 56) {
      sha256_transform(state, buf);
      memset(buf, 0, 64);
    }
    
    uint64_t bitlen = (uint64_t)len * 8;
    for (int j = 0; j < 8; ++j) buf[63-j] = (uint8_t)(bitlen >> (j * 8));
    sha256_transform(state, buf);
    
    for (i = 0; i < 8; ++i) {
      hash[i*4] = (uint8_t)(state[i] >> 24);
      hash[i*4+1] = (uint8_t)(state[i] >> 16);
      hash[i*4+2] = (uint8_t)(state[i] >> 8);
      hash[i*4+3] = (uint8_t)(state[i]);
    }
  }
#else
  #include <Crypto.h>
  #include <SHA256.h>
#endif

namespace mesh {
namespace bitchat {

bool normalizeHashtag(const char* input, char* output, size_t outputSize) {
    if (!input || !output || outputSize < 2) {
        return false;
    }
    
    // Check if already has # prefix
    if (input[0] == '#') {
        // Already has prefix, just copy
        strncpy(output, input, outputSize - 1);
        output[outputSize - 1] = '\0';
    } else {
        // Add # prefix
        if (outputSize <= strlen(input) + 1) {
            return false;  // Buffer too small
        }
        output[0] = '#';
        strncpy(output + 1, input, outputSize - 2);
        output[outputSize - 1] = '\0';
    }
    
    return true;
}

bool deriveHashtagKey(const char* hashtag, uint8_t outKey[HASHTAG_KEY_SIZE]) {
    if (!hashtag || !outKey) {
        return false;
    }
    
    // Normalize to ensure # prefix
    char normalized[64];
    if (!normalizeHashtag(hashtag, normalized, sizeof(normalized))) {
        return false;
    }
    
#ifdef NATIVE_TEST
    sha256_compute((const uint8_t*)normalized, strlen(normalized), outKey);
    return true;
#else
    SHA256 sha256;
    sha256.update(normalized, strlen(normalized));
    sha256.finalize(outKey, HASHTAG_KEY_SIZE);
    return true;
#endif
}

bool deriveHashtagKeyNormalized(const char* name, uint8_t outKey[HASHTAG_KEY_SIZE]) {
    return deriveHashtagKey(name, outKey);
}

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
