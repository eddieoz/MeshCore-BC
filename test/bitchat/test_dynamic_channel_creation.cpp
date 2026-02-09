/**
 * Story 4.3: Dynamic Channel Creation - BDD Tests
 * 
 * Feature: Runtime Channel Registration
 */

#include <unity.h>
#include "helpers/bitchat/BitchatConfig.h"
#include "helpers/bitchat/BitchatCLI.h"
#include "helpers/bitchat/ChannelRegistry.h"

#ifdef ENABLE_BITCHAT

using namespace mesh::bitchat;

namespace test {
namespace bitchat {

// Test hex key: 32 bytes = 64 hex chars
static const char* DYN_TEST_HEX_KEY = "a1b2c3d4e5f6789012345678901234567890abcdef1234567890abcdef123456";
static const uint8_t DYN_TEST_KEY[32] = {
    0xa1, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6, 0x78, 0x90,
    0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56,
    0x78, 0x90, 0xab, 0xcd, 0xef, 0x12, 0x34, 0x56,
    0x78, 0x90, 0xab, 0xcd, 0xef, 0x12, 0x34, 0x56
};

static BitchatConfigManager g_config;
static ChannelRegistry g_channels;

// Reset function called from test_main.cpp setUp
void resetChannelTestState() {
    g_config.begin();
    g_channels.clear();
}

/**
 * Scenario: User adds new channel mapping via CLI
 * 
 * Given BitChat CLI is ready
 * When user runs "bitchat channel add emergency <key>"
 * Then new mapping created
 */
void test_cli_channel_add(void) {
    BitchatCLI cli;
    cli.begin(&g_config, &g_channels);
    
    TEST_ASSERT_EQUAL(0, g_channels.getChannelCount());
    
    // When: Add channel via CLI
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "bitchat channel add emergency %s", DYN_TEST_HEX_KEY);
    bool handled = cli.processCommand(cmd);
    
    // Then: Command handled
    TEST_ASSERT_TRUE(handled);
    
    // And: Channel registered
    TEST_ASSERT_EQUAL(1, g_channels.getChannelCount());
    TEST_ASSERT_TRUE(g_channels.hasChannel("emergency"));
    
    // And: Key correct
    const uint8_t* key = g_channels.lookupByName("emergency");
    TEST_ASSERT_NOT_NULL(key);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(DYN_TEST_KEY, key, 32);
}

/**
 * Scenario: User removes channel mapping via CLI
 * 
 * Given channel exists
 * When user runs "bitchat channel remove <name>"
 * Then mapping removed
 */
void test_cli_channel_remove(void) {
    // Setup: Add channel first
    g_channels.registerChannel("testch", DYN_TEST_KEY);
    TEST_ASSERT_EQUAL(1, g_channels.getChannelCount());
    
    BitchatCLI cli;
    cli.begin(&g_config, &g_channels);
    
    // When: Remove via CLI
    bool handled = cli.processCommand("bitchat channel remove testch");
    
    // Then: Command handled
    TEST_ASSERT_TRUE(handled);
    
    // And: Channel removed
    TEST_ASSERT_EQUAL(0, g_channels.getChannelCount());
    TEST_ASSERT_FALSE(g_channels.hasChannel("testch"));
}

/**
 * Scenario: User lists channels via CLI
 * 
 * Given channels exist
 * When user runs "bitchat channel list"
 * Then channels displayed
 */
void test_cli_channel_list(void) {
    // Setup: Add channels
    g_channels.registerChannel("ch1", DYN_TEST_KEY);
    g_channels.registerChannel("ch2", DYN_TEST_KEY);
    
    BitchatCLI cli;
    cli.begin(&g_config, &g_channels);
    
    // When: List via CLI
    bool handled = cli.processCommand("bitchat channel list");
    
    // Then: Command handled
    TEST_ASSERT_TRUE(handled);
}

/**
 * Scenario: Channel add with invalid key fails
 * 
 * Given CLI ready
 * When user runs "bitchat channel add test shortkey"
 * Then error displayed
 */
void test_cli_channel_add_invalid_key(void) {
    BitchatCLI cli;
    cli.begin(&g_config, &g_channels);
    
    TEST_ASSERT_EQUAL(0, g_channels.getChannelCount());
    
    // When: Add with invalid key (too short)
    bool handled = cli.processCommand("bitchat channel add test short");
    
    // Then: Command handled but error
    TEST_ASSERT_TRUE(handled);
    
    // And: No channel added
    TEST_ASSERT_EQUAL(0, g_channels.getChannelCount());
}

/**
 * Scenario: Channel command without subcommand shows list
 * 
 * Given CLI ready
 * When user runs "bitchat channel"
 * Then list displayed
 */
void test_cli_channel_default_shows_list(void) {
    BitchatCLI cli;
    cli.begin(&g_config, &g_channels);
    
    // When: Channel command without subcommand
    bool handled = cli.processCommand("bitchat channel");
    
    // Then: Shows list (default behavior)
    TEST_ASSERT_TRUE(handled);
}

/**
 * Scenario: Channel help command
 * 
 * Given CLI ready
 * When user runs "bitchat channel help"
 * Then help displayed
 */
void test_cli_channel_help(void) {
    BitchatCLI cli;
    cli.begin(&g_config, &g_channels);
    
    // When: Help command
    bool handled = cli.processCommand("bitchat channel help");
    
    // Then: Command handled
    TEST_ASSERT_TRUE(handled);
}

} // namespace bitchat
} // namespace test

#else // !ENABLE_BITCHAT

namespace test {
namespace bitchat {
void test_dynamic_channel_placeholder(void) {
    TEST_IGNORE_MESSAGE("BitChat disabled - ENABLE_BITCHAT not defined");
}
} // namespace bitchat
} // namespace test

#endif // ENABLE_BITCHAT
