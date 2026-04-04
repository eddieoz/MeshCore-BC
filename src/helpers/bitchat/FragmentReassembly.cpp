/**
 * Story 7.3: Fragment Reassembly - Implementation
 */

#ifdef ENABLE_BITCHAT

#include "FragmentReassembly.h"

namespace mesh {
namespace bitchat {

FragmentReassembly::FragmentReassembly()
{
    clearAll();
}

bool FragmentReassembly::addFragment(const FragmentInfo& info)
{
    // Validate inputs
    if (info.data == nullptr || info.length == 0) {
        return false;
    }
    
    if (info.fragmentNum >= FRAGMENT_MAX_FRAGMENTS) {
        return false;
    }
    
    if (info.length > FRAGMENT_MAX_DATA_SIZE) {
        return false;
    }
    
    // Check if fragment already exists
    int existing = findFragment(info.messageId, info.fragmentNum);
    if (existing >= 0) {
        // Already have this fragment, ignore duplicate
        return true;
    }
    
    // Find slot
    int slot = findEmptySlot();
    if (slot < 0) {
        // No empty slot, try to find oldest slot for this message
        slot = findOldestSlotForMessage(info.messageId);
        if (slot < 0) {
            return false;  // No slot available
        }
    }
    
    // Store fragment
    FragmentEntry& entry = _fragments[slot];
    entry.messageId = info.messageId;
    entry.fragmentNum = info.fragmentNum;
    entry.totalFragments = info.totalFragments;
    entry.length = info.length;
    entry.timestamp = info.timestamp;
    entry.active = true;
    memcpy(entry.data, info.data, info.length);
    
    return true;
}

bool FragmentReassembly::isComplete(uint32_t messageId) const
{
    // Count fragments for this message
    uint8_t count = getFragmentCount(messageId);
    if (count == 0) {
        return false;
    }
    
    // Find any fragment to get total expected
    for (int i = 0; i < FRAGMENT_MAX_FRAGMENTS * FRAGMENT_MAX_MESSAGES; i++) {
        if (_fragments[i].active && _fragments[i].messageId == messageId) {
            return count >= _fragments[i].totalFragments;
        }
    }
    
    return false;
}

bool FragmentReassembly::hasFragment(uint32_t messageId, uint8_t fragmentNum) const
{
    return findFragment(messageId, fragmentNum) >= 0;
}

bool FragmentReassembly::reassemble(uint32_t messageId, uint8_t* output, size_t outputLen, size_t* outLen) const
{
    if (output == nullptr || outLen == nullptr) {
        return false;
    }
    
    if (!isComplete(messageId)) {
        return false;
    }
    
    // Calculate total size needed
    size_t totalSize = 0;
    uint8_t totalFrags = 0;
    
    for (int i = 0; i < FRAGMENT_MAX_FRAGMENTS * FRAGMENT_MAX_MESSAGES; i++) {
        if (_fragments[i].active && _fragments[i].messageId == messageId) {
            totalSize += _fragments[i].length;
            totalFrags = _fragments[i].totalFragments;
        }
    }
    
    if (outputLen < totalSize) {
        return false;
    }
    
    // Reassemble in order
    size_t offset = 0;
    for (uint8_t fragNum = 0; fragNum < totalFrags; fragNum++) {
        int slot = findFragment(messageId, fragNum);
        if (slot < 0) {
            return false;  // Missing fragment
        }
        
        const FragmentEntry& entry = _fragments[slot];
        memcpy(&output[offset], entry.data, entry.length);
        offset += entry.length;
    }
    
    *outLen = offset;
    return true;
}

void FragmentReassembly::clearOldFragments(uint32_t currentTime, uint32_t timeoutSeconds)
{
    uint32_t cutoff = currentTime - timeoutSeconds;
    
    for (int i = 0; i < FRAGMENT_MAX_FRAGMENTS * FRAGMENT_MAX_MESSAGES; i++) {
        if (_fragments[i].active && _fragments[i].timestamp < cutoff) {
            _fragments[i].active = false;
        }
    }
}

void FragmentReassembly::clearMessage(uint32_t messageId)
{
    for (int i = 0; i < FRAGMENT_MAX_FRAGMENTS * FRAGMENT_MAX_MESSAGES; i++) {
        if (_fragments[i].active && _fragments[i].messageId == messageId) {
            _fragments[i].active = false;
        }
    }
}

void FragmentReassembly::clearAll()
{
    for (int i = 0; i < FRAGMENT_MAX_FRAGMENTS * FRAGMENT_MAX_MESSAGES; i++) {
        _fragments[i] = FragmentEntry();
    }
}

uint8_t FragmentReassembly::getFragmentCount(uint32_t messageId) const
{
    uint8_t count = 0;
    for (int i = 0; i < FRAGMENT_MAX_FRAGMENTS * FRAGMENT_MAX_MESSAGES; i++) {
        if (_fragments[i].active && _fragments[i].messageId == messageId) {
            count++;
        }
    }
    return count;
}

int FragmentReassembly::findFragment(uint32_t messageId, uint8_t fragmentNum) const
{
    for (int i = 0; i < FRAGMENT_MAX_FRAGMENTS * FRAGMENT_MAX_MESSAGES; i++) {
        if (_fragments[i].active &&
            _fragments[i].messageId == messageId &&
            _fragments[i].fragmentNum == fragmentNum) {
            return i;
        }
    }
    return -1;
}

int FragmentReassembly::findEmptySlot() const
{
    for (int i = 0; i < FRAGMENT_MAX_FRAGMENTS * FRAGMENT_MAX_MESSAGES; i++) {
        if (!_fragments[i].active) {
            return i;
        }
    }
    return -1;
}

int FragmentReassembly::findOldestSlotForMessage(uint32_t messageId) const
{
    int oldestSlot = -1;
    uint32_t oldestTime = 0xFFFFFFFF;
    
    for (int i = 0; i < FRAGMENT_MAX_FRAGMENTS * FRAGMENT_MAX_MESSAGES; i++) {
        if (_fragments[i].active &&
            _fragments[i].messageId == messageId &&
            _fragments[i].timestamp < oldestTime) {
            oldestTime = _fragments[i].timestamp;
            oldestSlot = i;
        }
    }
    
    return oldestSlot;
}

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
