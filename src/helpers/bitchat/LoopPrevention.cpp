/**
 * Story 6.4: Loop Prevention - Implementation
 */

#ifdef ENABLE_BITCHAT

#include "LoopPrevention.h"
#include <cstring>

namespace mesh {
namespace bitchat {

// FNV-1a hash constants
#define FNV_32_OFFSET_BASIS 2166136261U
#define FNV_32_PRIME 16777619U

LoopPrevention::LoopPrevention()
    : _count(0)
    , _nextSlot(0)
{
    clear();
}

void LoopPrevention::markSeen(uint64_t messageId, uint64_t senderId, uint32_t timestamp)
{
    if (messageId == 0) {
        return;
    }
    
    // Check if already exists
    int existingSlot = findEntry(messageId, senderId);
    if (existingSlot >= 0) {
        // Update timestamp
        _cache[existingSlot].timestamp = timestamp;
        return;
    }
    
    // Find slot to use
    int slot = findSlotToUse();
    
    // Add entry
    SeenMessage& entry = _cache[slot];
    entry.messageId = messageId;
    entry.senderId = senderId;
    entry.timestamp = timestamp;
    entry.active = true;
    
    if (_count < LOOP_PREVENTION_CACHE_SIZE) {
        _count++;
    }
}

bool LoopPrevention::isDuplicate(uint64_t messageId, uint64_t senderId) const
{
    return findEntry(messageId, senderId) >= 0;
}

void LoopPrevention::clearOldEntries(uint32_t currentTime, uint32_t timeoutSeconds)
{
    uint32_t cutoff = currentTime - timeoutSeconds;
    
    for (int i = 0; i < LOOP_PREVENTION_CACHE_SIZE; i++) {
        if (_cache[i].active && _cache[i].timestamp < cutoff) {
            _cache[i].active = false;
            _cache[i].messageId = 0;
            _cache[i].senderId = 0;
            if (_count > 0) {
                _count--;
            }
        }
    }
}

void LoopPrevention::clear()
{
    for (int i = 0; i < LOOP_PREVENTION_CACHE_SIZE; i++) {
        _cache[i] = SeenMessage();
    }
    _count = 0;
    _nextSlot = 0;
}

size_t LoopPrevention::getCacheSize() const
{
    return _count;
}

uint32_t LoopPrevention::computeHash(const uint8_t* data, size_t len)
{
    if (data == nullptr || len == 0) {
        return 0;
    }
    
    uint32_t hash = FNV_32_OFFSET_BASIS;
    
    for (size_t i = 0; i < len; i++) {
        hash ^= data[i];
        hash *= FNV_32_PRIME;
    }
    
    return hash;
}

int LoopPrevention::findEntry(uint64_t messageId, uint64_t senderId) const
{
    for (int i = 0; i < LOOP_PREVENTION_CACHE_SIZE; i++) {
        if (_cache[i].active &&
            _cache[i].messageId == messageId &&
            _cache[i].senderId == senderId) {
            return i;
        }
    }
    return -1;
}

int LoopPrevention::findSlotToUse()
{
    // Simple round-robin eviction
    int slot = _nextSlot;
    _nextSlot = (_nextSlot + 1) % LOOP_PREVENTION_CACHE_SIZE;
    return slot;
}

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
