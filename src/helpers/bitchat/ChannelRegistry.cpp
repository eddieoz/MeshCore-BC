#include "ChannelRegistry.h"

#ifdef ENABLE_BITCHAT

// For SHA256 - use rweather/Crypto library on embedded, or simple implementation on native
#ifdef NATIVE_TEST
  // Simple SHA256 implementation for native tests - embedded here to avoid linking issues
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
  
  static void simple_sha256(const uint8_t* data, size_t len, uint8_t hash[32]) {
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

ChannelRegistry::ChannelRegistry() {
    clear();
}

bool ChannelRegistry::registerChannel(const char* name, const uint8_t* channelKey, ChannelType type) {
    if (!name || !channelKey) {
        return false;
    }
    
    // Check if already registered
    int existingSlot = findSlotByName(name);
    if (existingSlot >= 0) {
        // Update existing
        ChannelMapping& ch = _channels[existingSlot];
        memcpy(ch.channelKey, channelKey, BITCHAT_CHANNEL_KEY_SIZE);
        ch.type = type;
        ch.active = true;
        return true;
    }
    
    // Find empty slot
    int slot = findEmptySlot();
    if (slot < 0) {
        return false;  // Registry full
    }
    
    // Add new channel
    ChannelMapping& ch = _channels[slot];
    strncpy(ch.name, name, BITCHAT_MAX_CHANNEL_NAME_LEN - 1);
    ch.name[BITCHAT_MAX_CHANNEL_NAME_LEN - 1] = '\0';
    memcpy(ch.channelKey, channelKey, BITCHAT_CHANNEL_KEY_SIZE);
    ch.type = type;
    ch.active = true;
    ch.createdAt = 0;  // Could use millis() here
    
    return true;
}

bool ChannelRegistry::unregisterChannel(const char* name) {
    int slot = findSlotByName(name);
    if (slot >= 0) {
        _channels[slot] = ChannelMapping();  // Reset to default
        return true;
    }
    return false;
}

const uint8_t* ChannelRegistry::lookupByName(const char* name) const {
    const ChannelMapping* ch = findChannel(name);
    if (ch && ch->active) {
        return ch->channelKey;
    }
    return nullptr;
}

const char* ChannelRegistry::lookupByKey(const uint8_t* channelKey) const {
    if (!channelKey) {
        return nullptr;
    }
    
    int slot = findSlotByKey(channelKey);
    if (slot >= 0 && _channels[slot].active) {
        return _channels[slot].name;
    }
    return nullptr;
}

const ChannelMapping* ChannelRegistry::findChannel(const char* name) const {
    int slot = findSlotByName(name);
    if (slot >= 0) {
        return &_channels[slot];
    }
    return nullptr;
}

size_t ChannelRegistry::getChannelCount() const {
    size_t count = 0;
    for (int i = 0; i < BITCHAT_MAX_CHANNEL_MAPPINGS; i++) {
        if (_channels[i].active) {
            count++;
        }
    }
    return count;
}

bool ChannelRegistry::isFull() const {
    return getChannelCount() >= BITCHAT_MAX_CHANNEL_MAPPINGS;
}

bool ChannelRegistry::hasChannel(const char* name) const {
    return findSlotByName(name) >= 0;
}

void ChannelRegistry::clear() {
    for (int i = 0; i < BITCHAT_MAX_CHANNEL_MAPPINGS; i++) {
        _channels[i] = ChannelMapping();
    }
}

const ChannelMapping* ChannelRegistry::getChannel(size_t index) const {
    if (index < BITCHAT_MAX_CHANNEL_MAPPINGS && _channels[index].active) {
        return &_channels[index];
    }
    return nullptr;
}

bool ChannelRegistry::deriveKeyFromHashtag(const char* hashtag, uint8_t* outKey) {
    if (!hashtag || !outKey) {
        return false;
    }
    
#ifdef NATIVE_TEST
    // Simple SHA256 for native tests
    simple_sha256((const uint8_t*)hashtag, strlen(hashtag), outKey);
    return true;
#else
    // Use Arduino Crypto library
    SHA256 sha256;
    sha256.update(hashtag, strlen(hashtag));
    sha256.finalize(outKey, 32);
    return true;
#endif
}

bool ChannelRegistry::registerDefaultMeshChannel() {
    uint8_t meshKey[BITCHAT_CHANNEL_KEY_SIZE];
    if (!deriveKeyFromHashtag("#mesh", meshKey)) {
        return false;
    }
    return registerChannel("mesh", meshKey, ChannelType::HASHTAG);
}

int ChannelRegistry::findSlotByName(const char* name) const {
    for (int i = 0; i < BITCHAT_MAX_CHANNEL_MAPPINGS; i++) {
        if (_channels[i].active && strcmp(_channels[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

int ChannelRegistry::findEmptySlot() const {
    for (int i = 0; i < BITCHAT_MAX_CHANNEL_MAPPINGS; i++) {
        if (!_channels[i].active) {
            return i;
        }
    }
    return -1;
}

int ChannelRegistry::findSlotByKey(const uint8_t* channelKey) const {
    for (int i = 0; i < BITCHAT_MAX_CHANNEL_MAPPINGS; i++) {
        if (_channels[i].active && 
            memcmp(_channels[i].channelKey, channelKey, BITCHAT_CHANNEL_KEY_SIZE) == 0) {
            return i;
        }
    }
    return -1;
}

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
