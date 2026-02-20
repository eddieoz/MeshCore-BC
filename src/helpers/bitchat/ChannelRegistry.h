#pragma once

/**
 * Story 4.1: Channel Registry System
 * 
 * Bidirectional channel mapping between BitChat channel names
 * and MeshCore group channels
 * 
 * - Max 8 channel mappings
 * - Hashtag rooms (SHA256-derived keys)
 * - Regular group channels
 */

#ifdef ENABLE_BITCHAT

#include <cstdint>
#include <cstring>

namespace mesh {
namespace bitchat {

#define BITCHAT_MAX_CHANNEL_MAPPINGS 8
#define BITCHAT_MAX_CHANNEL_NAME_LEN 32
#define BITCHAT_CHANNEL_KEY_SIZE 32  // SHA256 hash size

// Channel types
enum class ChannelType : uint8_t {
    HASHTAG = 0,    // #channel - derived from hashtag
    DIRECT = 1,     // Direct message channel
    PRIVATE = 2     // Private group
};

/**
 * Channel mapping entry
 */
struct ChannelMapping {
    char name[BITCHAT_MAX_CHANNEL_NAME_LEN];      // BitChat channel name (e.g., "mesh", "general")
    uint8_t channelKey[BITCHAT_CHANNEL_KEY_SIZE]; // MeshCore channel key
    ChannelType type;
    bool active;                                    // Mapping is active
    uint32_t createdAt;
    
    ChannelMapping() 
        : type(ChannelType::HASHTAG)
        , active(false)
        , createdAt(0) 
    {
        memset(name, 0, sizeof(name));
        memset(channelKey, 0, sizeof(channelKey));
    }
};

/**
 * Channel Registry
 * 
 * Maps BitChat channel names to MeshCore group channels
 */
class ChannelRegistry {
public:
    ChannelRegistry();
    
    // Register a channel mapping
    // Returns true if registered, false if registry full
    bool registerChannel(const char* name, const uint8_t* channelKey, ChannelType type = ChannelType::HASHTAG);
    
    // Unregister a channel
    bool unregisterChannel(const char* name);
    
    // Look up channel key by name
    // Returns pointer to key or nullptr if not found
    const uint8_t* lookupByName(const char* name) const;
    
    // Look up channel name by key
    // Returns pointer to name or nullptr if not found
    const char* lookupByKey(const uint8_t* channelKey) const;
    
    // Find channel mapping by name
    const ChannelMapping* findChannel(const char* name) const;
    
    // Get number of registered channels
    size_t getChannelCount() const;
    
    // Check if registry is full
    bool isFull() const;
    
    // Check if channel name exists
    bool hasChannel(const char* name) const;
    
    // Clear all mappings
    void clear();
    
    // Get channel by index (for iteration)
    const ChannelMapping* getChannel(size_t index) const;
    
    // Derive channel key from hashtag (SHA256 of #channelname)
    static bool deriveKeyFromHashtag(const char* hashtag, uint8_t* outKey);
    
    // Auto-register default #mesh channel
    bool registerDefaultMeshChannel();

private:
    ChannelMapping _channels[BITCHAT_MAX_CHANNEL_MAPPINGS];
    
    // Find slot by name
    int findSlotByName(const char* name) const;
    
    // Find empty slot
    int findEmptySlot() const;
    
    // Find slot by key
    int findSlotByKey(const uint8_t* channelKey) const;
};

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
