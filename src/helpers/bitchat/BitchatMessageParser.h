#pragma once

/**
 * Story 5.1: BitChat Message Reception
 * 
 * Receive and Parse BitChat Messages
 * - Parse MESSAGE type from BitChat
 * - Extract sender, content, channel, timestamp
 * - Handle TLV payload format
 */

#ifdef ENABLE_BITCHAT

#include "../ble/BitchatBLEService.h"
#include <cstdint>
#include <cstring>

namespace mesh {
namespace bitchat {

// TLV types for MESSAGE payload
#define BITCHAT_TLV_MESSAGE_TEXT 0x10
#define BITCHAT_TLV_CHANNEL_NAME 0x11
#define BITCHAT_TLV_TIMESTAMP 0x12
#define BITCHAT_TLV_REPLY_TO 0x13
#define BITCHAT_TLV_SIGNATURE 0x20

// Maximum message sizes
#define BITCHAT_MAX_MESSAGE_TEXT 1024
#define BITCHAT_MAX_CHANNEL_NAME 32

/**
 * Parsed BitChat message structure
 */
struct ParsedBitchatMessage {
    uint8_t version;
    uint8_t type;
    uint8_t ttl;
    uint64_t timestamp;
    uint64_t senderId;
    
    // Parsed TLV fields
    char channel[BITCHAT_MAX_CHANNEL_NAME];
    char text[BITCHAT_MAX_MESSAGE_TEXT];
    uint64_t replyToMsgId;
    bool hasReplyTo;
    
    // Original payload for reference
    const uint8_t* originalPayload;
    size_t originalPayloadLen;
    
    ParsedBitchatMessage()
        : version(0)
        , type(0)
        , ttl(0)
        , timestamp(0)
        , senderId(0)
        , replyToMsgId(0)
        , hasReplyTo(false)
        , originalPayload(nullptr)
        , originalPayloadLen(0)
    {
        memset(channel, 0, sizeof(channel));
        memset(text, 0, sizeof(text));
    }
};

/**
 * BitChat Message Parser
 * 
 * Parses incoming BitChat messages and extracts fields
 */
class BitchatMessageParser {
public:
    BitchatMessageParser();
    
    // Parse a complete BitChat message packet
    // Returns true if parsing successful
    bool parseMessage(const uint8_t* data, size_t len, ParsedBitchatMessage& outMsg);
    
    // Parse just the TLV payload (after header)
    bool parsePayload(const uint8_t* payload, size_t len, ParsedBitchatMessage& outMsg);
    
    // Get last error message
    const char* getLastError() const { return _lastError; }
    
    // Check if message type is supported
    static bool isSupportedType(uint8_t type);

private:
    const char* _lastError;
    
    // Parse individual TLV
    bool parseTLV(const uint8_t* data, size_t len, size_t& offset, ParsedBitchatMessage& outMsg);
    
    // Header validation
    bool validateHeader(const uint8_t* data, size_t len);
    
    // Error handling
    void setError(const char* error);
};

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
