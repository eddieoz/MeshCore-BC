/**
 * Story 4.4: Hashtag Room Key Derivation - BDD Tests
 * 
 * Feature: BitChat-Compatible Hashtag Key Derivation
 */

#include <unity.h>
#include "helpers/bitchat/HashtagKey.h"

#ifdef ENABLE_BITCHAT

using namespace mesh::bitchat;

namespace test {
namespace bitchat {

// Known test vectors (SHA256 of hashtag)
// echo -n "#mesh" | sha256sum
static const uint8_t MESH_KEY[32] = {
    0x5b, 0x66, 0x4c, 0xde, 0x0b, 0x08, 0xb2, 0x20,
    0x61, 0x21, 0x13, 0xdb, 0x98, 0x06, 0x50, 0xf3,
    0x22, 0xd5, 0x2c, 0xf8, 0xa8, 0x25, 0x47, 0x71,
    0x0e, 0xc4, 0x5d, 0xa6, 0x4e, 0x11, 0x99, 0x77
};

// echo -n "#general" | sha256sum  
static const uint8_t GENERAL_KEY[32] = {
    0x4c, 0x49, 0xf3, 0xf2, 0x46, 0x29, 0xf5, 0xee,
    0x4a, 0xd5, 0xb3, 0x96, 0x5d, 0xb4, 0x79, 0x85,
    0xe0, 0x1e, 0xaf, 0xc5, 0x39, 0x22, 0xf9, 0xd0,
    0x11, 0xee, 0xd5, 0x13, 0xe4, 0x1e, 0x14, 0x9d
};

/**
 * Scenario: Derive channel key from hashtag name
 * 
 * Given channel name "#mesh"
 * When key is derived
 * Then result should be SHA256("#mesh")
 */
void test_derive_hashtag_key_with_prefix(void) {
    uint8_t key[32];
    
    // When: Derive key from "#mesh"
    bool derived = deriveHashtagKey("#mesh", key);
    
    // Then: Derived successfully
    TEST_ASSERT_TRUE(derived);
    
    // And: Matches expected key
    TEST_ASSERT_EQUAL_HEX8_ARRAY(MESH_KEY, key, 32);
}

/**
 * Scenario: Derive key from name without # prefix
 * 
 * Given channel name "mesh" (no #)
 * When key is derived
 * Then # prefix is added and SHA256 computed
 */
void test_derive_hashtag_key_without_prefix(void) {
    uint8_t key[32];
    
    // When: Derive key from "mesh" (no #)
    bool derived = deriveHashtagKey("mesh", key);
    
    // Then: Derived successfully
    TEST_ASSERT_TRUE(derived);
    
    // And: Same as "#mesh" result
    TEST_ASSERT_EQUAL_HEX8_ARRAY(MESH_KEY, key, 32);
}

/**
 * Scenario: Different hashtags produce different keys
 * 
 * Given different channel names
 * When keys are derived
 * Then different keys produced
 */
void test_different_hashtags_different_keys(void) {
    uint8_t key1[32], key2[32];
    
    // Derive both keys
    TEST_ASSERT_TRUE(deriveHashtagKey("#mesh", key1));
    TEST_ASSERT_TRUE(deriveHashtagKey("#general", key2));
    
    // Then: Different keys
    TEST_ASSERT_TRUE(memcmp(key1, key2, 32) != 0);
    
    // And: General key matches expected
    TEST_ASSERT_EQUAL_HEX8_ARRAY(GENERAL_KEY, key2, 32);
}

/**
 * Scenario: Normalize hashtag adds # prefix
 * 
 * Given name without #
 * When normalized
 * Then # prefix added
 */
void test_normalize_hashtag_adds_prefix(void) {
    char output[64];
    
    // When: Normalize "mesh"
    bool result = normalizeHashtag("mesh", output, sizeof(output));
    
    // Then: Successful
    TEST_ASSERT_TRUE(result);
    
    // And: Has # prefix
    TEST_ASSERT_EQUAL_STRING("#mesh", output);
}

/**
 * Scenario: Normalize hashtag preserves existing #
 * 
 * Given name with #
 * When normalized
 * Then unchanged
 */
void test_normalize_hashtag_preserves_prefix(void) {
    char output[64];
    
    // When: Normalize "#general"
    bool result = normalizeHashtag("#general", output, sizeof(output));
    
    // Then: Successful
    TEST_ASSERT_TRUE(result);
    
    // And: Still "#general"
    TEST_ASSERT_EQUAL_STRING("#general", output);
}

/**
 * Scenario: Same hashtag produces same key consistently
 * 
 * Given same hashtag
 * When derived multiple times
 * Then same key produced
 */
void test_hashtag_key_consistent(void) {
    uint8_t key1[32], key2[32], key3[32];
    
    // Derive same hashtag multiple times
    TEST_ASSERT_TRUE(deriveHashtagKey("#test", key1));
    TEST_ASSERT_TRUE(deriveHashtagKey("test", key2));   // Without #
    TEST_ASSERT_TRUE(deriveHashtagKey("#test", key3));
    
    // All should match
    TEST_ASSERT_EQUAL_HEX8_ARRAY(key1, key2, 32);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(key2, key3, 32);
}

/**
 * Scenario: Null parameters handled gracefully
 * 
 * Given null input
 * When deriving
 * Then returns false
 */
void test_hashtag_null_parameters(void) {
    uint8_t key[32];
    
    TEST_ASSERT_FALSE(deriveHashtagKey(nullptr, key));
    TEST_ASSERT_FALSE(deriveHashtagKey("#test", nullptr));
}

/**
 * Scenario: Empty hashtag handled
 * 
 * Given empty string
 * When deriving
 * Then treated as "#"
 */
void test_hashtag_empty_string(void) {
    uint8_t key[32];
    
    // Empty string should become "#"
    bool derived = deriveHashtagKey("", key);
    TEST_ASSERT_TRUE(derived);
    
    // Verify it's SHA256("#")
    uint8_t expected[32];
    deriveHashtagKey("#", expected);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, key, 32);
}

} // namespace bitchat
} // namespace test

#else // !ENABLE_BITCHAT

namespace test {
namespace bitchat {
void test_hashtag_placeholder(void) {
    TEST_IGNORE_MESSAGE("BitChat disabled - ENABLE_BITCHAT not defined");
}
} // namespace bitchat
} // namespace test

#endif // ENABLE_BITCHAT
