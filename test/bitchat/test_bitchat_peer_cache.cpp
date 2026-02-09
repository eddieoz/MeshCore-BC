/**
 * Story 3.2: Peer Identity Cache - BDD Tests
 * 
 * Feature: BitChat Peer Information Cache
 */

#include <unity.h>
#include "helpers/bitchat/BitchatPeerCache.h"

#ifdef ENABLE_BITCHAT

using namespace mesh::bitchat;

namespace test {
namespace bitchat {

static const uint8_t CACHE_TEST_PUBKEY[32] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20
};

static const uint64_t CACHE_TEST_PEER_ID = 0x0807060504030201ULL;

/**
 * Scenario: Add peer to cache
 * 
 * Given empty cache
 * When adding a peer
 * Then peer is stored successfully
 */
void test_add_peer_to_cache(void) {
    BitchatPeerCache cache;
    
    TEST_ASSERT_EQUAL(0, cache.getEntryCount());
    
    // When: Add peer
    bool added = cache.addOrUpdatePeer(CACHE_TEST_PEER_ID, "Alice", CACHE_TEST_PUBKEY);
    
    // Then: Added successfully
    TEST_ASSERT_TRUE(added);
    TEST_ASSERT_EQUAL(1, cache.getEntryCount());
}

/**
 * Scenario: Find peer by ID
 * 
 * Given cache with peer
 * When searching by peer ID
 * Then peer info is returned
 */
void test_find_peer_by_id(void) {
    BitchatPeerCache cache;
    cache.addOrUpdatePeer(CACHE_TEST_PEER_ID, "Alice", CACHE_TEST_PUBKEY);
    
    // When: Find peer
    const PeerCacheEntry* entry = cache.findPeer(CACHE_TEST_PEER_ID);
    
    // Then: Found
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_HEX64(CACHE_TEST_PEER_ID, entry->peerId);
    TEST_ASSERT_EQUAL_STRING("Alice", entry->nickname);
    TEST_ASSERT_TRUE(entry->valid);
}

/**
 * Scenario: Find peer by nickname
 * 
 * Given cache with peer
 * When searching by nickname
 * Then peer info is returned
 */
void test_find_peer_by_nickname(void) {
    BitchatPeerCache cache;
    cache.addOrUpdatePeer(CACHE_TEST_PEER_ID, "Alice", CACHE_TEST_PUBKEY);
    
    // When: Find by nickname
    const PeerCacheEntry* entry = cache.findPeerByNickname("Alice");
    
    // Then: Found
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_HEX64(CACHE_TEST_PEER_ID, entry->peerId);
}

/**
 * Scenario: Update existing peer
 * 
 * Given peer exists in cache
 * When adding same peer ID with new info
 * Then info is updated
 */
void test_update_existing_peer(void) {
    BitchatPeerCache cache;
    cache.addOrUpdatePeer(CACHE_TEST_PEER_ID, "Alice", CACHE_TEST_PUBKEY);
    
    uint8_t newPubkey[32];
    memset(newPubkey, 0xAA, 32);
    
    // When: Update same peer ID
    bool updated = cache.addOrUpdatePeer(CACHE_TEST_PEER_ID, "Alice2", newPubkey);
    
    // Then: Updated
    TEST_ASSERT_TRUE(updated);
    TEST_ASSERT_EQUAL(1, cache.getEntryCount());  // Still 1 entry
    
    const PeerCacheEntry* entry = cache.findPeer(CACHE_TEST_PEER_ID);
    TEST_ASSERT_EQUAL_STRING("Alice2", entry->nickname);
    TEST_ASSERT_EQUAL_HEX8(0xAA, entry->pubkey[0]);
}

/**
 * Scenario: Cache enforces size limit with LRU eviction
 * 
 * Given full cache (32 entries)
 * When adding 33rd peer
 * Then least recently used entry is evicted
 */
void test_lru_eviction(void) {
    BitchatPeerCache cache;
    
    // Fill cache
    for (int i = 0; i < BITCHAT_PEER_CACHE_SIZE; i++) {
        uint64_t peerId = 0x100 + i;
        char nickname[16];
        snprintf(nickname, sizeof(nickname), "Peer%d", i);
        
        uint8_t pubkey[32];
        memset(pubkey, i, 32);
        
        TEST_ASSERT_TRUE(cache.addOrUpdatePeer(peerId, nickname, pubkey));
    }
    
    TEST_ASSERT_EQUAL(BITCHAT_PEER_CACHE_SIZE, cache.getEntryCount());
    TEST_ASSERT_TRUE(cache.isFull());
    
    // Access first peer to make it recently used
    const PeerCacheEntry* firstPeer = cache.findPeer(0x100);
    TEST_ASSERT_NOT_NULL(firstPeer);
    
    // Add 33rd peer - should evict least recently used (peer 1)
    uint8_t newPubkey[32];
    memset(newPubkey, 0xFF, 32);
    bool added = cache.addOrUpdatePeer(0x999, "NewPeer", newPubkey);
    
    // Then: Still 32 entries, but different peers
    TEST_ASSERT_TRUE(added);
    TEST_ASSERT_EQUAL(BITCHAT_PEER_CACHE_SIZE, cache.getEntryCount());
    
    // First peer still there (was accessed)
    TEST_ASSERT_NOT_NULL(cache.findPeer(0x100));
    
    // New peer is there
    TEST_ASSERT_NOT_NULL(cache.findPeer(0x999));
}

/**
 * Scenario: Remove peer from cache
 * 
 * Given cache with peer
 * When removing peer
 * Then peer is deleted
 */
void test_remove_peer(void) {
    BitchatPeerCache cache;
    cache.addOrUpdatePeer(CACHE_TEST_PEER_ID, "Alice", CACHE_TEST_PUBKEY);
    TEST_ASSERT_EQUAL(1, cache.getEntryCount());
    
    // When: Remove peer
    bool removed = cache.removePeer(CACHE_TEST_PEER_ID);
    
    // Then: Removed
    TEST_ASSERT_TRUE(removed);
    TEST_ASSERT_EQUAL(0, cache.getEntryCount());
    TEST_ASSERT_NULL(cache.findPeer(CACHE_TEST_PEER_ID));
}

/**
 * Scenario: Touch peer updates timestamp
 * 
 * Given cache with peer
 * When touching peer
 * Then last_seen is updated
 */
void test_touch_peer(void) {
    BitchatPeerCache cache;
    cache.addOrUpdatePeer(CACHE_TEST_PEER_ID, "Alice", CACHE_TEST_PUBKEY);
    
    const PeerCacheEntry* entry1 = cache.findPeer(CACHE_TEST_PEER_ID);
    uint32_t oldTimestamp = entry1->lastSeen;
    
    // When: Touch peer
    bool touched = cache.touchPeer(CACHE_TEST_PEER_ID);
    
    // Then: Timestamp updated
    TEST_ASSERT_TRUE(touched);
    const PeerCacheEntry* entry2 = cache.findPeer(CACHE_TEST_PEER_ID);
    TEST_ASSERT_TRUE(entry2->lastSeen > oldTimestamp);
}

/**
 * Scenario: Clear cache removes all entries
 * 
 * Given cache with multiple peers
 * When clearing cache
 * Then all entries removed
 */
void test_clear_cache(void) {
    BitchatPeerCache cache;
    
    // Add some peers
    for (int i = 0; i < 5; i++) {
        uint8_t pubkey[32];
        memset(pubkey, i, 32);
        cache.addOrUpdatePeer(0x100 + i, "Peer", pubkey);
    }
    
    TEST_ASSERT_EQUAL(5, cache.getEntryCount());
    
    // When: Clear cache
    cache.clear();
    
    // Then: Empty
    TEST_ASSERT_EQUAL(0, cache.getEntryCount());
    TEST_ASSERT_FALSE(cache.isFull());
    TEST_ASSERT_NULL(cache.findPeer(0x100));
}

/**
 * Scenario: Null parameters handled gracefully
 * 
 * Given cache
 * When adding with null nickname or pubkey
 * Then operation fails
 */
void test_null_parameters(void) {
    BitchatPeerCache cache;
    
    // Null nickname
    TEST_ASSERT_FALSE(cache.addOrUpdatePeer(CACHE_TEST_PEER_ID, nullptr, CACHE_TEST_PUBKEY));
    
    // Null pubkey
    TEST_ASSERT_FALSE(cache.addOrUpdatePeer(CACHE_TEST_PEER_ID, "Alice", nullptr));
    
    TEST_ASSERT_EQUAL(0, cache.getEntryCount());
}

/**
 * Scenario: Find non-existent peer returns null
 * 
 * Given empty cache
 * When searching for peer
 * Then returns null
 */
void test_find_nonexistent_peer(void) {
    BitchatPeerCache cache;
    
    TEST_ASSERT_NULL(cache.findPeer(0x999));
    TEST_ASSERT_NULL(cache.findPeerByNickname("Nobody"));
}

/**
 * Scenario: Get entry by index
 * 
 * Given cache with peers
 * When getting entry by index
 * Then returns entry or null
 */
void test_get_entry_by_index(void) {
    BitchatPeerCache cache;
    cache.addOrUpdatePeer(CACHE_TEST_PEER_ID, "Alice", CACHE_TEST_PUBKEY);
    
    // Valid index should return entry
    const PeerCacheEntry* entry = cache.getEntry(0);
    TEST_ASSERT_NOT_NULL(entry);
    
    // Invalid index should return null
    TEST_ASSERT_NULL(cache.getEntry(100));
}

} // namespace bitchat
} // namespace test

#else // !ENABLE_BITCHAT

namespace test {
namespace bitchat {
void test_cache_placeholder(void) {
    TEST_IGNORE_MESSAGE("BitChat disabled - ENABLE_BITCHAT not defined");
}
} // namespace bitchat
} // namespace test

#endif // ENABLE_BITCHAT
