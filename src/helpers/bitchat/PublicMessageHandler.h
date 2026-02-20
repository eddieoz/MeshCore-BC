#pragma once

/**
 * Story 8.1: Public Message to BitChat
 * 
 * Routes MeshCore public broadcasts to BitChat
 * - Configurable target channel (default #mesh)
 * - Formats public messages for BitChat consumption
 */

#ifdef ENABLE_BITCHAT

#include "ChannelRegistry.h"
#include "MessageFormatter.h"
#include "../ble/BitchatBLEService.h"
#include <cstdint>
#include <cstring>

namespace mesh {
namespace bitchat {

/**
 * MeshCore public message structure
 */
struct MeshCorePublicMessage {
    const char* senderName;
    const char* content;
    uint32_t timestamp;
    
    MeshCorePublicMessage()
        : senderName(nullptr)
        , content(nullptr)
        , timestamp(0)
    {
    }
};

/**
 * Public Message Handler
 * 
 * Routes MeshCore public messages to BitChat
 */
class PublicMessageHandler {
public:
    /**
     * Constructor
     * @param registry Channel registry for lookups
     */
    explicit PublicMessageHandler(ChannelRegistry& registry);
    
    /**
     * Set target channel for public messages
     * @param channelName Target channel name (e.g., "mesh", "public")
     */
    void setTargetChannel(const char* channelName);
    
    /**
     * Get current target channel
     * @return Target channel name
     */
    const char* getTargetChannel() const;
    
    /**
     * Route public message to BitChat
     * 
     * @param pubMsg MeshCore public message
     * @param result Output BitChat message
     * @return true if routed successfully
     */
    bool routeToBitChat(const MeshCorePublicMessage& pubMsg, mesh::ble::BitchatMessage& result);

private:
    ChannelRegistry& _registry;
    char _targetChannel[BITCHAT_MAX_CHANNEL_NAME_LEN];
    MessageFormatter _formatter;
    
    /**
     * Validate public message
     */
    bool validateMessage(const MeshCorePublicMessage& pubMsg) const;
};

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
