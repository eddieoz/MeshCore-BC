#include "BitchatPeerCache.h"

#ifdef ENABLE_BITCHAT

#ifndef NATIVE_TEST
  #ifdef ESP32
    #include <Arduino.h>
  #elif defined(NRF52_PLATFORM)
    #include <Arduino.h>
  #endif
#endif

namespace mesh {
namespace bitchat {

BitchatPeerCache::BitchatPeerCache()
    : _accessCounter(0)
{
    clear();
}

bool BitchatPeerCache::addOrUpdatePeer(uint64_t peerId, const char* nickname, const uint8_t* pubkey) {
    if (!nickname || !pubkey) {
        return false;
    }
    
    // Check if peer already exists
    int slot = findSlot(peerId);
    
    if (slot >= 0) {
        // Update existing entry
        PeerCacheEntry& entry = _entries[slot];
        strncpy(entry.nickname, nickname, BITCHAT_MAX_NICKNAME_LEN - 1);
        entry.nickname[BITCHAT_MAX_NICKNAME_LEN - 1] = '\0';
        memcpy(entry.pubkey, pubkey, BITCHAT_PUBKEY_SIZE);
        entry.lastSeen = getCurrentTimestamp();
        entry.accessCount = ++_accessCounter;
        entry.valid = true;
        return true;
    }
    
    // Find empty slot
    slot = findEmptySlot();
    
    if (slot < 0) {
        // Cache full - evict LRU entry
        slot = findLRUVictim();
        if (slot < 0) {
            return false;  // Shouldn't happen unless cache is broken
        }
    }
    
    // Add new entry
    PeerCacheEntry& entry = _entries[slot];
    entry.peerId = peerId;
    strncpy(entry.nickname, nickname, BITCHAT_MAX_NICKNAME_LEN - 1);
    entry.nickname[BITCHAT_MAX_NICKNAME_LEN - 1] = '\0';
    memcpy(entry.pubkey, pubkey, BITCHAT_PUBKEY_SIZE);
    entry.lastSeen = getCurrentTimestamp();
    entry.accessCount = ++_accessCounter;
    entry.valid = true;
    
    return true;
}

const PeerCacheEntry* BitchatPeerCache::findPeer(uint64_t peerId) {
    int slot = findSlot(peerId);
    if (slot >= 0) {
        _entries[slot].accessCount = ++_accessCounter;
        return &_entries[slot];
    }
    return nullptr;
}

const PeerCacheEntry* BitchatPeerCache::findPeerByNickname(const char* nickname) {
    if (!nickname) {
        return nullptr;
    }
    
    for (int i = 0; i < BITCHAT_PEER_CACHE_SIZE; i++) {
        if (_entries[i].valid && strcmp(_entries[i].nickname, nickname) == 0) {
            _entries[i].accessCount = ++_accessCounter;
            return &_entries[i];
        }
    }
    return nullptr;
}

bool BitchatPeerCache::removePeer(uint64_t peerId) {
    int slot = findSlot(peerId);
    if (slot >= 0) {
        _entries[slot] = PeerCacheEntry();  // Reset to default
        return true;
    }
    return false;
}

size_t BitchatPeerCache::getEntryCount() const {
    size_t count = 0;
    for (int i = 0; i < BITCHAT_PEER_CACHE_SIZE; i++) {
        if (_entries[i].valid) {
            count++;
        }
    }
    return count;
}

bool BitchatPeerCache::isFull() const {
    return getEntryCount() >= BITCHAT_PEER_CACHE_SIZE;
}

void BitchatPeerCache::clear() {
    for (int i = 0; i < BITCHAT_PEER_CACHE_SIZE; i++) {
        _entries[i] = PeerCacheEntry();
    }
    _accessCounter = 0;
}

int BitchatPeerCache::findLRUVictim() const {
    int victim = -1;
    uint32_t lowestAccess = 0xFFFFFFFF;
    
    for (int i = 0; i < BITCHAT_PEER_CACHE_SIZE; i++) {
        if (_entries[i].valid && _entries[i].accessCount < lowestAccess) {
            lowestAccess = _entries[i].accessCount;
            victim = i;
        }
    }
    
    return victim;
}

const PeerCacheEntry* BitchatPeerCache::getEntry(size_t index) const {
    if (index < BITCHAT_PEER_CACHE_SIZE && _entries[index].valid) {
        return &_entries[index];
    }
    return nullptr;
}

bool BitchatPeerCache::touchPeer(uint64_t peerId) {
    int slot = findSlot(peerId);
    if (slot >= 0) {
        _entries[slot].lastSeen = getCurrentTimestamp();
        _entries[slot].accessCount = ++_accessCounter;
        return true;
    }
    return false;
}

uint32_t BitchatPeerCache::getCurrentTimestamp() const {
#ifdef NATIVE_TEST
    static uint32_t counter = 1000;
    return counter++;
#elif defined(ESP32) || defined(NRF52_PLATFORM)
    return millis() / 1000;
#else
    return 0;
#endif
}

int BitchatPeerCache::findSlot(uint64_t peerId) const {
    for (int i = 0; i < BITCHAT_PEER_CACHE_SIZE; i++) {
        if (_entries[i].valid && _entries[i].peerId == peerId) {
            return i;
        }
    }
    return -1;
}

int BitchatPeerCache::findEmptySlot() const {
    for (int i = 0; i < BITCHAT_PEER_CACHE_SIZE; i++) {
        if (!_entries[i].valid) {
            return i;
        }
    }
    return -1;
}

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
