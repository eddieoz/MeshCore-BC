/**
 * Story 7.2: Message Compression/Decompression - BDD Tests
 * 
 * Feature: BitChat Compression Support
 * 
 * TDD Approach:
 * 1. Write failing tests (RED)
 * 2. Implement to pass (GREEN)
 * 3. Refactor (REFACTOR)
 */

#include <unity.h>
#include "helpers/bitchat/Compression.h"

#ifdef ENABLE_BITCHAT

using namespace mesh::bitchat;

namespace test {
namespace bitchat {

// Test data
static const char TEST_DATA[] = "This is a test message that will be compressed. "
                                   "It contains some repetitive content that should compress well. "
                                   "Compression works best when there is repetition and patterns. "
                                   "This is a test message that will be compressed.";

/**
 * Scenario: Compress and decompress data
 * 
 * Given uncompressed data
 * When compressing then decompressing
 * Then recover original data
 */
void test_compress_decompress_roundtrip(void) {
    // Given: Uncompressed data
    const uint8_t* input = (const uint8_t*)TEST_DATA;
    size_t inputLen = strlen(TEST_DATA);
    
    // When: Compress
    uint8_t compressed[512];
    size_t compressedLen = 0;
    
    Compression compressor;
    bool compressOk = compressor.compress(input, inputLen, compressed, sizeof(compressed), &compressedLen);
    
    // Then: Should compress successfully
    TEST_ASSERT_TRUE(compressOk);
    TEST_ASSERT_TRUE(compressedLen > 0);
    // Note: In stub mode, compressed may be larger than input (header added)
    // Real compression would make it smaller
    
    // When: Decompress
    uint8_t decompressed[512];
    size_t decompressedLen = 0;
    
    bool decompressOk = compressor.decompress(compressed, compressedLen, decompressed, sizeof(decompressed), &decompressedLen);
    
    // Then: Should decompress successfully
    TEST_ASSERT_TRUE(decompressOk);
    TEST_ASSERT_EQUAL(inputLen, decompressedLen);
    TEST_ASSERT_EQUAL_MEMORY(input, decompressed, inputLen);
}

/**
 * Scenario: Small data may not compress well
 * 
 * Given small data
 * When compressing
 * Then may return uncompressed or slightly compressed
 */
void test_small_data_compression(void) {
    // Given: Small data
    const char* smallData = "Hi";
    const uint8_t* input = (const uint8_t*)smallData;
    size_t inputLen = strlen(smallData);
    
    // When: Compress
    uint8_t compressed[256];
    size_t compressedLen = 0;
    
    Compression compressor;
    bool compressOk = compressor.compress(input, inputLen, compressed, sizeof(compressed), &compressedLen);
    
    // Then: Should handle small data gracefully
    // May succeed or indicate data too small
    if (compressOk) {
        // If compressed, should be able to decompress
        uint8_t decompressed[256];
        size_t decompressedLen = 0;
        
        bool decompressOk = compressor.decompress(compressed, compressedLen, decompressed, sizeof(decompressed), &decompressedLen);
        TEST_ASSERT_TRUE(decompressOk);
        TEST_ASSERT_EQUAL(inputLen, decompressedLen);
        TEST_ASSERT_EQUAL_MEMORY(input, decompressed, inputLen);
    }
}

/**
 * Scenario: Compression buffer too small
 * 
 * Given output buffer too small
 * When compressing
 * Then return error
 */
void test_compression_buffer_too_small(void) {
    // Given: Data to compress
    const uint8_t* input = (const uint8_t*)TEST_DATA;
    size_t inputLen = strlen(TEST_DATA);
    
    // And: Too-small output buffer
    uint8_t compressed[10];
    size_t compressedLen = 0;
    
    // When: Compress
    Compression compressor;
    bool compressOk = compressor.compress(input, inputLen, compressed, sizeof(compressed), &compressedLen);
    
    // Then: Should fail (buffer too small)
    TEST_ASSERT_FALSE(compressOk);
}

/**
 * Scenario: Decompression buffer too small
 * 
 * Given valid compressed data
 * And too-small output buffer
 * When decompressing
 * Then return error
 */
void test_decompression_buffer_too_small(void) {
    // Given: Compressed data
    const uint8_t* input = (const uint8_t*)TEST_DATA;
    size_t inputLen = strlen(TEST_DATA);
    
    uint8_t compressed[512];
    size_t compressedLen = 0;
    
    Compression compressor;
    bool compressOk = compressor.compress(input, inputLen, compressed, sizeof(compressed), &compressedLen);
    TEST_ASSERT_TRUE(compressOk);
    
    // And: Too-small output buffer
    uint8_t decompressed[10];
    size_t decompressedLen = 0;
    
    // When: Decompress
    bool decompressOk = compressor.decompress(compressed, compressedLen, decompressed, sizeof(decompressed), &decompressedLen);
    
    // Then: Should fail (buffer too small)
    TEST_ASSERT_FALSE(decompressOk);
}

/**
 * Scenario: Check if data is compressed
 * 
 * Given compressed data
 * When checking isCompressed
 * Then return true
 */
void test_is_compressed_check(void) {
    // Given: Compressed data
    const uint8_t* input = (const uint8_t*)TEST_DATA;
    size_t inputLen = strlen(TEST_DATA);
    
    uint8_t compressed[512];
    size_t compressedLen = 0;
    
    Compression compressor;
    bool compressOk = compressor.compress(input, inputLen, compressed, sizeof(compressed), &compressedLen);
    TEST_ASSERT_TRUE(compressOk);
    
    // When/Then: Should detect as compressed
    TEST_ASSERT_TRUE(compressor.isCompressed(compressed, compressedLen));
    
    // Uncompressed data should not be detected as compressed
    TEST_ASSERT_FALSE(compressor.isCompressed(input, inputLen));
}

/**
 * Scenario: Get compression ratio
 * 
 * Given compressed data
 * When getting compression ratio
 * Then return original / compressed
 */
void test_compression_ratio(void) {
    // Given: Data
    const uint8_t* input = (const uint8_t*)TEST_DATA;
    size_t inputLen = strlen(TEST_DATA);
    
    uint8_t compressed[512];
    size_t compressedLen = 0;
    
    Compression compressor;
    bool compressOk = compressor.compress(input, inputLen, compressed, sizeof(compressed), &compressedLen);
    TEST_ASSERT_TRUE(compressOk);
    
    // When: Get ratio
    float ratio = compressor.getCompressionRatio(inputLen, compressedLen);
    
    // Then: Should be > 0
    TEST_ASSERT_TRUE(ratio > 0.0f);
    // Note: In stub mode, ratio may be < 1.0 (compressed larger)
    // Real compression would give ratio > 1.0
}

/**
 * Scenario: Empty data handling
 * 
 * Given empty input
 * When compressing
 * Then return error
 */
void test_compress_empty_data(void) {
    // Given: Empty data
    const uint8_t* input = nullptr;
    size_t inputLen = 0;
    
    uint8_t compressed[256];
    size_t compressedLen = 0;
    
    // When: Compress
    Compression compressor;
    bool compressOk = compressor.compress(input, inputLen, compressed, sizeof(compressed), &compressedLen);
    
    // Then: Should fail
    TEST_ASSERT_FALSE(compressOk);
}

/**
 * Scenario: Large data compression
 * 
 * Given large data (>1KB)
 * When compressing
 * Then handle appropriately
 */
void test_large_data_compression(void) {
    // Given: Large data
    uint8_t largeData[2048];
    for (size_t i = 0; i < sizeof(largeData); i++) {
        largeData[i] = 'A' + (i % 26);  // Repeating pattern
    }
    
    uint8_t compressed[4096];
    size_t compressedLen = 0;
    
    // When: Compress
    Compression compressor;
    bool compressOk = compressor.compress(largeData, sizeof(largeData), compressed, sizeof(compressed), &compressedLen);
    
    // Then: Should succeed
    TEST_ASSERT_TRUE(compressOk);
    
    // And: Should decompress correctly
    uint8_t decompressed[4096];
    size_t decompressedLen = 0;
    
    bool decompressOk = compressor.decompress(compressed, compressedLen, decompressed, sizeof(decompressed), &decompressedLen);
    TEST_ASSERT_TRUE(decompressOk);
    TEST_ASSERT_EQUAL(sizeof(largeData), decompressedLen);
    TEST_ASSERT_EQUAL_MEMORY(largeData, decompressed, sizeof(largeData));
}

} // namespace bitchat
} // namespace test

#endif // ENABLE_BITCHAT
