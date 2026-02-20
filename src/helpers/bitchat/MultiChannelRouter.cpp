/**
 * Story 5.3: Multi-Channel Encapsulation Routing - Implementation
 */

#ifdef ENABLE_BITCHAT

#include "MultiChannelRouter.h"
#include <cstring>

namespace mesh {
namespace bitchat {

MultiChannelRouter::MultiChannelRouter(ChannelRegistry& registry)
    : _registry(registry)
    , _hasDefault(false)
    , _lastStatus(RoutingStatus::CHANNEL_NOT_FOUND)
{
    memset(_defaultChannel, 0, sizeof(_defaultChannel));
}

bool MultiChannelRouter::routeMessage(const BitchatMessage& msg,
                                       const char* channelName,
                                       RoutingResult& result)
{
    // Reset result
    result = RoutingResult();
    result.timestamp = 1234567890;  // Would use actual time in production
    
    // Validate message
    if (msg.payloadLength == 0 || msg.payloadLength > BITCHAT_MAX_PAYLOAD_SIZE) {
        _lastStatus = RoutingStatus::INVALID_MESSAGE;
        result.status = _lastStatus;
        return false;
    }
    
    // Find channel key (handles default fallback)
    char actualChannel[BITCHAT_MAX_CHANNEL_NAME_LEN];
    const uint8_t* channelKey = findChannelKey(channelName, actualChannel, sizeof(actualChannel));
    
    if (channelKey == nullptr) {
        _lastStatus = RoutingStatus::CHANNEL_NOT_FOUND;
        result.status = _lastStatus;
        return false;
    }
    
    // Copy actual channel used to result
    strncpy(result.routedChannel, actualChannel, sizeof(result.routedChannel) - 1);
    result.routedChannel[sizeof(result.routedChannel) - 1] = '\0';
    
    // Determine if we need fragmentation
    size_t messageSize = BITCHAT_HEADER_SIZE + msg.payloadLength;
    bool needsFrag = BitchatMessageEncapsulator::needsFragmentation(messageSize);
    
    bool encapsulationOk;
    if (needsFrag) {
        // Fragmentation required
        encapsulationOk = _encapsulator.encapsulateWithFragmentation(
            msg,
            channelKey,
            result.packets,
            BITCHAT_MAX_FRAGMENTS,
            result.packetCount
        );
    } else {
        // Single packet
        result.packetCount = 1;
        encapsulationOk = _encapsulator.encapsulate(msg, channelKey, result.packets[0]);
    }
    
    if (!encapsulationOk) {
        _lastStatus = RoutingStatus::ENCAPSULATION_FAILED;
        result.status = _lastStatus;
        result.packetCount = 0;
        return false;
    }
    
    // Determine status based on whether default was used
    if (isNullOrEmpty(channelName)) {
        _lastStatus = RoutingStatus::ROUTED_TO_DEFAULT;
    } else {
        char normalized[BITCHAT_MAX_ROUTER_CHANNEL_NAME];
        if (normalizeChannelName(channelName, normalized, sizeof(normalized))) {
            if (strcmp(normalized, actualChannel) == 0) {
                _lastStatus = RoutingStatus::ROUTED;
            } else {
                _lastStatus = RoutingStatus::ROUTED_TO_DEFAULT;
            }
        } else {
            _lastStatus = RoutingStatus::ROUTED_TO_DEFAULT;
        }
    }
    
    result.status = _lastStatus;
    return true;
}

bool MultiChannelRouter::setDefaultChannel(const char* channelName)
{
    if (channelName == nullptr) {
        clearDefaultChannel();
        return true;
    }
    
    // Normalize the channel name
    char normalized[BITCHAT_MAX_ROUTER_CHANNEL_NAME];
    if (!normalizeChannelName(channelName, normalized, sizeof(normalized))) {
        return false;
    }
    
    // Verify channel exists in registry
    if (!_registry.hasChannel(normalized)) {
        return false;
    }
    
    // Store as default
    strncpy(_defaultChannel, normalized, sizeof(_defaultChannel) - 1);
    _defaultChannel[sizeof(_defaultChannel) - 1] = '\0';
    _hasDefault = true;
    
    return true;
}

void MultiChannelRouter::clearDefaultChannel()
{
    memset(_defaultChannel, 0, sizeof(_defaultChannel));
    _hasDefault = false;
}

const char* MultiChannelRouter::getDefaultChannel() const
{
    return _hasDefault ? _defaultChannel : nullptr;
}

bool MultiChannelRouter::isChannelRegistered(const char* channelName) const
{
    if (channelName == nullptr) {
        return false;
    }
    
    char normalized[BITCHAT_MAX_ROUTER_CHANNEL_NAME];
    if (!normalizeChannelName(channelName, normalized, sizeof(normalized))) {
        return false;
    }
    
    return _registry.hasChannel(normalized);
}

bool MultiChannelRouter::normalizeChannelName(const char* input, char* output, size_t outputSize) const
{
    if (input == nullptr || output == nullptr || outputSize == 0) {
        return false;
    }
    
    // Skip # prefix if present
    const char* start = input;
    if (input[0] == '#') {
        start = input + 1;
    }
    
    // Check for empty name after #
    if (start[0] == '\0') {
        return false;
    }
    
    // Copy normalized name
    size_t len = strlen(start);
    if (len >= outputSize) {
        len = outputSize - 1;
    }
    
    memcpy(output, start, len);
    output[len] = '\0';
    
    return true;
}

const uint8_t* MultiChannelRouter::findChannelKey(const char* channelName,
                                                   char* actualChannel,
                                                   size_t actualChannelSize)
{
    // Handle null/empty channel names - use default if available
    if (isNullOrEmpty(channelName)) {
        if (_hasDefault) {
            strncpy(actualChannel, _defaultChannel, actualChannelSize - 1);
            actualChannel[actualChannelSize - 1] = '\0';
            return _registry.lookupByName(_defaultChannel);
        }
        return nullptr;
    }
    
    // Normalize channel name
    char normalized[BITCHAT_MAX_ROUTER_CHANNEL_NAME];
    if (!normalizeChannelName(channelName, normalized, sizeof(normalized))) {
        // Invalid name, try default
        if (_hasDefault) {
            strncpy(actualChannel, _defaultChannel, actualChannelSize - 1);
            actualChannel[actualChannelSize - 1] = '\0';
            return _registry.lookupByName(_defaultChannel);
        }
        return nullptr;
    }
    
    // Look up in registry
    const uint8_t* key = _registry.lookupByName(normalized);
    if (key != nullptr) {
        // Found - use requested channel
        strncpy(actualChannel, normalized, actualChannelSize - 1);
        actualChannel[actualChannelSize - 1] = '\0';
        return key;
    }
    
    // Not found - use default if available
    if (_hasDefault) {
        strncpy(actualChannel, _defaultChannel, actualChannelSize - 1);
        actualChannel[actualChannelSize - 1] = '\0';
        return _registry.lookupByName(_defaultChannel);
    }
    
    // Not found and no default
    return nullptr;
}

bool MultiChannelRouter::isNullOrEmpty(const char* str) const
{
    return (str == nullptr) || (str[0] == '\0');
}

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
