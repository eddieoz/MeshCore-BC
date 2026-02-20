/**
 * Story 7.1: Timestamp Translation - BDD Tests
 * 
 * Feature: Timestamp Synchronization
 * 
 * TDD Approach:
 * 1. Write failing tests (RED)
 * 2. Implement to pass (GREEN)
 * 3. Refactor (REFACTOR)
 */

#include <unity.h>
#include "helpers/bitchat/TimestampTranslator.h"

#ifdef ENABLE_BITCHAT

using namespace mesh::bitchat;

namespace test {
namespace bitchat {

/**
 * Scenario: Sync time from BitChat
 * 
 * Given BitChat message has timestamp T
 * And device time is unsynced or differs >30s
 * When message received
 * Then update device time from BitChat
 */
void test_sync_time_from_bitchat(void) {
    // Given: Timestamp translator with current time (2024-01-01)
    uint32_t deviceTime = 1704067200;  // 2024-01-01 00:00:00 UTC
    TimestampTranslator translator(deviceTime);
    
    // And: BitChat message with different timestamp (>30s difference)
    uint32_t bitchatTime = deviceTime + 100;  // 100 seconds ahead
    
    // When: Process timestamp
    bool synced = translator.syncFromBitChat(bitchatTime);
    
    // Then: Should sync (difference > 30s)
    TEST_ASSERT_TRUE(synced);
    
    // And: Device time should update
    TEST_ASSERT_EQUAL(bitchatTime, translator.getCurrentTime());
}

/**
 * Scenario: Small time difference - no sync
 * 
 * Given BitChat timestamp within 30s of device time
 * When checking if should sync
 * Then do not sync
 */
void test_small_time_difference_no_sync(void) {
    // Given: Timestamp translator (2024-01-01)
    uint32_t deviceTime = 1704067200;
    TimestampTranslator translator(deviceTime);
    
    // And: BitChat timestamp within 30s
    uint32_t bitchatTime = deviceTime + 20;  // Only 20 seconds ahead
    
    // When: Process timestamp
    bool synced = translator.syncFromBitChat(bitchatTime);
    
    // Then: Should NOT sync (difference < 30s)
    TEST_ASSERT_FALSE(synced);
    
    // And: Device time unchanged
    TEST_ASSERT_EQUAL(deviceTime, translator.getCurrentTime());
}

/**
 * Scenario: BitChat timestamp for MeshCore
 * 
 * Given routing BitChat message to MeshCore
 * When creating MeshCore packet
 * Then use original BitChat timestamp
 */
void test_use_bitchat_timestamp_for_meshcore(void) {
    // Given: Timestamp translator (2024-01-01)
    uint32_t deviceTime = 1704067200;
    TimestampTranslator translator(deviceTime);
    
    // And: BitChat timestamp (within valid range)
    uint32_t bitchatTime = deviceTime + 60;  // 1 minute ahead
    
    // When: Get timestamp for MeshCore packet
    uint32_t meshTime = translator.getTimestampForMeshCore(bitchatTime);
    
    // Then: Should use original BitChat timestamp
    TEST_ASSERT_EQUAL(bitchatTime, meshTime);
}

/**
 * Scenario: Invalid timestamp handling
 * 
 * Given invalid timestamp (0 or future)
 * When validating
 * Then reject as invalid
 */
void test_invalid_timestamp_handling(void) {
    // Given: Timestamp translator (2024-01-01)
    uint32_t deviceTime = 1704067200;
    TimestampTranslator translator(deviceTime);
    
    // When: Check invalid timestamps
    TEST_ASSERT_FALSE(translator.isValidTimestamp(0));  // Zero is invalid
    TEST_ASSERT_FALSE(translator.isValidTimestamp(1));  // Too old (1970)
    
    // Valid timestamp (close to current time)
    TEST_ASSERT_TRUE(translator.isValidTimestamp(deviceTime));
}

/**
 * Scenario: Time difference calculation
 * 
 * Given two timestamps
 * When calculating difference
 * Then return absolute difference
 */
void test_time_difference_calculation(void) {
    // Given: Timestamp translator (2024-01-01)
    uint32_t deviceTime = 1704067200;
    TimestampTranslator translator(deviceTime);
    
    // When: Calculate differences
    uint32_t diff1 = translator.calculateDifference(deviceTime + 100, deviceTime);
    uint32_t diff2 = translator.calculateDifference(deviceTime, deviceTime + 100);
    
    // Then: Should be same (absolute)
    TEST_ASSERT_EQUAL(100, diff1);
    TEST_ASSERT_EQUAL(100, diff2);
}

/**
 * Scenario: Sync threshold configuration
 * 
 * Given custom sync threshold
 * When time difference exceeds threshold
 * Then sync
 */
void test_custom_sync_threshold(void) {
    // Given: Timestamp translator with custom threshold (60s)
    uint32_t deviceTime = 1704067200;
    TimestampTranslator translator(deviceTime);
    translator.setSyncThreshold(60);
    
    // When: Difference is 45s (less than 60s)
    bool synced1 = translator.syncFromBitChat(deviceTime + 45);
    TEST_ASSERT_FALSE(synced1);
    
    // When: Difference is 75s (more than 60s)
    bool synced2 = translator.syncFromBitChat(deviceTime + 75);
    TEST_ASSERT_TRUE(synced2);
}

/**
 * Scenario: Future timestamp rejection
 * 
 * Given timestamp too far in future
 * When validating
 * Then reject
 */
void test_future_timestamp_rejection(void) {
    // Given: Timestamp translator at current time
    uint32_t deviceTime = 1704067200;
    TimestampTranslator translator(deviceTime);
    
    // When: Check timestamp 1 hour in future
    uint32_t futureTime = deviceTime + 3600;
    
    // Then: Should be rejected (>30 min future)
    TEST_ASSERT_FALSE(translator.isValidTimestamp(futureTime));
}

/**
 * Scenario: Timestamp formatting
 * 
 * Given timestamp
 * When formatting for display
 * Then produce human-readable string
 */
void test_timestamp_formatting(void) {
    // Given: Timestamp translator
    uint32_t deviceTime = 1704067200;
    TimestampTranslator translator(deviceTime);
    
    // Test timestamp: 2024-01-01 00:00:00 UTC
    uint32_t timestamp = 1704067200;
    
    char buffer[32];
    bool formatted = translator.formatTimestamp(timestamp, buffer, sizeof(buffer));
    
    // Should format successfully
    TEST_ASSERT_TRUE(formatted);
    TEST_ASSERT_TRUE(strlen(buffer) > 0);
}

} // namespace bitchat
} // namespace test

#endif // ENABLE_BITCHAT
