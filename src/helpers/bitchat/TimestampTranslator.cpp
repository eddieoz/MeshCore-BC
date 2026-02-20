/**
 * Story 7.1: Timestamp Translation - Implementation
 */

#ifdef ENABLE_BITCHAT

#include "TimestampTranslator.h"
#include <cstdio>

namespace mesh {
namespace bitchat {

TimestampTranslator::TimestampTranslator(uint32_t currentTime)
    : _currentTime(currentTime)
    , _syncThreshold(TIMESTAMP_SYNC_THRESHOLD)
    , _lastSyncTime(0)
{
}

bool TimestampTranslator::syncFromBitChat(uint32_t bitchatTime)
{
    // Validate timestamp first
    if (!isValidTimestamp(bitchatTime)) {
        return false;
    }
    
    // Calculate difference
    uint32_t diff = calculateDifference(bitchatTime, _currentTime);
    
    // Sync if difference exceeds threshold
    if (diff > _syncThreshold) {
        _currentTime = bitchatTime;
        _lastSyncTime = bitchatTime;
        return true;
    }
    
    return false;
}

uint32_t TimestampTranslator::getTimestampForMeshCore(uint32_t bitchatTime) const
{
    // Use original BitChat timestamp for consistency
    // This ensures deterministic packet hashing for dedup
    if (isValidTimestamp(bitchatTime)) {
        return bitchatTime;
    }
    
    // Fall back to current time if invalid
    return _currentTime;
}

bool TimestampTranslator::isValidTimestamp(uint32_t timestamp) const
{
    // Reject zero
    if (timestamp == 0) {
        return false;
    }
    
    // Reject timestamps before 2020 (too old, likely wrong)
    if (timestamp < TIMESTAMP_MIN_VALID) {
        return false;
    }
    
    // Reject timestamps too far in future (>30 min from current)
    // Note: This check assumes _currentTime is reasonably accurate
    if (timestamp > _currentTime && (timestamp - _currentTime) > TIMESTAMP_MAX_FUTURE_OFFSET) {
        return false;
    }
    
    return true;
}

uint32_t TimestampTranslator::calculateDifference(uint32_t time1, uint32_t time2) const
{
    // Return absolute difference
    if (time1 > time2) {
        return time1 - time2;
    } else {
        return time2 - time1;
    }
}

bool TimestampTranslator::formatTimestamp(uint32_t timestamp, char* buffer, size_t bufferSize) const
{
    if (buffer == nullptr || bufferSize < 20) {
        return false;
    }
    
    // Simple ISO 8601 format: YYYY-MM-DD HH:MM:SS
    // This is a simplified version - full implementation would use gmtime
    
    // For testing purposes, just format as unix timestamp string
    int written = snprintf(buffer, bufferSize, "%u", timestamp);
    
    return (written > 0 && (size_t)written < bufferSize);
}

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
