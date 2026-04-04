/**
 * Story 2.1: Configuration Storage - BDD Tests
 * 
 * Feature: BitChat Configuration Persistence
 */

#include <unity.h>
#include "helpers/bitchat/BitchatConfig.h"

#ifdef NATIVE_TEST
  #include <cstdio>
  #include <cstdlib>
  #include <sys/stat.h>
  #include <unistd.h>
#endif

using namespace mesh::bitchat;

namespace test {
namespace bitchat {

#ifdef NATIVE_TEST
static const char* TEST_CONFIG_FILE = "/tmp/meshcore_test/bitchat.cfg";
static const char* TEST_CONFIG_DIR = "/tmp/meshcore_test";

void cleanupTestConfig() {
    remove(TEST_CONFIG_FILE);
    rmdir(TEST_CONFIG_DIR);
}
#endif

/**
 * Scenario: Config defaults to disabled
 * 
 * Given fresh config manager
 * When initialized without existing config
 * Then BitChat should be disabled by default
 * And config should be valid
 */
void test_config_defaults_to_disabled(void) {
#ifdef NATIVE_TEST
    cleanupTestConfig();
#endif
    
    BitchatConfigManager config;
    
    // When: Initialized
    TEST_ASSERT_TRUE(config.begin());
    
    // Then: Disabled by default
    TEST_ASSERT_FALSE(config.isEnabled());
    
    // And: Config is valid
    TEST_ASSERT_TRUE(config.getConfig().isValid());
    TEST_ASSERT_EQUAL(BITCHAT_CONFIG_MAGIC, config.getConfig().magic);
    TEST_ASSERT_EQUAL(BITCHAT_CONFIG_VERSION, config.getConfig().version);
    
#ifdef NATIVE_TEST
    cleanupTestConfig();
#endif
}

/**
 * Scenario: Enable BitChat and save
 * 
 * Given config manager is initialized
 * When user enables BitChat
 * And saves config
 * Then config should persist enabled state
 */
void test_enable_and_save_config(void) {
#ifdef NATIVE_TEST
    cleanupTestConfig();
#endif
    
    {
        BitchatConfigManager config;
        TEST_ASSERT_TRUE(config.begin());
        
        // Given: Initially disabled
        TEST_ASSERT_FALSE(config.isEnabled());
        
        // When: Enable and save
        config.setEnabled(true);
        TEST_ASSERT_TRUE(config.save());
        
        // Then: Enabled in memory
        TEST_ASSERT_TRUE(config.isEnabled());
    }
    
    // Verify persistence by creating new manager instance
    {
        BitchatConfigManager config;
        TEST_ASSERT_TRUE(config.begin());
        
        // Then: Config loaded with enabled state
        TEST_ASSERT_TRUE(config.isEnabled());
    }
    
#ifdef NATIVE_TEST
    cleanupTestConfig();
#endif
}

/**
 * Scenario: Disable BitChat and save
 * 
 * Given BitChat is enabled
 * When user disables BitChat
 * And saves config
 * Then config should persist disabled state
 */
void test_disable_and_save_config(void) {
#ifdef NATIVE_TEST
    cleanupTestConfig();
#endif
    
    // Setup: Enable first
    {
        BitchatConfigManager config;
        TEST_ASSERT_TRUE(config.begin());
        config.setEnabled(true);
        config.save();
    }
    
    // Test: Disable
    {
        BitchatConfigManager config;
        TEST_ASSERT_TRUE(config.begin());
        TEST_ASSERT_TRUE(config.isEnabled());  // Still enabled
        
        // When: Disable and save
        config.setEnabled(false);
        TEST_ASSERT_TRUE(config.save());
    }
    
    // Verify
    {
        BitchatConfigManager config;
        TEST_ASSERT_TRUE(config.begin());
        TEST_ASSERT_FALSE(config.isEnabled());  // Now disabled
    }
    
#ifdef NATIVE_TEST
    cleanupTestConfig();
#endif
}

/**
 * Scenario: Config persists across reboots
 * 
 * Given config is saved with specific state
 * When device reboots (new config manager instance)
 * Then state should be restored
 */
void test_config_persists_across_reboots(void) {
#ifdef NATIVE_TEST
    cleanupTestConfig();
#endif
    
    // First "boot" - enable and save
    {
        BitchatConfigManager config;
        TEST_ASSERT_TRUE(config.begin());
        
        config.setEnabled(true);
        TEST_ASSERT_TRUE(config.save());
        // Timestamp captured after save to match what was written
    }
    
    // Second "boot" - verify restored
    {
        BitchatConfigManager config;
        TEST_ASSERT_TRUE(config.begin());
        
        TEST_ASSERT_TRUE(config.isEnabled());
        // Just verify timestamp was saved (non-zero)
        TEST_ASSERT_TRUE(config.getLastModified() > 0);
    }
    
#ifdef NATIVE_TEST
    cleanupTestConfig();
#endif
}

/**
 * Scenario: Invalid config file is handled gracefully
 * 
 * Given config file is corrupted
 * When config manager initializes
 * Then it should use defaults
 * And save valid config
 */
void test_invalid_config_handled_gracefully(void) {
#ifdef NATIVE_TEST
    cleanupTestConfig();
    
    // Create invalid config file
    mkdir(TEST_CONFIG_DIR, 0755);
    FILE* fp = fopen(TEST_CONFIG_FILE, "wb");
    if (fp) {
        // Write garbage data
        uint8_t garbage[64];
        memset(garbage, 0xFF, sizeof(garbage));
        fwrite(garbage, 1, sizeof(garbage), fp);
        fclose(fp);
    }
#endif
    
    BitchatConfigManager config;
    TEST_ASSERT_TRUE(config.begin());
    
    // Should use defaults (disabled)
    TEST_ASSERT_FALSE(config.isEnabled());
    
    // Config should now be valid
    TEST_ASSERT_TRUE(config.getConfig().isValid());
    
#ifdef NATIVE_TEST
    cleanupTestConfig();
#endif
}

/**
 * Scenario: Reset config to defaults
 * 
 * Given config has custom values
 * When reset is called
 * Then config returns to defaults
 */
void test_reset_config_to_defaults(void) {
#ifdef NATIVE_TEST
    cleanupTestConfig();
#endif
    
    BitchatConfigManager config;
    TEST_ASSERT_TRUE(config.begin());
    
    // Setup: Enable and modify
    config.setEnabled(true);
    config.save();
    TEST_ASSERT_TRUE(config.isEnabled());
    
    uint32_t oldTimestamp = config.getLastModified();
    
    // When: Reset
    config.reset();
    
    // Then: Back to defaults
    TEST_ASSERT_FALSE(config.isEnabled());
    TEST_ASSERT_NOT_EQUAL(oldTimestamp, config.getLastModified());
    
    // And: Config still valid
    TEST_ASSERT_TRUE(config.getConfig().isValid());
    
#ifdef NATIVE_TEST
    cleanupTestConfig();
#endif
}

/**
 * Scenario: Timestamp updated on save
 * 
 * Given config is saved
 * When saved again later
 * Then timestamp should be updated
 */
void test_timestamp_updated_on_save(void) {
#ifdef NATIVE_TEST
    cleanupTestConfig();
#endif
    
    BitchatConfigManager config;
    TEST_ASSERT_TRUE(config.begin());
    
    // First save
    config.setEnabled(true);
    TEST_ASSERT_TRUE(config.save());
    uint32_t timestamp1 = config.getLastModified();
    
    // Wait a bit (for native test timestamp to increment)
    #ifdef NATIVE_TEST
    // Native test uses incrementing counter
    #else
    delay(1100);  // Wait > 1 second for timestamp to change
    #endif
    
    // Second save
    config.setEnabled(false);
    TEST_ASSERT_TRUE(config.save());
    uint32_t timestamp2 = config.getLastModified();
    
    // Then: Timestamp updated
    TEST_ASSERT_NOT_EQUAL(timestamp1, timestamp2);
    
#ifdef NATIVE_TEST
    cleanupTestConfig();
#endif
}

} // namespace bitchat
} // namespace test
