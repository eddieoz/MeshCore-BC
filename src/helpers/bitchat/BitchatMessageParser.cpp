#include "BitchatMessageParser.h"

#ifdef ENABLE_BITCHAT

namespace mesh {
namespace bitchat {

BitchatMessageParser::BitchatMessageParser()
    : _lastError(nullptr)
{
}

bool BitchatMessageParser::isSupportedType(uint8_t type) {
    return type == BITCHAT_MSG_MESSAGE || 
           type == BITCHAT_MSG_ANNOUNCE ||
           type == BITCHAT_MSG_PING ||
           type == BITCHAT_MSG_PONG ||
           type == BITCHAT_MSG_REQUEST_SYNC;
}

bool BitchatMessageParser::parseMessage(const uint8_t* data, size_t len, ParsedBitchatMessage& outMsg) {
    if (!data || len < BITCHAT_HEADER_SIZE) {
        setError("Message too short");
        return false;
    }
    
    // Parse header
    size_t offset = 0;
    outMsg.version = data[offset++];
    
    if (outMsg.version != BITCHAT_VERSION) {
        setError("Unsupported version");
        return false;
    }
    
    outMsg.type = data[offset++];
    outMsg.ttl = data[offset++];
    
    // Timestamp (8 bytes, big-endian)
    outMsg.timestamp = 0;
    for (int i = 0; i < 8; i++) {
        outMsg.timestamp = (outMsg.timestamp << 8) | data[offset++];
    }
    
    // Flags
    uint8_t flags = data[offset++];
    (void)flags;  // May be used later for compression/signature
    
    // Payload length
    uint16_t payloadLen = (static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1];
    offset += 2;
    
    // Sender ID (8 bytes, little-endian)
    outMsg.senderId = 0;
    for (int i = 0; i < 8; i++) {
        outMsg.senderId |= (static_cast<uint64_t>(data[offset + i]) << (i * 8));
    }
    offset += 8;
    
    // Check payload length
    if (offset + payloadLen > len) {
        setError("Payload length exceeds message size");
        return false;
    }
    
    // Store original payload reference
    outMsg.originalPayload = &data[offset];
    outMsg.originalPayloadLen = payloadLen;
    
    // Parse payload TLVs
    if (payloadLen > 0) {
        if (!parsePayload(&data[offset], payloadLen, outMsg)) {
            return false;
        }
    }
    
    return true;
}

bool BitchatMessageParser::parsePayload(const uint8_t* payload, size_t len, ParsedBitchatMessage& outMsg) {
    if (!payload || len == 0) {
        return true;  // Empty payload is valid
    }
    
    size_t offset = 0;
    while (offset < len) {
        if (!parseTLV(payload, len, offset, outMsg)) {
            return false;
        }
    }
    
    return true;
}

bool BitchatMessageParser::parseTLV(const uint8_t* data, size_t len, size_t& offset, ParsedBitchatMessage& outMsg) {
    // Need at least 2 bytes for type and length
    if (offset + 2 > len) {
        setError("TLV truncated");
        return false;
    }
    
    uint8_t type = data[offset++];
    uint8_t tlen = data[offset++];
    
    // Check value fits
    if (offset + tlen > len) {
        setError("TLV value truncated");
        return false;
    }
    
    const uint8_t* value = &data[offset];
    offset += tlen;
    
    // Process based on type
    switch (type) {
        case BITCHAT_TLV_MESSAGE_TEXT:
            // Text message content
            if (tlen < BITCHAT_MAX_MESSAGE_TEXT) {
                memcpy(outMsg.text, value, tlen);
                outMsg.text[tlen] = '\0';
            } else {
                memcpy(outMsg.text, value, BITCHAT_MAX_MESSAGE_TEXT - 1);
                outMsg.text[BITCHAT_MAX_MESSAGE_TEXT - 1] = '\0';
            }
            break;
            
        case BITCHAT_TLV_CHANNEL_NAME:
            // Channel name
            if (tlen < BITCHAT_MAX_CHANNEL_NAME) {
                memcpy(outMsg.channel, value, tlen);
                outMsg.channel[tlen] = '\0';
            } else {
                memcpy(outMsg.channel, value, BITCHAT_MAX_CHANNEL_NAME - 1);
                outMsg.channel[BITCHAT_MAX_CHANNEL_NAME - 1] = '\0';
            }
            break;
            
        case BITCHAT_TLV_TIMESTAMP:
            // Extended timestamp (optional)
            if (tlen == 8) {
                uint64_t ts = 0;
                for (int i = 0; i < 8; i++) {
                    ts = (ts << 8) | value[i];
                }
                outMsg.timestamp = ts;
            }
            break;
            
        case BITCHAT_TLV_REPLY_TO:
            // Reply to message ID
            if (tlen == 8) {
                outMsg.replyToMsgId = 0;
                for (int i = 0; i < 8; i++) {
                    outMsg.replyToMsgId |= (static_cast<uint64_t>(value[i]) << (i * 8));
                }
                outMsg.hasReplyTo = true;
            }
            break;
            
        case BITCHAT_TLV_SIGNATURE:
            // Signature - just skip for now
            break;
            
        default:
            // Unknown TLV - skip
            break;
    }
    
    return true;
}

bool BitchatMessageParser::validateHeader(const uint8_t* data, size_t len) {
    if (!data || len < BITCHAT_HEADER_SIZE) {
        return false;
    }
    
    if (data[0] != BITCHAT_VERSION) {
        return false;
    }
    
    return isSupportedType(data[1]);
}

void BitchatMessageParser::setError(const char* error) {
    _lastError = error;
}

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
