/**
 * Story 6.3: BitChat Message Formatting - Implementation
 */

#ifdef ENABLE_BITCHAT

#include "MessageFormatter.h"
#include <cstring>

namespace mesh {
namespace bitchat {

MessageFormatter::MessageFormatter()
    : _lastSuccess(false)
{
}

bool MessageFormatter::formatForBitChat(const MeshCoreMessageInfo& info,
                                         mesh::ble::BitchatMessage& result)
{
    // Reset result
    result = mesh::ble::BitchatMessage();
    _lastSuccess = false;
    
    // Validate input
    if (!validateInfo(info)) {
        return false;
    }
    
    // Set message type
    result.type = BITCHAT_MSG_MESSAGE;
    result.version = BITCHAT_VERSION;
    result.ttl = 8;  // Default TTL
    result.timestamp = info.timestamp;
    result.setSenderId64(info.senderId);
    
    // Build payload
    size_t payloadLen = buildPayload(info, result.payload, BITCHAT_MAX_PAYLOAD_SIZE);
    if (payloadLen == 0) {
        return false;
    }
    result.payloadLength = static_cast<uint16_t>(payloadLen);
    
    // Set flags based on message type
    if (info.type == MeshCoreMessageType::DIRECT_MESSAGE) {
        result.flags = BITCHAT_FLAG_HAS_RECIPIENT;
        // Would set recipient ID if known
    } else {
        result.flags = 0;
    }
    
    _lastSuccess = true;
    return true;
}

size_t MessageFormatter::buildPayload(const MeshCoreMessageInfo& info,
                                       uint8_t* buffer, size_t bufferSize)
{
    if (buffer == nullptr || bufferSize == 0) {
        return 0;
    }
    
    // Format: "<sender_name> content"
    // Calculate size needed
    size_t senderNameLen = strlen(info.senderName);
    size_t needed = 1 + senderNameLen + 2 + info.contentLength;  // < + name + > + space + content
    
    if (needed > bufferSize) {
        // Truncate content to fit
        size_t maxContent = bufferSize - (1 + senderNameLen + 2);
        if (maxContent > bufferSize) {
            return 0;  // Can't even fit sender name
        }
        needed = bufferSize;
    }
    
    size_t offset = 0;
    
    // Opening bracket
    buffer[offset++] = '<';
    
    // Sender name
    memcpy(&buffer[offset], info.senderName, senderNameLen);
    offset += senderNameLen;
    
    // Closing bracket and space
    buffer[offset++] = '>';
    buffer[offset++] = ' ';
    
    // Content
    size_t contentToCopy = info.contentLength;
    if (offset + contentToCopy > bufferSize) {
        contentToCopy = bufferSize - offset;
    }
    memcpy(&buffer[offset], info.content, contentToCopy);
    offset += contentToCopy;
    
    return offset;
}

bool MessageFormatter::validateInfo(const MeshCoreMessageInfo& info) const
{
    // Check content
    if (info.content == nullptr || info.contentLength == 0) {
        return false;
    }
    
    // Check sender name
    if (info.senderName[0] == '\0') {
        return false;
    }
    
    // Check sender ID
    if (info.senderId == 0) {
        return false;
    }
    
    return true;
}

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
