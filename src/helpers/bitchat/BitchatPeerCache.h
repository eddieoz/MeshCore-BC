#pragma once

/**
 * Story 3.2: Peer Identity Cache
 * 
 * LRU cache for BitChat peer information
 * - 32 entry limit
 * - Stores: peer_id, nickname, ed25519_pubkey, last_seen
 * - LRU eviction when full
 */

#ifdef ENABLE_BITCHAT

#include <cstdint>
#include <cstring>

namespace mesh {
namespace bitchat {

#define BITCHAT_PEER_CACHE_SIZE 32
#define BITCHAT_MAX_NICKNAME_LEN 32
#define BITCHAT_PUBKEY_SIZE 32

/**
 * Peer cache entry
 */
struct PeerCacheEntry {
    uint64_t peerId;                          // 8-byte BitChat peer ID
    char nickname[BITCHAT_MAX_NICKNAME_LEN];  // Null-terminated nickname
    uint8_t pubkey[BITCHAT_PUBKEY_SIZE];      // Ed25519 public key
    uint32_t lastSeen;                        // Timestamp (seconds)
    bool valid;                               // Entry is valid
    uint32_t accessCount;                     // For LRU tracking
    
    PeerCacheEntry() 
        : peerId(0)
        , lastSeen(0)
        , valid(false)
        , accessCount(0) 
    {
        memset(nickname, 0, sizeof(nickname));
        memset(pubkey, 0, sizeof(pubkey));
    }
};

/**
 * BitChat Peer Cache
 * 
 * Thread-safe (assumes single-threaded use in embedded context)
 */
class BitchatPeerCache {
public:
    BitchatPeerCache();
    
    // Add or update peer in cache
    // Returns true if added/updated, false if cache is full and couldn't evict
    bool addOrUpdatePeer(uint64_t peerId, const char* nickname, const uint8_t* pubkey);
    
    // Find peer by ID
    // Returns pointer to entry or nullptr if not found
    const PeerCacheEntry* findPeer(uint64_t peerId);
    
    // Find peer by nickname
    // Returns pointer to entry or nullptr if not found
    const PeerCacheEntry* findPeerByNickname(const char* nickname);
    
    // Remove peer from cache
    bool removePeer(uint64_t peerId);
    
    // Get number of valid entries
    size_t getEntryCount() const;
    
    // Check if cache is full
    bool isFull() const;
    
    // Clear all entries
    void clear();
    
    // Get oldest entry (for LRU eviction)
    // Returns index of entry with lowest access count, or -1 if empty
    int findLRUVictim() const;
    
    // Get entry by index (for iteration)
    const PeerCacheEntry* getEntry(size_t index) const;
    
    // Update last_seen timestamp (call when peer is seen)
    bool touchPeer(uint64_t peerId);
    
    // Get current timestamp (platform-specific)
    uint32_t getCurrentTimestamp() const;

private:
    PeerCacheEntry _entries[BITCHAT_PEER_CACHE_SIZE];
    uint32_t _accessCounter;  // Incremented on each access for LRU
    
    // Find empty slot or existing entry
    int findSlot(uint64_t peerId) const;
    int findEmptySlot() const;
};

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
