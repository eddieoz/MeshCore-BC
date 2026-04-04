#pragma once

/**
 * Story 7.1: Timestamp Translation
 * 
 * Handles timestamp synchronization between BitChat and MeshCore
 * - Sync device time from BitChat (if differs >30s)
 * - Use BitChat timestamps in MeshCore packets
 * - Validate timestamps (reject invalid/future)
 */

#ifdef ENABLE_BITCHAT

#include <cstdint>
#include <cstring>

namespace mesh {
namespace bitchat {

// Default sync threshold: 30 seconds
#define TIMESTAMP_SYNC_THRESHOLD 30

// Maximum future offset: 30 minutes
#define TIMESTAMP_MAX_FUTURE_OFFSET (30 * 60)

// Minimum valid timestamp (2020-01-01)
#define TIMESTAMP_MIN_VALID 1577836800

/**
 * Timestamp Translator
 * 
 * Manages timestamp synchronization between BitChat and MeshCore
 */
class TimestampTranslator {
public:
    /**
     * Constructor
     * @param currentTime Current device timestamp
     */
    explicit TimestampTranslator(uint32_t currentTime);
    
    /**
     * Sync device time from BitChat timestamp
     * 
     * @param bitchatTime BitChat message timestamp
     * @return true if synced (difference > threshold)
     * @return false if not synced
     */
    bool syncFromBitChat(uint32_t bitchatTime);
    
    /**
     * Get timestamp for MeshCore packet
     * Uses original BitChat timestamp for dedup consistency
     * 
     * @param bitchatTime Original BitChat timestamp
     * @return Timestamp to use in MeshCore packet
     */
    uint32_t getTimestampForMeshCore(uint32_t bitchatTime) const;
    
    /**
     * Get current device time
     */
    uint32_t getCurrentTime() const { return _currentTime; }
    
    /**
     * Set current device time
     */
    void setCurrentTime(uint32_t time) { _currentTime = time; }
    
    /**
     * Set sync threshold
     * @param thresholdSeconds Seconds difference required for sync
     */
    void setSyncThreshold(uint32_t thresholdSeconds) { _syncThreshold = thresholdSeconds; }
    
    /**
     * Get sync threshold
     */
    uint32_t getSyncThreshold() const { return _syncThreshold; }
    
    /**
     * Validate timestamp
     * Checks for: not 0, not too old, not too far in future
     * 
     * @param timestamp Timestamp to validate
     * @return true if valid
     */
    bool isValidTimestamp(uint32_t timestamp) const;
    
    /**
     * Calculate absolute difference between two timestamps
     * 
     * @param time1 First timestamp
     * @param time2 Second timestamp
     * @return Absolute difference
     */
    uint32_t calculateDifference(uint32_t time1, uint32_t time2) const;
    
    /**
     * Format timestamp as human-readable string
     * 
     * @param timestamp Timestamp to format
     * @param buffer Output buffer
     * @param bufferSize Buffer size
     * @return true if formatted successfully
     */
    bool formatTimestamp(uint32_t timestamp, char* buffer, size_t bufferSize) const;

private:
    uint32_t _currentTime;
    uint32_t _syncThreshold;
    uint32_t _lastSyncTime;
};

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
