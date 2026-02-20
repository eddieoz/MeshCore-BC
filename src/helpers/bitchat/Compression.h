#pragma once

/**
 * Story 7.2: Message Compression/Decompression
 * 
 * Handles BitChat message compression
 * - Compress/decompress using zlib/deflate
 * - Memory-constrained for embedded platforms
 * - Detect if data is compressed
 */

#ifdef ENABLE_BITCHAT

#include <cstdint>
#include <cstring>

namespace mesh {
namespace bitchat {

// Magic bytes for compressed data
#define COMPRESSION_MAGIC_1 0x78  // zlib header
#define COMPRESSION_MAGIC_2 0x9C  // zlib header

// Maximum compression buffer size (2KB)
#define COMPRESSION_MAX_SIZE 2048

/**
 * Compression handler
 * 
 * Compresses and decompresses BitChat message payloads
 * Uses miniz/tinfl for embedded compatibility
 */
class Compression {
public:
    Compression();
    
    /**
     * Compress data
     * 
     * @param input Input data
     * @param inputLen Input length
     * @param output Output buffer
     * @param outputLen Output buffer size
     * @param outCompressedLen Actual compressed length
     * @return true if compressed successfully
     */
    bool compress(const uint8_t* input, size_t inputLen,
                  uint8_t* output, size_t outputLen,
                  size_t* outCompressedLen);
    
    /**
     * Decompress data
     * 
     * @param input Compressed data
     * @param inputLen Compressed length
     * @param output Output buffer
     * @param outputLen Output buffer size
     * @param outDecompressedLen Actual decompressed length
     * @return true if decompressed successfully
     */
    bool decompress(const uint8_t* input, size_t inputLen,
                    uint8_t* output, size_t outputLen,
                    size_t* outDecompressedLen);
    
    /**
     * Check if data appears to be compressed
     * 
     * @param data Data to check
     * @param len Data length
     * @return true if likely compressed
     */
    bool isCompressed(const uint8_t* data, size_t len) const;
    
    /**
     * Get compression ratio
     * 
     * @param originalLen Original length
     * @param compressedLen Compressed length
     * @return Compression ratio (original/compressed)
     */
    float getCompressionRatio(size_t originalLen, size_t compressedLen) const;
    
    /**
     * Get maximum supported input size
     */
    size_t getMaxInputSize() const { return COMPRESSION_MAX_SIZE; }

private:
    // For native test environment: stub implementation
    // In real build with miniz, this would use actual compression
    bool _stubMode;
};

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
