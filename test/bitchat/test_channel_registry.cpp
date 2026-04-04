/**
 * Story 4.1: Channel Registry System - BDD Tests
 * 
 * Feature: Bidirectional Channel Mapping Registry
 */

#include <unity.h>
#include "helpers/bitchat/ChannelRegistry.h"

#ifdef ENABLE_BITCHAT

using namespace mesh::bitchat;

namespace test {
namespace bitchat {

static const uint8_t TEST_KEY[32] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20
};

/**
 * Scenario: Register channel mapping
 * 
 * Given empty registry
 * When registering a channel
 * Then channel is stored
 */
void test_register_channel(void) {
    ChannelRegistry registry;
    
    TEST_ASSERT_EQUAL(0, registry.getChannelCount());
    
    // When: Register channel
    bool registered = registry.registerChannel("general", TEST_KEY);
    
    // Then: Registered
    TEST_ASSERT_TRUE(registered);
    TEST_ASSERT_EQUAL(1, registry.getChannelCount());
}

/**
 * Scenario: Lookup by name
 * 
 * Given registered channel
 * When looking up by name
 * Then returns channel key
 */
void test_lookup_by_name(void) {
    ChannelRegistry registry;
    registry.registerChannel("general", TEST_KEY);
    
    // When: Lookup by name
    const uint8_t* key = registry.lookupByName("general");
    
    // Then: Returns key
    TEST_ASSERT_NOT_NULL(key);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(TEST_KEY, key, 32);
}

/**
 * Scenario: Lookup by key
 * 
 * Given registered channel
 * When looking up by key
 * Then returns channel name
 */
void test_lookup_by_key(void) {
    ChannelRegistry registry;
    registry.registerChannel("general", TEST_KEY);
    
    // When: Lookup by key
    const char* name = registry.lookupByKey(TEST_KEY);
    
    // Then: Returns name
    TEST_ASSERT_NOT_NULL(name);
    TEST_ASSERT_EQUAL_STRING("general", name);
}

/**
 * Scenario: Unregister channel
 * 
 * Given registered channel
 * When unregistering
 * Then channel removed
 */
void test_unregister_channel(void) {
    ChannelRegistry registry;
    registry.registerChannel("general", TEST_KEY);
    TEST_ASSERT_EQUAL(1, registry.getChannelCount());
    
    // When: Unregister
    bool unregistered = registry.unregisterChannel("general");
    
    // Then: Removed
    TEST_ASSERT_TRUE(unregistered);
    TEST_ASSERT_EQUAL(0, registry.getChannelCount());
    TEST_ASSERT_NULL(registry.lookupByName("general"));
}

/**
 * Scenario: Update existing channel
 * 
 * Given registered channel
 * When registering same name with new key
 * Then key is updated
 */
void test_update_existing_channel(void) {
    ChannelRegistry registry;
    registry.registerChannel("general", TEST_KEY);
    
    uint8_t newKey[32];
    memset(newKey, 0xAA, 32);
    
    // When: Register same name with different key
    bool updated = registry.registerChannel("general", newKey);
    
    // Then: Updated
    TEST_ASSERT_TRUE(updated);
    TEST_ASSERT_EQUAL(1, registry.getChannelCount());
    
    const uint8_t* key = registry.lookupByName("general");
    TEST_ASSERT_EQUAL_HEX8(0xAA, key[0]);
}

/**
 * Scenario: Registry enforces max size
 * 
 * Given full registry (8 channels)
 * When registering 9th channel
 * Then fails
 */
void test_registry_max_size(void) {
    ChannelRegistry registry;
    
    // Fill registry
    for (int i = 0; i < BITCHAT_MAX_CHANNEL_MAPPINGS; i++) {
        char name[16];
        snprintf(name, sizeof(name), "ch%d", i);
        
        uint8_t key[32];
        memset(key, i, 32);
        
        TEST_ASSERT_TRUE(registry.registerChannel(name, key));
    }
    
    TEST_ASSERT_EQUAL(BITCHAT_MAX_CHANNEL_MAPPINGS, registry.getChannelCount());
    TEST_ASSERT_TRUE(registry.isFull());
    
    // Try to add 9th
    uint8_t key9[32];
    memset(key9, 9, 32);
    bool added = registry.registerChannel("ch9", key9);
    
    TEST_ASSERT_FALSE(added);
    TEST_ASSERT_EQUAL(BITCHAT_MAX_CHANNEL_MAPPINGS, registry.getChannelCount());
}

/**
 * Scenario: Derive key from hashtag
 * 
 * Given a hashtag string
 * When deriving key
 * Then returns SHA256 hash
 */
void test_derive_key_from_hashtag(void) {
    uint8_t key[32];
    
    // When: Derive key from #mesh
    bool derived = ChannelRegistry::deriveKeyFromHashtag("#mesh", key);
    
    // Then: Derived successfully
    TEST_ASSERT_TRUE(derived);
    
    // Same hashtag should produce same key
    uint8_t key2[32];
    ChannelRegistry::deriveKeyFromHashtag("#mesh", key2);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(key, key2, 32);
    
    // Different hashtag should produce different key
    uint8_t key3[32];
    ChannelRegistry::deriveKeyFromHashtag("#general", key3);
    TEST_ASSERT_TRUE(memcmp(key, key3, 32) != 0);
}

/**
 * Scenario: Register default #mesh channel
 * 
 * Given empty registry
 * When registering default mesh channel
 * Then #mesh channel added
 */
void test_register_default_mesh_channel(void) {
    ChannelRegistry registry;
    
    // When: Register default mesh channel
    bool registered = registry.registerDefaultMeshChannel();
    
    // Then: Added
    TEST_ASSERT_TRUE(registered);
    TEST_ASSERT_EQUAL(1, registry.getChannelCount());
    TEST_ASSERT_TRUE(registry.hasChannel("mesh"));
    
    // Verify it's a hashtag-derived key
    const uint8_t* key = registry.lookupByName("mesh");
    TEST_ASSERT_NOT_NULL(key);
}

/**
 * Scenario: Clear registry
 * 
 * Given registry with channels
 * When clearing
 * Then all channels removed
 */
void test_clear_registry(void) {
    ChannelRegistry registry;
    registry.registerChannel("ch1", TEST_KEY);
    registry.registerChannel("ch2", TEST_KEY);
    
    TEST_ASSERT_EQUAL(2, registry.getChannelCount());
    
    // When: Clear
    registry.clear();
    
    // Then: Empty
    TEST_ASSERT_EQUAL(0, registry.getChannelCount());
    TEST_ASSERT_FALSE(registry.hasChannel("ch1"));
}

/**
 * Scenario: Get channel by index
 * 
 * Given registry with channels
 * When getting by index
 * Then returns channel or null
 */
void test_get_channel_by_index(void) {
    ChannelRegistry registry;
    registry.registerChannel("general", TEST_KEY);
    
    // Valid index
    const ChannelMapping* ch = registry.getChannel(0);
    TEST_ASSERT_NOT_NULL(ch);
    TEST_ASSERT_EQUAL_STRING("general", ch->name);
    
    // Invalid index
    TEST_ASSERT_NULL(registry.getChannel(100));
}

/**
 * Scenario: Null parameters handled gracefully
 * 
 * Given registry
 * When calling with null
 * Then handles gracefully
 */
void test_null_parameters_registry(void) {
    ChannelRegistry registry;
    
    // Null name
    TEST_ASSERT_FALSE(registry.registerChannel(nullptr, TEST_KEY));
    
    // Null key
    TEST_ASSERT_FALSE(registry.registerChannel("test", nullptr));
    
    // Null lookup
    TEST_ASSERT_NULL(registry.lookupByName(nullptr));
    TEST_ASSERT_NULL(registry.lookupByKey(nullptr));
    
    TEST_ASSERT_EQUAL(0, registry.getChannelCount());
}

/**
 * Scenario: Find non-existent channel
 * 
 * Given empty registry
 * When searching
 * Then returns null
 */
void test_find_nonexistent_channel(void) {
    ChannelRegistry registry;
    
    TEST_ASSERT_NULL(registry.lookupByName("nonexistent"));
    
    uint8_t fakeKey[32];
    memset(fakeKey, 0xFF, 32);
    TEST_ASSERT_NULL(registry.lookupByKey(fakeKey));
}

} // namespace bitchat
} // namespace test

#else // !ENABLE_BITCHAT

namespace test {
namespace bitchat {
void test_registry_placeholder(void) {
    TEST_IGNORE_MESSAGE("BitChat disabled - ENABLE_BITCHAT not defined");
}
} // namespace bitchat
} // namespace test

#endif // ENABLE_BITCHAT
