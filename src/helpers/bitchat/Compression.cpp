/**
 * Story 7.2: Message Compression/Decompression - Implementation
 * 
 * NOTE: This is a stub implementation for the native test environment.
 * In production with miniz available, this would use actual zlib/deflate
 * compression. The stub adds a header and copies data to simulate compression.
 */

#ifdef ENABLE_BITCHAT

#include "Compression.h"

namespace mesh {
namespace bitchat {

// Stub header for "compressed" data
#define STUB_COMPRESSION_HEADER 0xC0, 0xFF, 0xEE, 0xBA, 0xBE
#define STUB_HEADER_SIZE 5

Compression::Compression()
    : _stubMode(true)
{
}

bool Compression::compress(const uint8_t* input, size_t inputLen,
                           uint8_t* output, size_t outputLen,
                           size_t* outCompressedLen)
{
    // Validate inputs
    if (input == nullptr || output == nullptr || outCompressedLen == nullptr) {
        return false;
    }
    
    if (inputLen == 0) {
        return false;
    }
    
    // Check size limits
    if (inputLen > COMPRESSION_MAX_SIZE) {
        return false;
    }
    
    // Check output buffer size (need room for header + data)
    if (outputLen < inputLen + STUB_HEADER_SIZE) {
        return false;
    }
    
    // In stub mode: add header and copy data
    // In real implementation: use miniz to compress
    
    // Add stub header
    output[0] = 0xC0;
    output[1] = 0xFF;
    output[2] = 0xEE;
    output[3] = 0xBA;
    output[4] = 0xBE;
    
    // Copy input data
    memcpy(&output[STUB_HEADER_SIZE], input, inputLen);
    
    *outCompressedLen = inputLen + STUB_HEADER_SIZE;
    return true;
}

bool Compression::decompress(const uint8_t* input, size_t inputLen,
                             uint8_t* output, size_t outputLen,
                             size_t* outDecompressedLen)
{
    // Validate inputs
    if (input == nullptr || output == nullptr || outDecompressedLen == nullptr) {
        return false;
    }
    
    if (inputLen < STUB_HEADER_SIZE) {
        return false;
    }
    
    // Check for stub header
    if (input[0] != 0xC0 || input[1] != 0xFF || input[2] != 0xEE ||
        input[3] != 0xBA || input[4] != 0xBE) {
        // Not our stub compressed data - could be real zlib or uncompressed
        // For stub: treat as error (real implementation would try zlib)
        return false;
    }
    
    // Calculate decompressed length
    size_t dataLen = inputLen - STUB_HEADER_SIZE;
    
    // Check output buffer
    if (outputLen < dataLen) {
        return false;
    }
    
    // Copy data
    memcpy(output, &input[STUB_HEADER_SIZE], dataLen);
    *outDecompressedLen = dataLen;
    
    return true;
}

bool Compression::isCompressed(const uint8_t* data, size_t len) const
{
    if (data == nullptr || len < STUB_HEADER_SIZE) {
        return false;
    }
    
    // Check for stub header
    if (data[0] == 0xC0 && data[1] == 0xFF && data[2] == 0xEE &&
        data[3] == 0xBA && data[4] == 0xBE) {
        return true;
    }
    
    // Check for zlib header (0x78 0x9C is common)
    if (data[0] == COMPRESSION_MAGIC_1 && data[1] == COMPRESSION_MAGIC_2) {
        return true;
    }
    
    return false;
}

float Compression::getCompressionRatio(size_t originalLen, size_t compressedLen) const
{
    if (compressedLen == 0) {
        return 0.0f;
    }
    
    return (float)originalLen / (float)compressedLen;
}

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
