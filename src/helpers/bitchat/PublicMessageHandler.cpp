/**
 * Story 8.1: Public Message to BitChat - Implementation
 */

#ifdef ENABLE_BITCHAT

#include "PublicMessageHandler.h"

namespace mesh {
namespace bitchat {

PublicMessageHandler::PublicMessageHandler(ChannelRegistry& registry)
    : _registry(registry)
{
    // Default target channel is "mesh"
    strncpy(_targetChannel, "mesh", sizeof(_targetChannel) - 1);
    _targetChannel[sizeof(_targetChannel) - 1] = '\0';
}

void PublicMessageHandler::setTargetChannel(const char* channelName)
{
    if (channelName != nullptr) {
        strncpy(_targetChannel, channelName, sizeof(_targetChannel) - 1);
        _targetChannel[sizeof(_targetChannel) - 1] = '\0';
    }
}

const char* PublicMessageHandler::getTargetChannel() const
{
    return _targetChannel;
}

bool PublicMessageHandler::routeToBitChat(const MeshCorePublicMessage& pubMsg, mesh::ble::BitchatMessage& result)
{
    // Validate message
    if (!validateMessage(pubMsg)) {
        return false;
    }
    
    // Check if target channel exists
    const uint8_t* channelKey = _registry.lookupByName(_targetChannel);
    if (channelKey == nullptr) {
        return false;  // Unknown target channel
    }
    
    // Format for BitChat using MessageFormatter
    MeshCoreMessageInfo info;
    info.type = MeshCoreMessageType::PUBLIC_MESSAGE;
    strncpy(info.senderName, pubMsg.senderName, sizeof(info.senderName) - 1);
    strncpy(info.channelName, _targetChannel, sizeof(info.channelName) - 1);
    info.content = pubMsg.content;
    info.contentLength = strlen(pubMsg.content);
    info.timestamp = pubMsg.timestamp;
    // Generate a simple senderId from senderName hash
    uint64_t senderId = 0;
    for (size_t i = 0; pubMsg.senderName[i] != '\0'; i++) {
        senderId = senderId * 31 + pubMsg.senderName[i];
    }
    info.senderId = senderId ? senderId : 1;  // Ensure non-zero
    
    return _formatter.formatForBitChat(info, result);
}

bool PublicMessageHandler::validateMessage(const MeshCorePublicMessage& pubMsg) const
{
    // Check sender name
    if (pubMsg.senderName == nullptr || pubMsg.senderName[0] == '\0') {
        return false;
    }
    
    // Check content
    if (pubMsg.content == nullptr || pubMsg.content[0] == '\0') {
        return false;
    }
    
    // Check timestamp
    if (pubMsg.timestamp == 0) {
        return false;
    }
    
    return true;
}

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
