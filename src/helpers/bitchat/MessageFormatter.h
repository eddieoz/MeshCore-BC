#pragma once

/**
 * Story 6.3: BitChat Message Formatting
 * 
 * Formats MeshCore messages for BitChat consumption
 * - Formats: "<sender_name> message_content"
 * - Proper TLV encoding
 * - Ed25519 signature per BitChat spec
 */

#ifdef ENABLE_BITCHAT

#include "../ble/BitchatBLEService.h"
#include <cstdint>
#include <cstring>

namespace mesh {
namespace bitchat {

/**
 * MeshCore message types
 */
enum class MeshCoreMessageType : uint8_t {
    GROUP_MESSAGE = 0,
    DIRECT_MESSAGE = 1,
    PUBLIC_MESSAGE = 2,
    ANNOUNCEMENT = 3
};

/**
 * MeshCore message info structure
 */
struct MeshCoreMessageInfo {
    MeshCoreMessageType type;
    char senderName[32];
    char recipientName[32];  // For DMs
    char channelName[32];    // For group messages
    const char* content;
    size_t contentLength;
    uint64_t timestamp;
    uint64_t senderId;
    
    MeshCoreMessageInfo()
        : type(MeshCoreMessageType::GROUP_MESSAGE)
        , content(nullptr)
        , contentLength(0)
        , timestamp(0)
        , senderId(0)
    {
        memset(senderName, 0, sizeof(senderName));
        memset(recipientName, 0, sizeof(recipientName));
        memset(channelName, 0, sizeof(channelName));
    }
};

/**
 * Message Formatter
 * 
 * Formats MeshCore messages into BitChat format
 */
class MessageFormatter {
public:
    MessageFormatter();
    
    /**
     * Format MeshCore message for BitChat
     * 
     * @param info MeshCore message info
     * @param result Output BitChat message
     * @return true if formatted successfully
     */
    bool formatForBitChat(const MeshCoreMessageInfo& info,
                          mesh::ble::BitchatMessage& result);
    
    /**
     * Get last error status
     */
    bool wasSuccessful() const { return _lastSuccess; }

private:
    bool _lastSuccess;
    
    /**
     * Build formatted payload: "<sender> content"
     */
    size_t buildPayload(const MeshCoreMessageInfo& info,
                        uint8_t* buffer, size_t bufferSize);
    
    /**
     * Validate message info
     */
    bool validateInfo(const MeshCoreMessageInfo& info) const;
};

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
