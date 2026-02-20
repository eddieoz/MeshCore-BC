#pragma once

/**
 * Story 9.1: Unit Tests for Protocol Layer
 * 
 * Test vectors for protocol parsing validation
 * - Known-good packets for each message type
 * - Fuzz testing data generation
 */

#ifdef ENABLE_BITCHAT

#include <cstdint>
#include <cstring>

namespace mesh {
namespace bitchat {

// Maximum test vector size
#define TEST_VECTOR_MAX_SIZE 512

/**
 * Test vector structure
 */
struct TestVector {
    uint8_t data[TEST_VECTOR_MAX_SIZE];
    size_t length;
    uint8_t expectedVersion;
    uint8_t expectedType;
    bool isValid;
    
    TestVector()
        : length(0)
        , expectedVersion(1)
        , expectedType(0)
        , isValid(true)
    {
        memset(data, 0, sizeof(data));
    }
};

/**
 * Protocol Test Vectors
 * 
 * Provides known-good test vectors for protocol validation
 */
class ProtocolTestVectors {
public:
    ProtocolTestVectors();
    
    /**
     * Get test vector for MESSAGE type
     * @param out Output test vector
     * @return true if vector available
     */
    bool getMessageVector(TestVector& out) const;
    
    /**
     * Get test vector for ANNOUNCE type
     * @param out Output test vector
     * @return true if vector available
     */
    bool getAnnounceVector(TestVector& out) const;
    
    /**
     * Get test vector for LEAVE type
     * @param out Output test vector
     * @return true if vector available
     */
    bool getLeaveVector(TestVector& out) const;
    
    /**
     * Get test vector for CHANNEL type
     * @param out Output test vector
     * @return true if vector available
     */
    bool getChannelVector(TestVector& out) const;
    
    /**
     * Generate random data for fuzz testing
     * @param buffer Output buffer
     * @param maxLen Maximum length to generate
     * @param outLen Actual length generated
     */
    void generateRandomData(uint8_t* buffer, size_t maxLen, size_t* outLen) const;
    
    /**
     * Get number of available test vectors
     */
    size_t getVectorCount() const;

private:
    // Simple LCG random number generator for fuzzing
    mutable uint32_t _randomSeed;
    
    uint8_t nextRandomByte() const;
};

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
