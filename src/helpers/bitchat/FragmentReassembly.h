#pragma once

/**
 * Story 7.3: Fragment Reassembly
 * 
 * Reassembles fragmented BitChat messages
 * - 4-slot fragment buffer pool
 * - Reassembly by fragment ID
 * - Timeout handling
 */

#ifdef ENABLE_BITCHAT

#include <cstdint>
#include <cstring>

namespace mesh {
namespace bitchat {

// Maximum fragments per message
#define FRAGMENT_MAX_FRAGMENTS 4

// Maximum fragment data size
#define FRAGMENT_MAX_DATA_SIZE 256

// Maximum concurrent message reassemblies
#define FRAGMENT_MAX_MESSAGES 4

/**
 * Fragment info structure
 */
struct FragmentInfo {
    uint32_t messageId;      // Message identifier
    uint8_t fragmentNum;     // Fragment number (0-based)
    uint8_t totalFragments;  // Total fragments expected
    const uint8_t* data;     // Fragment data
    size_t length;           // Fragment length
    uint32_t timestamp;      // Arrival timestamp
    
    FragmentInfo()
        : messageId(0)
        , fragmentNum(0)
        , totalFragments(0)
        , data(nullptr)
        , length(0)
        , timestamp(0)
    {
    }
};

/**
 * Stored fragment entry
 */
struct FragmentEntry {
    uint32_t messageId;
    uint8_t fragmentNum;
    uint8_t totalFragments;
    uint8_t data[FRAGMENT_MAX_DATA_SIZE];
    size_t length;
    uint32_t timestamp;
    bool active;
    
    FragmentEntry()
        : messageId(0)
        , fragmentNum(0)
        , totalFragments(0)
        , length(0)
        , timestamp(0)
        , active(false)
    {
        memset(data, 0, sizeof(data));
    }
};

/**
 * Fragment Reassembly
 * 
 * Reassembles fragmented messages
 */
class FragmentReassembly {
public:
    FragmentReassembly();
    
    /**
     * Add a fragment
     * 
     * @param info Fragment info
     * @return true if added successfully
     */
    bool addFragment(const FragmentInfo& info);
    
    /**
     * Check if message is complete (all fragments received)
     * 
     * @param messageId Message identifier
     * @return true if complete
     */
    bool isComplete(uint32_t messageId) const;
    
    /**
     * Check if fragment exists
     * 
     * @param messageId Message identifier
     * @param fragmentNum Fragment number
     * @return true if fragment exists
     */
    bool hasFragment(uint32_t messageId, uint8_t fragmentNum) const;
    
    /**
     * Reassemble complete message
     * 
     * @param messageId Message identifier
     * @param output Output buffer
     * @param outputLen Output buffer size
     * @param outLen Actual reassembled length
     * @return true if reassembled successfully
     */
    bool reassemble(uint32_t messageId, uint8_t* output, size_t outputLen, size_t* outLen) const;
    
    /**
     * Clear old fragments (timeout)
     * 
     * @param currentTime Current timestamp
     * @param timeoutSeconds Timeout in seconds
     */
    void clearOldFragments(uint32_t currentTime, uint32_t timeoutSeconds);
    
    /**
     * Clear all fragments for a message
     * 
     * @param messageId Message identifier
     */
    void clearMessage(uint32_t messageId);
    
    /**
     * Clear all fragments
     */
    void clearAll();
    
    /**
     * Get fragment count for message
     */
    uint8_t getFragmentCount(uint32_t messageId) const;

private:
    FragmentEntry _fragments[FRAGMENT_MAX_FRAGMENTS * FRAGMENT_MAX_MESSAGES];
    
    int findFragment(uint32_t messageId, uint8_t fragmentNum) const;
    int findEmptySlot() const;
    int findOldestSlotForMessage(uint32_t messageId) const;
};

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
