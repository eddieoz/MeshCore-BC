#pragma once

/**
 * Story 2.2: CLI Commands for BitChat Control
 * Story 4.3: Dynamic Channel Creation
 * 
 * Runtime BitChat control via serial CLI
 * - bitchat enable/disable/status commands
 * - bitchat channel add/remove/list commands
 * - Integrates with existing MeshCore CLI
 * 
 * Story 2.3: Compile-time control via ENABLE_BITCHAT flag
 */

#ifdef ENABLE_BITCHAT

#include <functional>
#include <cstring>

namespace mesh {
namespace bitchat {

// Forward declarations
class BitchatConfigManager;
class ChannelRegistry;

/**
 * CLI Output callback - called to output text to user
 */
using CLIOutputCallback = std::function<void(const char* text)>;

/**
 * BitChat CLI Command Handler
 * 
 * Handles CLI commands for BitChat control:
 * - bitchat enable  - Enable BitChat service
 * - bitchat disable - Disable BitChat service  
 * - bitchat status  - Show current status
 * - bitchat help    - Show help
 */
class BitchatCLI {
public:
    BitchatCLI();
    
    // Initialize CLI handler
    void begin(BitchatConfigManager* configManager, ChannelRegistry* channelRegistry = nullptr);
    
    // Set output callback for CLI responses
    void setOutputCallback(CLIOutputCallback callback);
    
    // Get channel registry (for external access)
    ChannelRegistry* getChannelRegistry() const { return _channels; }
    
    // Process a CLI command line
    // Returns true if command was handled, false if not a bitchat command
    bool processCommand(const char* cmdLine);
    
    // Check if a command starts with "bitchat"
    static bool isBitchatCommand(const char* cmdLine);
    
    // Get help text
    static const char* getHelpText();
    
private:
    BitchatConfigManager* _config;
    ChannelRegistry* _channels;
    CLIOutputCallback _output;
    
    void print(const char* text);
    void println(const char* text);
    
    // Command handlers
    void cmdEnable();
    void cmdDisable();
    void cmdStatus();
    void cmdHelp();
    void cmdChannel(const char* subcmd);
    
    // Channel sub-commands
    void cmdChannelAdd(const char* args);
    void cmdChannelRemove(const char* name);
    void cmdChannelList();
    void cmdChannelHelp();
};

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
