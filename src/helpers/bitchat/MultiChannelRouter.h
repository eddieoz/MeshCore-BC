#pragma once

/**
 * Story 5.3: Multi-Channel Encapsulation Routing
 * 
 * Routes encapsulated BitChat messages to multiple MeshCore channels
 * based on channel registry lookups. Supports:
 * - Routing to any registered channel
 * - Default channel fallback for unknown channels
 * - Channel name normalization (#general -> general)
 * - Fragmentation for large messages
 */

#ifdef ENABLE_BITCHAT

#include "ChannelRegistry.h"
#include "BitchatMessageEncapsulator.h"
#include <cstdint>
#include <cstring>

namespace mesh {
namespace bitchat {

// Maximum fragments for a single message
#define BITCHAT_MAX_FRAGMENTS 4

// Maximum channel name length (including #)
#define BITCHAT_MAX_ROUTER_CHANNEL_NAME 33

/**
 * Routing status codes
 */
enum class RoutingStatus : uint8_t {
    ROUTED = 0,              // Successfully routed to requested channel
    ROUTED_TO_DEFAULT = 1,   // Routed to default channel (unknown requested)
    CHANNEL_NOT_FOUND = 2,   // Channel not found and no default set
    ENCAPSULATION_FAILED = 3,// Encapsulation error
    INVALID_MESSAGE = 4,     // Invalid input message
    REGISTRY_ERROR = 5       // Channel registry error
};

/**
 * Routing result structure
 */
struct RoutingResult {
    RoutingStatus status;                    // Routing status
    EncapsulatedPacket packets[BITCHAT_MAX_FRAGMENTS];  // Output packets
    size_t packetCount;                      // Number of packets created
    char routedChannel[BITCHAT_MAX_CHANNEL_NAME_LEN]; // Actual channel used
    uint32_t timestamp;                      // Routing timestamp
    
    RoutingResult()
        : status(RoutingStatus::CHANNEL_NOT_FOUND)
        , packetCount(0)
        , timestamp(0)
    {
        memset(routedChannel, 0, sizeof(routedChannel));
    }
};

/**
 * Multi-Channel Router
 * 
 * Routes BitChat messages to appropriate MeshCore channels
 * based on channel name lookups in the registry.
 */
class MultiChannelRouter {
public:
    /**
     * Constructor
     * @param registry Channel registry for lookups
     */
    explicit MultiChannelRouter(ChannelRegistry& registry);
    
    /**
     * Route a BitChat message to the appropriate channel
     * 
     * @param msg BitChat message to route
     * @param channelName Target channel name (e.g., "general" or "#general")
     * @param result Output routing result with packets
     * @return true if routed successfully (may use default channel)
     * @return false if routing failed (channel not found, no default)
     */
    bool routeMessage(const BitchatMessage& msg,
                      const char* channelName,
                      RoutingResult& result);
    
    /**
     * Set the default channel for unknown channel names
     * 
     * @param channelName Default channel name (nullptr to disable)
     * @return true if default was set (channel exists in registry)
     * @return false if channel not found in registry
     */
    bool setDefaultChannel(const char* channelName);
    
    /**
     * Clear the default channel setting
     */
    void clearDefaultChannel();
    
    /**
     * Get the current default channel name
     * @return Default channel name or nullptr if not set
     */
    const char* getDefaultChannel() const;
    
    /**
     * Check if a channel name is registered
     * 
     * @param channelName Channel name to check
     * @return true if channel exists in registry
     */
    bool isChannelRegistered(const char* channelName) const;
    
    /**
     * Get the last routing error status
     */
    RoutingStatus getLastStatus() const { return _lastStatus; }

private:
    ChannelRegistry& _registry;
    char _defaultChannel[BITCHAT_MAX_ROUTER_CHANNEL_NAME];
    bool _hasDefault;
    RoutingStatus _lastStatus;
    BitchatMessageEncapsulator _encapsulator;
    
    /**
     * Normalize channel name (remove # prefix)
     * 
     * @param input Input channel name
     * @param output Output buffer
     * @param outputSize Output buffer size
     * @return true if normalized successfully
     */
    bool normalizeChannelName(const char* input, char* output, size_t outputSize) const;
    
    /**
     * Find channel key for routing (handles default fallback)
     * 
     * @param channelName Requested channel name
     * @param actualChannel Output actual channel used
     * @param actualChannelSize Size of actualChannel buffer
     * @return Pointer to channel key or nullptr if not found
     */
    const uint8_t* findChannelKey(const char* channelName,
                                   char* actualChannel,
                                   size_t actualChannelSize);
    
    /**
     * Check if channel name is null or empty
     */
    bool isNullOrEmpty(const char* str) const;
};

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
