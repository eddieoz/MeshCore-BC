/**
 * Story 6.4: Loop Prevention - BDD Tests
 * 
 * Feature: Prevent Message Loopback
 * 
 * TDD Approach:
 * 1. Write failing tests (RED)
 * 2. Implement to pass (GREEN)
 * 3. Refactor (REFACTOR)
 */

#include <unity.h>
#include "helpers/bitchat/LoopPrevention.h"

#ifdef ENABLE_BITCHAT

using namespace mesh::bitchat;

namespace test {
namespace bitchat {

/**
 * Scenario: Detect already-seen message
 * 
 * Given message seen before
 * When checking if should relay
 * Then return true (is duplicate)
 */
void test_detect_duplicate_message(void) {
    // Given: Loop prevention cache
    LoopPrevention loopPrevention;
    
    // And: A message ID
    uint64_t msgId = 0x1234567890ABCDEFULL;
    uint64_t senderId = 0xAABBCCDDEEFF0011ULL;
    uint32_t timestamp = 1234567890;
    
    // When: Mark as seen
    loopPrevention.markSeen(msgId, senderId, timestamp);
    
    // Then: Should detect as seen
    TEST_ASSERT_TRUE(loopPrevention.isDuplicate(msgId, senderId));
}

/**
 * Scenario: New message not duplicate
 * 
 * Given message never seen
 * When checking if should relay
 * Then return false (not duplicate)
 */
void test_new_message_not_duplicate(void) {
    // Given: Loop prevention cache
    LoopPrevention loopPrevention;
    
    // And: A message ID not seen
    uint64_t msgId = 0x1234567890ABCDEFULL;
    uint64_t senderId = 0xAABBCCDDEEFF0011ULL;
    
    // When: Check without marking
    // Then: Should NOT be duplicate
    TEST_ASSERT_FALSE(loopPrevention.isDuplicate(msgId, senderId));
}

/**
 * Scenario: Different sender same message ID
 * 
 * Given same message ID from different senders
 * When checking duplicates
 * Then treat as different messages
 */
void test_different_senders_same_msgid(void) {
    // Given: Loop prevention cache
    LoopPrevention loopPrevention;
    
    uint64_t msgId = 0x1234567890ABCDEFULL;
    uint64_t sender1 = 0xAABBCCDDEEFF0011ULL;
    uint64_t sender2 = 0x1122334455667788ULL;
    uint32_t timestamp = 1234567890;
    
    // Mark from sender1
    loopPrevention.markSeen(msgId, sender1, timestamp);
    
    // Then: Same ID from sender2 is NOT duplicate
    TEST_ASSERT_FALSE(loopPrevention.isDuplicate(msgId, sender2));
    
    // But from sender1 it IS duplicate
    TEST_ASSERT_TRUE(loopPrevention.isDuplicate(msgId, sender1));
}

/**
 * Scenario: Cache eviction on full
 * 
 * Given full cache
 * When adding new entry
 * Then evict oldest entry
 */
void test_cache_eviction(void) {
    // Given: Small cache for testing
    LoopPrevention loopPrevention;
    
    // Fill cache (64 entries)
    for (int i = 0; i < 70; i++) {
        uint64_t msgId = 0x1000000000000000ULL + i;
        uint64_t senderId = 0xA000000000000000ULL;
        uint32_t timestamp = 1234567890 + i;
        loopPrevention.markSeen(msgId, senderId, timestamp);
    }
    
    // Oldest entries should be evicted
    // First few should be gone
    TEST_ASSERT_FALSE(loopPrevention.isDuplicate(0x1000000000000000ULL, 0xA000000000000000ULL));
    TEST_ASSERT_FALSE(loopPrevention.isDuplicate(0x1000000000000001ULL, 0xA000000000000000ULL));
    
    // Newer ones should still be there
    TEST_ASSERT_TRUE(loopPrevention.isDuplicate(0x1000000000000045ULL, 0xA000000000000000ULL));
}

/**
 * Scenario: Clear old entries
 * 
 * Given cache with old entries
 * When clearing entries older than threshold
 * Then remove only old entries
 */
void test_clear_old_entries(void) {
    // Given: Loop prevention cache
    LoopPrevention loopPrevention;
    
    uint32_t now = 1234567890;
    
    // Add old entries
    loopPrevention.markSeen(0x1111111111111111ULL, 0xAA00000000000000ULL, now - 600);  // 10 min old
    loopPrevention.markSeen(0x2222222222222222ULL, 0xBB00000000000000ULL, now - 300);  // 5 min old
    
    // Add recent entry
    loopPrevention.markSeen(0x3333333333333333ULL, 0xCC00000000000000ULL, now - 60);   // 1 min old
    
    // When: Clear entries older than 4 minutes
    loopPrevention.clearOldEntries(now, 240);  // 240 seconds = 4 minutes
    
    // Then: Old entries should be gone
    TEST_ASSERT_FALSE(loopPrevention.isDuplicate(0x1111111111111111ULL, 0xAA00000000000000ULL));
    TEST_ASSERT_FALSE(loopPrevention.isDuplicate(0x2222222222222222ULL, 0xBB00000000000000ULL));
    
    // And: Recent entry should remain
    TEST_ASSERT_TRUE(loopPrevention.isDuplicate(0x3333333333333333ULL, 0xCC00000000000000ULL));
}

/**
 * Scenario: Cache size limit
 * 
 * Given maximum cache size
 * When checking
 * Then return correct count
 */
void test_cache_size_limit(void) {
    // Given: Loop prevention cache
    LoopPrevention loopPrevention;
    
    // Then: Initial size should be 0
    TEST_ASSERT_EQUAL(0, loopPrevention.getCacheSize());
    
    // Add entries
    loopPrevention.markSeen(0x1111111111111111ULL, 0xAA00000000000000ULL, 1234567890);
    loopPrevention.markSeen(0x2222222222222222ULL, 0xBB00000000000000ULL, 1234567891);
    
    // Then: Size should be 2
    TEST_ASSERT_EQUAL(2, loopPrevention.getCacheSize());
}

/**
 * Scenario: Clear all entries
 * 
 * Given cache with entries
 * When clearing all
 * Then cache empty
 */
void test_clear_all_entries(void) {
    // Given: Loop prevention cache with entries
    LoopPrevention loopPrevention;
    loopPrevention.markSeen(0x1111111111111111ULL, 0xAA00000000000000ULL, 1234567890);
    loopPrevention.markSeen(0x2222222222222222ULL, 0xBB00000000000000ULL, 1234567891);
    
    // When: Clear all
    loopPrevention.clear();
    
    // Then: Cache should be empty
    TEST_ASSERT_EQUAL(0, loopPrevention.getCacheSize());
    TEST_ASSERT_FALSE(loopPrevention.isDuplicate(0x1111111111111111ULL, 0xAA00000000000000ULL));
    TEST_ASSERT_FALSE(loopPrevention.isDuplicate(0x2222222222222222ULL, 0xBB00000000000000ULL));
}

/**
 * Scenario: Generate message hash
 * 
 * Given message data
 * When generating hash
 * Then produce consistent FNV-1a hash
 */
void test_message_hash_consistency(void) {
    // Given: Message data
    uint8_t data1[] = "Hello World";
    uint8_t data2[] = "Hello World";
    uint8_t data3[] = "Different message";
    
    // When: Generate hashes
    uint32_t hash1 = LoopPrevention::computeHash(data1, sizeof(data1) - 1);
    uint32_t hash2 = LoopPrevention::computeHash(data2, sizeof(data2) - 1);
    uint32_t hash3 = LoopPrevention::computeHash(data3, sizeof(data3) - 1);
    
    // Then: Same data should produce same hash
    TEST_ASSERT_EQUAL(hash1, hash2);
    
    // And: Different data should (likely) produce different hash
    TEST_ASSERT_NOT_EQUAL(hash1, hash3);
}

} // namespace bitchat
} // namespace test

#endif // ENABLE_BITCHAT
