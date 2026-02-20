/**
 * Story 2.2: CLI Commands for BitChat Control - BDD Tests
 * 
 * Feature: Runtime BitChat Control via CLI
 */

#include <unity.h>
#include "helpers/bitchat/BitchatConfig.h"
#include "helpers/bitchat/BitchatCLI.h"

#ifdef NATIVE_TEST
  #include <cstdio>
  #include <cstdlib>
  #include <sys/stat.h>
  #include <unistd.h>
  #include <string>
  #include <vector>
#endif

using namespace mesh::bitchat;

namespace test {
namespace bitchat {

#ifdef NATIVE_TEST
// Use cleanupTestConfig() from test_bitchat_config.cpp
extern void cleanupTestConfig();

// Capture CLI output
std::vector<std::string> g_cliOutput;

void captureOutput(const char* text) {
    g_cliOutput.push_back(std::string(text));
}

void clearCapturedOutput() {
    g_cliOutput.clear();
}

bool outputContains(const char* text) {
    std::string search(text);
    for (const auto& line : g_cliOutput) {
        if (line.find(search) != std::string::npos) {
            return true;
        }
    }
    return false;
}
#endif

/**
 * Scenario: Enable BitChat via CLI
 * 
 * Given device has CLI interface
 * When user types "bitchat enable"
 * Then BitChat should be enabled in config
 * And message should display
 */
void test_cli_enable_bitchat(void) {
#ifdef NATIVE_TEST
    cleanupTestConfig();
    clearCapturedOutput();
#endif
    
    BitchatConfigManager config;
    TEST_ASSERT_TRUE(config.begin());
    TEST_ASSERT_FALSE(config.isEnabled());
    
    BitchatCLI cli;
    cli.begin(&config);
#ifdef NATIVE_TEST
    cli.setOutputCallback(captureOutput);
#endif
    
    // When: User types "bitchat enable"
    bool handled = cli.processCommand("bitchat enable");
    
    // Then: Command handled
    TEST_ASSERT_TRUE(handled);
    
    // And: Config enabled
    TEST_ASSERT_TRUE(config.isEnabled());
    
#ifdef NATIVE_TEST
    // And: Success message displayed
    TEST_ASSERT_TRUE(outputContains("enabled"));
#endif
    
#ifdef NATIVE_TEST
    cleanupTestConfig();
#endif
}

/**
 * Scenario: Disable BitChat via CLI
 * 
 * Given BitChat is enabled
 * When user types "bitchat disable"
 * Then BitChat should be disabled in config
 * And message should display
 */
void test_cli_disable_bitchat(void) {
#ifdef NATIVE_TEST
    cleanupTestConfig();
    clearCapturedOutput();
#endif
    
    BitchatConfigManager config;
    TEST_ASSERT_TRUE(config.begin());
    
    // Setup: Enable first
    config.setEnabled(true);
    config.save();
    TEST_ASSERT_TRUE(config.isEnabled());
    
    BitchatCLI cli;
    cli.begin(&config);
#ifdef NATIVE_TEST
    cli.setOutputCallback(captureOutput);
#endif
    
    // When: User types "bitchat disable"
    bool handled = cli.processCommand("bitchat disable");
    
    // Then: Command handled
    TEST_ASSERT_TRUE(handled);
    
    // And: Config disabled
    TEST_ASSERT_FALSE(config.isEnabled());
    
#ifdef NATIVE_TEST
    TEST_ASSERT_TRUE(outputContains("disabled"));
#endif
    
#ifdef NATIVE_TEST
    cleanupTestConfig();
#endif
}

/**
 * Scenario: Check BitChat status via CLI
 * 
 * Given CLI is ready
 * When user types "bitchat status"
 * Then current status should display
 */
void test_cli_status_bitchat(void) {
#ifdef NATIVE_TEST
    cleanupTestConfig();
    clearCapturedOutput();
#endif
    
    BitchatConfigManager config;
    TEST_ASSERT_TRUE(config.begin());
    
    BitchatCLI cli;
    cli.begin(&config);
#ifdef NATIVE_TEST
    cli.setOutputCallback(captureOutput);
#endif
    
    // When: User types "bitchat status"
    bool handled = cli.processCommand("bitchat status");
    
    // Then: Command handled
    TEST_ASSERT_TRUE(handled);
    
#ifdef NATIVE_TEST
    // And: Status displayed
    TEST_ASSERT_TRUE(outputContains("Status:"));
    TEST_ASSERT_TRUE(outputContains("DISABLED") || outputContains("ENABLED"));
#endif
    
#ifdef NATIVE_TEST
    cleanupTestConfig();
#endif
}

/**
 * Scenario: Show BitChat help
 * 
 * Given CLI is ready
 * When user types "bitchat help"
 * Then help text should display
 */
void test_cli_help_bitchat(void) {
#ifdef NATIVE_TEST
    cleanupTestConfig();
    clearCapturedOutput();
#endif
    
    BitchatConfigManager config;
    TEST_ASSERT_TRUE(config.begin());
    
    BitchatCLI cli;
    cli.begin(&config);
#ifdef NATIVE_TEST
    cli.setOutputCallback(captureOutput);
#endif
    
    // When: User types "bitchat help"
    bool handled = cli.processCommand("bitchat help");
    
    // Then: Command handled
    TEST_ASSERT_TRUE(handled);
    
#ifdef NATIVE_TEST
    // And: Help displayed
    TEST_ASSERT_TRUE(outputContains("BitChat Commands"));
    TEST_ASSERT_TRUE(outputContains("enable"));
    TEST_ASSERT_TRUE(outputContains("disable"));
    TEST_ASSERT_TRUE(outputContains("status"));
#endif
    
#ifdef NATIVE_TEST
    cleanupTestConfig();
#endif
}

/**
 * Scenario: Non-bitchat command is not handled
 * 
 * Given CLI is ready
 * When user types "mesh status"
 * Then command should not be handled by bitchat CLI
 */
void test_cli_non_bitchat_command_not_handled(void) {
    BitchatConfigManager config;
    TEST_ASSERT_TRUE(config.begin());
    
    BitchatCLI cli;
    cli.begin(&config);
    
    // When: User types non-bitchat command
    bool handled = cli.processCommand("mesh status");
    
    // Then: Not handled
    TEST_ASSERT_FALSE(handled);
}

/**
 * Scenario: Unknown bitchat subcommand shows error
 * 
 * Given CLI is ready
 * When user types "bitchat unknown"
 * Then error message should display
 */
void test_cli_unknown_subcommand_shows_error(void) {
#ifdef NATIVE_TEST
    cleanupTestConfig();
    clearCapturedOutput();
#endif
    
    BitchatConfigManager config;
    TEST_ASSERT_TRUE(config.begin());
    
    BitchatCLI cli;
    cli.begin(&config);
#ifdef NATIVE_TEST
    cli.setOutputCallback(captureOutput);
#endif
    
    // When: User types unknown command
    bool handled = cli.processCommand("bitchat unknown");
    
    // Then: Command handled (but shows error)
    TEST_ASSERT_TRUE(handled);
    
#ifdef NATIVE_TEST
    // And: Error displayed
    TEST_ASSERT_TRUE(outputContains("Unknown"));
#endif
    
#ifdef NATIVE_TEST
    cleanupTestConfig();
#endif
}

/**
 * Scenario: Command detection works correctly
 * 
 * Given various command strings
 * When checking if they are bitchat commands
 * Then only bitchat commands return true
 */
void test_cli_command_detection(void) {
    // Should be detected as bitchat commands
    TEST_ASSERT_TRUE(BitchatCLI::isBitchatCommand("bitchat"));
    TEST_ASSERT_TRUE(BitchatCLI::isBitchatCommand("bitchat enable"));
    TEST_ASSERT_TRUE(BitchatCLI::isBitchatCommand("bitchat disable"));
    TEST_ASSERT_TRUE(BitchatCLI::isBitchatCommand("  bitchat status"));  // Leading whitespace
    
    // Should NOT be detected as bitchat commands
    TEST_ASSERT_FALSE(BitchatCLI::isBitchatCommand("mesh status"));
    TEST_ASSERT_FALSE(BitchatCLI::isBitchatCommand("help"));
    TEST_ASSERT_FALSE(BitchatCLI::isBitchatCommand(""));
    TEST_ASSERT_FALSE(BitchatCLI::isBitchatCommand("bitchatcustom"));  // Not a prefix match
    TEST_ASSERT_FALSE(BitchatCLI::isBitchatCommand(nullptr));
}

/**
 * Scenario: Enable when already enabled shows info message
 * 
 * Given BitChat is already enabled
 * When user types "bitchat enable"
 * Then info message should display
 */
void test_cli_enable_when_already_enabled(void) {
#ifdef NATIVE_TEST
    cleanupTestConfig();
    clearCapturedOutput();
#endif
    
    BitchatConfigManager config;
    TEST_ASSERT_TRUE(config.begin());
    
    // Setup: Enable first
    config.setEnabled(true);
    config.save();
    
    BitchatCLI cli;
    cli.begin(&config);
#ifdef NATIVE_TEST
    cli.setOutputCallback(captureOutput);
#endif
    
    // When: User types "bitchat enable" again
    bool handled = cli.processCommand("bitchat enable");
    
    // Then: Command handled
    TEST_ASSERT_TRUE(handled);
    
#ifdef NATIVE_TEST
    // And: Already enabled message
    TEST_ASSERT_TRUE(outputContains("already enabled"));
#endif
    
#ifdef NATIVE_TEST
    cleanupTestConfig();
#endif
}

} // namespace bitchat
} // namespace test
