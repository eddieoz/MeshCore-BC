/**
 * Story 4.2: #mesh Channel Auto-Registration - BDD Tests
 * 
 * Feature: Automatic #mesh Channel Mapping
 */

#include <unity.h>
#include "helpers/bitchat/ChannelRegistry.h"

#ifdef ENABLE_BITCHAT

using namespace mesh::bitchat;

namespace test {
namespace bitchat {

// Expected #mesh channel key (SHA256 of "#mesh")
// Calculated: echo -n "#mesh" | sha256sum
// 5b664cde0b08b220612113db980650f322d52cf8a82547710ec45da64e119977
static const uint8_t EXPECTED_MESH_KEY[32] = {
    0x5b, 0x66, 0x4c, 0xde, 0x0b, 0x08, 0xb2, 0x20,
    0x61, 0x21, 0x13, 0xdb, 0x98, 0x06, 0x50, 0xf3,
    0x22, 0xd5, 0x2c, 0xf8, 0xa8, 0x25, 0x47, 0x71,
    0x0e, 0xc4, 0x5d, 0xa6, 0x4e, 0x11, 0x99, 0x77
};

/**
 * Scenario: Default #mesh channel setup
 * 
 * Given BitChat is enabled
 * When bridge initializes
 * Then #mesh hashtag channel should be auto-registered
 * And should use SHA256("#mesh") as key
 */
void test_mesh_channel_auto_registration(void) {
    ChannelRegistry registry;
    
    // Initially no channels
    TEST_ASSERT_EQUAL(0, registry.getChannelCount());
    TEST_ASSERT_FALSE(registry.hasChannel("mesh"));
    
    // When: Auto-register #mesh channel
    bool registered = registry.registerDefaultMeshChannel();
    
    // Then: Channel registered
    TEST_ASSERT_TRUE(registered);
    TEST_ASSERT_EQUAL(1, registry.getChannelCount());
    TEST_ASSERT_TRUE(registry.hasChannel("mesh"));
    
    // And: Channel has correct key
    const uint8_t* key = registry.lookupByName("mesh");
    TEST_ASSERT_NOT_NULL(key);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(EXPECTED_MESH_KEY, key, 32);
}

/**
 * Scenario: #mesh key is derived correctly
 * 
 * Given SHA256 of "#mesh"
 * When deriving key
 * Then matches expected value
 */
void test_mesh_key_derivation(void) {
    uint8_t derivedKey[32];
    
    // When: Derive key from "#mesh"
    bool derived = ChannelRegistry::deriveKeyFromHashtag("#mesh", derivedKey);
    
    // Then: Derived successfully
    TEST_ASSERT_TRUE(derived);
    
    // And: Matches expected key
    TEST_ASSERT_EQUAL_HEX8_ARRAY(EXPECTED_MESH_KEY, derivedKey, 32);
}

/**
 * Scenario: #mesh channel lookup works bidirectionally
 * 
 * Given registered #mesh channel
 * When looking up by name or key
 * Then correct mapping returned
 */
void test_mesh_channel_bidirectional_lookup(void) {
    ChannelRegistry registry;
    registry.registerDefaultMeshChannel();
    
    // Lookup by name -> key
    const uint8_t* key = registry.lookupByName("mesh");
    TEST_ASSERT_NOT_NULL(key);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(EXPECTED_MESH_KEY, key, 32);
    
    // Lookup by key -> name
    const char* name = registry.lookupByKey(EXPECTED_MESH_KEY);
    TEST_ASSERT_NOT_NULL(name);
    TEST_ASSERT_EQUAL_STRING("mesh", name);
}

/**
 * Scenario: Multiple auto-registrations are idempotent
 * 
 * Given #mesh already registered
 * When registering again
 * Then updates rather than duplicates
 */
void test_mesh_registration_idempotent(void) {
    ChannelRegistry registry;
    
    // Register once
    TEST_ASSERT_TRUE(registry.registerDefaultMeshChannel());
    TEST_ASSERT_EQUAL(1, registry.getChannelCount());
    
    // Register again (should update, not duplicate)
    TEST_ASSERT_TRUE(registry.registerDefaultMeshChannel());
    TEST_ASSERT_EQUAL(1, registry.getChannelCount());  // Still 1
}

} // namespace bitchat
} // namespace test

#else // !ENABLE_BITCHAT

namespace test {
namespace bitchat {
void test_mesh_auto_registration_placeholder(void) {
    TEST_IGNORE_MESSAGE("BitChat disabled - ENABLE_BITCHAT not defined");
}
} // namespace bitchat
} // namespace test

#endif // ENABLE_BITCHAT
