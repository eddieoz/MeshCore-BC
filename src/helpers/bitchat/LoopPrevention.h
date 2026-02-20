#pragma once

/**
 * Story 6.4: Loop Prevention
 * 
 * Prevents message loops between BitChat and MeshCore
 * - Tracks seen messages with FNV-1a hash
 * - 64-entry cache with LRU eviction
 * - 5-minute timeout for old entries
 */

#ifdef ENABLE_BITCHAT

#include <cstdint>
#include <cstring>

namespace mesh {
namespace bitchat {

// Cache size for seen messages
#define LOOP_PREVENTION_CACHE_SIZE 64

// Default timeout: 5 minutes (300 seconds)
#define LOOP_PREVENTION_DEFAULT_TIMEOUT 300

/**
 * Seen message entry
 */
struct SeenMessage {
    uint64_t messageId;
    uint64_t senderId;
    uint32_t timestamp;
    bool active;
    
    SeenMessage()
        : messageId(0)
        , senderId(0)
        , timestamp(0)
        , active(false)
    {
    }
};

/**
 * Loop Prevention
 * 
 * Prevents message loops by tracking seen messages
 */
class LoopPrevention {
public:
    LoopPrevention();
    
    /**
     * Mark a message as seen
     * 
     * @param messageId Message identifier
     * @param senderId Sender identifier
     * @param timestamp Current timestamp
     */
    void markSeen(uint64_t messageId, uint64_t senderId, uint32_t timestamp);
    
    /**
     * Check if message is a duplicate
     * 
     * @param messageId Message identifier
     * @param senderId Sender identifier
     * @return true if seen before
     */
    bool isDuplicate(uint64_t messageId, uint64_t senderId) const;
    
    /**
     * Clear entries older than timeout
     * 
     * @param currentTime Current timestamp
     * @param timeoutSeconds Timeout in seconds
     */
    void clearOldEntries(uint32_t currentTime, uint32_t timeoutSeconds);
    
    /**
     * Clear all entries
     */
    void clear();
    
    /**
     * Get current cache size
     */
    size_t getCacheSize() const;
    
    /**
     * Compute FNV-1a hash of data
     * 
     * @param data Data to hash
     * @param len Data length
     * @return 32-bit hash
     */
    static uint32_t computeHash(const uint8_t* data, size_t len);

private:
    SeenMessage _cache[LOOP_PREVENTION_CACHE_SIZE];
    size_t _count;
    uint8_t _nextSlot;
    
    /**
     * Find existing entry
     */
    int findEntry(uint64_t messageId, uint64_t senderId) const;
    
    /**
     * Find empty slot or slot to evict
     */
    int findSlotToUse();
};

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
